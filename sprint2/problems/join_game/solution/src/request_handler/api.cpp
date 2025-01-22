#include "api.h"
#include "../http_server.h"

namespace http_handler {

TypeRequest Api::ParseTarget(std::string_view target, std::string& res) const {
    std::string_view maps = "/api/v1/maps"sv;
    std::string_view join = "/api/v1/game/join"sv;
    std::string_view players = "/api/v1/game/players"sv;
    res = "";
    std::string uriDecode = http_server::uriDecode(target);
    // finded maps
    if (uriDecode.find(maps) != uriDecode.npos) {
        if (uriDecode.size() == maps.size())
            return TypeRequest::Maps;
        res = uriDecode.substr(maps.size() + 1, uriDecode.size() - maps.size());
        return TypeRequest::Map;
    }
    // join game
    if (uriDecode.find(join) != uriDecode.npos) {
        return TypeRequest::Join;
    }
    if (uriDecode.find(players) != uriDecode.npos) {
        return TypeRequest::Players;
    }
    return TypeRequest::None;
}

FileRequestResult Api::MakeGetResponse(const StringRequest& req, bool with_body) const {
    const auto text_response = [&](http::status status, std::string_view text) {
        return MakeStringResponse(status, text, req.version(), req.keep_alive());
    };
    std::string target;
    // if bad URI
    switch (ParseTarget(req.target(), target)) {
    case TypeRequest::Maps:
    case TypeRequest::Map: {
        http::status stat;
        auto [text, flag] = app_.GetMapBodyJson(target);
        stat = flag ? http::status::ok : http::status::not_found;
        return text_response(stat, with_body ? text : ""sv);
    }
    case TypeRequest::Join: {
        auto text = app::JsonMessage("invalidMethod"sv,
            "Only POST method is expected"sv);
        auto resp = MakeStringResponse(http::status::method_not_allowed, with_body ? text : ""sv,
            req.version(), req.keep_alive(), ContentType::APP_JSON, true);
        resp.set(http::field::allow, "POST"sv);
        return resp;
    }
    case TypeRequest::Players: {
        const auto invalid = [&](std::string_view text) {
            return MakeStringResponse(http::status::unauthorized, text, 
                req.version(), req.keep_alive(), ContentType::APP_JSON, true);
        };
        const auto invalidToken = invalid(app::JsonMessage("invalidToken"sv, "Authorization header is missing"sv));
        auto it = req.find(http::field::authorization);
        if (it == req.end())
            return invalidToken;
        std::string token{it->value()};
        auto [body, err] = app_.GetPlayers(token);
        http::status stat;
        switch (err) {
        case app::error_code::InvalidToken:
            return invalidToken;
        case app::error_code::UnknownToken:
            return invalid(app::JsonMessage("unknownToken"sv, "Player token has not been found"sv));
        case app::error_code::None:
            stat = http::status::ok;
            break;
        }
        auto resp = MakeStringResponse(http::status::ok, with_body ? body : ""sv,
            req.version(), req.keep_alive(), ContentType::APP_JSON, true);
        return resp;
    }
    default:
        return text_response(http::status::bad_request,
            with_body ? app::JsonMessage("badRequest", "Get/Head api not found") : ""s
        );
    }
}

FileRequestResult Api::MakePostResponse(const StringRequest& req) const {
    const auto text_response = [&](http::status status, std::string_view text) {
        return MakeStringResponse(status, text, req.version(), req.keep_alive(),
            ContentType::APP_JSON, true);
    };
    std::string target;
    switch (ParseTarget(req.target(), target)) {
    case TypeRequest::Join: {
        auto [body, err] = app_.ResponseJoin(req.body());
        http::status stat;
        switch (err) {
        case app::JoinError::BadJson:
        case app::JoinError::InvalidName:
            stat = http::status::bad_request;
            break;
        case app::JoinError::MapNotFound:
            stat = http::status::not_found;
            break;
        default:
            stat = http::status::ok;
        }
        return text_response(stat, body);

    }
    case TypeRequest::Map:
    case TypeRequest::Maps:
        return MakeInvalidMethod("GET, HEAD"sv, req.version(), req.keep_alive());
    case TypeRequest::Players: {
        return MakeInvalidMethod("GET, HEAD"sv, req.version(), req.keep_alive());
    }
    default:
        return text_response(http::status::bad_request,
            app::JsonMessage("badRequest", "Post api not found"));
    }
}
FileRequestResult Api::MakeOptionsResponse(const StringRequest& req) const
{
    std::string target;
    switch (ParseTarget(req.target(), target)) {
    case TypeRequest::Join:
        return MakeInvalidMethod("POST"sv, req.version(), req.keep_alive());
    case TypeRequest::Map:
    case TypeRequest::Maps:
        return MakeInvalidMethod("GET, HEAD"sv, req.version(), req.keep_alive());
    case TypeRequest::Players:
        return MakeInvalidMethod("GET, HEAD"sv, req.version(), req.keep_alive());
    default:
        return MakeInvalidMethod("GET, HEAD"sv, req.version(), req.keep_alive());
    }
}
FileRequestResult Api::MakePutResponse(const StringRequest& req) const
{
    std::string target;
    switch (ParseTarget(req.target(), target)) {
    case TypeRequest::Join:
        return MakeInvalidMethod("POST"sv, req.version(), req.keep_alive());
    case TypeRequest::Map:
    case TypeRequest::Maps:
        return MakeInvalidMethod("GET, HEAD"sv, req.version(), req.keep_alive());
    case TypeRequest::Players:
        return MakeInvalidMethod("GET, HEAD"sv, req.version(), req.keep_alive());
    default:
        return MakeInvalidMethod("GET, HEAD"sv, req.version(), req.keep_alive());
    }
}
FileRequestResult Api::MakePatchResponse(const StringRequest& req) const
{
    std::string target;
    switch (ParseTarget(req.target(), target)) {
    case TypeRequest::Join:
        return MakeInvalidMethod("POST"sv, req.version(), req.keep_alive());
    case TypeRequest::Map:
    case TypeRequest::Maps:
        return MakeInvalidMethod("GET, HEAD"sv, req.version(), req.keep_alive());
    case TypeRequest::Players:
        return MakeInvalidMethod("GET, HEAD"sv, req.version(), req.keep_alive());
    default:
        return MakeInvalidMethod("GET, HEAD"sv, req.version(), req.keep_alive());
    }
}
FileRequestResult Api::MakeDeleteResponse(const StringRequest& req) const
{
    std::string target;
    switch (ParseTarget(req.target(), target)) {
    case TypeRequest::Join:
        return MakeInvalidMethod("POST"sv, req.version(), req.keep_alive());
    case TypeRequest::Map:
    case TypeRequest::Maps:
        return MakeInvalidMethod("GET, HEAD"sv, req.version(), req.keep_alive());
    case TypeRequest::Players:
        return MakeInvalidMethod("GET, HEAD"sv, req.version(), req.keep_alive());
    default:
        return MakeInvalidMethod("GET, HEAD"sv, req.version(), req.keep_alive());
    }
}
}
