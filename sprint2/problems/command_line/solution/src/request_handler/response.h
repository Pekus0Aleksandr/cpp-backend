#pragma once 

#include <string_view>
#include <string>
#include <variant>
#include <boost/json.hpp>

#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>

namespace http_handler {

    namespace beast = boost::beast;
    namespace http = beast::http;

    using StringResponse = http::response<http::string_body>;

    class Response {
    public:
        Response() = delete;

        struct ContentType {
            ContentType() = delete;
            constexpr static std::string_view TEXT_JSON = "application/json";
            constexpr static std::string_view PLAIN_TEXT = "text/plain";
            constexpr static std::string_view HTML_TEXT = "text/html";
            constexpr static std::string_view OCTET_STREAM = "application/octet-stream";
        };

        static StringResponse Make(http::status status, std::string_view body, std::string_view content_type = ContentType::TEXT_JSON, std::string_view allow_field = "");
        static StringResponse MakeJSON(http::status status, std::string_view code, std::string_view message);
        static StringResponse MakeUnauthorizedErrorInvalidToken();
        static StringResponse MakeUnauthorizedErrorUnknownToken();
        static StringResponse MakeBadRequestInvalidArgument(std::string_view message);
        static StringResponse MakeMethodNotAllowed(std::string_view message, std::string_view allow);
    };

} //namespace http_handler
