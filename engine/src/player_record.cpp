#include "quiky/player_record.h"

#include <sstream>

namespace quiky {
namespace {

std::int16_t signed16FromBits(std::uint16_t bits) {
    if (bits <= 0x7fff) {
        return static_cast<std::int16_t>(bits);
    }
    return static_cast<std::int16_t>(-32768 + (bits - 0x8000));
}

std::int32_t signed32FromBits(std::uint32_t bits) {
    if (bits <= 0x7fffffffU) {
        return static_cast<std::int32_t>(bits);
    }
    const std::uint32_t magnitude = bits - 0x80000000U;
    return static_cast<std::int32_t>(-2147483647 - 1) +
           static_cast<std::int32_t>(magnitude);
}

} // namespace

PlayerRawRecord::PlayerRawRecord() : bytes() {
    bytes.fill(0);
}

void PlayerRawRecord::checkRange(std::size_t offset, std::size_t width) {
    if (offset > kSize || width > kSize - offset) {
        std::ostringstream message;
        message << "player record access outside 0x" << std::hex << kSize
                << "-byte record at offset 0x" << offset;
        throw FormatError(message.str());
    }
}

PlayerRawRecord PlayerRawRecord::fromBytes(const Bytes &source,
                                           std::size_t offset) {
    if (offset > source.size() || kSize > source.size() - offset) {
        throw FormatError("player trace record is shorter than 0x78 bytes");
    }

    PlayerRawRecord record;
    for (std::size_t index = 0; index < kSize; ++index) {
        record.bytes[index] = source[offset + index];
    }
    return record;
}

Bytes PlayerRawRecord::toBytes() const {
    return Bytes(bytes.begin(), bytes.end());
}

std::uint8_t PlayerRawRecord::u8(std::size_t offset) const {
    checkRange(offset, 1);
    return bytes[offset];
}

std::int8_t PlayerRawRecord::s8(std::size_t offset) const {
    return static_cast<std::int8_t>(u8(offset));
}

std::uint16_t PlayerRawRecord::u16(std::size_t offset) const {
    checkRange(offset, 2);
    return static_cast<std::uint16_t>(bytes[offset]) |
           static_cast<std::uint16_t>(bytes[offset + 1] << 8);
}

std::int16_t PlayerRawRecord::s16(std::size_t offset) const {
    return signed16FromBits(u16(offset));
}

std::uint32_t PlayerRawRecord::u32(std::size_t offset) const {
    checkRange(offset, 4);
    return static_cast<std::uint32_t>(bytes[offset]) |
           (static_cast<std::uint32_t>(bytes[offset + 1]) << 8) |
           (static_cast<std::uint32_t>(bytes[offset + 2]) << 16) |
           (static_cast<std::uint32_t>(bytes[offset + 3]) << 24);
}

std::int32_t PlayerRawRecord::s32(std::size_t offset) const {
    return signed32FromBits(u32(offset));
}

void PlayerRawRecord::setU8(std::size_t offset, std::uint8_t value) {
    checkRange(offset, 1);
    bytes[offset] = value;
}

void PlayerRawRecord::setU16(std::size_t offset, std::uint16_t value) {
    checkRange(offset, 2);
    bytes[offset] = static_cast<byte>(value & 0xff);
    bytes[offset + 1] = static_cast<byte>((value >> 8) & 0xff);
}

void PlayerRawRecord::setU32(std::size_t offset, std::uint32_t value) {
    checkRange(offset, 4);
    bytes[offset] = static_cast<byte>(value & 0xff);
    bytes[offset + 1] = static_cast<byte>((value >> 8) & 0xff);
    bytes[offset + 2] = static_cast<byte>((value >> 16) & 0xff);
    bytes[offset + 3] = static_cast<byte>((value >> 24) & 0xff);
}

void PlayerRawRecord::setS32(std::size_t offset, std::int32_t value) {
    setU32(offset, static_cast<std::uint32_t>(value));
}

RecoveredPlayerState::RecoveredPlayerState()
    : raw(),
      actionWord(0),
      positionX(),
      positionY(),
      velocityX(),
      velocityY(),
      field12(0),
      field14(0),
      field16(0),
      callbackOffset18(0),
      field1A(0),
      field1C(0),
      field1E(0),
      field20(0),
      field22(0),
      field24(0),
      field26(0),
      inputByte28(0),
      animationByte29(0),
      field2A(0),
      field2C(0),
      field2E(0),
      field30(0),
      callbackState32(0),
      timer34(0),
      animationState36(0),
      mode37(0),
      gate38(0),
      transition39(0),
      verticalResponse3A(0),
      sideResponse3B(0),
      field3C(0),
      resetDeathTimer3E(0),
      callbackCounter40(0),
      field42(0),
      savedX44(),
      savedY48(),
      field4C(0),
      field50(0),
      field54(0),
      field58(0),
      field5C(0),
      field60(0),
      field64(0),
      field68(0),
      field6C(0),
      field70(0),
      field72(0),
      field74(0),
      field76(0) {
}

RecoveredPlayerState RecoveredPlayerState::fromRaw(
    const PlayerRawRecord &record) {
    RecoveredPlayerState state;
    state.raw = record;
    state.syncFromRaw();
    return state;
}

void RecoveredPlayerState::syncFromRaw() {
    actionWord = raw.u16(0x00);
    positionX = Fixed16::fromRaw(raw.s32(0x02));
    positionY = Fixed16::fromRaw(raw.s32(0x06));
    velocityX = Fixed16::fromRaw(raw.s32(0x0a));
    velocityY = Fixed16::fromRaw(raw.s32(0x0e));
    field12 = raw.u16(0x12);
    field14 = raw.u16(0x14);
    field16 = raw.u16(0x16);
    callbackOffset18 = raw.u16(0x18);
    field1A = raw.u16(0x1a);
    field1C = raw.u16(0x1c);
    field1E = raw.u16(0x1e);
    field20 = raw.u16(0x20);
    field22 = raw.u16(0x22);
    field24 = raw.u16(0x24);
    field26 = raw.u16(0x26);
    inputByte28 = raw.u8(0x28);
    animationByte29 = raw.u8(0x29);
    field2A = raw.u16(0x2a);
    field2C = raw.u16(0x2c);
    field2E = raw.u16(0x2e);
    field30 = raw.u16(0x30);
    callbackState32 = raw.u16(0x32);
    timer34 = raw.u16(0x34);
    animationState36 = raw.u8(0x36);
    mode37 = raw.s8(0x37);
    gate38 = raw.u8(0x38);
    transition39 = raw.u8(0x39);
    verticalResponse3A = raw.u8(0x3a);
    sideResponse3B = raw.u8(0x3b);
    field3C = raw.u16(0x3c);
    resetDeathTimer3E = raw.u16(0x3e);
    callbackCounter40 = raw.u16(0x40);
    field42 = raw.u16(0x42);
    savedX44 = Fixed16::fromRaw(raw.s32(0x44));
    savedY48 = Fixed16::fromRaw(raw.s32(0x48));
    field4C = raw.s32(0x4c);
    field50 = raw.s32(0x50);
    field54 = raw.s32(0x54);
    field58 = raw.s32(0x58);
    field5C = raw.s32(0x5c);
    field60 = raw.s32(0x60);
    field64 = raw.s32(0x64);
    field68 = raw.u32(0x68);
    field6C = raw.u32(0x6c);
    field70 = raw.u16(0x70);
    field72 = raw.s16(0x72);
    field74 = raw.u16(0x74);
    field76 = raw.u16(0x76);
}

void RecoveredPlayerState::syncToRaw() {
    raw.setU16(0x00, actionWord);
    raw.setS32(0x02, positionX.raw);
    raw.setS32(0x06, positionY.raw);
    raw.setS32(0x0a, velocityX.raw);
    raw.setS32(0x0e, velocityY.raw);
    raw.setU16(0x12, field12);
    raw.setU16(0x14, field14);
    raw.setU16(0x16, field16);
    raw.setU16(0x18, callbackOffset18);
    raw.setU16(0x1a, field1A);
    raw.setU16(0x1c, field1C);
    raw.setU16(0x1e, field1E);
    raw.setU16(0x20, field20);
    raw.setU16(0x22, field22);
    raw.setU16(0x24, field24);
    raw.setU16(0x26, field26);
    raw.setU8(0x28, inputByte28);
    raw.setU8(0x29, animationByte29);
    raw.setU16(0x2a, field2A);
    raw.setU16(0x2c, field2C);
    raw.setU16(0x2e, field2E);
    raw.setU16(0x30, field30);
    raw.setU16(0x32, callbackState32);
    raw.setU16(0x34, timer34);
    raw.setU8(0x36, animationState36);
    raw.setU8(0x37, static_cast<std::uint8_t>(mode37));
    raw.setU8(0x38, gate38);
    raw.setU8(0x39, transition39);
    raw.setU8(0x3a, verticalResponse3A);
    raw.setU8(0x3b, sideResponse3B);
    raw.setU16(0x3c, field3C);
    raw.setU16(0x3e, resetDeathTimer3E);
    raw.setU16(0x40, callbackCounter40);
    raw.setU16(0x42, field42);
    raw.setS32(0x44, savedX44.raw);
    raw.setS32(0x48, savedY48.raw);
    raw.setS32(0x4c, field4C);
    raw.setS32(0x50, field50);
    raw.setS32(0x54, field54);
    raw.setS32(0x58, field58);
    raw.setS32(0x5c, field5C);
    raw.setS32(0x60, field60);
    raw.setS32(0x64, field64);
    raw.setU32(0x68, field68);
    raw.setU32(0x6c, field6C);
    raw.setU16(0x70, field70);
    raw.setU16(0x72, static_cast<std::uint16_t>(field72));
    raw.setU16(0x74, field74);
    raw.setU16(0x76, field76);
}

PlayerRawRecord RecoveredPlayerState::toRaw() const {
    RecoveredPlayerState copy = *this;
    copy.syncToRaw();
    return copy.raw;
}

} // namespace quiky
