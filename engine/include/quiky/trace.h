#ifndef QUIKY_TRACE_H
#define QUIKY_TRACE_H

#include "quiky/player_record.h"
#include "quiky/collision_kernel.h"
#include "quiky/scheduler.h"
#include "quiky/simulation.h"
#include "quiky/world_view.h"

#include <cstddef>
#include <cstdint>
#include <istream>
#include <ostream>
#include <string>
#include <vector>

namespace quiky {

struct TraceMapLookup {
    std::int32_t x;
    std::int32_t y;
    MapCell cell;
    TileDescriptor descriptor;
};

struct TraceStateWrite {
    std::uint16_t offset;
    std::uint8_t width;
    std::uint32_t value;
};

// The normalized field vocabulary follows the research ledgers: tick,
// input, raw player record, MAP/descriptors, callback order, state writes,
// and emitted events. It is intentionally lossless for the currently known
// fields and does not assign meaning to unknown player bytes.
struct TraceFrame {
    std::uint64_t tick;
    std::string sourceExperiment;
    std::uint64_t sequence;
    std::uint16_t inputFlags;
    std::uint16_t playerSelector;
    std::uint16_t playerOffset;
    PlayerRawRecord player;
    std::vector<TraceMapLookup> mapLookups;
    std::vector<CollisionProbe> collisionProbes;
    std::vector<SchedulerInvocation> schedulerCallbacks;
    std::vector<TraceStateWrite> stateWrites;
    std::vector<SimulationEvent> emittedEvents;

    TraceFrame();
};

class TraceWriter {
public:
    static const char *schemaName();

    explicit TraceWriter(std::ostream &stream) : _stream(stream) {}

    void writeHeader();
    void writeFrame(const TraceFrame &frame);

private:
    std::ostream &_stream;
};

struct TraceDifference {
    bool equal;
    std::size_t frameIndex;
    std::uint64_t tick;
    std::uint16_t inputFlags;
    std::string sourceExperiment;
    std::uint64_t sequence;
    std::string field;
    std::string expected;
    std::string actual;
    bool hasRawOffset;
    std::size_t rawOffset;
    std::string semanticField;
    std::string decodedExpected;
    std::string decodedActual;
    std::string expectedCollisionProbes;
    std::string actualCollisionProbes;

    TraceDifference();
};

class TraceComparator {
public:
    static TraceDifference compare(std::istream &expected,
                                   std::istream &actual);
    static TraceDifference compareFiles(const std::string &expectedPath,
                                        const std::string &actualPath);
};

} // namespace quiky

#endif
