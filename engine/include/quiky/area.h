#ifndef QUIKY_AREA_H
#define QUIKY_AREA_H

#include "quiky/types.h"

#include <cstdint>
#include <string>
#include <vector>

namespace quiky {

struct AreaEntity {
    std::uint32_t recordOffset;
    std::uint16_t type;
    std::uint16_t x;
    std::uint16_t y;
};

struct AreaReference {
    std::uint16_t value;
    std::uint32_t targetOffset;
    std::uint32_t occurrences;
    std::vector<AreaEntity> entities;
};

struct AreaPlacement {
    std::uint16_t type;
    std::uint16_t reference;
    std::uint16_t regionX;
    std::uint16_t regionY;
    std::uint16_t localX;
    std::uint16_t localY;
    std::uint32_t worldX;
    std::uint32_t worldY;
};

struct Area {
    static const std::size_t kLayoutOffset = 0x160;
    static const std::size_t kWidthOffset = 0x0e;
    static const std::size_t kHeightOffset = 0x10;
    static const std::size_t kDeclarationOffset = 0x14e0;
    static const std::size_t kFirstRecordOffset = 0x14e8;
    static const std::uint32_t kRegionSize = 64;

    std::uint16_t layoutWidth;
    std::uint16_t layoutHeight;
    std::vector<std::uint16_t> layout;
    std::vector<AreaReference> references;

    static Area parse(const Bytes &data, const std::string &source = "<memory>");
    std::vector<AreaPlacement> placements() const;
};

} // namespace quiky

#endif
