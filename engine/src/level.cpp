#include "quiky/level.h"
#include "quiky/player_update.h"

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

std::string completedGoalTarget(const std::string &mapName,
                                std::uint16_t puzzleMask) {
    if (upperAscii(mapName) != "W1L1.MAP") {
        return std::string();
    }
    // The focused 5010/18C7 evidence distinguishes the ordinary completion
    // route from the all-seven-letter bonus route. Keep other masks outside
    // the transition until their selector mapping is recovered.
    if (puzzleMask == 0) {
        return "W1L2.MAP";
    }
    if (puzzleMask == 0x007f) {
        return "W1L4.MAP";
    }
    return std::string();
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
      puzzleMask60d8(0),
      terminalX8828(0),
      terminalY882a(0),
      cloudSignal89e6(0),
      transitionGate89ea(0),
      platformLatch5006(0),
      platformCarryX8816(0),
      platformCarryY8812(0) {
}

LevelSessionConfig::LevelSessionConfig()
    : streamRadiusRegions(1),
      collectibleRadius(12),
      hazardRadius(12),
      edgeExitMargin(24),
      enableEdgeExit(true),
      hasSpawn(false),
      spawnX(100),
      spawnY(100),
      hasLeafPrngState(false),
      leafPrngIndex(0),
      leafPrngRing() {
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
      _alternateActionActive(false),
      _streamAnchorActive(false),
      _streamAnchorX(0),
      _streamAnchorY(0),
      _leafPrngIndex(config.leafPrngIndex),
      _leafPrngRing(config.leafPrngRing) {
    if (!config.hasLeafPrngState) {
        _leafPrngIndex = 0;
        _leafPrngRing.fill(0);
    }
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
        entity.initialX = static_cast<std::int32_t>(placement.worldX);
        entity.initialY = static_cast<std::int32_t>(placement.worldY);
        entity.x = entity.initialX;
        entity.y = entity.initialY;
        entity.kind = classify(entity.type);
        if (isNormalEnemyType(entity.type)) {
            // 6D5F/684A place the enemy object 0x20 pixels below the ARE
            // declaration. Keep the declaration anchor separately so reset
            // can reproduce the initializer rather than the last position.
            entity.y += 0x20;
        }
        entity.positionX = Fixed16::fromPixels(entity.x);
        entity.positionY = Fixed16::fromPixels(entity.y);
        entity.spriteSlot = spriteSlotFor(entity.type);
        entity.spriteResource = spriteResourceFor(entity.type);
        entity.effectSlot = effectSlotFor(entity.type);
        entity.effectResource = effectResourceFor(entity.type);
        entity.updateCallback = callbackFor(entity.type);
        entity.contactSubtype = collectibleSubtypeFor(entity.type);
        entity.collectionBit = collectionBitFor(entity.type);
        entity.collisionWidth = collisionWidthFor(entity.type);
        entity.collisionHeight = collisionHeightFor(entity.type);
        initializeEnemy(entity);
        initializeCollectible(entity);
        initializeWorldEffect(entity);
        initializeAmbientVisual(entity);
        initializeMovingPlatform(entity);
        _entities.push_back(entity);
    }
}

SpawnPoint LevelSession::spawnPoint() const {
    if (_config.hasSpawn) {
        return SpawnPoint(_config.spawnX, _config.spawnY);
    }

    // The native W1L1 player initializer and the complete jump/property
    // traces begin at (128,400). Keep this evidence-backed level default
    // separate from explicit test/frontend overrides; other levels retain
    // the conservative fallback until their player declarations are closed.
    if (upperAscii(_mapName) == "W1L1.MAP") {
        return SpawnPoint(128, 400);
    }

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
    if (_config.hasLeafPrngState) {
        _leafPrngIndex = _config.leafPrngIndex;
        _leafPrngRing = _config.leafPrngRing;
    } else {
        _leafPrngIndex = 0;
        _leafPrngRing.fill(0);
    }
    for (std::size_t index = 0; index < _entities.size(); ++index) {
        LevelEntity &entity = _entities[index];
        entity.phase = EntityPhase::Dormant;
        entity.x = entity.initialX;
        entity.y = entity.initialY + (isNormalEnemyType(entity.type) ? 0x20 : 0);
        entity.positionX = Fixed16::fromPixels(entity.x);
        entity.positionY = Fixed16::fromPixels(entity.y);
        entity.animationFrame = 0;
        entity.activeFrames = 0;
        entity.active = false;
        entity.collected = false;
        entity.pooledInteractionTriggered = false;
        entity.schedulerHandle = SchedulerHandle();
        entity.updateCallback = callbackFor(entity.type);
        entity.contactSubtype = collectibleSubtypeFor(entity.type);
        entity.collectionBit = collectionBitFor(entity.type);
        entity.streamSuppressed = false;
        entity.enemyContactPending = false;
        entity.contactCallback = CallbackIdentity();
        entity.responseTimer = 0;
        initializeEnemy(entity);
        initializeCollectible(entity);
        initializeWorldEffect(entity);
        initializeAmbientVisual(entity);
        initializeMovingPlatform(entity);
    }
    _effects.clear();
    resetPlayer(simulation);
}

void LevelSession::resetPlayer(Simulation &simulation) {
    const SpawnPoint spawn = spawnPoint();
    simulation.reset();
    PlayerRecord &player = simulation.stateForSetup().player;
    player.positionX = Fixed16::fromPixels(spawn.x);
    player.positionY = Fixed16::fromPixels(spawn.y);
    player.initializeRecoveredCallbackFields();
    // The native scheduler has advanced the player pool record to phase 2 by
    // the time 3FF8 is entered. This is a pool-lifecycle byte, not callback
    // policy, and is kept explicit at the session boundary.
    player.field17 = 2;
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
    if (type >= 0x1f && type <= 0x21) {
        return EntityKind::EnvironmentalEffect;
    }
    if (isCloudType(type) || isLeafType(type) ||
        isDedicatedEventType(type)) {
        return EntityKind::AmbientVisual;
    }
    return EntityKind::Unknown;
}

bool LevelSession::isNormalEnemyType(std::uint16_t type) {
    return isWurm2Type(type) || isBieneType(type);
}

bool LevelSession::isWurm2Type(std::uint16_t type) {
    return type == 0x01 || type == 0x02;
}

bool LevelSession::isBieneType(std::uint16_t type) {
    return type == 0x03 || type == 0x04;
}

bool LevelSession::isCloudType(std::uint16_t type) {
    return type == 0x28;
}

bool LevelSession::isLeafType(std::uint16_t type) {
    return type >= 0x29 && type <= 0x2b;
}

bool LevelSession::isDedicatedEventType(std::uint16_t type) {
    return type >= 0x65 && type <= 0x67;
}

void LevelSession::initializeEnemy(LevelEntity &entity) {
    if (!isNormalEnemyType(entity.type)) {
        entity.velocityX = Fixed16();
        entity.velocityY = Fixed16();
        entity.enemyPhaseTimer = 0;
        entity.enemyTimer = 0;
        entity.enemyState = 0;
        entity.mapBlocked = 0;
        entity.enemyAnimationDelay = 0;
        return;
    }

    const bool rightVariant = entity.type == 0x02 || entity.type == 0x04;
    entity.velocityX = Fixed16(rightVariant ? 0x15000 : -0x15000);
    entity.velocityY = Fixed16();
    entity.enemyPhaseTimer = 0;
    entity.enemyTimer = 0x14;
    entity.enemyState = 0;
    entity.mapBlocked = 0;
    entity.enemyAnimationDelay = 0x0e;
    entity.enemyContactPending = false;
    entity.contactCallback = CallbackIdentity();
    entity.responseTimer = 0;
}

void LevelSession::initializeCollectible(LevelEntity &entity) {
    if (entity.type == 0x6f) {
        // 01F7:8BC2: WERBE type 0x6F publishes slot 0x25F and places the
        // pooled object at (ARE x + 1, ARE y - 2).
        entity.x += 1;
        entity.y -= 2;
    } else if (entity.type == 0x70 || entity.type == 0x71) {
        // 01F7:8BE5/8C08: the health variants share the (x + 5, y + 10)
        // initializer projection and publish slots 0x260/0x261.
        entity.x += 5;
        entity.y += 10;
    } else if (entity.type == 0x72) {
        // 01F7:8C2B: invulnerability pickup placement.
        entity.x += 3;
        entity.y += 7;
    } else {
        return;
    }
    entity.positionX = Fixed16::fromPixels(entity.x);
    entity.positionY = Fixed16::fromPixels(entity.y);
}

void LevelSession::initializeWorldEffect(LevelEntity &entity) {
    entity.environmentSelector = 0;
    entity.environmentState = 0;
    if (!isWorldEffectType(entity.type)) {
        return;
    }

    // 01F7:8B3D/8B50/8B63 set object+0x2E to 1/2/3 and converge on
    // 01F7:8E4B. The separate +0x32 callback state starts at zero.
    entity.environmentSelector = static_cast<std::uint16_t>(
        entity.type - 0x1e);
}

void LevelSession::initializeAmbientVisual(LevelEntity &entity) {
    entity.ambientVelocityY = Fixed16();
    entity.ambientOriginX = entity.x;
    entity.ambientOriginY = entity.y;
    entity.ambientTimer = 0;
    entity.ambientAnimationDelay = 0;
    entity.ambientAnimationCursor = 0;
    entity.ambientTable = 0;
    entity.ambientRuntimeInitialized = false;

    if (!isLeafType(entity.type)) {
        return;
    }

    // 01F7:4727 is invoked when the pooled object is published, not while the
    // ARE declarations are merely parsed. Keep the declaration-side fields
    // at the confirmed zero-seed baseline until that publication edge.
    entity.ambientVelocityY = Fixed16(0x13000);
    entity.ambientOriginX = entity.x;
    entity.ambientOriginY = entity.y;
    entity.ambientTimer = 0x000c;
    entity.ambientTable = 1;
    entity.ambientAnimationDelay = 10;
    entity.ambientAnimationCursor = 0x3328;
}

std::uint8_t LevelSession::nextLeafPrngByte() {
    const std::uint8_t value = _leafPrngRing[_leafPrngIndex & 0x00ff];
    _leafPrngIndex = static_cast<std::uint16_t>((_leafPrngIndex + 1) & 0x00ff);
    return value;
}

void LevelSession::initializeAmbientVisualRuntime(LevelEntity &entity) {
    if (!isLeafType(entity.type)) {
        return;
    }

    // 01F7:4727:
    //   signed ring byte > 0 -> DS:3312 (delay 8, slots 700..707)
    //   signed ring byte <=0 -> DS:3326 (delay 10, slots 703..707,700..702)
    // The second ring byte is consumed by the fixed-point velocity seed.
    const std::int8_t tableSeed = static_cast<std::int8_t>(nextLeafPrngByte());
    entity.ambientTable = tableSeed > 0 ? 0 : 1;
    entity.ambientAnimationDelay = entity.ambientTable == 0 ? 8 : 10;
    entity.ambientAnimationCursor = entity.ambientTable == 0
        ? 0x3314 : 0x3328;
    entity.spriteSlot = entity.ambientTable == 0 ? 700 : 703;

    const std::int8_t velocitySeed =
        static_cast<std::int8_t>(nextLeafPrngByte());
    const std::int32_t perturbation =
        static_cast<std::int32_t>(velocitySeed) * 0x80;
    entity.ambientVelocityY = Fixed16::fromRaw(
        Fixed16::wrapSubRaw(0x13000, perturbation));
    entity.ambientOriginX = entity.x;
    entity.ambientOriginY = entity.y;
    entity.ambientTimer = 0x000c;
    entity.ambientRuntimeInitialized = true;
}

void LevelSession::initializeMovingPlatform(LevelEntity &entity) {
    entity.platformPreviousX = entity.x;
    entity.platformPreviousY = entity.y;
    entity.platformWait52 = 0;
    entity.platformWait54 = 0;
    entity.platformCooldown58 = 0;
    entity.platformCarryActive = false;
    if (entity.kind != EntityKind::MovingPlatform) {
        return;
    }

    // The four 9CF5/9D19/9D5E/9D82 initializers publish static PLATFW
    // geometry and flags. The archived post-initializer records show zero
    // initial velocity; authored motion may seed it later through the
    // address-qualified state path.
    entity.velocityX = Fixed16();
    entity.velocityY = Fixed16();
}

CallbackIdentity LevelSession::callbackFor(std::uint16_t type) {
    // These identities are the recovered ARE callback targets. Collectibles
    // and the two W1 normal-enemy families are executed by this slice; the
    // other identities are published for scheduler order and remain external
    // contracts.
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
    if (type == 0x01 || type == 0x02) {
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
    // 9256 leaves object+0x12 at FFFF. WOLKE slots 413-416 belong to the
    // unresolved special renderer and must not be published as a normal
    // scheduler slot.
    case 0x28: return 0xffff;
    case 0x29:
    case 0x2b: return 700;
    case 0x2a: return 703;
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
        const std::string world = worldForMap(_mapName);
        return world.empty() ? "WORLD.ICO" : world + ".ICO";
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

bool LevelSession::cloudOverlaps(const PlayerRecord &player,
                                 const LevelEntity &entity) const {
    // 01F7:9269 compares the cloud's 16x16 box against the normalized
    // player bounds. This is intentionally separate from the broad overlap
    // predicate used by provisional hazard families.
    const std::int32_t playerX = (player.positionX.floorPixels() / 16) * 16;
    const std::int32_t playerY = (player.positionY.floorPixels() / 16) * 16;
    const std::int32_t cloudX = (entity.x / 16) * 16;
    const std::int32_t cloudY = (entity.y / 16) * 16;
    return cloudX < playerX + 16 && cloudX + 16 > playerX &&
           cloudY < playerY + 16 && cloudY + 16 > playerY;
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

void LevelSession::setStreamAnchor(std::int32_t cameraX,
                                   std::int32_t cameraY) {
    _streamAnchorActive = true;
    _streamAnchorX = cameraX;
    _streamAnchorY = cameraY;
}

void LevelSession::clearStreamAnchor() {
    _streamAnchorActive = false;
    _streamAnchorX = 0;
    _streamAnchorY = 0;
}

bool LevelSession::updateStreamingImpl(ObjectScheduler *scheduler,
                                       std::int32_t playerX,
                                       std::int32_t playerY) {
    const std::int32_t regionX = floorRegion(playerX);
    const std::int32_t regionY = floorRegion(playerY);
    bool spawnedTransient = false;
    std::vector<std::size_t> streamOrder;
    streamOrder.reserve(_entities.size());
    for (std::size_t index = 0; index < _entities.size(); ++index) {
        streamOrder.push_back(index);
    }
    if (_streamAnchorActive) {
        // 01F7:1CDA scans the camera stream through the ARE layout's
        // vertical/horizontal edge passes. The native startup pool trace
        // establishes the resulting order as descending region X, then
        // ascending region Y, with declaration order as the tie-breaker.
        std::stable_sort(
            streamOrder.begin(), streamOrder.end(),
            [this](std::size_t left, std::size_t right) {
                const LevelEntity &a = _entities[left];
                const LevelEntity &b = _entities[right];
                if (a.regionX != b.regionX) {
                    return a.regionX > b.regionX;
                }
                if (a.regionY != b.regionY) {
                    return a.regionY < b.regionY;
                }
                return a.recordOffset < b.recordOffset;
            });
    }
    for (std::size_t orderIndex = 0;
         orderIndex < streamOrder.size(); ++orderIndex) {
        LevelEntity &entity = _entities[streamOrder[orderIndex]];
        if (entity.collected) {
            entity.phase = EntityPhase::Collected;
            entity.active = false;
            entity.pooledInteractionTriggered = false;
            releaseScheduledEntity(scheduler, entity);
            removeTransientEffectsFor(entity.id);
            continue;
        }
        const bool wasActive = entity.phase == EntityPhase::Active;
        bool visible = false;
        if (_streamAnchorActive) {
            // Static 01F7:1DCA: the object gate is pixel-relative to
            // DS:81C0/81C4, with X limits -0x80..+0x1c0 and Y limits
            // -0x80..+0x130. Do not reduce this to a centered region test:
            // W1L1's native startup camera at (0,262) accepts objects in
            // region X=2.
            visible = entity.x >= _streamAnchorX - 0x80 &&
                      entity.x <= _streamAnchorX + 0x1c0 &&
                      entity.y >= _streamAnchorY - 0x80 &&
                      entity.y <= _streamAnchorY + 0x130;
        } else {
            const std::int32_t distanceX =
                std::abs(static_cast<std::int32_t>(entity.regionX) - regionX);
            const std::int32_t distanceY =
                std::abs(static_cast<std::int32_t>(entity.regionY) - regionY);
            visible = distanceX <= _config.streamRadiusRegions &&
                      distanceY <= _config.streamRadiusRegions;
        }
        if (visible && (isNormalEnemyType(entity.type) ||
                        isWorldEffectType(entity.type) ||
                        isCloudType(entity.type)) &&
            entity.streamSuppressed && !wasActive) {
            // 01F7:1DEE and the state-10 8E4B path clear the object and its
            // active claim. A loaded-region sweep does not revisit that
            // declaration; reconstruction belongs to the level reload path,
            // not to this visibility pass.
            entity.phase = EntityPhase::Dormant;
            entity.active = false;
            continue;
        }
        entity.phase = visible ? EntityPhase::Active : EntityPhase::Dormant;
        entity.active = visible;
        if (visible && !wasActive) {
            if (isLeafType(entity.type)) {
                // 01F7:1CDA publishes the callback through the pooled object
                // initializer. Re-entry therefore restores the ARE anchor
                // before 01F7:4727 consumes the shared PRNG ring.
                entity.x = entity.initialX;
                entity.y = entity.initialY;
                entity.positionX = Fixed16::fromPixels(entity.x);
                entity.positionY = Fixed16::fromPixels(entity.y);
                initializeAmbientVisualRuntime(entity);
            }
            if (entity.kind == EntityKind::MovingPlatform) {
                // 01F7:1DEE clears the pooled callback and leaves the ARE
                // declaration eligible for reconstruction. Re-entry therefore
                // starts from the declaration anchor and the platform
                // initializer state rather than from the culled object.
                entity.x = entity.initialX;
                entity.y = entity.initialY;
                entity.positionX = Fixed16::fromPixels(entity.x);
                entity.positionY = Fixed16::fromPixels(entity.y);
                initializeMovingPlatform(entity);
            }
            if (scheduler != 0 && entity.updateCallback.offset != 0) {
                entity.schedulerHandle = scheduler->queueSpawn(
                    entity.updateCallback, entity.id, false);
            }
            spawnedTransient = spawnTransientEffect(entity) || spawnedTransient;
        } else if (!visible) {
            if (entity.kind == EntityKind::MovingPlatform) {
                // The off-camera callback is gone before A0B2 can publish a
                // new carry. Do not retain object-local carry phase or wait
                // counters across the pooled-object lifetime boundary.
                entity.platformCarryActive = false;
                entity.platformCooldown58 = 0;
                entity.platformWait52 = 0;
                entity.platformWait54 = 0;
            }
            if ((isNormalEnemyType(entity.type) ||
                 isWorldEffectType(entity.type) ||
                 isCloudType(entity.type)) && wasActive) {
                entity.streamSuppressed = true;
            }
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

void LevelSession::dispatchEnemyCallbacks(
    Simulation *simulation, const WorldCollisionView &world,
    const PlayerRecord &player) {
    ObjectScheduler *scheduler = simulation == 0
        ? 0 : &simulation->stateForSetup().scheduler;
    for (std::size_t index = 0; index < _entities.size(); ++index) {
        LevelEntity &entity = _entities[index];
        if (!entity.active || !isNormalEnemyType(entity.type) ||
            entity.updateCallback.offset == 0) {
            continue;
        }
        if (scheduler != 0 &&
            (!entity.schedulerHandle.valid() ||
             entity.schedulerHandle.slot >= scheduler->objects().size() ||
             !scheduler->objects()[entity.schedulerHandle.slot].active)) {
            continue;
        }

        if (entity.enemyContactPending) {
            advanceEnemyResponse(simulation, entity);
            continue;
        }

        if (isWurm2Type(entity.type)) {
            updateWurm2(entity, world);
        } else {
            updateBiene(entity, world);
        }

        // The family callbacks publish the same animation delay field as the
        // initializers. The BOB cursor tables remain renderer-owned; this
        // counter is retained here so the callback-visible timing is not
        // replaced with a generic frame counter.
        if (entity.enemyAnimationDelay != 0) {
            --entity.enemyAnimationDelay;
        } else {
            entity.enemyAnimationDelay = 0x0e;
            entity.animationFrame = static_cast<std::uint16_t>(
                (entity.animationFrame + 1) & 0x00ff);
        }

        // The native contact tail is a separate 4AB3/4C5D object response.
        // Keep the overlap predicate explicit until the persistent-player
        // coordinate source and family-specific bounds are moved into the
        // level trace contract. It must not be treated as player death.
        if (overlaps(player, entity, _config.hazardRadius)) {
            beginEnemyContact(entity);
        }
    }
}

void LevelSession::dispatchCloudCallbacks(Simulation *simulation,
                                          const PlayerRecord &player) {
    ObjectScheduler *scheduler = simulation == 0
        ? 0 : &simulation->stateForSetup().scheduler;
    for (std::size_t index = 0; index < _entities.size(); ++index) {
        LevelEntity &entity = _entities[index];
        if (!entity.active || !isCloudType(entity.type) ||
            entity.updateCallback.offset != 0x9269) {
            continue;
        }
        if (scheduler != 0 &&
            (!entity.schedulerHandle.valid() ||
             entity.schedulerHandle.slot >= scheduler->objects().size() ||
             !scheduler->objects()[entity.schedulerHandle.slot].active)) {
            continue;
        }

        // 01F7:92A9/92AF writes DS:89E6=-1 only on the accepted path. The
        // signal is a latch consumed by the outer 01D7:4EA0 transition pass;
        // rejected frames do not clear it here.
        if (_gameplayState.transitionGate89ea == 0 &&
            player.mode37 == 0 && cloudOverlaps(player, entity)) {
            _gameplayState.cloudSignal89e6 = 0xffff;
        }
    }
}

void LevelSession::dispatchMovingPlatformCallbacks(
    Simulation *simulation, const WorldCollisionView &world,
    PlayerRecord &player,
    std::vector<SimulationCallbackStep> &dependencyOrder) {
    for (std::size_t index = 0; index < _entities.size(); ++index) {
        LevelEntity &entity = _entities[index];
        if (!entity.active || entity.kind != EntityKind::MovingPlatform ||
            entity.updateCallback.offset != 0x9dc7) {
            continue;
        }

        dependencyOrder.push_back(SimulationCallbackStep(
            SimulationCallbackPhase::MovingPlatformBeforePlayer,
            entity.id, entity.updateCallback));

        // 01F7:9DC7 clears DS:5006 before A075. Keep the local publication
        // visible even when no platform accepts the player this frame.
        _gameplayState.platformLatch5006 = 0;
        _gameplayState.platformCarryX8816 = 0;
        _gameplayState.platformCarryY8812 = 0;

        bool accepted = false;
        if (entity.platformCooldown58 != 0) {
            --entity.platformCooldown58;
        } else if (player.mode37 >= 0) {
            const std::int32_t playerX = player.positionX.floorPixels();
            const std::int32_t playerY = player.positionY.floorPixels();
            const std::int32_t width = entity.collisionWidth;
            // 01F7:A075: strict X interval, half-open 12-pixel Y interval.
            accepted = entity.x < playerX &&
                       playerX < entity.x + width &&
                       entity.y <= playerY && playerY < entity.y + 12;
        }
        if (accepted) {
            _gameplayState.platformLatch5006 = 0xffff;
        }

        // Object+0x2A is the integer platform position captured before the
        // motion branch. A0B2 compares the post-motion object+0x04 against it.
        entity.platformPreviousX = entity.x;
        entity.platformPreviousY = entity.y;

        if (entity.platformWait54 != 0) {
            --entity.platformWait54;
        } else if (entity.platformWait52 != 0) {
            --entity.platformWait52;
        } else if (entity.velocityX.raw != 0) {
            const std::int32_t direction = entity.velocityX.raw < 0 ? -1 : 1;
            const std::int32_t probeX = entity.x + direction * 16;
            if (world.mapRawBit0800Confirmed(probeX, entity.y)) {
                entity.velocityX = Fixed16();
                entity.x = (entity.x / 16) * 16;
                entity.positionX = Fixed16::fromPixels(entity.x);
                entity.platformWait54 = 0x46;
            } else {
                entity.positionX.raw = Fixed16::wrapAddRaw(
                    entity.positionX.raw, entity.velocityX.raw);
                entity.x = entity.positionX.floorPixels();
            }
        } else if (entity.velocityY.raw != 0) {
            const std::int32_t direction = entity.velocityY.raw < 0 ? -1 : 1;
            const std::int32_t probeY = entity.y + direction * 16;
            if (world.mapRawBit0800Confirmed(entity.x, probeY)) {
                entity.velocityY = Fixed16();
                entity.y = (entity.y / 16) * 16;
                entity.positionY = Fixed16::fromPixels(entity.y);
                entity.platformWait54 = 0x46;
            } else {
                entity.positionY.raw = Fixed16::wrapAddRaw(
                    entity.positionY.raw, entity.velocityY.raw);
                entity.y = entity.positionY.floorPixels();
            }
        }

        if (accepted) {
            const std::int32_t pixelDelta =
                entity.x - entity.platformPreviousX;
            const std::uint32_t shiftedDelta = static_cast<std::uint32_t>(
                pixelDelta) << 16;
            const std::int32_t carryX = static_cast<std::int32_t>(
                shiftedDelta) + 1;
            const std::int32_t platformY = Fixed16::fromPixels(entity.y).raw;
            const std::int32_t carryY = Fixed16::wrapAddRaw(
                Fixed16::wrapSubRaw(platformY, player.positionY.raw), 1);
            _gameplayState.platformCarryX8816 = carryX;
            _gameplayState.platformCarryY8812 = carryY;
            entity.platformCarryActive = true;
            if (simulation != 0 && simulation->playerUpdater() != 0) {
                simulation->playerUpdater()->publishPlatformCarry(
                    carryX, carryY);
            }
        } else if (entity.platformCarryActive) {
            // A0B2 clears +0x5A and starts the 0x14-tick recontact cooldown
            // when the prior carry latch is no longer accepted.
            entity.platformCarryActive = false;
            entity.platformCooldown58 = 0x14;
        }
    }
}

bool LevelSession::dispatchWorldEffectCallbacks(Simulation *simulation) {
    ObjectScheduler *scheduler = simulation == 0
        ? 0 : &simulation->stateForSetup().scheduler;
    bool emitted = false;
    for (std::size_t index = 0; index < _entities.size(); ++index) {
        LevelEntity &entity = _entities[index];
        if (!entity.active || entity.kind != EntityKind::EnvironmentalEffect ||
            entity.updateCallback.offset != 0x8e4b) {
            continue;
        }
        if (scheduler != 0 &&
            (!entity.schedulerHandle.valid() ||
             entity.schedulerHandle.slot >= scheduler->objects().size() ||
             !scheduler->objects()[entity.schedulerHandle.slot].active)) {
            continue;
        }
        emitted = updateWorldEffect(simulation, entity) || emitted;
    }
    return emitted;
}

bool LevelSession::updateWorldEffect(Simulation *simulation,
                                      LevelEntity &entity) {
    // The 8E4B zero-state path has already passed through the scheduler's
    // camera/loaded-region gate. Its remaining 393C bounds gate is an
    // external persistent-player contract; keeping the object dormant until
    // it is streamed is the native state available to this engine boundary.
    if (entity.environmentState == 0) {
        entity.environmentState = 1;
        return false;
    }

    // 01F7:8E4B increments object+0x32 before dispatching states 4, 6, 8,
    // and 10. All other intermediate states return without side effects.
    ++entity.environmentState;
    const std::uint16_t state = entity.environmentState;
    bool emitted = false;
    if (state == 4 || state == 6 || state == 8 || state == 10) {
        emitted = emitWorldEffects(entity, state);
    }

    if (state == 10) {
        // The callback clears object+0x18 and publishes two int32 values at
        // DS:8828/DS:882A. 01F7:1AAA's indexed consumer is intentionally
        // outside this closure; these writes are retained as named state.
        const std::int32_t afterX = entity.x + 0x19;
        const std::int32_t afterY = entity.y + 0x46;
        _gameplayState.terminalX8828 = afterX;
        _gameplayState.terminalY882a = afterY;

        entity.active = false;
        entity.phase = EntityPhase::Dormant;
        entity.streamSuppressed = true;
        releaseScheduledEntity(
            simulation == 0 ? 0 : &simulation->stateForSetup().scheduler,
            entity);
    }
    return emitted;
}

void LevelSession::updateWurm2(LevelEntity &entity,
                               const WorldCollisionView &world) {
    const bool blocked = enemyMapBlocked(entity, world);
    entity.mapBlocked = blocked ? 1 : 0;
    if (blocked) {
        entity.velocityX.raw = Fixed16::wrapNegRaw(entity.velocityX.raw);
    }
    entity.positionX.raw = Fixed16::wrapAddRaw(entity.positionX.raw,
                                                entity.velocityX.raw);
    entity.x = entity.positionX.floorPixels();

    // 6DC4 uses +0x2A as a 0..0x96 patrol phase before the PRNG/state branch.
    // The branch's random vertical target is outside this W1L1 implementation
    // until its selected runtime value is captured; do not invent one here.
    if (entity.enemyPhaseTimer < 0x96) {
        ++entity.enemyPhaseTimer;
    } else {
        entity.enemyPhaseTimer = 0;
    }
}

void LevelSession::updateBiene(LevelEntity &entity,
                               const WorldCollisionView &world) {
    const bool blocked = enemyMapBlocked(entity, world);
    entity.mapBlocked = blocked ? 1 : 0;
    if (blocked) {
        entity.velocityX.raw = Fixed16::wrapNegRaw(entity.velocityX.raw);
    }
    entity.positionX.raw = Fixed16::wrapAddRaw(entity.positionX.raw,
                                                entity.velocityX.raw);
    entity.x = entity.positionX.floorPixels();

    // 68C0's state-1 sine/exit phases are identified in the static closure,
    // but their selected phase data is not part of the W1L1 runtime fixture.
    // State zero's horizontal path is closed and remains exact here.
}

bool LevelSession::enemyMapBlocked(const LevelEntity &entity,
                                   const WorldCollisionView &world) const {
    const std::int32_t direction = entity.velocityX.raw < 0 ? -1 : 1;
    const std::int32_t probeX = entity.x + direction * 0x26;
    if (isWurm2Type(entity.type)) {
        // 6DC4's outer checks use the -10/-10 WURM2 offsets and the
        // direction-dependent +/-0x26 x probes. 1C6E's raw 0x4000 test is
        // closed; the neighboring 1C4D return polarity remains address-
        // qualified and is not substituted with a player descriptor test.
        return world.mapRawBit4000Confirmed(probeX, entity.y - 10) ||
               world.mapRawBit4000Confirmed(probeX, entity.y + 10);
    }

    // 68C0 uses the BIENE -20/-10 outer y offsets with the same directional
    // x probe pair.
    return world.mapRawBit4000Confirmed(probeX, entity.y - 20) ||
           world.mapRawBit4000Confirmed(probeX, entity.y - 10);
}

void LevelSession::beginEnemyContact(LevelEntity &entity) {
    entity.enemyContactPending = true;
    entity.contactCallback = CallbackIdentity(
        0x01f7, 0x4ab3, "enemy_contact_4ab3");
    entity.responseTimer = 0x28;
    enqueueEvent(LevelEventType::EntityCollisionImpact, entity.id,
                 entity.type);
}

void LevelSession::advanceEnemyResponse(Simulation *simulation,
                                        LevelEntity &entity) {
    if (entity.responseTimer != 0) {
        --entity.responseTimer;
    }
    if (entity.responseTimer != 0) {
        return;
    }

    // 4C5D clears only the response object's callback/object-active word;
    // it is not the normal enemy death/drop path. Suppress this streamed
    // declaration until a level reload reconstructs it.
    entity.enemyContactPending = false;
    entity.active = false;
    entity.phase = EntityPhase::Dormant;
    entity.streamSuppressed = true;
    releaseScheduledEntity(
        simulation == 0 ? 0 : &simulation->stateForSetup().scheduler,
        entity);
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
        if (isLeafType(entity.type)) {
            if (!entity.ambientRuntimeInitialized) {
                initializeAmbientVisualRuntime(entity);
            }

            // 01F7:47E7 normal path: the opaque visibility/contact helpers
            // leave carry clear for ordinary W1L1 leaf frames, then the
            // callback applies the bounded fixed-point fall before 01F7:5D60
            // advances the animation descriptor.
            std::int32_t velocity = entity.ambientVelocityY.raw;
            if (velocity > 0x4000) {
                velocity = Fixed16::wrapSubRaw(velocity, 300);
            }
            entity.positionY.raw = Fixed16::wrapAddRaw(
                entity.positionY.raw, velocity);
            entity.ambientVelocityY.raw = velocity;
            entity.y = entity.positionY.floorPixels();

            // 01F7:5D60 decrements +0x20 while nonzero. When it reaches zero
            // it advances the +0x24 cursor through the table's signed -8
            // loop marker, publishes the selected slot at +0x12, and reloads
            // +0x20 from +0x1E.
            if (entity.ambientAnimationDelay != 0) {
                --entity.ambientAnimationDelay;
            } else {
                const std::uint16_t firstCursor =
                    entity.ambientTable == 0 ? 0x3314 : 0x3328;
                const std::uint16_t loopCursor =
                    entity.ambientTable == 0 ? 0x3324 : 0x3338;
                std::uint16_t cursor = static_cast<std::uint16_t>(
                    entity.ambientAnimationCursor + 2);
                if (cursor == loopCursor) {
                    cursor = firstCursor;
                }
                entity.ambientAnimationCursor = cursor;
                const std::uint16_t frame = static_cast<std::uint16_t>(
                    (cursor - firstCursor) / 2);
                static const std::uint16_t table0[8] =
                    {700, 701, 702, 703, 704, 705, 706, 707};
                static const std::uint16_t table1[8] =
                    {703, 704, 705, 706, 707, 700, 701, 702};
                entity.spriteSlot = entity.ambientTable == 0
                    ? table0[frame]
                    : table1[frame];
                entity.ambientAnimationDelay =
                    entity.ambientTable == 0 ? 8 : 10;
            }
            continue;
        }
        if (!isNormalEnemyType(entity.type) && !isWorldEffectType(entity.type) &&
            !isCloudType(entity.type) &&
            !isDedicatedEventType(entity.type)) {
            entity.animationFrame = static_cast<std::uint16_t>(
                (entity.animationFrame + 1) & 0x00ff);
        }
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
    effect.spriteSlot = 0xffff;
    effect.updateCallback = CallbackIdentity(
        0x01f7, 0x10b5, "dedicated_event_10b5");
    effect.eventSubtype = entity.type == 0x65
        ? 0x00 : (entity.type == 0x66 ? 0x08 : 0x10);
    // 01F7:1749 stores the source event's animation byte from the PRNG
    // helper. These are the confirmed first W1L1 values; other world seeds
    // remain data-dependent and are intentionally not synthesized here.
    if (worldForMap(_mapName) == "W1") {
        effect.eventAnimationState = entity.type == 0x66 ? 4 : 1;
    }
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
        effect.effectResource = entity.effectResource;
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
    const std::int32_t streamX = _streamAnchorActive
        ? _streamAnchorX : player.positionX.floorPixels();
    const std::int32_t streamY = _streamAnchorActive
        ? _streamAnchorY : player.positionY.floorPixels();
    bool spawnedTransient = updateStreaming(
        simulation, streamX, streamY);
    // 01D7/0E96 dispatches platform callbacks before the later player pass;
    // A0B2's carry globals must therefore be published before 3FF8 runs.
    std::vector<SimulationCallbackStep> dependencyOrder;
    dispatchMovingPlatformCallbacks(&simulation, world, player,
                                    dependencyOrder);
    simulation.tick(input, world, output);
    if (simulation.playerUpdater() != 0) {
        dependencyOrder.push_back(SimulationCallbackStep(
            SimulationCallbackPhase::PlayerUpdate, 0,
            CallbackIdentity(0x01f7, 0x3ff8, "update_player_3ff8")));
    }
    output.playerDependencyOrder = dependencyOrder;
    syncPlayerTimer(player);
    spawnedTransient = updateStreaming(
        simulation,
        _streamAnchorActive ? _streamAnchorX : player.positionX.floorPixels(),
        _streamAnchorActive ? _streamAnchorY : player.positionY.floorPixels()) ||
        spawnedTransient;
    advanceActiveEntities();
    advanceActiveEffects();
    dispatchCloudCallbacks(&simulation, player);
    if (_gameplayState.cloudSignal89e6 == 0xffff &&
        _gameplayState.transitionGate89ea == 0) {
        const std::string target = completedGoalTarget(
            _mapName, _gameplayState.puzzleMask60d8);
        if (!target.empty()) {
            // 89EA is the outer transition gate consumed by 5010. Latching
            // it here prevents a second LevelExit while the frontend performs
            // the reload closure.
            _gameplayState.transitionGate89ea = 0xffff;
            enqueueEvent(LevelEventType::LevelExit, 0, 0, target);
        }
    }
    const bool emittedTileEffect = dispatchWorldEffectCallbacks(&simulation);

    if (spawnedTransient) {
        enqueueEvent(LevelEventType::WorldObjectInteraction);
    }
    if (emittedTileEffect) {
        enqueueEvent(LevelEventType::TileInteraction);
    }

    if (alternatePressed) {
        enqueueEvent(LevelEventType::AlternateActionObject);
    }

    dispatchEnemyCallbacks(&simulation, world, player);
    dispatchCollectibleCallbacks(&simulation, player);

    for (std::size_t index = 0; index < _entities.size(); ++index) {
        LevelEntity &entity = _entities[index];
        if (!entity.active || entity.collected) {
            continue;
        }
        if (isNormalEnemyType(entity.type)) {
            // Normal WURM2/BIENE contact is handled by the recovered
            // 4AB3/4C5D response boundary above, not by the provisional
            // player-death hazard path.
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
                updateStreaming(
                    simulation,
                    _streamAnchorActive ? _streamAnchorX : player.positionX.floorPixels(),
                    _streamAnchorActive ? _streamAnchorY : player.positionY.floorPixels());
                output.player = player;
                output.player.syncToRaw();
                return;
            }
        } else if (entity.kind == EntityKind::Hazard &&
                   overlaps(player, entity, _config.hazardRadius)) {
            resetPlayer(simulation);
            ++_deaths;
            enqueueEvent(LevelEventType::PlayerDied, entity.id, entity.type);
            updateStreaming(
                simulation,
                _streamAnchorActive ? _streamAnchorX : player.positionX.floorPixels(),
                _streamAnchorActive ? _streamAnchorY : player.positionY.floorPixels());
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
