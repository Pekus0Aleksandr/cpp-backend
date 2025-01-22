#include "model.h"
#include <stdexcept>
#include <boost/random.hpp>
#include <boost/multiprecision/cpp_bin_float.hpp>
#include <boost/multiprecision/cpp_int.hpp>
#include <ctime>
#include <random>

#define MAX_ROADS_TO_FOUND 1000

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
            offices_.pop_back();
            throw;
        }
    }

    void Game::AddMap(const Map &map) {
        const size_t index = maps_.size();
        if (auto [it, inserted] = map_id_to_index_.emplace(map.GetId(), index); !inserted) {
            throw std::invalid_argument("Map with id "s + *map.GetId() + " already exists"s);
        } 
        else {
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
        if (auto it = map_id_to_game_sessions_index_.find(id); it != map_id_to_game_sessions_index_.end()) {
            return &game_sessions_.at(it->second);
        }
        return nullptr;
    }

    GameSession* Game::AddGameSession(const Map::Id& id) {
        GameSession gs{ FindMap(id) , randomize_spawn_points_ };
        const size_t index = game_sessions_.size();
        game_sessions_.emplace_back(std::move(gs));
        try {
            map_id_to_game_sessions_index_.emplace(id, index);
        }
        catch (...) {
            game_sessions_.pop_back();
            throw;
        }
        return &game_sessions_.back();
    }

    void Game::Tick(std::chrono::milliseconds time_delta_ms) {
        for (auto &game_session : game_sessions_) {
            game_session.Tick(time_delta_ms);
        }
    }

    Dog* GameSession::AddDog(std::string_view nick_name) {
        DPoint coord;
        if (randomize_spawn_points_) {
            coord = GetRandomRoadCoord();
        }
        else {
            coord = { .x = 0.0, .y = 0.0 };
        }
        Dog dog = Dog(nick_name, coord);
        const size_t index = dogs_.size();
        auto &o = dogs_.emplace_back(std::move(dog));
        try {
            dogs_id_to_index_.emplace(o.GetId(), index);
        }
        catch (...) {
            dogs_.pop_back();
            throw;
        }
        return &dogs_.back();
        return nullptr;
    }

    DPoint GameSession::GetRandomRoadCoord() {
        std::time_t now = std::time(0);
        boost::random::mt19937 gen{static_cast<std::uint32_t>(now)};
        auto random_double = [&](auto x1, auto x2) {
            if (x2 < x1) {
                std::swap(x1, x2);
            }
            std::uniform_real_distribution<double> ur(x1, x2);
            return ur(gen);
        };
        auto &roads = map_->GetRoads();
        if (roads.size() == 0) {
            return DPoint();
        } 
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

    void GameSession::LoadRoadMap() {
        const auto &roads = map_->GetRoads();
        for (const auto& road : roads) {
            auto start = road.GetStart();
            auto end = road.GetEnd();
            if (road.IsHorizontal()) {
                if (start.x > end.x) {
                    std::swap(start, end);
                }
                for (auto x = start.x; x <= end.x; x++) {
                    road_map.emplace(Point{.x = x, .y = start.y}, &road);
                }
            }
            if (road.IsVertical()) {
                if (start.y > end.y) {
                    std::swap(start, end);
                }
                for (auto y = start.y; y <= end.y; y++) {
                    road_map.emplace(Point{ .x = start.x, .y = y }, &road);
                }
            }
        }
    }

    Dog* GameSession::FindDog(std::string_view nick_name) {
        for (auto &dog : dogs_) {
            if (dog.GetName() == nick_name) {
                return &dog;
            }
        }
        return nullptr;
    }

    void GameSession::MoveDog(Dog::Id id, Move move) {
        auto& dog = dogs_[dogs_id_to_index_[id]];
        dog.Diraction(move, map_->GetDogSpeed());
    }

    DPoint GameSession::MoveDog(DPoint start_pos, DPoint end_pos) {
        auto pos_round = [](auto &pos) {
            Point point_pos{ .x = static_cast<Coord>(std::round(pos.x)), .y = static_cast<Coord>(std::round(pos.y))};
            return point_pos;
        };
        if (start_pos == end_pos) {
            return start_pos;
        }          
        auto road_offset = map_->GetRoadOffset();
        auto prev_pos = start_pos;
        int protect = 0;
        do {
            Point dog_cell = pos_round(start_pos);
            auto roads = road_map.equal_range(dog_cell);
            if (PosInRoads(roads, end_pos)) {
                return end_pos;
            }
            prev_pos = start_pos;
            start_pos = GetExtremePos(roads, end_pos);
            if (++protect >= MAX_ROADS_TO_FOUND) {
                throw std::logic_error("Error, not found end cell in roads"s);
            }
        } while (prev_pos != start_pos);
        return start_pos;
    }

    void GameSession::Tick(std::chrono::milliseconds time_delta_ms) {
        for (auto& dog : dogs_) {
            if (dog.IsStanding()) {
                continue;
            }
            auto start_pos = dog.GetPoint();
            auto end_pos = dog.GetEndPoint(time_delta_ms);
            auto move_pos = MoveDog(start_pos, end_pos);
            dog.SetPoint(move_pos);
            if (move_pos != end_pos) {
                dog.Stop();
            } 
        }
    }

    bool GameSession::PosInRoads(RoadMapIter roads, DPoint pos) {
        for (auto it = roads.first; it != roads.second; ++it) {
            auto &road = *it->second;
            auto [x0, x1, y0, y1] = road.GetRectangle();
            if (pos.x >= x0 && pos.x <= x1 && pos.y >= y0 && pos.y <= y1) {
                return true;
            }
        }
        return false;
    }
    DPoint GameSession::GetExtremePos(RoadMapIter roads, DPoint pos) {
        typedef std::numeric_limits<DDimension> dbl; 
        DPoint min;
        DDimension min_d = dbl::max(), distance;
        for (auto it = roads.first; it != roads.second; ++it) {
            auto& road = *it->second;
            auto [x0, x1, y0, y1] = road.GetRectangle();
            if (pos.x >= x0 && pos.x <= x1) {
                if (pos.y <= y0) {
                    if ((distance = std::abs(y0 - pos.y)) < min_d) {
                        min_d = distance;
                        min.y = y0;
                        min.x = pos.x;
                        continue;
                    }
                }
                if (pos.y >= y1) {
                    if ((distance = std::abs(y1 - pos.y)) < min_d) {
                        min_d = distance;
                        min.y = y1;
                        min.x = pos.x;
                        continue;
                    }
                }
            }
            if (pos.y >= y0 && pos.y <= y1) {
                if (pos.x <= x0) {
                    if ((distance = std::abs(x0 - pos.x)) < min_d) {
                        min_d = distance;
                        min.x = x0;
                        min.y = pos.y;
                        continue;
                    }   
                }
                if (pos.x >= x1) {
                    if ((distance = std::abs(x1 - pos.x)) < min_d) {
                        min_d = distance;
                        min.x = x1;
                        min.y = pos.y;
                        continue;
                    }
                }
            }
        }
        return min;
    }

} //namespace model
