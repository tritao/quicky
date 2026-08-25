#include "quiky/sfx.h"

#include <cstdlib>
#include <iostream>
#include <string>

#ifdef QUIKY_WITH_SDL
#include <SDL3/SDL.h>
#endif

namespace {

void usage() {
    std::cerr << "usage: quiky-sfx ARCHIVE list TRACK\n"
              << "       quiky-sfx ARCHIVE info TRACK ID\n"
              << "       quiky-sfx ARCHIVE render TRACK ID OUTPUT.wav [SECONDS]\n"
              << "       quiky-sfx ARCHIVE play TRACK ID [SECONDS]\n";
}

int parseNumber(const char *value, const char *name) {
    char *end = nullptr;
    const long result = std::strtol(value, &end, 0);
    if (end == value || *end != '\0' || result < 0 || result > 0x7fffffffL) {
        throw quiky::FormatError(std::string("invalid ") + name + ": " + value);
    }
    return static_cast<int>(result);
}

std::uint32_t parseSeconds(const char *value) {
    const int seconds = parseNumber(value, "seconds");
    if (seconds == 0) {
        throw quiky::FormatError("seconds must be positive");
    }
    return static_cast<std::uint32_t>(seconds);
}

#ifdef QUIKY_WITH_SDL
void playAudio(const quiky::Pcm16Stereo &audio) {
    if (!SDL_Init(SDL_INIT_AUDIO)) {
        throw quiky::FormatError(std::string("SDL audio initialization failed: ") + SDL_GetError());
    }

    SDL_AudioSpec spec;
    SDL_zero(spec);
    spec.freq = static_cast<int>(audio.sampleRate);
    spec.format = SDL_AUDIO_S16LE;
    spec.channels = 2;
    SDL_AudioStream *stream = SDL_OpenAudioDeviceStream(
        SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &spec, nullptr, nullptr);
    if (stream == nullptr) {
        const std::string error = SDL_GetError();
        SDL_Quit();
        throw quiky::FormatError("SDL audio device failed: " + error);
    }
    const int bytes = static_cast<int>(audio.samples.size() * sizeof(std::int16_t));
    if (!SDL_PutAudioStreamData(stream, audio.samples.data(), bytes) ||
        !SDL_FlushAudioStream(stream) || !SDL_ResumeAudioStreamDevice(stream)) {
        const std::string error = SDL_GetError();
        SDL_DestroyAudioStream(stream);
        SDL_Quit();
        throw quiky::FormatError("SDL audio playback failed: " + error);
    }
    const std::uint32_t durationMs = static_cast<std::uint32_t>(
        (audio.frames() * 1000ULL) / audio.sampleRate);
    SDL_Delay(durationMs + 100);
    SDL_DestroyAudioStream(stream);
    SDL_Quit();
}
#endif

} // namespace

int main(int argc, char **argv) {
    if (argc < 4 || argc > 7) {
        usage();
        return EXIT_FAILURE;
    }

    try {
        const quiky::Archive archive = quiky::Archive::load(argv[1]);
        const std::string command(argv[2]);
        const std::string track(argv[3]);
        quiky::SfxModule module(archive, track);

        if (command == "list" && argc == 4) {
            for (int id = 0; id < module.effects(); ++id) {
                const quiky::SfxEffectInfo effect = module.info(id);
                std::cout << effect.id << " voice=" << effect.voice
                          << " priority=" << effect.priority
                          << " macro=0x" << std::hex << effect.macroOffset
                          << " sample=0x" << effect.sampleStart
                          << " length=" << std::dec << effect.sampleLength << "\n";
            }
            return EXIT_SUCCESS;
        }

        if (command == "info" && argc == 5) {
            const quiky::SfxEffectInfo effect = module.info(parseNumber(argv[4], "effect ID"));
            std::cout << "id: " << effect.id << "\n"
                      << "voice: " << effect.voice << "\n"
                      << "priority: " << effect.priority << "\n"
                      << "macro-offset: 0x" << std::hex << effect.macroOffset << std::dec << "\n"
                      << "sample-start: 0x" << std::hex << effect.sampleStart << std::dec << "\n"
                      << "sample-length: " << effect.sampleLength << "\n";
            return EXIT_SUCCESS;
        }

        if (command == "render" && (argc == 6 || argc == 7)) {
            const int id = parseNumber(argv[4], "effect ID");
            const std::uint32_t seconds = argc == 7 ? parseSeconds(argv[6]) : 3;
            const quiky::Pcm16Stereo audio = module.render(id, seconds);
            quiky::writeWave(argv[5], audio);
            std::cout << "wrote " << audio.frames() << " frames at "
                      << audio.sampleRate << " Hz to " << argv[5] << "\n";
            return EXIT_SUCCESS;
        }

        if (command == "play" && (argc == 5 || argc == 6)) {
#ifdef QUIKY_WITH_SDL
            const int id = parseNumber(argv[4], "effect ID");
            const std::uint32_t seconds = argc == 6 ? parseSeconds(argv[5]) : 3;
            playAudio(module.render(id, seconds));
            return EXIT_SUCCESS;
#else
            throw quiky::FormatError("this build has no SDL3 audio backend; use render or configure SDL3");
#endif
        }
    } catch (const std::exception &error) {
        std::cerr << "error: " << error.what() << "\n";
        return EXIT_FAILURE;
    }

    usage();
    return EXIT_FAILURE;
}
