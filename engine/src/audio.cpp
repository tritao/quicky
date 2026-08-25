#include "quiky/audio.h"

#include <algorithm>
#include <limits>

namespace quiky {

namespace {

std::int16_t clampPcm(int value) {
    if (value < std::numeric_limits<std::int16_t>::min()) {
        return std::numeric_limits<std::int16_t>::min();
    }
    if (value > std::numeric_limits<std::int16_t>::max()) {
        return std::numeric_limits<std::int16_t>::max();
    }
    return static_cast<std::int16_t>(value);
}

} // namespace

AudioMixer::AudioMixer(const Archive &archive, const std::string &musicTrack,
                       const std::string &sfxTrack, std::uint32_t musicSeconds)
    : _music(archive, musicTrack),
      _sfx(archive, sfxTrack),
      _musicAudio(_music.render(0, musicSeconds, true)),
      _musicFrame(0),
      _framesUntilSfxTick(0) {}

void AudioMixer::resetSfx() {
    _sfx.reset();
}

bool AudioMixer::triggerSfx(int id) {
    return _sfx.trigger(id);
}

Pcm16Stereo AudioMixer::mix(std::size_t frames) {
    Pcm16Stereo output;
    output.sampleRate = sampleRate();
    output.samples.reserve(frames * 2);

    while (frames != 0) {
        if (_framesUntilSfxTick == 0) {
            _sfx.advance();
            _framesUntilSfxTick = sampleRate() / 50;
        }
        const std::size_t chunk = std::min(frames, _framesUntilSfxTick);
        const Pcm16Stereo sfxAudio = _sfx.mix(chunk, sampleRate());
        for (std::size_t frame = 0; frame < chunk; ++frame) {
            const std::size_t musicIndex = _musicFrame * 2;
            const std::size_t sfxIndex = frame * 2;
            const int musicLeft = _musicAudio.samples.empty()
                                      ? 0
                                      : _musicAudio.samples[musicIndex];
            const int musicRight = _musicAudio.samples.empty()
                                       ? 0
                                       : _musicAudio.samples[musicIndex + 1];
            output.samples.push_back(clampPcm(
                musicLeft + sfxAudio.samples[sfxIndex]));
            output.samples.push_back(clampPcm(
                musicRight + sfxAudio.samples[sfxIndex + 1]));
            if (!_musicAudio.samples.empty()) {
                _musicFrame = (_musicFrame + 1) % _musicAudio.frames();
            }
        }
        _framesUntilSfxTick -= chunk;
        frames -= chunk;
    }
    return output;
}

} // namespace quiky
