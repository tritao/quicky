#include "quiky/player_animation.h"

namespace quiky {

namespace {

const std::uint16_t kIdle[] = {
    0, 16, 17, 18, 18, 19, 19, 19, 18, 17, 16,
};
const std::uint16_t kWalk[] = {0, 1, 2, 3, 4, 5, 6, 7};
const std::uint16_t kRising[] = {10, 11, 12};
const std::uint16_t kFalling[] = {13, 14, 15};
// The native table contains 20..28 followed by a -3 marker. The animation
// updater jumps back three words, yielding the terminal 26,27,28 loop.
const std::uint16_t kDeath[] = {20, 21, 22, 23, 24, 25, 26, 27, 28};

} // namespace

PlayerAnimation::PlayerAnimation()
    : _action(0), _table(Table::Idle), _cursor(0), _delay(14), _slot(0),
      _facingRight(true), _death(false) {
}

void PlayerAnimation::reset() {
    _action = 0;
    _table = Table::Idle;
    _cursor = 0;
    _delay = tableDelay(_table);
    _slot = tableFrame(_table, _cursor);
    _facingRight = true;
    _death = false;
}

void PlayerAnimation::setAction(std::uint16_t action) {
    if (_action == action) {
        return;
    }
    _action = action;
    // Force table selection on the next update. The action word alone does
    // not identify death: native running also uses action 4.
    _cursor = 0;
    _delay = 0;
}

void PlayerAnimation::setDeath(bool death) {
    if (_death == death) {
        return;
    }
    _death = death;
    _cursor = 0;
    _delay = 0;
}

PlayerAnimation::Table PlayerAnimation::tableFor(const PlayerState &player,
                                                  std::uint16_t action,
                                                  bool death) {
    if (death) {
        return Table::Death;
    }
    (void)action;
    const std::int32_t speed = player.velocityX.raw < 0
                                   ? -player.velocityX.raw
                                   : player.velocityX.raw;
    if (!player.grounded) {
        return player.velocityY.raw < 0 ? Table::Rising : Table::Falling;
    }
    return speed > Fixed16::kOne / 4 ? Table::Walk : Table::Idle;
}

std::uint16_t PlayerAnimation::tableDelay(Table table) {
    switch (table) {
    case Table::Idle: return 14;
    case Table::Walk: return 4;
    case Table::Rising: return 8;
    case Table::Falling: return 20;
    case Table::Death: return 14;
    }
    return 14;
}

std::uint16_t PlayerAnimation::tableLength(Table table) {
    switch (table) {
    case Table::Idle: return static_cast<std::uint16_t>(sizeof(kIdle) / sizeof(kIdle[0]));
    case Table::Walk: return static_cast<std::uint16_t>(sizeof(kWalk) / sizeof(kWalk[0]));
    case Table::Rising: return static_cast<std::uint16_t>(sizeof(kRising) / sizeof(kRising[0]));
    case Table::Falling: return static_cast<std::uint16_t>(sizeof(kFalling) / sizeof(kFalling[0]));
    case Table::Death: return static_cast<std::uint16_t>(sizeof(kDeath) / sizeof(kDeath[0]));
    }
    return 1;
}

std::uint16_t PlayerAnimation::tableFrame(Table table, std::uint16_t cursor) {
    switch (table) {
    case Table::Idle: return kIdle[cursor % (sizeof(kIdle) / sizeof(kIdle[0]))];
    case Table::Walk: return kWalk[cursor % (sizeof(kWalk) / sizeof(kWalk[0]))];
    case Table::Rising: return kRising[cursor % (sizeof(kRising) / sizeof(kRising[0]))];
    case Table::Falling: return kFalling[cursor % (sizeof(kFalling) / sizeof(kFalling[0]))];
    case Table::Death: return kDeath[cursor % (sizeof(kDeath) / sizeof(kDeath[0]))];
    }
    return 0;
}

void PlayerAnimation::install(Table table, bool facingRight) {
    _table = table;
    _cursor = 0;
    _delay = tableDelay(table);
    _facingRight = facingRight;
    const std::uint16_t mirror = facingRight ? 0 : 50;
    _slot = static_cast<std::uint16_t>(tableFrame(table, _cursor) + mirror);
}

void PlayerAnimation::advance(const PlayerState &player) {
    const Table desired = tableFor(player, _action, _death);
    if (desired != _table) {
        install(desired, player.facingRight);
        return;
    }
    if (player.facingRight != _facingRight) {
        // Direction is represented by the paired records, not a second
        // cursor. Keep the native cursor and delay while selecting its
        // mirrored slot family.
        _facingRight = player.facingRight;
        const std::uint16_t mirror = _facingRight ? 0 : 50;
        _slot = static_cast<std::uint16_t>(tableFrame(_table, _cursor) + mirror);
        return;
    }
    if (_delay == 0) {
        if (_table == Table::Death && _cursor >= 8) {
            // The native negative marker is -3, so 5D60 jumps back three
            // words from the marker to frame 26 rather than restarting at
            // frame 20. This leaves the terminal 26,27,28 loop in place.
            _cursor = 6;
        } else {
            _cursor = static_cast<std::uint16_t>(
                (_cursor + 1) % tableLength(_table));
        }
        _delay = tableDelay(_table);
        const std::uint16_t mirror = _facingRight ? 0 : 50;
        _slot = static_cast<std::uint16_t>(tableFrame(_table, _cursor) + mirror);
        return;
    }

    // 5D60 decrements the reload word first and advances on the following
    // callback when it observes zero.
    --_delay;
}

} // namespace quiky
