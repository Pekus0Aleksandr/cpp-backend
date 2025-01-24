#include "sdk.h"
//
#include <boost/asio/io_context.hpp>
#include <boost/asio/signal_set.hpp>
#include <thread>


#include "json_loader.h"
#include "request_handler/request_handler.h"
#include "log.h"

using namespace std::literals;
namespace sys = boost::system;
namespace net = boost::asio;
namespace fs = std::filesystem;

namespace {
// Запускает функцию fn на n потоках, включая текущий
template <typename Fn>
void RunThreads(unsigned n, const Fn& fn) {
    n = std::max(1u, n);
    std::vector<std::jthread> workers;
    workers.reserve(n - 1);
    // Запускаем n-1 рабочих потоков, выполняющих функцию fn
    while (--n) {
        workers.emplace_back(fn);
    }
    fn();
}
}  // namespace

int main(int argc, const char* argv[]) {
    if (argc != 3) {
        std::cerr << "Usage: game_server <game-config-json> <static-files>"sv << std::endl;
        return EXIT_FAILURE;
    }
    server_logging::InitBoostLogFilter();
    try {
        // 1. Загружаем карту из файла и построить модель игры
        model::Game game = json_loader::LoadGame(argv[1]);
        // 1.a Get and check path
        fs::path static_path = argv[2];
        // 2. Инициализируем io_context
        const unsigned num_threads = std::thread::hardware_concurrency();
        net::io_context ioc(num_threads);
        // 3. Добавляем асинхронный обработчик сигналов SIGINT и SIGTERM
        net::signal_set signals(ioc, SIGINT, SIGTERM);
        signals.async_wait([&ioc](const sys::error_code& ec, [[maybe_unused]] int signal_number) {
            if (!ec) {
                LOGSRV().end(ec);
                ioc.stop();
            }
        });
        auto api_strand = net::make_strand(ioc);
        // 4. Создаём обработчик HTTP-запросов и связываем его с моделью игры
        auto handler = std::make_shared<http_handler::RequestHandler>(static_path, api_strand, game);
        // Оборачиваем его в логирующий декоратор
        server_logging::LoggingRequestHandler logging_handler{
            [handler](auto&& endpoint, auto&& req, auto&& send) {
                // Обрабатываем запрос
                (*handler)(std::forward<decltype(endpoint)>(endpoint),
                    std::forward<decltype(req)>(req),
                    std::forward<decltype(send)>(send));
            }
        };
        // 5. Запустить обработчик HTTP-запросов, делегируя их обработчику запросов
        const auto address = net::ip::make_address("0.0.0.0");
        constexpr net::ip::port_type port = 8080;
        // Запускаем обработку запросов
        http_server::ServerHttp(ioc, { address, port }, logging_handler);
        // Эта надпись сообщает тестам о том, что сервер запущен и готов обрабатывать запросы
        LOGSRV().start(address.to_string(), port);
        // 6. Запускаем обработку асинхронных операций
        RunThreads(std::max(1u, num_threads), [&ioc] {
            ioc.run();
        });
    } catch (const std::exception& ex) {
        std::cerr << ex.what() << std::endl;
        return EXIT_FAILURE;
    }
}
