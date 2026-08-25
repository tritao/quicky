#include "quiky/archive.h"
#include "quiky/audio.h"
#include "quiky/sfx.h"

#include <cassert>
#include <cstdint>
#include <iostream>

namespace {

bool hasSignal(const quiky::Pcm16Stereo &audio) {
    for (std::size_t index = 0; index < audio.samples.size(); ++index) {
        if (audio.samples[index] != 0) {
            return true;
        }
    }
    return false;
}

void testGameplayEffects(const std::string &archivePath) {
    const quiky::Archive archive = quiky::Archive::load(archivePath);
    quiky::SfxModule module(archive, "ONGAME2");
    assert(module.effects() == 14);

    const quiky::SfxEffectInfo first = module.info(0);
    assert(first.voice == 3);
    assert(first.priority == 0x60);
    assert(first.macroOffset == 0x2b18);
    assert(first.sampleStart == 0x7212);
    assert(first.sampleLength == 0x20);

    const quiky::SfxEffectInfo last = module.info(13);
    assert(last.priority == 0x6a);
    assert(last.sampleStart + last.sampleLength == 0xf3a2);

    for (int id = 0; id < module.effects(); ++id) {
        const quiky::Pcm16Stereo audio = module.render(id, 2);
        assert(audio.sampleRate == 44100);
        assert(audio.frames() > 0);
        assert(hasSignal(audio));
    }

    module.reset();
    assert(module.trigger(13));
    assert(!module.trigger(3));
    module.reset();
    assert(module.trigger(3));
    assert(module.trigger(13));

    module.reset();
    assert(!module.active());
    assert(module.trigger(0));
    assert(module.active());
    bool blockHasSignal = false;
    for (int tick = 0; tick < 5; ++tick) {
        module.advance();
        const quiky::Pcm16Stereo block = module.mix(882);
        assert(block.frames() == 882);
        blockHasSignal = blockHasSignal || hasSignal(block);
    }
    assert(blockHasSignal);

    quiky::AudioMixer mixer(archive, "ONGAME2", "ONGAME2", 1);
    assert(mixer.sampleRate() == 44100);
    assert(mixer.triggerSfx(0));
    const quiky::Pcm16Stereo mixed = mixer.mix(4410);
    assert(mixed.frames() == 4410);
    assert(hasSignal(mixed));
}

} // namespace

int main(int argc, char **argv) {
    try {
        if (argc != 2) {
            std::cerr << "usage: quiky-sfx-tests ARCHIVE\n";
            return 1;
        }
        testGameplayEffects(argv[1]);
    } catch (const std::exception &error) {
        std::cerr << "unexpected SFX test failure: " << error.what() << "\n";
        return 1;
    }
    std::cout << "all quiky SFX tests passed\n";
    return 0;
}
