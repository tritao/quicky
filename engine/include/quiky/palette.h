#ifndef QUIKY_PALETTE_H
#define QUIKY_PALETTE_H

#include "quiky/types.h"

#include <array>
#include <cstdint>
#include <string>

namespace quiky {

struct RGB {
    byte red;
    byte green;
    byte blue;
};

struct Palette {
    std::array<RGB, 256> colors;

    static Palette parsePcx(const Bytes &data, const std::string &source = "<memory>");

    // Convert source PCC channels to the RGB888 values presented by the
    // game's 6-bit VGA DAC path. Parsing remains raw for asset inspection.
    Palette toVgaOutput() const;
};

} // namespace quiky

#endif
