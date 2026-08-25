#include "quiky/level_runtime.h"

#include <cctype>

namespace quiky {

namespace {

std::string worldFor(const std::string &mapName) {
    if (mapName.size() < 2 || (mapName[0] != 'W' && mapName[0] != 'w')) {
        throw FormatError("cannot derive world from MAP resource name");
    }
    std::string world(mapName.substr(0, 2));
    world[0] = 'W';
    world[1] = static_cast<char>(std::toupper(static_cast<unsigned char>(world[1])));
    return world;
}

std::string areaNameFor(const std::string &mapName) {
    const std::size_t extension = mapName.find_last_of('.');
    if (extension == std::string::npos) {
        throw FormatError("MAP resource has no extension: " + mapName);
    }
    return mapName.substr(0, extension) + ".ARE";
}

} // namespace

std::unique_ptr<LevelRuntime> LevelRuntime::load(
    const Archive &archive, const std::string &mapName,
    const std::string &playerBobName, const LevelSessionConfig &config) {
    const std::string worldName = worldFor(mapName);
    const std::string areaName = areaNameFor(mapName);
    const Map map = Map::parse(archive.read(mapName), mapName);
    const Area area = Area::parse(archive.read(areaName), areaName);
    const Palette palette = Palette::parsePcxDac(
        archive.read(worldName + ".PCC"), worldName + ".PCC");
    const PcxImage gamebar = PcxImage::parse(
        archive.read("GAMEBAR.PCC"), "GAMEBAR.PCC");
    const Tileset tileset = Tileset::parseIco(
        archive.read(worldName + ".ICO"), worldName + ".ICO");
    const std::string loopName = "LOOP_" + worldName + ".ICO";
    const Tileset loopTileset = Tileset::parseIco(
        archive.read(loopName), loopName);
    const Bob playerBob = Bob::parse(
        archive.read(playerBobName), playerBobName);

    std::unique_ptr<LevelRuntime> result(new LevelRuntime(
        mapName, areaName, worldName, playerBobName, map, area, palette,
        gamebar, tileset, loopTileset, playerBob, config));
    result->loadEntityBobs(archive);
    return result;
}

LevelRuntime::LevelRuntime(const std::string &mapName,
                           const std::string &areaName,
                           const std::string &worldName,
                           const std::string &playerBobName,
                           const Map &map, const Area &area,
                           const Palette &palette, const PcxImage &gamebar,
                           const Tileset &tileset,
                           const Tileset &loopTileset, const Bob &playerBob,
                           const LevelSessionConfig &config)
    : _mapName(mapName),
      _areaName(areaName),
      _worldName(worldName),
      _playerBobName(playerBobName),
      _map(map),
      _area(area),
      _palette(palette),
      _gamebar(gamebar),
      _tileset(tileset),
      _loopTileset(loopTileset),
      _playerBob(playerBob),
      _descriptors(playerDescriptorTableForWorld(worldName)),
      _session(mapName, _map, _area, config),
      _entityBobs() {
}

void LevelRuntime::loadEntityBobs(const Archive &archive) {
    for (std::size_t index = 0; index < _session.entities().size(); ++index) {
        const LevelEntity &entity = _session.entities()[index];
        if (entity.spriteResource.empty() ||
            _entityBobs.find(entity.spriteResource) != _entityBobs.end()) {
            continue;
        }
        const Bob bob = Bob::parse(
            archive.read(entity.spriteResource), entity.spriteResource);
        _entityBobs.insert(std::make_pair(entity.spriteResource, bob));
    }
}

void LevelRuntime::reset(Simulation &simulation) {
    _session.reset(simulation);
    const PlayerRecord &player = simulation.state().player;
    _session.updateStreaming(simulation, player.positionX.floorPixels(),
                             player.positionY.floorPixels());
}

void LevelRuntime::tick(Simulation &simulation,
                        const InputState &input,
                        SimulationOutput &output) {
    const WorldCollisionView world(_map, &_descriptors);
    _session.tick(simulation, world, input, output);
}

} // namespace quiky
