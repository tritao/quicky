#include "quiky/archive.h"
#include "quiky/area.h"
#include "quiky/bob.h"
#include "quiky/map.h"
#include "quiky/palette.h"
#include "quiky/player_update.h"
#include "quiky/renderer.h"
#include "quiky/simulation.h"
#include "quiky/tileset.h"
#include "quiky/world_view.h"

#include <cctype>
#include <cstdlib>
#include <exception>
#include <iostream>

namespace {

void usage() {
    std::cerr << "usage: quiky-player-frame ARCHIVE MAP-RESOURCE BOB-RESOURCE "
                 "START-X START-Y FRAMES ACTION-FLAGS OUTPUT.BMP [--overlay-are]\n";
}

long parseNumber(const char *value, const char *name) {
    char *end = nullptr;
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
    world[1] = static_cast<char>(std::toupper(static_cast<unsigned char>(world[1])));
    return world;
}

std::string areaNameFor(const std::string &mapName) {
    const std::size_t extension = mapName.find_last_of('.');
    return mapName.substr(0, extension) + ".ARE";
}

const quiky::BobRecord &findPlayerRecord(const quiky::Bob &bob) {
    const quiky::BobRecord *match = nullptr;
    for (std::size_t index = 0; index < bob.records.size(); ++index) {
        if (bob.records[index].slot == 0) {
            if (match != nullptr) {
                throw quiky::FormatError("player BOB resource contains duplicate slot 0");
            }
            match = &bob.records[index];
        }
    }
    if (match == nullptr) {
        throw quiky::FormatError("player BOB resource has no slot 0");
    }
    return *match;
}

} // namespace

int main(int argc, char **argv) {
    if (argc != 9 && argc != 10) {
        usage();
        return EXIT_FAILURE;
    }

    try {
        const quiky::Archive archive = quiky::Archive::load(argv[1]);
        const std::string mapName(argv[2]);
        const std::string bobName(argv[3]);
        const std::string world = worldFor(mapName);
        const long startX = parseNumber(argv[4], "start X");
        const long startY = parseNumber(argv[5], "start Y");
        const long frames = parseNumber(argv[6], "frame count");
        const long actionFlags = parseNumber(argv[7], "action flags");
        if (frames < 0 || actionFlags < 0 || actionFlags > 0xffff) {
            throw quiky::FormatError("frame count and action flags must be non-negative");
        }
        if (argc == 10 && std::string(argv[9]) != "--overlay-are") {
            usage();
            return EXIT_FAILURE;
        }

        const quiky::Map map = quiky::Map::parse(archive.read(mapName), mapName);
        quiky::Palette palette = quiky::Palette::parsePcx(
            archive.read(world + ".PCC"), world + ".PCC");
        const quiky::Tileset tileset = quiky::Tileset::parseIco(
            archive.read(world + ".ICO"), world + ".ICO");
        const quiky::Bob bob = quiky::Bob::parse(archive.read(bobName), bobName);
        const quiky::BobRecord &playerRecord = findPlayerRecord(bob);

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
            quiky::playerDescriptorTableForWorld(world);
        const quiky::WorldCollisionView worldView(map, &descriptors);
        quiky::SimulationOutput output;
        const quiky::InputState input = quiky::InputState::fromActionFlags(
            static_cast<std::uint16_t>(actionFlags));
        for (long frame = 0; frame < frames; ++frame) {
            simulation.tick(input, worldView, output);
        }
        const quiky::PlayerRecord &player = output.player;

        quiky::IndexedSurface surface = quiky::renderMap(map, tileset);
        if (argc == 10) {
            const quiky::Area area = quiky::Area::parse(
                archive.read(areaNameFor(mapName)), areaNameFor(mapName));
            quiky::overlayArea(surface, palette, area);
        }
        quiky::drawBobRecord(surface, playerRecord,
                             player.positionX.floorPixels(),
                             player.positionY.floorPixels());
        quiky::writeBmp(argv[8], surface, palette);
        std::cout << mapName << ": player slot 0, frame " << frames
                  << ", anchor=" << player.positionX.floorPixels() << ","
                  << player.positionY.floorPixels() << ", origin="
                  << playerRecord.originX << "," << playerRecord.originY
                  << ", output=" << surface.width << "x" << surface.height << "\n";
    } catch (const std::exception &error) {
        std::cerr << "error: " << error.what() << "\n";
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}
