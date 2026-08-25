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

    // Parse a PCX palette through the executable's VGA DAC path: source
    // 8-bit components are truncated to six bits, then expanded back to the
    // 8-bit presentation range used by SDL/BMP output.
    static Palette parsePcxDac(const Bytes &data,
                               const std::string &source = "<memory>");

    // Mirrors the executable's DAC helper, which clears one indexed entry by
    // writing three zero RGB components.
    void blackoutEntry(std::size_t index);

    // Apply the observed level-entry fade order (high index down to low).
    void blackoutDescending(std::size_t high, std::size_t low);
};

} // namespace quiky

#endif
