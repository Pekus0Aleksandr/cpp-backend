#include "json_loader.h"
#include <fstream>
#include <iostream>
#include <sstream>

namespace json_loader {

    namespace json = boost::json;
    using namespace model;

    std::string PrintErrorMsgJson(json::string_view code, json::string_view message) {
        json::object err_msg_js;
        err_msg_js.emplace(CODE_KEY, code);
        err_msg_js.emplace(MESSAGE_KEY, message);
        std::stringstream ss;
        ss << json::serialize(err_msg_js);
        return ss.str();
    }

    std::string PrintMapList(const Game& game) {
        json::array map_list_js;

        for(const auto& map : game.GetMaps()) {
            json::object map_js;
            map_js.emplace(ID_KEY, *map.GetId());
            map_js.emplace(NAME_KEY, map.GetName());
            map_list_js.push_back(map_js);
        }
        std::stringstream ss;
        ss << json::serialize(map_list_js);
        return ss.str();
    }

    json::object MakeRoadJson(const Road& road) {
        json::object road_js;
        road_js.emplace(X0_KEY, road.GetStart().x);
        road_js.emplace(Y0_KEY, road.GetStart().y);

        if(road.IsHorizontal()) {
            road_js.emplace(X1_KEY, road.GetEnd().x);
        } 
        else {
            road_js.emplace(Y1_KEY, road.GetEnd().y);
        }
        return road_js;
    }

    json::object MakeBuildingJson(const Building& bd) {
        json::object bd_js;
        bd_js.emplace(X_KEY, bd.GetBounds().position.x);
        bd_js.emplace(Y_KEY, bd.GetBounds().position.y);

        bd_js.emplace(W_KEY, bd.GetBounds().size.width);
        bd_js.emplace(H_KEY, bd.GetBounds().size.height);

        return bd_js;
    }

    json::object MakeOfficeJson(const Office& offc) {
        json::object offc_js;
        offc_js.emplace(ID_KEY, *offc.GetId());
        offc_js.emplace(X_KEY, offc.GetPosition().x);
        offc_js.emplace(Y_KEY, offc.GetPosition().y);
        offc_js.emplace(OFFSETX_KEY, offc.GetOffset().dx);
        offc_js.emplace(OFFSETY_KEY, offc.GetOffset().dy);

        return offc_js;
    }

    std::string PrintMap(const Map& map) {
        json::object map_js;

        map_js.emplace(ID_KEY, *map.GetId());
        map_js.emplace(NAME_KEY, map.GetName());

        json::array roads_js;
        for(const auto& road : map.GetRoads()) {
            roads_js.push_back(MakeRoadJson(road));
        }

        map_js.emplace(ROADS_KEY, roads_js);

        json::array buildings_js;
        for(const auto& bd : map.GetBuildings()) {
            buildings_js.push_back(MakeBuildingJson(bd));
        }

        map_js.emplace(BUILDINGS_KEY, buildings_js);

        json::array offcs_js;
        for(const auto& offc : map.GetOffices()) {
            offcs_js.push_back(MakeOfficeJson(offc));
        }

        map_js.emplace(OFFICES_KEY, offcs_js);

        std::stringstream ss;
        ss << json::serialize(map_js);
        return ss.str();
    }

    Map BuildMapFromJson(const json::value& map_json) {

        const auto& map_json_dict = map_json.as_object();
        const auto id_ptr = map_json_dict.if_contains(ID_KEY);
        const auto name_ptr = map_json_dict.if_contains(NAME_KEY);

        if (!map_json_dict.if_contains(ID_KEY) || !map_json_dict.if_contains(NAME_KEY)) {
            throw std::logic_error("No id or name key found in map json");
        }

        Map new_map(Map::Id(json::value_to<std::string>(*id_ptr)), json::value_to<std::string>(*name_ptr));

        LoadRoads(map_json_dict, new_map);
        LoadBuildings(map_json_dict, new_map);
        LoadOffices(map_json_dict, new_map);

        return new_map;
    }

    void LoadRoads(const json::object& map_json_dict, Map& new_map) {
        if (const auto roads_ptr = map_json_dict.if_contains(ROADS_KEY)) {
            for (const auto& road_jv : roads_ptr->as_array()) {
                Point start;
                start.x = static_cast<Coord>(road_jv.as_object().at(X0_KEY).as_int64());
                start.y = static_cast<Coord>(road_jv.as_object().at(Y0_KEY).as_int64());

                if (const auto x_coord = road_jv.as_object().if_contains(X1_KEY)) {
                    new_map.AddRoad({ Road::HORIZONTAL, start, static_cast<Coord>(x_coord->as_int64()) });
                }
                else {
                    new_map.AddRoad({
                        Road::VERTICAL, start,
                        static_cast<Coord>(road_jv.as_object().at(Y1_KEY).as_int64())
                        });
                }
            }
        }
    }

    void LoadBuildings(const json::object& map_json_dict, Map& new_map) {
        if (const auto buildings_ptr = map_json_dict.if_contains(BUILDINGS_KEY)) {
            for (const auto& bld_jv : buildings_ptr->as_array()) {
                Point pos;
                pos.x = static_cast<Coord>(bld_jv.as_object().at(X_KEY).as_int64());
                pos.y = static_cast<Coord>(bld_jv.as_object().at(Y_KEY).as_int64());

                Size sz;
                sz.width = static_cast<Dimension>(bld_jv.as_object().at(W_KEY).as_int64());
                sz.height = static_cast<Dimension>(bld_jv.as_object().at(H_KEY).as_int64());

                new_map.AddBuilding(Building({ pos, sz }));
            }
        }
    }

    void LoadOffices(const json::object& map_json_dict, Map& new_map) {
        if (const auto offices_ptr = map_json_dict.if_contains(OFFICES_KEY)) {
            for (const auto& offc_jv : offices_ptr->as_array()) {
                const auto& offc_obj = offc_jv.as_object();
                Office::Id id(std::string(offc_obj.at(ID_KEY).as_string()));

                Point pos;
                pos.x = static_cast<Coord>(offc_obj.at(X_KEY).as_int64());
                pos.y = static_cast<Coord>(offc_obj.at(Y_KEY).as_int64());

                Offset ofs;
                ofs.dx = static_cast<Dimension>(offc_obj.at(OFFSETX_KEY).as_int64());
                ofs.dy = static_cast<Dimension>(offc_obj.at(OFFSETY_KEY).as_int64());

                new_map.AddOffice({ id, pos, ofs });
            }
        }
    }

    model::Game LoadGame(const std::filesystem::path &json_path) {

        if (!std::filesystem::exists(json_path)) {
            throw std::logic_error("File does not exist: " + json_path.string());
        }

        std::ifstream input_file(json_path);
        if (!input_file.is_open()) {
            throw std::logic_error("Failed to open file: " + json_path.string());
        }

        std::string input(std::istreambuf_iterator<char>(input_file), {});
        auto doc = json::parse(input);
        const auto map_array_ptr = doc.as_object().if_contains(MAP_KEY);

        if (!map_array_ptr) {
            throw std::logic_error("No maps in file");
        }

        model::Game game;

        for (const auto &json_map: map_array_ptr->as_array()) {
            try {
                game.AddMap(BuildMapFromJson(json_map));
            } catch (std::exception &ex) {
                std::cerr << ex.what() << std::endl;
            }
        }
        return game;
    }
    
} // namespace json_loader
