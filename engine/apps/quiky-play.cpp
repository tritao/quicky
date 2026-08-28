#include "quiky/archive.h"
#include "quiky/bob.h"
#include "quiky/camera.h"
#include "quiky/level_runtime.h"
#include "quiky/player_update.h"
#include "quiky/player_animation.h"
#ifdef QUIKY_WITH_MUSIC
#include "quiky/audio.h"
#include "quiky/sfx_events.h"
#endif
#include "quiky/renderer.h"
#include "quiky/runtime.h"
#include "quiky/scene_renderer.h"

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

const int kLogicalWidth = 320;
const int kLogicalHeight = 200;
const int kWorldViewportHeight = 176;
const std::uint64_t kTickNanoseconds = 1000000000ULL / 60ULL;

void usage() {
    std::cerr << "usage: quiky-play ARCHIVE [MAP-RESOURCE BOB-RESOURCE] "
                 "[START-X START-Y] [--overlay-are] [--entities] [--no-music]\n"
                 "controls: arrows/A,D move, space/W/up jump, R reset, "
                 "Alt alternate action, P pause, N step, F1 toggle ARE, "
                 "F2 toggle active entities, Esc quit\n";
}

long parseNumber(const std::string &value, const char *name) {
    char *end = nullptr;
    const long result = std::strtol(value.c_str(), &end, 0);
    if (end == value.c_str() || *end != '\0') {
        throw quiky::FormatError(std::string("invalid ") + name + ": " + value);
    }
    return result;
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
    SDL_AudioStream *audioStream;
    quiky::AudioMixer *audioMixer;
#endif

    SdlState()
        : window(nullptr), renderer(nullptr), texture(nullptr)
#ifdef QUIKY_WITH_MUSIC
          , audioStream(nullptr), audioMixer(nullptr)
#endif
    {}

    ~SdlState() {
#ifdef QUIKY_WITH_MUSIC
        if (audioStream != nullptr) {
            SDL_DestroyAudioStream(audioStream);
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
void audioStreamCallback(void *userdata, SDL_AudioStream *stream,
                         int additionalAmount, int) {
    if (additionalAmount <= 0) {
        return;
    }
    quiky::AudioMixer *mixer = static_cast<quiky::AudioMixer *>(userdata);
    const std::size_t frames = static_cast<std::size_t>(additionalAmount + 3) / 4;
    try {
        const quiky::Pcm16Stereo audio = mixer->mix(frames);
        SDL_PutAudioStreamData(stream, audio.samples.data(),
                               static_cast<int>(audio.samples.size() * sizeof(std::int16_t)));
    } catch (const std::exception &) {
        // The callback cannot propagate exceptions across the SDL audio thread.
        // Leaving the stream short is safer than terminating the process here.
    }
}

bool startGameplayAudio(const quiky::Archive &archive, SdlState &sdl,
                        std::unique_ptr<quiky::AudioMixer> &audioMixer) {
    try {
        audioMixer.reset(new quiky::AudioMixer(archive, "ONGAME2", "ONGAME2"));
        SDL_AudioSpec spec;
        SDL_zero(spec);
        spec.freq = static_cast<int>(audioMixer->sampleRate());
        spec.format = SDL_AUDIO_S16LE;
        spec.channels = 2;
        sdl.audioStream = SDL_OpenAudioDeviceStream(
            SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &spec, nullptr, nullptr);
        if (sdl.audioStream == nullptr) {
            std::cerr << "warning: gameplay music unavailable: " << SDL_GetError() << "\n";
            audioMixer.reset();
            return false;
        }
        sdl.audioMixer = audioMixer.get();
        if (!SDL_SetAudioStreamGetCallback(sdl.audioStream, audioStreamCallback,
                                            sdl.audioMixer) ||
            !SDL_ResumeAudioStreamDevice(sdl.audioStream)) {
            std::cerr << "warning: gameplay music could not start: " << SDL_GetError() << "\n";
            SDL_DestroyAudioStream(sdl.audioStream);
            sdl.audioStream = nullptr;
            sdl.audioMixer = nullptr;
            audioMixer.reset();
            return false;
        }
        return true;
    } catch (const std::exception &error) {
        std::cerr << "warning: gameplay music unavailable: " << error.what() << "\n";
        return false;
    }
}

void triggerLevelSfx(SdlState &sdl, quiky::AudioMixer *audioMixer,
                     const quiky::LevelEvent &event) {
    const quiky::GameplaySfx effect = quiky::gameplaySfxForEvent(event);
    if (effect == quiky::GameplaySfx::Silent ||
        audioMixer == nullptr || sdl.audioStream == nullptr) {
        return;
    }
    if (!SDL_LockAudioStream(sdl.audioStream)) {
        return;
    }
    audioMixer->triggerSfx(static_cast<int>(effect));
    SDL_UnlockAudioStream(sdl.audioStream);
}
#endif

void uploadSurface(SDL_Texture *texture, const quiky::IndexedSurface &surface,
                   const quiky::Palette &palette,
                   const quiky::Palette *gamebarPalette = nullptr,
                   const quiky::Palette *smallFontPalette = nullptr) {
    std::vector<quiky::byte> rgb(surface.pixels.size() * 3);
    for (std::size_t index = 0; index < surface.pixels.size(); ++index) {
        const std::uint32_t y = static_cast<std::uint32_t>(
            index / surface.width);
        // GAMEBAR.PCC owns the DAC entries used by its 320x24 layer. The
        // world palette reuses those indices for terrain colors, so applying
        // one palette to the already-composited indexed frame turns the HUD
        // into a solid neon strip. Select the layer palette only at this
        // final RGB conversion boundary; indexed world composition remains
        // unchanged.
        const quiky::byte pixel = surface.pixels[index];
        const bool inGamebar = gamebarPalette != nullptr &&
                               y >= kWorldViewportHeight;
        // SMFONT's two-color glyphs are copied into the GAMEBAR surface at
        // runtime. GAMEBAR leaves those low indices as magenta placeholders,
        // so resolve them through SMFONT's shared UI palette while retaining
        // GAMEBAR's resource-owned 99..123 colors for the strip itself.
        const bool isSmallFontPixel =
            smallFontPalette != nullptr && (pixel == 1 || pixel == 49);
        const quiky::Palette &activePalette =
            inGamebar && isSmallFontPixel ? *smallFontPalette
            : inGamebar ? *gamebarPalette
                        : palette;
        const quiky::RGB &color = activePalette.colors[pixel];
        rgb[index * 3 + 0] = color.red;
        rgb[index * 3 + 1] = color.green;
        rgb[index * 3 + 2] = color.blue;
    }
    checkSdl(SDL_UpdateTexture(texture, nullptr, rgb.data(),
                               static_cast<int>(surface.width * 3)),
             "SDL_UpdateTexture");
}

void resetGameplayCamera(quiky::GameplayCamera &camera,
                         const quiky::LevelRuntime &runtime,
                         const quiky::PlayerRecord &player) {
    const int worldWidth = static_cast<int>(runtime.map().width) * 16;
    const int worldHeight = static_cast<int>(runtime.map().height) * 16;
    const int initialX = player.positionX.floorPixels() - kLogicalWidth / 2;
    const int centeredY =
        player.positionY.floorPixels() - kWorldViewportHeight / 2;
    // W1L1's authored/native startup anchor is (0,262).  Other levels use
    // their spawn-centered fallback until a level-specific camera trace is
    // available.
    const int initialY = runtime.mapName() == "W1L1.MAP" ? 262 : centeredY;
    camera.reset(initialX, initialY, worldWidth, worldHeight);
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
                 const quiky::PlayerRecord &player, std::uint64_t frame,
                 std::uint16_t slot, bool paused,
                 const quiky::LevelSession &level,
                 const std::string &eventText) {
    std::ostringstream title;
    title << "Quiky | " << mapName << " | " << (paused ? "PAUSED | " : "")
          << "frame=" << frame << " slot=" << slot
          << " x=" << player.positionX.floorPixels()
          << " y=" << player.positionY.floorPixels()
          << " vx=" << player.velocityX.floorPixels()
          << " vy=" << player.velocityY.floorPixels()
          << " mode=" << static_cast<int>(player.mode37)
          << " score=" << level.score()
          << " ammo=" << level.gameplayState().ammo880c
          << " deaths=" << level.deaths();
    if (!eventText.empty()) {
        title << " " << eventText;
    }
    SDL_SetWindowTitle(window, title.str().c_str());
}

quiky::IndexedSurface gameplayGamebar(const quiky::LevelRuntime &runtime) {
    quiky::IndexedSurface gamebar = runtime.gamebar().surface();
    const quiky::IndexedSurface smallFont = runtime.smallFont().surface();
    const quiky::LevelGameplayState &state = runtime.session().gameplayState();

    // 01F7:5A03-5BEC writes the live counters into the 320x24 GAMEBAR
    // surface using SMFONT's packed 6x8 decimal glyphs. Keep the positions in the
    // same bar-local coordinate system instead of baking digits into the
    // resource, so pickup and damage writes are visible immediately.
    quiky::drawSmallFontNumber(gamebar, smallFont, state.score881c, 6,
                               0xce, 8);
    quiky::drawSmallFontNumber(
        gamebar, smallFont,
        std::min<std::uint16_t>(9, state.lives880a), 1, 0x5a, 8);
    quiky::drawSmallFontNumber(
        gamebar, smallFont,
        std::min<std::uint16_t>(99, state.ammo880c), 2, 0xb4, 8);
    quiky::drawCollectedPuzzleLetters(
        gamebar, runtime.nesquikBob(), state.puzzleMask60d8);
    return gamebar;
}

bool isKey(SDL_Scancode key, SDL_Scancode first, SDL_Scancode second = SDL_SCANCODE_UNKNOWN) {
    return key == first || key == second;
}

void advancePlayerAnimation(quiky::PlayerAnimation &animation,
                            const quiky::PlayerRecord &player,
                            const quiky::LevelRuntime &runtime) {
    // The native transition gate remains signed-negative throughout the
    // death hold while the outer lifecycle consumer counts down to recovery.
    animation.advance(player,
                      runtime.session().playerDeathAnimationActive(player));
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
        quiky::Simulation simulation;
        quiky::TraceClosedPlayerUpdate playerUpdater;
        simulation.setExperimentalPlayerUpdater(&playerUpdater);
        std::unique_ptr<quiky::LevelRuntime> runtime =
            quiky::LevelRuntime::load(archive, mapName, bobName, levelConfig);
        quiky::SimulationOutput output;
        runtime->reset(simulation);
        output.player = simulation.state().player;
        quiky::PlayerRecord &player = output.player;
        quiky::PlayerAnimation playerAnimation;
        playerAnimation.reset();
        advancePlayerAnimation(playerAnimation, player, *runtime);

        checkSdl(SDL_Init(SDL_INIT_VIDEO), "SDL_Init");
#ifdef QUIKY_WITH_MUSIC
        std::unique_ptr<quiky::AudioMixer> audioMixer;
#endif
        SdlState sdl;
#ifdef QUIKY_WITH_MUSIC
        if (musicEnabled) {
            if (!SDL_InitSubSystem(SDL_INIT_AUDIO)) {
                std::cerr << "warning: gameplay music unavailable: " << SDL_GetError() << "\n";
            } else {
                startGameplayAudio(archive, sdl, audioMixer);
            }
        }
#else
        (void)musicEnabled;
#endif
        checkSdl(SDL_CreateWindowAndRenderer(
                     "Quiky", kLogicalWidth * 2, kLogicalHeight * 2,
                     SDL_WINDOW_RESIZABLE, &sdl.window, &sdl.renderer),
                 "SDL_CreateWindowAndRenderer");
        checkSdl(SDL_SetRenderLogicalPresentation(
                     sdl.renderer, kLogicalWidth, kLogicalHeight,
                     SDL_LOGICAL_PRESENTATION_INTEGER_SCALE),
                 "SDL_SetRenderLogicalPresentation");

        quiky::GameplayCamera camera(kLogicalWidth, kWorldViewportHeight);
        resetGameplayCamera(camera, *runtime, player);
        runtime->setStreamAnchor(camera.x(), camera.y());
        const quiky::IndexedSurface initialWorld =
            quiky::renderMap(runtime->map(), runtime->tileset());
        const quiky::IndexedSurface initialGamebar = gameplayGamebar(*runtime);
        const quiky::IndexedSurface initialSurface = quiky::composeGameplayFrame(
            initialWorld, initialGamebar, camera.x(), camera.y());
        sdl.texture = createSurfaceTexture(sdl.renderer, initialSurface);

        bool running = true;
        bool paused = false;
        bool left = false;
        bool right = false;
        bool upHeld = false;
        bool upQueued = false;
        bool jumpHeld = false;
        bool jumpQueued = false;
        bool alternate = false;
        bool stepRequested = false;
        std::uint64_t frame = 0;
        std::uint64_t lastTime = SDL_GetTicksNS();
        std::uint64_t accumulator = 0;
        std::uint64_t titleTime = 0;
        std::uint64_t eventUntil = 0;
        std::string eventText;
        const auto currentInput = [&]() {
            quiky::InputState input;
            input.left = left;
            input.right = right;
            input.up = upHeld || upQueued;
            input.jump = jumpHeld || jumpQueued;
            input.alternate = alternate;
            return input;
        };

        while (running) {
            const std::uint64_t now = SDL_GetTicksNS();
            const std::uint64_t elapsed = std::min<std::uint64_t>(now - lastTime, 250000000ULL);
            lastTime = now;
            accumulator += elapsed;

            SDL_Event event;
            while (SDL_PollEvent(&event)) {
                if (event.type == SDL_EVENT_QUIT) {
                    running = false;
                } else if (event.type == SDL_EVENT_WINDOW_FOCUS_LOST) {
                    left = false;
                    right = false;
                    upHeld = false;
                    jumpHeld = false;
                    upQueued = false;
                    jumpQueued = false;
                    alternate = false;
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
                    } else if (isKey(key, SDL_SCANCODE_LALT, SDL_SCANCODE_RALT)) {
                        alternate = down;
                    } else if (isKey(key, SDL_SCANCODE_SPACE, SDL_SCANCODE_W)) {
                        if (down && !event.key.repeat) {
                            jumpHeld = true;
                            jumpQueued = true;
                        } else if (!down) {
                            jumpHeld = false;
                        }
                    } else if (key == SDL_SCANCODE_UP) {
                        if (down && !event.key.repeat) {
                            upHeld = true;
                            upQueued = true;
                        } else if (!down) {
                            upHeld = false;
                        }
                    } else if (key == SDL_SCANCODE_P && down && !event.key.repeat) {
                        paused = !paused;
                    } else if (key == SDL_SCANCODE_N && down && !event.key.repeat) {
                        stepRequested = true;
                    } else if (key == SDL_SCANCODE_R && down && !event.key.repeat) {
                        runtime->reset(simulation);
                        output.player = simulation.state().player;
                        resetGameplayCamera(camera, *runtime, output.player);
                        runtime->setStreamAnchor(camera.x(), camera.y());
                        playerAnimation.reset();
                        advancePlayerAnimation(playerAnimation, player, *runtime);
                        frame = 0;
                        accumulator = 0;
                        upQueued = false;
                        jumpQueued = false;
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
                    runtime->tick(simulation, currentInput(), output);
                    advancePlayerAnimation(playerAnimation, player, *runtime);
                    ++frame;
                    upQueued = false;
                    jumpQueued = false;
                    const quiky::LevelEvent event = runtime->session().consumeEvent();
#ifdef QUIKY_WITH_MUSIC
                    triggerLevelSfx(sdl,
                                    audioMixer.get(),
                                    event);
#endif
                    if (event.type == quiky::LevelEventType::Collected) {
                        eventText = "collected";
                        eventUntil = now + 1000000000ULL;
                    } else if (event.type == quiky::LevelEventType::PlayerJumped) {
                        eventText = "jump";
                        eventUntil = now + 500000000ULL;
                    } else if (event.type == quiky::LevelEventType::EntityCollisionImpact) {
                        eventText = "entity-impact";
                        eventUntil = now + 1000000000ULL;
                    } else if (event.type == quiky::LevelEventType::TileInteraction) {
                        eventText = "tile-interaction";
                        eventUntil = now + 1000000000ULL;
                    } else if (event.type == quiky::LevelEventType::AlternateActionObject) {
                        eventText = "alternate-action";
                        eventUntil = now + 1000000000ULL;
                    } else if (event.type == quiky::LevelEventType::PooledObjectInteractionBurst) {
                        eventText = "object-interaction-burst";
                        eventUntil = now + 1000000000ULL;
                    } else if (event.type == quiky::LevelEventType::WorldObjectInteraction) {
                        eventText = "world-object";
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
                            quiky::LevelSessionConfig transitionConfig = levelConfig;
                            // Explicit coordinates are a fixture/startup
                            // override, not a replacement for 1AAA's target
                            // level spawn declaration.
                            transitionConfig.hasSpawn = false;
                            quiky::LevelReloadTrace reloadTrace;
                            std::unique_ptr<quiky::LevelRuntime> next =
                                runtime->reload(archive, event.targetLevel,
                                                simulation, transitionConfig,
                                                &reloadTrace);
                            runtime.swap(next);
                            output.player = simulation.state().player;
                            resetGameplayCamera(camera, *runtime, output.player);
                            runtime->setStreamAnchor(camera.x(), camera.y());
                            const quiky::IndexedSurface nextWorld =
                                quiky::renderMap(runtime->map(), runtime->tileset());
                            const quiky::IndexedSurface nextGamebar =
                                gameplayGamebar(*runtime);
                            const quiky::IndexedSurface nextSurface =
                                quiky::composeGameplayFrame(
                                    nextWorld, nextGamebar,
                                    camera.x(), camera.y());
                            replaceSurfaceTexture(sdl, nextSurface);
                            playerAnimation.reset();
                            advancePlayerAnimation(playerAnimation, player, *runtime);
                            frame = 0;
                            accumulator = 0;
                            upQueued = false;
                            jumpQueued = false;
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
                runtime->tick(simulation, currentInput(), output);
                advancePlayerAnimation(playerAnimation, player, *runtime);
                ++frame;
                upQueued = false;
                jumpQueued = false;
                stepRequested = false;
                const quiky::LevelEvent event = runtime->session().consumeEvent();
#ifdef QUIKY_WITH_MUSIC
                triggerLevelSfx(sdl,
                                audioMixer.get(),
                                event);
#endif
            }

            quiky::Palette framePalette = runtime->palette();
            quiky::IndexedSurface worldSurface =
                quiky::renderMap(runtime->map(), runtime->tileset());
            if (showArea) {
                quiky::overlayArea(worldSurface, framePalette, runtime->area());
            }
            if (showEntities) {
                drawEntityMarkers(worldSurface, framePalette, runtime->session());
            }
            quiky::drawLevelEntities(worldSurface, *runtime);
            quiky::drawTransientEffects(worldSurface, *runtime);
            quiky::drawPlayerSprite(worldSurface, *runtime,
                                    playerAnimation, player);

            camera.follow(player.positionX.floorPixels(),
                          player.positionY.floorPixels(),
                          static_cast<int>(worldSurface.width),
                          static_cast<int>(worldSurface.height));
            const int cameraX = camera.x();
            const int cameraY = camera.y();
            const quiky::IndexedSurface gamebar = gameplayGamebar(*runtime);
            const quiky::IndexedSurface surface = quiky::composeGameplayFrame(
                worldSurface, gamebar, cameraX, cameraY);
            // The native ARE object gate is camera-relative. Publish the
            // renderer's settled camera for the next simulation tick rather
            // than deriving the stream window from player Y.
            runtime->setStreamAnchor(cameraX, cameraY);
            uploadSurface(sdl.texture, surface, framePalette,
                          &runtime->gamebar().palette,
                          &runtime->smallFont().palette);
            const SDL_FRect source = {
                0.0f, 0.0f, static_cast<float>(kLogicalWidth),
                static_cast<float>(kLogicalHeight)};
            const SDL_FRect destination = {
                0.0f, 0.0f, static_cast<float>(kLogicalWidth),
                static_cast<float>(kLogicalHeight)};
            checkSdl(SDL_RenderClear(sdl.renderer), "SDL_RenderClear");
            checkSdl(SDL_RenderTexture(sdl.renderer, sdl.texture, &source, &destination),
                     "SDL_RenderTexture");
            SDL_RenderPresent(sdl.renderer);

            if (now - titleTime >= 250000000ULL) {
                if (now >= eventUntil) {
                    eventText.clear();
                }
                updateTitle(sdl.window, runtime->mapName(), player, frame,
                            quiky::selectPlayerFrame(*runtime, playerAnimation).slot,
                            paused, runtime->session(), eventText);
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
