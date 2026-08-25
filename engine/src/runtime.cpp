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
    input.alternate = (flags & 0x10) != 0;
    return input;
}

PlayerConfig::PlayerConfig()
    : width(16),
      height(32),
      // Recovered callback steps: the normal horizontal ramp advances in
      // 0x0800 units, release/reversal uses the 0x2000 path, and the object
      // speed cap is +0x5c = 0x18000.
      acceleration(0x0800),
      maxHorizontalSpeed(0x18000),
      friction(0x2000),
      // The ascent path adds +0x2000 and clamps at -0x20000.
      gravity(0x2000),
      jumpVelocity(-0x20000) {
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

PlayerDescriptorTable::PlayerDescriptorTable() : _words() {
    _words.fill(0);
}

PlayerDescriptorTable::PlayerDescriptorTable(
    const std::array<std::uint16_t, PlayerDescriptorRules::kEntryCount> &words)
    : _words(words) {
}

std::uint16_t PlayerDescriptorTable::word(std::uint16_t tileId) const {
    return _words[tileId & 0x01ff];
}

void PlayerDescriptorTable::setWord(std::uint16_t tileId,
                                    std::uint16_t descriptor) {
    _words[tileId & 0x01ff] = descriptor;
}

MapDescriptorQuery::MapDescriptorQuery(
    const Map &map, const PlayerDescriptorTable &descriptors)
    : _map(map), _descriptors(descriptors) {
}

std::int32_t MapDescriptorQuery::floorTile(std::int32_t pixels) {
    if (pixels >= 0) {
        return pixels / 16;
    }
    return -static_cast<std::int32_t>(
        (-static_cast<std::int64_t>(pixels) + 15) / 16);
}

std::uint16_t MapDescriptorQuery::tileIdAt(std::int32_t tileX,
                                           std::int32_t tileY) const {
    if (tileX < 0 || tileY < 0 ||
        tileX >= static_cast<std::int32_t>(_map.width) ||
        tileY >= static_cast<std::int32_t>(_map.height)) {
        throw FormatError("MAP descriptor coordinate is outside the map");
    }
    return Map::tileId(_map.cell(static_cast<std::uint16_t>(tileX),
                                 static_cast<std::uint16_t>(tileY)));
}

std::uint16_t MapDescriptorQuery::descriptorAt(std::int32_t tileX,
                                               std::int32_t tileY) const {
    return _descriptors.word(tileIdAt(tileX, tileY));
}

std::uint16_t MapDescriptorQuery::descriptorAtPixel(std::int32_t x,
                                                    std::int32_t y) const {
    return descriptorAt(floorTile(x), floorTile(y));
}

bool MapDescriptorQuery::blocksProbeAt(std::int32_t x,
                                       std::int32_t y) const {
    return PlayerDescriptorRules::blocksProbe(descriptorAtPixel(x, y),
                                              static_cast<std::uint16_t>(x),
                                              static_cast<std::uint16_t>(y));
}

bool MapDescriptorQuery::hasVerticalResponseAt(std::int32_t x,
                                               std::int32_t y) const {
    return PlayerDescriptorRules::hasVerticalResponse(descriptorAtPixel(x, y));
}

bool MapDescriptorQuery::alignsEightPixelsAt(std::int32_t x,
                                              std::int32_t y) const {
    return PlayerDescriptorRules::alignsEightPixels(descriptorAtPixel(x, y));
}

MapDescriptorCollisionQuery::MapDescriptorCollisionQuery(
    const Map &map, const PlayerDescriptorTable &descriptors)
    : _query(map, descriptors) {
}

bool MapDescriptorCollisionQuery::blocksHorizontal(
    std::int32_t tileX, std::int32_t tileY) const {
    // The coarse adapter uses the tile centre. PlayerSimulation uses the
    // exact pixel probes when this query is available.
    return blocksProbeAt(tileX * 16 + 8, tileY * 16 + 8);
}

bool MapDescriptorCollisionQuery::blocksFloor(
    std::int32_t tileX, std::int32_t tileY) const {
    return hasVerticalResponseAt(tileX * 16 + 8, tileY * 16 + 8);
}

bool MapDescriptorCollisionQuery::blocksCeiling(
    std::int32_t tileX, std::int32_t tileY) const {
    return hasVerticalResponseAt(tileX * 16 + 8, tileY * 16 + 8);
}

bool MapDescriptorCollisionQuery::blocksProbeAt(std::int32_t x,
                                                std::int32_t y) const {
    return _query.blocksProbeAt(x, y);
}

bool MapDescriptorCollisionQuery::hasVerticalResponseAt(
    std::int32_t x, std::int32_t y) const {
    return _query.hasVerticalResponseAt(x, y);
}

bool MapDescriptorCollisionQuery::alignsEightPixelsAt(
    std::int32_t x, std::int32_t y) const {
    return _query.alignsEightPixelsAt(x, y);
}

PlayerState::PlayerState()
    : x(), y(), velocityX(), velocityY(), grounded(false), facingRight(true),
      callbackMode(0), callbackGate(0), transition(0), verticalResponse(0),
      sideResponse(1), deathTimer(0) {
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
    player.callbackMode = 0;
    player.callbackGate = 0;
    player.transition = 0;
    player.verticalResponse = 0;
    player.sideResponse = 1;
    player.deathTimer = 0;
}

bool PlayerSimulation::collidesHorizontal(const CollisionQuery &collision,
                                          const PlayerState &player) const {
    const std::int32_t left = player.x.floorPixels();
    const std::int32_t right = floorFixed(player.x.raw + _config.width * Fixed16::kOne - 1);
    const std::int32_t top = player.y.floorPixels();
    const std::int32_t bottom = floorFixed(player.y.raw + _config.height * Fixed16::kOne - 1);
    const std::int32_t edge = player.velocityX.raw > 0 ? right : left;
    const PlayerProbeQuery *probes =
        dynamic_cast<const PlayerProbeQuery *>(&collision);
    if (probes != 0) {
        const std::int32_t middle = top + _config.height / 2;
        return probes->blocksProbeAt(edge, top) ||
               probes->blocksProbeAt(edge, middle) ||
               probes->blocksProbeAt(edge, bottom);
    }
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

    const bool movingRight = player.velocityX.raw > 0;
    if (movingRight) {
        const std::int32_t right = floorFixed(player.x.raw + _config.width * Fixed16::kOne - 1);
        const std::int32_t tileX = floorTile(right);
        player.x.raw = (tileX * 16 - _config.width) * Fixed16::kOne;
    } else {
        const std::int32_t left = player.x.floorPixels();
        const std::int32_t tileX = floorTile(left);
        player.x.raw = (tileX + 1) * 16 * Fixed16::kOne;
    }
    const PlayerProbeQuery *probes =
        dynamic_cast<const PlayerProbeQuery *>(&collision);
    if (probes != 0) {
        const std::int32_t probeX = movingRight
                                         ? player.x.floorPixels() + _config.width
                                         : player.x.floorPixels();
        if (probes->alignsEightPixelsAt(probeX, player.y.floorPixels())) {
            const std::int32_t quantum = 8 * Fixed16::kOne;
            player.x.raw = (player.x.raw / quantum) * quantum;
        }
    }
    player.velocityX.raw = 0;
    player.sideResponse = 0;
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
        player.callbackMode = 1;
        player.verticalResponse = 1;
    } else if (player.velocityY.raw < 0) {
        if (!collidesCeiling(collision, player)) {
            return;
        }
        const std::int32_t top = player.y.floorPixels();
        const std::int32_t tileY = floorTile(top);
        player.y.raw = (tileY + 1) * 16 * Fixed16::kOne;
        player.velocityY.raw = 0;
        player.callbackMode = 1;
        player.verticalResponse = 1;
    }
}

void PlayerSimulation::tick(PlayerState &player, const CollisionQuery &collision,
                             const InputState &input) const {
    const bool launch = (input.jump || input.up) && player.grounded;

    if (input.left && input.right) {
        // The simultaneous-input capture follows the right branch.
        player.velocityX.raw = std::min(
            _config.maxHorizontalSpeed,
            player.velocityX.raw + _config.acceleration);
        player.facingRight = true;
    } else if (input.left) {
        if (player.velocityX.raw > 0) {
            // Reversal clears the old direction; the next frame starts the
            // negative acceleration ramp.
            player.velocityX.raw = 0;
        } else {
            player.velocityX.raw = clampVelocity(
                player.velocityX.raw - _config.acceleration,
                _config.maxHorizontalSpeed);
        }
        player.facingRight = false;
    } else if (input.right) {
        if (player.velocityX.raw < 0) {
            player.velocityX.raw = 0;
        } else {
            player.velocityX.raw = clampVelocity(
                player.velocityX.raw + _config.acceleration,
                _config.maxHorizontalSpeed);
        }
        player.facingRight = true;
    } else {
        if (player.velocityX.raw > 0) {
            player.velocityX.raw = std::max(0, player.velocityX.raw - _config.friction);
        } else if (player.velocityX.raw < 0) {
            player.velocityX.raw = std::min(0, player.velocityX.raw + _config.friction);
        }
    }

    if (launch) {
        player.velocityY.raw = _config.jumpVelocity;
        player.grounded = false;
        player.callbackMode = -1;
        player.verticalResponse = 0;
    } else if (player.velocityY.raw < 0) {
        player.velocityY.raw = std::min(0, player.velocityY.raw + _config.gravity);
    } else if (player.velocityY.raw > 0) {
        player.velocityY.raw = std::min(
            0x20000, player.velocityY.raw + _config.gravity);
    } else if (!player.grounded) {
        // A reset starts airborne in the compatibility model; the callback's
        // positive path supplies the first downward step before probing the
        // floor.
        player.velocityY.raw = std::min(
            0x20000, player.velocityY.raw + _config.gravity);
    }

    moveHorizontal(player, collision);
    moveVertical(player, collision);
}

void PlayerSimulation::tick(PlayerState &player, const Map &map,
                            const InputState &input) const {
    const MapCollisionQuery collision(map, _collision);
    tick(player, collision, input);
}

void PlayerSimulation::tick(PlayerState &player, const Map &map,
                            const PlayerDescriptorTable &descriptors,
                            const InputState &input) const {
    const MapDescriptorCollisionQuery collision(map, descriptors);
    tick(player, collision, input);
}

} // namespace quiky
