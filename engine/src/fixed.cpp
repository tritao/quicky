#include "quiky/fixed.h"

#include <cstdint>

namespace quiky {
namespace {

std::int32_t fromUnsignedBits(std::uint32_t value) {
    if (value <= 0x7fffffffU) {
        return static_cast<std::int32_t>(value);
    }
    const std::uint32_t magnitude = value - 0x80000000U;
    if (magnitude == 0) {
        return static_cast<std::int32_t>(-2147483647 - 1);
    }
    return static_cast<std::int32_t>(-2147483647 - 1) +
           static_cast<std::int32_t>(magnitude);
}

std::int32_t wrapFromInt64(std::int64_t value) {
    return fromUnsignedBits(static_cast<std::uint32_t>(value));
}

std::int64_t arithmeticShiftRight64(std::int64_t value, unsigned bits) {
    if (bits == 0) {
        return value;
    }
    if (bits >= 63) {
        return value < 0 ? -1 : 0;
    }

    const std::int64_t divisor = static_cast<std::int64_t>(1ULL << bits);
    if (value >= 0) {
        return value / divisor;
    }

    // C++ integer division truncates toward zero. This is the floor-like
    // result produced by the arithmetic right shift used by the DOS target.
    const std::int64_t magnitude = -value;
    return -((magnitude + divisor - 1) / divisor);
}

} // namespace

Fixed16 Fixed16::fromRaw(std::int32_t rawValue) {
    return Fixed16(rawValue);
}

Fixed16 Fixed16::fromPixels(std::int32_t pixels) {
    const std::uint32_t bits = static_cast<std::uint32_t>(pixels) <<
                               kFractionBits;
    return Fixed16(fromUnsignedBits(bits));
}

std::int32_t Fixed16::floorPixels() const {
    return floorRaw(raw);
}

std::int32_t Fixed16::truncPixels() const {
    return truncRaw(raw);
}

std::uint16_t Fixed16::fractionRaw() const {
    return static_cast<std::uint16_t>(static_cast<std::uint32_t>(raw) &
                                      0xffffU);
}

std::int32_t Fixed16::floorRaw(std::int32_t rawValue) {
    return arithmeticShiftRight(rawValue, kFractionBits);
}

std::int32_t Fixed16::truncRaw(std::int32_t rawValue) {
    return rawValue / kOne;
}

std::int32_t Fixed16::arithmeticShiftRight(std::int32_t value,
                                           unsigned bits) {
    return wrapFromInt64(arithmeticShiftRight64(value, bits));
}

std::int32_t Fixed16::wrapAddRaw(std::int32_t lhs, std::int32_t rhs) {
    return wrapFromInt64(static_cast<std::int64_t>(lhs) + rhs);
}

std::int32_t Fixed16::wrapSubRaw(std::int32_t lhs, std::int32_t rhs) {
    return wrapFromInt64(static_cast<std::int64_t>(lhs) - rhs);
}

std::int32_t Fixed16::wrapNegRaw(std::int32_t value) {
    return fromUnsignedBits(0U - static_cast<std::uint32_t>(value));
}

std::int32_t Fixed16::wrapMulRaw(std::int32_t lhs, std::int32_t rhs) {
    const std::int64_t product = static_cast<std::int64_t>(lhs) * rhs;
    return wrapFromInt64(arithmeticShiftRight64(product, kFractionBits));
}

std::int32_t Fixed16::clampRaw(std::int32_t value,
                               std::int32_t minimum,
                               std::int32_t maximum) {
    if (value < minimum) {
        return minimum;
    }
    if (value > maximum) {
        return maximum;
    }
    return value;
}

Fixed16 Fixed16::operator+(Fixed16 other) const {
    return Fixed16(wrapAddRaw(raw, other.raw));
}

Fixed16 Fixed16::operator-(Fixed16 other) const {
    return Fixed16(wrapSubRaw(raw, other.raw));
}

Fixed16 Fixed16::operator*(Fixed16 other) const {
    return Fixed16(wrapMulRaw(raw, other.raw));
}

Fixed16 Fixed16::operator-() const {
    return Fixed16(wrapNegRaw(raw));
}

Fixed16 &Fixed16::operator+=(Fixed16 other) {
    raw = wrapAddRaw(raw, other.raw);
    return *this;
}

Fixed16 &Fixed16::operator-=(Fixed16 other) {
    raw = wrapSubRaw(raw, other.raw);
    return *this;
}

Fixed16 &Fixed16::operator*=(Fixed16 other) {
    raw = wrapMulRaw(raw, other.raw);
    return *this;
}

bool operator==(Fixed16 lhs, Fixed16 rhs) {
    return lhs.raw == rhs.raw;
}

bool operator!=(Fixed16 lhs, Fixed16 rhs) {
    return !(lhs == rhs);
}

} // namespace quiky
