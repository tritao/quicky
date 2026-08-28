#include "quiky/archive.h"
#include "quiky/level_runtime.h"
#include "quiky/player_update.h"
#include "quiky/runtime.h"
#include "quiky/simulation.h"

#include <array>
#include <algorithm>
#include <cstdlib>
#include <exception>
#include <fstream>
#include <iomanip>
#include <iostream>
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
                 "[--camera-x N --camera-y N] "
                 "[--startup-camera-sweep-from N --startup-camera-sweep-to N] "
                 "[--leaf-prng-index N --leaf-prng-ring-hex HEX]\n";
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

std::string jsonEscape(const std::string &value) {
    std::string result;
    for (std::size_t index = 0; index < value.size(); ++index) {
        if (value[index] == '"' || value[index] == '\\') result += '\\';
        result += value[index];
    }
    return result;
}

void writeCallback(std::ostream &output,
                   const quiky::CallbackIdentity &callback) {
    output << "{\"segment\":" << callback.segment
           << ",\"offset\":" << callback.offset << "}";
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

void writeEntity(std::ostream &output, const quiky::LevelEntity &entity) {
    output << "{\"id\":" << entity.id
           << ",\"record_offset\":" << entity.recordOffset
           << ",\"type\":" << entity.type
           << ",\"region_x\":" << entity.regionX
           << ",\"region_y\":" << entity.regionY
           << ",\"x\":" << entity.x << ",\"y\":" << entity.y
           << ",\"x_fixed\":" << entity.positionX.raw
           << ",\"y_fixed\":" << entity.positionY.raw
           << ",\"velocity_x_fixed\":" << entity.velocityX.raw
           << ",\"velocity_y_fixed\":" << entity.velocityY.raw
           << ",\"phase\":" << static_cast<unsigned>(entity.phase)
           << ",\"kind\":" << static_cast<unsigned>(entity.kind)
           << ",\"callback\":";
    writeCallback(output, entity.updateCallback);
    output << ",\"scheduler_slot\":" << entity.schedulerHandle.slot
           << ",\"scheduler_generation\":"
           << entity.schedulerHandle.generation
           << ",\"sprite_slot\":" << entity.spriteSlot
           << ",\"effect_slot\":" << entity.effectSlot
           << ",\"animation_frame\":" << entity.animationFrame
           << ",\"ambient_velocity_y_fixed\":"
           << entity.ambientVelocityY.raw
           << ",\"ambient_origin_x\":" << entity.ambientOriginX
           << ",\"ambient_origin_y\":" << entity.ambientOriginY
           << ",\"ambient_timer\":" << entity.ambientTimer
           << ",\"ambient_animation_delay\":"
           << entity.ambientAnimationDelay
           << ",\"ambient_animation_cursor\":"
           << entity.ambientAnimationCursor
           << ",\"ambient_table\":"
           << static_cast<unsigned>(entity.ambientTable)
           // Address-qualified callback state is emitted verbatim for the
           // normal-enemy closure. These fields are intentionally not folded
           // into a generic enemy abstraction: the WURM2 trace needs to
           // distinguish byte-shaped state from fixed-point words and the
           // BUMP trace needs its independent +0x20/+0x24 timer/cursor.
           << ",\"enemy_phase_timer\":" << entity.enemyPhaseTimer
           << ",\"enemy_timer\":" << entity.enemyTimer
           << ",\"enemy_state\":" << entity.enemyState
           << ",\"enemy_orientation\":"
           << static_cast<int>(entity.enemyOrientation)
           << ",\"enemy_patrol_direction\":"
           << static_cast<int>(entity.enemyPatrolDirection)
           << ",\"enemy_transition_timer\":"
           << entity.enemyTransitionTimer
           << ",\"enemy_phase34\":" << entity.enemyPhase34
           << ",\"enemy_sine_or_probe39\":"
           << entity.enemySineOrProbe39
           << ",\"enemy_vertical_state36\":"
           << static_cast<int>(entity.enemyVerticalState36)
           << ",\"enemy_transition_state3d\":"
           << static_cast<int>(entity.enemyTransitionState3d)
           << ",\"enemy_source_or_kind2c\":"
           << static_cast<int>(entity.enemySourceOrKind2c)
           << ",\"enemy_aux3e\":" << entity.enemyAux3e
           << ",\"enemy_vertical_offset40\":"
           << entity.enemyVerticalOffset40
           << ",\"enemy_origin_y36\":" << entity.enemyOriginY36
           << ",\"enemy_saved_velocity3a\":"
           << entity.enemySavedVelocity3a
           << ",\"enemy_saved_direction44\":"
           << static_cast<int>(entity.enemySavedDirection44)
           << ",\"map_blocked\":"
           << static_cast<unsigned>(entity.mapBlocked)
           << ",\"target_cursor_30\":" << entity.targetCursor30
           << ",\"contact_callback\":";
    writeCallback(output, entity.contactCallback);
    output
           << ",\"enemy_animation_delay\":"
           << entity.enemyAnimationDelay
           << ",\"enemy_animation_sequence\":"
           << entity.enemyAnimationSequence
           << ",\"bump_animation_delay20\":"
           << entity.bumpAnimationDelay20
           << ",\"bump_animation_cursor24\":"
           << entity.bumpAnimationCursor24
           << ",\"active_frames\":" << entity.activeFrames
           << ",\"platform_carry_active\":"
           << (entity.platformCarryActive ? "true" : "false")
           << ",\"active\":" << (entity.active ? "true" : "false")
           << ",\"collected\":" << (entity.collected ? "true" : "false")
           << "}";
}

void writeSimulationEvent(std::ostream &output,
                          const quiky::SimulationEvent &event) {
    output << "{\"kind\":" << static_cast<unsigned>(event.kind)
           << ",\"tick\":" << event.tick
           << ",\"source_id\":" << event.sourceId
           << ",\"code\":" << event.code
           << ",\"value\":" << event.value << "}";
}

void writeLevelEvent(std::ostream &output, const quiky::LevelEvent &event) {
    output << "{\"type\":" << static_cast<unsigned>(event.type)
           << ",\"entity_id\":" << event.entityId
           << ",\"entity_type\":" << event.entityType
           << ",\"tile_id\":" << event.tileId
           << ",\"target_level\":\""
           << jsonEscape(event.targetLevel) << "\",\"state_writes\":[";
    for (std::size_t index = 0; index < event.stateWrites.size(); ++index) {
        if (index != 0) output << ',';
        const quiky::LevelStateWrite &write = event.stateWrites[index];
        output << "{\"address\":" << write.address
               << ",\"width\":" << static_cast<unsigned>(write.width)
               << ",\"before\":" << write.before
               << ",\"after\":" << write.after << "}";
    }
    output << "]}";
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

void writeScheduler(std::ostream &output, const quiky::Simulation &simulation,
                    const quiky::SimulationOutput &snapshot) {
    const std::vector<quiky::SchedulerObject> &objects =
        simulation.state().scheduler.objects();
    output << "[";
    for (std::size_t index = 0; index < snapshot.schedulerCallbacks.size(); ++index) {
        if (index != 0) output << ',';
        const quiky::SchedulerInvocation &invocation =
            snapshot.schedulerCallbacks[index];
        output << "{\"tick\":" << invocation.tick
               << ",\"slot\":" << invocation.slot
               << ",\"generation\":" << invocation.generation
               << ",\"player_callback\":"
               << (invocation.playerCallback ? "true" : "false")
               << ",\"camera_participating\":"
               << (invocation.cameraParticipating ? "true" : "false")
               << ",\"source_id\":";
        if (invocation.slot < objects.size()) {
            output << objects[invocation.slot].sourceId;
        } else {
            output << 0;
        }
        output << ",\"callback\":";
        writeCallback(output, invocation.callback);
        output << "}";
    }
    output << "]";
}

void writeSample(std::ostream &output, const InputFrame &input,
                 const quiky::Simulation &simulation,
                 const quiky::SimulationOutput &snapshot,
                 const quiky::PlayerUpdateTrace &trace,
                 const quiky::LevelRuntime &runtime) {
    const quiky::LevelSession &session = runtime.session();
    output << "{\"schema\":\"quiky.parity-state-v1\",\"sequence\":"
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
            quiky::LevelRuntime::load(archive, mapName, "QUIKYW1.BOB", config);
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
