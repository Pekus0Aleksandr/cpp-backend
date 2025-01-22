#include "api_request.h"

namespace http_handler {

    using namespace std::literals;

    http::status ApiRequestHandler::ErrorCodeToStatus(app::error_code ec) const {
        http::status stat = http::status::ok;
        switch (ec) {
        case app::error_code::InvalidToken:
            stat = http::status::unauthorized;
            break;
        case app::error_code::UnknownToken:
            stat = http::status::unauthorized;
            break;
        case app::error_code::InvalidArgument:
            stat = http::status::bad_request;
            break;
        case app::error_code::None:
            stat = http::status::ok;
            break;
        }
        return stat;
    }

    void ApiRequestHandler::LinkJoinWithoutAuthorize() {
        auto ptr = uri_handler_.AddEndpoint(Endpoint::JOIN_GAME);
        if (ptr) {
            ptr->SetNeedAuthorization(false)
                .SetAllowedMethods({http::verb::post}, ErrorMessage::POST_IS_EXPECTED, MiscMessage::ALLOWED_POST_METHOD)
                .SetProcessFunction([&](std::string_view body) {
                    return ProcessPostEndpoitWithoutAuthorization(body); 
                });
        }
    }
    void ApiRequestHandler::LinkPlayersToUriHandler() {
        auto ptr = uri_handler_.AddEndpoint(Endpoint::PLAYERS_LIST);
        if (ptr) {
            ptr->SetNeedAuthorization(true)
                .SetAllowedMethods({http::verb::get, http::verb::head}, ErrorMessage::GET_IS_EXPECTED, MiscMessage::ALLOWED_GET_HEAD_METHOD)
                .SetProcessFunction([&](const Token& token, std::string_view body) {
                return ExecuteAuthorizedPost(token, body, [&](const Token& token, std::string_view) {
                        return app_.GetPlayers(token);
                });
            });
        }
    }

    void ApiRequestHandler::LinkMapsAndMaps() {
        auto ptr = uri_handler_.AddEndpoint(Endpoint::MAPS);
        if (ptr) {
            ptr->SetNeedAuthorization(false)
                .SetNeedTargetProcessing(true)
                .SetAllowedMethods({ http::verb::get, http::verb::head }, ErrorMessage::GET_IS_EXPECTED, MiscMessage::ALLOWED_GET_HEAD_METHOD)
                .SetProcessFunction([&](std::string_view target, std::string_view body) {
                    std::string_view res;
                    auto sz_maps = Endpoint::MAPS.size();
                    if (target.size() != sz_maps) {
                        res = target.substr(sz_maps + 1, target.size() - sz_maps);
                    }
                    http::status stat;
                    auto [text, flag] = app_.GetMapBodyJson(res);
                    stat = flag ? http::status::ok : http::status::not_found;
                    return Response::Make(stat, text);
                });
        }
    }

    void ApiRequestHandler::LinkGameState() {
        auto ptr = uri_handler_.AddEndpoint(Endpoint::GAME_STATE);
        if (ptr) {
            ptr->SetNeedAuthorization(true)
                .SetAllowedMethods({ http::verb::get, http::verb::head }, ErrorMessage::GET_IS_EXPECTED, MiscMessage::ALLOWED_GET_HEAD_METHOD)
                .SetProcessFunction([&](const Token& token, std::string_view body) {
                return ExecuteAuthorizedPost(token, body, [&](const Token& token, std::string_view) {
                    return app_.GetState(token);
                });
            });
        }
    }

    void ApiRequestHandler::LinkGameActionMove() {
        auto ptr = uri_handler_.AddEndpoint(Endpoint::GAME_ACTION);
        if (ptr) {
            ptr->SetNeedAuthorization(true)
                .SetAllowedMethods({ http::verb::post }, ErrorMessage::POST_IS_EXPECTED, MiscMessage::ALLOWED_POST_METHOD)
                .SetProcessFunction([&](const Token& token, std::string_view body) {
                return ExecuteAuthorizedPost(token, body, [&](const Token& token, std::string_view body) {
                    return app_.ActionMove(token, body);
                });
            });
        }
    }

    void ApiRequestHandler::LinkGameTick() {
        auto ptr = uri_handler_.AddEndpoint(Endpoint::GAME_TICK);
        if (ptr) {
            ptr->SetNeedAuthorization(false)
                .SetAllowedMethods({ http::verb::post }, ErrorMessage::POST_IS_EXPECTED, MiscMessage::ALLOWED_POST_METHOD)
                .SetProcessFunction([&](std::string_view body) {
                    auto [text, err] = app_.Tick(body);
                    return Response::Make(ErrorCodeToStatus(err), text);
            });
        }
    }

    StringResponse ApiRequestHandler::ProcessPostEndpoitWithoutAuthorization(std::string_view body) {
        auto [text, err] = app_.ResponseJoin(body);
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
        return Response::Make(stat, text);
    }

} //namespace http_handler
