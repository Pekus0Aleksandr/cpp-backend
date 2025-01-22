#include "app.h"
#include "log.h"

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

std::string_view GetToken(std::string_view autorization_text) {
    std::string_view bearer = "Bearer "sv;
    std::string_view ex_token = "6516861d89ebfff147bf2eb2b5153ae1"sv;
    std::string_view nullstr = autorization_text.substr(0, 0);
    if (autorization_text.substr(0, bearer.size()) != bearer)
        return nullstr;
    if (autorization_text.size() < (bearer.size() + ex_token.size()))
        return nullstr;
    size_t begin = bearer.size();
    size_t end = autorization_text.size() - 1;
    for (; end > begin && autorization_text[end] == ' '; end--);
    std::string_view out = autorization_text.substr(begin, end);
    if (out.size() != ex_token.size())
        return nullstr;
    return out;
}

std::string ModelToJson::GetMaps() {
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



std::string ModelToJson::GetMap(std::string_view nameMap) {
    model::Map::Id idmap{nameMap.data()};
    auto map = game_.FindMap({ idmap });
    if (map == nullptr)
        return "";
    js::object mapEl;
    mapEl["id"] = *map->GetId();
    mapEl["name"] = map->GetName();
    mapEl["roads"] = GetRoads(map->GetRoads());
    mapEl["buildings"] = GetBuildings(map->GetBuildings());
    mapEl["offices"] = GetOffice(map->GetOffices());
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

Player* PlayerTokens::FindPlayer(Token token) const
{
    if (token_to_player.contains(token))
        return token_to_player.at(token);
    return nullptr;
}

Token PlayerTokens::AddPlayer(Player* player)
{
    Token token{GetToken()};
    token_to_player[token] = player;
    return token;
}

std::string PlayerTokens::ToHex(uint64_t n) const {
    std::string hex;
    //loop runs til n is greater than 0
    while (n > 0) {
        int r = n % 16;
        //remainder converted to hexadecimal digit
        char c = (r < 10) ? ('0' + r) : ('a' + r - 10);
        //hexadecimal digit added to start of the string
        hex = c + hex;
        n /= 16;
    }
    while (hex.size() < 16)
        hex = '0' + hex;
    return hex;
}

std::pair<std::string, bool>
App::GetMapBodyJson(std::string_view mapName) const {
    ModelToJson jmodel(game_);
    std::string body;
    if (mapName.empty()) {
        body = jmodel.GetMaps();
    }
    else {
        // if map not found
        body = jmodel.GetMap(mapName);
        if (!body.size()) {
            return std::make_pair(JsonMessage("mapNotFound"sv, "Map not found"sv), false);
        }
    }
    return std::make_pair(std::move(body), true);
}
//
std::pair<std::string, JoinError>
App::ResponseJoin(std::string_view jsonBody) {
    auto parseError = std::make_pair(
        JsonMessage("invalidArgument"sv, "Join game request parse error"sv),
        JoinError::BadJson
    );
    js::error_code ec;
    js::string_view jb{jsonBody.data(), jsonBody.size()};
    std::string resp;
    std::string userName;
    std::string mapId;
    js::value const jv = js::parse(jb, ec);
    if (ec)
        return parseError;
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
    if (userName.empty())
        return std::make_pair(
            JsonMessage("invalidArgument"sv, "Invalid name"sv),
            JoinError::InvalidName
        );
    model::Map::Id idmap{mapId.data()};
    auto map = game_.FindMap({ idmap });
    if (map == nullptr)
        return std::make_pair(
            JsonMessage("mapNotFound"sv, "Map not found"sv),
            JoinError::MapNotFound
        );
    js::object msg;
    std::string token;
    uint64_t id;
    auto player = GetPlayer(userName, mapId);
    token = *player_tokens_.AddPlayer(player);
    //token = player_tokens_.FindPlayer();
    id = *player->GetId();
    msg["authToken"] = token;
    msg["playerId"]  = id;
    return std::make_pair(
        std::move(serialize(msg)),
        JoinError::None
    );
}

std::pair<std::string, error_code> App::GetPlayers(std::string_view auth_text) const {
    js::object msg;
    std::string_view authToken = GetToken(auth_text);
    if (authToken.empty())
        return std::make_pair(
            JsonMessage("invalidToken"sv, "Authorization header is missing"sv),
            error_code::InvalidToken
        );
    Token token{std::string(authToken.data(), authToken.size())};
    Player* player = player_tokens_.FindPlayer(token);
    if (player == nullptr)
        return std::make_pair(
            JsonMessage("unknownToken"sv, "Player token has not been found"sv),
            error_code::UnknownToken
        );
    for (uint64_t id = 0; (player = players_.FindPlayer(Player::Id{id})) != nullptr; id++) {
        js::object jname;
        jname["name"] = to_booststr(player->GetName());
        msg[std::to_string(id)] = jname;
    }
    std::string out = serialize(msg);
    return std::make_pair(
        std::move(out),
        error_code::None
    );
}

Player* App::GetPlayer(std::string_view nickName, std::string_view mapId)
{
    model::Map::Id map_id{mapId.data()};
    auto playerId = players_.FindPlayerId(nickName);
    if (playerId == nullptr) {
        auto session = game_.FindGameSession(map_id);
        if (session == nullptr)
            session = game_.AddGameSession(map_id);
        model::Dog* dog = session->AddDog(nickName);
        auto player = players_.Add(dog, session);
        //player_tokens_.AddPlayer(player);
        return player;
    }
    else {
        return players_.FindPlayer(*playerId, map_id);
    }
    return nullptr;
}

Player* Players::Add(model::Dog* dog, model::GameSession* session) {
    const size_t index = players_.size();
    std::unique_ptr<Player> player = std::make_unique<Player>(session, dog);
    if (auto [it, inserted] = player_id_to_index_.emplace(player->GetId(), index); !inserted) {
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
        if (pl->MapId() == map_id)
            return pl;
    }
    return nullptr;
}
const Player::Id* Players::FindPlayerId(std::string_view player_name) const noexcept
{
    for (const auto& player : players_) {
        if (player->GetName() == player_name) {
            return &player->GetId();
        }
    }
    return nullptr;
}
Player* Players::FindPlayer(Player::Id player_id) const noexcept
{
    if (auto it = player_id_to_index_.find(player_id); it != player_id_to_index_.end()) {
        auto pl = players_.at(it->second).get();
        return pl;
    }
    return nullptr;
}
};
