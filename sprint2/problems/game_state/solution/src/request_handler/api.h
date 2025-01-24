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
public:
	Api(model::Game& game)
		: app_(game) {}


};
} // namespace http_handler
