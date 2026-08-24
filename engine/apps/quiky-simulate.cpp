#include "quiky/archive.h"
#include "quiky/map.h"
#include "quiky/runtime.h"

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

        quiky::PlayerSimulation simulation;
        quiky::PlayerState player;
        simulation.reset(player, static_cast<std::int32_t>(startX),
                          static_cast<std::int32_t>(startY));
        const quiky::InputState input = quiky::InputState::fromActionFlags(
            static_cast<std::uint16_t>(actionFlags));
        for (long frame = 0; frame < frames; ++frame) {
            simulation.tick(player, map, input);
        }

        std::cout << mapName << ": frames=" << frames
                  << " x=" << player.x.floorPixels()
                  << " y=" << player.y.floorPixels()
                  << " vx=" << player.velocityX.raw
                  << " vy=" << player.velocityY.raw
                  << " grounded=" << (player.grounded ? "yes" : "no")
                  << " facing=" << (player.facingRight ? "right" : "left") << "\n";
    } catch (const std::exception &error) {
        std::cerr << "error: " << error.what() << "\n";
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}
