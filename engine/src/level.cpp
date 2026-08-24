#include "quiky/level.h"

#include <algorithm>
#include <cctype>
#include <cstdlib>

namespace quiky {

namespace {

std::string upperAscii(const std::string &value) {
    std::string result(value);
    for (std::size_t index = 0; index < result.size(); ++index) {
        result[index] = static_cast<char>(
            std::toupper(static_cast<unsigned char>(result[index])));
    }
    return result;
}

std::int32_t floorRegion(std::int32_t pixel) {
    if (pixel >= 0) {
        return pixel / static_cast<std::int32_t>(Area::kRegionSize);
    }
    return -(((-pixel) + static_cast<std::int32_t>(Area::kRegionSize) - 1) /
             static_cast<std::int32_t>(Area::kRegionSize));
}

std::int32_t floorTile(std::int32_t pixel) {
    if (pixel >= 0) {
        return pixel / 16;
    }
    return -static_cast<std::int32_t>((-static_cast<std::int64_t>(pixel) + 15) / 16);
}

std::string worldForMap(const std::string &mapName) {
    if (mapName.size() < 2 || (mapName[0] != 'W' && mapName[0] != 'w')) {
        return std::string();
    }
    std::string world(mapName.substr(0, 2));
    world[0] = 'W';
    world[1] = static_cast<char>(std::toupper(static_cast<unsigned char>(world[1])));
    return world;
}

class EntityCollisionQuery : public CollisionQuery {
public:
    EntityCollisionQuery(const CollisionQuery &base,
                         const std::vector<LevelEntity> &entities)
        : _base(base), _entities(entities) {}

    bool blocksHorizontal(std::int32_t tileX,
                          std::int32_t tileY) const override {
        return _base.blocksHorizontal(tileX, tileY);
    }

    bool blocksFloor(std::int32_t tileX,
                     std::int32_t tileY) const override {
        // The confirmed platform footprints are known, but their motion
        // tables are not. Treating them as one-way floor preserves the
        // useful static interaction without inventing a trajectory.
        if (_base.blocksFloor(tileX, tileY)) {
            return true;
        }
        for (std::size_t index = 0; index < _entities.size(); ++index) {
            const LevelEntity &entity = _entities[index];
            if (entity.kind != EntityKind::MovingPlatform ||
                entity.phase != EntityPhase::Active ||
                entity.collisionWidth == 0 || entity.collisionHeight == 0) {
                continue;
            }
            const std::int32_t platformTop = floorTile(entity.y);
            const std::int32_t platformLeft = floorTile(entity.x);
            const std::int32_t platformRight = floorTile(
                entity.x + static_cast<std::int32_t>(entity.collisionWidth) - 1);
            if (tileY == platformTop && tileX >= platformLeft &&
                tileX <= platformRight) {
                return true;
            }
        }
        return false;
    }

    bool blocksCeiling(std::int32_t tileX,
                       std::int32_t tileY) const override {
        return _base.blocksCeiling(tileX, tileY);
    }

private:
    const CollisionQuery &_base;
    const std::vector<LevelEntity> &_entities;
};

} // namespace

LevelSessionConfig::LevelSessionConfig()
    : streamRadiusRegions(1),
      collectibleRadius(12),
      hazardRadius(12),
      edgeExitMargin(24),
      enableEdgeExit(true),
      hasSpawn(false),
      spawnX(100),
      spawnY(100) {
}

LevelSession::LevelSession(const std::string &mapName, const Map &map,
                           const Area &area, const LevelSessionConfig &config)
    : _mapName(mapName),
      _map(map),
      _area(area),
      _config(config),
      _entities(),
      _event(),
      _score(0),
      _deaths(0) {
    const std::vector<AreaPlacement> placements = _area.placements();
    _entities.reserve(placements.size());
    for (std::size_t index = 0; index < placements.size(); ++index) {
        const AreaPlacement &placement = placements[index];
        LevelEntity entity;
        entity.id = static_cast<std::uint32_t>(index + 1);
        entity.recordOffset = placement.recordOffset;
        entity.type = placement.type;
        entity.regionX = placement.regionX;
        entity.regionY = placement.regionY;
        entity.x = static_cast<std::int32_t>(placement.worldX);
        entity.y = static_cast<std::int32_t>(placement.worldY);
        entity.kind = classify(entity.type);
        entity.spriteSlot = spriteSlotFor(entity.type);
        entity.spriteResource = spriteResourceFor(entity.type);
        entity.effectSlot = effectSlotFor(entity.type);
        entity.effectResource = effectResourceFor(entity.type);
        entity.collisionWidth = collisionWidthFor(entity.type);
        entity.collisionHeight = collisionHeightFor(entity.type);
        _entities.push_back(entity);
    }
}

SpawnPoint LevelSession::spawnPoint() const {
    if (_config.hasSpawn) {
        return SpawnPoint(_config.spawnX, _config.spawnY);
    }

    // No player declaration has been correlated in ARE yet. W1L1's 100,100
    // anchor is the existing runtime smoke-test point; other levels use the
    // same conservative fallback until a live player trace identifies their
    // true spawn records.
    const std::int32_t mapWidth = static_cast<std::int32_t>(_map.width) * 16;
    const std::int32_t mapHeight = static_cast<std::int32_t>(_map.height) * 16;
    return SpawnPoint(std::min<std::int32_t>(_config.spawnX, std::max(0, mapWidth - 32)),
                      std::min<std::int32_t>(_config.spawnY, std::max(0, mapHeight - 32)));
}

void LevelSession::reset(PlayerState &player, const PlayerSimulation &simulation) {
    _event = LevelEvent();
    _score = 0;
    _deaths = 0;
    for (std::size_t index = 0; index < _entities.size(); ++index) {
        LevelEntity &entity = _entities[index];
        entity.phase = EntityPhase::Dormant;
        entity.animationFrame = 0;
        entity.activeFrames = 0;
        entity.active = false;
        entity.collected = false;
    }
    resetPlayer(player, simulation);
}

void LevelSession::resetPlayer(PlayerState &player,
                                const PlayerSimulation &simulation) const {
    const SpawnPoint spawn = spawnPoint();
    simulation.reset(player, spawn.x, spawn.y);
}

EntityKind LevelSession::classify(std::uint16_t type) {
    // These ranges are the confirmed families in entity-types.json. Unknown
    // and effect/event types remain visible to the debug overlay but inert.
    if (type >= 0x01 && type <= 0x1c) {
        return EntityKind::Hazard;
    }
    if (type == 0x34) {
        return EntityKind::Hazard;
    }
    if ((type >= 0x6f && type <= 0x72) || (type >= 0x79 && type <= 0x7f)) {
        return EntityKind::Collectible;
    }
    if (type >= 0x3d && type <= 0x40) {
        return EntityKind::MovingPlatform;
    }
    return EntityKind::Unknown;
}

std::uint16_t LevelSession::spriteSlotFor(std::uint16_t type) {
    switch (type) {
    case 0x01: return 281;
    case 0x02: return 231;
    case 0x03: return 276;
    case 0x04: return 226;
    case 0x05: return 254;
    case 0x06: return 204;
    case 0x07: return 200;
    case 0x08: return 250;
    case 0x09: return 250;
    case 0x0a: return 200;
    case 0x0b: return 209;
    case 0x0c: return 209;
    case 0x15: return 208;
    case 0x16: return 208;
    case 0x17: return 250;
    case 0x18: return 200;
    case 0x19: return 250;
    case 0x1a: return 200;
    case 0x1b: return 264;
    case 0x1c: return 214;
    case 0x28: return 413;
    case 0x29:
    case 0x2a:
    case 0x2b: return 700;
    case 0x34: return 400;
    case 0x3d: return 301;
    case 0x3e: return 300;
    case 0x3f: return 301;
    case 0x40: return 300;
    default: break;
    }
    if (type >= 0x6f && type <= 0x72) {
        return static_cast<std::uint16_t>(607 + (type - 0x6f));
    }
    if (type >= 0x79 && type <= 0x7f) {
        return static_cast<std::uint16_t>(600 + (type - 0x79));
    }
    return 0xffff;
}

std::uint16_t LevelSession::effectSlotFor(std::uint16_t type) {
    // World-ICO effects select their tile from the active world's state
    // machine, so they deliberately do not have a fixed slot here.
    (void)type;
    return 0xffff;
}

std::uint16_t LevelSession::collisionWidthFor(std::uint16_t type) {
    if (type == 0x3d || type == 0x3f) {
        return 32;
    }
    if (type == 0x3e || type == 0x40) {
        return 48;
    }
    return 0;
}

std::uint16_t LevelSession::collisionHeightFor(std::uint16_t type) {
    if (type == 0x3d || type == 0x3f) {
        return 14;
    }
    if (type == 0x3e || type == 0x40) {
        return 16;
    }
    return 0;
}

std::string LevelSession::spriteResourceFor(std::uint16_t type) const {
    if (type >= 0x01 && type <= 0x02) {
        return "WURM2.BOB";
    }
    if (type >= 0x03 && type <= 0x04) {
        return "BIENE.BOB";
    }
    if (type >= 0x05 && type <= 0x06) {
        return "FISCH.BOB";
    }
    if (type >= 0x07 && type <= 0x08) {
        return "KRABBE.BOB";
    }
    if (type >= 0x09 && type <= 0x0a) {
        return "PENGO.BOB";
    }
    if (type >= 0x0b && type <= 0x0c) {
        return "SCHNEE.BOB";
    }
    if (type >= 0x15 && type <= 0x16) {
        return "FLIEGE.BOB";
    }
    if (type >= 0x17 && type <= 0x18) {
        return "SPINNE.BOB";
    }
    if (type >= 0x19 && type <= 0x1a) {
        return "BUGGY.BOB";
    }
    if (type >= 0x1b && type <= 0x1c) {
        return "UFO.BOB";
    }
    if (type == 0x28) {
        return "WOLKE.BOB";
    }
    if (type >= 0x29 && type <= 0x2b) {
        return "BLATT.BOB";
    }
    if (type == 0x34) {
        return "BUMP_" + worldForMap(_mapName) + ".BOB";
    }
    if (type == 0x3d) {
        return "PLATFW4.BOB";
    }
    if (type == 0x3e) {
        return "PLATFW3.BOB";
    }
    if (type == 0x3f) {
        return "PLATFW1.BOB";
    }
    if (type == 0x40) {
        return "PLATFW2.BOB";
    }
    if (type >= 0x6f && type <= 0x72) {
        return "WERBE.BOB";
    }
    if (type >= 0x79 && type <= 0x7f) {
        return "PUZZLE.BOB";
    }
    return std::string();
}

std::string LevelSession::effectResourceFor(std::uint16_t type) const {
    if (type >= 0x1f && type <= 0x21) {
        return "WORLD";
    }
    return std::string();
}

std::uint32_t LevelSession::collectibleValue(std::uint16_t type) {
    if (type == 0x6f) {
        return 10;
    }
    if (type == 0x70) {
        return 25;
    }
    if (type == 0x71) {
        return 50;
    }
    if (type == 0x72) {
        return 100;
    }
    return 1;
}

std::string LevelSession::nextLevelName(const std::string &mapName) {
    const std::string upper = upperAscii(mapName);
    const std::size_t levelMarker = upper.find('L', 2);
    if (levelMarker == std::string::npos || levelMarker + 1 >= upper.size()) {
        return std::string();
    }
    std::size_t end = levelMarker + 1;
    while (end < upper.size() && std::isdigit(static_cast<unsigned char>(upper[end]))) {
        ++end;
    }
    if (end == levelMarker + 1) {
        return std::string();
    }
    const int level = std::atoi(upper.substr(levelMarker + 1, end - levelMarker - 1).c_str());
    if (level >= 9) {
        return std::string();
    }
    return upper.substr(0, levelMarker + 1) + char('0' + level + 1) + upper.substr(end);
}

bool LevelSession::overlaps(const PlayerState &player, const PlayerConfig &playerConfig,
                            const LevelEntity &entity, std::int32_t radius) const {
    const std::int32_t left = player.x.floorPixels() - radius;
    const std::int32_t right = player.x.floorPixels() + playerConfig.width + radius;
    const std::int32_t top = player.y.floorPixels() - radius;
    const std::int32_t bottom = player.y.floorPixels() + playerConfig.height + radius;
    return entity.x >= left && entity.x <= right && entity.y >= top && entity.y <= bottom;
}

bool LevelSession::atRightExit(const PlayerState &player) const {
    if (!_config.enableEdgeExit) {
        return false;
    }
    const std::int32_t mapWidth = static_cast<std::int32_t>(_map.width) * 16;
    return player.x.floorPixels() + _config.edgeExitMargin >= mapWidth;
}

void LevelSession::updateStreaming(std::int32_t playerX, std::int32_t playerY) {
    const std::int32_t regionX = floorRegion(playerX);
    const std::int32_t regionY = floorRegion(playerY);
    for (std::size_t index = 0; index < _entities.size(); ++index) {
        LevelEntity &entity = _entities[index];
        if (entity.collected) {
            entity.phase = EntityPhase::Collected;
            entity.active = false;
            continue;
        }
        const std::int32_t distanceX = std::abs(static_cast<std::int32_t>(entity.regionX) - regionX);
        const std::int32_t distanceY = std::abs(static_cast<std::int32_t>(entity.regionY) - regionY);
        const bool visible = distanceX <= _config.streamRadiusRegions &&
                             distanceY <= _config.streamRadiusRegions;
        entity.phase = visible ? EntityPhase::Active : EntityPhase::Dormant;
        entity.active = visible;
    }
}

void LevelSession::advanceActiveEntities() {
    for (std::size_t index = 0; index < _entities.size(); ++index) {
        LevelEntity &entity = _entities[index];
        if (entity.phase != EntityPhase::Active) {
            continue;
        }
        ++entity.activeFrames;
        entity.animationFrame = static_cast<std::uint16_t>(
            (entity.animationFrame + 1) & 0x00ff);
    }
}

void LevelSession::tick(PlayerState &player, const PlayerSimulation &simulation,
                        const InputState &input) {
    const MapCollisionQuery collision(_map, simulation.collisionRules());
    tick(player, simulation, collision, input);
}

void LevelSession::tick(PlayerState &player, const PlayerSimulation &simulation,
                        const CollisionQuery &collision, const InputState &input) {
    _event = LevelEvent();
    updateStreaming(player.x.floorPixels(), player.y.floorPixels());
    const EntityCollisionQuery entityCollision(collision, _entities);
    simulation.tick(player, entityCollision, input);
    updateStreaming(player.x.floorPixels(), player.y.floorPixels());
    advanceActiveEntities();

    for (std::size_t index = 0; index < _entities.size(); ++index) {
        LevelEntity &entity = _entities[index];
        if (!entity.active || entity.collected) {
            continue;
        }
        if (entity.kind == EntityKind::Collectible &&
            overlaps(player, simulation.config(), entity, _config.collectibleRadius)) {
            entity.collected = true;
            entity.active = false;
            entity.phase = EntityPhase::Collected;
            _score += collectibleValue(entity.type);
            _event.type = LevelEventType::Collected;
            _event.entityId = entity.id;
            _event.entityType = entity.type;
        } else if (entity.kind == EntityKind::Hazard &&
                   overlaps(player, simulation.config(), entity, _config.hazardRadius)) {
            resetPlayer(player, simulation);
            ++_deaths;
            _event.type = LevelEventType::PlayerDied;
            _event.entityId = entity.id;
            _event.entityType = entity.type;
            updateStreaming(player.x.floorPixels(), player.y.floorPixels());
            return;
        }
    }

    if (atRightExit(player)) {
        _event.type = LevelEventType::LevelExit;
        _event.targetLevel = nextLevelName(_mapName);
    }
}

LevelEvent LevelSession::consumeEvent() {
    const LevelEvent event = _event;
    _event = LevelEvent();
    return event;
}

} // namespace quiky
