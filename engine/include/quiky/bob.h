#ifndef QUIKY_BOB_H
#define QUIKY_BOB_H

#include "quiky/renderer.h"
#include "quiky/types.h"

#include <cstdint>
#include <string>
#include <vector>

namespace quiky {

struct BobRecord {
    std::uint32_t recordOffset;
    std::uint16_t slot;
    std::uint16_t originX;
    std::uint16_t originY;
    std::uint16_t width;
    std::uint16_t height;
    std::vector<std::uint16_t> codeOffsets;
    Bytes blitterCode;
};

struct Bob {
    std::vector<BobRecord> records;

    static Bob parse(const Bytes &data, const std::string &source = "<memory>");
};

std::vector<std::int16_t> decodeBobRecord(const BobRecord &record);
IndexedSurface renderBobSheet(const Bob &bob, Palette &palette, std::uint32_t columns = 8);
void drawBobRecord(IndexedSurface &surface, const BobRecord &record,
                   std::int32_t worldX, std::int32_t worldY,
                   bool applyOrigin = true);

} // namespace quiky

#endif
