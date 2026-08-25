#include "quiky/pcx.h"

#include <cstddef>

namespace quiky {

namespace {

std::uint16_t readU16LE(const Bytes &data, std::size_t offset,
                        const std::string &source) {
    if (offset + 2 > data.size()) {
        throw FormatError(source + ": truncated PCX header");
    }
    return static_cast<std::uint16_t>(data[offset]) |
           static_cast<std::uint16_t>(data[offset + 1]) << 8;
}

} // namespace

PcxImage PcxImage::parse(const Bytes &data, const std::string &source) {
    if (data.size() < 128 + 769) {
        throw FormatError(source + ": truncated PCX image");
    }
    if (data[0] != 0x0a || data[2] != 0x01 || data[3] != 0x08 ||
        data[65] != 0x01) {
        throw FormatError(source + ": expected an 8-bit, single-plane PCX");
    }

    const std::uint16_t xMin = readU16LE(data, 4, source);
    const std::uint16_t yMin = readU16LE(data, 6, source);
    const std::uint16_t xMax = readU16LE(data, 8, source);
    const std::uint16_t yMax = readU16LE(data, 10, source);
    if (xMax < xMin || yMax < yMin) {
        throw FormatError(source + ": invalid PCX bounds");
    }

    PcxImage result;
    result.width = static_cast<std::uint32_t>(xMax - xMin) + 1;
    result.height = static_cast<std::uint32_t>(yMax - yMin) + 1;
    result.bytesPerLine = readU16LE(data, 66, source);
    if (result.bytesPerLine < result.width || result.bytesPerLine == 0) {
        throw FormatError(source + ": PCX scanline is narrower than the image");
    }
    const std::size_t markerOffset = data.size() - 769;
    if (data[markerOffset] != 0x0c) {
        throw FormatError(source + ": missing PCX DAC palette marker");
    }

    const std::size_t decodedSize =
        static_cast<std::size_t>(result.bytesPerLine) * result.height;
    Bytes decoded;
    decoded.reserve(decodedSize);
    std::size_t cursor = 128;
    while (cursor < markerOffset && decoded.size() < decodedSize) {
        const byte value = data[cursor++];
        std::size_t count = 1;
        byte pixel = value;
        if ((value & 0xc0) == 0xc0) {
            count = value & 0x3f;
            if (count == 0 || cursor >= markerOffset) {
                throw FormatError(source + ": truncated PCX run");
            }
            pixel = data[cursor++];
        }
        if (count > decodedSize - decoded.size()) {
            throw FormatError(source + ": PCX RLE exceeds image bounds");
        }
        decoded.insert(decoded.end(), count, pixel);
    }
    if (decoded.size() != decodedSize) {
        throw FormatError(source + ": PCX RLE ends before the image is complete");
    }

    result.pixels.resize(static_cast<std::size_t>(result.width) * result.height);
    for (std::uint32_t y = 0; y < result.height; ++y) {
        const std::size_t sourceRow = static_cast<std::size_t>(y) * result.bytesPerLine;
        const std::size_t destinationRow = static_cast<std::size_t>(y) * result.width;
        for (std::uint32_t x = 0; x < result.width; ++x) {
            result.pixels[destinationRow + x] = decoded[sourceRow + x];
        }
    }
    result.palette = Palette::parsePcx(data, source);
    return result;
}

IndexedSurface PcxImage::surface() const {
    IndexedSurface result(width, height);
    result.pixels = pixels;
    return result;
}

} // namespace quiky
