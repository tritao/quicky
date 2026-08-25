#ifndef QUIKY_RENDERER_H
#define QUIKY_RENDERER_H

#include "quiky/area.h"
#include "quiky/map.h"
#include "quiky/palette.h"
#include "quiky/tileset.h"

#include <cstdint>
#include <string>
#include <vector>

namespace quiky {

struct IndexedSurface {
    std::uint32_t width;
    std::uint32_t height;
    std::vector<byte> pixels;

    IndexedSurface(std::uint32_t width, std::uint32_t height);
    byte &at(std::uint32_t x, std::uint32_t y);
    byte at(std::uint32_t x, std::uint32_t y) const;
};

enum class SurfaceBlitMode {
    Opaque,
    TransparentZero,
};

// The native gameplay frame is a 320x200 logical surface: the world renderer
// owns rows 0..175 and GAMEBAR.PCC is an opaque 320x24 strip at y=176.
void compositeGamebar(IndexedSurface &screen, const IndexedSurface &gamebar);

// Crop the camera-visible world to the native 320x176 gameplay area, then
// append the opaque GAMEBAR strip to produce one complete 320x200 frame.
IndexedSurface composeGameplayFrame(const IndexedSurface &world,
                                    const IndexedSurface &gamebar,
                                    std::int32_t cameraX,
                                    std::int32_t cameraY);

IndexedSurface renderMap(const Map &map, const Tileset &tileset);
// Composite a screen-space layer with clipped indexed writes.  HUD/menu
// surfaces use Opaque; callers that intentionally need a masked utility layer
// can opt into TransparentZero.  The native ICO helper itself writes all four
// planes, including indexed zero, so drawIcoTile defaults to opaque writes.
void blitIndexedSurface(IndexedSurface &destination,
                        const IndexedSurface &source,
                        std::int32_t destinationX,
                        std::int32_t destinationY,
                        SurfaceBlitMode mode = SurfaceBlitMode::Opaque);
void drawIcoTile(IndexedSurface &surface, const Tileset &tileset,
                 std::uint16_t tileIndex, std::int32_t worldX,
                 std::int32_t worldY, bool transparentZero = false);
void overlayArea(IndexedSurface &surface, Palette &palette, const Area &area);
void writeBmp(const std::string &path, const IndexedSurface &surface, const Palette &palette);

} // namespace quiky

#endif
