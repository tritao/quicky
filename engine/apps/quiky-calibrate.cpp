#include "quiky/archive.h"
#include "quiky/map.h"
#include "quiky/runtime.h"

#include <algorithm>
#include <cstdlib>
#include <exception>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <string>
#include <vector>

namespace {

struct Sample {
    std::string phase;
    int step;
    std::int32_t x;
    std::int32_t y;
    std::int32_t velocityX;
    std::int32_t velocityY;
    bool grounded;
    bool facingRight;
};

void usage() {
    std::cerr << "usage: quiky-calibrate ARCHIVE [MAP-RESOURCE] "
                 "[START-X START-Y] [TRACE.csv]\n";
}

long parseNumber(const std::string &value, const char *name) {
    char *end = nullptr;
    const long result = std::strtol(value.c_str(), &end, 0);
    if (end == value.c_str() || *end != '\0') {
        throw quiky::FormatError(std::string("invalid ") + name + ": " + value);
    }
    return result;
}

void writeSample(std::vector<Sample> &samples, const std::string &phase, int step,
                 const quiky::PlayerState &player) {
    Sample sample;
    sample.phase = phase;
    sample.step = step;
    sample.x = player.x.floorPixels();
    sample.y = player.y.floorPixels();
    sample.velocityX = player.velocityX.raw;
    sample.velocityY = player.velocityY.raw;
    sample.grounded = player.grounded;
    sample.facingRight = player.facingRight;
    samples.push_back(sample);
}

void writeCsv(std::ostream &output, const std::vector<Sample> &samples) {
    output << "phase,step,x,y,vx_raw,vy_raw,grounded,facing\n";
    for (std::size_t index = 0; index < samples.size(); ++index) {
        const Sample &sample = samples[index];
        output << sample.phase << ',' << sample.step << ',' << sample.x << ','
               << sample.y << ',' << sample.velocityX << ',' << sample.velocityY
               << ',' << (sample.grounded ? 1 : 0) << ','
               << (sample.facingRight ? "right" : "left") << '\n';
    }
}

} // namespace

int main(int argc, char **argv) {
    if (argc < 2 || argc > 7) {
        usage();
        return EXIT_FAILURE;
    }

    try {
        const quiky::Archive archive = quiky::Archive::load(argv[1]);
        std::string mapName = "W1L1.MAP";
        std::int32_t startX = 100;
        std::int32_t startY = 100;
        std::string outputPath;

        const int optionalCount = argc - 2;
        if (optionalCount == 1) {
            mapName = argv[2];
        } else if (optionalCount == 3 || optionalCount == 4) {
            mapName = argv[2];
            const long parsedX = parseNumber(argv[3], "start X");
            const long parsedY = parseNumber(argv[4], "start Y");
            if (parsedX < 0 || parsedY < 0 || parsedX > 0x7fffffffL ||
                parsedY > 0x7fffffffL) {
                throw quiky::FormatError("start coordinates must be non-negative 32-bit values");
            }
            startX = static_cast<std::int32_t>(parsedX);
            startY = static_cast<std::int32_t>(parsedY);
            if (optionalCount == 4) {
                outputPath = argv[5];
            }
        } else if (optionalCount != 0) {
            usage();
            return EXIT_FAILURE;
        }

        const quiky::Map map = quiky::Map::parse(archive.read(mapName), mapName);
        quiky::PlayerSimulation simulation;
        quiky::PlayerState player;
        simulation.reset(player, startX, startY);
        std::vector<Sample> samples;
        samples.reserve(500);

        for (int frame = 0; frame < 120; ++frame) {
            simulation.tick(player, map, quiky::InputState());
            writeSample(samples, "settle", frame + 1, player);
        }
        const std::int32_t runStartX = player.x.floorPixels();
        std::int32_t maxSpeed = 0;
        for (int frame = 0; frame < 120; ++frame) {
            quiky::InputState input;
            input.right = true;
            simulation.tick(player, map, input);
            maxSpeed = std::max(maxSpeed, player.velocityX.raw);
            writeSample(samples, "run-right", frame + 1, player);
        }
        const std::int32_t runEndX = player.x.floorPixels();
        int stopFrame = -1;
        for (int frame = 0; frame < 120; ++frame) {
            simulation.tick(player, map, quiky::InputState());
            if (stopFrame < 0 && player.velocityX.raw == 0) {
                stopFrame = frame + 1;
            }
            writeSample(samples, "brake", frame + 1, player);
        }

        while (!player.grounded) {
            simulation.tick(player, map, quiky::InputState());
        }
        const int jumpStartY = player.y.floorPixels();
        const int jumpStartFrame = static_cast<int>(samples.size()) + 1;
        quiky::InputState jump;
        jump.jump = true;
        simulation.tick(player, map, jump);
        writeSample(samples, "jump", 1, player);
        int apexY = player.y.floorPixels();
        int landingFrame = -1;
        for (int frame = 1; frame < 180; ++frame) {
            simulation.tick(player, map, quiky::InputState());
            apexY = std::min(apexY, player.y.floorPixels());
            if (landingFrame < 0 && player.grounded) {
                landingFrame = frame + 1;
            }
            writeSample(samples, "jump", frame + 1, player);
            if (landingFrame >= 0) {
                break;
            }
        }

        if (outputPath.empty()) {
            writeCsv(std::cout, samples);
        } else {
            std::ofstream output(outputPath.c_str());
            if (!output) {
                throw quiky::FormatError(outputPath + ": cannot open calibration output");
            }
            writeCsv(output, samples);
        }

        std::cerr << std::fixed << std::setprecision(3)
                  << "calibration map=" << mapName
                  << " settle_grounded=" << (samples[119].grounded ? "yes" : "no")
                  << " run_displacement=" << (runEndX - runStartX)
                  << " max_speed_pixels_per_frame="
                  << static_cast<double>(maxSpeed) / quiky::Fixed16::kOne
                  << " stop_frame=" << stopFrame
                  << " jump_start_y=" << jumpStartY
                  << " jump_apex_y=" << apexY
                  << " jump_landing_frame=" << landingFrame
                  << " jump_trace_frame=" << jumpStartFrame << '\n';
    } catch (const std::exception &error) {
        std::cerr << "error: " << error.what() << '\n';
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}
