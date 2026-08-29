#ifndef QUIKY_BIENE_RUNTIME_TABLE_H
#define QUIKY_BIENE_RUNTIME_TABLE_H

#include <array>
#include <cstdint>
#include <cstddef>

namespace quiky {

// 01F7:0A43 writes 0x800 signed bytes at DS:7974. BIENE state 1 indexes
// the first 0x400 entries, but the complete table is retained for parity.
static const std::size_t kBieneRuntimeTableSize = 0x800;

// Reproduce the byte result of the retail 0A43 -> 15E5 software-float path.
// The DOS-time seed in 19EE belongs to the separate 646C PRNG ring and does
// not affect DS:7974.
std::array<std::int8_t, kBieneRuntimeTableSize>
generateBieneRuntimeTable();

} // namespace quiky

#endif
