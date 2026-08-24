#ifndef QUIKY_MUSIC_H
#define QUIKY_MUSIC_H

#include "quiky/archive.h"

#include <cstdint>
#include <string>
#include <vector>

namespace quiky {

class TfmxDecoder;

struct MusicTrack {
    std::string name;
    std::uint32_t tfxSize;
    std::uint32_t samSize;
};

struct MusicInfo {
    std::string track;
    std::string formatId;
    std::string formatName;
    int songs;
    int voices;
    std::uint32_t durationMs;
};

struct Pcm16Stereo {
    std::uint32_t sampleRate;
    std::vector<std::int16_t> samples;

    std::size_t frames() const { return samples.size() / 2; }
};

std::vector<MusicTrack> listMusicTracks(const Archive &archive);

class MusicModule {
public:
    MusicModule(const Archive &archive, const std::string &track);
    ~MusicModule();

    MusicModule(const MusicModule &) = delete;
    MusicModule &operator=(const MusicModule &) = delete;

    MusicInfo info(int song = 0);
    Pcm16Stereo render(int song = 0, std::uint32_t maxSeconds = 180,
                       bool loop = false);

private:
    void load(int song);

    std::string _track;
    TfmxDecoder *_decoder;
    int _loadedSong;
};

void writeWave(const std::string &path, const Pcm16Stereo &audio);

} // namespace quiky

#endif
