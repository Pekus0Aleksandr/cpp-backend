#pragma once
#include "http_server.h"
#include "model.h"

#include <boost/json/detail/index_sequence.hpp>
#include <ranges>
#include <sstream>
#include <string>

#include "json_loader.h"

namespace http_handler {
    namespace beast = boost::beast;
    namespace http = beast::http;
    namespace view = std::views;
    namespace json = boost::json;

    using namespace std::literals;

    using StringRequest = http::request<http::string_body>;
    using StringResponse = http::response<http::string_body>;

    struct ContentType {
        ContentType() = delete;
        constexpr static std::string_view TEXT_HTML = "text/html"sv;
        constexpr static std::string_view APP_JSON = "application/json"sv;
    };

    StringResponse MakeStringResponse(http::status status, std::string_view body, unsigned http_version,
                                      bool keep_alive, std::string_view content_type = ContentType::APP_JSON);

    class RequestHandler {
    public:
        explicit RequestHandler(model::Game& game) : game_{ game } {}

        RequestHandler(const RequestHandler&) = delete;
        RequestHandler& operator=(const RequestHandler&) = delete;

        template<typename Body, typename Allocator, typename Send>
        void operator()(
            http::request<Body, http::basic_fields<Allocator>>&& req, Send&& send) {

            auto text_response = [&](http::status status, std::string_view text) {
                return MakeStringResponse(status, text, req.version(), req.keep_alive());
                };

            if (req.method() != http::verb::get) {
                auto response = text_response(http::status::method_not_allowed, "Invalid method"sv);
                response.set(http::field::allow, "GET"sv);
                send(response);
                return;
            }

            std::string_view tg_full = req.target();
            auto uri_tokens = ParseUri(tg_full);
            if (uri_tokens.empty()) {
                send(text_response(http::status::bad_request, json_loader::PrintErrorMsgJson("badRequest", "Bad request")));
                return;
            }

            if (tg_full.rfind(api_prefix, 0) == 0) {
                HandleApiRequests(tg_full, uri_tokens, send, text_response);
            }
            else {
                send(text_response(http::status::not_found, json_loader::PrintErrorMsgJson("notFound", "Not found")));
            }
        }

    private:
        void HandleApiRequests(std::string_view tg_full, const std::vector<std::string>& uri_tokens, auto&& send, auto&& text_response) {
            if (uri_tokens.size() > 2 && tg_full.rfind(maps_list_uri, 0) == 0) {
                if (uri_tokens.size() > 3) {
                    const auto map = game_.FindMap(model::Map::Id(uri_tokens[3]));
                    if (!map) {
                        send(text_response(http::status::not_found,
                            json_loader::PrintErrorMsgJson("mapNotFound", "Map not found")));
                        return;
                    }
                    send(text_response(http::status::ok, json_loader::PrintMap(*map)));
                }
                else {
                    send(text_response(http::status::ok, json_loader::PrintMapList(game_)));
                }
            }
            else {
                send(text_response(http::status::bad_request,
                    json_loader::PrintErrorMsgJson("badRequest", "Bad request")));
            }
        }

        std::vector<std::string> ParseUri(std::string_view uri);

    private:
        model::Game& game_;
        const std::string_view api_prefix{ "/api/"sv };
        const std::string_view maps_list_uri{ "/api/v1/maps"sv };
    };
} // namespace http_handler
