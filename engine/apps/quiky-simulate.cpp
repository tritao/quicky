#include "quiky/archive.h"
#include "quiky/map.h"
#include "quiky/player_update.h"
#include "quiky/simulation.h"
#include "quiky/world_view.h"

#include <cstdlib>
#include <exception>
#include <iostream>

namespace {

void usage() {
    std::cerr << "usage: quiky-simulate ARCHIVE MAP-RESOURCE START-X START-Y FRAMES [ACTION-FLAGS]\n";
}

long parseNumber(const char *value, const char *name) {
    char *end = nullptr;
    const long result = std::strtol(value, &end, 0);
    if (end == value || *end != '\0') {
        throw quiky::FormatError(std::string("invalid ") + name + ": " + value);
    }
    return result;
}

} // namespace

int main(int argc, char **argv) {
    if (argc != 6 && argc != 7) {
        usage();
        return EXIT_FAILURE;
    }

    try {
        const quiky::Archive archive = quiky::Archive::load(argv[1]);
        const std::string mapName(argv[2]);
        const quiky::Map map = quiky::Map::parse(archive.read(mapName), mapName);
        const long startX = parseNumber(argv[3], "start X");
        const long startY = parseNumber(argv[4], "start Y");
        const long frames = parseNumber(argv[5], "frame count");
        const long actionFlags = argc == 7 ? parseNumber(argv[6], "action flags") : 0;
        if (frames < 0 || actionFlags < 0 || actionFlags > 0xffff) {
            throw quiky::FormatError("frame count and action flags must be non-negative");
        }

        quiky::Simulation simulation;
        quiky::ExperimentalHorizontalPlayerUpdate playerUpdater;
        simulation.setExperimentalPlayerUpdater(&playerUpdater);
        simulation.reset();
        quiky::PlayerRecord &setup = simulation.stateForSetup().player;
        setup.initializeConfirmedHorizontalFields();
        setup.positionX = quiky::Fixed16::fromPixels(static_cast<std::int32_t>(startX));
        setup.positionY = quiky::Fixed16::fromPixels(static_cast<std::int32_t>(startY));
        setup.syncToRaw();
        const quiky::PlayerDescriptorTable descriptors =
            quiky::playerDescriptorTableForWorld(mapName.substr(0, 2));
        const quiky::WorldCollisionView world(map, &descriptors);
        quiky::SimulationOutput output;
        const quiky::InputState input = quiky::InputState::fromActionFlags(
            static_cast<std::uint16_t>(actionFlags));
        for (long frame = 0; frame < frames; ++frame) {
            simulation.tick(input, world, output);
        }
        const quiky::PlayerRecord &player = output.player;

        std::cout << mapName << ": frames=" << frames
                  << " x=" << player.positionX.floorPixels()
                  << " y=" << player.positionY.floorPixels()
                  << " vx=" << player.velocityX.raw
                  << " vy=" << player.velocityY.raw
                  << " mode=" << static_cast<int>(player.mode37)
                  << " facing=" << (player.motionDirectionByte29 == 0xff
                                          ? "left" : "right") << "\n";
    } catch (const std::exception &error) {
        std::cerr << "error: " << error.what() << "\n";
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}
