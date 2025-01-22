#pragma once 
#include "sdk.h"
#include <string>
#include <unordered_map>
#include <vector>
#include <deque>
#include <atomic>
#include <map>
#include <memory>
#include "tagged.h"
#include "log.h"

namespace model {
 
    using DDimension = double;
    using DCoord = DDimension;

    enum class Move {
        LEFT,
        RIGHT,
        UP,
        DOWN,
        STAND
    };

    enum class Direction {
        NORTH, 
        SOUTH,
        WEST, 
        EAST
    };

    struct DPoint {
        bool operator==(const DPoint& p) const {
            return (this->x == p.x && this->y == p.y);
        }
        DDimension x = 0, y = 0;
    };

    struct DSpeed {
        bool operator==(const DSpeed& p) const {
            return (this->x == p.x && this->y == p.y);
        }
        double x = 0, y = 0;
    };

    class Dog {
    public:
        using Id = util::Tagged<uint64_t, Dog>;
        Dog(std::string_view nickname, const DPoint& coord) 
            : nickname_(nickname.data()
            , nickname.size())
            , id_(Id{ idn++ })
            , coord_(coord){
        }
        Dog(const Dog& other) 
            : id_(other.id_)
            , nickname_(other.nickname_),
            coord_(other.coord_)
            , speed_(other.speed_)
            , dir_(other.dir_) {
        }
        Dog(Dog&& other) noexcept 
            : id_(std::move(other.id_))
            , nickname_(std::move(other.nickname_))
            , coord_(std::move(other.coord_))
            , speed_(std::move(other.speed_))
            , dir_(std::move(other.dir_)) {
        }
        const Id& GetId() const {
            return id_;
        }
        std::string_view GetName() const noexcept {
            return nickname_;
        }
        const DPoint& GetPoint() const {
            return coord_;
        }
        const DSpeed& GetSpeed() const {
            return speed_;
        }
        std::string GetDirection() const;
        void Diraction(Move move, DDimension speed);
        DPoint GetEndPoint(std::chrono::milliseconds move_time_ms);
        void SetPoint(DPoint coord) {
            coord_ = coord;
        }
        bool IsStanding() {
            return speed_ == zero_speed_;
        }
        void Stop() {
            speed_ = zero_speed_;
        }
    private:
        static std::atomic<uint64_t> idn;
        static DSpeed zero_speed_;
        Id id_ = Id{0};
        std::string nickname_ = "";
        DPoint coord_;
        DSpeed speed_ = zero_speed_;
        Direction dir_ = Direction::NORTH;
    };

} //namespace model
