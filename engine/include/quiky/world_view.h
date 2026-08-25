#ifndef QUIKY_WORLD_VIEW_H
#define QUIKY_WORLD_VIEW_H

#include "quiky/map.h"
#include "quiky/runtime.h"

#include <cstdint>

namespace quiky {

// A decoded MAP lookup keeps both the raw archive value and its current
// decoded projections. Out-of-bounds is data, not an exception, so a probe can
// be compared with traces before boundary semantics are finalized.
struct MapCell {
    bool inBounds;
    std::int32_t cellX;
    std::int32_t cellY;
    std::uint16_t rawWord;
    std::uint16_t tileId;
    std::uint16_t flags;

    MapCell();
};

struct TileDescriptor {
    bool valid;
    std::uint16_t tileId;
    std::uint16_t descriptorWord;
    std::uint16_t runtimeFlags;

    TileDescriptor();
};

// Read-only world boundary for the simulation. It exposes confirmed helper
// predicates only; it deliberately has no broad isSolid() abstraction.
class WorldCollisionView {
public:
    WorldCollisionView(const Map &map,
                       const PlayerDescriptorTable *descriptors = 0);

    MapCell cellAt(std::int32_t cellX, std::int32_t cellY) const;
    TileDescriptor descriptorFor(const MapCell &cell) const;

    bool hasDescriptorTable() const;
    bool blocksProbeConfirmed(std::int32_t x, std::int32_t y) const;
    bool hasVerticalResponseConfirmed(std::int32_t x,
                                      std::int32_t y) const;
    bool alignsEightPixelsConfirmed(std::int32_t x,
                                    std::int32_t y) const;
    // Exact raw MAP contract of 01F7:1C6E: test word bit 0x4000 at the
    // 16-pixel cell selected by (x,y), independent of player descriptors.
    bool mapRawBit4000Confirmed(std::int32_t x, std::int32_t y) const;

private:
    static std::int32_t floorTile(std::int32_t pixels);

    const Map &_map;
    const PlayerDescriptorTable *_descriptors;
};

} // namespace quiky

#endif
