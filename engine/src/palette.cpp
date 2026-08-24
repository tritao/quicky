#include "quiky/palette.h"

namespace quiky {

Palette Palette::parsePcx(const Bytes &data, const std::string &source) {
    if (data.size() < 769 || data[0] != 0x0a || data[3] != 0x08) {
        throw FormatError(source + ": not an 8-bit PCX palette");
    }
    const std::size_t markerOffset = data.size() - 769;
    if (data[markerOffset] != 0x0c) {
        throw FormatError(source + ": missing PCX palette marker");
    }

    Palette result;
    for (std::size_t index = 0; index < result.colors.size(); ++index) {
        result.colors[index].red = data[markerOffset + 1 + index * 3];
        result.colors[index].green = data[markerOffset + 2 + index * 3];
        result.colors[index].blue = data[markerOffset + 3 + index * 3];
    }
    return result;
}

} // namespace quiky
