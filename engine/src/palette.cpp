#include "quiky/palette.h"

namespace quiky {

namespace {

byte vgaDacExpand(byte component) {
    const byte sixBit = static_cast<byte>(component >> 2);
    // DOSBox's VGA DAC uses (c * 255 + 31) / 63.  This integer form is the
    // exact lookup-table expression used by the reference VGA implementation.
    return static_cast<byte>((static_cast<unsigned>(sixBit) * 259u + 33u) >> 6);
}

} // namespace

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

Palette Palette::parsePcxDac(const Bytes &data, const std::string &source) {
    if (data.size() < 769 || data[0] != 0x0a || data[3] != 0x08) {
        throw FormatError(source + ": not an 8-bit PCX palette");
    }
    const std::size_t markerOffset = data.size() - 769;
    if (data[markerOffset] != 0x0c) {
        throw FormatError(source + ": missing PCX palette marker");
    }

    Palette result;
    for (std::size_t index = 0; index < result.colors.size(); ++index) {
        result.colors[index].red = vgaDacExpand(
            data[markerOffset + 1 + index * 3]);
        result.colors[index].green = vgaDacExpand(
            data[markerOffset + 2 + index * 3]);
        result.colors[index].blue = vgaDacExpand(
            data[markerOffset + 3 + index * 3]);
    }
    return result;
}

void Palette::blackoutEntry(std::size_t index) {
    if (index >= colors.size()) {
        throw FormatError("palette entry out of range");
    }
    colors[index] = RGB{0, 0, 0};
}

void Palette::blackoutDescending(std::size_t high, std::size_t low) {
    if (high >= colors.size() || low >= colors.size() || low > high) {
        throw FormatError("invalid descending palette range");
    }
    for (std::size_t index = high;; --index) {
        blackoutEntry(index);
        if (index == low) {
            break;
        }
    }
}

} // namespace quiky
