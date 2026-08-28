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

// Address-annotated orchestration stages recovered from the focused
// 01D7:5010 transition closure. These are engine boundaries, not guesses
// about the presentation-only helpers inside each stage.
enum class LevelReloadStage {
    TransitionGate5010,
    ObjectTeardown0908,
    ResourceLookup18C7,
    TransitionBufferCopy0D5A,
    PlayerReposition1AAA,
    AnimationLoader5D38,
    CameraRebuild321F,
    WorldDispatch313D,
    Cleanup504F,
};

struct LevelReloadTrace {
    std::string targetMap;
    std::vector<LevelReloadStage> stages;
    bool hasPlayerSpawn;
    std::int32_t playerSpawnX;
    std::int32_t playerSpawnY;

    LevelReloadTrace()
        : targetMap(), stages(), hasPlayerSpawn(false), playerSpawnX(0),
          playerSpawnY(0) {}
};

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
    const PcxImage &smallFont() const { return _smallFont; }
    // GAMEBAR.BOB contains the native numeric/status sprites. NESQUIK.BOB
    // contains the seven small letters used by the in-game progress display.
    const Bob &gamebarBob() const { return _gamebarBob; }
    const Bob &nesquikBob() const { return _nesquikBob; }
    const Tileset &tileset() const { return _tileset; }
    const Tileset &loopTileset() const { return _loopTileset; }
    const Bob &playerBob() const { return _playerBob; }
    const PlayerDescriptorTable &descriptors() const { return _descriptors; }
    const std::map<std::string, Bob> &entityBobs() const { return _entityBobs; }
    const std::map<std::string, Bob> &effectBobs() const { return _effectBobs; }

    LevelSession &session() { return _session; }
    const LevelSession &session() const { return _session; }

    void reset(Simulation &simulation);
    void tick(Simulation &simulation, const InputState &input,
              SimulationOutput &output);

    // Publish the renderer's camera position for the next ARE streaming
    // pass. The native object gate is camera-relative and is not equivalent
    // to the player's authored spawn.
    void setStreamAnchor(std::int32_t cameraX, std::int32_t cameraY);
    void clearStreamAnchor();

    // Execute the closed high-level equivalent of the native 5010 reload
    // boundary. The returned runtime owns the newly loaded MAP/ARE/resource
    // set; simulation is reset before object reconstruction and then
    // initialized from the target level's player declaration.
    std::unique_ptr<LevelRuntime> reload(
        const Archive &archive, const std::string &targetMapName,
        Simulation &simulation,
        const LevelSessionConfig &config = LevelSessionConfig(),
        LevelReloadTrace *trace = 0) const;

private:
    LevelRuntime(const std::string &mapName, const std::string &areaName,
                 const std::string &worldName, const std::string &playerBobName,
                 const Map &map, const Area &area, const Palette &palette,
                 const PcxImage &gamebar, const PcxImage &smallFont,
                 const Bob &gamebarBob, const Bob &nesquikBob,
                 const Tileset &tileset,
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
    PcxImage _smallFont;
    Bob _gamebarBob;
    Bob _nesquikBob;
    Tileset _tileset;
    Tileset _loopTileset;
    Bob _playerBob;
    PlayerDescriptorTable _descriptors;
    LevelSession _session;
    std::map<std::string, Bob> _entityBobs;
    std::map<std::string, Bob> _effectBobs;
};

} // namespace quiky

#endif
