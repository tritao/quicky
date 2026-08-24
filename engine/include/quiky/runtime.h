#ifndef QUIKY_RUNTIME_H
#define QUIKY_RUNTIME_H

#include "quiky/map.h"

#include <cstdint>

namespace quiky {

struct Fixed16 {
    static const std::int32_t kOne = 1 << 16;

    std::int32_t raw;

    Fixed16() : raw(0) {}
    explicit Fixed16(std::int32_t rawValue) : raw(rawValue) {}

    static Fixed16 fromPixels(std::int32_t pixels) {
        return Fixed16(pixels * kOne);
    }

    std::int32_t floorPixels() const;
};

struct InputState {
    bool left;
    bool right;
    bool jump;

    InputState() : left(false), right(false), jump(false) {}

    static InputState fromActionFlags(std::uint16_t flags);
};

struct PlayerConfig {
    std::int32_t width;
    std::int32_t height;
    std::int32_t acceleration;
    std::int32_t maxHorizontalSpeed;
    std::int32_t friction;
    std::int32_t gravity;
    std::int32_t jumpVelocity;

    PlayerConfig();
};

struct CollisionRules {
    // These masks are evidence-guided defaults, not a completed collision map.
    // MAP property B was tentatively associated with side/solid behavior and D
    // with head impacts in the existing research notes.
    std::uint16_t horizontalMask;
    std::uint16_t floorMask;
    std::uint16_t ceilingMask;
    bool outsideIsSolid;

    CollisionRules();
};

struct PlayerState {
    Fixed16 x;
    Fixed16 y;
    Fixed16 velocityX;
    Fixed16 velocityY;
    bool grounded;
    bool facingRight;

    PlayerState();
};

struct SpawnPoint {
    std::int32_t x;
    std::int32_t y;

    SpawnPoint(std::int32_t xValue = 0, std::int32_t yValue = 0)
        : x(xValue), y(yValue) {}
};

class PlayerSimulation {
public:
    PlayerSimulation(const PlayerConfig &config = PlayerConfig(),
                     const CollisionRules &collision = CollisionRules());

    void reset(PlayerState &player, std::int32_t x, std::int32_t y) const;
    void tick(PlayerState &player, const Map &map, const InputState &input) const;

    const PlayerConfig &config() const { return _config; }
    const CollisionRules &collisionRules() const { return _collision; }

private:
    bool propertyMatches(const Map &map, std::int32_t tileX, std::int32_t tileY,
                         std::uint16_t mask) const;
    bool collidesHorizontal(const Map &map, const PlayerState &player) const;
    bool collidesFloor(const Map &map, const PlayerState &player) const;
    bool collidesCeiling(const Map &map, const PlayerState &player) const;
    void moveHorizontal(PlayerState &player, const Map &map) const;
    void moveVertical(PlayerState &player, const Map &map) const;

    PlayerConfig _config;
    CollisionRules _collision;
};

} // namespace quiky

#endif
