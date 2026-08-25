#include "quiky/archive.h"
#include "quiky/map.h"
#include "quiky/player_update.h"
#include "quiky/runtime.h"
#include "quiky/world_view.h"

#include <cstdlib>
#include <cctype>
#include <exception>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

namespace {

void usage() {
    std::cerr << "usage: quiky-player-trace ARCHIVE MAP-RESOURCE START-X "
                 "START-Y FRAMES ACTION-FLAGS OUTPUT.JSON "
                 "[--replay-tsv MANIFEST]\n";
}

long parseNumber(const char *value, const char *name) {
    char *end = 0;
    const long result = std::strtol(value, &end, 0);
    if (end == value || *end != '\0') {
        throw quiky::FormatError(std::string("invalid ") + name + ": " + value);
    }
    return result;
}

std::string worldFor(const std::string &mapName) {
    if (mapName.size() < 2 || (mapName[0] != 'W' && mapName[0] != 'w')) {
        throw quiky::FormatError("cannot derive world from MAP resource name");
    }
    std::string world(mapName.substr(0, 2));
    world[0] = 'W';
    world[1] = static_cast<char>(
        std::toupper(static_cast<unsigned char>(world[1])));
    return world;
}

std::string hexBytes(const quiky::Bytes &bytes) {
    std::ostringstream output;
    output << std::hex << std::setfill('0');
    for (std::size_t index = 0; index < bytes.size(); ++index) {
        output << std::setw(2) << static_cast<unsigned>(bytes[index]);
    }
    return output.str();
}

std::int64_t parseSigned(const std::string &value,
                         const std::string &name) {
    if (value == "?") {
        throw quiky::FormatError("missing replay field: " + name);
    }
    char *end = 0;
    const long long result = std::strtoll(value.c_str(), &end, 0);
    if (end == value.c_str() || *end != '\0') {
        throw quiky::FormatError("invalid replay " + name + ": " + value);
    }
    return static_cast<std::int64_t>(result);
}

std::uint16_t parseUnsigned16(const std::string &value,
                              const std::string &name) {
    const std::int64_t parsed = parseSigned(value, name);
    if (parsed < 0 || parsed > 0xffff) {
        throw quiky::FormatError("replay " + name + " outside uint16 range");
    }
    return static_cast<std::uint16_t>(parsed);
}

std::int16_t parseSigned16(const std::string &value,
                           const std::string &name) {
    const std::int64_t parsed = parseSigned(value, name);
    if (parsed < -0x8000 || parsed > 0x7fff) {
        throw quiky::FormatError("replay " + name + " outside int16 range");
    }
    return static_cast<std::int16_t>(parsed);
}

std::int32_t parseSigned32(const std::string &value,
                           const std::string &name) {
    const std::int64_t parsed = parseSigned(value, name);
    if (parsed < static_cast<std::int64_t>(-0x80000000LL) ||
        parsed > static_cast<std::int64_t>(0x7fffffffLL)) {
        throw quiky::FormatError("replay " + name + " outside int32 range");
    }
    return static_cast<std::int32_t>(parsed);
}

quiky::PlayerRawRecord parseRecordHex(const std::string &value) {
    if (value.size() != quiky::PlayerRawRecord::kSize * 2) {
        throw quiky::FormatError("replay player record is not 0x78 bytes");
    }
    quiky::Bytes bytes;
    bytes.reserve(quiky::PlayerRawRecord::kSize);
    for (std::size_t index = 0; index < value.size(); index += 2) {
        const std::string pair = value.substr(index, 2);
        char *end = 0;
        const long parsed = std::strtol(pair.c_str(), &end, 16);
        if (end == pair.c_str() || *end != '\0' || parsed < 0 || parsed > 0xff) {
            throw quiky::FormatError("replay player record is not hexadecimal");
        }
        bytes.push_back(static_cast<quiky::byte>(parsed));
    }
    return quiky::PlayerRawRecord::fromBytes(bytes);
}

struct ReplayFrame {
    std::size_t sequence;
    std::uint16_t actionFlags;
    quiky::PlayerRawRecord record;
    std::vector<std::string> fields;
};

std::vector<ReplayFrame> readReplay(const std::string &path) {
    std::ifstream input(path.c_str());
    if (!input) {
        throw quiky::FormatError("cannot open replay manifest: " + path);
    }

    std::vector<ReplayFrame> frames;
    std::string line;
    std::size_t lineNumber = 0;
    while (std::getline(input, line)) {
        ++lineNumber;
        if (line.empty() || line[0] == '#') {
            continue;
        }
        std::istringstream row(line);
        std::vector<std::string> fields;
        std::string field;
        while (row >> field) {
            fields.push_back(field);
        }
        if (fields.size() != 22) {
            std::ostringstream message;
            message << "replay manifest line " << lineNumber
                    << " must contain 22 fields";
            throw quiky::FormatError(message.str());
        }
        const std::int64_t sequence = parseSigned(fields[0], "sequence");
        if (sequence < 0) {
            throw quiky::FormatError("replay sequence must be non-negative");
        }
        ReplayFrame frame;
        frame.sequence = static_cast<std::size_t>(sequence);
        frame.actionFlags = parseUnsigned16(fields[1], "action_flags");
        frame.record = parseRecordHex(fields[21]);
        frame.fields = fields;
        frames.push_back(frame);
    }
    if (frames.empty()) {
        throw quiky::FormatError("replay manifest has no frames");
    }
    return frames;
}

bool present(const std::string &value) {
    return value != "?";
}

void applyReplayGlobals(const ReplayFrame &frame,
                        quiky::PlayerCallbackGlobals &globals) {
    const std::vector<std::string> &f = frame.fields;
    if (present(f[2])) globals.deferredY8812 = parseSigned32(f[2], "deferred_y");
    if (present(f[3])) globals.externalXDelta8816 = parseSigned32(f[3], "external_x_delta");
    if (present(f[4])) globals.timerClear8810 = parseUnsigned16(f[4], "timer_clear");
    if (present(f[5])) globals.inputRunCounter4FEC = parseUnsigned16(f[5], "input_run_counter");
    if (present(f[6])) globals.horizontalAccumulator4FE2 = parseSigned32(f[6], "horizontal_accumulator");
    if (present(f[7])) globals.viewStateA4FE4 = parseSigned16(f[7], "view_state_a");
    if (present(f[8])) globals.viewStateB4FE6 = parseSigned16(f[8], "view_state_b");
    if (present(f[9])) globals.horizontalAccel4FE8 = parseSigned32(f[9], "horizontal_accel");
    if (present(f[10])) globals.idleCounter4FEE = parseUnsigned16(f[10], "idle_counter");
    if (present(f[11])) globals.actionLowCopy4FF0 = static_cast<std::uint8_t>(
        parseUnsigned16(f[11], "action_low_copy"));
    if (present(f[12])) globals.pendingEvent612E = parseUnsigned16(f[12], "pending_event");
    if (present(f[13])) globals.cameraX81C0 = parseUnsigned16(f[13], "camera_x");
    if (present(f[14])) globals.cameraY81C4 = parseUnsigned16(f[14], "camera_y");
    if (present(f[15])) globals.cameraYLimit81CC = parseUnsigned16(f[15], "camera_y_limit");
    if (present(f[16])) globals.actionSource656C = parseUnsigned16(f[16], "action_source");
    if (present(f[17])) globals.activationState85DA = parseSigned16(f[17], "activation_state");
    if (present(f[18])) globals.specialSpeedCapMode88B6 = parseUnsigned16(f[18], "speed_cap_mode");
    if (present(f[19])) globals.actionSuppressor89E6 = parseSigned16(f[19], "action_suppressor");
    if (present(f[20])) globals.collisionTransitionMode89EA = parseSigned16(f[20], "transition_mode");
}

void writeProbe(std::ostream &output,
                const quiky::CollisionProbe &probe,
                bool occupied) {
    output << "{\"x\":" << probe.pixelX
           << ",\"y\":" << probe.pixelY
           << ",\"cell_word\":" << probe.mapWord
           << ",\"tile_id\":" << probe.tileId
           << ",\"descriptor_word\":" << probe.descriptorWord
           << ",\"quadrant_mask\":"
           << static_cast<unsigned>(probe.quadrantMask)
           << ",\"occupied\":" << (occupied ? "true" : "false") << "}";
}

void writeGlobal(std::ostream &output, const quiky::PlayerGlobalWrite &write) {
    output << "{\"offset\":" << write.address
           << ",\"width\":" << static_cast<unsigned>(write.width)
           << ",\"before\":" << write.before
           << ",\"after\":" << write.after << "}";
}

void writeEffect(std::ostream &output,
                 const quiky::PlayerEffectDispatch &effect) {
    output << "{\"address\":" << effect.address
           << ",\"code\":" << effect.code << "}";
}

void writeSample(std::ostream &output,
                 std::size_t sequence,
                 const quiky::PlayerUpdateTrace &trace) {
    output << "{\"sequence\":" << sequence
           << ",\"player_callback\":{";
    output << "\"input_flags\":" << trace.inputFlags
           << ",\"pre_record_hex\":\""
           << hexBytes(trace.preState.toBytes())
           << "\",\"post_record_hex\":\""
           << hexBytes(trace.postState.toBytes()) << "\",\"global_writes\":[";
    for (std::size_t index = 0; index < trace.globalWrites.size(); ++index) {
        if (index != 0) output << ',';
        writeGlobal(output, trace.globalWrites[index]);
    }
    output << "],\"effects\":[";
    for (std::size_t index = 0; index < trace.effectDispatches.size(); ++index) {
        if (index != 0) output << ',';
        writeEffect(output, trace.effectDispatches[index]);
    }
    output << "],\"factory_event\":{\"created_objects\":[]},"
              "\"stages\":[";
    for (std::size_t index = 0; index < trace.stages.size(); ++index) {
        if (index != 0) output << ',';
        output << static_cast<unsigned>(trace.stages[index]);
    }
    output << "]},\"collisions\":[";
    for (std::size_t index = 0; index < trace.collisionProbes.size(); ++index) {
        if (index != 0) output << ',';
        const quiky::CollisionProbe &probe = trace.collisionProbes[index];
        const bool occupied = index < trace.collisionOccupied.size()
                                  ? trace.collisionOccupied[index]
                                  : quiky::CollisionKernel::occupied(probe);
        writeProbe(output, probe, occupied);
    }
    output << "]}";
}

} // namespace

int main(int argc, char **argv) {
    const bool replayMode = argc == 10 && std::string(argv[8]) == "--replay-tsv";
    if (argc != 8 && !replayMode) {
        usage();
        return EXIT_FAILURE;
    }

    try {
        const quiky::Archive archive = quiky::Archive::load(argv[1]);
        const std::string mapName(argv[2]);
        const quiky::Map map = quiky::Map::parse(
            archive.read(mapName), mapName);
        const long startX = parseNumber(argv[3], "start X");
        const long startY = parseNumber(argv[4], "start Y");
        const long frames = parseNumber(argv[5], "frame count");
        const long actionFlags = parseNumber(argv[6], "action flags");
        if (frames < 0 || actionFlags < 0 || actionFlags > 0xffff) {
            throw quiky::FormatError(
                "frame count and action flags must be non-negative");
        }

        const std::string world = worldFor(mapName);
        const quiky::PlayerDescriptorTable descriptors =
            quiky::playerDescriptorTableForWorld(world);
        const quiky::WorldCollisionView worldView(map, &descriptors);

        quiky::TraceClosedPlayerUpdate updater;

        std::ofstream output(argv[7]);
        if (!output) {
            throw quiky::FormatError(std::string("cannot open trace output: ") +
                                     argv[7]);
        }
        output << "{\"trace_schema_version\":3,\"samples\":[";
        if (replayMode) {
            const std::vector<ReplayFrame> replay = readReplay(argv[9]);
            for (std::size_t index = 0; index < replay.size(); ++index) {
                if (index != 0) output << ',';
                quiky::PlayerRecord player = quiky::PlayerRecord::fromRaw(
                    replay[index].record);
                applyReplayGlobals(replay[index], updater.globalsForSetup());
                quiky::PlayerUpdateTrace trace;
                updater.updatePlayer(
                    player,
                    quiky::InputState::fromActionFlags(replay[index].actionFlags),
                    worldView, &trace);
                writeSample(output, replay[index].sequence, trace);
            }
        } else {
            quiky::PlayerRecord player;
            player.initializeConfirmedHorizontalFields();
            player.positionX = quiky::Fixed16::fromPixels(
                static_cast<std::int32_t>(startX));
            player.positionY = quiky::Fixed16::fromPixels(
                static_cast<std::int32_t>(startY));
            player.syncToRaw();
            const quiky::InputState input = quiky::InputState::fromActionFlags(
                static_cast<std::uint16_t>(actionFlags));
            for (long frame = 0; frame < frames; ++frame) {
                if (frame != 0) output << ',';
                quiky::PlayerUpdateTrace trace;
                updater.updatePlayer(player, input, worldView, &trace);
                writeSample(output, static_cast<std::size_t>(frame + 1), trace);
            }
        }
        output << "]}\n";
    } catch (const std::exception &error) {
        std::cerr << "error: " << error.what() << "\n";
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}
