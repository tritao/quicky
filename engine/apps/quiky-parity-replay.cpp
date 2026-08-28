#include "quiky/archive.h"
#include "quiky/level_runtime.h"
#include "quiky/player_update.h"
#include "quiky/runtime.h"
#include "quiky/simulation.h"

#include <array>
#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <exception>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <map>
#include <sstream>
#include <string>
#include <vector>

namespace {

struct InputFrame {
    std::size_t sequence;
    std::size_t guestFrame;
    std::uint16_t actionFlags;
    bool hasCamera;
    std::int32_t cameraX;
    std::int32_t cameraY;

    InputFrame(std::size_t sequenceValue = 0,
               std::uint16_t actionFlagsValue = 0)
        : sequence(sequenceValue), actionFlags(actionFlagsValue),
          guestFrame(sequenceValue),
          hasCamera(false), cameraX(0), cameraY(0) {}
};

void usage() {
    std::cerr << "usage: quiky-parity-replay ARCHIVE MAP-RESOURCE OUTPUT.JSON "
                 "[--frames N] [--action-flags N] "
                 "[--input-jsonl PATH] "
                 "[--player-bob RESOURCE] "
                 "[--camera-x N --camera-y N] "
                 "[--startup-camera-sweep-from N --startup-camera-sweep-to N] "
                 "[--leaf-prng-index N --leaf-prng-ring-hex HEX]\n";
}

std::string playerBobFor(const std::string &mapName) {
    if (mapName.size() < 2 || (mapName[0] != 'W' && mapName[0] != 'w') ||
            !std::isdigit(static_cast<unsigned char>(mapName[1]))) {
        throw quiky::FormatError(
            "cannot derive player BOB from MAP resource name: " + mapName);
    }
    return std::string("QUIKYW") + mapName[1] + ".BOB";
}

long parseNumber(const std::string &value, const char *name) {
    char *end = 0;
    const long result = std::strtol(value.c_str(), &end, 0);
    if (end == value.c_str() || *end != '\0') {
        throw quiky::FormatError(std::string("invalid ") + name + ": " + value);
    }
    return result;
}

unsigned hexDigit(char value) {
    if (value >= '0' && value <= '9') return static_cast<unsigned>(value - '0');
    if (value >= 'a' && value <= 'f') {
        return static_cast<unsigned>(value - 'a' + 10);
    }
    if (value >= 'A' && value <= 'F') {
        return static_cast<unsigned>(value - 'A' + 10);
    }
    throw quiky::FormatError("leaf PRNG ring contains a non-hex digit");
}

std::array<std::uint8_t, 0x100> parseLeafPrngRing(
    const std::string &value) {
    if (value.size() != 0x200) {
        throw quiky::FormatError(
            "leaf PRNG ring must contain exactly 256 bytes of hex");
    }
    std::array<std::uint8_t, 0x100> result;
    for (std::size_t index = 0; index < result.size(); ++index) {
        result[index] = static_cast<std::uint8_t>(
            (hexDigit(value[index * 2]) << 4) |
            hexDigit(value[index * 2 + 1]));
    }
    return result;
}

std::string hexBytes(const quiky::Bytes &bytes) {
    std::ostringstream output;
    output << std::hex << std::setfill('0');
    for (std::size_t index = 0; index < bytes.size(); ++index) {
        output << std::setw(2) << static_cast<unsigned>(bytes[index]);
    }
    return output.str();
}

void writeProbe(std::ostream &output, const quiky::CollisionProbe &probe,
                bool occupied) {
    output << "{\"x\":" << probe.pixelX
           << ",\"y\":" << probe.pixelY
           << ",\"map_word\":" << probe.mapWord
           << ",\"tile_id\":" << probe.tileId
           << ",\"descriptor_word\":" << probe.descriptorWord
           << ",\"quadrant_mask\":"
           << static_cast<unsigned>(probe.quadrantMask)
           << ",\"occupied\":" << (occupied ? "true" : "false") << "}";
}

long jsonInteger(const std::string &line, const char *key, bool &present) {
    const std::string needle = std::string("\"") + key + "\"";
    const std::size_t keyPosition = line.find(needle);
    if (keyPosition == std::string::npos) {
        present = false;
        return 0;
    }
    const std::size_t colon = line.find(':', keyPosition + needle.size());
    if (colon == std::string::npos) {
        throw quiky::FormatError(std::string("input JSONL field has no colon: ") + key);
    }
    const char *start = line.c_str() + colon + 1;
    while (*start == ' ' || *start == '\t') ++start;
    char *end = 0;
    const long result = std::strtol(start, &end, 0);
    if (end == start) {
        throw quiky::FormatError(std::string("input JSONL field is not an integer: ") + key);
    }
    present = true;
    return result;
}

void validateInputJsonShape(const std::string &line) {
    if (line.size() < 2 || line.front() != '{' || line.back() != '}') {
        throw quiky::FormatError("input JSONL row must be one JSON object");
    }
    const char *allowed[] = {
        "sequence", "guest_frame", "input_flags", "camera", "x", "y"
    };
    std::size_t position = 0;
    std::map<std::string, unsigned> counts;
    while ((position = line.find('"', position)) != std::string::npos) {
        const std::size_t end = line.find('"', position + 1);
        if (end == std::string::npos) {
            throw quiky::FormatError("input JSONL contains an unterminated key");
        }
        const std::string key = line.substr(position + 1, end - position - 1);
        bool known = false;
        for (std::size_t index = 0; index < sizeof(allowed) / sizeof(allowed[0]); ++index) {
            if (key == allowed[index]) known = true;
        }
        if (!known) {
            throw quiky::FormatError("input JSONL contains unknown field: " + key);
        }
        if (++counts[key] != 1) {
            throw quiky::FormatError("input JSONL contains duplicate field: " + key);
        }
        position = end + 1;
    }
    for (const char *key : {"sequence", "guest_frame", "input_flags"}) {
        if (counts[key] != 1) {
            throw quiky::FormatError(std::string("input JSONL is missing field: ") + key);
        }
    }
    const bool camera = counts["camera"] == 1;
    if ((counts["x"] == 1 || counts["y"] == 1) != camera ||
            counts["x"] != counts["y"]) {
        throw quiky::FormatError("input JSONL camera requires exactly x and y");
    }
}

std::vector<InputFrame> readInputJsonl(const std::string &path) {
    std::ifstream input(path.c_str());
    if (!input) {
        throw quiky::FormatError("cannot open input JSONL: " + path);
    }
    std::vector<InputFrame> frames;
    std::string line;
    std::size_t previousSequence = 0;
    while (std::getline(input, line)) {
        if (line.empty()) {
            throw quiky::FormatError("input JSONL contains a blank line");
        }
        validateInputJsonShape(line);
        bool hasSequence = false;
        bool hasFlags = false;
        const long sequence = jsonInteger(line, "sequence", hasSequence);
        const long flags = jsonInteger(line, "input_flags", hasFlags);
        if (!hasSequence || sequence < 1 ||
            static_cast<std::size_t>(sequence) <= previousSequence) {
            throw quiky::FormatError(
                "input JSONL sequences must be strictly increasing positive integers");
        }
        if (!hasFlags || flags < 0 || flags > 0xffff) {
            throw quiky::FormatError("input JSONL input_flags must be a uint16");
        }
        InputFrame frame(static_cast<std::size_t>(sequence),
                         static_cast<std::uint16_t>(flags));
        bool hasGuestFrame = false;
        const long guestFrame = jsonInteger(line, "guest_frame", hasGuestFrame);
        if (!hasGuestFrame || guestFrame < 0) {
            throw quiky::FormatError(
                "input JSONL guest_frame must be a non-negative integer");
        }
        frame.guestFrame = static_cast<std::size_t>(guestFrame);
        bool hasCamera = false;
        bool hasCameraX = false;
        bool hasCameraY = false;
        const std::size_t cameraPosition = line.find("\"camera\"");
        if (cameraPosition != std::string::npos) {
            frame.cameraX = static_cast<std::int32_t>(
                jsonInteger(line.substr(cameraPosition), "x", hasCameraX));
            frame.cameraY = static_cast<std::int32_t>(
                jsonInteger(line.substr(cameraPosition), "y", hasCameraY));
            hasCamera = hasCameraX && hasCameraY;
            if (!hasCamera) {
                throw quiky::FormatError(
                    "input JSONL camera requires integer x and y");
            }
        }
        frame.hasCamera = hasCamera;
        frames.push_back(frame);
        previousSequence = static_cast<std::size_t>(sequence);
    }
    if (frames.empty()) {
        throw quiky::FormatError("input JSONL contains no rows");
    }
    return frames;
}

void writeSample(std::ostream &output, const InputFrame &input,
                 const quiky::Simulation &simulation,
                 const quiky::SimulationOutput &snapshot,
                 const quiky::PlayerUpdateTrace &trace,
                 const quiky::LevelRuntime &runtime) {
    const quiky::LevelSession &session = runtime.session();
    output << "{\"schema\":\"quiky.parity-state-v2\",\"sequence\":"
           << input.sequence << ",\"pre_record\":\""
           << hexBytes(trace.preState.toBytes()) << "\",\"post_record\":\""
           << hexBytes(trace.postState.toBytes()) << "\",\"input_flags\":"
           << input.actionFlags << ",\"camera\":{\"x\":"
           << session.streamAnchorX() << ",\"y\":" << session.streamAnchorY()
           << "},\"probes\":[";
    for (std::size_t index = 0; index < trace.collisionProbes.size(); ++index) {
        if (index != 0) output << ',';
        const bool occupied = index < trace.collisionOccupied.size()
            ? trace.collisionOccupied[index]
            : quiky::CollisionKernel::occupied(trace.collisionProbes[index]);
        writeProbe(output, trace.collisionProbes[index], occupied);
    }
    output << "],\"global_writes\":[";
    for (std::size_t index = 0; index < trace.globalWrites.size(); ++index) {
        if (index != 0) output << ',';
        const quiky::PlayerGlobalWrite &write = trace.globalWrites[index];
        output << "{\"offset\":" << write.address << ",\"width\":"
               << static_cast<unsigned>(write.width) << ",\"before\":"
               << write.before << ",\"after\":" << write.after << "}";
    }
    output << "],\"effects\":[";
    for (std::size_t index = 0; index < trace.effectDispatches.size(); ++index) {
        if (index != 0) output << ',';
        const quiky::PlayerEffectDispatch &effect = trace.effectDispatches[index];
        output << "{\"address\":" << effect.address
               << ",\"code\":" << effect.code << "}";
    }
    output << "],\"scheduler_callbacks\":[";
    for (std::size_t index = 0; index < snapshot.schedulerCallbacks.size(); ++index) {
        if (index != 0) output << ',';
        output << snapshot.schedulerCallbacks[index].callback.offset;
    }
    std::vector<const quiky::LevelEntity *> entities;
    for (std::size_t index = 0; index < session.entities().size(); ++index) {
        const std::uint16_t callback =
            session.entities()[index].updateCallback.offset;
        if (session.entities()[index].active && callback != 0 &&
            callback != 0x3FF8 && callback != 0xFFFF)
            entities.push_back(&session.entities()[index]);
    }
    std::sort(entities.begin(), entities.end(),
              [](const quiky::LevelEntity *left, const quiky::LevelEntity *right) {
        if (left->updateCallback.offset != right->updateCallback.offset)
            return left->updateCallback.offset < right->updateCallback.offset;
        if (left->x != right->x) return left->x < right->x;
        if (left->y != right->y) return left->y < right->y;
        return left->spriteSlot < right->spriteSlot;
    });
    output << "],\"active_objects\":[";
    for (std::size_t index = 0; index < entities.size(); ++index) {
        if (index != 0) output << ',';
        const quiky::LevelEntity &entity = *entities[index];
        output << "{\"callback\":" << entity.updateCallback.offset
               << ",\"x\":" << entity.x << ",\"y\":" << entity.y
               << ",\"sprite_slot\":" << entity.spriteSlot;
        if (entity.updateCallback.offset == 0x47E7) {
            output << ",\"velocity_y_fixed\":" << entity.ambientVelocityY.raw
                   << ",\"animation_delay\":" << entity.ambientAnimationDelay
                   << ",\"animation_cursor\":" << entity.ambientAnimationCursor;
        }
        output << "}";
    }
    const quiky::LevelGameplayState &gameplay = session.gameplayState();
    output << "],\"lifecycle\":{\"health\":" << gameplay.currentHealth8822
           << ",\"lives\":" << gameplay.lives880a << ",\"gate\":"
           << static_cast<std::int16_t>(gameplay.transitionGate89ea)
           << ",\"mode\":"
           << static_cast<unsigned>(static_cast<std::uint8_t>(snapshot.player.mode37))
           << ",\"position\":{\"x\":" << snapshot.player.positionX.floorPixels()
           << ",\"y\":" << snapshot.player.positionY.floorPixels() << "}}}";
}

} // namespace

int main(int argc, char **argv) {
    if (argc < 4) {
        usage();
        return EXIT_FAILURE;
    }

    try {
        const std::string archivePath(argv[1]);
        const std::string mapName(argv[2]);
        const std::string outputPath(argv[3]);
        std::string playerBobName = playerBobFor(mapName);
        long frameCount = 4;
        std::uint16_t actionFlags = 0;
        std::string inputJsonl;
        bool hasCamera = false;
        std::int32_t cameraX = 0;
        std::int32_t cameraY = 262;
        bool hasStartupCameraSweep = false;
        std::int32_t startupCameraFrom = 0;
        std::int32_t startupCameraTo = 0;
        bool hasLeafPrngIndex = false;
        bool hasLeafPrngRing = false;
        std::uint16_t leafPrngIndex = 0;
        std::array<std::uint8_t, 0x100> leafPrngRing;
        for (int index = 4; index < argc; ++index) {
            const std::string option(argv[index]);
            if (option == "--frames" && index + 1 < argc) {
                frameCount = parseNumber(argv[++index], "frame count");
            } else if (option == "--action-flags" && index + 1 < argc) {
                const long parsed = parseNumber(argv[++index], "action flags");
                if (parsed < 0 || parsed > 0xffff) {
                    throw quiky::FormatError("action flags outside uint16 range");
                }
                actionFlags = static_cast<std::uint16_t>(parsed);
            } else if (option == "--input-jsonl" && index + 1 < argc) {
                inputJsonl = argv[++index];
            } else if (option == "--player-bob" && index + 1 < argc) {
                playerBobName = argv[++index];
            } else if (option == "--camera-x" && index + 1 < argc) {
                cameraX = static_cast<std::int32_t>(
                    parseNumber(argv[++index], "camera X"));
                hasCamera = true;
            } else if (option == "--camera-y" && index + 1 < argc) {
                cameraY = static_cast<std::int32_t>(
                    parseNumber(argv[++index], "camera Y"));
                hasCamera = true;
            } else if (option == "--startup-camera-sweep-from" &&
                       index + 1 < argc) {
                startupCameraFrom = static_cast<std::int32_t>(
                    parseNumber(argv[++index], "startup camera from"));
                hasStartupCameraSweep = true;
            } else if (option == "--startup-camera-sweep-to" &&
                       index + 1 < argc) {
                startupCameraTo = static_cast<std::int32_t>(
                    parseNumber(argv[++index], "startup camera to"));
                hasStartupCameraSweep = true;
            } else if (option == "--leaf-prng-index" && index + 1 < argc) {
                const long parsed = parseNumber(
                    argv[++index], "leaf PRNG index");
                if (parsed < 0 || parsed > 0xff) {
                    throw quiky::FormatError(
                        "leaf PRNG index outside uint8 range");
                }
                leafPrngIndex = static_cast<std::uint16_t>(parsed);
                hasLeafPrngIndex = true;
            } else if (option == "--leaf-prng-ring-hex" && index + 1 < argc) {
                leafPrngRing = parseLeafPrngRing(argv[++index]);
                hasLeafPrngRing = true;
            } else {
                usage();
                return EXIT_FAILURE;
            }
        }
        if (frameCount < 0) {
            throw quiky::FormatError("frame count must be non-negative");
        }
        if (hasLeafPrngIndex != hasLeafPrngRing) {
            throw quiky::FormatError(
                "leaf PRNG index and ring must be supplied together");
        }

        std::vector<InputFrame> inputs;
        if (!inputJsonl.empty()) {
            inputs = readInputJsonl(inputJsonl);
        } else {
            for (long index = 0; index < frameCount; ++index) {
                InputFrame frame(static_cast<std::size_t>(index + 1),
                                 actionFlags);
                frame.hasCamera = hasCamera;
                frame.cameraX = cameraX;
                frame.cameraY = cameraY;
                inputs.push_back(frame);
            }
        }

        const quiky::Archive archive = quiky::Archive::load(archivePath);
        quiky::LevelSessionConfig config;
        if (hasLeafPrngIndex) {
            config.hasLeafPrngState = true;
            config.leafPrngIndex = leafPrngIndex;
            config.leafPrngRing = leafPrngRing;
        }
        std::unique_ptr<quiky::LevelRuntime> runtime =
            quiky::LevelRuntime::load(archive, mapName, playerBobName, config);
        quiky::Simulation simulation;
        quiky::TraceClosedPlayerUpdate updater;
        simulation.setExperimentalPlayerUpdater(&updater);
        if (hasStartupCameraSweep) {
            if (!hasCamera || startupCameraFrom < startupCameraTo) {
                throw quiky::FormatError(
                    "startup camera sweep requires camera Y and descending X bounds");
            }
            runtime->setStreamAnchor(startupCameraFrom, cameraY);
        }
        runtime->reset(simulation);
        if (hasStartupCameraSweep) {
            for (std::int32_t startupX = startupCameraFrom;
                 startupX >= startupCameraTo; --startupX) {
                runtime->setStreamAnchor(startupX, cameraY);
                runtime->session().updateStreaming(
                    simulation, startupX, cameraY);
            }
            runtime->setStreamAnchor(startupCameraTo, cameraY);
        } else if (hasCamera && !inputs.empty() && inputs[0].hasCamera) {
            runtime->setStreamAnchor(inputs[0].cameraX, inputs[0].cameraY);
        }

        std::ofstream output(outputPath.c_str());
        if (!output) {
            throw quiky::FormatError("cannot open trace output: " + outputPath);
        }
        for (std::size_t index = 0; index < inputs.size(); ++index) {
            const InputFrame &input = inputs[index];
            if (input.hasCamera) {
                runtime->setStreamAnchor(input.cameraX, input.cameraY);
            }
            const quiky::InputState state =
                quiky::InputState::fromActionFlags(input.actionFlags);
            quiky::PlayerUpdateTrace trace;
            simulation.setPlayerTraceSink(&trace);
            quiky::SimulationOutput snapshot;
            runtime->tick(simulation, state, snapshot);
            simulation.setPlayerTraceSink(0);

            std::vector<quiky::LevelEvent> levelEvents;
            while (runtime->session().hasPendingEvents()) {
                levelEvents.push_back(runtime->session().consumeEvent());
            }
            writeSample(output, input, simulation, snapshot, trace, *runtime);
            output << '\n';
        }
    } catch (const std::exception &error) {
        std::cerr << "error: " << error.what() << "\n";
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}
