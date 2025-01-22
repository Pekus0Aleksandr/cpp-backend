#pragma once
#include <boost/json.hpp>
#include <filesystem>
#include <iostream>
#include <variant>

#include "http_server.h"
#include "model.h"
#include "log.h"

namespace http_handler {
namespace beast = boost::beast;
namespace http = beast::http;
namespace js = boost::json;
namespace fs = std::filesystem;
namespace sys = boost::system;
using namespace std::literals;
// Запрос, тело которого представлено в виде строки
using StringRequest = http::request<http::string_body>;
// Ответ, тело которого представлено в виде строки
using StringResponse = http::response<http::string_body>;
using FileResponse = http::response<http::file_body>;
using VariantResponse = std::variant<StringResponse, FileResponse>;

enum class TypeRequest {
    None,
    StaticFiles,
    Maps,
    Map
};

struct ContentType {
    ContentType() = delete;
    constexpr static std::string_view TEXT_HTML  = "text/html"sv; // .htm, .html
    constexpr static std::string_view TEXT_CSS   = "text/css"sv; // .css
    constexpr static std::string_view TEXT_PLAIN = "text/plain"sv; // .txt
    constexpr static std::string_view TEXT_JS    = "text/javascript"sv; // .js
    constexpr static std::string_view APP_JSON   = "application/json"sv; // .json
    constexpr static std::string_view APP_XML    = "application/xml"sv; // .xml
    constexpr static std::string_view IMAGE_PNG  = "image/png"sv; // .png
    constexpr static std::string_view IMAGE_JPG  = "image/jpeg"sv; // .jpg, .jpe, .jpeg
    constexpr static std::string_view IMAGE_GIF  = "image/gif"sv; // .gif
    constexpr static std::string_view IMAGE_BMP  = "image/bmp"sv; // .bmp
    constexpr static std::string_view IMAGE_ICO  = "image/vnd.microsoft.icon"sv; // .ico
    constexpr static std::string_view IMAGE_TIFF = "image/tiff"sv; // .tiff, .tif
    constexpr static std::string_view IMAGE_SVG  = "image/svg+xml"sv; // .svg, .svgz
    constexpr static std::string_view AUDIO_MP3  = "audio/mpeg"sv; // .mp3
    constexpr static std::string_view EMPTY      = "application/octet-stream"sv;
    // file extension
    constexpr static std::string_view FE_TEXT_HTML_1 = ".htm"sv;
    constexpr static std::string_view FE_TEXT_HTML_2 = ".html"sv;
    constexpr static std::string_view FE_TEXT_CSS    = ".css"sv;
    constexpr static std::string_view FE_TEXT_PLAIN  = ".txt"sv;
    constexpr static std::string_view FE_TEXT_JS     = ".js"sv;
    constexpr static std::string_view FE_APP_JSON    = ".json"sv;
    constexpr static std::string_view FE_APP_XML     = ".xml"sv;
    constexpr static std::string_view FE_IMAGE_PNG   = ".png"sv;
    constexpr static std::string_view FE_IMAGE_JPG_1 = ".jpg"sv;
    constexpr static std::string_view FE_IMAGE_JPG_2 = ".jpe"sv;
    constexpr static std::string_view FE_IMAGE_JPG_3 = ".jpeg"sv;
    constexpr static std::string_view FE_IMAGE_GIF   = ".gif"sv;
    constexpr static std::string_view FE_IMAGE_BMP   = ".bmp"sv;
    constexpr static std::string_view FE_IMAGE_ICO   = ".ico"sv;
    constexpr static std::string_view FE_IMAGE_TIFF_1 = ".tiff"sv;
    constexpr static std::string_view FE_IMAGE_TIFF_2 = ".tif"sv;
    constexpr static std::string_view FE_IMAGE_SVG_1 = ".svg"sv;
    constexpr static std::string_view FE_IMAGE_SVG_2 = ".svgz"sv;
    constexpr static std::string_view FE_AUDIO_MP3   = ".mp3"sv;
    constexpr static std::string_view FE_EMPTY       = ""sv;

    static std::string_view get(std::string_view key) {
        if (!type.contains(key))
            return EMPTY;
        return type.at(key);
    }
private:
    static std::unordered_map<std::string_view, std::string_view> type;
};

class ModelToJson {
public:
    explicit ModelToJson(model::Game& game)
        : game_{game} {
    }
    std::string GetMaps();
    std::string GetMap(std::string_view nameMap);
private:
    model::Game& game_;
    static js::array GetRoads(const model::Map::Roads& roads);
    static js::array GetBuildings(const model::Map::Buildings& buildings);
    static js::array GetOffice(const model::Map::Offices& offices);
};

class RequestHandler {
public:
    explicit RequestHandler(model::Game& game, const fs::path &static_path)
        : game_{ game }, static_path_{ CheckStaticPath(static_path)} {

    }

    RequestHandler(const RequestHandler&) = delete;
    RequestHandler& operator=(const RequestHandler&) = delete;

    template <typename Body, typename Allocator, typename Send>
    void operator()(http::request<Body, http::basic_fields<Allocator>>&& req, Send&& send) {
        // Обработать запрос request и отправить ответ, используя send
        auto res = HandleRequest(std::forward<decltype(req)>(req));
        if (std::holds_alternative<StringResponse>(res))
            send(std::get<StringResponse>(res));
        else
            send(std::get<FileResponse>(res));
    }

private:
    model::Game& game_;
    const fs::path static_path_;
    VariantResponse HandleRequest(StringRequest&& req);
    std::pair<std::string, http::status> GetMapBodyJson(std::string_view requestTarget);
    std::string StatusToJson(std::string_view code, std::string_view message);
    StringResponse MakeStringResponse(
        http::status status, std::string_view requestTarget, unsigned http_version,
        bool keep_alive, std::string_view content_type = ContentType::APP_JSON);
    StringResponse MakeBadResponse(
        http::status status, unsigned http_version,
        bool keep_alive, std::string_view content_type = ContentType::APP_JSON);
    VariantResponse MakeGetResponse(StringRequest& req, bool with_body);
    VariantResponse StaticFilesResponse(
        std::string_view responseText, bool with_body,
        unsigned http_version, bool keep_alive);
    static fs::path CheckStaticPath(const fs::path& path_static);
    bool CheckFileExist(std::string& file) const;
    bool FileInRootStaticDir(std::string_view file) const;
};

}  // namespace http_handler
