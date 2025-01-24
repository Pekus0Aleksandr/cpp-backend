#include "model.h"

#include <stdexcept>

std::atomic<uint64_t> model::Dog::idn = 0;

namespace model {
using namespace std::literals;


std::string Dog::GetDirection() const
{
    switch (dir_) {
    case Direction::NORTH:
        return "U";
    case Direction::EAST:
        return "R";
    case Direction::SOUTH:
        return "D";
    case Direction::WEST:
        return "L";
    }
    return "U";
}

void Dog::Diraction(Move move, DDimension speed) {
    switch (move) {
    case Move::LEFT:
        speed_ = { -1*speed, 0.0 };
    case Move::RIGHT:
        speed_ = { speed, 0.0 };
    case Move::UP:
        speed_ = { 0.0, -1*speed };
    case Move::DOWN:
        speed_ = { 0.0, speed };
    case Move::STAND:
        speed_ = { 0.0, 0.0 };
    }
}

}  // namespace model
