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

namespace {

void usage() {
    std::cerr << "usage: quiky-player-trace ARCHIVE MAP-RESOURCE START-X "
                 "START-Y FRAMES ACTION-FLAGS OUTPUT.JSON\n";
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

void writeSample(std::ostream &output,
                 std::size_t sequence,
                 const quiky::PlayerUpdateTrace &trace) {
    output << "{\"sequence\":" << sequence
           << ",\"player_callback\":{";
    output << "\"pre_record_hex\":\""
           << hexBytes(trace.preState.toBytes())
           << "\",\"post_record_hex\":\""
           << hexBytes(trace.postState.toBytes()) << "\",\"global_writes\":[";
    for (std::size_t index = 0; index < trace.globalWrites.size(); ++index) {
        if (index != 0) output << ',';
        writeGlobal(output, trace.globalWrites[index]);
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
    if (argc != 8) {
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

        quiky::PlayerRecord player;
        player.initializeConfirmedHorizontalFields();
        player.positionX = quiky::Fixed16::fromPixels(
            static_cast<std::int32_t>(startX));
        player.positionY = quiky::Fixed16::fromPixels(
            static_cast<std::int32_t>(startY));
        player.syncToRaw();
        quiky::TraceClosedPlayerUpdate updater;
        const quiky::InputState input = quiky::InputState::fromActionFlags(
            static_cast<std::uint16_t>(actionFlags));

        std::ofstream output(argv[7]);
        if (!output) {
            throw quiky::FormatError(std::string("cannot open trace output: ") +
                                     argv[7]);
        }
        output << "{\"trace_schema_version\":2,\"samples\":[";
        for (long frame = 0; frame < frames; ++frame) {
            if (frame != 0) output << ',';
            quiky::PlayerUpdateTrace trace;
            updater.updatePlayer(player, input, worldView, &trace);
            writeSample(output, static_cast<std::size_t>(frame + 1), trace);
        }
        output << "]}\n";
    } catch (const std::exception &error) {
        std::cerr << "error: " << error.what() << "\n";
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}
