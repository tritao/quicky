#ifndef QUIKY_PLAYER_RECORD_H
#define QUIKY_PLAYER_RECORD_H

#include "quiky/fixed.h"
#include "quiky/types.h"

#include <array>
#include <cstddef>
#include <cstdint>

namespace quiky {

// This is the exact byte container observed at ES:DI. Unknown bytes remain
// addressable through their offset in raw; they are never dropped when a
// typed projection is edited.
struct PlayerRawRecord {
    static const std::size_t kSize = 0x78;

    std::array<byte, kSize> bytes;

    PlayerRawRecord();

    static PlayerRawRecord fromBytes(const Bytes &source,
                                     std::size_t offset = 0);
    Bytes toBytes() const;

    std::uint8_t u8(std::size_t offset) const;
    std::int8_t s8(std::size_t offset) const;
    std::uint16_t u16(std::size_t offset) const;
    std::int16_t s16(std::size_t offset) const;
    std::uint32_t u32(std::size_t offset) const;
    std::int32_t s32(std::size_t offset) const;

    void setU8(std::size_t offset, std::uint8_t value);
    void setU16(std::size_t offset, std::uint16_t value);
    void setU32(std::size_t offset, std::uint32_t value);
    void setS32(std::size_t offset, std::int32_t value);

private:
    static void checkRange(std::size_t offset, std::size_t width);
};

enum class PlayerFieldConfidence {
    Unknown,
    ObservedLayout,
    MechanicallyConfirmed
};

// Typed projection of the recovered 0x78-byte record. The names and offsets
// below are evidence-qualified by research/notes/player-contract.md. Fields
// that are not yet semantically understood intentionally retain fieldXX names.
// The legacy PlayerState in runtime.h is a separate provisional frontend type.
struct RecoveredPlayerState {
    PlayerRawRecord raw;

    // Mechanically confirmed fixed-point layout.
    std::uint16_t actionWord;
    Fixed16 positionX;
    Fixed16 positionY;
    Fixed16 velocityX;
    Fixed16 velocityY;

    // Stable names for observed bytes/words whose gameplay meaning is still
    // under investigation.
    std::uint16_t field12;
    std::uint16_t field14;
    std::uint16_t field16;
    std::uint16_t callbackOffset18;
    std::uint16_t field1A;
    std::uint16_t field1C;
    std::uint16_t field1E;
    std::uint16_t field20;
    std::uint16_t field22;
    std::uint16_t field24;
    std::uint16_t field26;
    std::uint8_t inputByte28;
    std::uint8_t animationByte29;
    std::uint16_t field2A;
    std::uint16_t field2C;
    std::uint16_t field2E;
    std::uint16_t field30;
    std::uint16_t callbackState32;
    std::uint16_t timer34;
    std::uint8_t animationState36;
    std::int8_t mode37;
    std::uint8_t gate38;
    std::uint8_t transition39;
    std::uint8_t verticalResponse3A;
    std::uint8_t sideResponse3B;
    std::uint16_t field3C;
    std::uint16_t resetDeathTimer3E;
    std::uint16_t callbackCounter40;
    std::uint16_t field42;
    Fixed16 savedX44;
    Fixed16 savedY48;
    std::int32_t field4C;
    std::int32_t field50;
    std::int32_t field54;
    std::int32_t field58;
    std::int32_t field5C;
    std::int32_t field60;
    std::int32_t field64;
    std::uint32_t field68;
    std::uint32_t field6C;
    std::uint16_t field70;
    std::int16_t field72;
    std::uint16_t field74;
    std::uint16_t field76;

    RecoveredPlayerState();

    static RecoveredPlayerState fromRaw(const PlayerRawRecord &record);
    void syncFromRaw();
    void syncToRaw();
    PlayerRawRecord toRaw() const;
};

} // namespace quiky

#endif
