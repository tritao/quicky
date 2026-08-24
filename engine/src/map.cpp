#include "quiky/map.h"

#include "quiky/binary_reader.h"

#include <sstream>

namespace quiky {

Map Map::parse(const Bytes &data, const std::string &source) {
    if (data.size() < 10) {
        throw FormatError(source + ": MAP is shorter than its header");
    }
    if (!(data[0] == 'T' && data[1] == 'L' && data[2] == 'E' && data[3] == '1')) {
        throw FormatError(source + ": unsupported MAP signature");
    }

    BinaryReader reader(data, source);
    reader.skip(4);
    Map result;
    result.width = reader.readU16BE();
    result.height = reader.readU16BE();
    result.unknown = reader.readU16BE();

    const std::size_t cellCount = static_cast<std::size_t>(result.width) * result.height;
    if (cellCount > (reader.remaining() / 2)) {
        std::ostringstream message;
        message << source << ": MAP is truncated (expected " << (10 + cellCount * 2)
                << " bytes, got " << data.size() << ")";
        throw FormatError(message.str());
    }
    result.cells.reserve(cellCount);
    for (std::size_t index = 0; index < cellCount; ++index) {
        result.cells.push_back(reader.readU16BE());
    }
    return result;
}

std::uint16_t Map::cell(std::uint16_t x, std::uint16_t y) const {
    if (x >= width || y >= height) {
        throw FormatError("MAP cell coordinate is outside the map");
    }
    return cells[static_cast<std::size_t>(y) * width + x];
}

} // namespace quiky
