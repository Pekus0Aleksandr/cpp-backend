#include "sdk.h"
//
#include <boost/asio/signal_set.hpp>
#include <iostream>
#include <mutex>
#include <thread>
#include <vector>

#include "http_server.h"

namespace {
    namespace net = boost::asio;
    using namespace std::literals;
    namespace sys = boost::system;
    namespace http = boost::beast::http;

    // Запрос, тело которого представлено в виде строки
    using StringRequest = http::request<http::string_body>;
    // Ответ, тело которого представлено в виде строки
    using StringResponse = http::response<http::string_body>;

    struct ContentType {
        ContentType() = delete;
        constexpr static std::string_view TEXT_HTML = "text/html"sv;
        // При необходимости внутрь ContentType можно добавить и другие типы контента
    };

    // Создаёт StringResponse с заданными параметрами
    StringResponse MakeStringResponse(http::status status, std::string_view body, unsigned http_version,
        bool keep_alive, http::verb type,
        std::string_view content_type = ContentType::TEXT_HTML) {
        StringResponse response(status, http_version);
        response.set(http::field::content_type, content_type);
        if (type != http::verb::head)
            response.body() = body;
        if (status != http::status::ok)
            response.set(http::field::allow, "GET, HEAD"sv);
        response.content_length(body.size());
        response.keep_alive(keep_alive);
        return response;
    }

    StringResponse HandleRequest(StringRequest&& req) {
        const auto text_response = [&req](http::status status, std::string_view text) {
            return MakeStringResponse(status, text, req.version(), req.keep_alive(), req.method());
            };
        if (req.method() == http::verb::get || req.method() == http::verb::head)
        {
            std::string str = "Hello"; //</strong>
            if (req.target().substr(1).size() > 0)
            {
                str.append(", ");
                str.append(req.target().substr(1));
            }
            // str.append("</strong>");
            std::string_view sv_str(str);
            // Здесь можно обработать запрос и сформировать ответ, но пока всегда отвечаем: Hello
            return text_response(http::status::ok, sv_str/*"<strong>Hello</strong>"sv*/);
        }
        else
        {
            return text_response(http::status::method_not_allowed, "Invalid method"sv);
        }

    }

    // Запускает функцию fn на n потоках, включая текущий
    template <typename Fn>
    void RunWorkers(unsigned n, const Fn& fn) {
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

int main() {
    const unsigned num_threads = std::thread::hardware_concurrency();
    std::cout << "num_threads:" << num_threads << std::endl;

    net::io_context ioc(num_threads);

    // Подписываемся на сигналы и при их получении завершаем работу сервера
    net::signal_set signals(ioc, SIGINT, SIGTERM);
    signals.async_wait([&ioc](const sys::error_code& ec, [[maybe_unused]] int signal_number) {
        if (!ec) {
            ioc.stop();
        }
        });

    const auto address = net::ip::make_address("0.0.0.0");
    constexpr net::ip::port_type port = 8080;
    http_server::ServeHttp(ioc, { address, port }, [](auto&& req, auto&& sender) {
        sender(HandleRequest(std::forward<decltype(req)>(req)));
        });

    // Эта надпись сообщает тестам о том, что сервер запущен и готов обрабатывать запросы
    std::cout << "Server has started..."sv << std::endl;

    RunWorkers(num_threads, [&ioc] {
        ioc.run();
        });

    std::cout << "Shutting down"sv << std::endl;
}