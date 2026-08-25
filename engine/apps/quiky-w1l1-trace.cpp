#include "quiky/archive.h"
#include "quiky/level_runtime.h"
#include "quiky/player_update.h"
#include "quiky/runtime.h"
#include "quiky/simulation.h"

#include <array>
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
    std::uint16_t actionFlags;
    bool hasCamera;
    std::int32_t cameraX;
    std::int32_t cameraY;

    InputFrame(std::size_t sequenceValue = 0,
               std::uint16_t actionFlagsValue = 0)
        : sequence(sequenceValue), actionFlags(actionFlagsValue),
          hasCamera(false), cameraX(0), cameraY(0) {}
};

void usage() {
    std::cerr << "usage: quiky-w1l1-trace ARCHIVE MAP-RESOURCE OUTPUT.JSON "
                 "[--frames N] [--action-flags N] [--input-tsv PATH] "
                 "[--camera-x N --camera-y N] "
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

std::string jsonEscape(const std::string &value) {
    std::ostringstream output;
    for (std::size_t index = 0; index < value.size(); ++index) {
        const unsigned char character =
            static_cast<unsigned char>(value[index]);
        switch (character) {
        case '"': output << "\\\""; break;
        case '\\': output << "\\\\"; break;
        case '\b': output << "\\b"; break;
        case '\f': output << "\\f"; break;
        case '\n': output << "\\n"; break;
        case '\r': output << "\\r"; break;
        case '\t': output << "\\t"; break;
        default:
            if (character < 0x20) {
                output << "\\u" << std::hex << std::setfill('0')
                       << std::setw(4) << static_cast<unsigned>(character)
                       << std::dec;
            } else {
                output << value[index];
            }
            break;
        }
    }
    return output.str();
}

std::string hexBytes(const quiky::Bytes &bytes) {
    std::ostringstream output;
    output << std::hex << std::setfill('0');
    for (std::size_t index = 0; index < bytes.size(); ++index) {
        output << std::setw(2) << static_cast<unsigned>(bytes[index]);
    }
    return output.str();
}

void writeCallback(std::ostream &output,
                   const quiky::CallbackIdentity &callback) {
    output << "{\"segment\":" << callback.segment
           << ",\"offset\":" << callback.offset
           << ",\"label\":\"" << jsonEscape(callback.label) << "\"}";
}

void writeGlobalState(std::ostream &output,
                      const quiky::LevelGameplayState &state) {
    output << "{\"ammo_880c\":" << state.ammo880c
           << ",\"lives_880a\":" << state.lives880a
           << ",\"score_881c\":" << state.score881c
           << ",\"current_health_8822\":" << state.currentHealth8822
           << ",\"maximum_health_8824\":" << state.maximumHealth8824
           << ",\"invulnerability_8810\":"
           << state.invulnerabilityGate8810
           << ",\"pending_event_612e\":" << state.pendingEvent612e
           << ",\"player_timer_0034\":" << state.playerTimer0034
           << ",\"puzzle_mask_60d8\":" << state.puzzleMask60d8
           << ",\"terminal_x_8828\":" << state.terminalX8828
           << ",\"terminal_y_882a\":" << state.terminalY882a
           << ",\"cloud_signal_89e6\":" << state.cloudSignal89e6
           << ",\"transition_gate_89ea\":" << state.transitionGate89ea
           << ",\"platform_latch_5006\":" << state.platformLatch5006
           << ",\"platform_carry_x_8816\":"
           << state.platformCarryX8816
           << ",\"platform_carry_y_8812\":"
           << state.platformCarryY8812 << "}";
}

void writeProbe(std::ostream &output, const quiky::CollisionProbe &probe,
                bool occupied) {
    output << "{\"x\":" << probe.pixelX
           << ",\"y\":" << probe.pixelY
           << ",\"in_bounds\":" << (probe.inBounds ? "true" : "false")
           << ",\"map_word\":" << probe.mapWord
           << ",\"tile_id\":" << probe.tileId
           << ",\"descriptor_word\":" << probe.descriptorWord
           << ",\"quadrant_mask\":"
           << static_cast<unsigned>(probe.quadrantMask)
           << ",\"occupied\":" << (occupied ? "true" : "false") << "}";
}

void writePlayerTrace(std::ostream &output,
                      const quiky::PlayerUpdateTrace &trace) {
    output << "{\"input_flags\":" << trace.inputFlags
           << ",\"pre_record_hex\":\""
           << hexBytes(trace.preState.toBytes())
           << "\",\"post_record_hex\":\""
           << hexBytes(trace.postState.toBytes())
           << "\",\"stages\":[";
    for (std::size_t index = 0; index < trace.stages.size(); ++index) {
        if (index != 0) output << ',';
        output << static_cast<unsigned>(trace.stages[index]);
    }
    output << "],\"state_writes\":[";
    for (std::size_t index = 0; index < trace.stateWrites.size(); ++index) {
        if (index != 0) output << ',';
        const quiky::TraceStateWrite &write = trace.stateWrites[index];
        output << "{\"offset\":" << write.offset
               << ",\"width\":" << static_cast<unsigned>(write.width)
               << ",\"value\":" << write.value << "}";
    }
    output << "],\"collisions\":[";
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
        output << "{\"offset\":" << write.address
               << ",\"width\":" << static_cast<unsigned>(write.width)
               << ",\"before\":" << write.before
               << ",\"after\":" << write.after << "}";
    }
    output << "],\"effects\":[";
    for (std::size_t index = 0; index < trace.effectDispatches.size(); ++index) {
        if (index != 0) output << ',';
        const quiky::PlayerEffectDispatch &effect = trace.effectDispatches[index];
        output << "{\"address\":" << effect.address
               << ",\"code\":" << effect.code << "}";
    }
    output << "]}";
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

std::vector<InputFrame> readInputTsv(const std::string &path) {
    std::ifstream input(path.c_str());
    if (!input) {
        throw quiky::FormatError("cannot open input TSV: " + path);
    }
    std::vector<InputFrame> frames;
    std::string line;
    while (std::getline(input, line)) {
        if (line.empty() || line[0] == '#') continue;
        std::istringstream fields(line);
        std::string sequenceText;
        std::string flagsText;
        std::string cameraXText;
        std::string cameraYText;
        if (!(fields >> sequenceText >> flagsText)) {
            throw quiky::FormatError("input TSV needs sequence and action flags");
        }
        InputFrame frame(
            static_cast<std::size_t>(parseNumber(sequenceText, "sequence")),
            0);
        const long parsedFlags = parseNumber(flagsText, "action flags");
        if (parsedFlags < 0 || parsedFlags > 0xffff) {
            throw quiky::FormatError("action flags outside uint16 range");
        }
        frame.actionFlags = static_cast<std::uint16_t>(parsedFlags);
        if (fields >> cameraXText >> cameraYText) {
            frame.hasCamera = true;
            frame.cameraX = static_cast<std::int32_t>(
                parseNumber(cameraXText, "camera X"));
            frame.cameraY = static_cast<std::int32_t>(
                parseNumber(cameraYText, "camera Y"));
        }
        frames.push_back(frame);
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
                 const quiky::LevelRuntime &runtime,
                 const std::vector<quiky::LevelEvent> &levelEvents) {
    const quiky::LevelSession &session = runtime.session();
    output << "{\"sequence\":" << input.sequence
           << ",\"tick\":" << snapshot.tick
           << ",\"input_flags\":" << input.actionFlags
           << ",\"camera\":{\"x\":" << session.streamAnchorX()
           << ",\"y\":" << session.streamAnchorY()
           << ",\"active\":"
           << (session.hasStreamAnchor() ? "true" : "false") << "}"
           << ",\"player_record_hex\":\""
           << hexBytes(snapshot.player.toBytes()) << "\",\"player_callback\":";
    writePlayerTrace(output, trace);
    output << ",\"player_dependency_order\":[";
    for (std::size_t index = 0; index < snapshot.playerDependencyOrder.size(); ++index) {
        if (index != 0) output << ',';
        const quiky::SimulationCallbackStep &step =
            snapshot.playerDependencyOrder[index];
        output << "{\"phase\":" << static_cast<unsigned>(step.phase)
               << ",\"source_id\":" << step.sourceId
               << ",\"callback\":";
        writeCallback(output, step.callback);
        output << "}";
    }
    output << "],\"scheduler_callbacks\":";
    writeScheduler(output, simulation, snapshot);
    output << ",\"entities\":[";
    bool first = true;
    for (std::size_t index = 0; index < session.entities().size(); ++index) {
        const quiky::LevelEntity &entity = session.entities()[index];
        if (!entity.active) continue;
        if (!first) output << ',';
        first = false;
        writeEntity(output, entity);
    }
    output << "],\"effects\":[";
    for (std::size_t index = 0; index < session.effects().size(); ++index) {
        if (index != 0) output << ',';
        const quiky::LevelEffect &effect = session.effects()[index];
        output << "{\"source_entity_id\":" << effect.sourceEntityId
               << ",\"source_type\":" << effect.sourceType
               << ",\"x\":" << effect.x << ",\"y\":" << effect.y
               << ",\"effect_slot\":" << effect.effectSlot
               << ",\"sprite_slot\":" << effect.spriteSlot
               << ",\"animation_frame\":" << effect.animationFrame
               << ",\"callback_state\":" << effect.callbackState
               << ",\"event_subtype\":"
               << static_cast<unsigned>(effect.eventSubtype)
               << ",\"event_animation_state\":"
               << static_cast<unsigned>(effect.eventAnimationState)
               << ",\"callback\":";
        writeCallback(output, effect.updateCallback);
        output << ",\"active\":" << (effect.active ? "true" : "false")
               << "}";
    }
    output << "],\"gameplay_state\":";
    writeGlobalState(output, session.gameplayState());
    output << ",\"simulation_game_events\":[";
    for (std::size_t index = 0; index < snapshot.gameEvents.size(); ++index) {
        if (index != 0) output << ',';
        writeSimulationEvent(output, snapshot.gameEvents[index]);
    }
    output << "],\"simulation_audio_events\":[";
    for (std::size_t index = 0; index < snapshot.audioEvents.size(); ++index) {
        if (index != 0) output << ',';
        const quiky::AudioEvent &event = snapshot.audioEvents[index];
        output << "{\"tick\":" << event.tick
               << ",\"source_id\":" << event.sourceId
               << ",\"code\":" << event.code
               << ",\"value\":" << event.value << "}";
    }
    output << "],\"level_events\":[";
    for (std::size_t index = 0; index < levelEvents.size(); ++index) {
        if (index != 0) output << ',';
        writeLevelEvent(output, levelEvents[index]);
    }
    output << "]}";
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
        std::string inputTsv;
        bool hasCamera = false;
        std::int32_t cameraX = 0;
        std::int32_t cameraY = 262;
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
            } else if (option == "--input-tsv" && index + 1 < argc) {
                inputTsv = argv[++index];
            } else if (option == "--camera-x" && index + 1 < argc) {
                cameraX = static_cast<std::int32_t>(
                    parseNumber(argv[++index], "camera X"));
                hasCamera = true;
            } else if (option == "--camera-y" && index + 1 < argc) {
                cameraY = static_cast<std::int32_t>(
                    parseNumber(argv[++index], "camera Y"));
                hasCamera = true;
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
        if (!inputTsv.empty()) {
            inputs = readInputTsv(inputTsv);
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
        runtime->reset(simulation);
        if (hasCamera && !inputs.empty() && inputs[0].hasCamera) {
            runtime->setStreamAnchor(inputs[0].cameraX, inputs[0].cameraY);
        }

        std::ofstream output(outputPath.c_str());
        if (!output) {
            throw quiky::FormatError("cannot open trace output: " + outputPath);
        }
        output << "{\"schema\":\"quiky-w1l1-session-v1\",\"archive\":\""
               << jsonEscape(archivePath) << "\",\"map\":\""
               << jsonEscape(mapName) << "\",\"samples\":[";
        for (std::size_t index = 0; index < inputs.size(); ++index) {
            if (index != 0) output << ',';
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
            writeSample(output, input, simulation, snapshot, trace, *runtime,
                        levelEvents);
        }
        output << "]}\n";
    } catch (const std::exception &error) {
        std::cerr << "error: " << error.what() << "\n";
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}
