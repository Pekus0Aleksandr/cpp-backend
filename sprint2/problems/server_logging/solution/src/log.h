#pragma once 
#include <ostream>
#include <iostream>
#include <boost/json.hpp>
#include <boost/asio/buffer.hpp>
#include <boost/system.hpp>

#define LOG() Logger::Log::GetInstance()
#define LOGSRV() Logger::Server::GetInstance()

namespace Logger {

namespace sys = boost::system;
namespace net = boost::asio;
namespace json = boost::json;
    
void InitBoostLogFilter();

// Для синхронного вывода в Boost.Asio могут использовать любой тип, удовлетворяющий требованиям,
// описанным в документе:
// https://www.boost.org/doc/libs/1_78_0/doc/html/boost_asio/reference/SyncWriteStream.html
class Log {
    Log() = default;
    Log(const Log&) = delete;
public:
    static Log& GetInstance() {
        static Log obj;
        return obj;
    }
    static void info(std::string_view data, std::string_view message);

    template <typename ConstBufferSequence>
    size_t write_some(const ConstBufferSequence& cbs, sys::error_code& ec) {
        const size_t total_size = net::buffer_size(cbs);
        if (total_size == 0) {
            ec = {};
            return 0;
        }
        size_t bytes_written = 0;
        for (const auto& cb : cbs) {
            const size_t size = cb.size();
            const char* const data = reinterpret_cast<const char*>(cb.data());
            if (size > 0) {
                if (!os_.write(reinterpret_cast<const char*>(data), size)) {
                    ec = make_error_code(boost::system::errc::io_error);
                    return bytes_written;
                }
                bytes_written += size;
            }
        }
        ec = {};
        return bytes_written;
    }

    template <typename ConstBufferSequence>
    size_t write_some(const ConstBufferSequence& cbs) {
        sys::error_code ec;
        const size_t bytes_written = write_some(cbs, ec);
        if (ec) {
            throw std::runtime_error("Failed to write");
        }
        return bytes_written;
    }

private:
    std::ostream& os_ = std::cout;
};

class Server {
private:
    Log& log_; 
    Server() : log_(Log::GetInstance()) {}
public:
    enum class Where {
        read,
        write,
        accept
    };
    static Server& GetInstance() {
        static Server obj;
        return obj;
    }
    void start(std::string_view address, int port);
    void end(const sys::error_code& ec);
    void error(const sys::error_code& ec, Where where);
    void request(std::string_view address, std::string_view uri, std::string_view method);
    void response(long long response_time, unsigned status_code, std::string_view content_type);
};



} // namespace logger
