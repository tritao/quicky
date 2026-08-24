#include "quiky/archive.h"
#include "quiky/bob.h"
#include "quiky/palette.h"
#include "quiky/renderer.h"

#include <cstdlib>
#include <exception>
#include <iostream>

namespace {

void usage() {
    std::cerr << "usage: quiky-bob-sheet ARCHIVE BOB-RESOURCE PALETTE-RESOURCE OUTPUT.BMP [COLUMNS]\n";
}

} // namespace

int main(int argc, char **argv) {
    if (argc != 5 && argc != 6) {
        usage();
        return EXIT_FAILURE;
    }

    try {
        const quiky::Archive archive = quiky::Archive::load(argv[1]);
        const std::string bobName(argv[2]);
        const std::string paletteName(argv[3]);
        const quiky::Bob bob = quiky::Bob::parse(archive.read(bobName), bobName);
        quiky::Palette palette = quiky::Palette::parsePcx(
            archive.read(paletteName), paletteName);
        const std::uint32_t columns = argc == 6
            ? static_cast<std::uint32_t>(std::strtoul(argv[5], nullptr, 10))
            : 8;
        const quiky::IndexedSurface surface = quiky::renderBobSheet(bob, palette, columns);
        quiky::writeBmp(argv[4], surface, palette);
        std::cout << bobName << ": " << bob.records.size()
                  << " records, output " << surface.width << "x" << surface.height
                  << " pixels\n";
    } catch (const std::exception &error) {
        std::cerr << "error: " << error.what() << "\n";
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}
