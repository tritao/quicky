#ifndef QUIKY_PCX_H
#define QUIKY_PCX_H

#include "quiky/palette.h"
#include "quiky/renderer.h"

#include <cstdint>
#include <string>

namespace quiky {

// The original title, menu, font, and status-bar resources are 8-bit,
// single-plane PCX/PCC images.  Keep the decoded indexed pixels alongside
// their file-owned DAC palette so callers can composite them without losing
// palette provenance.
struct PcxImage {
    std::uint32_t width;
    std::uint32_t height;
    std::uint16_t bytesPerLine;
    Bytes pixels;
    Palette palette;

    static PcxImage parse(const Bytes &data,
                          const std::string &source = "<memory>");
    IndexedSurface surface() const;
};

} // namespace quiky

#endif
