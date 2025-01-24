#pragma once
#include "../sdk.h"
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/strand.hpp>
#include <filesystem>
#include <iostream>
#include <variant>

#include "../http_server.h"
#include "../model.h"
#include "../log.h"
#include "../app.h"

#include "file.h"
#include "api.h"

namespace http_handler {

class RequestHandler : public std::enable_shared_from_this<RequestHandler> {
public:
    using Strand = net::strand<net::io_context::executor_type>;

    RequestHandler(const fs::path& static_path, Strand api_strand, model::Game& game)
        : file{ static_path }
        , api_strand_{ api_strand }
        , api(game) {
        }


    RequestHandler(const RequestHandler&) = delete;
    RequestHandler& operator=(const RequestHandler&) = delete;


    template <typename Body, typename Allocator, typename Send>
    void operator()(tcp::endpoint, http::request<Body, http::basic_fields<Allocator>>&& req, Send&& send) {
        auto version = req.version();
        auto keep_alive = req.keep_alive();
        std::string target{StringRequest(req).target().data(), StringRequest(req).target().size()};
        target = http_server::uriDecode(target);
        std::string_view api = "/api/"sv;
        bool is_target = target.size() > api.size() && (target.substr(0, api.size()) == api);
        try {
            /*req относится к API?*/
            if (is_target) {
                auto handle = [self = shared_from_this(), send,
                    req = std::forward<decltype(req)>(req), version, keep_alive] {
                    try {
                        // Этот assert не выстрелит, так как лямбда-функция будет выполняться внутри strand
                        assert(self->api_strand_.running_in_this_thread());
                        return send(std::get<StringResponse>(
                            self->api.HandleRequest(req)));
                    }
                    catch (...) {
                        send(self->ReportServerError(version, keep_alive));
                    }
                };
                return net::dispatch(api_strand_, handle);
            }
            // Возвращаем результат обработки запроса к файлу
            return std::visit(
                [&send](auto&& result) {
                    send(std::forward<decltype(result)>(result));
                },
                file.HandleRequest(req)
            );
        }
        catch (...) {
            auto text = app::JsonMessage("ServerError"sv,
                "request processing error"sv);
            send(Base::MakeStringResponse(http::status::bad_request, text, version, keep_alive));
            //send(ReportServerError(version, keep_alive));
        }
    }

private:
    StringResponse ReportServerError(unsigned version, bool keep_alive) const;

    Api api;
    File file;
    Strand api_strand_;
};

}  // namespace http_handler
