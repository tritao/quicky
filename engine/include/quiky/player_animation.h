#ifndef QUIKY_PLAYER_ANIMATION_H
#define QUIKY_PLAYER_ANIMATION_H

#include "quiky/player_record.h"

#include <cstdint>

namespace quiky {

// The executable stores a movement/action word separately from the animation
// table. Action 0 is idle and action 4 is also used by ordinary running; the
// native death transition retains that word while switching table/state bytes.
class PlayerAnimation {
public:
    PlayerAnimation();

    void reset();
    void setAction(std::uint16_t action);
    void setDeath(bool death);
    void advance(const PlayerRecord &player);

    std::uint16_t action() const { return _action; }
    bool death() const { return _death; }
    std::uint16_t slot() const { return _slot; }
    std::uint16_t delay() const { return _delay; }
    std::uint16_t cursor() const { return _cursor; }

private:
    enum class Table {
        Idle,
        Walk,
        Rising,
        Falling,
        Death,
    };

    static Table tableFor(const PlayerRecord &player, std::uint16_t action,
                          bool death);
    static std::uint16_t tableDelay(Table table);
    static std::uint16_t tableFrame(Table table, std::uint16_t cursor);
    static std::uint16_t tableLength(Table table);
    void install(Table table, bool facingRight);

    std::uint16_t _action;
    Table _table;
    std::uint16_t _cursor;
    std::uint16_t _delay;
    std::uint16_t _slot;
    bool _facingRight;
    bool _death;
};

} // namespace quiky

#endif
