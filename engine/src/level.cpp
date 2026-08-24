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
    // These are the direct runtime-confirmed families. Other entity slots
    // remain unknown until their world-specific BOB resource is selected.
    if (type >= 0x6f && type <= 0x72) {
        return static_cast<std::uint16_t>(607 + (type - 0x6f));
    }
    if (type >= 0x79 && type <= 0x7f) {
        return static_cast<std::uint16_t>(600 + (type - 0x79));
    }
    return 0xffff;
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
    _event = LevelEvent();
    simulation.tick(player, _map, input);
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
