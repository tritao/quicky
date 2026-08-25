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
              << "  source: " << difference.sourceExperiment
              << ", sequence: " << difference.sequence
              << ", input_flags: 0x" << std::hex << difference.inputFlags
              << std::dec << "\n";
    if (difference.hasRawOffset) {
        std::cout << "  raw offset: 0x" << std::hex << difference.rawOffset
                  << std::dec << " (" << difference.semanticField << ")\n"
                  << "  expected byte: " << difference.expected
                  << " decoded: " << difference.decodedExpected << "\n"
                  << "  actual byte:   " << difference.actual
                  << " decoded: " << difference.decodedActual << "\n";
    } else {
        std::cout << "  expected: " << difference.expected << "\n"
                  << "  actual:   " << difference.actual << "\n";
    }
    if (!difference.expectedCollisionProbes.empty() ||
        !difference.actualCollisionProbes.empty()) {
        std::cout << "  expected collision probes: "
                  << difference.expectedCollisionProbes << "\n"
                  << "  actual collision probes:   "
                  << difference.actualCollisionProbes << "\n";
    }
    return 1;
}
