#include "quiky/palette.h"

#include <algorithm>

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

Palette Palette::toVgaOutput() const {
    Palette result = *this;
    const auto channel = [](byte source) {
        // The original palette loader reduces PCC bytes with >> 2. Captured
        // gameplay frames show the active table one DAC step below that
        // quotient; DOSBox expands the 6-bit value with this rounded 6->8
        // mapping.
        const int dac = std::max(0, static_cast<int>(source >> 2) - 1);
        return static_cast<byte>((dac * 259 + 33) >> 6);
    };
    for (RGB &color : result.colors) {
        color.red = channel(color.red);
        color.green = channel(color.green);
        color.blue = channel(color.blue);
    }
    return result;
}

} // namespace quiky
