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

enum class LevelEventType {
    None,
    Collected,
    PlayerDied,
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
    // ARE origins are aligned to 64 pixels in the original runtime.
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

struct LevelEntity {
    std::uint32_t id;
    std::uint32_t recordOffset;
    std::uint16_t type;
    std::uint16_t regionX;
    std::uint16_t regionY;
    std::int32_t x;
    std::int32_t y;
    EntityKind kind;
    bool active;
    bool collected;

    LevelEntity()
        : id(0), recordOffset(0), type(0), regionX(0), regionY(0), x(0), y(0),
          kind(EntityKind::Unknown), active(false), collected(false) {}
};

class LevelSession {
public:
    LevelSession(const std::string &mapName, const Map &map, const Area &area,
                 const LevelSessionConfig &config = LevelSessionConfig());

    SpawnPoint spawnPoint() const;
    void reset(PlayerState &player, const PlayerSimulation &simulation) const;
    void tick(PlayerState &player, const PlayerSimulation &simulation,
              const InputState &input);

    void updateStreaming(std::int32_t playerX, std::int32_t playerY);
    const std::vector<LevelEntity> &entities() const { return _entities; }
    LevelEvent consumeEvent();
    std::uint32_t score() const { return _score; }
    std::uint32_t deaths() const { return _deaths; }
    const std::string &mapName() const { return _mapName; }

private:
    static EntityKind classify(std::uint16_t type);
    static std::uint32_t collectibleValue(std::uint16_t type);
    static std::string nextLevelName(const std::string &mapName);
    bool overlaps(const PlayerState &player, const PlayerConfig &playerConfig,
                  const LevelEntity &entity, std::int32_t radius) const;
    bool atRightExit(const PlayerState &player) const;

    std::string _mapName;
    const Map &_map;
    const Area &_area;
    LevelSessionConfig _config;
    std::vector<LevelEntity> _entities;
    LevelEvent _event;
    std::uint32_t _score;
    std::uint32_t _deaths;
};

} // namespace quiky

#endif
