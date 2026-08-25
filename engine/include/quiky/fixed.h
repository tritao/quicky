#ifndef QUIKY_FIXED_H
#define QUIKY_FIXED_H

#include <cstdint>

namespace quiky {

// The original runtime stores positions and velocities as signed 16.16
// values. Keep all representation-changing arithmetic here so gameplay code
// cannot depend on the host compiler's overflow or shift rules.
struct Fixed16 {
    static const std::int32_t kFractionBits = 16;
    static const std::int32_t kOne = 1 << kFractionBits;

    std::int32_t raw;

    Fixed16() : raw(0) {}
    explicit Fixed16(std::int32_t rawValue) : raw(rawValue) {}

    static Fixed16 fromRaw(std::int32_t rawValue);
    static Fixed16 fromPixels(std::int32_t pixels);

    std::int32_t floorPixels() const;
    std::int32_t truncPixels() const;
    std::uint16_t fractionRaw() const;

    static std::int32_t floorRaw(std::int32_t rawValue);
    static std::int32_t truncRaw(std::int32_t rawValue);
    static std::int32_t arithmeticShiftRight(std::int32_t value,
                                              unsigned bits);
    static std::int32_t wrapAddRaw(std::int32_t lhs, std::int32_t rhs);
    static std::int32_t wrapSubRaw(std::int32_t lhs, std::int32_t rhs);
    static std::int32_t wrapNegRaw(std::int32_t value);
    static std::int32_t wrapMulRaw(std::int32_t lhs, std::int32_t rhs);
    static std::int32_t clampRaw(std::int32_t value,
                                 std::int32_t minimum,
                                 std::int32_t maximum);

    Fixed16 operator+(Fixed16 other) const;
    Fixed16 operator-(Fixed16 other) const;
    Fixed16 operator*(Fixed16 other) const;
    Fixed16 operator-() const;
    Fixed16 &operator+=(Fixed16 other);
    Fixed16 &operator-=(Fixed16 other);
    Fixed16 &operator*=(Fixed16 other);
};

bool operator==(Fixed16 lhs, Fixed16 rhs);
bool operator!=(Fixed16 lhs, Fixed16 rhs);

} // namespace quiky

#endif
