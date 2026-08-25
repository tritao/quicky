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

std::string worldForMap(const std::string &mapName) {
    if (mapName.size() < 2 || (mapName[0] != 'W' && mapName[0] != 'w')) {
        return std::string();
    }
    std::string world(mapName.substr(0, 2));
    world[0] = 'W';
    world[1] = static_cast<char>(std::toupper(static_cast<unsigned char>(world[1])));
    return world;
}

std::uint16_t worldEffectSlotFor(const std::string &world,
                                 std::uint16_t tile) {
    if (world == "W1") {
        if (tile >= 200 && tile <= 204) return static_cast<std::uint16_t>(120 + tile - 200);
        if (tile >= 220 && tile <= 224) return static_cast<std::uint16_t>(140 + tile - 220);
        if (tile == 232) return 230;
        if (tile == 233) return 231;
        if (tile == 235) return 236;
        if (tile >= 240 && tile <= 244) return static_cast<std::uint16_t>(160 + tile - 240);
        if (tile == 252) return 250;
        if (tile == 253) return 251;
        if (tile == 255) return 256;
        if (tile >= 260 && tile <= 264) return static_cast<std::uint16_t>(180 + tile - 260);
    } else if (world == "W2") {
        if (tile >= 120 && tile <= 124) return static_cast<std::uint16_t>(126 + tile - 120);
        if (tile >= 140 && tile <= 144) return static_cast<std::uint16_t>(146 + tile - 140);
        if (tile >= 160 && tile <= 164) return static_cast<std::uint16_t>(166 + tile - 160);
        if (tile >= 180 && tile <= 184) return static_cast<std::uint16_t>(186 + tile - 180);
        if (tile >= 376 && tile <= 377) return static_cast<std::uint16_t>(378 + tile - 376);
        if (tile >= 396 && tile <= 397) return static_cast<std::uint16_t>(398 + tile - 396);
    } else if (world == "W3") {
        if (tile >= 405 && tile <= 409) return static_cast<std::uint16_t>(400 + tile - 405);
        if (tile >= 425 && tile <= 429) return static_cast<std::uint16_t>(420 + tile - 425);
        if (tile >= 445 && tile <= 449) return static_cast<std::uint16_t>(440 + tile - 445);
        if (tile >= 465 && tile <= 469) return static_cast<std::uint16_t>(460 + tile - 465);
    } else if (world == "W4") {
        if (tile >= 160 && tile <= 164) return static_cast<std::uint16_t>(240 + tile - 160);
        if (tile >= 180 && tile <= 184) return static_cast<std::uint16_t>(260 + tile - 180);
        if (tile >= 200 && tile <= 204) return static_cast<std::uint16_t>(280 + tile - 200);
        if (tile >= 220 && tile <= 224) return static_cast<std::uint16_t>(300 + tile - 220);
    } else if (world == "W5") {
        if (tile >= 141 && tile <= 144) return static_cast<std::uint16_t>(61 + tile - 141);
        if (tile == 149) return 60;
        if (tile >= 160 && tile <= 164) return static_cast<std::uint16_t>(80 + tile - 160);
        if (tile >= 180 && tile <= 184) return static_cast<std::uint16_t>(100 + tile - 180);
        if (tile >= 200 && tile <= 204) return static_cast<std::uint16_t>(120 + tile - 200);
    }
    return 0;
}

void appendStateWrite(std::vector<LevelStateWrite> &writes,
                      std::uint16_t address, std::uint8_t width,
                      std::uint32_t before, std::uint32_t after) {
    if (before != after) {
        writes.push_back(LevelStateWrite(address, width, before, after));
    }
}

} // namespace

LevelGameplayState::LevelGameplayState()
    : ammo880c(0),
      lives880a(4),
      score881c(0),
      currentHealth8822(5),
      maximumHealth8824(5),
      invulnerabilityGate8810(0),
      pendingEvent612e(0),
      playerTimer0034(0),
      puzzleMask60d8(0) {
}

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
      _effects(),
      _events(),
      _score(0),
      _deaths(0),
      _gameplayState(),
      _alternateActionActive(false) {
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
        entity.updateCallback = callbackFor(entity.type);
        entity.contactSubtype = collectibleSubtypeFor(entity.type);
        entity.collectionBit = collectionBitFor(entity.type);
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

void LevelSession::reset(Simulation &simulation) {
    _events.clear();
    _score = 0;
    _deaths = 0;
    _gameplayState = LevelGameplayState();
    _alternateActionActive = false;
    for (std::size_t index = 0; index < _entities.size(); ++index) {
        LevelEntity &entity = _entities[index];
        entity.phase = EntityPhase::Dormant;
        entity.animationFrame = 0;
        entity.activeFrames = 0;
        entity.active = false;
        entity.collected = false;
        entity.pooledInteractionTriggered = false;
        entity.schedulerHandle = SchedulerHandle();
        entity.updateCallback = callbackFor(entity.type);
        entity.contactSubtype = collectibleSubtypeFor(entity.type);
        entity.collectionBit = collectionBitFor(entity.type);
    }
    _effects.clear();
    resetPlayer(simulation);
}

void LevelSession::resetPlayer(Simulation &simulation) {
    const SpawnPoint spawn = spawnPoint();
    simulation.reset();
    PlayerRecord &player = simulation.stateForSetup().player;
    player.initializeConfirmedHorizontalFields();
    player.positionX = Fixed16::fromPixels(spawn.x);
    player.positionY = Fixed16::fromPixels(spawn.y);
    player.velocityX = Fixed16();
    player.velocityY = Fixed16();
    player.mode37 = 0;
    player.gate38 = 0;
    player.transition39 = 0;
    player.verticalResponse3A = 0;
    player.sideResponse3B = 1;
    player.syncToRaw();

    // Simulation::reset clears the object pool as part of the player respawn.
    // Invalidate the corresponding ARE-side handles so a later streaming pass
    // can publish each visible callback exactly once.
    for (std::size_t index = 0; index < _entities.size(); ++index) {
        LevelEntity &entity = _entities[index];
        entity.schedulerHandle = SchedulerHandle();
        if (!entity.collected) {
            entity.phase = EntityPhase::Dormant;
            entity.active = false;
            entity.pooledInteractionTriggered = false;
        }
    }
}

EntityKind LevelSession::classify(std::uint16_t type) {
    // These ranges are the confirmed gameplay families in entity-types.json.
    // Unknown and effect/event types are handled by the visual layer instead
    // of participating in entity collision or collection.
    if (type >= 0x01 && type <= 0x1c) {
        return EntityKind::Hazard;
    }
    if (type == 0x34) {
        return EntityKind::Hazard;
    }
    if (type == 0x2c || (type >= 0x6f && type <= 0x72) ||
        (type >= 0x79 && type <= 0x7f)) {
        return EntityKind::Collectible;
    }
    if (type >= 0x3d && type <= 0x40) {
        return EntityKind::MovingPlatform;
    }
    return EntityKind::Unknown;
}

CallbackIdentity LevelSession::callbackFor(std::uint16_t type) {
    // These identities are the recovered ARE callback targets. Only the
    // collectible family is executed by this slice; the other identities are
    // published for scheduler order and remain external contracts.
    if (type == 0x6f || type == 0x70 || type == 0x71 || type == 0x72 ||
        type == 0x2c || (type >= 0x79 && type <= 0x7f)) {
        return CallbackIdentity(0x01f7, 0x8d20, "collectible_8d20");
    }
    if (type >= 0x3d && type <= 0x40) {
        return CallbackIdentity(0x01f7, 0x9dc7, "moving_platform_9dc7");
    }
    if (type == 0x34) {
        return CallbackIdentity(0x01f7, 0x9c0c, "bump_9c0c");
    }
    if (type == 0x01) {
        return CallbackIdentity(0x01f7, 0x6dc4, "entity_wurm2_6dc4");
    }
    if (type == 0x03 || type == 0x04) {
        return CallbackIdentity(0x01f7, 0x68c0, "entity_biene_68c0");
    }
    if (type >= 0x1f && type <= 0x21) {
        return CallbackIdentity(0x01f7, 0x8e4b, "world_effect_8e4b");
    }
    if (type == 0x28) {
        return CallbackIdentity(0x01f7, 0x9269, "cloud_9269");
    }
    if (type >= 0x29 && type <= 0x2b) {
        return CallbackIdentity(0x01f7, 0x47e7, "falling_leaf_47e7");
    }
    return CallbackIdentity();
}

std::uint8_t LevelSession::collectibleSubtypeFor(std::uint16_t type) {
    switch (type) {
    case 0x6f: return 1;
    case 0x70: return 2;
    case 0x71: return 3;
    case 0x72: return 4;
    case 0x2c: return 5;
    default: return 0;
    }
}

std::uint8_t LevelSession::collectionBitFor(std::uint16_t type) {
    if (type >= 0x79 && type <= 0x7f) {
        return static_cast<std::uint8_t>(1u << (type - 0x79));
    }
    return 0;
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

std::uint16_t LevelSession::effectSlotFor(std::uint16_t type) const {
    // Dedicated LOOP effects use a world-relative representative slot. The
    // shared WORLD state machine resolves its slot from MAP data instead.
    if (type == 0x65) {
        return worldForMap(_mapName) == "W5" ? 4 : 1;
    }
    if (type == 0x66) {
        return worldForMap(_mapName) == "W2" ? 8 : 6;
    }
    if (type == 0x67) {
        return worldForMap(_mapName) == "W4" ? 22 : 6;
    }
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
    if (type >= 0x65 && type <= 0x67) {
        return "LOOP_" + worldForMap(_mapName) + ".ICO";
    }
    return std::string();
}

std::string LevelSession::highEffectSpriteResource() const {
    // The callback contract is shared by all worlds, but slot 611 resolves to
    // the world-2 dimensions only in PUFFW2.BOB. Other worlds use PUFF.BOB.
    return worldForMap(_mapName) == "W2" ? "PUFFW2.BOB" : "PUFF.BOB";
}

bool LevelSession::isTransientEffectType(std::uint16_t type) {
    return type >= 0x65 && type <= 0x67;
}

bool LevelSession::isWorldEffectType(std::uint16_t type) {
    return type >= 0x1f && type <= 0x21;
}

bool LevelSession::isPooledInteractionType(std::uint16_t type) {
    return type >= 0x05 && type <= 0x08;
}

std::uint32_t LevelSession::collectibleValue(std::uint16_t type) {
    switch (type) {
    case 0x2c: return 500;
    case 0x6f: return 50;
    case 0x70: return 250;
    case 0x71: return 100;
    case 0x72: return 150;
    default: break;
    }
    if (type >= 0x79 && type <= 0x7f) {
        return 100;
    }
    return 0;
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

bool LevelSession::overlaps(const PlayerRecord &player,
                            const LevelEntity &entity,
                            std::int32_t radius) const {
    const std::int32_t left = player.positionX.floorPixels() - radius;
    const std::int32_t right = player.positionX.floorPixels() + 16 + radius;
    const std::int32_t top = player.positionY.floorPixels() - radius;
    const std::int32_t bottom = player.positionY.floorPixels() + 32 + radius;
    return entity.x >= left && entity.x <= right && entity.y >= top && entity.y <= bottom;
}

bool LevelSession::pooledInteractionOverlaps(const PlayerRecord &player,
                                             const LevelEntity &entity) const {
    const std::int32_t playerX = player.positionX.floorPixels();
    const std::int32_t playerY = player.positionY.floorPixels();
    const bool crab = entity.type == 0x07 || entity.type == 0x08;
    const std::int32_t left = crab ? 10 : 17;
    const std::int32_t right = crab ? 10 : 18;
    const std::int32_t top = crab ? 35 : 20;
    const std::int32_t objectY = entity.y + (crab ? 32 : 16);
    return entity.x - left < playerX && playerX < entity.x + right &&
           objectY - top < playerY && playerY < objectY + 5;
}

bool LevelSession::atRightExit(const PlayerRecord &player) const {
    if (!_config.enableEdgeExit) {
        return false;
    }
    const std::int32_t mapWidth = static_cast<std::int32_t>(_map.width) * 16;
    return player.positionX.floorPixels() + _config.edgeExitMargin >= mapWidth;
}

bool LevelSession::updateStreaming(std::int32_t playerX, std::int32_t playerY) {
    return updateStreamingImpl(0, playerX, playerY);
}

bool LevelSession::updateStreaming(Simulation &simulation,
                                   std::int32_t playerX,
                                   std::int32_t playerY) {
    return updateStreamingImpl(&simulation.stateForSetup().scheduler,
                               playerX, playerY);
}

bool LevelSession::updateStreamingImpl(ObjectScheduler *scheduler,
                                       std::int32_t playerX,
                                       std::int32_t playerY) {
    const std::int32_t regionX = floorRegion(playerX);
    const std::int32_t regionY = floorRegion(playerY);
    bool spawnedTransient = false;
    for (std::size_t index = 0; index < _entities.size(); ++index) {
        LevelEntity &entity = _entities[index];
        if (entity.collected) {
            entity.phase = EntityPhase::Collected;
            entity.active = false;
            entity.pooledInteractionTriggered = false;
            releaseScheduledEntity(scheduler, entity);
            removeTransientEffectsFor(entity.id);
            continue;
        }
        const bool wasActive = entity.phase == EntityPhase::Active;
        const std::int32_t distanceX = std::abs(static_cast<std::int32_t>(entity.regionX) - regionX);
        const std::int32_t distanceY = std::abs(static_cast<std::int32_t>(entity.regionY) - regionY);
        const bool visible = distanceX <= _config.streamRadiusRegions &&
                             distanceY <= _config.streamRadiusRegions;
        entity.phase = visible ? EntityPhase::Active : EntityPhase::Dormant;
        entity.active = visible;
        if (visible && !wasActive) {
            if (scheduler != 0 && entity.updateCallback.offset != 0) {
                entity.schedulerHandle = scheduler->queueSpawn(
                    entity.updateCallback, entity.id, false);
            }
            spawnedTransient = spawnTransientEffect(entity) || spawnedTransient;
        } else if (!visible) {
            releaseScheduledEntity(scheduler, entity);
            entity.pooledInteractionTriggered = false;
            removeTransientEffectsFor(entity.id);
        }
    }
    return spawnedTransient;
}

void LevelSession::releaseScheduledEntity(ObjectScheduler *scheduler,
                                          LevelEntity &entity) {
    if (scheduler != 0 && entity.schedulerHandle.valid()) {
        scheduler->queueRelease(entity.schedulerHandle);
    }
    entity.schedulerHandle = SchedulerHandle();
}

void LevelSession::dispatchCollectibleCallbacks(Simulation *simulation,
                                                PlayerRecord &player) {
    ObjectScheduler *scheduler = simulation == 0
        ? 0 : &simulation->stateForSetup().scheduler;
    for (std::size_t index = 0; index < _entities.size(); ++index) {
        LevelEntity &entity = _entities[index];
        if (!entity.active || entity.collected ||
            entity.kind != EntityKind::Collectible ||
            entity.updateCallback.offset != 0x8d20) {
            continue;
        }

        // A queued ARE object becomes callable at beginTick(). The legacy
        // overload has no scheduler and remains useful for setup-only callers.
        if (scheduler != 0) {
            if (!entity.schedulerHandle.valid() ||
                entity.schedulerHandle.slot >= scheduler->objects().size() ||
                !scheduler->objects()[entity.schedulerHandle.slot].active) {
                continue;
            }
        }

        // The static 8D31 -> 393C bounds contract is represented at this
        // boundary by the existing player/entity overlap predicate. The
        // predicate is intentionally kept in one place until the exact DOS
        // fixed-point bounds trace is promoted from research evidence.
        if (!overlaps(player, entity, _config.collectibleRadius)) {
            continue;
        }

        std::vector<LevelStateWrite> writes;
        applyCollectibleCallback(entity, player, writes);
        entity.collected = true;
        entity.active = false;
        entity.phase = EntityPhase::Collected;
        entity.pooledInteractionTriggered = false;
        releaseScheduledEntity(scheduler, entity);
        removeTransientEffectsFor(entity.id);
        appendCollectedEvent(entity, writes);
    }
}

void LevelSession::applyCollectibleCallback(
    LevelEntity &entity, PlayerRecord &player,
    std::vector<LevelStateWrite> &writes) {
    const std::uint32_t scoreBefore = _score;
    std::uint16_t soundAction = 0;

    switch (entity.contactSubtype) {
    case 1: {
        soundAction = 9;
        const std::uint16_t before = _gameplayState.ammo880c;
        const std::uint16_t after = static_cast<std::uint16_t>(
            std::min<std::uint32_t>(99, static_cast<std::uint32_t>(before) + 10));
        _gameplayState.ammo880c = after;
        appendStateWrite(writes, 0x880c, 2, before, after);
        break;
    }
    case 2: {
        soundAction = 9;
        const std::uint16_t maximumBefore = _gameplayState.maximumHealth8824;
        const std::uint16_t maximumAfter = static_cast<std::uint16_t>(
            std::min<std::uint32_t>(5,
                                    static_cast<std::uint32_t>(maximumBefore) + 1));
        _gameplayState.maximumHealth8824 = maximumAfter;
        appendStateWrite(writes, 0x8824, 2, maximumBefore, maximumAfter);

        const std::uint16_t currentBefore = _gameplayState.currentHealth8822;
        _gameplayState.currentHealth8822 = maximumAfter;
        appendStateWrite(writes, 0x8822, 2, currentBefore, maximumAfter);
        break;
    }
    case 3: {
        soundAction = 10;
        const std::uint16_t before = _gameplayState.currentHealth8822;
        const std::uint16_t after = before < _gameplayState.maximumHealth8824
            ? static_cast<std::uint16_t>(before + 1) : before;
        _gameplayState.currentHealth8822 = after;
        appendStateWrite(writes, 0x8822, 2, before, after);
        break;
    }
    case 4: {
        soundAction = 12;
        const std::uint16_t before = _gameplayState.invulnerabilityGate8810;
        _gameplayState.invulnerabilityGate8810 = 0xffff;
        appendStateWrite(writes, 0x8810, 2, before, 0xffff);
        const std::uint16_t timerBefore = player.timer34;
        player.timer34 = 0x02bc;
        appendStateWrite(writes, 0x0034, 2, timerBefore, player.timer34);
        _gameplayState.playerTimer0034 = 0x02bc;
        break;
    }
    case 5: {
        soundAction = 12;
        const std::uint16_t before = _gameplayState.lives880a;
        const std::uint16_t after = static_cast<std::uint16_t>(
            std::min<std::uint32_t>(9, static_cast<std::uint32_t>(before) + 1));
        _gameplayState.lives880a = after;
        appendStateWrite(writes, 0x880a, 2, before, after);
        break;
    }
    default:
        if (entity.collectionBit != 0) {
            soundAction = 11;
            const std::uint16_t before = _gameplayState.puzzleMask60d8;
            const std::uint16_t after = static_cast<std::uint16_t>(
                before | entity.collectionBit);
            _gameplayState.puzzleMask60d8 = after;
            appendStateWrite(writes, 0x60d8, 2, before, after);
        }
        break;
    }

    if (soundAction != 0) {
        const std::uint16_t before = _gameplayState.pendingEvent612e;
        _gameplayState.pendingEvent612e = soundAction;
        appendStateWrite(writes, 0x612e, 2, before, soundAction);
    }

    const std::uint32_t value = collectibleValue(entity.type);
    const std::uint32_t scoreAfter = scoreBefore + value;
    _score = scoreAfter;
    _gameplayState.score881c = scoreAfter;
    appendStateWrite(writes, 0x881c, 4, scoreBefore, scoreAfter);
}

void LevelSession::appendCollectedEvent(
    const LevelEntity &entity, const std::vector<LevelStateWrite> &writes) {
    LevelEvent event;
    event.type = LevelEventType::Collected;
    event.entityId = entity.id;
    event.entityType = entity.type;
    event.stateWrites = writes;
    _events.push_back(event);
}

void LevelSession::syncPlayerTimer(const PlayerRecord &player) {
    if (player.timer34 != 0) {
        _gameplayState.playerTimer0034 = player.timer34;
    } else if (_gameplayState.playerTimer0034 != 0) {
        _gameplayState.playerTimer0034 = 0;
        _gameplayState.invulnerabilityGate8810 = 0;
    }
}

void LevelSession::emitHighEffect(std::int32_t sourceX, std::int32_t sourceY) {
    LevelEffect effect;
    // The DOS object has +0x1A=FFFF and is not tied to an ARE declaration.
    effect.sourceEntityId = 0;
    effect.sourceType = 0;
    effect.x = sourceX;
    effect.y = sourceY + 10;
    effect.spriteSlot = 611;
    effect.spriteResource = highEffectSpriteResource();
    effect.animationFrame = 0; // callback-specific +0x2A cursor
    effect.lifetime = 31; // terminal 30 -> 31 clears object+0x18
    effect.active = true;
    _effects.push_back(effect);
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

bool LevelSession::spawnTransientEffect(const LevelEntity &entity) {
    if (!isTransientEffectType(entity.type)) {
        return false;
    }
    LevelEffect effect;
    effect.sourceEntityId = entity.id;
    effect.sourceType = entity.type;
    effect.x = entity.x;
    effect.y = entity.y;
    effect.effectSlot = entity.effectSlot;
    effect.effectResource = entity.effectResource;
    effect.animationFrame = 0;
    // The event object is short-lived and advances its animation byte modulo
    // eight. Its exact removal timing is not yet fully mapped; eight ticks
    // preserves the observed event animation without turning the ARE seed
    // into a permanent sprite.
    effect.lifetime = 8;
    effect.active = true;
    _effects.push_back(effect);
    return true;
}

void LevelSession::removeTransientEffectsFor(std::uint32_t entityId) {
    for (std::size_t index = 0; index < _effects.size();) {
        if (_effects[index].sourceEntityId == entityId) {
            _effects.erase(_effects.begin() + static_cast<std::ptrdiff_t>(index));
        } else {
            ++index;
        }
    }
}

void LevelSession::advanceActiveEffects() {
    for (std::size_t index = 0; index < _effects.size();) {
        LevelEffect &effect = _effects[index];
        if (!effect.active) {
            _effects.erase(_effects.begin() + static_cast<std::ptrdiff_t>(index));
            continue;
        }

        if (!effect.spriteResource.empty()) {
            // 4C74 increments +0x2A before selecting the next frame. Slots
            // 611, 612, and 613 cover cursors 0..9, 10..19, and 20..30;
            // cursor 30 -> 31 is the terminal clear and is not rendered.
            if (effect.animationFrame >= 30) {
                _effects.erase(_effects.begin() + static_cast<std::ptrdiff_t>(index));
                continue;
            }
            ++effect.animationFrame;
            const std::uint16_t spriteGroup = std::min<std::uint16_t>(
                2, static_cast<std::uint16_t>(effect.animationFrame / 10));
            effect.spriteSlot = static_cast<std::uint16_t>(611 + spriteGroup);
            ++index;
            continue;
        }

        if (effect.animationFrame + 1 >= effect.lifetime) {
            _effects.erase(_effects.begin() + static_cast<std::ptrdiff_t>(index));
            continue;
        }
        ++effect.animationFrame;
        ++index;
    }
}

bool LevelSession::emitWorldEffectsForActiveEntities() {
    bool emitted = false;
    for (std::size_t index = 0; index < _entities.size(); ++index) {
        const LevelEntity &entity = _entities[index];
        if (entity.phase != EntityPhase::Active ||
            !isWorldEffectType(entity.type)) {
            continue;
        }
        if (entity.activeFrames == 4 || entity.activeFrames == 6 ||
            entity.activeFrames == 8 || entity.activeFrames == 10) {
            emitted = emitWorldEffects(
                          entity,
                          static_cast<std::uint16_t>(entity.activeFrames)) || emitted;
        }
    }
    return emitted;
}

bool LevelSession::emitWorldEffects(const LevelEntity &entity,
                                    std::uint16_t state) {
    bool emitted = false;
    const std::int32_t yOffset = state == 4 ? 0 : (state / 2 - 2) * 16;
    const std::int32_t firstXOffset = state == 4 ? 0 : 16;
    const std::int32_t xOffsets[] = {16, 0, 32, 48, 64};
    const std::string world = worldForMap(_mapName);

    for (std::size_t index = 0; index < 5; ++index) {
        const std::int32_t x = entity.x +
            (state == 4 ? xOffsets[index] : firstXOffset +
             static_cast<std::int32_t>(index) * 16);
        const std::int32_t y = entity.y + yOffset;
        if (x < 0 || y < 0) {
            continue;
        }
        const std::int32_t tileX = x / 16;
        const std::int32_t tileY = y / 16;
        if (tileX >= _map.width || tileY >= _map.height) {
            continue;
        }
        const std::uint16_t tile = Map::tileId(
            _map.cell(static_cast<std::uint16_t>(tileX),
                      static_cast<std::uint16_t>(tileY)));
        const std::uint16_t effectSlot = worldEffectSlotFor(world, tile);
        if (effectSlot == 0) {
            continue;
        }

        LevelEffect effect;
        effect.sourceEntityId = entity.id;
        effect.sourceType = entity.type;
        effect.x = x;
        effect.y = y;
        effect.effectSlot = effectSlot;
        effect.effectResource = "WORLD";
        effect.animationFrame = 0;
        effect.lifetime = 3;
        effect.active = true;
        _effects.push_back(effect);
        emitted = true;
    }
    return emitted;
}

void LevelSession::tick(Simulation &simulation,
                        const WorldCollisionView &world,
                        const InputState &input,
                        SimulationOutput &output) {
    PlayerRecord &player = simulation.stateForSetup().player;
    const bool alternatePressed = input.alternate && !_alternateActionActive;
    _alternateActionActive = input.alternate;
    bool spawnedTransient = updateStreaming(
        simulation, player.positionX.floorPixels(), player.positionY.floorPixels());
    simulation.tick(input, world, output);
    syncPlayerTimer(player);
    spawnedTransient = updateStreaming(
        simulation, player.positionX.floorPixels(), player.positionY.floorPixels()) ||
        spawnedTransient;
    advanceActiveEntities();
    advanceActiveEffects();
    const bool emittedTileEffect = emitWorldEffectsForActiveEntities();

    if (spawnedTransient) {
        enqueueEvent(LevelEventType::WorldObjectInteraction);
    }
    if (emittedTileEffect) {
        enqueueEvent(LevelEventType::TileInteraction);
    }

    if (alternatePressed) {
        enqueueEvent(LevelEventType::AlternateActionObject);
    }

    dispatchCollectibleCallbacks(&simulation, player);

    for (std::size_t index = 0; index < _entities.size(); ++index) {
        LevelEntity &entity = _entities[index];
        if (!entity.active || entity.collected) {
            continue;
        }
        if (entity.kind == EntityKind::Hazard &&
                   isPooledInteractionType(entity.type)) {
            const bool pooledContact = pooledInteractionOverlaps(player, entity);
            if (!pooledContact) {
                entity.pooledInteractionTriggered = false;
            } else if (!entity.pooledInteractionTriggered) {
                entity.pooledInteractionTriggered = true;
                enqueueEvent(LevelEventType::PooledObjectInteractionBurst,
                             entity.id, entity.type);
            }
            if (overlaps(player, entity, _config.hazardRadius)) {
                resetPlayer(simulation);
                ++_deaths;
                enqueueEvent(LevelEventType::PlayerDied, entity.id, entity.type);
                updateStreaming(simulation, player.positionX.floorPixels(),
                                player.positionY.floorPixels());
                output.player = player;
                output.player.syncToRaw();
                return;
            }
        } else if (entity.kind == EntityKind::Hazard &&
                   overlaps(player, entity, _config.hazardRadius)) {
            resetPlayer(simulation);
            ++_deaths;
            enqueueEvent(LevelEventType::PlayerDied, entity.id, entity.type);
            updateStreaming(simulation, player.positionX.floorPixels(),
                            player.positionY.floorPixels());
            output.player = player;
            output.player.syncToRaw();
            return;
        }
    }

    if (atRightExit(player)) {
        enqueueEvent(LevelEventType::LevelExit, 0, 0, nextLevelName(_mapName));
    }

    output.player = player;
    output.player.syncToRaw();
}

void LevelSession::enqueueEvent(LevelEventType type, std::uint32_t entityId,
                                std::uint16_t entityType,
                                const std::string &targetLevel) {
    LevelEvent event;
    event.type = type;
    event.entityId = entityId;
    event.entityType = entityType;
    event.targetLevel = targetLevel;
    _events.push_back(event);
}

LevelEvent LevelSession::consumeEvent() {
    if (_events.empty()) {
        return LevelEvent();
    }
    const LevelEvent event = _events.front();
    _events.pop_front();
    return event;
}

} // namespace quiky
