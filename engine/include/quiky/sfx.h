#ifndef QUIKY_SFX_H
#define QUIKY_SFX_H

#include "quiky/music.h"

#include <cstddef>

namespace quiky {

struct SfxEffectInfo {
    int id;
    int voice;
    int priority;
    std::uint32_t macroOffset;
    std::uint32_t sampleStart;
    std::uint32_t sampleLength;
};

class SfxModule {
public:
    SfxModule(const Archive &archive, const std::string &track);
    ~SfxModule();

    SfxModule(const SfxModule &) = delete;
    SfxModule &operator=(const SfxModule &) = delete;

    int effects() const;
    SfxEffectInfo info(int id) const;

    void reset();
    bool trigger(int id);
    void advance();
    bool active() const;
    Pcm16Stereo mix(std::size_t frames, std::uint32_t sampleRate = 44100);
    Pcm16Stereo render(std::uint32_t maxSeconds = 3);
    Pcm16Stereo render(int id, std::uint32_t maxSeconds = 3);

private:
    class Decoder;
    Decoder *_decoder;
};

} // namespace quiky

#endif
