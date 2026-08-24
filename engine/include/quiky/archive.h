#ifndef QUIKY_ARCHIVE_H
#define QUIKY_ARCHIVE_H

#include "quiky/types.h"

#include <cstddef>
#include <string>
#include <vector>

namespace quiky {

struct ArchiveEntry {
    std::string name;
    std::uint32_t offset;
    std::uint32_t size;
};

class Archive {
public:
    static Archive load(const std::string &path);
    static Archive fromBytes(const Bytes &data, const std::string &source = "<memory>");

    const std::string &source() const { return _source; }
    const std::vector<ArchiveEntry> &entries() const { return _entries; }

    const ArchiveEntry &find(const std::string &name) const;
    Bytes read(const std::string &name) const;

private:
    Archive(const Bytes &data, const std::string &source);

    Bytes _data;
    std::string _source;
    std::uint32_t _directoryOffset;
    std::vector<ArchiveEntry> _entries;
};

} // namespace quiky

#endif
