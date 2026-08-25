#include "quiky/trace.h"

#include <iostream>

int main(int argc, char **argv) {
    if (argc != 3) {
        std::cerr << "usage: quiky-trace-compare EXPECTED.trace ACTUAL.trace\n";
        return 2;
    }

    const quiky::TraceDifference difference =
        quiky::TraceComparator::compareFiles(argv[1], argv[2]);
    if (difference.equal) {
        std::cout << "traces equal\n";
        return 0;
    }

    std::cout << "first divergence: frame " << difference.frameIndex
              << ", tick " << difference.tick << ", field "
              << difference.field << "\n"
              << "  expected: " << difference.expected << "\n"
              << "  actual:   " << difference.actual << "\n";
    return 1;
}
