#include "quiky/archive.h"
#include "quiky/bob.h"
#include "quiky/level_runtime.h"
#include "quiky/player_animation.h"
#include "quiky/player_update.h"
#include "quiky/renderer.h"

#include <algorithm>
#include <cstdlib>
#include <exception>
#include <iostream>
#include <map>
#include <sstream>
#include <string>

namespace {

const std::uint32_t kFrameWidth = 320;
const std::uint32_t kFrameHeight = 200;

void usage() {
    std::cerr << "usage: quiky-frame ARCHIVE MAP-RESOURCE OUTPUT.BMP "
                 "[--player-bob NAME] [--start-x N --start-y N] "
                 "[--camera-x N --camera-y N] [--frames N] "
                 "[--action-flags N] [--high-effect X Y] "
                 "[--no-player] [--no-entities] [--no-effects] "
                 "[--high-effect-only]\n";
}

long parseNumber(const std::string &value, const char *name) {
    char *end = nullptr;
    const long result = std::strtol(value.c_str(), &end, 0);
    if (end == value.c_str() || *end != '\0') {
        throw quiky::FormatError(std::string("invalid ") + name + ": " + value);
    }
    return result;
}

std::string requireValue(int &index, int argc, char **argv, const char *name) {
    if (index + 1 >= argc) {
        throw quiky::FormatError(std::string("missing value for ") + name);
    }
    ++index;
    return std::string(argv[index]);
}

const quiky::BobRecord *findSlot(const quiky::Bob &bob, std::uint16_t slot) {
    for (std::size_t index = 0; index < bob.records.size(); ++index) {
        if (bob.records[index].slot == slot) {
            return &bob.records[index];
        }
    }
    return nullptr;
}

std::uint16_t animatedEntitySlot(const quiky::LevelEntity &entity) {
    if (entity.type == 0x28) {
        return static_cast<std::uint16_t>(413 + ((entity.animationFrame / 8) % 4));
    }
    if (entity.type >= 0x29 && entity.type <= 0x2b) {
        return static_cast<std::uint16_t>(700 + ((entity.animationFrame / 8) % 8));
    }
    return entity.spriteSlot;
}

std::uint16_t worldEffectTile(const std::string &worldName,
                              std::uint16_t animationFrame) {
    const std::uint16_t frame = static_cast<std::uint16_t>(animationFrame / 3);
    if (worldName == "W1" || worldName == "W2") {
        const std::uint16_t sequence[] = {127, 126, 128, 129, 130};
        return sequence[frame % 5];
    }
    if (worldName == "W3") {
        const std::uint16_t sequence[] = {401, 400, 402, 403, 404};
        return sequence[frame % 5];
    }
    if (worldName == "W4") {
        const std::uint16_t sequence[] = {241, 240, 242, 243, 244};
        return sequence[frame % 5];
    }
    if (worldName == "W5") {
        const std::uint16_t sequence[] = {62, 61, 63, 64};
        return sequence[frame % 4];
    }
    return 0xffff;
}

std::uint16_t dedicatedEffectSlot(const quiky::LevelEffect &effect,
                                  const std::string &worldName) {
    if (effect.sourceType == 0x65 && worldName == "W1") {
        return static_cast<std::uint16_t>(1 + (effect.animationFrame & 1));
    }
    return effect.effectSlot;
}

void drawEntitySprites(quiky::IndexedSurface &surface,
                       const quiky::LevelRuntime &runtime) {
    const quiky::LevelSession &level = runtime.session();
    const std::map<std::string, quiky::Bob> &resources = runtime.entityBobs();
    for (std::size_t index = 0; index < level.entities().size(); ++index) {
        const quiky::LevelEntity &entity = level.entities()[index];
        if (entity.phase != quiky::EntityPhase::Active) {
            continue;
        }

        if (!entity.spriteResource.empty() && entity.spriteSlot != 0xffff) {
            const std::map<std::string, quiky::Bob>::const_iterator resource =
                resources.find(entity.spriteResource);
            if (resource != resources.end()) {
                const std::uint16_t slot = animatedEntitySlot(entity);
                const quiky::BobRecord *record = findSlot(resource->second, slot);
                if (record == nullptr) {
                    std::ostringstream message;
                    message << entity.spriteResource << " is missing entity sprite slot "
                            << slot;
                    throw quiky::FormatError(message.str());
                }
                quiky::drawBobRecord(surface, *record, entity.x, entity.y);
            }
        }

        if (entity.effectResource == "WORLD") {
            const std::uint16_t tile = worldEffectTile(
                runtime.worldName(), entity.animationFrame);
            if (tile != 0xffff) {
                quiky::drawIcoTile(surface, runtime.tileset(), tile,
                                   entity.x, entity.y);
            }
        }
    }
}

void drawTransientEffects(quiky::IndexedSurface &surface,
                          const quiky::LevelRuntime &runtime,
                          bool highEffectOnly) {
    const std::vector<quiky::LevelEffect> &effects = runtime.session().effects();
    const std::map<std::string, quiky::Bob> &effectBobs = runtime.effectBobs();
    for (std::size_t index = 0; index < effects.size(); ++index) {
        const quiky::LevelEffect &effect = effects[index];
        if (!effect.active) {
            continue;
        }
        if (highEffectOnly && effect.sourceEntityId != 0) {
            continue;
        }
        if (!effect.spriteResource.empty()) {
            const std::map<std::string, quiky::Bob>::const_iterator resource =
                effectBobs.find(effect.spriteResource);
            if (resource == effectBobs.end()) {
                throw quiky::FormatError("missing effect BOB resource: " +
                                         effect.spriteResource);
            }
            const quiky::BobRecord *record = findSlot(resource->second,
                                                       effect.spriteSlot);
            if (record == nullptr) {
                std::ostringstream message;
                message << effect.spriteResource << " is missing effect sprite slot "
                        << effect.spriteSlot;
                throw quiky::FormatError(message.str());
            }
            quiky::drawBobRecord(surface, *record, effect.x, effect.y);
        } else {
            const std::uint16_t slot = dedicatedEffectSlot(
                effect, runtime.worldName());
            quiky::drawIcoTile(surface, runtime.loopTileset(), slot,
                               effect.x, effect.y);
        }
    }
}

const quiky::BobRecord &choosePlayerFrame(const quiky::Bob &bob,
                                          const quiky::PlayerAnimation &animation) {
    const quiky::BobRecord *record = findSlot(bob, animation.slot());
    if (record == nullptr) {
        record = findSlot(bob, animation.slot() >= 50 ? 50 : 0);
    }
    if (record == nullptr) {
        throw quiky::FormatError("player BOB resource is missing the selected frame");
    }
    return *record;
}

int clampCamera(int camera, int mapPixels, int viewportPixels) {
    const int maximum = std::max(0, mapPixels - viewportPixels);
    return std::max(0, std::min(maximum, camera));
}

quiky::IndexedSurface cropSurface(const quiky::IndexedSurface &source,
                                  int cameraX, int cameraY) {
    quiky::IndexedSurface result(kFrameWidth, kFrameHeight);
    for (std::uint32_t y = 0; y < kFrameHeight; ++y) {
        for (std::uint32_t x = 0; x < kFrameWidth; ++x) {
            const int sourceX = cameraX + static_cast<int>(x);
            const int sourceY = cameraY + static_cast<int>(y);
            if (sourceX >= 0 && sourceY >= 0 &&
                static_cast<std::uint32_t>(sourceX) < source.width &&
                static_cast<std::uint32_t>(sourceY) < source.height) {
                result.at(x, y) = source.at(static_cast<std::uint32_t>(sourceX),
                                            static_cast<std::uint32_t>(sourceY));
            }
        }
    }
    return result;
}

} // namespace

int main(int argc, char **argv) {
    if (argc < 4) {
        usage();
        return EXIT_FAILURE;
    }

    try {
        const quiky::Archive archive = quiky::Archive::load(argv[1]);
        const std::string mapName(argv[2]);
        const std::string outputName(argv[3]);

        std::string playerBobName = "QUIKYW1.BOB";
        bool hasStartX = false;
        bool hasStartY = false;
        std::int32_t startX = 0;
        std::int32_t startY = 0;
        bool hasCameraX = false;
        bool hasCameraY = false;
        int cameraX = 0;
        int cameraY = 0;
        long frames = 0;
        std::uint16_t actionFlags = 0;
        bool hasHighEffect = false;
        std::int32_t highEffectX = 0;
        std::int32_t highEffectY = 0;
        bool drawPlayer = true;
        bool drawEntities = true;
        bool drawEffects = true;
        bool highEffectOnly = false;

        for (int index = 4; index < argc; ++index) {
            const std::string argument(argv[index]);
            if (argument == "--player-bob") {
                playerBobName = requireValue(index, argc, argv, "--player-bob");
            } else if (argument == "--start-x") {
                startX = static_cast<std::int32_t>(parseNumber(
                    requireValue(index, argc, argv, "--start-x"), "start X"));
                hasStartX = true;
            } else if (argument == "--start-y") {
                startY = static_cast<std::int32_t>(parseNumber(
                    requireValue(index, argc, argv, "--start-y"), "start Y"));
                hasStartY = true;
            } else if (argument == "--camera-x") {
                cameraX = static_cast<int>(parseNumber(
                    requireValue(index, argc, argv, "--camera-x"), "camera X"));
                hasCameraX = true;
            } else if (argument == "--camera-y") {
                cameraY = static_cast<int>(parseNumber(
                    requireValue(index, argc, argv, "--camera-y"), "camera Y"));
                hasCameraY = true;
            } else if (argument == "--frames") {
                frames = parseNumber(requireValue(index, argc, argv, "--frames"),
                                     "frame count");
            } else if (argument == "--action-flags") {
                const long parsed = parseNumber(
                    requireValue(index, argc, argv, "--action-flags"), "action flags");
                if (parsed < 0 || parsed > 0xffff) {
                    throw quiky::FormatError("action flags must fit in 16 bits");
                }
                actionFlags = static_cast<std::uint16_t>(parsed);
            } else if (argument == "--high-effect") {
                highEffectX = static_cast<std::int32_t>(parseNumber(
                    requireValue(index, argc, argv, "high-effect X"), "high-effect X"));
                highEffectY = static_cast<std::int32_t>(parseNumber(
                    requireValue(index, argc, argv, "high-effect Y"), "high-effect Y"));
                hasHighEffect = true;
            } else if (argument == "--no-player") {
                drawPlayer = false;
            } else if (argument == "--no-entities") {
                drawEntities = false;
            } else if (argument == "--no-effects") {
                drawEffects = false;
            } else if (argument == "--high-effect-only") {
                highEffectOnly = true;
            } else {
                usage();
                return EXIT_FAILURE;
            }
        }

        if (hasStartX != hasStartY || hasCameraX != hasCameraY) {
            throw quiky::FormatError("start and camera coordinates must be supplied in pairs");
        }
        if (frames < 0) {
            throw quiky::FormatError("frame count must be non-negative");
        }

        quiky::LevelSessionConfig config;
        if (hasStartX) {
            config.hasSpawn = true;
            config.spawnX = startX;
            config.spawnY = startY;
        }
        quiky::Simulation simulation;
        quiky::ExperimentalHorizontalPlayerUpdate playerUpdater;
        simulation.setExperimentalPlayerUpdater(&playerUpdater);
        std::unique_ptr<quiky::LevelRuntime> runtime =
            quiky::LevelRuntime::load(archive, mapName, playerBobName, config);
        quiky::SimulationOutput output;
        runtime->reset(simulation);
        output.player = simulation.state().player;
        quiky::PlayerRecord &player = output.player;
        quiky::PlayerAnimation playerAnimation;
        playerAnimation.reset();
        playerAnimation.advance(player);

        if (hasHighEffect) {
            runtime->session().emitHighEffect(highEffectX, highEffectY);
        }
        const quiky::InputState input = quiky::InputState::fromActionFlags(actionFlags);
        for (long frame = 0; frame < frames; ++frame) {
            runtime->tick(simulation, input, output);
            playerAnimation.advance(player);
        }

        quiky::IndexedSurface scene = quiky::renderMap(runtime->map(), runtime->tileset());
        if (drawEntities) {
            drawEntitySprites(scene, *runtime);
        }
        if (drawEffects) {
            drawTransientEffects(scene, *runtime, highEffectOnly);
        }
        if (drawPlayer) {
            const quiky::BobRecord &record = choosePlayerFrame(
                runtime->playerBob(), playerAnimation);
            quiky::drawBobRecord(scene, record,
                                 player.positionX.floorPixels(),
                                 player.positionY.floorPixels());
        }

        if (!hasCameraX) {
            cameraX = player.positionX.floorPixels() - static_cast<int>(kFrameWidth / 2);
            cameraY = player.positionY.floorPixels() - static_cast<int>(kFrameHeight / 2);
        }
        cameraX = clampCamera(cameraX, static_cast<int>(scene.width), kFrameWidth);
        cameraY = clampCamera(cameraY, static_cast<int>(scene.height), kFrameHeight);
        const quiky::IndexedSurface frame = cropSurface(scene, cameraX, cameraY);
        quiky::writeBmp(outputName, frame, runtime->palette());

        std::cout << mapName << ": frame=" << frames
                  << " camera=" << cameraX << "," << cameraY
                  << " player=" << player.positionX.floorPixels() << ","
                  << player.positionY.floorPixels() << " output=" << outputName << "\n";
    } catch (const std::exception &error) {
        std::cerr << "error: " << error.what() << "\n";
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}
