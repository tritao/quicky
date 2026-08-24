#include "quiky/types.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <exception>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <limits>
#include <sstream>
#include <string>
#include <vector>

namespace {

struct TraceSample {
    std::string phase;
    int step;
    int x;
    int y;
    std::int32_t velocityX;
    std::int32_t velocityY;
    bool grounded;
    bool facingRight;
};

struct Metric {
    double sum;
    double maximum;

    Metric() : sum(0.0), maximum(0.0) {}

    void add(double value) {
        sum += value;
        maximum = std::max(maximum, value);
    }

    double mean(std::size_t count) const {
        return count == 0 ? 0.0 : sum / static_cast<double>(count);
    }
};

struct Comparison {
    std::size_t compared;
    std::size_t keyMismatches;
    std::size_t groundedMismatches;
    std::size_t facingMismatches;
    Metric x;
    Metric y;
    Metric velocityX;
    Metric velocityY;
    std::string firstDifference;

    Comparison()
        : compared(0), keyMismatches(0), groundedMismatches(0),
          facingMismatches(0), x(), y(), velocityX(), velocityY(),
          firstDifference() {}

    bool differs() const {
        return keyMismatches != 0 || groundedMismatches != 0 ||
               facingMismatches != 0 || x.maximum != 0.0 || y.maximum != 0.0 ||
               velocityX.maximum != 0.0 || velocityY.maximum != 0.0;
    }
};

void usage() {
    std::cerr << "usage: quiky-compare-trace OBSERVED.csv REFERENCE.csv [--strict]\n"
                 "CSV header: phase,step,x,y,vx_raw,vy_raw,grounded,facing\n";
}

std::string trim(const std::string &value) {
    const std::string whitespace = " \t\r";
    const std::size_t first = value.find_first_not_of(whitespace);
    if (first == std::string::npos) {
        return std::string();
    }
    const std::size_t last = value.find_last_not_of(whitespace);
    return value.substr(first, last - first + 1);
}

std::vector<std::string> split(const std::string &line) {
    std::vector<std::string> fields;
    std::size_t start = 0;
    while (start <= line.size()) {
        const std::size_t comma = line.find(',', start);
        const std::size_t end = comma == std::string::npos ? line.size() : comma;
        fields.push_back(trim(line.substr(start, end - start)));
        if (comma == std::string::npos) {
            break;
        }
        start = comma + 1;
    }
    return fields;
}

long parseNumber(const std::string &value, const std::string &source,
                 std::size_t line, const char *field) {
    char *end = nullptr;
    const long result = std::strtol(value.c_str(), &end, 0);
    if (end == value.c_str() || *end != '\0') {
        std::ostringstream message;
        message << source << ':' << line << ": invalid " << field << " value '"
                << value << "'";
        throw quiky::FormatError(message.str());
    }
    return result;
}

bool parseBool(const std::string &value, const std::string &source,
               std::size_t line, const char *field) {
    if (value == "1" || value == "yes" || value == "true") {
        return true;
    }
    if (value == "0" || value == "no" || value == "false") {
        return false;
    }
    std::ostringstream message;
    message << source << ':' << line << ": invalid " << field << " value '"
            << value << "'";
    throw quiky::FormatError(message.str());
}

std::vector<TraceSample> loadTrace(const std::string &path) {
    std::ifstream input(path.c_str());
    if (!input) {
        throw quiky::FormatError(path + ": cannot open trace");
    }

    const std::string expectedHeader =
        "phase,step,x,y,vx_raw,vy_raw,grounded,facing";
    std::string line;
    std::size_t lineNumber = 0;
    bool headerSeen = false;
    std::vector<TraceSample> result;
    while (std::getline(input, line)) {
        ++lineNumber;
        line = trim(line);
        if (line.empty() || line[0] == '#') {
            continue;
        }
        if (!headerSeen) {
            if (line != expectedHeader) {
                throw quiky::FormatError(
                    path + ": unsupported calibration CSV header");
            }
            headerSeen = true;
            continue;
        }

        const std::vector<std::string> fields = split(line);
        if (fields.size() != 8) {
            std::ostringstream message;
            message << path << ':' << lineNumber << ": expected 8 CSV fields";
            throw quiky::FormatError(message.str());
        }
        TraceSample sample;
        sample.phase = fields[0];
        sample.step = static_cast<int>(parseNumber(fields[1], path, lineNumber, "step"));
        sample.x = static_cast<int>(parseNumber(fields[2], path, lineNumber, "x"));
        sample.y = static_cast<int>(parseNumber(fields[3], path, lineNumber, "y"));
        sample.velocityX = static_cast<std::int32_t>(
            parseNumber(fields[4], path, lineNumber, "vx_raw"));
        sample.velocityY = static_cast<std::int32_t>(
            parseNumber(fields[5], path, lineNumber, "vy_raw"));
        sample.grounded = parseBool(fields[6], path, lineNumber, "grounded");
        if (fields[7] != "left" && fields[7] != "right") {
            throw quiky::FormatError(path + ": invalid facing value at line " +
                                     std::to_string(lineNumber));
        }
        sample.facingRight = fields[7] == "right";
        result.push_back(sample);
    }
    if (!headerSeen || result.empty()) {
        throw quiky::FormatError(path + ": trace has no samples");
    }
    return result;
}

void noteFirstDifference(Comparison &comparison, std::size_t index,
                         const TraceSample &observed,
                         const TraceSample &reference,
                         const std::string &field) {
    if (!comparison.firstDifference.empty()) {
        return;
    }
    std::ostringstream message;
    message << "index=" << index << " phase=" << observed.phase
            << " step=" << observed.step << " field=" << field
            << " observed/reference differ";
    if (observed.phase != reference.phase || observed.step != reference.step) {
        message << " (reference phase=" << reference.phase
                << " step=" << reference.step << ')';
    }
    comparison.firstDifference = message.str();
}

Comparison compare(const std::vector<TraceSample> &observed,
                   const std::vector<TraceSample> &reference) {
    Comparison result;
    result.compared = std::min(observed.size(), reference.size());
    for (std::size_t index = 0; index < result.compared; ++index) {
        const TraceSample &left = observed[index];
        const TraceSample &right = reference[index];
        if (left.phase != right.phase || left.step != right.step) {
            ++result.keyMismatches;
            noteFirstDifference(result, index, left, right, "phase/step");
        }
        const double x = std::abs(left.x - right.x);
        const double y = std::abs(left.y - right.y);
        const double velocityX = std::abs(
            static_cast<double>(left.velocityX) - right.velocityX);
        const double velocityY = std::abs(
            static_cast<double>(left.velocityY) - right.velocityY);
        result.x.add(x);
        result.y.add(y);
        result.velocityX.add(velocityX);
        result.velocityY.add(velocityY);
        if (x != 0.0) {
            noteFirstDifference(result, index, left, right, "x");
        } else if (y != 0.0) {
            noteFirstDifference(result, index, left, right, "y");
        } else if (velocityX != 0.0) {
            noteFirstDifference(result, index, left, right, "vx_raw");
        } else if (velocityY != 0.0) {
            noteFirstDifference(result, index, left, right, "vy_raw");
        }
        if (left.grounded != right.grounded) {
            ++result.groundedMismatches;
            noteFirstDifference(result, index, left, right, "grounded");
        }
        if (left.facingRight != right.facingRight) {
            ++result.facingMismatches;
            noteFirstDifference(result, index, left, right, "facing");
        }
    }
    return result;
}

} // namespace

int main(int argc, char **argv) {
    if (argc != 3 && argc != 4) {
        usage();
        return EXIT_FAILURE;
    }

    try {
        const bool strict = argc == 4 && std::string(argv[3]) == "--strict";
        if (argc == 4 && !strict) {
            usage();
            return EXIT_FAILURE;
        }
        const std::vector<TraceSample> observed = loadTrace(argv[1]);
        const std::vector<TraceSample> reference = loadTrace(argv[2]);
        const Comparison result = compare(observed, reference);
        std::cout << std::fixed << std::setprecision(3)
                  << "observed_samples=" << observed.size()
                  << " reference_samples=" << reference.size()
                  << " compared=" << result.compared << '\n'
                  << "x_mae_px=" << result.x.mean(result.compared)
                  << " x_max_px=" << result.x.maximum << '\n'
                  << "y_mae_px=" << result.y.mean(result.compared)
                  << " y_max_px=" << result.y.maximum << '\n'
                  << "vx_mae_raw=" << result.velocityX.mean(result.compared)
                  << " vx_max_raw=" << result.velocityX.maximum << '\n'
                  << "vy_mae_raw=" << result.velocityY.mean(result.compared)
                  << " vy_max_raw=" << result.velocityY.maximum << '\n'
                  << "key_mismatches=" << result.keyMismatches
                  << " grounded_mismatches=" << result.groundedMismatches
                  << " facing_mismatches=" << result.facingMismatches << '\n';
        if (!result.firstDifference.empty()) {
            std::cout << "first_difference=" << result.firstDifference << '\n';
        }
        if (observed.size() != reference.size()) {
            std::cout << "sample_count_mismatch=1\n";
        }
        std::cout << "status=" << (result.differs() || observed.size() != reference.size()
                                        ? "different"
                                        : "match")
                  << '\n';
        if (strict && (result.differs() || observed.size() != reference.size())) {
            return EXIT_FAILURE;
        }
    } catch (const std::exception &error) {
        std::cerr << "error: " << error.what() << '\n';
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}
