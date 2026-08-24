#include "quiky/runtime.h"

#include <algorithm>

namespace quiky {

namespace {

std::int32_t floorFixed(std::int32_t raw) {
    if (raw >= 0) {
        return raw / Fixed16::kOne;
    }
    return -static_cast<std::int32_t>((-static_cast<std::int64_t>(raw) + Fixed16::kOne - 1) /
                                      Fixed16::kOne);
}

std::int32_t clampVelocity(std::int32_t value, std::int32_t limit) {
    return std::max(-limit, std::min(limit, value));
}

} // namespace

std::int32_t Fixed16::floorPixels() const {
    return floorFixed(raw);
}

InputState InputState::fromActionFlags(std::uint16_t flags) {
    InputState input;
    input.left = (flags & 0x08) != 0;
    input.right = (flags & 0x04) != 0;
    input.jump = (flags & 0x20) != 0;
    return input;
}

PlayerConfig::PlayerConfig()
    : width(16),
      height(32),
      acceleration(Fixed16::kOne / 2),
      maxHorizontalSpeed(3 * Fixed16::kOne),
      friction(Fixed16::kOne / 2),
      gravity(Fixed16::kOne / 2),
      jumpVelocity(-8 * Fixed16::kOne) {
}

CollisionRules::CollisionRules()
    : horizontalMask(0x20), floorMask(0x20), ceilingMask(0x08), outsideIsSolid(true) {
}

PlayerState::PlayerState()
    : x(), y(), velocityX(), velocityY(), grounded(false), facingRight(true) {
}

PlayerSimulation::PlayerSimulation(const PlayerConfig &config, const CollisionRules &collision)
    : _config(config), _collision(collision) {
}

void PlayerSimulation::reset(PlayerState &player, std::int32_t x, std::int32_t y) const {
    player.x = Fixed16::fromPixels(x);
    player.y = Fixed16::fromPixels(y);
    player.velocityX = Fixed16();
    player.velocityY = Fixed16();
    player.grounded = false;
    player.facingRight = true;
}

bool PlayerSimulation::propertyMatches(const Map &map, std::int32_t tileX,
                                       std::int32_t tileY, std::uint16_t mask) const {
    if (tileX < 0 || tileY < 0 || tileX >= map.width || tileY >= map.height) {
        return _collision.outsideIsSolid;
    }
    return (Map::properties(map.cell(static_cast<std::uint16_t>(tileX),
                                     static_cast<std::uint16_t>(tileY))) & mask) != 0;
}

bool PlayerSimulation::collidesHorizontal(const Map &map, const PlayerState &player) const {
    const std::int32_t left = player.x.floorPixels();
    const std::int32_t right = floorFixed(player.x.raw + _config.width * Fixed16::kOne - 1);
    const std::int32_t top = player.y.floorPixels();
    const std::int32_t bottom = floorFixed(player.y.raw + _config.height * Fixed16::kOne - 1);
    const std::int32_t edge = player.velocityX.raw > 0 ? right : left;
    const std::int32_t tileX = floorFixed(edge * Fixed16::kOne) / 16;
    const std::int32_t firstTileY = top / 16;
    const std::int32_t lastTileY = bottom / 16;
    for (std::int32_t tileY = firstTileY; tileY <= lastTileY; ++tileY) {
        if (propertyMatches(map, tileX, tileY, _collision.horizontalMask)) {
            return true;
        }
    }
    return false;
}

bool PlayerSimulation::collidesFloor(const Map &map, const PlayerState &player) const {
    const std::int32_t left = player.x.floorPixels();
    const std::int32_t right = floorFixed(player.x.raw + _config.width * Fixed16::kOne - 1);
    const std::int32_t bottom = floorFixed(player.y.raw + _config.height * Fixed16::kOne - 1);
    const std::int32_t tileY = bottom / 16;
    const std::int32_t firstTileX = left / 16;
    const std::int32_t lastTileX = right / 16;
    for (std::int32_t tileX = firstTileX; tileX <= lastTileX; ++tileX) {
        if (propertyMatches(map, tileX, tileY, _collision.floorMask)) {
            return true;
        }
    }
    return false;
}

bool PlayerSimulation::collidesCeiling(const Map &map, const PlayerState &player) const {
    const std::int32_t left = player.x.floorPixels();
    const std::int32_t right = floorFixed(player.x.raw + _config.width * Fixed16::kOne - 1);
    const std::int32_t top = player.y.floorPixels();
    const std::int32_t tileY = top / 16;
    const std::int32_t firstTileX = left / 16;
    const std::int32_t lastTileX = right / 16;
    for (std::int32_t tileX = firstTileX; tileX <= lastTileX; ++tileX) {
        if (propertyMatches(map, tileX, tileY, _collision.ceilingMask)) {
            return true;
        }
    }
    return false;
}

void PlayerSimulation::moveHorizontal(PlayerState &player, const Map &map) const {
    if (player.velocityX.raw == 0) {
        return;
    }
    player.x.raw += player.velocityX.raw;
    if (!collidesHorizontal(map, player)) {
        return;
    }

    if (player.velocityX.raw > 0) {
        const std::int32_t right = floorFixed(player.x.raw + _config.width * Fixed16::kOne - 1);
        const std::int32_t tileX = right / 16;
        player.x.raw = (tileX * 16 - _config.width) * Fixed16::kOne;
    } else {
        const std::int32_t left = player.x.floorPixels();
        const std::int32_t tileX = left / 16;
        player.x.raw = (tileX + 1) * 16 * Fixed16::kOne;
    }
    player.velocityX.raw = 0;
}

void PlayerSimulation::moveVertical(PlayerState &player, const Map &map) const {
    player.y.raw += player.velocityY.raw;
    if (player.velocityY.raw > 0) {
        player.grounded = false;
        if (!collidesFloor(map, player)) {
            return;
        }
        const std::int32_t bottom = floorFixed(player.y.raw + _config.height * Fixed16::kOne - 1);
        const std::int32_t tileY = bottom / 16;
        player.y.raw = (tileY * 16 - _config.height) * Fixed16::kOne;
        player.velocityY.raw = 0;
        player.grounded = true;
    } else if (player.velocityY.raw < 0) {
        if (!collidesCeiling(map, player)) {
            return;
        }
        const std::int32_t top = player.y.floorPixels();
        const std::int32_t tileY = top / 16;
        player.y.raw = (tileY + 1) * 16 * Fixed16::kOne;
        player.velocityY.raw = 0;
    }
}

void PlayerSimulation::tick(PlayerState &player, const Map &map,
                             const InputState &input) const {
    if (input.left == input.right) {
        if (player.velocityX.raw > 0) {
            player.velocityX.raw = std::max(0, player.velocityX.raw - _config.friction);
        } else if (player.velocityX.raw < 0) {
            player.velocityX.raw = std::min(0, player.velocityX.raw + _config.friction);
        }
    } else if (input.left) {
        player.velocityX.raw = clampVelocity(
            player.velocityX.raw - _config.acceleration, _config.maxHorizontalSpeed);
        player.facingRight = false;
    } else {
        player.velocityX.raw = clampVelocity(
            player.velocityX.raw + _config.acceleration, _config.maxHorizontalSpeed);
        player.facingRight = true;
    }

    if (input.jump && player.grounded) {
        player.velocityY.raw = _config.jumpVelocity;
        player.grounded = false;
    }

    moveHorizontal(player, map);
    if (!player.grounded || player.velocityY.raw != 0) {
        player.velocityY.raw += _config.gravity;
    }
    moveVertical(player, map);
}

} // namespace quiky
