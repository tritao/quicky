#ifndef QUIKY_AUDIO_H
#define QUIKY_AUDIO_H

#include "quiky/sfx.h"

#include <cstddef>
#include <cstdint>
#include <string>

namespace quiky {

// The runtime mixer keeps the music timeline and the 50 Hz SFX timeline in
// one PCM stream so effects are inserted at the same point the game emits
// them, instead of being played through a second unsynchronized device.
class AudioMixer {
public:
    AudioMixer(const Archive &archive, const std::string &musicTrack,
               const std::string &sfxTrack, std::uint32_t musicSeconds = 180);

    AudioMixer(const AudioMixer &) = delete;
    AudioMixer &operator=(const AudioMixer &) = delete;

    void resetSfx();
    bool triggerSfx(int id);
    Pcm16Stereo mix(std::size_t frames);

    std::uint32_t sampleRate() const { return 44100; }

private:
    MusicModule _music;
    SfxModule _sfx;
    Pcm16Stereo _musicAudio;
    std::size_t _musicFrame;
    std::size_t _framesUntilSfxTick;
};

} // namespace quiky

#endif
