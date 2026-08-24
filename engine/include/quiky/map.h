#ifndef QUIKY_MAP_H
#define QUIKY_MAP_H

#include "quiky/types.h"

#include <cstdint>
#include <string>
#include <vector>

namespace quiky {

struct Map {
    std::uint16_t width;
    std::uint16_t height;
    std::uint16_t unknown;
    std::vector<std::uint16_t> cells;

    static Map parse(const Bytes &data, const std::string &source = "<memory>");

    std::uint16_t cell(std::uint16_t x, std::uint16_t y) const;
    static std::uint16_t tileId(std::uint16_t value) { return value & 0x01ff; }
    static std::uint16_t properties(std::uint16_t value) { return value >> 9; }
};

} // namespace quiky

#endif
