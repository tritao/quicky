#ifndef QUIKY_RENDERER_H
#define QUIKY_RENDERER_H

#include "quiky/map.h"
#include "quiky/palette.h"
#include "quiky/tileset.h"

#include <cstdint>
#include <string>
#include <vector>

namespace quiky {

struct IndexedSurface {
    std::uint32_t width;
    std::uint32_t height;
    std::vector<byte> pixels;

    IndexedSurface(std::uint32_t width, std::uint32_t height);
    byte &at(std::uint32_t x, std::uint32_t y);
    byte at(std::uint32_t x, std::uint32_t y) const;
};

IndexedSurface renderMap(const Map &map, const Tileset &tileset);
void writeBmp(const std::string &path, const IndexedSurface &surface, const Palette &palette);

} // namespace quiky

#endif
