#ifndef QUIKY_TYPES_H
#define QUIKY_TYPES_H

#include <cstdint>
#include <stdexcept>
#include <string>
#include <vector>

namespace quiky {

typedef std::uint8_t byte;
typedef std::vector<byte> Bytes;

struct FormatError : public std::runtime_error {
    explicit FormatError(const std::string &message) : std::runtime_error(message) {}
};

} // namespace quiky

#endif
