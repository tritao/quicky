#ifndef QUIKY_LEVEL_H
#define QUIKY_LEVEL_H

#include "quiky/area.h"
#include "quiky/runtime.h"

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
    PlayerDied,
    LevelExit,
};

struct LevelEvent {
    LevelEventType type;
    std::uint32_t entityId;
    std::uint16_t entityType;
    std::uint16_t tileId;
    std::string targetLevel;

    LevelEvent()
        : type(LevelEventType::None), entityId(0), entityType(0), tileId(0xffff),
          targetLevel() {}
};

struct LevelSessionConfig {
    // ARE origins are aligned to 64-pixel regions.
    std::int32_t streamRadiusRegions;
    std::int32_t collectibleRadius;
    std::int32_t hazardRadius;
    std::int32_t edgeExitMargin;
    bool enableEdgeExit;
    bool hasSpawn;
    std::int32_t spawnX;
    std::int32_t spawnY;

    LevelSessionConfig();
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
    std::uint16_t lifetime;
    bool active;

    LevelEffect()
        : sourceEntityId(0), sourceType(0), x(0), y(0), effectSlot(0xffff),
          effectResource(), spriteSlot(0xffff), spriteResource(),
          animationFrame(0), lifetime(0), active(false) {}
};

struct LevelEntity {
    std::uint32_t id;
    std::uint32_t recordOffset;
    std::uint16_t type;
    std::uint16_t regionX;
    std::uint16_t regionY;
    std::int32_t x;
    std::int32_t y;
    EntityKind kind;
    EntityPhase phase;
    std::uint16_t spriteSlot;
    std::string spriteResource;
    std::uint16_t effectSlot;
    std::string effectResource;
    std::uint16_t collisionWidth;
    std::uint16_t collisionHeight;
    std::uint16_t animationFrame;
    std::uint32_t activeFrames;
    bool active;
    bool collected;
    bool pooledInteractionTriggered;

    LevelEntity()
        : id(0), recordOffset(0), type(0), regionX(0), regionY(0), x(0), y(0),
          kind(EntityKind::Unknown), phase(EntityPhase::Dormant),
          spriteSlot(0xffff), spriteResource(), effectSlot(0xffff), effectResource(),
          collisionWidth(0), collisionHeight(0),
          animationFrame(0), activeFrames(0),
          active(false), collected(false), pooledInteractionTriggered(false) {}
};

class LevelSession {
public:
    LevelSession(const std::string &mapName, const Map &map, const Area &area,
                 const LevelSessionConfig &config = LevelSessionConfig());

    SpawnPoint spawnPoint() const;
    void reset(PlayerState &player, const PlayerSimulation &simulation);
    void tick(PlayerState &player, const PlayerSimulation &simulation,
              const InputState &input);
    void tick(PlayerState &player, const PlayerSimulation &simulation,
              const CollisionQuery &collision, const InputState &input);

    bool updateStreaming(std::int32_t playerX, std::int32_t playerY);
    // Emit the source-less high-address effect recovered from the 4B70/4C74
    // callback chain. sourceX/sourceY are the hit object's coordinates; the
    // pooled effect is positioned at sourceY + 10 pixels.
    void emitHighEffect(std::int32_t sourceX, std::int32_t sourceY);
    const std::vector<LevelEntity> &entities() const { return _entities; }
    const std::vector<LevelEffect> &effects() const { return _effects; }
    bool hasPendingEvents() const { return !_events.empty(); }
    LevelEvent consumeEvent();
    std::uint32_t score() const { return _score; }
    std::uint32_t deaths() const { return _deaths; }
    const std::string &mapName() const { return _mapName; }

private:
    static EntityKind classify(std::uint16_t type);
    static std::uint16_t spriteSlotFor(std::uint16_t type);
    std::uint16_t effectSlotFor(std::uint16_t type) const;
    static std::uint16_t collisionWidthFor(std::uint16_t type);
    static std::uint16_t collisionHeightFor(std::uint16_t type);
    std::string spriteResourceFor(std::uint16_t type) const;
    std::string effectResourceFor(std::uint16_t type) const;
    std::string highEffectSpriteResource() const;
    static std::uint32_t collectibleValue(std::uint16_t type);
    static std::string nextLevelName(const std::string &mapName);
    void resetPlayer(PlayerState &player, const PlayerSimulation &simulation) const;
    void advanceActiveEntities();
    void advanceActiveEffects();
    bool emitWorldEffectsForActiveEntities();
    bool emitWorldEffects(const LevelEntity &entity, std::uint16_t state);
    bool spawnTransientEffect(const LevelEntity &entity);
    void removeTransientEffectsFor(std::uint32_t entityId);
    static bool isTransientEffectType(std::uint16_t type);
    static bool isWorldEffectType(std::uint16_t type);
    static bool isPooledInteractionType(std::uint16_t type);
    bool overlaps(const PlayerState &player, const PlayerConfig &playerConfig,
                  const LevelEntity &entity, std::int32_t radius) const;
    bool pooledInteractionOverlaps(const PlayerState &player,
                                   const LevelEntity &entity) const;
    bool atRightExit(const PlayerState &player) const;
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
    bool _alternateActionActive;
};

} // namespace quiky

#endif
