#include "quiky/renderer.h"

#include <fstream>
#include <sstream>

namespace quiky {

namespace {

void writeU16LE(std::ostream &output, std::uint16_t value) {
    output.put(static_cast<char>(value & 0xff));
    output.put(static_cast<char>((value >> 8) & 0xff));
}

void writeU32LE(std::ostream &output, std::uint32_t value) {
    output.put(static_cast<char>(value & 0xff));
    output.put(static_cast<char>((value >> 8) & 0xff));
    output.put(static_cast<char>((value >> 16) & 0xff));
    output.put(static_cast<char>((value >> 24) & 0xff));
}

} // namespace

IndexedSurface::IndexedSurface(std::uint32_t widthValue, std::uint32_t heightValue)
    : width(widthValue), height(heightValue), pixels(static_cast<std::size_t>(widthValue) * heightValue, 0) {
}

byte &IndexedSurface::at(std::uint32_t x, std::uint32_t y) {
    if (x >= width || y >= height) {
        throw FormatError("surface coordinate is outside the image");
    }
    return pixels[static_cast<std::size_t>(y) * width + x];
}

byte IndexedSurface::at(std::uint32_t x, std::uint32_t y) const {
    if (x >= width || y >= height) {
        throw FormatError("surface coordinate is outside the image");
    }
    return pixels[static_cast<std::size_t>(y) * width + x];
}

IndexedSurface renderMap(const Map &map, const Tileset &tileset) {
    const std::uint32_t width = static_cast<std::uint32_t>(map.width) * 16;
    const std::uint32_t height = static_cast<std::uint32_t>(map.height) * 16;
    IndexedSurface surface(width, height);

    for (std::uint32_t mapY = 0; mapY < map.height; ++mapY) {
        for (std::uint32_t mapX = 0; mapX < map.width; ++mapX) {
            const std::uint16_t tileIndex = Map::tileId(map.cell(static_cast<std::uint16_t>(mapX),
                                                                  static_cast<std::uint16_t>(mapY)));
            if (tileIndex >= tileset.tiles.size()) {
                for (std::uint32_t tileY = 0; tileY < 16; ++tileY) {
                    for (std::uint32_t tileX = 0; tileX < 16; ++tileX) {
                        surface.at(mapX * 16 + tileX, mapY * 16 + tileY) = 0xff;
                    }
                }
                continue;
            }

            const Tile &tile = tileset.tiles[tileIndex];
            for (std::uint32_t tileY = 0; tileY < 16; ++tileY) {
                for (std::uint32_t tileX = 0; tileX < 16; ++tileX) {
                    surface.at(mapX * 16 + tileX, mapY * 16 + tileY) = tile[tileY * 16 + tileX];
                }
            }
        }
    }
    return surface;
}

void writeBmp(const std::string &path, const IndexedSurface &surface, const Palette &palette) {
    const std::uint32_t rowStride = (surface.width + 3) & ~static_cast<std::uint32_t>(3);
    const std::uint32_t pixelBytes = rowStride * surface.height;
    const std::uint32_t pixelOffset = 14 + 40 + 256 * 4;
    const std::uint32_t fileSize = pixelOffset + pixelBytes;

    std::ofstream output(path.c_str(), std::ios::binary);
    if (!output) {
        throw FormatError(path + ": cannot open output file");
    }

    output.put('B');
    output.put('M');
    writeU32LE(output, fileSize);
    writeU16LE(output, 0);
    writeU16LE(output, 0);
    writeU32LE(output, pixelOffset);

    writeU32LE(output, 40);
    writeU32LE(output, surface.width);
    writeU32LE(output, surface.height);
    writeU16LE(output, 1);
    writeU16LE(output, 8);
    writeU32LE(output, 0);
    writeU32LE(output, pixelBytes);
    writeU32LE(output, 0);
    writeU32LE(output, 0);
    writeU32LE(output, 256);
    writeU32LE(output, 256);

    for (std::size_t index = 0; index < palette.colors.size(); ++index) {
        output.put(static_cast<char>(palette.colors[index].blue));
        output.put(static_cast<char>(palette.colors[index].green));
        output.put(static_cast<char>(palette.colors[index].red));
        output.put(0);
    }

    const std::size_t padding = rowStride - surface.width;
    for (std::uint32_t row = 0; row < surface.height; ++row) {
        const std::uint32_t y = surface.height - row - 1;
        output.write(reinterpret_cast<const char *>(&surface.pixels[static_cast<std::size_t>(y) * surface.width]),
                     static_cast<std::streamsize>(surface.width));
        for (std::size_t index = 0; index < padding; ++index) {
            output.put(0);
        }
    }

    if (!output) {
        throw FormatError(path + ": failed while writing BMP");
    }
}

} // namespace quiky
