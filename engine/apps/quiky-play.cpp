#include "quiky/archive.h"
#include "quiky/bob.h"
#include "quiky/level_runtime.h"
#ifdef QUIKY_WITH_MUSIC
#include "quiky/music.h"
#endif
#include "quiky/renderer.h"
#include "quiky/runtime.h"

#include <SDL3/SDL.h>

#include <algorithm>
#include <cctype>
#include <cstdint>
#include <cstdlib>
#include <exception>
#include <iostream>
#include <map>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

namespace {

const int kViewportWidth = 640;
const int kViewportHeight = 360;
const std::uint64_t kTickNanoseconds = 1000000000ULL / 60ULL;

void usage() {
    std::cerr << "usage: quiky-play ARCHIVE [MAP-RESOURCE BOB-RESOURCE] "
                 "[START-X START-Y] [--overlay-are] [--entities] [--no-music]\n"
                 "controls: arrows/A,D move, space/W/up jump, R reset, "
                 "P pause, N step, F1 toggle ARE, F2 toggle active entities, Esc quit\n";
}

long parseNumber(const std::string &value, const char *name) {
    char *end = nullptr;
    const long result = std::strtol(value.c_str(), &end, 0);
    if (end == value.c_str() || *end != '\0') {
        throw quiky::FormatError(std::string("invalid ") + name + ": " + value);
    }
    return result;
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

std::uint16_t dedicatedEffectSlot(const quiky::LevelEffect &effect,
                                  const std::string &worldName) {
    if (effect.sourceType == 0x65 && worldName == "W1") {
        // W1 probes captured LOOP_W1 records 1 and 2 as the event animation
        // byte advances. Other world-specific representatives are fixed by
        // the confirmed cross-world resource probes.
        return static_cast<std::uint16_t>(1 + (effect.animationFrame & 1));
    }
    return effect.effectSlot;
}

void drawTransientEffects(quiky::IndexedSurface &surface,
                          const quiky::LevelRuntime &runtime) {
    const std::vector<quiky::LevelEffect> &effects = runtime.session().effects();
    for (std::size_t index = 0; index < effects.size(); ++index) {
        const quiky::LevelEffect &effect = effects[index];
        if (!effect.active) {
            continue;
        }
        const std::uint16_t slot = dedicatedEffectSlot(effect, runtime.worldName());
        const quiky::Tileset &tileset = effect.effectResource == "WORLD"
                                            ? runtime.tileset()
                                            : runtime.loopTileset();
        quiky::drawIcoTile(surface, tileset, slot,
                           effect.x, effect.y);
    }
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

    }
}

const quiky::BobRecord &choosePlayerFrame(const quiky::Bob &bob,
                                          const quiky::PlayerState &player,
                                          std::uint64_t frame) {
    // QUIKYW1.BOB currently has two 40-frame ranges, 0..39 and 50..89. The
    // direction assignment and exact animation cadence remain provisional;
    // keeping the policy here makes it easy to replace with trace evidence.
    const std::uint16_t base = player.facingRight ? 0 : 50;
    const std::int32_t speed = player.velocityX.raw < 0
                                   ? -player.velocityX.raw
                                   : player.velocityX.raw;
    std::uint16_t slot = base;
    if (!player.grounded) {
        slot = static_cast<std::uint16_t>(base + 20);
    } else if (speed > quiky::Fixed16::kOne / 4) {
        slot = static_cast<std::uint16_t>(base + ((frame / 6) % 10));
    }
    const quiky::BobRecord *record = findSlot(bob, slot);
    if (record == nullptr) {
        record = findSlot(bob, base);
    }
    if (record == nullptr) {
        throw quiky::FormatError("player BOB resource is missing the selected frame");
    }
    return *record;
}

void checkSdl(bool success, const char *operation) {
    if (!success) {
        throw quiky::FormatError(std::string(operation) + ": " + SDL_GetError());
    }
}

struct SdlState {
    SDL_Window *window;
    SDL_Renderer *renderer;
    SDL_Texture *texture;
#ifdef QUIKY_WITH_MUSIC
    SDL_AudioStream *musicStream;
#endif

    SdlState()
        : window(nullptr), renderer(nullptr), texture(nullptr)
#ifdef QUIKY_WITH_MUSIC
          , musicStream(nullptr)
#endif
    {}

    ~SdlState() {
#ifdef QUIKY_WITH_MUSIC
        if (musicStream != nullptr) {
            SDL_DestroyAudioStream(musicStream);
        }
#endif
        if (texture != nullptr) {
            SDL_DestroyTexture(texture);
        }
        if (renderer != nullptr) {
            SDL_DestroyRenderer(renderer);
        }
        if (window != nullptr) {
            SDL_DestroyWindow(window);
        }
        SDL_Quit();
    }
};

SDL_Texture *createSurfaceTexture(SDL_Renderer *renderer,
                                  const quiky::IndexedSurface &surface) {
    SDL_Texture *texture = SDL_CreateTexture(
        renderer, SDL_PIXELFORMAT_RGB24, SDL_TEXTUREACCESS_STREAMING,
        static_cast<int>(surface.width), static_cast<int>(surface.height));
    if (texture == nullptr) {
        throw quiky::FormatError(std::string("SDL_CreateTexture: ") + SDL_GetError());
    }
    checkSdl(SDL_SetTextureScaleMode(texture, SDL_SCALEMODE_NEAREST),
             "SDL_SetTextureScaleMode");
    return texture;
}

void replaceSurfaceTexture(SdlState &sdl,
                           const quiky::IndexedSurface &surface) {
    SDL_Texture *texture = createSurfaceTexture(sdl.renderer, surface);
    if (sdl.texture != nullptr) {
        SDL_DestroyTexture(sdl.texture);
    }
    sdl.texture = texture;
}

#ifdef QUIKY_WITH_MUSIC
bool startGameplayMusic(const quiky::Archive &archive, SdlState &sdl) {
    try {
        quiky::MusicModule module(archive, "ONGAME2");
        const quiky::Pcm16Stereo audio = module.render(0, 180, false);
        SDL_AudioSpec spec;
        SDL_zero(spec);
        spec.freq = static_cast<int>(audio.sampleRate);
        spec.format = SDL_AUDIO_S16LE;
        spec.channels = 2;
        sdl.musicStream = SDL_OpenAudioDeviceStream(
            SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &spec, nullptr, nullptr);
        if (sdl.musicStream == nullptr) {
            std::cerr << "warning: gameplay music unavailable: " << SDL_GetError() << "\n";
            return false;
        }
        const int bytes = static_cast<int>(audio.samples.size() * sizeof(std::int16_t));
        if (!SDL_PutAudioStreamData(sdl.musicStream, audio.samples.data(), bytes) ||
            !SDL_FlushAudioStream(sdl.musicStream) ||
            !SDL_ResumeAudioStreamDevice(sdl.musicStream)) {
            std::cerr << "warning: gameplay music could not start: " << SDL_GetError() << "\n";
            SDL_DestroyAudioStream(sdl.musicStream);
            sdl.musicStream = nullptr;
            return false;
        }
        return true;
    } catch (const std::exception &error) {
        std::cerr << "warning: gameplay music unavailable: " << error.what() << "\n";
        return false;
    }
}
#endif

void uploadSurface(SDL_Texture *texture, const quiky::IndexedSurface &surface,
                   const quiky::Palette &palette) {
    std::vector<quiky::byte> rgb(surface.pixels.size() * 3);
    for (std::size_t index = 0; index < surface.pixels.size(); ++index) {
        const quiky::RGB &color = palette.colors[surface.pixels[index]];
        rgb[index * 3 + 0] = color.red;
        rgb[index * 3 + 1] = color.green;
        rgb[index * 3 + 2] = color.blue;
    }
    checkSdl(SDL_UpdateTexture(texture, nullptr, rgb.data(),
                               static_cast<int>(surface.width * 3)),
             "SDL_UpdateTexture");
}

int clampCamera(int camera, int mapPixels, int viewportPixels) {
    const int maximum = std::max(0, mapPixels - viewportPixels);
    return std::max(0, std::min(maximum, camera));
}

void drawEntityMarkers(quiky::IndexedSurface &surface, quiky::Palette &palette,
                       const quiky::LevelSession &level) {
    palette.colors[236].red = 240;
    palette.colors[236].green = 72;
    palette.colors[236].blue = 72;
    palette.colors[237].red = 72;
    palette.colors[237].green = 240;
    palette.colors[237].blue = 112;
    palette.colors[238].red = 72;
    palette.colors[238].green = 160;
    palette.colors[238].blue = 240;

    for (std::size_t index = 0; index < level.entities().size(); ++index) {
        const quiky::LevelEntity &entity = level.entities()[index];
        if (!entity.active || entity.kind == quiky::EntityKind::Unknown) {
            continue;
        }
        const quiky::byte marker = entity.kind == quiky::EntityKind::Hazard
                                       ? 236
                                       : entity.kind == quiky::EntityKind::Collectible
                                             ? 237
                                             : 238;
        for (std::int32_t deltaY = -4; deltaY <= 4; ++deltaY) {
            for (std::int32_t deltaX = -4; deltaX <= 4; ++deltaX) {
                const std::int32_t x = entity.x + deltaX;
                const std::int32_t y = entity.y + deltaY;
                if (x < 0 || y < 0 || static_cast<std::uint32_t>(x) >= surface.width ||
                    static_cast<std::uint32_t>(y) >= surface.height) {
                    continue;
                }
                surface.at(static_cast<std::uint32_t>(x), static_cast<std::uint32_t>(y)) =
                    (std::abs(deltaX) == 4 || std::abs(deltaY) == 4) ? 0 : marker;
            }
        }
    }
}

void updateTitle(SDL_Window *window, const std::string &mapName,
                 const quiky::PlayerState &player, std::uint64_t frame,
                 std::uint16_t slot, bool paused, const quiky::LevelSession &level,
                 std::uint32_t carriedScore, std::uint32_t carriedDeaths,
                 const std::string &eventText) {
    std::ostringstream title;
    title << "Quiky | " << mapName << " | " << (paused ? "PAUSED | " : "")
          << "frame=" << frame << " slot=" << slot
          << " x=" << player.x.floorPixels()
          << " y=" << player.y.floorPixels()
          << " vx=" << player.velocityX.floorPixels()
          << " vy=" << player.velocityY.floorPixels()
          << (player.grounded ? " grounded" : " airborne")
          << " score=" << carriedScore + level.score()
          << " deaths=" << carriedDeaths + level.deaths();
    if (!eventText.empty()) {
        title << " " << eventText;
    }
    SDL_SetWindowTitle(window, title.str().c_str());
}

bool isKey(SDL_Scancode key, SDL_Scancode first, SDL_Scancode second = SDL_SCANCODE_UNKNOWN) {
    return key == first || key == second;
}

} // namespace

int main(int argc, char **argv) {
    if (argc < 2) {
        usage();
        return EXIT_FAILURE;
    }

    try {
        const quiky::Archive archive = quiky::Archive::load(argv[1]);
        std::string mapName = "W1L1.MAP";
        std::string bobName = "QUIKYW1.BOB";
        std::vector<std::string> positional;
        bool showArea = false;
        bool showEntities = false;
        bool musicEnabled = true;
        for (int index = 2; index < argc; ++index) {
            const std::string argument(argv[index]);
            if (argument == "--overlay-are") {
                showArea = true;
            } else if (argument == "--entities") {
                showEntities = true;
            } else if (argument == "--no-music") {
                musicEnabled = false;
            } else {
                positional.push_back(argument);
            }
        }
        if (positional.size() == 1 || positional.size() == 2 || positional.size() == 4) {
            mapName = positional[0];
        }
        if (positional.size() == 2 || positional.size() == 4) {
            bobName = positional[1];
        }
        if (positional.size() != 0 && positional.size() != 1 &&
            positional.size() != 2 && positional.size() != 4) {
            usage();
            return EXIT_FAILURE;
        }

        bool hasStartOverride = false;
        std::int32_t startX = 0;
        std::int32_t startY = 0;
        if (positional.size() == 4) {
            const long parsedX = parseNumber(positional[2], "start X");
            const long parsedY = parseNumber(positional[3], "start Y");
            if (parsedX < 0 || parsedY < 0 || parsedX > 0x7fffffffL ||
                parsedY > 0x7fffffffL) {
                throw quiky::FormatError("start coordinates must be non-negative 32-bit values");
            }
            startX = static_cast<std::int32_t>(parsedX);
            startY = static_cast<std::int32_t>(parsedY);
            hasStartOverride = true;
        }
        quiky::LevelSessionConfig levelConfig;
        if (hasStartOverride) {
            levelConfig.hasSpawn = true;
            levelConfig.spawnX = startX;
            levelConfig.spawnY = startY;
        }
        quiky::PlayerSimulation simulation;
        std::unique_ptr<quiky::LevelRuntime> runtime =
            quiky::LevelRuntime::load(archive, mapName, bobName, levelConfig);
        quiky::PlayerState player;
        runtime->reset(player, simulation);

        checkSdl(SDL_Init(SDL_INIT_VIDEO), "SDL_Init");
        SdlState sdl;
#ifdef QUIKY_WITH_MUSIC
        if (musicEnabled) {
            if (!SDL_InitSubSystem(SDL_INIT_AUDIO)) {
                std::cerr << "warning: gameplay music unavailable: " << SDL_GetError() << "\n";
            } else {
                startGameplayMusic(archive, sdl);
            }
        }
#else
        (void)musicEnabled;
#endif
        checkSdl(SDL_CreateWindowAndRenderer(
                     "Quiky", kViewportWidth * 2, kViewportHeight * 2,
                     SDL_WINDOW_RESIZABLE, &sdl.window, &sdl.renderer),
                 "SDL_CreateWindowAndRenderer");
        checkSdl(SDL_SetRenderLogicalPresentation(
                     sdl.renderer, kViewportWidth, kViewportHeight,
                     SDL_LOGICAL_PRESENTATION_INTEGER_SCALE),
                 "SDL_SetRenderLogicalPresentation");

        const quiky::IndexedSurface initialSurface =
            quiky::renderMap(runtime->map(), runtime->tileset());
        sdl.texture = createSurfaceTexture(sdl.renderer, initialSurface);

        bool running = true;
        bool paused = false;
        bool left = false;
        bool right = false;
        bool jumpPressed = false;
        bool stepRequested = false;
        std::uint64_t frame = 0;
        std::uint64_t lastTime = SDL_GetTicksNS();
        std::uint64_t accumulator = 0;
        std::uint64_t titleTime = 0;
        std::uint64_t eventUntil = 0;
        std::uint32_t carriedScore = 0;
        std::uint32_t carriedDeaths = 0;
        std::string eventText;

        while (running) {
            const std::uint64_t now = SDL_GetTicksNS();
            const std::uint64_t elapsed = std::min<std::uint64_t>(now - lastTime, 250000000ULL);
            lastTime = now;
            accumulator += elapsed;

            SDL_Event event;
            while (SDL_PollEvent(&event)) {
                if (event.type == SDL_EVENT_QUIT) {
                    running = false;
                } else if (event.type == SDL_EVENT_KEY_DOWN ||
                           event.type == SDL_EVENT_KEY_UP) {
                    const bool down = event.key.down;
                    const SDL_Scancode key = event.key.scancode;
                    if (key == SDL_SCANCODE_ESCAPE && down) {
                        running = false;
                    } else if (isKey(key, SDL_SCANCODE_LEFT, SDL_SCANCODE_A)) {
                        left = down;
                    } else if (isKey(key, SDL_SCANCODE_RIGHT, SDL_SCANCODE_D)) {
                        right = down;
                    } else if (isKey(key, SDL_SCANCODE_SPACE, SDL_SCANCODE_W) ||
                               key == SDL_SCANCODE_UP) {
                        if (down && !event.key.repeat) {
                            jumpPressed = true;
                        }
                    } else if (key == SDL_SCANCODE_P && down && !event.key.repeat) {
                        paused = !paused;
                    } else if (key == SDL_SCANCODE_N && down && !event.key.repeat) {
                        stepRequested = true;
                    } else if (key == SDL_SCANCODE_R && down && !event.key.repeat) {
                        runtime->reset(player, simulation);
                        frame = 0;
                        accumulator = 0;
                        jumpPressed = false;
                        stepRequested = false;
                    } else if (key == SDL_SCANCODE_F1 && down && !event.key.repeat) {
                        showArea = !showArea;
                    } else if (key == SDL_SCANCODE_F2 && down && !event.key.repeat) {
                        showEntities = !showEntities;
                    }
                }
            }

            while (accumulator >= kTickNanoseconds) {
                if (!paused) {
                    quiky::InputState input;
                    input.left = left;
                    input.right = right;
                    input.jump = jumpPressed;
                    runtime->tick(player, simulation, input);
                    ++frame;
                    jumpPressed = false;
                    const quiky::LevelEvent event = runtime->session().consumeEvent();
                    if (event.type == quiky::LevelEventType::Collected) {
                        eventText = "collected";
                        eventUntil = now + 1000000000ULL;
                    } else if (event.type == quiky::LevelEventType::PlayerDied) {
                        eventText = "player-died";
                        eventUntil = now + 1000000000ULL;
                    } else if (event.type == quiky::LevelEventType::LevelExit) {
                        eventText = event.targetLevel.empty()
                                        ? "exit"
                                        : "exit->" + event.targetLevel;
                        eventUntil = now + 5000000000ULL;
                        if (!event.targetLevel.empty()) {
                            std::unique_ptr<quiky::LevelRuntime> next =
                                quiky::LevelRuntime::load(
                                    archive, event.targetLevel, bobName);
                            const quiky::IndexedSurface nextSurface =
                                quiky::renderMap(next->map(), next->tileset());
                            replaceSurfaceTexture(sdl, nextSurface);
                            carriedScore += runtime->session().score();
                            carriedDeaths += runtime->session().deaths();
                            runtime.swap(next);
                            runtime->reset(player, simulation);
                            frame = 0;
                            accumulator = 0;
                            jumpPressed = false;
                            stepRequested = false;
                            eventText = "loaded " + runtime->mapName();
                            eventUntil = now + 2000000000ULL;
                            break;
                        }
                    }
                }
                accumulator -= kTickNanoseconds;
            }
            if (paused && stepRequested) {
                quiky::InputState input;
                input.left = left;
                input.right = right;
                input.jump = jumpPressed;
                runtime->tick(player, simulation, input);
                ++frame;
                jumpPressed = false;
                stepRequested = false;
                runtime->session().consumeEvent();
            }

            quiky::Palette framePalette = runtime->palette();
            quiky::IndexedSurface surface =
                quiky::renderMap(runtime->map(), runtime->tileset());
            if (showArea) {
                quiky::overlayArea(surface, framePalette, runtime->area());
            }
            if (showEntities) {
                drawEntityMarkers(surface, framePalette, runtime->session());
            }
            drawEntitySprites(surface, *runtime);
            drawTransientEffects(surface, *runtime);
            const quiky::BobRecord &record =
                choosePlayerFrame(runtime->playerBob(), player, frame);
            quiky::drawBobRecord(surface, record,
                                 player.x.floorPixels(), player.y.floorPixels());
            uploadSurface(sdl.texture, surface, framePalette);

            int cameraX = player.x.floorPixels() - kViewportWidth / 2;
            int cameraY = player.y.floorPixels() - kViewportHeight / 2;
            cameraX = clampCamera(cameraX, static_cast<int>(surface.width), kViewportWidth);
            cameraY = clampCamera(cameraY, static_cast<int>(surface.height), kViewportHeight);
            const SDL_FRect source = {
                static_cast<float>(cameraX), static_cast<float>(cameraY),
                static_cast<float>(kViewportWidth), static_cast<float>(kViewportHeight)};
            const SDL_FRect destination = {
                0.0f, 0.0f, static_cast<float>(kViewportWidth),
                static_cast<float>(kViewportHeight)};
            checkSdl(SDL_RenderClear(sdl.renderer), "SDL_RenderClear");
            checkSdl(SDL_RenderTexture(sdl.renderer, sdl.texture, &source, &destination),
                     "SDL_RenderTexture");
            SDL_RenderPresent(sdl.renderer);

            if (now - titleTime >= 250000000ULL) {
                if (now >= eventUntil) {
                    eventText.clear();
                }
                updateTitle(sdl.window, runtime->mapName(), player, frame,
                            record.slot, paused, runtime->session(), carriedScore,
                            carriedDeaths, eventText);
                titleTime = now;
            }
            if (accumulator < kTickNanoseconds) {
                SDL_Delay(1);
            }
        }
    } catch (const std::exception &error) {
        std::cerr << "error: " << error.what() << "\n";
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}
