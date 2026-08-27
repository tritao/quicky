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

std::int16_t addSignedWord(std::int16_t left, std::int16_t right) {
    return static_cast<std::int16_t>(
        static_cast<std::uint16_t>(left) +
        static_cast<std::uint16_t>(right));
}

bool signedWordSubtractionIsNegative(std::int16_t left,
                                     std::int16_t right) {
    // 01F7:1A7D is SUB AX,BX followed by JS. Test the sign bit of the
    // wrapped 16-bit result so the C++ model does not depend on signed
    // overflow or implementation-defined promotion behavior.
    const std::uint16_t result = static_cast<std::uint16_t>(
        static_cast<std::uint16_t>(left) -
        static_cast<std::uint16_t>(right));
    return (result & 0x8000U) != 0;
}

std::int16_t fixedHighWord(std::int32_t raw) {
    // ES:[DI+04]/ES:[DI+08] are the signed high words of the fixed-point
    // positions. This is an arithmetic shift, not host-language division.
    return static_cast<std::int16_t>(static_cast<std::uint16_t>(
        static_cast<std::uint32_t>(raw) >> 16));
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
      spawnRows8828(),
      terminalX8828(0),
      terminalY882a(0),
      sharedTargetActiveCount8806(0),
      sharedTargetCapacity8808(4),
      sharedTargetRows87de(),
      cloudSignal89e6(0),
      transitionGate89ea(0),
      transitionEffectBits8950(0),
      platformLatch5006(0),
      platformCarryX8816(0),
      platformCarryY8812(0) {
}

LevelSessionConfig::LevelSessionConfig()
    : streamRadiusRegions(1),
      hazardRadius(12),
      edgeExitMargin(24),
      enableEdgeExit(true),
      hasSpawn(false),
      spawnX(100),
      spawnY(100),
      hasLeafPrngState(false),
      leafPrngIndex(0),
      leafPrngRing(),
      hasBieneRuntimeTable(false),
      bieneRuntimeTable() {
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
            entity.y += normalEnemyYOffset(entity.type);
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
        initializeBump(entity);
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
    if (upperAscii(_mapName) == "W1L1.MAP") {
        // Native W1L1 selector startup publishes three health units; the
        // generic state default remains five for levels without a closed
        // setup record.
        _gameplayState.currentHealth8822 = 3;
        _gameplayState.maximumHealth8824 = 3;
    }
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
        entity.y = entity.initialY + normalEnemyYOffset(entity.type);
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
        initializeBump(entity);
    }
    _effects.clear();
    resetPlayer(simulation);
}

void LevelSession::restorePersistentStateForReload(
    const LevelGameplayState &previousState, std::uint32_t previousDeaths) {
    // The native reload trace shows DS:881C/881E, DS:880A, DS:8822, and
    // DS:8824 surviving 5010->1AAA. DS:60D8, DS:880C, DS:8810, DS:612E,
    // and the transition/contact gates are rebuilt or cleared by the new
    // level setup and must not be copied across this boundary.
    _score = previousState.score881c;
    _deaths = previousDeaths;
    _gameplayState.score881c = previousState.score881c;
    _gameplayState.lives880a = previousState.lives880a;
    _gameplayState.currentHealth8822 = previousState.currentHealth8822;
    _gameplayState.maximumHealth8824 = previousState.maximumHealth8824;
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
    return (type >= 0x01 && type <= 0x0c) ||
           (type >= 0x15 && type <= 0x1c);
}

bool LevelSession::isWurm2Type(std::uint16_t type) {
    return type == 0x01 || type == 0x02;
}

bool LevelSession::isBieneType(std::uint16_t type) {
    return type == 0x03 || type == 0x04;
}

std::int32_t LevelSession::normalEnemyYOffset(std::uint16_t type) {
    // 01F7:7AE3 is the only normal-enemy initializer in this closure that
    // uses +0x10. The other family initializers use +0x20.
    if (!isNormalEnemyType(type)) {
        return 0;
    }
    return type == 0x05 || type == 0x06 ? 0x10 : 0x20;
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
        entity.enemyOrientation = -1;
        entity.enemyPatrolDirection = -1;
        entity.enemyTransitionTimer = 0;
        entity.enemyPhase34 = 0;
        entity.enemySineOrProbe39 = 0;
        entity.enemyVerticalState36 = 0;
        entity.enemyTransitionState3d = 0;
        entity.enemySourceOrKind2c = -1;
        entity.enemyAux3e = 0;
        entity.enemyVerticalOffset40 = 0;
        entity.enemyOriginY36 = 0;
        entity.enemySavedVelocity3a = 0;
        entity.enemySavedDirection44 = 0;
        entity.mapBlocked = 0;
        entity.enemyAnimationDelay = 0;
        entity.enemyAnimationSequence = 0;
        return;
    }

    const bool rightVariant = (entity.type & 1U) == 0;
    const std::int32_t direction = rightVariant ? 1 : -1;
    entity.velocityX = Fixed16();
    entity.velocityY = Fixed16();
    entity.enemyPhaseTimer = 0;
    entity.enemyTimer = 0x14;
    entity.enemyState = 0;
    entity.enemyOrientation = static_cast<std::int8_t>(direction);
    entity.enemyPatrolDirection = static_cast<std::int8_t>(direction);
    entity.enemyTransitionTimer = 0;
    entity.enemyPhase34 = 0;
    entity.enemySineOrProbe39 = 0;
    entity.enemyVerticalState36 = 0;
    entity.enemyTransitionState3d = 0;
    entity.enemySourceOrKind2c = -1;
    entity.enemyAux3e = 0;
    entity.enemyVerticalOffset40 = 0;
    entity.enemyOriginY36 = entity.positionY.raw;
    entity.enemySavedVelocity3a = 0;
    entity.enemySavedDirection44 = 0;
    entity.mapBlocked = 0;
    entity.enemyAnimationDelay = 0x0e;
    entity.enemyAnimationSequence = 0;
    switch (entity.type) {
    case 0x01:
    case 0x02:
        entity.velocityX = Fixed16(direction * 0x15000);
        entity.enemyAnimationSequence = 0x33ee;
        break;
    case 0x03:
    case 0x04:
        entity.velocityX = Fixed16(direction * 0x15000);
        entity.enemyOriginY36 = entity.positionY.raw;
        break;
    case 0x05:
    case 0x06:
        entity.velocityX = Fixed16(direction * 0x7000);
        entity.enemyPhaseTimer = 0;
        entity.enemySineOrProbe39 = 0x14;
        entity.enemyTransitionState3d = 0;
        entity.enemyTimer = 0x19;
        break;
    case 0x07:
    case 0x08:
        entity.velocityX = Fixed16(direction * 0x5000);
        entity.enemySineOrProbe39 = 0x14;
        break;
    case 0x09:
    case 0x0a:
        entity.velocityX = Fixed16(direction * 0x6000);
        entity.velocityY = Fixed16(-0x30000);
        entity.enemyTransitionState3d = 0;
        break;
    case 0x0b:
    case 0x0c:
        entity.velocityX = Fixed16(direction * 0x15000);
        entity.enemyPhaseTimer = 0x14;
        break;
    case 0x15:
    case 0x16:
        entity.velocityX = Fixed16(direction * 0x17000);
        entity.enemyAnimationDelay = 8;
        break;
    case 0x17:
    case 0x18:
        entity.velocityX = Fixed16(direction * 0x12000);
        entity.enemyAnimationDelay = 16;
        break;
    case 0x19:
    case 0x1a:
        entity.velocityX = Fixed16(direction * 0x15000);
        entity.enemyAnimationDelay = 8;
        break;
    case 0x1b:
    case 0x1c:
        entity.velocityX = Fixed16(direction * 0x15000);
        entity.enemyAnimationDelay = 6;
        break;
    default:
        break;
    }
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

std::uint8_t LevelSession::nextSharedPrngByte() {
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
    const std::int8_t tableSeed = static_cast<std::int8_t>(nextSharedPrngByte());
    entity.ambientTable = tableSeed > 0 ? 0 : 1;
    entity.ambientAnimationDelay = entity.ambientTable == 0 ? 8 : 10;
    entity.ambientAnimationCursor = entity.ambientTable == 0
        ? 0x3314 : 0x3328;
    entity.spriteSlot = entity.ambientTable == 0 ? 700 : 703;

    const std::int8_t velocitySeed =
        static_cast<std::int8_t>(nextSharedPrngByte());
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
    entity.platformDirectionY4c = -1;
    entity.platformEdgeLatch50 = -1;
    entity.platformDirectionX4e = 1;
    entity.platformHorizontal4a = false;
    entity.platformAxisMarker4b = 0;
    entity.platformMotionGate59 = true;
    entity.platformInitializerMapChecked = false;
    entity.platformCarryActive = false;
    if (entity.kind != EntityKind::MovingPlatform) {
        return;
    }

    // 01F7:9C70 clears both fixed-point velocities, initializes +4C=-1,
    // +4E=1, +50=-1, +59=1, and leaves +52=0 until the type wrapper writes
    // 0x14. The 5DA1 word probe can clear +59 when the initializer cell has
    // raw bit 0x0200; it is deferred until the first callback because the
    // constructor does not own a WorldCollisionView.
    //
    // 9CF5/9D19 select the horizontal variants (3D/3E), while 9D5E/9D82
    // select the vertical variants (3F/40). +4B is retained as an exact
    // byte-shaped initializer field even though 9DC7 branches on +4A and
    // uses +4C/+4E as the live movement directions.
    entity.platformHorizontal4a = entity.type == 0x3d || entity.type == 0x3e;
    entity.platformAxisMarker4b = entity.platformHorizontal4a ? 0 : 0xff;
    entity.platformWait52 = 0x14;
    entity.velocityX = Fixed16();
    entity.velocityY = Fixed16();
}

void LevelSession::initializeBump(LevelEntity &entity) {
    entity.bumpAnimationDelay20 = 0;
    entity.bumpAnimationCursor24 = 0;
    if (entity.type != 0x34) {
        return;
    }

    // 01F7:9BEE performs one word ADD +0x10 at object+0x04 and two
    // successive word ADD +0x10 operations at object+0x08. The ARE anchor is
    // retained in initialX/initialY so re-streaming reconstructs the pooled
    // object rather than carrying a culled position forward.
    const auto addObjectWord = [](std::int32_t value,
                                  std::uint16_t amount) {
        // 01F7:9BEE updates the pooled object with 16-bit ADD instructions.
        // Keep the word wrap/sign conversion visible instead of allowing a
        // host-width coordinate add to change the boundary behavior.
        return static_cast<std::int32_t>(static_cast<std::int16_t>(
            static_cast<std::uint16_t>(value) + amount));
    };
    entity.x = addObjectWord(entity.initialX, 0x10);
    entity.y = addObjectWord(entity.initialY, 0x20);
    entity.positionX = Fixed16::fromPixels(entity.x);
    entity.positionY = Fixed16::fromPixels(entity.y);
    entity.bumpAnimationDelay20 = 6;
    entity.bumpAnimationCursor24 = 0;
    entity.animationFrame = 0;
    entity.spriteSlot = 400;
}

CallbackIdentity LevelSession::callbackFor(std::uint16_t type) {
    // These identities are the recovered ARE callback targets. The normal
    // enemy family is deliberately explicit: the callback address is part of
    // the replay contract even when a presentation-only tail remains opaque.
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
    if (type == 0x05 || type == 0x06) {
        return CallbackIdentity(0x01f7, 0x7b71, "entity_fisch_7b71");
    }
    if (type == 0x07 || type == 0x08) {
        return CallbackIdentity(0x01f7, 0x778c, "entity_krabbe_778c");
    }
    if (type == 0x09 || type == 0x0a) {
        return CallbackIdentity(0x01f7, 0x715e, "entity_pengo_715e");
    }
    if (type == 0x0b || type == 0x0c) {
        return CallbackIdentity(0x01f7, 0x66e1, "entity_schnee_66e1");
    }
    if (type == 0x15 || type == 0x16) {
        return CallbackIdentity(0x01f7, 0x7ef8, "entity_fliege_7ef8");
    }
    if (type == 0x17 || type == 0x18) {
        return CallbackIdentity(0x01f7, 0x8472, "entity_spinne_8472");
    }
    if (type == 0x19 || type == 0x1a) {
        return CallbackIdentity(0x01f7, 0x5071, "entity_buggy_5071");
    }
    if (type == 0x1b || type == 0x1c) {
        return CallbackIdentity(0x01f7, 0x5f28, "entity_ufo_5f28");
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

bool LevelSession::collectibleOverlaps(const PlayerRecord &player,
                                       const LevelEntity &entity) const {
    // 01F7:393C returns zero bounds while DS:89EA is nonzero.  01F7:8D31
    // consumes the returned AX/CX/BX/DX values with signed strict compares;
    // this is deliberately not the broad radius predicate used by hazards.
    if (_gameplayState.transitionGate89ea != 0) {
        return false;
    }

    const std::int32_t playerX = player.positionX.floorPixels();
    const std::int32_t playerY = player.positionY.floorPixels();
    const std::int32_t left = playerX + player.state2C;
    const std::int32_t right = playerX + player.state30;
    const std::int32_t top = playerY +
        static_cast<std::int16_t>(player.callbackState32);
    const std::int32_t bottom = playerY + player.verticalStepOrDirection2E;

    // 8D31 reads the object's integer X/Y words.  Its Y path clears the low
    // four bits of the signed 16-bit integer word before adding 0x10.
    const std::int32_t objectX = static_cast<std::int16_t>(entity.x);
    const std::int32_t objectY = static_cast<std::int16_t>(entity.y) & ~0x0f;
    return objectX < right && objectX + 0x10 > left &&
           objectY < top && objectY + 0x10 > bottom;
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
            if (entity.type == 0x34) {
                // 01F7:1DEE clears the pooled callback off camera; a later
                // ARE scan re-enters 9BEE and reapplies the initializer word
                // shifts and descriptor start.
                initializeBump(entity);
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

        // 01F7:8D31 -> 393C uses the live player's four signed bounds and
        // strict comparisons against the pickup's 16x16 aligned box.
        if (!collectibleOverlaps(player, entity)) {
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

void LevelSession::applyCompletionPresentation(LevelEvent &event) {
    // 01D7:14E1 consumes DS:880C one unit at a time and adds 0x14 to the
    // score for each unit. The all-seven branch at 01D7:16C6 then adds
    // 0x7D0. The native W1L1 trace reaches 5010 with the aggregate already
    // applied: score 750 -> 2950, DS:880C 10 -> 0, and DS:612E -> 12.
    // The presentation frames themselves remain outside LevelSession; the
    // state writes are published on the exit event for replay/parity tools.
    const std::uint32_t scoreBefore = _gameplayState.score881c;
    const std::uint16_t cerealBefore = _gameplayState.ammo880c;
    const std::uint32_t scoreAfter =
        scoreBefore + static_cast<std::uint32_t>(cerealBefore) * 0x14u + 0x7d0u;
    _score = scoreAfter;
    _gameplayState.score881c = scoreAfter;
    appendStateWrite(event.stateWrites, 0x881c, 4, scoreBefore, scoreAfter);

    _gameplayState.ammo880c = 0;
    appendStateWrite(event.stateWrites, 0x880c, 2, cerealBefore, 0);

    const std::uint16_t actionBefore = _gameplayState.pendingEvent612e;
    _gameplayState.pendingEvent612e = 12;
    appendStateWrite(event.stateWrites, 0x612e, 2, actionBefore, 12);
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
    PlayerRecord &player,
    std::vector<SimulationCallbackStep> &dependencyOrder) {
    ObjectScheduler *scheduler = simulation == 0
        ? 0 : &simulation->stateForSetup().scheduler;
    for (std::size_t index = 0; index < _entities.size(); ++index) {
        LevelEntity &entity = _entities[index];
        if (!entity.active ||
            (!isNormalEnemyType(entity.type) && entity.type != 0x34) ||
            entity.updateCallback.offset == 0) {
            continue;
        }
        if (scheduler != 0 &&
            (!entity.schedulerHandle.valid() ||
             entity.schedulerHandle.slot >= scheduler->objects().size() ||
             (!scheduler->objects()[entity.schedulerHandle.slot].active &&
              !scheduler->objects()[entity.schedulerHandle.slot].pendingSpawn))) {
            continue;
        }

        // 01F7:0E06 gives normal ARE objects phase +0x17=1 and their
        // initializer installs the primary callback at +0x18. 01F7:1036
        // publishes that callback in the 01F7:0E96 phase list.  The player
        // object is phase 2, so this callback must precede 01F7:3FF8.
        dependencyOrder.push_back(SimulationCallbackStep(
            SimulationCallbackPhase::GameplayObjectBeforePlayer,
            entity.id, entity.updateCallback));

        if (entity.type == 0x34) {
            updateBump(simulation, entity, player);
            continue;
        }

        if (entity.enemyContactPending) {
            advanceEnemyResponse(simulation, entity);
            continue;
        }

        // 01F7:6DC4 and 01F7:68C0 call 1B77 immediately after their
        // visibility gate, before the MAP probes and movement state machine.
        // The candidate position is therefore the pre-update object word.
        if (isWurm2Type(entity.type) || isBieneType(entity.type)) {
            std::vector<LevelStateWrite> writes;
            if (applyNormalEnemyDamage(entity, player, writes)) {
                LevelEvent event;
                event.type = LevelEventType::PlayerDamaged;
                event.entityId = entity.id;
                event.entityType = entity.type;
                event.stateWrites = writes;
                _events.push_back(event);
            }
        }

        if (isWurm2Type(entity.type)) {
            updateWurm2(entity, world);
        } else if (isBieneType(entity.type)) {
            updateBiene(entity, world, player);
        } else {
            updateNormalEnemy(entity, world);
        }

        // The family callbacks publish the same animation delay field as the
        // initializers. The BOB cursor tables remain renderer-owned; this
        // counter is retained here so the callback-visible timing is not
        // replaced with a generic frame counter.  The delay is a family
        // contract recovered from the corresponding loader, not a universal
        // enemy constant.
        const std::uint16_t animationDelay =
            entity.type == 0x15 || entity.type == 0x16 ||
            entity.type == 0x19 || entity.type == 0x1a ? 8 :
            (entity.type == 0x17 || entity.type == 0x18 ? 16 :
             (entity.type == 0x1b || entity.type == 0x1c ? 6 : 14));
        if (entity.enemyAnimationDelay != 0) {
            --entity.enemyAnimationDelay;
        } else {
            entity.enemyAnimationDelay = animationDelay;
            entity.animationFrame = static_cast<std::uint16_t>(
                (entity.animationFrame + 1) & 0x00ff);
        }

        // The native contact tail is a separate 4AB3/4C5D object response for
        // the remaining normal-enemy families. WURM2/BIENE already executed
        // their 1B77 damage route above and do not enter this tail.
        if (!isWurm2Type(entity.type) && !isBieneType(entity.type) &&
            overlaps(player, entity, _config.hazardRadius)) {
            beginEnemyContact(entity);
        }
    }
}

void LevelSession::updateBump(Simulation *simulation, LevelEntity &entity,
                              PlayerRecord &player) {
    // 01F7:9C0C: the callback's DS:85DA gate is a byte comparison. Values
    // below 0x32 execute visibility/animation advancement; the >=0x32 path
    // skips that prefix but continues into the same proximity test.
    const std::int16_t activation = simulation != 0 &&
        simulation->playerUpdater() != 0
        ? simulation->playerUpdater()->activationState85DA() : 0;
    const bool runCameraAnimationPrefix =
        static_cast<std::uint8_t>(activation) < 0x32;

    if (runCameraAnimationPrefix) {
        // The enclosing stream pass already applies 1DCA/1DEE.  5D60's
        // object-local contract is retained exactly: decrement +0x20 first,
        // then advance the cursor when the next callback observes zero.
        if (entity.bumpAnimationDelay20 != 0) {
            --entity.bumpAnimationDelay20;
        } else {
            static const std::uint16_t kBumpSlots[4] =
                {400, 402, 403, 401};
            entity.bumpAnimationCursor24 = static_cast<std::uint16_t>(
                (entity.bumpAnimationCursor24 + 1) & 3U);
            entity.spriteSlot =
                kBumpSlots[entity.bumpAnimationCursor24];
            entity.animationFrame = entity.bumpAnimationCursor24;
            entity.bumpAnimationDelay20 = 6;
        }
    }

    player.syncToRaw();
    // 01F7:39FE returns the signed high words of player +0x04/+0x08 and the
    // signed byte at +0x37. The strict comparisons below preserve the raw
    // 16-bit object/player coordinate convention.
    const std::int16_t playerX = player.xPixel();
    const std::int16_t playerY = player.yPixel();
    const std::int8_t playerMode = player.mode37;
    if (playerMode <= 0) {
        return;
    }

    const std::int16_t objectX = static_cast<std::int16_t>(entity.x);
    const std::int16_t objectY = static_cast<std::int16_t>(entity.y);
    if (!(addSignedWord(objectX, static_cast<std::int16_t>(-0x19)) < playerX &&
          playerX < addSignedWord(objectX, 0x19) &&
          addSignedWord(objectY, static_cast<std::int16_t>(-0x08)) < playerY &&
          playerY < objectY)) {
        return;
    }

    // 01F7:9C57 reloads the BUMP object descriptor from DS:3568 before the
    // player response. The table contents are a resource boundary, but the
    // observed logical start is closed and remains explicit here.
    entity.bumpAnimationDelay20 = 6;
    entity.bumpAnimationCursor24 = 0;
    entity.animationFrame = 0;
    entity.spriteSlot = 400;

    // 01F7:1B07, reached by 1B5D, resolves DS:881A to this persistent player.
    // Preserve the mask/write order and the exact typed player offsets.
    const std::uint16_t effectBitsBefore =
        _gameplayState.transitionEffectBits8950;
    const std::uint16_t effectBitsAfter = static_cast<std::uint16_t>(
        effectBitsBefore & 0xffcfU);
    _gameplayState.transitionEffectBits8950 = effectBitsAfter;

    player.sideResponse3B = 0;
    player.resetDeathTimer3E = 0x03e8;
    player.mode37 = static_cast<std::int8_t>(0xff);
    player.verticalResponse3A = 0;
    player.contactScratch2B = 0xff;

    // 1B07 copies player+0x64 to +0x0E; 1B5D then subtracts wrapping dword
    // 0x1B000. Do not express this as a host signed subtraction.
    player.velocityY.raw = Fixed16::wrapSubRaw(
        player.negativeYSpeed64.raw, 0x0001b000);
    if ((effectBitsAfter & 0x0004U) == 0) {
        // 5D38(0x3160): delay 8, first frame 10, cursor 0x3162. The
        // direction byte selects the mirrored frame family by +0x32.
        player.field1E = 8;
        player.animationDelay20 = 8;
        player.animationCursor22 = 0x3162;
        player.field24 = 0x3162;
        const std::int16_t frame = player.directionByte28 == 0xff
            ? static_cast<std::int16_t>(10 + 0x32) : 10;
        player.statusWord12 = static_cast<std::uint16_t>(frame);
    }

    const std::uint16_t pendingBefore = _gameplayState.pendingEvent612e;
    _gameplayState.pendingEvent612e = 4;

    LevelEvent event;
    event.type = LevelEventType::EntityCollisionImpact;
    event.entityId = entity.id;
    event.entityType = entity.type;
    appendStateWrite(event.stateWrites, 0x8950, 2,
                     effectBitsBefore, effectBitsAfter);
    appendStateWrite(event.stateWrites, 0x612e, 2,
                     pendingBefore, _gameplayState.pendingEvent612e);
    _events.push_back(event);
}

bool LevelSession::applyNormalEnemyDamage(
    const LevelEntity &entity, PlayerRecord &player,
    std::vector<LevelStateWrite> &writes) {
    // 01F7:393C returns zero bounds while DS:89EA is nonzero, but 1B77 still
    // performs its signed comparisons. Preserve that exact order instead of
    // turning the gate into a higher-level early return.
    player.syncToRaw();
    const std::int16_t playerX = player.xPixel();
    const std::int16_t playerY = player.yPixel();
    const std::int16_t boundsAX = _gameplayState.transitionGate89ea != 0
        ? 0 : addSignedWord(playerX, player.state2C);
    const std::int16_t boundsBX = _gameplayState.transitionGate89ea != 0
        ? 0 : addSignedWord(playerY, player.verticalStepOrDirection2E);
    const std::int16_t boundsCX = _gameplayState.transitionGate89ea != 0
        ? 0 : addSignedWord(playerX, player.state30);
    const std::int16_t boundsDX = _gameplayState.transitionGate89ea != 0
        ? 0 : addSignedWord(
            playerY, static_cast<std::int16_t>(player.callbackState32));

    const std::int16_t candidateX = static_cast<std::int16_t>(entity.x);
    const std::int16_t candidateY = static_cast<std::int16_t>(entity.y);
    const std::int16_t xOffset = -10;
    const std::int16_t yOffset = isBieneType(entity.type) ? -20 : -10;
    const std::int16_t xExtent = 20;
    const std::int16_t yExtent = 20;

    std::int16_t edge = addSignedWord(candidateX, xOffset);
    if (edge >= boundsCX) {
        return false;
    }
    edge = addSignedWord(edge, xExtent);
    if (edge <= boundsAX) {
        return false;
    }

    edge = addSignedWord(candidateY, yOffset);
    if (edge >= boundsDX) {
        return false;
    }
    edge = addSignedWord(edge, yExtent);
    if (edge <= boundsBX) {
        return false;
    }

    // 01F7:1BB5-1BBF returns AX=1 and does not call 19E6 while DS:8810 is
    // nonzero. The caller ignores AX, but the absence of writes is part of
    // the observable contract.
    if (_gameplayState.invulnerabilityGate8810 != 0) {
        return false;
    }

    // 01F7:19E6, 19EB-1A95. The player timer is the first gate inside the
    // damage writer; 1B77 still reports a contact, but no state is changed.
    if (player.timer34 != 0) {
        return false;
    }

    const std::uint16_t pendingBefore = _gameplayState.pendingEvent612e;
    _gameplayState.pendingEvent612e = 1;
    appendStateWrite(writes, 0x612e, 2, pendingBefore, 1);

    const std::uint16_t healthBefore = _gameplayState.currentHealth8822;
    const std::uint16_t healthAfter = static_cast<std::uint16_t>(
        healthBefore - 1);
    _gameplayState.currentHealth8822 = healthAfter;
    appendStateWrite(writes, 0x8822, 2, healthBefore, healthAfter);

    if (healthAfter != 0) {
        // 01F7:1A73 is the complete nonterminal player write.
        player.timer34 = 0x00d2;
    } else {
        // 01F7:1A06-1A6B. The effect/IRQ-driven recovery consumer remains
        // outside this native boundary; these are only the writes made by
        // 19E6 itself.
        const std::uint16_t livesBefore = _gameplayState.lives880a;
        const std::uint16_t livesAfter = static_cast<std::uint16_t>(
            livesBefore - 1);
        _gameplayState.lives880a = livesAfter;
        appendStateWrite(writes, 0x880a, 2, livesBefore, livesAfter);

        const std::uint16_t effectBitsBefore =
            _gameplayState.transitionEffectBits8950;
        const std::uint16_t effectBitsAfter = static_cast<std::uint16_t>(
            effectBitsBefore & 0xffcfU);
        _gameplayState.transitionEffectBits8950 = effectBitsAfter;
        appendStateWrite(writes, 0x8950, 2, effectBitsBefore, effectBitsAfter);

        player.velocityY.raw = static_cast<std::int32_t>(0xfffe0000U);
        player.acceleration4C.raw = 0x00002000;
        player.positiveYAcceleration50.raw = 0x00002000;
        player.horizontalSpeedCap5C.raw = 0x00018000;
        player.positiveYSpeedCap60.raw = 0x00040000;

        const std::uint16_t gateBefore = _gameplayState.transitionGate89ea;
        _gameplayState.transitionGate89ea = 0xffff;
        appendStateWrite(writes, 0x89ea, 2, gateBefore, 0xffff);

        player.velocityY.raw = static_cast<std::int32_t>(0xfffe0000U);
        player.sideResponse3B = 0;
        player.resetDeathTimer3E = 0x03e8;
        player.mode37 = -1;
        player.verticalResponse3A = 0;
        player.contactScratch2B = 0;
    }

    // 01F7:1A79-1A8C: SUB player+0x04,BX followed by JS. BX is the
    // candidate object's integer X word, not the post-update position.
    player.velocityX.raw = signedWordSubtractionIsNegative(
        playerX, candidateX) ? static_cast<std::int32_t>(0xfffe8000U)
                             : 0x00018000;
    player.syncToRaw();
    return true;
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

void LevelSession::publishMovingPlatformCarry(
    LevelEntity &entity, PlayerRecord &player, Simulation *simulation) {
    // 01F7:A0B2 consumes DS:5006, not the pre-motion overlap result. The X
    // subtraction is a 16-bit object-word subtraction before the fixed-point
    // shift; preserve that wrap even when the platform reverses.
    if (_gameplayState.platformLatch5006 != 0) {
        const std::uint16_t deltaX = static_cast<std::uint16_t>(
            entity.x - entity.platformPreviousX);
        const std::int32_t carryX = Fixed16::wrapAddRaw(
            static_cast<std::int32_t>(static_cast<std::uint32_t>(deltaX) << 16),
            1);
        const std::int32_t carryY = Fixed16::wrapAddRaw(
            Fixed16::wrapSubRaw(entity.positionY.raw, player.positionY.raw), 1);
        _gameplayState.platformCarryX8816 = carryX;
        _gameplayState.platformCarryY8812 = carryY;
        entity.platformCarryActive = true; // object +0x5A = 0xff
        entity.platformMotionGate59 = false; // object +0x59 = 0
        if (simulation != 0 && simulation->playerUpdater() != 0) {
            simulation->playerUpdater()->publishPlatformCarry(carryX, carryY);
        }
        return;
    }

    if (entity.platformCarryActive) {
        // A0B2 clears +0x5A and starts the 0x14-tick recontact cooldown.
        entity.platformCarryActive = false;
        entity.platformCooldown58 = 0x14;
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

        if (!entity.platformInitializerMapChecked) {
            // 01F7:9C70 -> 5DA1 tests the initializer cell with TEST AH,0x02.
            // The constructor cannot access the collision view, so publish
            // this one-time +0x59 decision at the first callback.
            const MapCell initializerCell = world.cellAt(
                entity.x / 16, entity.y / 16);
            entity.platformMotionGate59 =
                !initializerCell.inBounds ||
                (initializerCell.rawWord & 0x0200U) == 0;
            entity.platformInitializerMapChecked = true;
        }

        if (entity.platformCooldown58 != 0) {
            --entity.platformCooldown58;
        } else if (player.mode37 >= 0) {
            const std::int32_t playerX = player.positionX.floorPixels();
            const std::int32_t playerY = player.positionY.floorPixels();
            // 01F7:A075: strict X interval, half-open 12-pixel Y band.
            if (entity.x < playerX && playerX < entity.x + entity.collisionWidth &&
                entity.y <= playerY && playerY < entity.y + 12) {
                _gameplayState.platformLatch5006 = 0xffff;
            }
        }

        // Object +0x2A/+0x2C are captured before the movement state machine.
        entity.platformPreviousX = entity.x;
        entity.platformPreviousY = entity.y;

        // The initializer's +0x59==1 branch jumps directly to A0B2.
        if (entity.platformMotionGate59) {
            publishMovingPlatformCarry(entity, player, simulation);
            continue;
        }

        const auto setIntegerX = [&entity](std::int32_t value) {
            entity.x = value;
            entity.positionX.raw = (entity.positionX.raw & 0x0000ffff) |
                                   (static_cast<std::int32_t>(value) << 16);
        };
        const auto setIntegerY = [&entity](std::int32_t value) {
            entity.y = value;
            entity.positionY.raw = (entity.positionY.raw & 0x0000ffff) |
                                   (static_cast<std::int32_t>(value) << 16);
        };

        if (entity.platformHorizontal4a) {
            if (entity.platformWait54 != 0) {
                --entity.platformWait54;
            } else if (entity.velocityX.raw != 0) {
                const std::int32_t oldX = entity.x;
                entity.positionX.raw = Fixed16::wrapAddRaw(
                    entity.positionX.raw, entity.velocityX.raw);
                entity.x = entity.positionX.floorPixels();
                if (((entity.x ^ oldX) & 0xfff0) != 0) {
                    const std::int32_t probeX =
                        (entity.x & ~0x0f) +
                        (entity.platformDirectionX4e == 1
                             ? entity.collisionWidth - 0x10 : 0x10);
                    if (world.mapRawBit0800Confirmed(probeX, entity.y)) {
                        entity.velocityX = Fixed16();
                        entity.platformWait54 = 0x46;
                        setIntegerX(entity.x & ~0x0f);
                        if (entity.platformDirectionX4e != 1) {
                            setIntegerX(entity.x + 0x10);
                        }
                    }
                }
            } else if (entity.platformWait52 != 0) {
                --entity.platformWait52;
            } else if (entity.platformEdgeLatch50 == -1) {
                entity.platformEdgeLatch50 = 0;
                const bool blockedAhead = world.mapRawBit0800Confirmed(
                    entity.x + entity.collisionWidth, entity.y);
                if (blockedAhead) {
                    entity.velocityX = Fixed16(-0x28000);
                    setIntegerX(entity.x - 1);
                    entity.platformDirectionX4e = -1;
                } else {
                    entity.velocityX = Fixed16(0x28000);
                    setIntegerX(entity.x + 1);
                    entity.platformDirectionX4e = 1;
                }
            } else {
                const bool blockedBehind = world.mapRawBit0800Confirmed(
                    entity.x - 0x10, entity.y);
                bool reverse = false;
                if (!blockedBehind) {
                    const bool blockedAhead = world.mapRawBit0800Confirmed(
                        entity.x + entity.collisionWidth, entity.y);
                    reverse = !blockedAhead && entity.platformDirectionX4e == -1;
                }
                if (reverse) {
                    entity.velocityX = Fixed16(-0x28000);
                    setIntegerX(entity.x - 1);
                    entity.platformDirectionX4e = -1;
                } else {
                    entity.velocityX = Fixed16(0x28000);
                    setIntegerX(entity.x + 1);
                    entity.platformDirectionX4e = 1;
                }
            }
        } else {
            if (entity.platformWait54 != 0) {
                --entity.platformWait54;
            } else if (entity.velocityY.raw != 0) {
                const std::int32_t oldY = entity.y;
                entity.positionY.raw = Fixed16::wrapAddRaw(
                    entity.positionY.raw, entity.velocityY.raw);
                entity.y = entity.positionY.floorPixels();
                if (((entity.y ^ oldY) & 0xfff0) != 0) {
                    const std::int32_t probeY =
                        (entity.y & ~0x0f) +
                        (entity.platformDirectionY4c == 1 ? 0x10 : 0);
                    if (world.mapRawBit0800Confirmed(entity.x, probeY)) {
                        entity.velocityY = Fixed16();
                        entity.platformWait54 = 0x46;
                        setIntegerY(entity.y & ~0x0f);
                        if (entity.platformDirectionY4c != 1) {
                            setIntegerY(entity.y + 0x10);
                        }
                    }
                }
            } else if (entity.platformWait52 != 0) {
                --entity.platformWait52;
            } else {
                const bool blockedUp = world.mapRawBit0800Confirmed(
                    entity.x, entity.y + 0x10);
                if (blockedUp) {
                    entity.velocityY = Fixed16(-0x28000);
                    setIntegerY(entity.y - 1);
                    entity.platformDirectionY4c = -1;
                } else {
                    const bool blockedDown = world.mapRawBit0800Confirmed(
                        entity.x, entity.y - 0x10);
                    if (!blockedDown && entity.platformDirectionY4c == 1) {
                        entity.velocityY = Fixed16(0x28000);
                        setIntegerY(entity.y + 1);
                        entity.platformDirectionY4c = 1;
                    } else {
                        entity.velocityY = Fixed16(-0x28000);
                        setIntegerY(entity.y - 1);
                        entity.platformDirectionY4c = -1;
                    }
                }
            }
        }

        publishMovingPlatformCarry(entity, player, simulation);
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
        // 01F7:8E4B stores words at row zero, DS:8828/DS:882A. Preserve the
        // 16-bit write width and keep the legacy scalar fields as aliases.
        const std::uint16_t afterX = static_cast<std::uint16_t>(
            entity.x + 0x19);
        const std::uint16_t afterY = static_cast<std::uint16_t>(
            entity.y + 0x46);
        _gameplayState.spawnRows8828[0] = SpawnCoordinateRow{afterX, afterY};
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
    // 01F7:6DC4 publishes +0x2F from 1C4D/5C27 before entering its signed
    // state machine. The player damage overlap was already evaluated by the
    // phase-1 caller immediately before this movement path.
    entity.mapBlocked = enemyMapBlocked(entity, world) ? 1 : 0;
    const auto clampVelocity = [](std::int32_t value) {
        return std::max<std::int32_t>(-0x15000,
               std::min<std::int32_t>(0x15000, value));
    };
    const auto advanceX = [&entity]() {
        entity.positionX.raw = Fixed16::wrapAddRaw(entity.positionX.raw,
                                                    entity.velocityX.raw);
        entity.x = entity.positionX.floorPixels();
    };
    const auto loadAnimation = [&entity](std::uint16_t sequence) {
        // 01F7:5D38 writes the sequence/cursor and reloads the object delay.
        // The native renderer owns the BOB cursor; retain the callback-visible
        // delay and selected sequence at the entity boundary.
        entity.enemyAnimationDelay = 0x0e;
        entity.enemyAnimationSequence = sequence;
        (void)sequence;
    };

    const std::int32_t orientation = entity.enemyOrientation;
    const std::int32_t patrolDirection = entity.enemyPatrolDirection;
    const auto finishCallback = [this, &entity]() {
        // 01F7:707B follows every state-machine exit, including early returns
        // from state 0, state 2, and the non-state-3 movement branch.
        consumeWurm2TargetTail(entity);
    };
    if (entity.enemyState < 1) {
        // 6E31-6E36: the normalized map result preserves the raw +0x2F
        // polarity. Positive means the 6E3A descriptor-contact response;
        // zero/nonpositive means the ordinary 6F16 integration path.
        if (static_cast<std::int8_t>(entity.mapBlocked) > 0) {
            if (patrolDirection < 0) {
                if (entity.enemyTimer == 0x14) {
                    loadAnimation(0x33ee);
                }
                entity.velocityX.raw = clampVelocity(
                    entity.velocityX.raw - (orientation << 12));
                advanceX();
                const std::uint16_t nextTimer = static_cast<std::uint16_t>(
                    entity.enemyTimer - 1);
                entity.enemyTimer = nextTimer;
                if (static_cast<std::int16_t>(nextTimer) < 0) {
                    entity.enemyOrientation = static_cast<std::int8_t>(
                        0 - entity.enemyOrientation);
                    entity.enemyPatrolDirection = static_cast<std::int8_t>(
                        0 - entity.enemyPatrolDirection);
                    entity.enemyTimer = 0x3c;
                    entity.velocityX.raw =
                        static_cast<std::int32_t>(entity.enemyOrientation) << 9;
                }
            } else {
                entity.velocityX.raw = clampVelocity(
                    entity.velocityX.raw + (orientation << 10));
                advanceX();
                const std::uint16_t nextTimer = static_cast<std::uint16_t>(
                    entity.enemyTimer - 1);
                entity.enemyTimer = nextTimer;
                if (static_cast<std::int16_t>(nextTimer) < 0) {
                    entity.enemyPatrolDirection = static_cast<std::int8_t>(
                        0 - entity.enemyPatrolDirection);
                    entity.mapBlocked = 0xff;
                    entity.enemyTimer = 0x14;
                }
            }
        } else {
            advanceX();
            entity.enemyPhaseTimer = static_cast<std::uint16_t>(
                entity.enemyPhaseTimer + 1);
            if (entity.enemyPhaseTimer > 0x96) {
                // 01F7:6DC4 uses the same DS:6468/646C byte ring as the
                // falling-leaf and dedicated-event initializers. The signed
                // byte is sign-extended, then logically shifted right once.
                const std::int16_t randomByte = static_cast<std::int8_t>(
                    nextSharedPrngByte());
                entity.enemyPhaseTimer = static_cast<std::uint16_t>(
                    static_cast<std::uint16_t>(randomByte) >> 1);
                entity.enemyState = 1;
            }
        }
        finishCallback();
        return;
    }

    if (static_cast<std::int8_t>(entity.mapBlocked) > 0) {
        entity.enemyState = 0;
        entity.enemyPhaseTimer = 0x78;
        finishCallback();
        return;
    }

    if (entity.enemyState == 2) {
        entity.enemyTransitionTimer = static_cast<std::uint16_t>(
            entity.enemyTransitionTimer + 1);
        if (entity.enemyTransitionTimer > 0x4b) {
            entity.enemyTransitionTimer = 0;
            entity.enemyState = 3;
            loadAnimation(0x33e2);
        } else {
            finishCallback();
            return;
        }
    } else if (entity.enemyState != 3) {
        loadAnimation(0x33e2);
        entity.velocityX.raw = clampVelocity(
            entity.velocityX.raw - (orientation << 11));
        advanceX();
        const bool movingInOrientation = entity.enemyOrientation <= 0
            ? entity.velocityX.raw < 0 : entity.velocityX.raw > 0;
        if (movingInOrientation) {
            finishCallback();
            return;
        }
        entity.velocityX.raw = 0;
        entity.enemyState = 2;
        loadAnimation(0x33fa);
        finishCallback();
        return;
    }

    entity.velocityX.raw = clampVelocity(
        entity.velocityX.raw + (orientation << 11));
    advanceX();
    if ((entity.enemyOrientation > 0 && entity.velocityX.raw >= 0x15000) ||
        (entity.enemyOrientation <= 0 && entity.velocityX.raw <= -0x15000)) {
        entity.enemyState = 0;
        loadAnimation(0x33ee);
    }
    finishCallback();
}

void LevelSession::consumeWurm2TargetTail(LevelEntity &entity) {
    // 01F7:707B-7080: the shared target scan is skipped when DS:8806 is
    // zero. The ring is four-byte rows at DS:87DE; its capacity is the word
    // at DS:8808 and the object cursor is +0x30.
    if (_gameplayState.sharedTargetActiveCount8806 == 0) {
        return;
    }

    std::uint16_t index = entity.targetCursor30;
    // 7086-7092 uses signed JL. Normal runtime cursors are nonnegative, but
    // retain the raw 16-bit comparison so an address-qualified negative value
    // also wraps to row zero instead of indexing the host container.
    if (static_cast<std::int16_t>(index) >= static_cast<std::int16_t>(
            _gameplayState.sharedTargetCapacity8808)) {
        entity.targetCursor30 = 0;
        index = 0;
    }
    const TargetCoordinateRow &target =
        _gameplayState.sharedTargetRows87de[index & 0x007fU];
    const std::int16_t objectX = static_cast<std::int16_t>(entity.x);
    const std::int16_t objectY = static_cast<std::int16_t>(entity.y);
    const std::int16_t targetX = static_cast<std::int16_t>(target.x);
    const std::int16_t targetY = static_cast<std::int16_t>(target.y);
    const std::int16_t xLower = static_cast<std::int16_t>(
        static_cast<std::uint16_t>(objectX - 0x19));
    const std::int16_t xUpper = static_cast<std::int16_t>(
        static_cast<std::uint16_t>(objectX + 0x19));
    const std::int16_t yUpper = static_cast<std::int16_t>(
        static_cast<std::uint16_t>(objectY + 5));
    const std::int16_t yLower = static_cast<std::int16_t>(
        static_cast<std::uint16_t>(objectY - 0x0f));

    // 709E-70C1: strict X window (object-0x19, object+0x19) and strict Y
    // window (object-0x0f, object+5), using the original JLE/JGE exits.
    if (targetX > xLower && targetX < xUpper &&
        targetY < yUpper && targetY > yLower) {
        // 70C3 clears only the row X word.  70C9 publishes the 4AB3 response
        // callback; its child/effect lifetime is intentionally not synthesized
        // here because that callback is a separate contract boundary.
        _gameplayState.sharedTargetRows87de[index & 0x007fU].x = 0;
        entity.contactCallback = CallbackIdentity(
            0x01f7, 0x4ab3, "contact_response_action_0d_4ab3");
    }
    // 70CF advances +0x30 even when the selected target does not overlap.
    entity.targetCursor30 = static_cast<std::uint16_t>(index + 1);
}

void LevelSession::updateBiene(LevelEntity &entity,
                               const WorldCollisionView &world,
                               const PlayerRecord &player) {
    const bool blocked = enemyMapBlocked(entity, world);
    entity.mapBlocked = blocked ? 1 : 0;

    const auto advanceX = [&entity]() {
        entity.positionX.raw = Fixed16::wrapAddRaw(entity.positionX.raw,
                                                    entity.velocityX.raw);
        entity.x = entity.positionX.floorPixels();
    };
    const auto advanceY = [&entity]() {
        entity.y = entity.positionY.floorPixels();
    };
    const auto clampWrapped = [](std::int32_t value,
                                 std::int32_t minimum,
                                 std::int32_t maximum) {
        return value < minimum ? minimum : (value > maximum ? maximum : value);
    };
    const auto loadAnimation = [&entity](std::uint16_t sequence) {
        // 01F7:5D38 publishes the callback-visible sequence and reloads the
        // object animation delay. The BOB cursor remains renderer-owned.
        entity.enemyAnimationDelay = 0x0e;
        entity.enemyAnimationSequence = sequence;
    };
    const auto replaceHighWord = [](std::int32_t raw, std::uint16_t high) {
        return static_cast<std::int32_t>(
            (static_cast<std::uint32_t>(raw) & 0x0000ffffU) |
            (static_cast<std::uint32_t>(high) << 16));
    };
    const auto transitionX = [&entity]() {
        return fixedHighWord(entity.positionX.raw);
    };
    const auto transitionY = [&entity]() {
        return fixedHighWord(entity.positionY.raw);
    };

    // 01F7:68F0-6A61. The state-zero path uses the same raw 1C4D MAP gate
    // published in entity.mapBlocked. Its range gate depends on DS:81C4;
    // the native caller may supply that value through the camera stream
    // anchor, while a missing anchor leaves the transition explicitly inert.
    if (static_cast<std::uint16_t>(entity.enemyState) < 1) {
        const std::int32_t orientation = entity.enemyOrientation;
        const std::int32_t patrolDirection = entity.enemySourceOrKind2c;
        // 68F9-68FE: the raw +0x2F latch is inverted relative to the
        // native boolean spelling. A positive MAP/contact result takes the
        // 6902 patrol/contact response; a nonpositive result takes 69DD and
        // integrates the already-published velocity.
        if (static_cast<std::int8_t>(entity.mapBlocked) > 0) {
            if (patrolDirection < 0) {
                if (entity.enemyTimer == 0x14) {
                    loadAnimation(0x33c0);
                }
                entity.velocityX.raw = clampWrapped(
                    Fixed16::wrapSubRaw(entity.velocityX.raw,
                                        orientation * 0x1000),
                    static_cast<std::int32_t>(0xfffeb000U), 0x15000);
                advanceX();
                const std::uint16_t timer = entity.enemyTimer;
                entity.enemyTimer = static_cast<std::uint16_t>(timer - 1);
                if (timer == 0) {
                    entity.enemyOrientation = static_cast<std::int8_t>(
                        0 - entity.enemyOrientation);
                    entity.enemyPatrolDirection = static_cast<std::int8_t>(
                        0 - entity.enemyPatrolDirection);
                    entity.enemySourceOrKind2c = static_cast<std::int8_t>(
                        0 - entity.enemySourceOrKind2c);
                    entity.velocityX.raw = static_cast<std::int32_t>(
                        entity.enemyOrientation) * 0x0200;
                    entity.enemyTimer = 0x3c;
                }
            } else {
                entity.velocityX.raw = clampWrapped(
                    Fixed16::wrapAddRaw(entity.velocityX.raw,
                                        orientation * 0x0400),
                    static_cast<std::int32_t>(0xfffeb000U), 0x15000);
                advanceX();
                const std::uint16_t timer = entity.enemyTimer;
                entity.enemyTimer = static_cast<std::uint16_t>(timer - 1);
                if (timer == 0) {
                    entity.enemySourceOrKind2c = static_cast<std::int8_t>(
                        0 - entity.enemySourceOrKind2c);
                    entity.mapBlocked = 0xff;
                    entity.enemyTimer = 0x14;
                }
            }
        } else {
            advanceX();
            entity.enemyPhaseTimer = static_cast<std::uint16_t>(
                entity.enemyPhaseTimer + 1);
            if (entity.enemyPhaseTimer > 0x96) {
                entity.enemyPhaseTimer = 0;
                entity.mapBlocked = 1;
            }
        }

        if (_streamAnchorActive) {
            const std::int16_t playerX = static_cast<std::int16_t>(
                player.positionX.floorPixels());
            const std::int16_t playerY = static_cast<std::int16_t>(
                player.positionY.floorPixels());
            const std::int16_t objectX = transitionX();
            const std::int16_t objectY = transitionY();
            bool inXRange = false;
            if (entity.enemyOrientation < 0) {
                const std::int16_t right = static_cast<std::int16_t>(
                    playerX + 0x28);
                inXRange = objectX < right && objectX >
                    static_cast<std::int16_t>(right - 5);
            } else {
                const std::int16_t left = static_cast<std::int16_t>(
                    playerX - 0x28);
                inXRange = objectX > left && objectX <
                    static_cast<std::int16_t>(left + 5);
            }
            if (inXRange && objectY < playerY &&
                objectY > static_cast<std::int16_t>(_streamAnchorY)) {
                entity.enemyState = 1;
                loadAnimation(0x33c8);
            }
        }
        return;
    }

    // States 1-8 consume startup-built DS:7974. Keep this as an explicit
    // external-data boundary when the runtime table has not been injected;
    // state-zero motion above is fully independent of that table.
    if (!_config.hasBieneRuntimeTable)
        return;

    // 01F7:6A69-6AC5. DS:7974 is indexed by a 10-bit wrapped phase. The
    // original writes the vertical high word as a 16-bit word, so preserve
    // that width instead of treating the operation as fixed-point addition.
    if (static_cast<std::uint16_t>(entity.enemyState) < 2) {
        const std::uint16_t oldHigh = static_cast<std::uint16_t>(
            fixedHighWord(entity.positionY.raw));
        const std::uint16_t priorVertical = static_cast<std::uint16_t>(
            entity.enemyVerticalOffset40);
        const std::uint16_t nextHigh = static_cast<std::uint16_t>(
            oldHigh - priorVertical);
        entity.positionY.raw = replaceHighWord(entity.positionY.raw, nextHigh);
        const std::uint16_t phase = static_cast<std::uint16_t>(
            (static_cast<std::uint16_t>(entity.enemyAux3e) + 0x20U) & 0x03ffU);
        entity.enemyAux3e = phase;
        const std::int32_t tableByte = static_cast<std::int8_t>(
            _config.bieneRuntimeTable[phase]);
        const std::int32_t phaseDelta = Fixed16::arithmeticShiftRight(
            tableByte, 5);
        entity.enemyVerticalOffset40 = static_cast<std::int16_t>(phaseDelta);
        const std::uint16_t adjustedHigh = static_cast<std::uint16_t>(
            nextHigh + static_cast<std::uint16_t>(
                static_cast<std::int16_t>(phaseDelta)));
        entity.positionY.raw = replaceHighWord(entity.positionY.raw,
                                               adjustedHigh);
        entity.positionY.raw = Fixed16::wrapSubRaw(
            entity.positionY.raw, 0x1388);
        entity.positionX.raw = Fixed16::wrapSubRaw(
            entity.positionX.raw,
            static_cast<std::int32_t>(entity.enemyOrientation) * 0x2000);
        entity.x = entity.positionX.floorPixels();
        advanceY();
        entity.enemyPhase34 = static_cast<std::uint16_t>(
            static_cast<std::uint16_t>(entity.enemyPhase34) + 1);
        if (static_cast<std::uint16_t>(entity.enemyPhase34) > 0x32) {
            entity.enemyPhase34 = 0;
            entity.enemyState = 2;
        }
    }

    // 01F7:6ACB-6C0F. The setup write and timer decrement intentionally use
    // the original order; state 2 therefore enters state 3 in this callback.
    if (static_cast<std::uint16_t>(entity.enemyState) < 5) {
        entity.enemySavedVelocity3a = entity.velocityX.raw;
        if (entity.enemyState != 3 && entity.enemyState != 4) {
            entity.enemyState = 3;
            entity.velocityY.raw = 0x1f4;
            entity.enemyTimer = 0x3c;
        }
        if (entity.enemyState == 3) {
            entity.velocityY.raw = clampWrapped(
                Fixed16::wrapAddRaw(entity.velocityY.raw, 0x4e20),
                static_cast<std::int32_t>(0xfffb0000U), 0x50000);
            entity.velocityX.raw = clampWrapped(
                Fixed16::wrapAddRaw(entity.velocityX.raw,
                                    static_cast<std::int32_t>(
                                        entity.enemyOrientation) * 0x10000),
                static_cast<std::int32_t>(0xfffeb000U), 0x15000);
            entity.positionY.raw = Fixed16::wrapAddRaw(
                entity.positionY.raw, entity.velocityY.raw);
            entity.positionX.raw = Fixed16::wrapAddRaw(
                entity.positionX.raw, entity.velocityX.raw);
            entity.x = entity.positionX.floorPixels();
            advanceY();
            const std::uint16_t timer = entity.enemyTimer;
            entity.enemyTimer = static_cast<std::uint16_t>(timer - 1);
            if (timer == 0) {
                entity.enemyState = 4;
                entity.enemyTimer = 0x3c;
            }
        } else {
            entity.positionY.raw = Fixed16::wrapAddRaw(
                entity.positionY.raw, entity.velocityY.raw);
            advanceY();
        }
        if (world.transitionDescriptorProbeConfirmed(
                transitionX(), transitionY())) {
            entity.enemyState = 5;
            loadAnimation(0x33d2);
        }
    }

    // 01F7:6C12-6C35. This is the fixed response wait before the exit arc.
    if (static_cast<std::uint16_t>(entity.enemyState) < 6) {
        entity.enemyPhase34 = static_cast<std::uint16_t>(
            static_cast<std::uint16_t>(entity.enemyPhase34) + 1);
        if (static_cast<std::uint16_t>(entity.enemyPhase34) > 0x6e) {
            entity.enemyPhase34 = 0;
            entity.enemyState = 6;
            loadAnimation(0x33c0);
        }
    }

    // 01F7:6C3A-6CFC. The final exit motion uses the raw MAP 0x4000 probe
    // from 1C4D at (x +/- 10, y - 10), then restores state zero below the
    // original Y origin. No descriptor polarity is invented here.
    if (entity.enemyState != 7 && entity.enemyState != 8) {
        entity.enemyState = 7;
        entity.velocityY.raw = static_cast<std::int32_t>(0xfffffe0cU);
        entity.enemyTimer = 0x46;
    }
    if (entity.enemyState == 7) {
        entity.velocityY.raw = clampWrapped(
            Fixed16::wrapSubRaw(entity.velocityY.raw, 0xfa0),
            static_cast<std::int32_t>(0xfffc0000U), 0x40000);
        entity.positionY.raw = Fixed16::wrapAddRaw(
            entity.positionY.raw, entity.velocityY.raw);
        advanceY();
        const std::int32_t probeX = static_cast<std::int32_t>(
            fixedHighWord(entity.positionX.raw)) +
            (entity.enemyOrientation == 1 ? 10 : -10);
        const std::int32_t probeY = static_cast<std::int32_t>(
            fixedHighWord(entity.positionY.raw)) - 10;
        if (world.mapRawBit4000Confirmed(probeX, probeY)) {
            entity.enemyOrientation = static_cast<std::int8_t>(
                0 - entity.enemyOrientation);
            entity.enemyPatrolDirection = static_cast<std::int8_t>(
                0 - entity.enemyPatrolDirection);
        }
        entity.positionX.raw = Fixed16::wrapAddRaw(
            entity.positionX.raw,
            static_cast<std::int32_t>(entity.enemyOrientation) * 0x20000);
        entity.x = entity.positionX.floorPixels();
        const std::uint16_t timer = entity.enemyTimer;
        entity.enemyTimer = static_cast<std::uint16_t>(timer - 1);
        if (timer == 0) {
            entity.enemyState = 8;
            entity.enemyTimer = 0x46;
        }
    } else {
        entity.positionY.raw = Fixed16::wrapAddRaw(
            entity.positionY.raw, entity.velocityY.raw);
        advanceY();
    }

    if (entity.positionY.raw < entity.enemyOriginY36) {
        entity.enemyState = 0;
        entity.enemyTimer = 0x14;
        entity.velocityX.raw = entity.enemySavedVelocity3a;
    }
}

bool LevelSession::enemyMapBlocked(const LevelEntity &entity,
                                   const WorldCollisionView &world) const {
    const std::int32_t direction = entity.velocityX.raw < 0 ? -1 : 1;
    std::int32_t horizontalProbe = 0x26;
    if (entity.type == 0x05 || entity.type == 0x06 ||
        entity.type == 0x07 || entity.type == 0x08) {
        // FISCH and KRABBE use the object+0x39 directional probe field. The
        // initializer seeds it to 0x14; subsequent transitions may update it,
        // so read the field rather than replacing it with a constant.
        horizontalProbe = entity.enemySineOrProbe39;
    } else if (entity.type == 0x09 || entity.type == 0x0a) {
        horizontalProbe = 0x0e;
    } else if (entity.type == 0x15 || entity.type == 0x16) {
        horizontalProbe = 0x20;
    } else if (entity.type == 0x17 || entity.type == 0x18) {
        horizontalProbe = 0x23;
    } else if (entity.type == 0x19 || entity.type == 0x1a) {
        horizontalProbe = 0x28;
    } else if (entity.type == 0x1b || entity.type == 0x1c) {
        horizontalProbe = 0x14;
    } else if (entity.type == 0x0b || entity.type == 0x0c) {
        horizontalProbe = 0x20;
    }
    const std::int32_t probeX = entity.x + direction * horizontalProbe;
    if (isWurm2Type(entity.type)) {
        // 01F7:6DC4 first consumes 01F7:1C4D with AX=+0x28, BX=-0x28.
        // 1C4D returns CF when its 5C27 descriptor probe is occupied and
        // 6DC4 stores +0x2F=1 on that carry. It then makes one direct 5C27
        // probe at object Y and X +/-0x26; the JZ path stores +0x2F=1 when
        // that probe is clear. Preserve the raw branch polarity here. A
        // missing descriptor table is an explicit external boundary and
        // retains the initializer-equivalent nonpositive state.
        if (!world.hasDescriptorTable()) {
            return false;
        }
        const std::int32_t objectX = entity.x;
        const std::int32_t objectY = entity.y;
        const std::int32_t orientedX = objectX +
            (entity.enemyOrientation == 1 ? 0x28 : -0x28);
        if (world.blocksProbeConfirmed(orientedX, objectY - 0x28)) {
            return true;
        }
        const std::int32_t sideX = objectX +
            (entity.enemyOrientation > 0 ? 0x26 : -0x26);
        return !world.blocksProbeConfirmed(sideX, objectY);
    }

    if (isBieneType(entity.type)) {
        // 01F7:1C4D receives (x=20,y=-20) from 68C0. It negates the X
        // offset unless +0x29 is exactly one, then forwards one raw 1C6E
        // 0x4000 MAP probe; there is no second Y probe in this helper.
        const std::int32_t orientedX = static_cast<std::int32_t>(
            fixedHighWord(entity.positionX.raw)) +
            (entity.enemyOrientation == 1 ? 20 : -20);
        const std::int32_t orientedY = static_cast<std::int32_t>(
            fixedHighWord(entity.positionY.raw)) - 20;
        return world.mapRawBit4000Confirmed(orientedX, orientedY);
    }
    if (entity.type == 0x0b || entity.type == 0x0c) {
        // 66E1/6757's MAP gate is an outer 20x40 pair.
        return world.mapRawBit4000Confirmed(probeX, entity.y - 20) ||
               world.mapRawBit4000Confirmed(probeX, entity.y + 20);
    }
    if (entity.type == 0x09 || entity.type == 0x0a) {
        return world.mapRawBit4000Confirmed(probeX, entity.y - 10) ||
               world.mapRawBit4000Confirmed(probeX, entity.y + 10);
    }
    // The remaining normal callbacks use the three directional MAP probes
    // at y-1, y-0x11, and y-0x0c. This is the shared 1C4D/1C6E contract;
    // descriptor meaning is intentionally not inferred here.
    return world.mapRawBit4000Confirmed(probeX, entity.y - 1) ||
           world.mapRawBit4000Confirmed(probeX, entity.y - 0x11) ||
           world.mapRawBit4000Confirmed(probeX, entity.y - 0x0c);
}

void LevelSession::updateNormalEnemy(LevelEntity &entity,
                                      const WorldCollisionView &world) {
    // The eight additional family callbacks share a completely recovered
    // state-0 patrol boundary. Their later vertical/transition states are
    // retained as address-qualified fields until the runtime table and the
    // family-specific animation/effect loaders are wired into the native
    // object trace. This keeps the new dispatch useful without inventing a
    // generic enemy AI for states whose external data is not present here.
    const bool blocked = enemyMapBlocked(entity, world);
    entity.mapBlocked = blocked ? 1 : 0;
    const std::int32_t direction = entity.enemyPatrolDirection < 0 ? -1 : 1;
    const auto clampVelocity = [](std::int32_t value,
                                  std::int32_t cap) {
        return std::max<std::int32_t>(-cap, std::min<std::int32_t>(cap, value));
    };
    const auto advanceX = [&entity]() {
        entity.positionX.raw = Fixed16::wrapAddRaw(entity.positionX.raw,
                                                    entity.velocityX.raw);
        entity.x = entity.positionX.floorPixels();
    };
    if (entity.type == 0x0b || entity.type == 0x0c) {
        // 01F7:6757 adds x velocity and abs(velocity_x >> 1) to Y. The
        // callback's timer/clear edge remains the closed 6838 contract.
        advanceX();
        const std::int32_t verticalStep =
            Fixed16::arithmeticShiftRight(entity.velocityX.raw, 1);
        entity.positionY.raw = Fixed16::wrapAddRaw(
            entity.positionY.raw, verticalStep < 0 ? -verticalStep : verticalStep);
        entity.y = entity.positionY.floorPixels();
        return;
    }

    if (entity.type == 0x09 || entity.type == 0x0a) {
        // 01F7:715E's state-0 blocked branch is distinct from the ordinary
        // patrol path: source -1 applies direction*-0x200, while the later
        // response applies direction*0x400, both with a +/-0x6000 cap.
        if (!blocked) {
            advanceX();
            entity.enemyPhaseTimer = static_cast<std::uint16_t>(
                entity.enemyPhaseTimer + 1);
            if (entity.enemyPhaseTimer > 0x96) {
                entity.enemyPhaseTimer = 0;
                entity.enemyState = 1;
            }
            return;
        }
        const std::int32_t delta = entity.enemySourceOrKind2c < 0
            ? direction * -0x200 : direction * 0x400;
        entity.velocityX.raw = clampVelocity(
            Fixed16::wrapAddRaw(entity.velocityX.raw, delta), 0x6000);
        advanceX();
        const std::int16_t nextTimer = static_cast<std::int16_t>(
            static_cast<std::uint16_t>(entity.enemyTimer - 1));
        entity.enemyTimer = static_cast<std::uint16_t>(nextTimer);
        if (nextTimer < 0) {
            entity.enemyOrientation = static_cast<std::int8_t>(
                -entity.enemyOrientation);
            entity.enemyPatrolDirection = static_cast<std::int8_t>(
                -entity.enemyPatrolDirection);
            entity.enemySourceOrKind2c = static_cast<std::int8_t>(
                -entity.enemySourceOrKind2c);
            entity.velocityX.raw = static_cast<std::int32_t>(
                entity.enemyPatrolDirection) * 0x20;
            entity.enemyTimer = 0x19;
        }
        return;
    }

    if (entity.enemyState > 0) {
        // State transitions are present in the static closure, but cannot be
        // reduced to the existing world view without the family runtime-table/effect
        // tables. Preserve state and all typed fields for a later trace-backed
        // expansion rather than applying a guessed vertical trajectory.
        return;
    }

    if (!blocked) {
        advanceX();
        entity.enemyPhaseTimer = static_cast<std::uint16_t>(
            entity.enemyPhaseTimer + 1);
        if (entity.enemyPhaseTimer > 0x96) {
            entity.enemyPhaseTimer = 0;
            entity.enemyState = 1;
        }
        return;
    }

    std::int32_t delta = direction * 0x400;
    std::int32_t cap = 0x15000;
    if (entity.type == 0x07 || entity.type == 0x08) {
        delta = entity.enemySourceOrKind2c < 0 ? direction * -0x400
                                               : direction * 0x400;
        cap = 0x5000;
    } else if (entity.type == 0x15 || entity.type == 0x16) {
        delta = entity.enemySourceOrKind2c < 0 ? direction * -0x1000
                                               : direction * 0x400;
        cap = 0x17000;
    } else if (entity.type == 0x17 || entity.type == 0x18) {
        delta = entity.enemySourceOrKind2c < 0 ? direction * -0x1000
                                               : direction * 0x400;
        cap = 0x12000;
    } else if (entity.type == 0x19 || entity.type == 0x1a ||
               entity.type == 0x1b || entity.type == 0x1c) {
        delta = entity.enemySourceOrKind2c < 0 ? direction * -0x1000
                                               : direction * 0x400;
        cap = 0x15000;
    }
    entity.velocityX.raw = clampVelocity(
        Fixed16::wrapAddRaw(entity.velocityX.raw, delta), cap);
    advanceX();
    const std::int16_t nextTimer = static_cast<std::int16_t>(
        static_cast<std::uint16_t>(entity.enemyTimer - 1));
    entity.enemyTimer = static_cast<std::uint16_t>(nextTimer);
    if (nextTimer < 0) {
        entity.enemyOrientation = static_cast<std::int8_t>(
            -entity.enemyOrientation);
        entity.enemyPatrolDirection = static_cast<std::int8_t>(
            -entity.enemyPatrolDirection);
        entity.enemySourceOrKind2c = static_cast<std::int8_t>(
            -entity.enemySourceOrKind2c);
        entity.velocityX.raw = static_cast<std::int32_t>(
            entity.enemyPatrolDirection) * 0x20;
        entity.enemyTimer = 0x19;
    }
}

void LevelSession::beginEnemyContact(LevelEntity &entity) {
    entity.enemyContactPending = true;
    const bool alternateResponse = entity.type == 0x05 || entity.type == 0x06 ||
        entity.type == 0x07 || entity.type == 0x08;
    entity.contactCallback = CallbackIdentity(
        0x01f7, alternateResponse ? 0x4ba0 : 0x4ab3,
        alternateResponse ? "enemy_contact_4ba0" : "enemy_contact_4ab3");
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
        if (entity.type == 0x34) {
            // 01F7:9C0C owns BUMP's descriptor countdown in the phase-1
            // callback, before the player callback. Do not replace its
            // object+0x20 timer with the generic native frame counter.
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
    // 01F7:0E96 dispatches phase-1 normal-enemy primary callbacks before the
    // phase-2 player callback. Keep this call before Simulation::tick(); the
    // simulation boundary owns the player update itself.
    dispatchEnemyCallbacks(&simulation, world, player, dependencyOrder);
    // 01F7:19E6 may publish DS:89EA during the phase-1 enemy pass. The
    // phase-2 3FF8 callback reads the same global before any movement path;
    // publish it through the updater boundary before Simulation::tick().
    if (simulation.playerUpdater() != 0) {
        simulation.playerUpdater()->publishTransitionGate(
            _gameplayState.transitionGate89ea);
    }
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
            LevelEvent event;
            event.type = LevelEventType::LevelExit;
            event.targetLevel = target;
            if (_gameplayState.puzzleMask60d8 == 0x007f) {
                applyCompletionPresentation(event);
            }
            _events.push_back(event);
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

    dispatchCollectibleCallbacks(&simulation, player);

    for (std::size_t index = 0; index < _entities.size(); ++index) {
        LevelEntity &entity = _entities[index];
        if (!entity.active || entity.collected) {
            continue;
        }
        if (isNormalEnemyType(entity.type)) {
            // All normal-enemy contact decisions are made in the phase-1
            // callback dispatch above. WURM2/BIENE use 1B77 -> 19E6;
            // the remaining families use their recovered 4AB3/4C5D tail.
            // Do not run the broad provisional hazard reset a second time.
            continue;
        }
        if (entity.type == 0x34) {
            // BUMP's 9C0C contact path is a player response, not the generic
            // death/reset hazard tail. It has already run before 3FF8.
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
