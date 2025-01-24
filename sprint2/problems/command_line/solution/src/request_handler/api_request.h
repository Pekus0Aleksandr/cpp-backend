#pragma once

#include <optional>
#include <mutex>
#include <boost/asio.hpp>

#include "response.h"
#include "defs.h"
#include "uri_api.h"
#include "../app.h"

namespace http_handler {

    namespace net = boost::asio;

    class ApiRequestHandler {
        using Strand = net::strand<net::io_context::executor_type>;
        http::status ErrorCodeToStatus(app::error_code ec) const;

    public:
        ApiRequestHandler(Strand api_strand, model::Game& game, bool on_tick_api) : uri_handler_(), api_strand_(api_strand), app_(game) {
            LinkJoinWithoutAuthorize();
            LinkPlayersToUriHandler();
            LinkMapsAndMaps();
            LinkGameState();
            LinkGameActionMove();
            if (on_tick_api) {
                LinkGameTick();
            }
        };

        template <typename Body, typename Allocator>
        bool IsApiRequest(http::request<Body, http::basic_fields<Allocator>>& req) {
            return req.target().starts_with(Endpoint::API);
        }

        StringResponse ProcessPostEndpoitWithoutAuthorization(std::string_view body);

        template <typename Body, typename Allocator>
        StringResponse ProcessGameRequest(const http::request<Body, http::basic_fields<Allocator>>& req) {
            return uri_handler_.Process(req);
        }

        template <typename Body, typename Allocator>
        bool IsGameRequest(http::request<Body, http::basic_fields<Allocator>>& req) {
            return req.target().starts_with(Endpoint::GAME);
        }

        template <typename Body, typename Allocator>
        auto HandleGameRequest(const http::request<Body, http::basic_fields<Allocator>>& req) {
            auto response = ProcessGameRequest(req);
            response.keep_alive(req.keep_alive());
            response.version(req.version());
            return response;
        }

        template <typename Body, typename Allocator>
        StringResponse Handle(const http::request<Body, http::basic_fields<Allocator>>& req) {
            return std::move(HandleGameRequest(req));
        }

        template <typename Fn>
        StringResponse ExecuteAuthorizedPost(const Token& token, std::string_view body, Fn&& action) const {
            auto [error_body, error_code] = app_.CheckToken(token);
            if (error_code != app::error_code::None) {
                return Response::Make(ErrorCodeToStatus(error_code), error_body);
            }
            auto [text, error_action] = action(token, body);
            return Response::Make(ErrorCodeToStatus(error_action), text);
        }

    private:
        uri_api::UriData uri_handler_;
        Strand api_strand_;
        app::App app_;

    private:
        void LinkJoinWithoutAuthorize();
        void LinkPlayersToUriHandler();
        void LinkMapsAndMaps();
        void LinkGameState();
        void LinkGameActionMove();
        void LinkGameTick();
    };

} //namespace http_handler
