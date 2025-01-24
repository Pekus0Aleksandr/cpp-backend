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

}  // namespace model
