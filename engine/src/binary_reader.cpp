#include "quiky/binary_reader.h"

#include <fstream>
#include <sstream>

namespace quiky {

BinaryReader::BinaryReader(const byte *data, std::size_t size, const std::string &source)
    : _data(data), _size(size), _position(0), _source(source) {
}

BinaryReader::BinaryReader(const Bytes &data, const std::string &source)
    : BinaryReader(data.empty() ? nullptr : &data[0], data.size(), source) {
}

void BinaryReader::require(std::size_t count) const {
    if (count > remaining()) {
        std::ostringstream message;
        message << _source << ": truncated data at offset 0x" << std::hex << _position;
        throw FormatError(message.str());
    }
}

byte BinaryReader::readU8() {
    require(1);
    return _data[_position++];
}

std::uint16_t BinaryReader::readU16LE() {
    require(2);
    const std::uint16_t value = static_cast<std::uint16_t>(_data[_position]) |
                                static_cast<std::uint16_t>(_data[_position + 1]) << 8;
    _position += 2;
    return value;
}

std::uint16_t BinaryReader::readU16BE() {
    require(2);
    const std::uint16_t value = static_cast<std::uint16_t>(_data[_position]) << 8 |
                                static_cast<std::uint16_t>(_data[_position + 1]);
    _position += 2;
    return value;
}

std::uint32_t BinaryReader::readU32LE() {
    require(4);
    const std::uint32_t value = static_cast<std::uint32_t>(_data[_position]) |
                                static_cast<std::uint32_t>(_data[_position + 1]) << 8 |
                                static_cast<std::uint32_t>(_data[_position + 2]) << 16 |
                                static_cast<std::uint32_t>(_data[_position + 3]) << 24;
    _position += 4;
    return value;
}

Bytes BinaryReader::readBytes(std::size_t count) {
    require(count);
    Bytes result(_data + _position, _data + _position + count);
    _position += count;
    return result;
}

void BinaryReader::skip(std::size_t count) {
    require(count);
    _position += count;
}

Bytes readFile(const std::string &path) {
    std::ifstream input(path.c_str(), std::ios::binary);
    if (!input) {
        throw FormatError(path + ": cannot open file");
    }

    input.seekg(0, std::ios::end);
    const std::streamoff end = input.tellg();
    if (end < 0) {
        throw FormatError(path + ": cannot determine file size");
    }
    input.seekg(0, std::ios::beg);

    Bytes data(static_cast<std::size_t>(end));
    if (!data.empty()) {
        input.read(reinterpret_cast<char *>(&data[0]), static_cast<std::streamsize>(data.size()));
        if (!input) {
            throw FormatError(path + ": failed while reading file");
        }
    }
    return data;
}

} // namespace quiky
