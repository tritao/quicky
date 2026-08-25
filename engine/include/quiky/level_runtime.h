#ifndef QUIKY_LEVEL_RUNTIME_H
#define QUIKY_LEVEL_RUNTIME_H

#include "quiky/archive.h"
#include "quiky/area.h"
#include "quiky/bob.h"
#include "quiky/level.h"
#include "quiky/map.h"
#include "quiky/palette.h"
#include "quiky/pcx.h"
#include "quiky/tileset.h"

#include <map>
#include <memory>
#include <string>

namespace quiky {

// Owns all resources and runtime state needed to play one MAP/ARE level.
// Instances are heap-owned because LevelSession keeps references to the map
// and area members; keeping the bundle at a stable address makes transitions
// safe and keeps the frontend from managing parallel resource lifetimes.
class LevelRuntime {
public:
    static std::unique_ptr<LevelRuntime> load(
        const Archive &archive, const std::string &mapName,
        const std::string &playerBobName = "QUIKYW1.BOB",
        const LevelSessionConfig &config = LevelSessionConfig());

    LevelRuntime(const LevelRuntime &) = delete;
    LevelRuntime &operator=(const LevelRuntime &) = delete;

    const std::string &mapName() const { return _mapName; }
    const std::string &areaName() const { return _areaName; }
    const std::string &worldName() const { return _worldName; }
    const std::string &playerBobName() const { return _playerBobName; }

    const Map &map() const { return _map; }
    const Area &area() const { return _area; }
    const Palette &palette() const { return _palette; }
    const PcxImage &gamebar() const { return _gamebar; }
    const Tileset &tileset() const { return _tileset; }
    const Tileset &loopTileset() const { return _loopTileset; }
    const Bob &playerBob() const { return _playerBob; }
    const std::map<std::string, Bob> &entityBobs() const { return _entityBobs; }
    const std::map<std::string, Bob> &effectBobs() const { return _effectBobs; }

    LevelSession &session() { return _session; }
    const LevelSession &session() const { return _session; }

    void reset(PlayerState &player, const PlayerSimulation &simulation);
    void tick(PlayerState &player, const PlayerSimulation &simulation,
              const InputState &input);

private:
    LevelRuntime(const std::string &mapName, const std::string &areaName,
                 const std::string &worldName, const std::string &playerBobName,
                 const Map &map, const Area &area, const Palette &palette,
                 const PcxImage &gamebar, const Tileset &tileset,
                 const Tileset &loopTileset,
                 const Bob &playerBob,
                 const LevelSessionConfig &config);

    void loadEntityBobs(const Archive &archive);
    void loadEffectBobs(const Archive &archive);

    std::string _mapName;
    std::string _areaName;
    std::string _worldName;
    std::string _playerBobName;
    Map _map;
    Area _area;
    Palette _palette;
    PcxImage _gamebar;
    Tileset _tileset;
    Tileset _loopTileset;
    Bob _playerBob;
    LevelSession _session;
    std::map<std::string, Bob> _entityBobs;
    std::map<std::string, Bob> _effectBobs;
};

} // namespace quiky

#endif
