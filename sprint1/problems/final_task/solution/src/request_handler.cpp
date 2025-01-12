#include "request_handler.h"

namespace http_handler {

    StringResponse MakeStringResponse(http::status status, std::string_view body,
                                      unsigned http_version, bool keep_alive, std::string_view content_type) {
        StringResponse response(status, http_version);
        response.set(http::field::content_type, content_type);
        response.body() = body;
        response.content_length(body.size());
        response.keep_alive(keep_alive);
        return response;
    }

    std::vector<std::string> RequestHandler::ParseUri(std::string_view uri) {
        uri.remove_prefix(1);
        std::istringstream ss{std::string(uri)};

        std::vector<std::string> tokens;
        std::string token;

        while (std::getline(ss, token, '/')) {
            tokens.push_back(token);
        }
        return tokens;
    }
} // namespace http_handler
