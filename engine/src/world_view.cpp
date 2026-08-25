#include "quiky/world_view.h"

#include <cstdint>

namespace quiky {

MapCell::MapCell()
    : inBounds(false), cellX(0), cellY(0), rawWord(0), tileId(0), flags(0) {
}

TileDescriptor::TileDescriptor()
    : valid(false), tileId(0), descriptorWord(0), runtimeFlags(0) {
}

WorldCollisionView::WorldCollisionView(
    const Map &map, const PlayerDescriptorTable *descriptors)
    : _map(map), _descriptors(descriptors) {
}

MapCell WorldCollisionView::cellAt(std::int32_t cellX,
                                   std::int32_t cellY) const {
    MapCell result;
    result.cellX = cellX;
    result.cellY = cellY;
    if (cellX < 0 || cellY < 0 ||
        cellX >= static_cast<std::int32_t>(_map.width) ||
        cellY >= static_cast<std::int32_t>(_map.height)) {
        return result;
    }

    result.inBounds = true;
    result.rawWord = _map.cell(static_cast<std::uint16_t>(cellX),
                               static_cast<std::uint16_t>(cellY));
    result.tileId = Map::tileId(result.rawWord);
    result.flags = Map::properties(result.rawWord);
    return result;
}

TileDescriptor WorldCollisionView::descriptorFor(const MapCell &cell) const {
    TileDescriptor result;
    if (!cell.inBounds || _descriptors == 0) {
        return result;
    }

    result.valid = true;
    result.tileId = cell.tileId;
    result.descriptorWord = _descriptors->word(cell.tileId);
    // The supplied descriptor table is the runtime flag source recovered from
    // the executable. Keep the named alias until individual bits have final
    // semantic names.
    result.runtimeFlags = result.descriptorWord;
    return result;
}

bool WorldCollisionView::hasDescriptorTable() const {
    return _descriptors != 0;
}

bool WorldCollisionView::blocksProbeConfirmed(std::int32_t x,
                                              std::int32_t y) const {
    const MapCell cell = cellAt(floorTile(x), floorTile(y));
    const TileDescriptor descriptor = descriptorFor(cell);
    if (!descriptor.valid) {
        return false;
    }
    return PlayerDescriptorRules::blocksProbe(
        descriptor.descriptorWord, static_cast<std::uint16_t>(x),
        static_cast<std::uint16_t>(y));
}

bool WorldCollisionView::hasVerticalResponseConfirmed(std::int32_t x,
                                                      std::int32_t y) const {
    const MapCell cell = cellAt(floorTile(x), floorTile(y));
    const TileDescriptor descriptor = descriptorFor(cell);
    return descriptor.valid &&
           PlayerDescriptorRules::hasVerticalResponse(
               descriptor.descriptorWord);
}

bool WorldCollisionView::alignsEightPixelsConfirmed(std::int32_t x,
                                                    std::int32_t y) const {
    const MapCell cell = cellAt(floorTile(x), floorTile(y));
    const TileDescriptor descriptor = descriptorFor(cell);
    return descriptor.valid &&
           PlayerDescriptorRules::alignsEightPixels(
               descriptor.descriptorWord);
}

bool WorldCollisionView::mapRawBit4000Confirmed(std::int32_t x,
                                                std::int32_t y) const {
    const MapCell cell = cellAt(floorTile(x), floorTile(y));
    return cell.inBounds && (cell.rawWord & 0x4000U) != 0;
}

std::int32_t WorldCollisionView::floorTile(std::int32_t pixels) {
    if (pixels >= 0) {
        return pixels / 16;
    }
    return -static_cast<std::int32_t>(
        (-static_cast<std::int64_t>(pixels) + 15) / 16);
}

} // namespace quiky
