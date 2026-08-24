#include "quiky/area.h"

#include "quiky/binary_reader.h"

#include <map>
#include <sstream>

namespace quiky {

Area Area::parse(const Bytes &data, const std::string &source) {
    if (data.size() < kDeclarationOffset) {
        std::ostringstream message;
        message << source << ": ARE is shorter than its fixed layout region";
        throw FormatError(message.str());
    }

    BinaryReader header(data, source);
    header.skip(kWidthOffset);
    const std::uint16_t width = header.readU16BE();
    const std::uint16_t height = header.readU16BE();
    if (width == 0 || height == 0) {
        throw FormatError(source + ": ARE has zero layout dimensions");
    }

    const std::size_t wordCount = static_cast<std::size_t>(width) * height;
    const std::size_t layoutBytes = wordCount * 2;
    if (layoutBytes > kDeclarationOffset - kLayoutOffset) {
        throw FormatError(source + ": ARE layout overlaps declarations");
    }

    BinaryReader layoutReader(&data[kLayoutOffset], layoutBytes, source + " layout");
    Area result;
    result.layoutWidth = width;
    result.layoutHeight = height;
    result.layout.reserve(wordCount);

    std::map<std::uint16_t, std::uint32_t> occurrenceCounts;
    for (std::size_t index = 0; index < wordCount; ++index) {
        const std::uint16_t value = layoutReader.readU16BE();
        result.layout.push_back(value);
        if (value != 0 && value != 0xffff) {
            ++occurrenceCounts[value];
        }
    }

    for (std::map<std::uint16_t, std::uint32_t>::const_iterator it = occurrenceCounts.begin();
         it != occurrenceCounts.end(); ++it) {
        const std::uint16_t reference = it->first;
        const std::size_t targetOffset = kLayoutOffset + reference;
        if (targetOffset < kFirstRecordOffset || targetOffset + 2 > data.size()) {
            std::ostringstream message;
            message << source << ": ARE reference 0x" << std::hex << reference
                    << " points outside declarations";
            throw FormatError(message.str());
        }

        BinaryReader declaration(&data[targetOffset], data.size() - targetOffset,
                                 source + " declaration");
        AreaReference parsed;
        parsed.value = reference;
        parsed.targetOffset = static_cast<std::uint32_t>(targetOffset);
        parsed.occurrences = it->second;
        while (true) {
            const std::uint32_t recordOffset =
                static_cast<std::uint32_t>(targetOffset + declaration.position());
            const std::uint16_t type = declaration.readU16BE();
            if (type == 0xffff) {
                break;
            }
            AreaEntity entity;
            entity.recordOffset = recordOffset;
            entity.type = type;
            entity.x = declaration.readU16BE();
            entity.y = declaration.readU16BE();
            parsed.entities.push_back(entity);
        }
        result.references.push_back(parsed);
    }
    return result;
}

std::vector<AreaPlacement> Area::placements() const {
    std::map<std::uint16_t, const AreaReference *> references;
    for (std::size_t index = 0; index < this->references.size(); ++index) {
        references[this->references[index].value] = &this->references[index];
    }

    std::vector<AreaPlacement> result;
    for (std::size_t index = 0; index < layout.size(); ++index) {
        const std::uint16_t referenceValue = layout[index];
        if (referenceValue == 0 || referenceValue == 0xffff) {
            continue;
        }
        const std::map<std::uint16_t, const AreaReference *>::const_iterator reference =
            references.find(referenceValue);
        if (reference == references.end()) {
            throw FormatError("ARE layout references an unparsed declaration");
        }

        const std::uint16_t regionX = static_cast<std::uint16_t>(index % layoutWidth);
        const std::uint16_t regionY = static_cast<std::uint16_t>(index / layoutWidth);
        for (std::size_t entityIndex = 0;
             entityIndex < reference->second->entities.size(); ++entityIndex) {
            const AreaEntity &entity = reference->second->entities[entityIndex];
            AreaPlacement placement;
            placement.type = entity.type;
            placement.reference = referenceValue;
            placement.regionX = regionX;
            placement.regionY = regionY;
            placement.localX = entity.x;
            placement.localY = entity.y;
            placement.worldX = static_cast<std::uint32_t>(regionX) * kRegionSize + entity.x;
            placement.worldY = static_cast<std::uint32_t>(regionY) * kRegionSize + entity.y;
            result.push_back(placement);
        }
    }
    return result;
}

} // namespace quiky
