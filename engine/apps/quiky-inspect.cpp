#include "quiky/archive.h"

#include <cstdlib>
#include <exception>
#include <iomanip>
#include <iostream>

namespace {

void usage() {
    std::cerr << "usage: quiky-inspect ARCHIVE list\n"
              << "       quiky-inspect ARCHIVE info RESOURCE\n";
}

} // namespace

int main(int argc, char **argv) {
    if (argc < 3 || argc > 4) {
        usage();
        return EXIT_FAILURE;
    }

    try {
        const quiky::Archive archive = quiky::Archive::load(argv[1]);
        const std::string command(argv[2]);
        if (command == "list" && argc == 3) {
            for (std::size_t index = 0; index < archive.entries().size(); ++index) {
                const quiky::ArchiveEntry &entry = archive.entries()[index];
                std::cout << std::setw(3) << index << " " << entry.name
                          << " offset=0x" << std::hex << entry.offset
                          << " size=0x" << entry.size << std::dec << " ("
                          << entry.size << ")\n";
            }
            return EXIT_SUCCESS;
        }
        if (command == "info" && argc == 4) {
            const quiky::ArchiveEntry &entry = archive.find(argv[3]);
            std::cout << entry.name << "\n"
                      << "offset: " << entry.offset << " (0x" << std::hex << entry.offset << std::dec << ")\n"
                      << "size: " << entry.size << "\n";
            return EXIT_SUCCESS;
        }
    } catch (const std::exception &error) {
        std::cerr << "error: " << error.what() << "\n";
        return EXIT_FAILURE;
    }

    usage();
    return EXIT_FAILURE;
}
