#ifndef QUIKY_LEVEL_H
#define QUIKY_LEVEL_H

#include "quiky/area.h"
#include "quiky/runtime.h"

#include <cstdint>
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
    PlayerDied,
    PlayerRecovered,
    LevelExit,
};

struct LevelEvent {
    LevelEventType type;
    std::uint32_t entityId;
    std::uint16_t entityType;
    std::string targetLevel;

    LevelEvent()
        : type(LevelEventType::None), entityId(0), entityType(0), targetLevel() {}
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
    // Native player control decrements from -1 and requests recovery once it
    // passes -349. This is measured in player-callback updates, not wall time.
    std::uint32_t deathRecoveryFrames;

    LevelSessionConfig();
};

// A short-lived ICO-only object emitted by an ARE effect/event record. These
// objects are deliberately separate from gameplay entities: they do not
// participate in collision, collection, or hazard handling.
struct LevelEffect {
    std::uint32_t sourceEntityId;
    std::uint16_t sourceType;
    std::int32_t x;
    std::int32_t y;
    std::uint16_t effectSlot;
    std::string effectResource;
    std::uint16_t animationFrame;
    std::uint16_t lifetime;
    bool active;

    LevelEffect()
        : sourceEntityId(0), sourceType(0), x(0), y(0), effectSlot(0xffff),
          effectResource(), animationFrame(0), lifetime(0), active(false) {}
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

    LevelEntity()
        : id(0), recordOffset(0), type(0), regionX(0), regionY(0), x(0), y(0),
          kind(EntityKind::Unknown), phase(EntityPhase::Dormant),
          spriteSlot(0xffff), spriteResource(), effectSlot(0xffff), effectResource(),
          collisionWidth(0), collisionHeight(0),
          animationFrame(0), activeFrames(0),
          active(false), collected(false) {}
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

    void updateStreaming(std::int32_t playerX, std::int32_t playerY);
    const std::vector<LevelEntity> &entities() const { return _entities; }
    const std::vector<LevelEffect> &effects() const { return _effects; }
    LevelEvent consumeEvent();
    std::uint32_t score() const { return _score; }
    std::uint32_t deaths() const { return _deaths; }
    std::uint16_t goalMask() const { return _goalMask; }
    bool playerDying() const { return _playerLife == PlayerLife::Dying; }
    std::uint32_t deathFrames() const { return _deathFrames; }
    SpawnPoint checkpoint() const { return SpawnPoint(_checkpointX, _checkpointY); }
    const std::string &mapName() const { return _mapName; }

private:
    enum class PlayerLife {
        Alive,
        Dying,
    };

    static EntityKind classify(std::uint16_t type);
    static std::uint16_t spriteSlotFor(std::uint16_t type);
    std::uint16_t effectSlotFor(std::uint16_t type) const;
    static std::uint16_t collisionWidthFor(std::uint16_t type);
    static std::uint16_t collisionHeightFor(std::uint16_t type);
    std::string spriteResourceFor(std::uint16_t type) const;
    std::string effectResourceFor(std::uint16_t type) const;
    static std::uint32_t collectibleValue(std::uint16_t type);
    static std::string nextLevelName(const std::string &mapName,
                                     std::uint16_t goalMask);
    void resetPlayer(PlayerState &player, const PlayerSimulation &simulation) const;
    void resetPlayerAt(PlayerState &player, const PlayerSimulation &simulation,
                       std::int32_t x, std::int32_t y) const;
    void publishCheckpoint(const LevelEntity &entity);
    void advanceActiveEntities();
    void advanceActiveEffects();
    void emitWorldEffectsForActiveEntities();
    void emitWorldEffects(const LevelEntity &entity, std::uint16_t state);
    void spawnTransientEffect(const LevelEntity &entity);
    void removeTransientEffectsFor(std::uint32_t entityId);
    static bool isTransientEffectType(std::uint16_t type);
    static bool isWorldEffectType(std::uint16_t type);
    bool overlaps(const PlayerState &player, const PlayerConfig &playerConfig,
                  const LevelEntity &entity, std::int32_t radius) const;
    bool atRightExit(const PlayerState &player) const;

    std::string _mapName;
    const Map &_map;
    const Area &_area;
    LevelSessionConfig _config;
    std::vector<LevelEntity> _entities;
    std::vector<LevelEffect> _effects;
    LevelEvent _event;
    std::uint32_t _score;
    std::uint32_t _deaths;
    std::uint16_t _goalMask;
    PlayerLife _playerLife;
    std::uint32_t _deathFrames;
    std::int32_t _checkpointX;
    std::int32_t _checkpointY;
};

} // namespace quiky

#endif
