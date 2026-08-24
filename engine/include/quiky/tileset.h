#ifndef QUIKY_TILESET_H
#define QUIKY_TILESET_H

#include "quiky/types.h"

#include <array>
#include <cstddef>
#include <string>
#include <vector>

namespace quiky {

typedef std::array<byte, 16 * 16> Tile;

struct Tileset {
    std::vector<Tile> tiles;

    static Tileset parseIco(const Bytes &data, const std::string &source = "<memory>");
};

} // namespace quiky

#endif
