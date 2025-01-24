#include "http_server.h"

#include <boost/asio/dispatch.hpp>
#include <iostream>

namespace http_server {

    std::string uriDecode(std::string_view src) {
        std::string ret;
        char ch;
        int i, ii;
        for (i = 0; i < src.length(); i++) {
            if (src[i] == '%') {
                [[maybe_unused]] auto s = sscanf(src.substr(i + 1, 2).data(), "%x", &ii);
                ch = static_cast<char>(ii);
                ret += ch;
                i = i + 2;
            }
            else if (src[i] == '+') {
                ret += ' ';
                i = i + 1;
            }
            else if (src[i] >= 'A' && src[i] <= 'Z') {
                ret += src[i] - 'A' + 'a';
            }
            else {
                ret += src[i];
            }
        }
        return ret;
    }

    void SessionBase::Run() {
        net::dispatch(stream_.get_executor(), beast::bind_front_handler(&SessionBase::Read, GetSharedThis()));
    }

    void SessionBase::Read() {
        using namespace std::literals;
        request_ = {};
        stream_.expires_after(30s);
        http::async_read(stream_, buffer_, request_, beast::bind_front_handler(&SessionBase::OnRead, GetSharedThis()));
    }

    void SessionBase::OnRead(beast::error_code ec, [[maybe_unused]] std::size_t bytes_read) {
        using namespace std::literals;
        if (ec == http::error::end_of_stream) {
            return Close();
        }
        if (ec) {
            LOGSRV().Error(ec, server_logging::Server::Where::read);
            return;
        }
        std::string uri = uriDecode(request_.target());
        auto rmeth = http::to_string(request_.method());
        LOGSRV().Request(stream_.socket().remote_endpoint().address().to_string(), uri, std::string_view(rmeth.data(), rmeth.size()));
        start_time_ = steady_clock::now();
        HandleRequest(std::move(request_));
    }

    void SessionBase::OnWrite(bool close, beast::error_code ec, [[maybe_unused]] std::size_t bytes_written) {
        if (ec) {
            LOGSRV().Error(ec, server_logging::Server::Where::write);
            return;
        }
        if (close) {
            return Close();
        }
        Read();
    }

    void SessionBase::Close() {
        stream_.socket().shutdown(tcp::socket::shutdown_send);
    }

} //namespace http_server
