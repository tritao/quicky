#ifndef QUIKY_RUNTIME_H
#define QUIKY_RUNTIME_H

#include <array>
#include <cstddef>
#include <cstdint>
#include <string>

namespace quiky {

struct InputState {
    bool left;
    bool right;
    bool up;
    bool down;
    bool jump;
    bool alternate;

    InputState()
        : left(false), right(false), up(false), down(false), jump(false),
          alternate(false) {}

    static InputState fromActionFlags(std::uint16_t flags);
    std::uint16_t actionFlags() const;
};

// Rules recovered from the protected-mode 5C27/3D02 probes.
struct PlayerDescriptorRules {
    static const std::size_t kEntryCount = 512;

    static std::uint16_t quadrantMask(std::uint16_t x, std::uint16_t y);
    static bool blocksProbe(std::uint16_t descriptor,
                            std::uint16_t x, std::uint16_t y);
    static bool hasVerticalResponse(std::uint16_t descriptor);
    static bool alignsEightPixels(std::uint16_t descriptor);
    static std::uint16_t snapProbeY(std::uint16_t y);
};

// The executable keeps collision descriptors in a runtime table separate
// from the upper seven MAP-cell property bits.  The table is deliberately
// supplied by the caller: its bytes come from the loaded game's descriptor
// segment, not from the MAP archive payload.
class PlayerDescriptorTable {
public:
    PlayerDescriptorTable();
    explicit PlayerDescriptorTable(
        const std::array<std::uint16_t, PlayerDescriptorRules::kEntryCount> &words);

    std::uint16_t word(std::uint16_t tileId) const;
    void setWord(std::uint16_t tileId, std::uint16_t descriptor);

private:
    std::array<std::uint16_t, PlayerDescriptorRules::kEntryCount> _words;
};

// The retail executable builds one 512-entry descriptor table per world from
// its world initializer.  Keep the recovered tables in the engine so MAP
// collision uses the same tile-ID-to-descriptor mapping as the game.
PlayerDescriptorTable playerDescriptorTableForWorld(const std::string &worldName);

} // namespace quiky

#endif
