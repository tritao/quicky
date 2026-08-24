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
};

} // namespace quiky

#endif
