#include "model.h"

#include <stdexcept>

std::atomic<uint64_t> model::Dog::idn = 0;

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
        // Удаляем офис из вектора, если не удалось вставить в unordered_map
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
    Dog dog = Dog(nick_name);
    const size_t index = dogs_.size();
    auto o = dogs_.emplace_back(std::move(dog));
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

Dog* GameSession::FindDog(std::string_view nick_name)
{
    for (auto &dog : dogs_) {
        if (dog.GetName() == nick_name)
            return &dog;
    }
    return nullptr;
}

}  // namespace model
