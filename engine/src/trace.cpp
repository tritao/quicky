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

std::string collisionProbes(const std::vector<CollisionProbe> &items) {
    std::ostringstream stream;
    bool first = true;
    for (std::size_t index = 0; index < items.size(); ++index) {
        const CollisionProbe &item = items[index];
        if (!first) {
            stream << ';';
        }
        first = false;
        stream << item.pixelX << ':' << item.pixelY << ':' << item.inBounds
               << ':' << item.mapWord << ':' << item.tileId << ':'
               << item.descriptorWord << ':'
               << static_cast<unsigned>(item.quadrantMask) << ':'
               << CollisionKernel::occupied(item);
    }
    return stream.str();
}

std::uint8_t hexValue(char value) {
    if (value >= '0' && value <= '9') {
        return static_cast<std::uint8_t>(value - '0');
    }
    if (value >= 'a' && value <= 'f') {
        return static_cast<std::uint8_t>(value - 'a' + 10);
    }
    if (value >= 'A' && value <= 'F') {
        return static_cast<std::uint8_t>(value - 'A' + 10);
    }
    return 0;
}

std::uint32_t rawWord(const std::string &hex,
                      std::size_t offset,
                      std::size_t width) {
    if (hex.size() < (offset + width) * 2) {
        return 0;
    }
    std::uint32_t value = 0;
    for (std::size_t index = 0; index < width; ++index) {
        const std::size_t position = (offset + index) * 2;
        value |= static_cast<std::uint32_t>(hexValue(hex[position]))
                 << (4 * index + 4);
        value |= static_cast<std::uint32_t>(hexValue(hex[position + 1])) << 4 * index;
    }
    return value;
}

std::int32_t signed32(std::uint32_t value) {
    if (value <= 0x7fffffffU) {
        return static_cast<std::int32_t>(value);
    }
    return static_cast<std::int32_t>(-2147483647 - 1) +
           static_cast<std::int32_t>(value - 0x80000000U);
}

std::string semanticFieldAt(std::size_t offset,
                            const std::string &hex,
                            std::string &decoded) {
    std::size_t base = offset;
    std::size_t width = 1;
    const char *name = 0;
    if (offset <= 0x01) {
        base = 0x00;
        width = 2;
        name = "action_word";
        decoded = std::to_string(rawWord(hex, base, width));
    } else if (offset >= 0x02 && offset <= 0x05) {
        base = 0x02;
        width = 4;
        name = "x_fixed";
        decoded = std::to_string(signed32(rawWord(hex, base, width)));
    } else if (offset >= 0x06 && offset <= 0x09) {
        base = 0x06;
        width = 4;
        name = "y_fixed";
        decoded = std::to_string(signed32(rawWord(hex, base, width)));
    } else if (offset >= 0x0a && offset <= 0x0d) {
        base = 0x0a;
        width = 4;
        name = "velocity_x_fixed";
        decoded = std::to_string(signed32(rawWord(hex, base, width)));
    } else if (offset >= 0x0e && offset <= 0x11) {
        base = 0x0e;
        width = 4;
        name = "velocity_y_fixed";
        decoded = std::to_string(signed32(rawWord(hex, base, width)));
    } else if (offset >= 0x12 && offset <= 0x13) {
        base = 0x12;
        width = 2;
        name = "status_word";
        decoded = std::to_string(rawWord(hex, base, width));
    } else if (offset >= 0x18 && offset <= 0x19) {
        base = 0x18;
        width = 2;
        name = "callback_offset";
        decoded = std::to_string(rawWord(hex, base, width));
    } else if (offset == 0x28) {
        name = "direction_byte";
        decoded = std::to_string(rawWord(hex, offset, 1));
    } else if (offset == 0x29) {
        name = "motion_direction_byte";
        decoded = std::to_string(rawWord(hex, offset, 1));
    } else if (offset == 0x2a) {
        name = "action_counter";
        decoded = std::to_string(rawWord(hex, offset, 1));
    } else if (offset == 0x37) {
        name = "signed_callback_mode";
        decoded = std::to_string(static_cast<std::int8_t>(rawWord(hex, offset, 1)));
    } else if (offset == 0x38) {
        name = "collision_gate";
        decoded = std::to_string(rawWord(hex, offset, 1));
    } else if (offset == 0x39) {
        name = "transition_pending";
        decoded = std::to_string(rawWord(hex, offset, 1));
    } else if (offset == 0x3a) {
        name = "vertical_response_latch";
        decoded = std::to_string(static_cast<std::int8_t>(rawWord(hex, offset, 1)));
    } else if (offset == 0x3b) {
        name = "side_response_latch";
        decoded = std::to_string(rawWord(hex, offset, 1));
    } else if (offset >= 0x4c && offset <= 0x4f) {
        base = 0x4c;
        width = 4;
        name = "acceleration_fixed";
        decoded = std::to_string(signed32(rawWord(hex, base, width)));
    } else if (offset >= 0x54 && offset <= 0x57) {
        base = 0x54;
        width = 4;
        name = "friction_fixed";
        decoded = std::to_string(signed32(rawWord(hex, base, width)));
    } else if (offset >= 0x5c && offset <= 0x5f) {
        base = 0x5c;
        width = 4;
        name = "horizontal_speed_cap_fixed";
        decoded = std::to_string(signed32(rawWord(hex, base, width)));
    } else if (offset == 0x72 || offset == 0x73) {
        base = 0x72;
        width = 2;
        name = "vertical_step_pixels";
        decoded = std::to_string(rawWord(hex, base, width));
    }
    (void)base;
    (void)width;
    return name == 0 ? "field_0x" + std::to_string(offset) : name;
}

std::string byteAtHex(const std::string &hex, std::size_t offset) {
    if (hex.size() < (offset + 1) * 2) {
        return "<missing>";
    }
    return "0x" + hex.substr(offset * 2, 2);
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
      sourceExperiment(),
      sequence(0),
      inputFlags(0),
      playerSelector(0),
      playerOffset(0),
      player(),
      mapLookups(),
      collisionProbes(),
      schedulerCallbacks(),
      stateWrites(),
      emittedEvents() {
}

const char *TraceWriter::schemaName() {
    return "quiky-trace-v2";
}

void TraceWriter::writeHeader() {
    _stream << "# schema=" << schemaName() << '\n';
    _stream << "tick,input_flags,source_experiment,sequence,"
                "player_selector,player_offset,raw_player_record,map_lookups,"
                "collision_probes,scheduler_callbacks,"
                "state_writes,emitted_events\n";
}

void TraceWriter::writeFrame(const TraceFrame &frame) {
    _stream << frame.tick << ',' << frame.inputFlags << ','
            << frame.sourceExperiment << ',' << frame.sequence << ','
            << frame.playerSelector << ',' << frame.playerOffset << ','
            << playerHex(frame.player) << ',' << mapLookups(frame.mapLookups)
            << ',' << collisionProbes(frame.collisionProbes) << ','
            << callbacks(frame.schedulerCallbacks) << ','
            << stateWrites(frame.stateWrites) << ',' << events(frame.emittedEvents)
            << '\n';
}

TraceDifference::TraceDifference()
    : equal(true),
      frameIndex(0),
      tick(0),
      inputFlags(0),
      sourceExperiment(),
      sequence(0),
      field(),
      expected(),
      actual(),
      hasRawOffset(false),
      rawOffset(0),
      semanticField(),
      decodedExpected(),
      decodedActual(),
      expectedCollisionProbes(),
      actualCollisionProbes() {
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
        const std::uint16_t inputFlags = expectedFields.size() > 1
                                             ? static_cast<std::uint16_t>(
                                                   std::strtoul(expectedFields[1].c_str(), 0, 10))
                                             : 0;
        const std::string source = expectedFields.size() > 2
                                       ? expectedFields[2]
                                       : std::string();
        const std::uint64_t sequence = expectedFields.size() > 3
                                           ? static_cast<std::uint64_t>(
                                                 std::strtoull(expectedFields[3].c_str(), 0, 10))
                                           : 0;
        for (std::size_t field = 0; field < fieldCount; ++field) {
            if (expectedFields[field] != actualFields[field]) {
                static const char *const names[] = {
                    "tick", "input_flags", "source_experiment", "sequence",
                    "player_selector", "player_offset", "raw_player_record",
                    "map_lookups", "collision_probes", "scheduler_callbacks",
                    "state_writes", "emitted_events"};
                TraceDifference result = mismatch(
                    frameIndex, tick, field < 12 ? names[field] : "field",
                    expectedFields[field], actualFields[field]);
                result.inputFlags = inputFlags;
                result.sourceExperiment = source;
                result.sequence = sequence;
                if (field == 6) {
                    const std::size_t limit = expectedFields[field].size() <
                                                      actualFields[field].size()
                                                  ? expectedFields[field].size()
                                                  : actualFields[field].size();
                    std::size_t differingByte = 0;
                    while (differingByte * 2 + 1 < limit &&
                           expectedFields[field][differingByte * 2] ==
                               actualFields[field][differingByte * 2] &&
                           expectedFields[field][differingByte * 2 + 1] ==
                               actualFields[field][differingByte * 2 + 1]) {
                        ++differingByte;
                    }
                    result.hasRawOffset = true;
                    result.rawOffset = differingByte;
                    result.expected = byteAtHex(expectedFields[field], differingByte);
                    result.actual = byteAtHex(actualFields[field], differingByte);
                    result.semanticField = semanticFieldAt(
                        differingByte, expectedFields[field], result.decodedExpected);
                    std::string ignored;
                    semanticFieldAt(differingByte, actualFields[field],
                                    result.decodedActual);
                }
                if (expectedFields.size() > 8 && actualFields.size() > 8) {
                    result.expectedCollisionProbes = expectedFields[8];
                    result.actualCollisionProbes = actualFields[8];
                }
                return result;
            }
        }
        if (expectedFields.size() != actualFields.size()) {
            TraceDifference result = mismatch(frameIndex, tick, "field-count",
                                              std::string(), std::string());
            result.inputFlags = inputFlags;
            result.sourceExperiment = source;
            result.sequence = sequence;
            return result;
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
