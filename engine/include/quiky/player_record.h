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

struct PlayerRecordLayout {
    enum : std::size_t {
        kSize = 0x78,
        kActionWord = 0x00,
        kXFixed = 0x02,
        kYFixed = 0x06,
        kVelocityXFixed = 0x0a,
        kVelocityYFixed = 0x0e,
        kStatusWord = 0x12,
        kCallbackOffset = 0x18,
        kDirectionByte = 0x28,
        kMotionDirectionByte = 0x29,
        kActionCounter = 0x2a,
        kContactScratch = 0x2b,
        kState2C = 0x2c,
        kVerticalStepOrDirection = 0x2e,
        kState30 = 0x30,
        kCallbackState = 0x32,
        kTimer = 0x34,
        kAnimationState = 0x36,
        kMode = 0x37,
        kCollisionGate = 0x38,
        kTransitionPending = 0x39,
        kVerticalResponse = 0x3a,
        kSideResponse = 0x3b,
        kResetDeathTimer = 0x3e,
        kCallbackCounter = 0x40,
        kSavedY = 0x44,
        kSavedX = 0x48,
        kAcceleration = 0x4c,
        kPositiveYAcceleration = 0x50,
        kFriction = 0x54,
        kNegativeYAcceleration = 0x58,
        kHorizontalSpeedCap = 0x5c,
        kPositiveYSpeedCap = 0x60,
        kNegativeYSpeed = 0x64,
        kVerticalStepPixels = 0x72
    };
};

static_assert(PlayerRawRecord::kSize == PlayerRecordLayout::kSize,
              "player record size constant drifted");
static_assert(sizeof(PlayerRawRecord::bytes) == PlayerRecordLayout::kSize,
              "player raw record must contain exactly 0x78 bytes");

enum class PlayerFieldConfidence {
    Unknown,
    ObservedLayout,
    MechanicallyConfirmed
};

// Typed projection of the recovered 0x78-byte record. The names and offsets
// below are evidence-qualified by research/notes/player-static-closure.md and
// research/notes/player-horizontal.md. Fields
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

    // Exact static-closure fields. Unknown bytes retain offset names and are
    // intentionally not promoted to gameplay concepts.
    std::uint16_t statusWord12;
    std::uint16_t field14;
    std::uint8_t field16;
    std::uint8_t field17;
    std::uint16_t callbackOffset18;
    std::uint16_t field1A;
    std::uint16_t field1C;
    std::uint16_t field1E;
    std::uint16_t animationDelay20;
    std::uint16_t animationCursor22;
    std::uint16_t field24;
    std::uint16_t field26;
    std::uint8_t directionByte28;
    std::uint8_t motionDirectionByte29;
    std::uint8_t actionCounter2A;
    std::uint8_t contactScratch2B;
    std::int16_t state2C;
    std::int16_t verticalStepOrDirection2E;
    std::int16_t state30;
    std::uint16_t callbackState32;
    std::uint16_t timer34;
    std::uint8_t animationState36;
    std::int8_t mode37;
    std::uint8_t gate38;
    std::uint8_t transition39;
    std::int8_t verticalResponse3A;
    std::uint8_t sideResponse3B;
    std::uint16_t field3C;
    std::uint16_t resetDeathTimer3E;
    std::uint16_t callbackCounter40;
    std::uint16_t field42;
    Fixed16 savedY44;
    Fixed16 savedX48;
    Fixed16 acceleration4C;
    Fixed16 positiveYAcceleration50;
    Fixed16 friction54;
    Fixed16 negativeYAcceleration58;
    Fixed16 horizontalSpeedCap5C;
    Fixed16 positiveYSpeedCap60;
    Fixed16 negativeYSpeed64;
    std::uint32_t field68;
    std::uint32_t field6C;
    std::uint16_t field70;
    std::uint16_t verticalStepPixels72;
    std::uint16_t field74;
    std::uint16_t field76;

    RecoveredPlayerState();

    static RecoveredPlayerState fromRaw(const PlayerRawRecord &record);
    void syncFromRaw();
    void syncToRaw();
    PlayerRawRecord toRaw() const;
};

// Canonical engine-facing name for the exact runtime record. The recovered
// projection remains a base so existing foundation clients can continue to
// use it while new code gets explicit record/trace terminology.
struct PlayerRecord : public RecoveredPlayerState {
    PlayerRecord();

    static PlayerRecord fromRaw(const PlayerRawRecord &record);
    static PlayerRecord fromBytes(const Bytes &source,
                                  std::size_t offset = 0);
    Bytes toBytes() const;

    std::uint16_t statusWord() const;
    void setStatusWord(std::uint16_t value);
    std::int16_t xPixel() const;
    std::int16_t yPixel() const;
    std::int16_t viewAnchorX() const;
    void setXPixel(std::int16_t value);
    void setYPixel(std::int16_t value);
    void initializeConfirmedHorizontalFields();
};

} // namespace quiky

#endif
