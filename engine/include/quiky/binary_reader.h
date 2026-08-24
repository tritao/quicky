#ifndef QUIKY_BINARY_READER_H
#define QUIKY_BINARY_READER_H

#include "quiky/types.h"

#include <cstddef>
#include <string>

namespace quiky {

class BinaryReader {
public:
    BinaryReader(const byte *data, std::size_t size, const std::string &source = "<memory>");
    explicit BinaryReader(const Bytes &data, const std::string &source = "<memory>");

    std::size_t position() const { return _position; }
    std::size_t size() const { return _size; }
    std::size_t remaining() const { return _size - _position; }

    byte readU8();
    std::uint16_t readU16LE();
    std::uint16_t readU16BE();
    std::uint32_t readU32LE();
    Bytes readBytes(std::size_t count);
    void skip(std::size_t count);

private:
    void require(std::size_t count) const;

    const byte *_data;
    std::size_t _size;
    std::size_t _position;
    std::string _source;
};

Bytes readFile(const std::string &path);

} // namespace quiky

#endif
