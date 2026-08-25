#include "quiky/trace.h"

#include <cstdlib>
#include <fstream>
#include <iomanip>
#include <sstream>

namespace quiky {
namespace {

std::string hexByte(byte value) {
    std::ostringstream stream;
    stream << std::hex << std::setfill('0') << std::setw(2)
           << static_cast<unsigned>(value);
    return stream.str();
}

std::string playerHex(const PlayerRawRecord &record) {
    std::ostringstream stream;
    for (std::size_t index = 0; index < PlayerRawRecord::kSize; ++index) {
        stream << hexByte(record.bytes[index]);
    }
    return stream.str();
}

template <typename T>
void appendNumber(std::ostringstream &stream, T value) {
    stream << value;
}

template <typename T>
void appendSeparated(std::ostringstream &stream, bool &first, T value) {
    if (!first) {
        stream << ';';
    }
    first = false;
    appendNumber(stream, value);
}

std::string mapLookups(const std::vector<TraceMapLookup> &lookups) {
    std::ostringstream stream;
    bool first = true;
    for (std::size_t index = 0; index < lookups.size(); ++index) {
        const TraceMapLookup &lookup = lookups[index];
        if (!first) {
            stream << ';';
        }
        first = false;
        stream << lookup.x << ':' << lookup.y << ':'
               << lookup.cell.inBounds << ':' << lookup.cell.rawWord << ':'
               << lookup.cell.tileId << ':' << lookup.cell.flags << ':'
               << lookup.descriptor.valid << ':'
               << lookup.descriptor.descriptorWord << ':'
               << lookup.descriptor.runtimeFlags;
    }
    return stream.str();
}

std::string callbacks(const std::vector<SchedulerInvocation> &items) {
    std::ostringstream stream;
    bool first = true;
    for (std::size_t index = 0; index < items.size(); ++index) {
        const SchedulerInvocation &item = items[index];
        if (!first) {
            stream << ';';
        }
        first = false;
        stream << item.slot << ':' << item.generation << ':'
               << item.callback.segment << ':' << item.callback.offset << ':'
               << item.playerCallback << ':' << item.cameraParticipating;
    }
    return stream.str();
}

std::string stateWrites(const std::vector<TraceStateWrite> &items) {
    std::ostringstream stream;
    bool first = true;
    for (std::size_t index = 0; index < items.size(); ++index) {
        const TraceStateWrite &item = items[index];
        if (!first) {
            stream << ';';
        }
        first = false;
        stream << item.offset << ':' << static_cast<unsigned>(item.width)
               << ':' << item.value;
    }
    return stream.str();
}

std::string events(const std::vector<SimulationEvent> &items) {
    std::ostringstream stream;
    bool first = true;
    for (std::size_t index = 0; index < items.size(); ++index) {
        const SimulationEvent &item = items[index];
        if (!first) {
            stream << ';';
        }
        first = false;
        stream << static_cast<unsigned>(item.kind) << ':' << item.tick << ':'
               << item.sourceId << ':' << item.code << ':' << item.value;
    }
    return stream.str();
}

bool nextDataLine(std::istream &stream, std::string &line) {
    while (std::getline(stream, line)) {
        if (!line.empty() && line[0] != '#') {
            return true;
        }
    }
    return false;
}

std::vector<std::string> splitFields(const std::string &line) {
    std::vector<std::string> fields;
    std::size_t start = 0;
    while (true) {
        const std::size_t comma = line.find(',', start);
        if (comma == std::string::npos) {
            fields.push_back(line.substr(start));
            return fields;
        }
        fields.push_back(line.substr(start, comma - start));
        start = comma + 1;
    }
}

std::uint64_t parseTick(const std::vector<std::string> &fields) {
    if (fields.empty()) {
        return 0;
    }
    return static_cast<std::uint64_t>(std::strtoull(fields[0].c_str(), 0, 10));
}

TraceDifference mismatch(std::size_t frameIndex,
                          std::uint64_t tick,
                          const std::string &field,
                          const std::string &expected,
                          const std::string &actual) {
    TraceDifference result;
    result.equal = false;
    result.frameIndex = frameIndex;
    result.tick = tick;
    result.field = field;
    result.expected = expected;
    result.actual = actual;
    return result;
}

} // namespace

TraceFrame::TraceFrame()
    : tick(0),
      inputFlags(0),
      playerSelector(0),
      playerOffset(0),
      player(),
      mapLookups(),
      schedulerCallbacks(),
      stateWrites(),
      emittedEvents() {
}

const char *TraceWriter::schemaName() {
    return "quiky-trace-v1";
}

void TraceWriter::writeHeader() {
    _stream << "# schema=" << schemaName() << '\n';
    _stream << "tick,input_flags,player_selector,player_offset,"
                "raw_player_record,map_lookups,scheduler_callbacks,"
                "state_writes,emitted_events\n";
}

void TraceWriter::writeFrame(const TraceFrame &frame) {
    _stream << frame.tick << ',' << frame.inputFlags << ','
            << frame.playerSelector << ',' << frame.playerOffset << ','
            << playerHex(frame.player) << ',' << mapLookups(frame.mapLookups)
            << ',' << callbacks(frame.schedulerCallbacks) << ','
            << stateWrites(frame.stateWrites) << ',' << events(frame.emittedEvents)
            << '\n';
}

TraceDifference::TraceDifference()
    : equal(true), frameIndex(0), tick(0), field(), expected(), actual() {
}

TraceDifference TraceComparator::compare(std::istream &expected,
                                         std::istream &actual) {
    std::string expectedHeader;
    std::string actualHeader;
    if (!nextDataLine(expected, expectedHeader) ||
        !nextDataLine(actual, actualHeader)) {
        return mismatch(0, 0, "header", expectedHeader, actualHeader);
    }
    if (expectedHeader != actualHeader) {
        return mismatch(0, 0, "header", expectedHeader, actualHeader);
    }

    std::size_t frameIndex = 0;
    while (true) {
        std::string expectedLine;
        std::string actualLine;
        const bool hasExpected = nextDataLine(expected, expectedLine);
        const bool hasActual = nextDataLine(actual, actualLine);
        if (!hasExpected || !hasActual) {
            if (hasExpected != hasActual) {
                return mismatch(frameIndex, 0, "frame-count",
                                hasExpected ? expectedLine : "<eof>",
                                hasActual ? actualLine : "<eof>");
            }
            return TraceDifference();
        }

        const std::vector<std::string> expectedFields =
            splitFields(expectedLine);
        const std::vector<std::string> actualFields = splitFields(actualLine);
        const std::size_t fieldCount = expectedFields.size() < actualFields.size()
                                           ? expectedFields.size()
                                           : actualFields.size();
        const std::uint64_t tick = parseTick(expectedFields);
        for (std::size_t field = 0; field < fieldCount; ++field) {
            if (expectedFields[field] != actualFields[field]) {
                static const char *const names[] = {
                    "tick", "input_flags", "player_selector", "player_offset",
                    "raw_player_record", "map_lookups", "scheduler_callbacks",
                    "state_writes", "emitted_events"};
                const std::string name = field < 9 ? names[field] : "field";
                return mismatch(frameIndex, tick, name, expectedFields[field],
                                actualFields[field]);
            }
        }
        if (expectedFields.size() != actualFields.size()) {
            return mismatch(frameIndex, tick, "field-count",
                            std::string(), std::string());
        }
        ++frameIndex;
    }
}

TraceDifference TraceComparator::compareFiles(const std::string &expectedPath,
                                              const std::string &actualPath) {
    std::ifstream expected(expectedPath.c_str());
    std::ifstream actual(actualPath.c_str());
    if (!expected || !actual) {
        return mismatch(0, 0, "file", expected ? "" : expectedPath,
                        actual ? "" : actualPath);
    }
    return compare(expected, actual);
}

} // namespace quiky
