#include "app.h"
#include "log.h"
#include <boost/format.hpp>

std::atomic<uint64_t> app::Player::idn = 0;

std::string app::JsonMessage(std::string_view code, std::string_view message) {
    js::object msg;
    msg["code"] = code.data();
    msg["message"] = message.data();
    return serialize(msg);
}

namespace app {

    using namespace std::literals;

    static auto to_booststr = [](std::string_view str) {
        return boost::string_view(str.data(), str.size());
    };

    std::string ModelToJson::GetMaps() const {
        const auto& maps = game_.GetMaps();
        js::array obj;
        for (auto& map : maps) {
            js::object mapEl;
            mapEl["id"] = *map.GetId();
            mapEl["name"] = map.GetName();
            obj.emplace_back(mapEl);
        }
        return serialize(obj);
    }

    std::string ModelToJson::GetMap(std::string_view nameMap) const {
        model::Map::Id idmap{nameMap.data()};
        js::object mapEl;
        if (auto map = game_.FindMap({ idmap }); map != nullptr) {
            mapEl["id"] = *map->GetId();
            mapEl["name"] = map->GetName();
            mapEl["roads"] = GetRoads(map->GetRoads());
            mapEl["buildings"] = GetBuildings(map->GetBuildings());
            mapEl["offices"] = GetOffice(map->GetOffices());
        }
        return serialize(mapEl);
    }

    js::array ModelToJson::GetRoads(const model::Map::Roads& roads) {
        js::array arr;
        for (auto& r : roads) {
            js::object road;
            road["x0"] = r.GetStart().x;
            road["y0"] = r.GetStart().y;
            if (r.IsHorizontal()) {
                road["x1"] = r.GetEnd().x;
            }
            else {
                road["y1"] = r.GetEnd().y;
            }
            arr.emplace_back(std::move(road));
        }
        return arr;
    }

    js::array ModelToJson::GetBuildings(const model::Map::Buildings& buildings) {
        js::array arr;
        for (auto& b : buildings) {
            js::object building;
            building["x"] = b.GetBounds().position.x;
            building["y"] = b.GetBounds().position.y;
            building["w"] = b.GetBounds().size.width;
            building["h"] = b.GetBounds().size.height;
            arr.emplace_back(std::move(building));
        }
        return arr;
    }

    js::array ModelToJson::GetOffice(const model::Map::Offices& offices) {
        js::array arr;
        for (auto& o : offices) {
            js::object office;
            office["id"] = *o.GetId();
            office["x"] = o.GetPosition().x;
            office["y"] = o.GetPosition().y;
            office["offsetX"] = o.GetOffset().dx;
            office["offsetY"] = o.GetOffset().dy;
            arr.emplace_back(std::move(office));
        }
        return arr;
    }

    void Player::Move(std::string_view move_cmd) {
        model::Move dog_move;
        std::cout << "Move: " << move_cmd << std::endl;
        if (move_cmd == "L"sv) {
            dog_move = model::Move::LEFT;
        }
        else if (move_cmd == "R"sv) {
            dog_move = model::Move::RIGHT;
        }
        else if (move_cmd == "U"sv) {
            dog_move = model::Move::UP;
        }
        else if (move_cmd == "D"sv) {
            dog_move = model::Move::DOWN;
        }
        else if (move_cmd == ""sv) {
            dog_move = model::Move::STAND;
        }
        else {
            throw std::invalid_argument("Invalid argument");
        }
        session_->MoveDog(dog_->GetId(), dog_move);    
    }

    Player* PlayerTokens::FindPlayer(Token token) const {
        if (token_to_player.contains(token)) {
            return token_to_player.at(token);
        }
        return nullptr;
    }

    Token PlayerTokens::AddPlayer(Player* player) {
        Token token{GetToken()};
        token_to_player[token] = player;
        return token;
    }

    std::string ToHex(uint64_t n) {
        std::string hex;
        while (n > 0) {
            int r = n % 16;
            char c = (r < 10) ? ('0' + r) : ('a' + r - 10);
            hex = c + hex;
            n /= 16;
        }
        while (hex.size() < 16) {
            hex = '0' + hex;
        }
        return hex;
    }

    std::pair<std::string, bool> App::GetMapBodyJson(std::string_view mapName) const {
        ModelToJson jmodel(game_);
        std::string body;
        if (mapName.empty()) {
            body = jmodel.GetMaps();
        }
        else {
            body = jmodel.GetMap(mapName);
            if (!body.size()) {
                return std::make_pair(JsonMessage("mapNotFound"sv, "Map not found"sv), false);
            }
        }
        return std::make_pair(std::move(body), true);
    }
    //
    std::pair<std::string, JoinError> App::ResponseJoin(std::string_view jsonBody) {
        auto parseError = std::make_pair(JsonMessage("invalidArgument"sv, "Join game request parse error"sv), JoinError::BadJson);
        js::error_code ec;
        js::string_view jb{jsonBody.data(), jsonBody.size()};
        std::string userName;
        std::string mapId;
        js::value const jv = js::parse(jb, ec);
        if (ec) {
            return parseError;
        }
        try {
            userName = jv.at("userName").as_string();
            mapId = jv.at("mapId").as_string();
        }
        catch (const std::system_error&) {
            return parseError;
        }
        catch (const std::out_of_range&) {
            return parseError;
        }
        if (userName.empty()) {
            return std::make_pair(JsonMessage("invalidArgument"sv, "Invalid name"sv), JoinError::InvalidName);
        }
        model::Map::Id idmap{mapId.data()};
        auto map = game_.FindMap({ idmap });
        if (map == nullptr) {
            return std::make_pair(JsonMessage("mapNotFound"sv, "Map not found"sv), JoinError::MapNotFound);
        }
        js::object msg;
        std::string token;
        uint64_t id;
        auto player = GetPlayer(userName, mapId);
        token = *player_tokens_.AddPlayer(player);
        id = *player->GetId();
        msg["authToken"] = token;
        msg["playerId"]  = id;
        return std::make_pair(std::move(serialize(msg)), JoinError::None);
    }

    std::pair<std::string, error_code> App::ActionMove(const Token& token, std::string_view jsonBody) {
        std::string move;
        try {
            js::value const jv = js::parse(to_booststr(jsonBody));
            move = jv.at("move").as_string();
            Player* player = player_tokens_.FindPlayer(token);
            player->Move(move);
        }
        catch (const std::exception&) {
            return std::make_pair(JsonMessage("invalidArgument"sv, "Failed to parse action"sv), error_code::InvalidArgument);
        }
        
        js::object msg;
        return std::make_pair(std::move(serialize(msg)), error_code::None);
    }

    std::pair<std::string, error_code> App::Tick(std::string_view jsonBody) {
        std::chrono::milliseconds time_delta_mc;
        try {
            auto jv = js::parse(to_booststr(jsonBody));
            auto jv_time_delta = jv.at("timeDelta");
            uint64_t mc;
            if (const auto* n = jv_time_delta.if_int64(); n && *n > 0) {
                mc = *n;
            } 
            else if (const auto* n = jv_time_delta.if_uint64()) {
                mc = *n;
            } 
            else {
                throw std::out_of_range{"not an uint64_t"};
            }
            if (mc == 0) {
                throw std::out_of_range{ "The time should not be zero" };
            }
            time_delta_mc = std::chrono::milliseconds(mc);
        }
        catch (const std::exception&) {
            return std::make_pair(JsonMessage("invalidArgument"sv, "Failed to parse tick request JSON"sv), error_code::InvalidArgument);
        }
        game_.Tick(time_delta_mc);
        js::object msg;
        return std::make_pair(std::move(serialize(msg)), error_code::None);
    }

    std::pair<std::string, error_code> App::GetPlayers(const Token& token) const {
        js::object msg;
        Player* player = player_tokens_.FindPlayer(token);
        auto session = player->GetSession();
        const auto &dogs = session->GetDogs();
        for (const auto& dog : dogs) {
            js::object jname;
            jname["name"] = to_booststr(dog.GetName());
            msg[std::to_string(*dog.GetId())] = jname;
        }
        return std::make_pair(std::move(serialize(msg)),error_code::None);
    }

    std::pair<std::string, error_code> App::GetState(const Token& token) const {
        auto put_array = [](const auto &x, const auto &y) {
            js::array jarr;
            jarr.emplace_back(x);
            jarr.emplace_back(y);
            return jarr;
        };
        Player* player = player_tokens_.FindPlayer(token);
        js::object state;
        auto session = player->GetSession();
        const auto &dogs = session->GetDogs();
        for (const auto &dog : dogs) {
            js::object dog_param;
            dog_param["pos"] = put_array(dog.GetPoint().x, dog.GetPoint().y);
            dog_param["speed"] = put_array(dog.GetSpeed().x, dog.GetSpeed().y);
            dog_param["dir"] = dog.GetDirection();
            state[std::to_string(*dog.GetId())] = dog_param;
        }
        js::object players;
        players["players"] = state;
        return std::make_pair(std::move(serialize(players)), error_code::None);
    }

    std::pair<std::string, error_code> App::CheckToken(const Token& token) const {
        Player* player = player_tokens_.FindPlayer(token);
        if (player == nullptr) {
            return std::make_pair(std::move(app::JsonMessage("unknownToken"sv, "Player token has not been found"sv)), error_code::UnknownToken);
        }
        return std::make_pair("", error_code::None);
    }

    Player* App::GetPlayer(const Token& token) const {
        Player* player = player_tokens_.FindPlayer(token);
        return player;
    }

    Player* App::GetPlayer(std::string_view nickName, std::string_view mapId) {
        model::Map::Id map_id{mapId.data()};
        auto session = game_.FindGameSession(map_id);
        if (session == nullptr) {
            session = game_.AddGameSession(map_id);
        }
        model::Dog* dog = session->AddDog(nickName);
        auto player = players_.Add(dog, session);
        return player;
    }

    Player* Players::Add(model::Dog* dog, model::GameSession* session) {
        const size_t index = players_.size();
        std::unique_ptr<Player> player = std::make_unique<Player>(session, dog);
        Player::Id id{*dog->GetId()};
        if (auto [it, inserted] = player_id_to_index_.emplace(id, index); !inserted) {
            throw std::invalid_argument("Player with id "s + std::to_string(*player->GetId()) + " already exists"s);
        }
        else {
            try {
                players_.emplace_back(std::move(player));
            }
            catch (...) {
                player_id_to_index_.erase(it);
                throw;
            }
        }
        return players_.back().get();
    }

    Player* Players::FindPlayer(Player::Id player_id, model::Map::Id map_id) noexcept {
        if (auto it = player_id_to_index_.find(player_id); it != player_id_to_index_.end()) {
            auto pl = players_.at(it->second).get();
            if (pl->MapId() == map_id) {
                return pl;
            }
        }
        return nullptr;
    }

    const Player::Id* Players::FindPlayerId(std::string_view player_name) const noexcept {
        auto it = std::find_if(players_.begin(), players_.end(), [&](const auto& player) {
                return player->GetName() == player_name;
            }
        );
        if (it != players_.end()) {
            return &(*it)->GetId();
        }
        return nullptr;
    }

    Player* Players::FindPlayer(Player::Id player_id) const noexcept {
        if (auto it = player_id_to_index_.find(player_id); it != player_id_to_index_.end()) {
            auto pl = players_.at(it->second).get();
            return pl;
        }
        return nullptr;
    }

}; //namespace app
