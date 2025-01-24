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

struct DPoint {
    //DPoint() {}
    //explicit DPoint(double x, double y) : x(x), y(y)
    //{}
    //friend DogCoord operator"" _posx(long double val);
    //friend DogCoord operator"" _posy(long double val);
    DDimension x = 0, y = 0;
};
/*
DogCoord operator"" _posx(long double val) {
    return DogCoord(val);
}
DogCoord operator"" _posy(long double val) {
    return DogCoord(val);
}*/
struct DSpeed {
    //explicit DSpeed(double x, double y) : x(x), y(y)
    //{}
    double x = 0, y = 0;
};

enum class Direction {
    NORTH, 
    SOUTH,
    WEST, 
    EAST
};

class Dog {
public:
    using Id = util::Tagged<uint64_t, Dog>;
    Dog(std::string_view nickname, const DPoint& coord) :
        nickname_(nickname.data(), nickname.size()), id_(Id{ idn++ }),
        coord_(coord){
    }
    Dog(const Dog& other) : id_(other.id_), nickname_(other.nickname_),
        coord_(other.coord_), speed_(other.speed_), dir_(other.dir_) {
    }
    Dog(Dog&& other) noexcept :
        id_(std::move(other.id_)), nickname_(std::move(other.nickname_)),
        coord_(std::move(other.coord_)), speed_(std::move(other.speed_)),
        dir_(std::move(other.dir_)) {
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

private:
    static std::atomic<uint64_t> idn;
    Id id_ = Id{0};
    std::string nickname_ = "";
    DPoint coord_;
    DSpeed speed_ = DSpeed{0.0f, 0.0f};
    Direction dir_ = Direction::NORTH;
};

}  // namespace model
