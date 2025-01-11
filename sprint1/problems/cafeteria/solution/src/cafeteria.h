#pragma once
#ifdef _WIN32
#include <sdkddkver.h>
#endif

#include <boost/asio.hpp>
#include <boost/asio/steady_timer.hpp>
#include <boost/asio/strand.hpp>
#include <memory>
#include <chrono>

#include "hotdog.h"
#include "result.h"

namespace net = boost::asio;

// Функция-обработчик операции приготовления хот-дога
using HotDogHandler = std::function<void(Result<HotDog> hot_dog)>;
using Timer = net::steady_timer;
using namespace std::chrono;
using namespace std::literals;

class ThreadChecker {
public:
    explicit ThreadChecker(std::atomic_int& counter)
        : counter_{ counter } {
    }

    ThreadChecker(const ThreadChecker&) = delete;
    ThreadChecker& operator=(const ThreadChecker&) = delete;

    ~ThreadChecker() {
        // assert выстрелит, если между вызовом конструктора и деструктора
        // значение expected_counter_ изменится
        assert(expected_counter_ == counter_);
    }

private:
    std::atomic_int& counter_;
    int expected_counter_ = ++counter_;
};

class Logger {
public:
    explicit Logger(std::string id)
        : id_(std::move(id)) {
    }

    void LogMessage(std::string_view message, int id = -1) const {
        std::osyncstream os{ std::cout };
        os << id_ << "> ["sv << duration<double>(steady_clock::now() - start_time_).count()
            << "s] "sv << "<"sv << std::to_string(id) << ">"sv << message << std::endl;
    }

private:
    std::string id_;
    steady_clock::time_point start_time_{ steady_clock::now() };
};

class Order : public std::enable_shared_from_this<Order> {
public:
    Order(net::io_context& io, int id,
        std::shared_ptr<GasCooker> gas_cooker,
        std::shared_ptr<Bread>   bread,
        std::shared_ptr<Sausage> sausage,
        HotDogHandler handler)
        : io_{ io }
        , id_{ id }
        , gas_cooker_{ gas_cooker }
        , bread_{ bread }
        , sausage_{ sausage }
        , handler_{ std::move(handler) } {
    }

    // Запускает асинхронное выполнение заказа
    void Execute() {
        logger_.LogMessage("Order has been started."sv, id_);
        FryBread();
        FrySausage();
    }
private:
    net::io_context& io_;
    int id_;
    std::shared_ptr<GasCooker> gas_cooker_;
    std::shared_ptr<Bread>   bread_;
    std::shared_ptr<Sausage> sausage_;
    bool delivered_ = false;
    bool sausuge_fried_ = false;
    bool bread_fried_ = false;
    HotDogHandler handler_;
    Logger logger_{ std::to_string(id_) };
    std::atomic_int counter_{ 0 };
    net::strand<net::io_context::executor_type> strand_{ net::make_strand(io_) };

    void FryBread() {
        logger_.LogMessage("Start frying bread"sv, bread_->GetId());
        try {
            bread_->StartBake(*gas_cooker_.get(),
                [self = shared_from_this(), this]() {
                    std::shared_ptr<Timer> bread_frying_timer_{ std::make_shared<Timer>(io_, 1000ms) };
                    bread_frying_timer_->async_wait(
                        // OnSausageFried будет вызван последовательным исполнителем strand_
                        net::bind_executor(strand_, [self = shared_from_this(), timer = bread_frying_timer_](sys::error_code ec) {
                            self->OnBreadFried(ec);
                        }));
                });
        }
        catch (std::exception& ex) {
            logger_.LogMessage(ex.what());
        }
    }

    void OnBreadFried(sys::error_code ec) {
        ThreadChecker checker{ counter_ };
        if (ec) {
            logger_.LogMessage("Fry bread error : "s + ec.what(), bread_->GetId());
        }
        else {
            logger_.LogMessage("Bread has been fried."sv, bread_->GetId());
            try {
                bread_->StopBaking();
            }
            catch (std::exception& ex) {
                logger_.LogMessage(ex.what());
            }
            bread_fried_ = true;
        }
        CheckReadiness(ec);
    }

    void FrySausage() {
        logger_.LogMessage("Start frying sausage"sv, sausage_->GetId());
        sausage_->StartFry(*gas_cooker_.get(),
            [self = shared_from_this(), this]() {
                std::shared_ptr<Timer> sausage_fying_timer_{ std::make_shared<Timer>(io_, 1500ms) };
                sausage_fying_timer_->async_wait(
                    net::bind_executor(strand_, [self = shared_from_this(), timer = sausage_fying_timer_](sys::error_code ec) {
                        self->OnSausageFried(ec);
                        }));
            });
    }

    void OnSausageFried(sys::error_code ec) {
        ThreadChecker checker{ counter_ };
        if (ec) {
            logger_.LogMessage("Fry sausage error: "s + ec.what(), sausage_->GetId());
        }
        else {
            logger_.LogMessage("Sausage has been fried."sv, sausage_->GetId());
            sausage_->StopFry();
            sausuge_fried_ = true;
        }
        CheckReadiness(ec);
    }

    void CheckReadiness(sys::error_code ec) {
        if (delivered_) {
            // Выходим, если заказ уже доставлен либо клиента уведомили об ошибке
            return;
        }
        if (ec) {
            // В случае ошибки уведомляем клиента о невозможности выполнить заказ
            return Deliver(ec);
        }
        // Если все компоненты гамбургера готовы, упаковываем его
        if (IsReadyToPack()) {
            Pack();
        }
    }
    void Deliver(sys::error_code ec) {
        // Защита заказа от повторной доставки
        delivered_ = true;
        if (ec)
            handler_(Result<HotDog>{std::make_exception_ptr(ec)});
        else
            handler_(Result<HotDog>(HotDog(id_, sausage_, bread_)));
    }
    [[nodiscard]] bool IsReadyToPack() const {
        return sausage_->IsCooked() && bread_->IsCooked();
    }

    void Pack() {
        logger_.LogMessage("Packed"sv, id_);

        Deliver({});
    }
};
// Класс "Кафетерий". Готовит хот-доги
class Cafeteria {
public:
    explicit Cafeteria(net::io_context& io)
        : io_{ io } {
    }

    // Асинхронно готовит хот-дог и вызывает handler, как только хот-дог будет готов.
    // Этот метод может быть вызван из произвольного потока
    void OrderHotDog(HotDogHandler handler) {

        net::dispatch(strand_, [this, handler = std::move(handler)]() mutable {
            std::make_shared<Order>(io_, ++next_order_id_, gas_cooker_, store_.GetBread(),
                store_.GetSausage(), std::move(handler))->Execute();
            });

    }

private:
    using Strand = net::strand<net::io_context::executor_type>;
    net::io_context& io_;
    Strand strand_{ net::make_strand(io_) };
    int next_order_id_ = 0;
    // std::mutex mut;
     // Используется для создания ингредиентов хот-дога
    Store store_;
    // Газовая плита. По условию задачи в кафетерии есть только одна газовая плита на 8 горелок
    // Используйте её для приготовления ингредиентов хот-дога.
    // Плита создаётся с помощью make_shared, так как GasCooker унаследован от
    // enable_shared_from_this.
    std::shared_ptr<GasCooker> gas_cooker_ = std::make_shared<GasCooker>(io_);
};