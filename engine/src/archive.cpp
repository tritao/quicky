#include "quiky/archive.h"

#include "quiky/binary_reader.h"

#include <algorithm>
#include <cctype>
#include <sstream>

namespace quiky {

namespace {

std::string upperAscii(const std::string &value) {
    std::string result(value);
    for (std::size_t index = 0; index < result.size(); ++index) {
        result[index] = static_cast<char>(std::toupper(static_cast<unsigned char>(result[index])));
    }
    return result;
}

} // namespace

Archive::Archive(const Bytes &data, const std::string &source)
    : _data(data), _source(source), _directoryOffset(0), _entries() {
    if (_data.size() < 8) {
        throw FormatError(_source + ": archive is shorter than its trailer");
    }

    BinaryReader trailer(&_data[_data.size() - 8], 8, _source + " trailer");
    _directoryOffset = trailer.readU32LE();
    const std::uint32_t storedCountMinusOne = trailer.readU32LE();
    const std::size_t directoryEnd = _data.size() - 8;
    if (_directoryOffset > directoryEnd) {
        throw FormatError(_source + ": directory offset lies outside archive");
    }

    BinaryReader directory(&_data[_directoryOffset], directoryEnd - _directoryOffset,
                           _source + " directory");
    std::uint32_t previousOffset = 0;
    bool havePreviousOffset = false;
    while (directory.remaining() != 0) {
        const std::uint16_t nameLength = directory.readU16LE();
        if (nameLength == 0) {
            throw FormatError(_source + ": empty archive filename");
        }
        const Bytes nameBytes = directory.readBytes(nameLength);
        const std::uint32_t offset = directory.readU32LE();
        for (std::size_t index = 0; index < nameBytes.size(); ++index) {
            if (nameBytes[index] < 0x20 || nameBytes[index] > 0x7e) {
                throw FormatError(_source + ": archive filename is not ASCII");
            }
        }
        if (havePreviousOffset && offset < previousOffset) {
            throw FormatError(_source + ": archive payload offsets are not sorted");
        }
        if (offset > _directoryOffset) {
            throw FormatError(_source + ": archive payload offset lies in directory");
        }
        ArchiveEntry entry;
        entry.name.assign(nameBytes.begin(), nameBytes.end());
        entry.offset = offset;
        entry.size = 0;
        _entries.push_back(entry);
        previousOffset = offset;
        havePreviousOffset = true;
    }

    if (_entries.empty()) {
        throw FormatError(_source + ": archive has no entries");
    }
    if (storedCountMinusOne != _entries.size() - 1) {
        std::ostringstream message;
        message << _source << ": trailer entry count mismatch (stored "
                << storedCountMinusOne << ", parsed " << (_entries.size() - 1) << ")";
        throw FormatError(message.str());
    }

    for (std::size_t index = 0; index < _entries.size(); ++index) {
        const std::uint32_t end = index + 1 < _entries.size()
                                      ? _entries[index + 1].offset
                                      : _directoryOffset;
        if (end < _entries[index].offset) {
            throw FormatError(_source + ": archive entry has negative size");
        }
        _entries[index].size = end - _entries[index].offset;
    }
}

Archive Archive::load(const std::string &path) {
    return Archive(readFile(path), path);
}

Archive Archive::fromBytes(const Bytes &data, const std::string &source) {
    return Archive(data, source);
}

const ArchiveEntry &Archive::find(const std::string &name) const {
    const std::string wanted = upperAscii(name);
    for (std::size_t index = 0; index < _entries.size(); ++index) {
        if (upperAscii(_entries[index].name) == wanted) {
            return _entries[index];
        }
    }
    throw FormatError(_source + ": archive entry not found: " + name);
}

Bytes Archive::read(const std::string &name) const {
    const ArchiveEntry &entry = find(name);
    return Bytes(_data.begin() + entry.offset,
                 _data.begin() + entry.offset + entry.size);
}

} // namespace quiky
