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

std::int32_t floorTile(std::int32_t pixels) {
    if (pixels >= 0) {
        return pixels / 16;
    }
    return -static_cast<std::int32_t>((-static_cast<std::int64_t>(pixels) + 15) / 16);
}

} // namespace

std::int32_t Fixed16::floorPixels() const {
    return floorFixed(raw);
}

InputState InputState::fromActionFlags(std::uint16_t flags) {
    InputState input;
    input.left = (flags & 0x08) != 0;
    input.right = (flags & 0x04) != 0;
    input.up = (flags & 0x02) != 0;
    input.down = (flags & 0x01) != 0;
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

std::uint16_t PlayerDescriptorRules::quadrantMask(std::uint16_t x,
                                                  std::uint16_t y) {
    const bool xBit3 = (x & 0x08) != 0;
    const bool yBit3 = (y & 0x08) != 0;
    if (yBit3) {
        return xBit3 ? 0x02 : 0x01;
    }
    return xBit3 ? 0x04 : 0x08;
}

bool PlayerDescriptorRules::blocksProbe(std::uint16_t descriptor,
                                        std::uint16_t x,
                                        std::uint16_t y) {
    return (descriptor & quadrantMask(x, y)) != 0;
}

bool PlayerDescriptorRules::hasVerticalResponse(std::uint16_t descriptor) {
    return (descriptor & 0x20) != 0;
}

bool PlayerDescriptorRules::alignsEightPixels(std::uint16_t descriptor) {
    return (descriptor & 0x40) != 0;
}

std::uint16_t PlayerDescriptorRules::snapProbeY(std::uint16_t y) {
    return static_cast<std::uint16_t>(y & 0xfff8);
}

PlayerState::PlayerState()
    : x(), y(), velocityX(), velocityY(), grounded(false), facingRight(true) {
}

MapCollisionQuery::MapCollisionQuery(const Map &map, const CollisionRules &rules)
    : _map(map), _rules(rules) {
}

bool MapCollisionQuery::propertyMatches(std::int32_t tileX, std::int32_t tileY,
                                        std::uint16_t mask) const {
    if (tileX < 0 || tileY < 0 || tileX >= _map.width || tileY >= _map.height) {
        return _rules.outsideIsSolid;
    }
    return (Map::properties(_map.cell(static_cast<std::uint16_t>(tileX),
                                      static_cast<std::uint16_t>(tileY))) & mask) != 0;
}

bool MapCollisionQuery::blocksHorizontal(std::int32_t tileX,
                                         std::int32_t tileY) const {
    return propertyMatches(tileX, tileY, _rules.horizontalMask);
}

bool MapCollisionQuery::blocksFloor(std::int32_t tileX,
                                    std::int32_t tileY) const {
    return propertyMatches(tileX, tileY, _rules.floorMask);
}

bool MapCollisionQuery::blocksCeiling(std::int32_t tileX,
                                      std::int32_t tileY) const {
    return propertyMatches(tileX, tileY, _rules.ceilingMask);
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

bool PlayerSimulation::collidesHorizontal(const CollisionQuery &collision,
                                          const PlayerState &player) const {
    const std::int32_t left = player.x.floorPixels();
    const std::int32_t right = floorFixed(player.x.raw + _config.width * Fixed16::kOne - 1);
    const std::int32_t top = player.y.floorPixels();
    const std::int32_t bottom = floorFixed(player.y.raw + _config.height * Fixed16::kOne - 1);
    const std::int32_t edge = player.velocityX.raw > 0 ? right : left;
    const std::int32_t tileX = floorTile(edge);
    const std::int32_t firstTileY = floorTile(top);
    const std::int32_t lastTileY = floorTile(bottom);
    for (std::int32_t tileY = firstTileY; tileY <= lastTileY; ++tileY) {
        if (collision.blocksHorizontal(tileX, tileY)) {
            return true;
        }
    }
    return false;
}

bool PlayerSimulation::collidesFloor(const CollisionQuery &collision,
                                     const PlayerState &player) const {
    const std::int32_t left = player.x.floorPixels();
    const std::int32_t right = floorFixed(player.x.raw + _config.width * Fixed16::kOne - 1);
    const std::int32_t bottom = floorFixed(player.y.raw + _config.height * Fixed16::kOne - 1);
    const std::int32_t tileY = floorTile(bottom);
    const std::int32_t firstTileX = floorTile(left);
    const std::int32_t lastTileX = floorTile(right);
    for (std::int32_t tileX = firstTileX; tileX <= lastTileX; ++tileX) {
        if (collision.blocksFloor(tileX, tileY)) {
            return true;
        }
    }
    return false;
}

bool PlayerSimulation::collidesCeiling(const CollisionQuery &collision,
                                       const PlayerState &player) const {
    const std::int32_t left = player.x.floorPixels();
    const std::int32_t right = floorFixed(player.x.raw + _config.width * Fixed16::kOne - 1);
    const std::int32_t top = player.y.floorPixels();
    const std::int32_t tileY = floorTile(top);
    const std::int32_t firstTileX = floorTile(left);
    const std::int32_t lastTileX = floorTile(right);
    for (std::int32_t tileX = firstTileX; tileX <= lastTileX; ++tileX) {
        if (collision.blocksCeiling(tileX, tileY)) {
            return true;
        }
    }
    return false;
}

void PlayerSimulation::moveHorizontal(PlayerState &player,
                                      const CollisionQuery &collision) const {
    if (player.velocityX.raw == 0) {
        return;
    }
    player.x.raw += player.velocityX.raw;
    if (!collidesHorizontal(collision, player)) {
        return;
    }

    if (player.velocityX.raw > 0) {
        const std::int32_t right = floorFixed(player.x.raw + _config.width * Fixed16::kOne - 1);
        const std::int32_t tileX = floorTile(right);
        player.x.raw = (tileX * 16 - _config.width) * Fixed16::kOne;
    } else {
        const std::int32_t left = player.x.floorPixels();
        const std::int32_t tileX = floorTile(left);
        player.x.raw = (tileX + 1) * 16 * Fixed16::kOne;
    }
    player.velocityX.raw = 0;
}

void PlayerSimulation::moveVertical(PlayerState &player,
                                    const CollisionQuery &collision) const {
    player.y.raw += player.velocityY.raw;
    if (player.velocityY.raw > 0) {
        player.grounded = false;
        if (!collidesFloor(collision, player)) {
            return;
        }
        const std::int32_t bottom = floorFixed(player.y.raw + _config.height * Fixed16::kOne - 1);
        const std::int32_t tileY = floorTile(bottom);
        player.y.raw = (tileY * 16 - _config.height) * Fixed16::kOne;
        player.velocityY.raw = 0;
        player.grounded = true;
    } else if (player.velocityY.raw < 0) {
        if (!collidesCeiling(collision, player)) {
            return;
        }
        const std::int32_t top = player.y.floorPixels();
        const std::int32_t tileY = floorTile(top);
        player.y.raw = (tileY + 1) * 16 * Fixed16::kOne;
        player.velocityY.raw = 0;
    }
}

void PlayerSimulation::tick(PlayerState &player, const CollisionQuery &collision,
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

    if ((input.jump || input.up) && player.grounded) {
        player.velocityY.raw = _config.jumpVelocity;
        player.grounded = false;
    }

    moveHorizontal(player, collision);
    if (!player.grounded || player.velocityY.raw != 0) {
        player.velocityY.raw += _config.gravity;
    }
    moveVertical(player, collision);
}

void PlayerSimulation::tick(PlayerState &player, const Map &map,
                             const InputState &input) const {
    const MapCollisionQuery collision(map, _collision);
    tick(player, collision, input);
}

} // namespace quiky
