#ifndef QUIKY_LEVEL_H
#define QUIKY_LEVEL_H

#include "quiky/area.h"
#include "quiky/scheduler.h"
#include "quiky/simulation.h"

#include <array>
#include <cstdint>
#include <deque>
#include <string>
#include <vector>

namespace quiky {

enum class EntityKind {
    Unknown,
    Hazard,
    Collectible,
    MovingPlatform,
    EnvironmentalEffect,
    AmbientVisual,
};

enum class EntityPhase {
    Dormant,
    Active,
    Collected,
};

enum class LevelEventType {
    None,
    Collected,
    PlayerJumped,
    EntityCollisionImpact,
    TileInteraction,
    AlternateActionObject,
    PooledObjectInteractionBurst,
    WorldObjectInteraction,
    // 01F7:1B77 -> 19E6 damage state transition. The native 19FB effect
    // dispatcher remains a separate presentation boundary.
    PlayerDamaged,
    PlayerDied,
    LevelExit,
};

struct LevelStateWrite {
    std::uint16_t address;
    std::uint8_t width;
    std::uint32_t before;
    std::uint32_t after;

    LevelStateWrite(std::uint16_t addressValue = 0,
                    std::uint8_t widthValue = 0,
                    std::uint32_t beforeValue = 0,
                    std::uint32_t afterValue = 0)
        : address(addressValue), width(widthValue), before(beforeValue),
          after(afterValue) {}
};

struct LevelEvent {
    LevelEventType type;
    std::uint32_t entityId;
    std::uint16_t entityType;
    std::uint16_t tileId;
    std::string targetLevel;
    std::vector<LevelStateWrite> stateWrites;

    LevelEvent()
        : type(LevelEventType::None), entityId(0), entityType(0), tileId(0xffff),
          targetLevel(), stateWrites() {}
};

// 01F7:87DE is a four-byte shared target/contact row: signed 16-bit X/Y
// words.  The ring is published by the player contact-effect path and
// consumed by enemy callback tails such as 01F7:707B.
struct TargetCoordinateRow {
    std::uint16_t x;
    std::uint16_t y;

    TargetCoordinateRow(std::uint16_t xValue = 0,
                        std::uint16_t yValue = 0)
        : x(xValue), y(yValue) {}
};

struct SpawnCoordinateRow {
    std::uint16_t x;
    std::uint16_t y;
};

// The fields below are the confirmed external state writes made by the
// shared 01F7:8D20 collectible callback. They remain separate from the
// recovered 0x78 player record because the retail callback writes DS globals
// and the persistent player-object timer.
struct LevelGameplayState {
    std::uint16_t ammo880c;
    std::uint16_t lives880a;
    std::uint32_t score881c;
    std::uint16_t currentHealth8822;
    std::uint16_t maximumHealth8824;
    std::uint16_t invulnerabilityGate8810;
    std::uint16_t pendingEvent612e;
    std::uint16_t playerTimer0034;
    std::uint16_t puzzleMask60d8;
    // 01D7:34C7 publishes 32 interleaved word rows at DS:8828..DS:88A7;
    // 01F7:1AAA consumes the row selected by DS:85D2. State-10 8E4B writes
    // row zero. The scalar fields remain row-zero aliases for existing trace
    // consumers until the outer transition path is implemented.
    std::array<SpawnCoordinateRow, 0x20> spawnRows8828;
    std::uint16_t terminalX8828;
    std::uint16_t terminalY882a;
    // 01F7:44FF initializes DS:8806/DS:8808 and clears DS:87DE. The player
    // contact producer currently remains an explicit updater-side boundary;
    // these fields let level callbacks consume captured or test-published
    // rows without assigning semantics to absent runtime data.
    std::uint16_t sharedTargetActiveCount8806;
    std::uint16_t sharedTargetCapacity8808;
    std::array<TargetCoordinateRow, 0x80> sharedTargetRows87de;
    // 01F7:9269 latches DS:89E6 when the cloud/endpoint gate succeeds.
    std::uint16_t cloudSignal89e6;
    // Input/transition gate consumed by the same callback; its outer state
    // machine remains outside this level closure.
    std::uint16_t transitionGate89ea;
    // 01F7:19E6 clears bits 0x30 on terminal damage. The remaining bits are
    // retained because the delayed death/recovery consumer is not yet native.
    std::uint16_t transitionEffectBits8950;
    // Moving-platform publication boundary from 01F7:A075/A0B2.
    std::uint16_t platformLatch5006;
    std::int32_t platformCarryX8816;
    std::int32_t platformCarryY8812;

    LevelGameplayState();
};

struct LevelSessionConfig {
    // ARE origins are aligned to 64-pixel regions.
    std::int32_t streamRadiusRegions;
    std::int32_t hazardRadius;
    std::int32_t edgeExitMargin;
    bool enableEdgeExit;
    bool hasSpawn;
    std::int32_t spawnX;
    std::int32_t spawnY;

    // 01F7:4727/47E7 consume the process-global DS:6468/646C ring while
    // constructing and recycling falling leaves. A replay can publish the
    // captured ring and cursor here; ordinary tests use the zeroed default.
    bool hasLeafPrngState;
    std::uint16_t leafPrngIndex;
    std::array<std::uint8_t, 0x100> leafPrngRing;

    // 01F7:0A43 builds DS:7974 at startup and BIENE state 1 consumes its
    // first 0x400 signed bytes.  The generated values are intentionally an
    // explicit replay input until the DOS-time/software-float initializer is
    // ported; a zeroed/default config never enables this path.
    bool hasBieneRuntimeTable;
    std::array<std::int8_t, 0x400> bieneRuntimeTable;

    LevelSessionConfig();
};

struct SpawnPoint {
    std::int32_t x;
    std::int32_t y;

    SpawnPoint(std::int32_t xValue = 0, std::int32_t yValue = 0)
        : x(xValue), y(yValue) {}
};

// A short-lived visual object emitted by an ARE effect/event record or by a
// source-less high-address callback chain. These objects are deliberately
// separate from gameplay entities: they do not participate in collision,
// collection, or hazard handling. ICO effects use effectResource/effectSlot;
// BOB effects use spriteResource/spriteSlot.
struct LevelEffect {
    std::uint32_t sourceEntityId;
    std::uint16_t sourceType;
    std::int32_t x;
    std::int32_t y;
    std::uint16_t effectSlot;
    std::string effectResource;
    std::uint16_t spriteSlot;
    std::string spriteResource;
    std::uint16_t animationFrame;
    // 01F7:16CE/10B5 event-object metadata. effectSlot is the resolved
    // LOOP_Wn.ICO record; callbackState remains 0xffff when the live child
    // selector was not captured for this source event.
    std::uint16_t callbackState;
    std::uint8_t eventSubtype;
    std::uint8_t eventAnimationState;
    CallbackIdentity updateCallback;
    std::uint16_t lifetime;
    bool active;

    LevelEffect()
        : sourceEntityId(0), sourceType(0), x(0), y(0), effectSlot(0xffff),
          effectResource(), spriteSlot(0xffff), spriteResource(),
          animationFrame(0), callbackState(0xffff), eventSubtype(0),
          eventAnimationState(0), updateCallback(), lifetime(0), active(false) {}
};

struct LevelEntity {
    std::uint32_t id;
    std::uint32_t recordOffset;
    std::uint16_t type;
    std::uint16_t regionX;
    std::uint16_t regionY;
    std::int32_t x;
    std::int32_t y;
    std::int32_t initialX;
    std::int32_t initialY;
    Fixed16 positionX;
    Fixed16 positionY;
    Fixed16 velocityX;
    Fixed16 velocityY;
    EntityKind kind;
    EntityPhase phase;
    std::uint16_t spriteSlot;
    std::string spriteResource;
    std::uint16_t effectSlot;
    std::string effectResource;
    CallbackIdentity updateCallback;
    SchedulerHandle schedulerHandle;
    std::uint8_t contactSubtype;
    std::uint8_t collectionBit;
    std::uint16_t enemyPhaseTimer;
    std::uint16_t enemyTimer;
    std::int16_t enemyState;
    std::int8_t enemyOrientation;
    std::int8_t enemyPatrolDirection;
    std::uint16_t enemyTransitionTimer;
    // Address-qualified normal-object fields from the focused Ghidra
    // closure. Keep the raw-width distinctions explicit; several families
    // use the same offsets with different byte/word roles.
    std::int32_t enemyPhase34;
    std::int32_t enemySineOrProbe39;
    std::int8_t enemyVerticalState36;
    std::int8_t enemyTransitionState3d;
    std::int8_t enemySourceOrKind2c;
    std::int32_t enemyAux3e;
    std::int32_t enemyVerticalOffset40;
    std::int32_t enemyOriginY36;
    std::int32_t enemySavedVelocity3a;
    std::int8_t enemySavedDirection44;
    std::uint8_t mapBlocked;
    // 01F7:707B uses object+0x30 as the shared DS:87DE row cursor.
    std::uint16_t targetCursor30;
    std::uint16_t enemyAnimationDelay;
    std::uint16_t enemyAnimationSequence;
    // 8E4B: object+0x2E variant selector and object+0x32 callback state.
    std::uint16_t environmentSelector;
    std::uint16_t environmentState;
    // 01F7:9C0C / 01F7:5D38 BUMP descriptor state. The timer is object
    // +0x20 and the cursor is retained separately from generic animation
    // counters so the seven-tick 400/402/403/401 cycle is auditable.
    std::uint16_t bumpAnimationDelay20;
    std::uint16_t bumpAnimationCursor24;
    // 01F7:474D/47E7 initializer-visible state for pooled BLATT children.
    // The signed PRNG perturbation and source emission cadence remain
    // address-qualified; these fields retain the confirmed fixed-point
    // contract instead of a generic animation approximation.
    Fixed16 ambientVelocityY;
    std::int32_t ambientOriginX;
    std::int32_t ambientOriginY;
    std::uint16_t ambientTimer;
    std::uint16_t ambientAnimationDelay;
    std::uint16_t ambientAnimationCursor;
    std::uint8_t ambientTable;
    bool ambientRuntimeInitialized;
    // 01F7:9DC7/A0B2 platform object state.
    std::int32_t platformPreviousX;
    std::int32_t platformPreviousY;
    std::uint16_t platformWait52;
    std::uint16_t platformWait54;
    std::uint16_t platformCooldown58;
    // 01F7:9C70/9CF5/9D19/9D5E/9D82 state. These remain byte-shaped
    // values because 9DC7 compares them against literal -1/0/1 values.
    std::int8_t platformDirectionY4c;
    std::int8_t platformEdgeLatch50;
    std::int8_t platformDirectionX4e;
    bool platformHorizontal4a;
    std::uint8_t platformAxisMarker4b;
    bool platformMotionGate59;
    bool platformInitializerMapChecked;
    bool platformCarryActive;
    bool streamSuppressed;
    bool enemyContactPending;
    CallbackIdentity contactCallback;
    std::uint16_t responseTimer;
    std::uint16_t collisionWidth;
    std::uint16_t collisionHeight;
    std::uint16_t animationFrame;
    std::uint32_t activeFrames;
    bool active;
    bool collected;
    bool pooledInteractionTriggered;

    LevelEntity()
        : id(0), recordOffset(0), type(0), regionX(0), regionY(0), x(0), y(0),
          initialX(0), initialY(0), positionX(), positionY(), velocityX(), velocityY(),
          kind(EntityKind::Unknown), phase(EntityPhase::Dormant),
          spriteSlot(0xffff), spriteResource(), effectSlot(0xffff), effectResource(),
          updateCallback(), schedulerHandle(), contactSubtype(0), collectionBit(0),
          enemyPhaseTimer(0), enemyTimer(0), enemyState(0),
          enemyOrientation(-1), enemyPatrolDirection(-1),
          enemyTransitionTimer(0), enemyPhase34(0), enemySineOrProbe39(0),
          enemyVerticalState36(0), enemyTransitionState3d(0),
          enemySourceOrKind2c(-1), enemyAux3e(0), enemyVerticalOffset40(0),
          enemyOriginY36(0), enemySavedVelocity3a(0),
          enemySavedDirection44(0), mapBlocked(0), targetCursor30(0),
          enemyAnimationDelay(0),
          enemyAnimationSequence(0), environmentSelector(0),
          environmentState(0), bumpAnimationDelay20(0),
          bumpAnimationCursor24(0), ambientVelocityY(), ambientOriginX(0),
          ambientOriginY(0), ambientTimer(0), ambientAnimationDelay(0),
          ambientAnimationCursor(0), ambientTable(0),
          ambientRuntimeInitialized(false), platformPreviousX(0),
          platformPreviousY(0), platformWait52(0), platformWait54(0),
          platformCooldown58(0), platformDirectionY4c(-1),
          platformEdgeLatch50(-1), platformDirectionX4e(1),
          platformHorizontal4a(false), platformAxisMarker4b(0),
          platformMotionGate59(true), platformInitializerMapChecked(false),
          platformCarryActive(false),
          streamSuppressed(false),
          enemyContactPending(false), contactCallback(), responseTimer(0),
          collisionWidth(0), collisionHeight(0),
          animationFrame(0), activeFrames(0),
          active(false), collected(false), pooledInteractionTriggered(false) {}
};

class LevelSession {
public:
    LevelSession(const std::string &mapName, const Map &map, const Area &area,
                 const LevelSessionConfig &config = LevelSessionConfig());

    SpawnPoint spawnPoint() const;
    void reset(Simulation &simulation);
    // 01D7:5010/01F7:1AAA preserves the persistent gameplay fields while
    // rebuilding the target level. Transient level/object fields are reset
    // by reset().
    void restorePersistentStateForReload(
        const LevelGameplayState &previousState, std::uint32_t previousDeaths);
    void tick(Simulation &simulation, const WorldCollisionView &world,
              const InputState &input, SimulationOutput &output);

    bool updateStreaming(std::int32_t playerX, std::int32_t playerY);
    bool updateStreaming(Simulation &simulation, std::int32_t playerX,
                         std::int32_t playerY);
    // The native loader/renderer streams ARE declarations from the camera
    // window, not directly from the player's authored spawn Y. Callers that
    // own camera state can publish that anchor; setup-only callers retain the
    // player-coordinate fallback.
    void setStreamAnchor(std::int32_t cameraX, std::int32_t cameraY);
    void clearStreamAnchor();
    bool hasStreamAnchor() const { return _streamAnchorActive; }
    std::int32_t streamAnchorX() const { return _streamAnchorX; }
    std::int32_t streamAnchorY() const { return _streamAnchorY; }
    // Emit the source-less high-address effect recovered from the 4B70/4C74
    // callback chain. sourceX/sourceY are the hit object's coordinates; the
    // pooled effect is positioned at sourceY + 10 pixels.
    void emitHighEffect(std::int32_t sourceX, std::int32_t sourceY);
    const std::vector<LevelEntity> &entities() const { return _entities; }
    std::vector<LevelEntity> &entitiesForSetup() { return _entities; }
    const std::vector<LevelEffect> &effects() const { return _effects; }
    bool hasPendingEvents() const { return !_events.empty(); }
    LevelEvent consumeEvent();
    std::uint32_t score() const { return _score; }
    std::uint32_t deaths() const { return _deaths; }
    const LevelGameplayState &gameplayState() const { return _gameplayState; }
    LevelGameplayState &gameplayStateForSetup() { return _gameplayState; }
    const std::string &mapName() const { return _mapName; }

private:
    static EntityKind classify(std::uint16_t type);
    static bool isNormalEnemyType(std::uint16_t type);
    static bool isWurm2Type(std::uint16_t type);
    static bool isBieneType(std::uint16_t type);
    static std::int32_t normalEnemyYOffset(std::uint16_t type);
    static bool isCloudType(std::uint16_t type);
    static bool isLeafType(std::uint16_t type);
    static bool isDedicatedEventType(std::uint16_t type);
    static CallbackIdentity callbackFor(std::uint16_t type);
    static std::uint8_t collectibleSubtypeFor(std::uint16_t type);
    static std::uint8_t collectionBitFor(std::uint16_t type);
    static std::uint16_t spriteSlotFor(std::uint16_t type);
    std::uint16_t effectSlotFor(std::uint16_t type) const;
    static std::uint16_t collisionWidthFor(std::uint16_t type);
    static std::uint16_t collisionHeightFor(std::uint16_t type);
    std::string spriteResourceFor(std::uint16_t type) const;
    std::string effectResourceFor(std::uint16_t type) const;
    std::string highEffectSpriteResource() const;
    static std::uint32_t collectibleValue(std::uint16_t type);
    static std::string nextLevelName(const std::string &mapName);
    void resetPlayer(Simulation &simulation);
    bool updateStreamingImpl(ObjectScheduler *scheduler,
                             std::int32_t playerX, std::int32_t playerY);
    void dispatchCollectibleCallbacks(Simulation *simulation,
                                      PlayerRecord &player);
    void dispatchEnemyCallbacks(Simulation *simulation,
                                 const WorldCollisionView &world,
                                 PlayerRecord &player,
                                 std::vector<SimulationCallbackStep> &dependencyOrder);
    void dispatchCloudCallbacks(Simulation *simulation,
                                const PlayerRecord &player);
    void dispatchMovingPlatformCallbacks(Simulation *simulation,
                                         const WorldCollisionView &world,
                                         PlayerRecord &player,
                                         std::vector<SimulationCallbackStep>
                                             &dependencyOrder);
    void publishMovingPlatformCarry(LevelEntity &entity,
                                    PlayerRecord &player,
                                    Simulation *simulation);
    bool dispatchWorldEffectCallbacks(Simulation *simulation);
    void initializeEnemy(LevelEntity &entity);
    void initializeCollectible(LevelEntity &entity);
    void initializeWorldEffect(LevelEntity &entity);
    void initializeAmbientVisual(LevelEntity &entity);
    void initializeAmbientVisualRuntime(LevelEntity &entity);
    // DS:6468/646C is shared by leaf, event, and normal-enemy paths. The
    // configuration retains its historical leaf-facing names for replay
    // compatibility, but this accessor names the recovered global contract.
    std::uint8_t nextSharedPrngByte();
    void initializeMovingPlatform(LevelEntity &entity);
    void initializeBump(LevelEntity &entity);
    bool updateWorldEffect(Simulation *simulation, LevelEntity &entity);
    void updateWurm2(LevelEntity &entity, const WorldCollisionView &world);
    // 01F7:707B target/contact tail. The subsequent 4AB3 response callback
    // remains an address-qualified external object contract.
    void consumeWurm2TargetTail(LevelEntity &entity);
    void updateBiene(LevelEntity &entity, const WorldCollisionView &world,
                     const PlayerRecord &player);
    void updateBump(Simulation *simulation, LevelEntity &entity,
                    PlayerRecord &player);
    void updateNormalEnemy(LevelEntity &entity,
                           const WorldCollisionView &world);
    bool enemyMapBlocked(const LevelEntity &entity,
                         const WorldCollisionView &world) const;
    void beginEnemyContact(LevelEntity &entity);
    // 01F7:1B77 -> 01F7:393C -> 01F7:19E6. This is the WURM2/BIENE
    // damage route; it is deliberately separate from the 4AB3/4BA0 object
    // response tail used by other normal-enemy families.
    bool applyNormalEnemyDamage(const LevelEntity &entity,
                                PlayerRecord &player,
                                std::vector<LevelStateWrite> &writes);
    void advanceEnemyResponse(Simulation *simulation, LevelEntity &entity);
    void applyCollectibleCallback(LevelEntity &entity, PlayerRecord &player,
                                  std::vector<LevelStateWrite> &writes);
    void applyCompletionPresentation(LevelEvent &event);
    void releaseScheduledEntity(ObjectScheduler *scheduler,
                                LevelEntity &entity);
    void appendCollectedEvent(const LevelEntity &entity,
                              const std::vector<LevelStateWrite> &writes);
    void syncPlayerTimer(const PlayerRecord &player);
    void advanceActiveEntities();
    void advanceActiveEffects();
    bool emitWorldEffects(const LevelEntity &entity, std::uint16_t state);
    bool spawnTransientEffect(const LevelEntity &entity);
    void removeTransientEffectsFor(std::uint32_t entityId);
    static bool isTransientEffectType(std::uint16_t type);
    static bool isWorldEffectType(std::uint16_t type);
    static bool isPooledInteractionType(std::uint16_t type);
    bool overlaps(const PlayerRecord &player, const LevelEntity &entity,
                  std::int32_t radius) const;
    bool collectibleOverlaps(const PlayerRecord &player,
                             const LevelEntity &entity) const;
    bool cloudOverlaps(const PlayerRecord &player,
                       const LevelEntity &entity) const;
    bool pooledInteractionOverlaps(const PlayerRecord &player,
                                   const LevelEntity &entity) const;
    bool atRightExit(const PlayerRecord &player) const;
    void enqueueEvent(LevelEventType type, std::uint32_t entityId = 0,
                      std::uint16_t entityType = 0,
                      const std::string &targetLevel = std::string());

    std::string _mapName;
    const Map &_map;
    const Area &_area;
    LevelSessionConfig _config;
    std::vector<LevelEntity> _entities;
    std::vector<LevelEffect> _effects;
    std::deque<LevelEvent> _events;
    std::uint32_t _score;
    std::uint32_t _deaths;
    LevelGameplayState _gameplayState;
    bool _alternateActionActive;
    bool _streamAnchorActive;
    std::int32_t _streamAnchorX;
    std::int32_t _streamAnchorY;
    std::uint16_t _leafPrngIndex;
    std::array<std::uint8_t, 0x100> _leafPrngRing;
};

} // namespace quiky

#endif
