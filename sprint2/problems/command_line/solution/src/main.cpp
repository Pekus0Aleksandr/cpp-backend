#include "sdk.h"
//
#include <boost/asio/io_context.hpp>
#include <boost/asio/signal_set.hpp>
#include <boost/program_options.hpp>
#include <thread>


#include "json_loader.h"
#include "request_handler/request_handler.h"
#include "log.h"
#include "app.h"
#include "ticker.h"
#include "http_server.h"

using namespace std::literals;
namespace sys = boost::system;
namespace net = boost::asio;
namespace fs = std::filesystem;

namespace {

    template <typename Fn>
    void RunThreads(unsigned number_of_threads, const Fn& fn) {
        number_of_threads = std::max(1u, number_of_threads);
        std::vector<std::jthread> workers;
        workers.reserve(number_of_threads - 1);
        while (--number_of_threads) {
            workers.emplace_back(fn);
        }
        fn();
    }

    struct Args {
        std::vector<std::string> source;
        std::string config_file;
        std::string www_root;
        std::chrono::milliseconds tick_period;
        bool on_tick_api = false;
        bool randomize_spawn_points = false;
    };

    [[nodiscard]] std::optional<Args> ParseCommandLine(int argc, const char* const argv[]) {
        namespace po = boost::program_options;
        po::options_description desc{"All options"s};
        Args args;
        uint64_t time = 0;
        desc.add_options()
            // Добавляем опцию --help и её короткую версию -h
            ("help,h", "produce help message")
            // Задаёт период автоматического обновления игрового состояния в миллисекундах
            ("tick-period,t", po::value(&time)->value_name("milliseconds"s), "set tick period")
            // Задаёт путь к конфигурационному JSON-файлу игры
            ("config-file,c", po::value(&args.config_file)->value_name("file"s), "set config file path")
            // Задаёт путь к каталогу со статическими файлами игры
            ("www-root,w", po::value(&args.www_root)->value_name("dir"s), "set static files root")
            // включает режим, при котором пёс игрока появляется в случайной точке случайно выбранной дороги карты.
            ("randomize-spawn-points", "spawn dogs at random positions");
    
        // variables_map хранит значения опций после разбора
        po::variables_map vm;
        po::store(po::parse_command_line(argc, argv, desc), vm);
        po::notify(vm);
        if (vm.contains("help"s)) {
            std::cout << desc;
            return std::nullopt;
        }
        if (vm.contains("tick-period"s)) {
            args.tick_period = std::chrono::milliseconds{ time };
            args.on_tick_api = false;
        }
        else {
            args.tick_period = std::chrono::milliseconds{ 0 };
            args.on_tick_api = true;
        }

        if (!vm.contains("config-file"s)) {
            throw std::runtime_error("Config files have not been specified"s);
        }
        if (!vm.contains("www-root"s)) {
            throw std::runtime_error("Static file path is not specified"s);
        }
        if (!vm.contains("randomize-spawn-points"s)) {
            args.randomize_spawn_points = true;
        }
        return args;
    }

} //namespace

int main(int argc, const char* argv[]) {
    Args args;
    try {
        if (auto args_opt = ParseCommandLine(argc, argv))
            args = std::move(args_opt.value());
        else
            return EXIT_SUCCESS;
    }
    catch (const std::exception& e) {
        std::cout << e.what() << std::endl;
        return EXIT_FAILURE;
    }
    server_logging::InitBoostLogFilter();
    try {
        // 1. Загружаем карту из файла и построить модель игры
        model::Game game = json_loader::LoadGame(args.config_file);
        game.SetRandomizeSpawnPoints(args.randomize_spawn_points);        
        // 1.a Get and check path
        fs::path static_path = args.www_root;
        // 2. Инициализируем io_context
        const unsigned num_threads = std::thread::hardware_concurrency();
        net::io_context ioc(num_threads);
        // 3. Добавляем асинхронный обработчик сигналов SIGINT и SIGTERM
        net::signal_set signals(ioc, SIGINT, SIGTERM);
        signals.async_wait([&ioc](const sys::error_code& ec, [[maybe_unused]] int signal_number) {
            if (!ec) {
                LOGSRV().End(ec);
                ioc.stop();
            }
        });
        // strand, используемый для доступа к API
        auto api_strand = net::make_strand(ioc);
        // 4. Создаём обработчик HTTP-запросов и связываем его с моделью игры
        auto handler = std::make_shared<http_handler::RequestHandler>(static_path, api_strand, game, args.on_tick_api);
        // Настраиваем вызов метода Application::Tick каждые 50 миллисекунд внутри strand
        auto ticker = std::make_shared<ticker::Ticker>(api_strand, args.tick_period,
            [&game](std::chrono::milliseconds delta) { game.Tick(delta); }
        );
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
        const auto address = net::ip::make_address(ServerParam::ADDR);
        constexpr net::ip::port_type port = ServerParam::PORT;
        // Запускаем обработку запросов
        http_server::ServerHttp(ioc, { address, port }, logging_handler);
        ticker->Start();
        // Эта надпись сообщает тестам о том, что сервер запущен и готов обрабатывать запросы
        LOGSRV().Start(address.to_string(), port);
        // 6. Запускаем обработку асинхронных операций
        RunThreads(num_threads, [&ioc] {
            ioc.run();
        });
    } catch (const std::exception& ex) {
        std::cerr << ex.what() << std::endl;
        return EXIT_FAILURE;
    }
}
