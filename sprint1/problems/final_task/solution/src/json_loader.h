#pragma once

#include <filesystem>
#include <boost/json.hpp>

#include "model.h"

namespace json_loader {

    namespace json = boost::json;
    using namespace model;

    using key_type = json::string_view;

    static constexpr key_type CODE_KEY = "code";
    static constexpr key_type MAP_KEY = "maps";
    static constexpr key_type ID_KEY = "id";
    static constexpr key_type NAME_KEY = "name";
    static constexpr key_type MESSAGE_KEY = "message";
    static constexpr key_type ROADS_KEY = "roads";
    static constexpr key_type BUILDINGS_KEY = "buildings";
    static constexpr key_type OFFICES_KEY = "offices";
    static constexpr key_type OFFSETX_KEY = "offsetX";
    static constexpr key_type OFFSETY_KEY = "offsetY";
    static constexpr key_type X_KEY = "x";
    static constexpr key_type Y_KEY = "y";
    static constexpr key_type X0_KEY = "x0";
    static constexpr key_type Y0_KEY = "y0";
    static constexpr key_type X1_KEY = "x1";
    static constexpr key_type Y1_KEY = "y1";
    static constexpr key_type W_KEY = "w";
    static constexpr key_type H_KEY = "h";

    std::string PrintMap(const Map& map);
    std::string PrintMapList(const Game& game);
    std::string PrintErrorMsgJson(json::string_view code, json::string_view message);

    void LoadRoads(const json::object& map_json_dict, Map& new_map);
    void LoadBuildings(const json::object& map_json_dict, Map& new_map);
    void LoadOffices(const json::object& map_json_dict, Map& new_map);

    model::Game LoadGame(const std::filesystem::path &json_path);
} // namespace json_loader
