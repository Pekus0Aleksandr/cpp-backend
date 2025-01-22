#include "request_handler.h"
#include <boost/algorithm/string.hpp>
#include <iostream>

namespace http_handler {

TypeRequest parse_target(std::string_view target, std::string &res) {
    std::string_view api  = "/api/"sv;
    std::string_view maps = "/api/v1/maps"sv;
    std::string_view join = "/api/v1/game/join"sv;
    std::string_view players = "/api/v1/game/players"sv;
    res = "";
    std::string uriDecode = http_server::uriDecode(target);
    // request stitic files
    auto pos = target.find(api);
    if (pos == target.npos) {
        res = target;
        return TypeRequest::StaticFiles;
    }
    // finded maps
    pos = target.find(maps);
    if (pos != target.npos) {
        if (target.size() == maps.size())
            return TypeRequest::Maps;
        res = target.substr(maps.size() + 1, target.size() - maps.size());
        return TypeRequest::Map;
    }
    // join game
    pos = target.find(join);
    if (pos != target.npos) {
        return TypeRequest::Join;
    }
    if (target.find(players) != target.npos) {
        return TypeRequest::Players;
    }
    return TypeRequest::None;
}

StringResponse RequestHandler::ReportServerError(unsigned version, bool keep_alive) const
{
    return StringResponse();
}

}  // namespace http_handler
