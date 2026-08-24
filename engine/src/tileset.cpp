#include "quiky/tileset.h"

namespace quiky {

Tileset Tileset::parseIco(const Bytes &data, const std::string &source) {
    if (data.empty() || data.size() % 256 != 0) {
        throw FormatError(source + ": ICO is not a whole-tile file");
    }

    const bool kellmap = data[0] >= 0x80;
    Tileset result;
    result.tiles.reserve(data.size() / 256);
    for (std::size_t tileOffset = 0; tileOffset < data.size(); tileOffset += 256) {
        Tile tile;
        for (std::size_t y = 0; y < 16; ++y) {
            for (std::size_t displayX = 0; displayX < 16; ++displayX) {
                const std::size_t rawX = ((displayX * 4) & 0x0f) + (displayX >> 2);
                byte color = data[tileOffset + y * 16 + rawX];
                if (kellmap) {
                    if (color >= 0xa0) {
                        color = static_cast<byte>((color - 0xa0) + 32);
                    } else if (color >= 0x90) {
                        color = static_cast<byte>((color - 0x90) + 16);
                    }
                }
                tile[y * 16 + displayX] = color;
            }
        }
        result.tiles.push_back(tile);
    }
    return result;
}

} // namespace quiky
