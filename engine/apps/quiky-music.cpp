#include "quiky/music.h"

#include <cstdlib>
#include <iostream>
#include <string>

#ifdef QUIKY_WITH_SDL
#include <SDL3/SDL.h>
#endif

namespace {

void usage() {
    std::cerr << "usage: quiky-music ARCHIVE list\n"
              << "       quiky-music ARCHIVE info TRACK [SONG]\n"
              << "       quiky-music ARCHIVE render TRACK OUTPUT.wav [SONG]\n"
              << "       quiky-music ARCHIVE play TRACK [SONG]\n";
}

int parseSong(const char *value) {
    const int song = std::atoi(value);
    if (song < 0) {
        throw quiky::FormatError("song number cannot be negative");
    }
    return song;
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
    if (!stream) {
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
    SDL_Delay(durationMs + 250);
    SDL_DestroyAudioStream(stream);
    SDL_Quit();
}
#endif

} // namespace

int main(int argc, char **argv) {
    if (argc < 3 || argc > 6) {
        usage();
        return EXIT_FAILURE;
    }

    try {
        const quiky::Archive archive = quiky::Archive::load(argv[1]);
        const std::string command(argv[2]);
        if (command == "list" && argc == 3) {
            const std::vector<quiky::MusicTrack> tracks = quiky::listMusicTracks(archive);
            for (std::size_t i = 0; i < tracks.size(); ++i) {
                std::cout << tracks[i].name << " TFX=" << tracks[i].tfxSize
                          << " SAM=" << tracks[i].samSize << "\n";
            }
            return EXIT_SUCCESS;
        }

        if ((command == "info" || command == "render" || command == "play") && argc >= 4) {
            const std::string track(argv[3]);
            int song = 0;
            if ((command == "info" && argc == 5) ||
                (command == "render" && argc == 6) ||
                (command == "play" && argc == 5)) {
                song = parseSong(argv[argc - 1]);
            }

            quiky::MusicModule module(archive, track);
            if (command == "info" && (argc == 4 || argc == 5)) {
                const quiky::MusicInfo info = module.info(song);
                std::cout << "track: " << info.track << "\n"
                          << "format: " << info.formatId << " (" << info.formatName << ")\n"
                          << "songs: " << info.songs << "\n"
                          << "voices: " << info.voices << "\n"
                          << "duration-ms: " << info.durationMs << "\n";
                return EXIT_SUCCESS;
            }
            if (command == "render" && (argc == 5 || argc == 6)) {
                const quiky::Pcm16Stereo audio = module.render(song);
                quiky::writeWave(argv[4], audio);
                std::cout << "wrote " << audio.frames() << " frames at "
                          << audio.sampleRate << " Hz to " << argv[4] << "\n";
                return EXIT_SUCCESS;
            }
            if (command == "play" && (argc == 4 || argc == 5)) {
#ifdef QUIKY_WITH_SDL
                playAudio(module.render(song));
                return EXIT_SUCCESS;
#else
                throw quiky::FormatError("this build has no SDL3 audio backend; use render or configure SDL3");
#endif
            }
        }
    } catch (const std::exception &error) {
        std::cerr << "error: " << error.what() << "\n";
        return EXIT_FAILURE;
    }

    usage();
    return EXIT_FAILURE;
}
