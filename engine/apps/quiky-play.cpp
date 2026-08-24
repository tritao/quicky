#include "quiky/archive.h"
#include "quiky/area.h"
#include "quiky/bob.h"
#include "quiky/map.h"
#include "quiky/palette.h"
#include "quiky/renderer.h"
#include "quiky/runtime.h"
#include "quiky/tileset.h"

#include <SDL3/SDL.h>

#include <algorithm>
#include <cctype>
#include <cstdint>
#include <cstdlib>
#include <exception>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

namespace {

const int kViewportWidth = 640;
const int kViewportHeight = 360;
const std::uint64_t kTickNanoseconds = 1000000000ULL / 60ULL;

void usage() {
    std::cerr << "usage: quiky-play ARCHIVE MAP-RESOURCE BOB-RESOURCE "
                 "[START-X START-Y] [--overlay-are]\n"
                 "controls: arrows/A,D move, space/W/up jump, R reset, "
                 "P pause, N step, F1 toggle ARE, Esc quit\n";
}

long parseNumber(const std::string &value, const char *name) {
    char *end = nullptr;
    const long result = std::strtol(value.c_str(), &end, 0);
    if (end == value.c_str() || *end != '\0') {
        throw quiky::FormatError(std::string("invalid ") + name + ": " + value);
    }
    return result;
}

std::string worldFor(const std::string &mapName) {
    if (mapName.size() < 2 || (mapName[0] != 'W' && mapName[0] != 'w')) {
        throw quiky::FormatError("cannot derive world from MAP resource name");
    }
    std::string world(mapName.substr(0, 2));
    world[0] = 'W';
    world[1] = static_cast<char>(std::toupper(static_cast<unsigned char>(world[1])));
    return world;
}

std::string areaNameFor(const std::string &mapName) {
    const std::size_t extension = mapName.find_last_of('.');
    return mapName.substr(0, extension) + ".ARE";
}

const quiky::BobRecord *findSlot(const quiky::Bob &bob, std::uint16_t slot) {
    for (std::size_t index = 0; index < bob.records.size(); ++index) {
        if (bob.records[index].slot == slot) {
            return &bob.records[index];
        }
    }
    return nullptr;
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

    SdlState() : window(nullptr), renderer(nullptr), texture(nullptr) {}

    ~SdlState() {
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

void updateTitle(SDL_Window *window, const std::string &mapName,
                 const quiky::PlayerState &player, std::uint64_t frame,
                 std::uint16_t slot, bool paused) {
    std::ostringstream title;
    title << "Quiky | " << mapName << " | " << (paused ? "PAUSED | " : "")
          << "frame=" << frame << " slot=" << slot
          << " x=" << player.x.floorPixels()
          << " y=" << player.y.floorPixels()
          << " vx=" << player.velocityX.floorPixels()
          << " vy=" << player.velocityY.floorPixels()
          << (player.grounded ? " grounded" : " airborne");
    SDL_SetWindowTitle(window, title.str().c_str());
}

bool isKey(SDL_Scancode key, SDL_Scancode first, SDL_Scancode second = SDL_SCANCODE_UNKNOWN) {
    return key == first || key == second;
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
        const std::string bobName(argv[3]);
        const std::string world = worldFor(mapName);

        std::vector<std::string> positional;
        bool showArea = false;
        for (int index = 4; index < argc; ++index) {
            const std::string argument(argv[index]);
            if (argument == "--overlay-are") {
                showArea = true;
            } else {
                positional.push_back(argument);
            }
        }
        if (positional.size() != 0 && positional.size() != 2) {
            usage();
            return EXIT_FAILURE;
        }

        const long startX = positional.empty() ? 100 : parseNumber(positional[0], "start X");
        const long startY = positional.empty() ? 100 : parseNumber(positional[1], "start Y");
        if (startX < 0 || startY < 0 || startX > 0x7fffffffL || startY > 0x7fffffffL) {
            throw quiky::FormatError("start coordinates must be non-negative 32-bit values");
        }

        const quiky::Map map = quiky::Map::parse(archive.read(mapName), mapName);
        const quiky::Palette palette = quiky::Palette::parsePcx(
            archive.read(world + ".PCC"), world + ".PCC");
        const quiky::Tileset tileset = quiky::Tileset::parseIco(
            archive.read(world + ".ICO"), world + ".ICO");
        const quiky::Bob bob = quiky::Bob::parse(archive.read(bobName), bobName);
        const std::string areaName = areaNameFor(mapName);
        const quiky::Area area = quiky::Area::parse(
            archive.read(areaName), areaName);

        quiky::PlayerSimulation simulation;
        quiky::PlayerState player;
        simulation.reset(player, static_cast<std::int32_t>(startX),
                         static_cast<std::int32_t>(startY));

        checkSdl(SDL_Init(SDL_INIT_VIDEO), "SDL_Init");
        SdlState sdl;
        checkSdl(SDL_CreateWindowAndRenderer(
                     "Quiky", kViewportWidth * 2, kViewportHeight * 2,
                     SDL_WINDOW_RESIZABLE, &sdl.window, &sdl.renderer),
                 "SDL_CreateWindowAndRenderer");
        checkSdl(SDL_SetRenderLogicalPresentation(
                     sdl.renderer, kViewportWidth, kViewportHeight,
                     SDL_LOGICAL_PRESENTATION_INTEGER_SCALE),
                 "SDL_SetRenderLogicalPresentation");

        const quiky::IndexedSurface initialSurface = quiky::renderMap(map, tileset);
        sdl.texture = SDL_CreateTexture(
            sdl.renderer, SDL_PIXELFORMAT_RGB24, SDL_TEXTUREACCESS_STREAMING,
            static_cast<int>(initialSurface.width), static_cast<int>(initialSurface.height));
        if (sdl.texture == nullptr) {
            throw quiky::FormatError(std::string("SDL_CreateTexture: ") + SDL_GetError());
        }
        checkSdl(SDL_SetTextureScaleMode(sdl.texture, SDL_SCALEMODE_NEAREST),
                 "SDL_SetTextureScaleMode");

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
                        simulation.reset(player, static_cast<std::int32_t>(startX),
                                         static_cast<std::int32_t>(startY));
                        frame = 0;
                        accumulator = 0;
                    } else if (key == SDL_SCANCODE_F1 && down && !event.key.repeat) {
                        showArea = !showArea;
                    }
                }
            }

            while (accumulator >= kTickNanoseconds) {
                if (!paused) {
                    quiky::InputState input;
                    input.left = left;
                    input.right = right;
                    input.jump = jumpPressed;
                    simulation.tick(player, map, input);
                    ++frame;
                    jumpPressed = false;
                }
                accumulator -= kTickNanoseconds;
            }
            if (paused && stepRequested) {
                quiky::InputState input;
                input.left = left;
                input.right = right;
                input.jump = jumpPressed;
                simulation.tick(player, map, input);
                ++frame;
                jumpPressed = false;
                stepRequested = false;
            }

            quiky::Palette framePalette = palette;
            quiky::IndexedSurface surface = quiky::renderMap(map, tileset);
            if (showArea) {
                quiky::overlayArea(surface, framePalette, area);
            }
            const quiky::BobRecord &record = choosePlayerFrame(bob, player, frame);
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
                updateTitle(sdl.window, mapName, player, frame, record.slot, paused);
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
