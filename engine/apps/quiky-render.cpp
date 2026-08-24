#include "quiky/archive.h"
#include "quiky/map.h"
#include "quiky/palette.h"
#include "quiky/renderer.h"
#include "quiky/tileset.h"

#include <cstdlib>
#include <cctype>
#include <exception>
#include <iostream>

namespace {

void usage() {
    std::cerr << "usage: quiky-render ARCHIVE MAP-RESOURCE OUTPUT.BMP\n";
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

} // namespace

int main(int argc, char **argv) {
    if (argc != 4) {
        usage();
        return EXIT_FAILURE;
    }

    try {
        const quiky::Archive archive = quiky::Archive::load(argv[1]);
        const std::string mapName(argv[2]);
        const std::string world = worldFor(mapName);
        const quiky::Map map = quiky::Map::parse(archive.read(mapName), mapName);
        const quiky::Palette palette = quiky::Palette::parsePcx(
            archive.read(world + ".PCC"), world + ".PCC");
        const quiky::Tileset tileset = quiky::Tileset::parseIco(
            archive.read(world + ".ICO"), world + ".ICO");
        const quiky::IndexedSurface surface = quiky::renderMap(map, tileset);
        quiky::writeBmp(argv[3], surface, palette);
        std::cout << mapName << ": " << map.width << "x" << map.height
                  << " tiles, output " << surface.width << "x" << surface.height
                  << " pixels\n";
    } catch (const std::exception &error) {
        std::cerr << "error: " << error.what() << "\n";
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}
