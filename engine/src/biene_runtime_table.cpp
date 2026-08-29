#include "quiky/biene_runtime_table.h"

#include <cmath>

namespace quiky {

std::array<std::int8_t, kBieneRuntimeTableSize>
generateBieneRuntimeTable() {
    std::array<std::int8_t, kBieneRuntimeTableSize> table;
    const double twoPi = 2.0 * std::acos(-1.0);
    for (std::size_t index = 0; index < kBieneRuntimeTableSize; ++index) {
        const double angle = twoPi * static_cast<double>(index) /
                             static_cast<double>(kBieneRuntimeTableSize);
        const long value = std::lround(127.0 * std::sin(angle));
        table[index] = static_cast<std::int8_t>(value);
    }
    return table;
}

} // namespace quiky
