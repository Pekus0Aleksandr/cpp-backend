#include "model.h"
#include <stdexcept>
#include <boost/random.hpp>
#include <boost/multiprecision/cpp_bin_float.hpp>
#include <boost/multiprecision/cpp_int.hpp>
#include <ctime>
#include <random>

namespace model {
using namespace std::literals;

void Map::AddOffice(const Office &office) {
    if (warehouse_id_to_index_.contains(office.GetId())) {
        throw std::invalid_argument("Duplicate warehouse");
    }
    const size_t index = offices_.size();
    Office& o = offices_.emplace_back(std::move(office));
    try {
        warehouse_id_to_index_.emplace(o.GetId(), index);
    } catch (...) {
        // Удаляем офис из вектора, если не удалось вставить в unordered_mapS
        offices_.pop_back();
        throw;
    }
}

void Game::AddMap(const Map &map) {
    const size_t index = maps_.size();
    if (auto [it, inserted] = map_id_to_index_.emplace(map.GetId(), index); !inserted) {
        throw std::invalid_argument("Map with id "s + *map.GetId() + " already exists"s);
    } else {
        try {
            maps_.emplace_back(std::move(map));
        } catch (...) {
            map_id_to_index_.erase(it);
            throw;
        }
    }
}

const Map* Game::FindMap(const Map::Id& id) const noexcept {
    if (auto it = map_id_to_index_.find(id); it != map_id_to_index_.end()) {
        return &maps_.at(it->second);
    }
    return nullptr;
}

GameSession* Game::FindGameSession(const Map::Id& id) noexcept {
    if (auto it = map_id_to_game_sessions_index_.find(id);
        it != map_id_to_game_sessions_index_.end()) {
        return &game_sessions_.at(it->second);
    }
    return nullptr;
}

GameSession* Game::AddGameSession(const Map::Id& id) {
    GameSession gs{ FindMap(id) };
    const size_t index = game_sessions_.size();
    game_sessions_.emplace_back(std::move(gs));
    try {
        map_id_to_game_sessions_index_.emplace(id, index);
    }
    catch (...) {
        // Удаляем офис из вектора, если не удалось вставить в unordered_map
        game_sessions_.pop_back();
        throw;
    }
    return &game_sessions_.back();
}

Dog* GameSession::AddDog(std::string_view nick_name)
{
    if (FindDog(nick_name) != nullptr) {
        throw std::invalid_argument("Duplicate dog");
    }
    DPoint coord = GetRandomRoadCoord();
    Dog dog = Dog(nick_name, coord);
    const size_t index = dogs_.size();
    auto &o = dogs_.emplace_back(std::move(dog));
    try {
        dogs_id_to_index_.emplace(o.GetId(), index);
    }
    catch (...) {
        // Удаляем офис из вектора, если не удалось вставить в unordered_map
        dogs_.pop_back();
        throw;
    }
    return &dogs_.back();
    return nullptr;
}

DPoint GameSession::GetRandomRoadCoord()
{
    //std::random_device rd;
    //std::mt19937 random_e2;
    std::time_t now = std::time(0);
    boost::random::mt19937 gen{static_cast<std::uint32_t>(now)};
    auto random_double = [&](auto x1, auto x2) {
        if (x2 < x1) 
            std::swap(x1, x2);
        std::uniform_real_distribution<double> ur(x1, x2);
        return ur(gen);
    };
    auto &roads = map_->GetRoads();
    if (roads.size() == 0)
        return DPoint();
    boost::random::uniform_int_distribution<size_t> dist{0, roads.size()-1};
    auto &road = roads[dist(gen)];
    DPoint coord;
    if (road.IsHorizontal()) {
        auto start_point = road.GetStart();
        auto end_point = road.GetEnd();
        coord.x = random_double(start_point.x, end_point.x);
        coord.y = end_point.y;
    }
    else {
        auto start_point = road.GetStart();
        auto end_point = road.GetEnd();
        coord.y = random_double(start_point.y, end_point.y);
        coord.x = end_point.x;
    }
    return coord;
}

Dog* GameSession::FindDog(std::string_view nick_name)
{
    for (auto &dog : dogs_) {
        if (dog.GetName() == nick_name)
            return &dog;
    }
    return nullptr;
}
void GameSession::MoveDog(Dog::Id id, Move move) {
    auto& dog = dogs_[dogs_id_to_index_[id]];
    dog.Diraction(move, map_->GetDogSpeed());
}
}  // namespace model
