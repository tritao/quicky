#include "quiky/bob.h"

#include "quiky/binary_reader.h"

#include <algorithm>
#include <sstream>

namespace quiky {

Bob Bob::parse(const Bytes &data, const std::string &source) {
    Bob result;
    BinaryReader reader(data, source);
    while (reader.remaining() != 0) {
        const std::uint32_t recordOffset = static_cast<std::uint32_t>(reader.position());
        if (reader.remaining() < 12) {
            throw FormatError(source + ": truncated BOB record header");
        }

        BobRecord record;
        record.recordOffset = recordOffset;
        record.slot = reader.readU16LE();
        record.originX = reader.readU16LE();
        record.originY = reader.readU16LE();
        record.width = reader.readU16LE();
        record.height = reader.readU16LE();
        const std::uint16_t tableSize = reader.readU16LE();
        if (record.width == 0 || record.height == 0) {
            throw FormatError(source + ": BOB record has zero dimensions");
        }
        if (tableSize == 0 || (tableSize & 1) != 0) {
            throw FormatError(source + ": BOB record has an invalid offset table");
        }
        if (reader.remaining() < static_cast<std::size_t>(tableSize) + 2) {
            throw FormatError(source + ": truncated BOB offset table");
        }

        const std::size_t offsetCount = tableSize / 2;
        record.codeOffsets.reserve(offsetCount);
        for (std::size_t index = 0; index < offsetCount; ++index) {
            record.codeOffsets.push_back(reader.readU16LE());
        }
        const std::uint16_t codeSize = reader.readU16LE();
        if (codeSize == 0 || reader.remaining() < codeSize) {
            throw FormatError(source + ": truncated BOB blitter code");
        }
        record.blitterCode = reader.readBytes(codeSize);

        for (std::size_t index = 1; index < record.codeOffsets.size(); ++index) {
            if (record.codeOffsets[index - 1] > record.codeOffsets[index]) {
                throw FormatError(source + ": BOB code offsets are not monotonic");
            }
        }
        if (record.codeOffsets.back() >= codeSize) {
            throw FormatError(source + ": BOB code offset lies outside blitter code");
        }
        result.records.push_back(record);
    }

    if (result.records.empty()) {
        throw FormatError(source + ": empty BOB file");
    }
    return result;
}

std::vector<std::int16_t> decodeBobRecord(const BobRecord &record) {
    std::vector<std::int16_t> pixels(static_cast<std::size_t>(record.width) * record.height, -1);
    int phase = -1;
    std::size_t cursor = 0;

    const auto writePixel = [&](std::uint32_t displacement, byte value) {
        const std::uint32_t x = (displacement % 88) * 4 + static_cast<std::uint32_t>(phase);
        const std::uint32_t y = displacement / 88;
        if (phase < 0 || x >= record.width || y >= record.height) {
            std::ostringstream message;
            message << "BOB slot " << record.slot << ": blitter write exceeds canvas";
            throw FormatError(message.str());
        }
        pixels[static_cast<std::size_t>(y) * record.width + x] = value;
    };

    while (cursor < record.blitterCode.size()) {
        const Bytes &code = record.blitterCode;
        if (cursor + 3 <= code.size() && code[cursor] == 0xee &&
            code[cursor + 1] == 0xd0 && code[cursor + 2] == 0xc0) {
            phase = (phase + 1) % 4;
            cursor += 3;
        } else if (phase >= 0 && cursor + 5 <= code.size() &&
                   code[cursor] == 0xc6 && code[cursor + 1] == 0x84) {
            const std::uint32_t displacement =
                static_cast<std::uint32_t>(code[cursor + 2]) |
                static_cast<std::uint32_t>(code[cursor + 3]) << 8;
            writePixel(displacement, code[cursor + 4]);
            cursor += 5;
        } else if (phase >= 0 && cursor + 6 <= code.size() &&
                   code[cursor] == 0xc7 && code[cursor + 1] == 0x84) {
            const std::uint32_t displacement =
                static_cast<std::uint32_t>(code[cursor + 2]) |
                static_cast<std::uint32_t>(code[cursor + 3]) << 8;
            const byte low = code[cursor + 4];
            const byte high = code[cursor + 5];
            writePixel(displacement, low);
            writePixel(displacement + 1, high);
            cursor += 6;
        } else {
            ++cursor;
        }
    }
    return pixels;
}

IndexedSurface renderBobSheet(const Bob &bob, Palette &palette, std::uint32_t columns) {
    if (columns == 0) {
        throw FormatError("BOB contact sheet requires at least one column");
    }

    std::vector<const BobRecord *> records;
    for (std::size_t index = 0; index < bob.records.size(); ++index) {
        records.push_back(&bob.records[index]);
    }
    std::sort(records.begin(), records.end(), [](const BobRecord *left, const BobRecord *right) {
        return left->slot < right->slot;
    });

    std::uint32_t cellWidth = 0;
    std::uint32_t cellHeight = 0;
    for (std::size_t index = 0; index < records.size(); ++index) {
        cellWidth = std::max(cellWidth, static_cast<std::uint32_t>(records[index]->width) + 4);
        cellHeight = std::max(cellHeight, static_cast<std::uint32_t>(records[index]->height) + 4);
    }
    const std::uint32_t rows = static_cast<std::uint32_t>((records.size() + columns - 1) / columns);
    IndexedSurface surface(cellWidth * columns, cellHeight * rows);

    palette.colors[236].red = 42;
    palette.colors[236].green = 42;
    palette.colors[236].blue = 42;
    palette.colors[237].red = 68;
    palette.colors[237].green = 68;
    palette.colors[237].blue = 68;
    for (std::uint32_t y = 0; y < surface.height; ++y) {
        for (std::uint32_t x = 0; x < surface.width; ++x) {
            surface.at(x, y) = ((x / 4 + y / 4) & 1) ? 236 : 237;
        }
    }

    for (std::size_t index = 0; index < records.size(); ++index) {
        const BobRecord &record = *records[index];
        const std::uint32_t cellX = static_cast<std::uint32_t>(index % columns) * cellWidth;
        const std::uint32_t cellY = static_cast<std::uint32_t>(index / columns) * cellHeight;
        const std::uint32_t left = cellX + (cellWidth - record.width) / 2;
        const std::uint32_t top = cellY + (cellHeight - record.height) / 2;
        const std::vector<std::int16_t> pixels = decodeBobRecord(record);
        for (std::uint32_t y = 0; y < record.height; ++y) {
            for (std::uint32_t x = 0; x < record.width; ++x) {
                const std::int16_t color = pixels[static_cast<std::size_t>(y) * record.width + x];
                if (color >= 0) {
                    surface.at(left + x, top + y) = static_cast<byte>(color);
                }
            }
        }
    }
    return surface;
}

void drawBobRecord(IndexedSurface &surface, const BobRecord &record,
                   std::int32_t worldX, std::int32_t worldY, bool applyOrigin) {
    const std::vector<std::int16_t> pixels = decodeBobRecord(record);
    const std::int32_t left = worldX - (applyOrigin ? record.originX : 0);
    const std::int32_t top = worldY - (applyOrigin ? record.originY : 0);
    for (std::int32_t y = 0; y < record.height; ++y) {
        for (std::int32_t x = 0; x < record.width; ++x) {
            const std::int16_t color = pixels[static_cast<std::size_t>(y) * record.width + x];
            const std::int32_t destinationX = left + x;
            const std::int32_t destinationY = top + y;
            if (color < 0 || destinationX < 0 || destinationY < 0 ||
                static_cast<std::uint32_t>(destinationX) >= surface.width ||
                static_cast<std::uint32_t>(destinationY) >= surface.height) {
                continue;
            }
            surface.at(static_cast<std::uint32_t>(destinationX),
                       static_cast<std::uint32_t>(destinationY)) = static_cast<byte>(color);
        }
    }
}

} // namespace quiky
