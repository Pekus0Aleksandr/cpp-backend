#pragma once
#include "base.h"
#include "../app.h"

namespace http_handler {

class Api : public Base {
private:
	mutable app::App app_;
    virtual FileRequestResult MakeGetResponse(
        const StringRequest& req, bool with_body) const override;
	virtual FileRequestResult MakePostResponse(
        const StringRequest& req) const override;
    virtual FileRequestResult MakeOptionsResponse(
        const StringRequest& req) const override;
    virtual FileRequestResult MakePutResponse(
        const StringRequest& req) const override;
    virtual FileRequestResult MakePatchResponse(
        const StringRequest& req) const override;
    virtual FileRequestResult MakeDeleteResponse(
        const StringRequest& req) const override;
    TypeRequest ParseTarget(
        std::string_view target, std::string& res) const;
    std::string CheckToken(std::string_view token) const;
    std::string GetToken(const StringRequest& req) const;
    http::status ErrorCodeToStatus(app::error_code ec) const;


    template <typename Fn>
    StringResponse ExecuteAuthorized(const StringRequest& req, bool with_body, Fn&& action) const {
        const auto body_response = [&](http::status status, std::string_view text) {
            return MakeStringResponse(status, with_body ? text : ""sv, req.version(),
                req.keep_alive(), ContentType::APP_JSON, true);
        };
        std::string token = GetToken(req);
        auto error_body = CheckToken(token);
        if (!error_body.empty()) {
            return body_response(http::status::unauthorized, error_body);
        }
        std::string token_str;
        auto [body, error_code] = app_.CheckToken(token, token_str);
        if (error_code != app::error_code::None)
            return body_response(ErrorCodeToStatus(error_code), body);
        body = action(token_str);
        return body_response(ErrorCodeToStatus(error_code), body);
    }

    template <typename Fn>
    StringResponse ExecuteAuthorizedPost(const StringRequest& req, bool with_body, Fn&& action) const {
        const auto body_response = [&](http::status status, std::string_view text) {
            return MakeStringResponse(status, with_body ? text : ""sv, req.version(),
                req.keep_alive(), ContentType::APP_JSON, true);
        };
        std::string token = GetToken(req);
        auto error_body_check = CheckToken(token);
        if (!error_body_check.empty()) {
            return body_response(http::status::unauthorized, error_body_check);
        }
        std::string token_str;
        auto [error_body, error_code] = app_.CheckToken(token, token_str);
        if (error_code != app::error_code::None)
            return body_response(ErrorCodeToStatus(error_code), error_body);
        auto [body, error_action] = action(token_str);
        return body_response(ErrorCodeToStatus(error_action), body);
    }

public:
	Api(model::Game& game)
		: app_(game) {}


};
} // namespace http_handler
