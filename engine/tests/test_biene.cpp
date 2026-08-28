#include "quiky/area.h"
#include "quiky/level.h"
#include "quiky/map.h"
#include "quiky/simulation.h"
#include "quiky/world_view.h"

#include <cassert>
#include <cstdint>
#include <iostream>

namespace {

void writeU16BEAt(quiky::Bytes &data, std::size_t offset,
                  std::uint16_t value) {
    data[offset] = static_cast<quiky::byte>(value >> 8);
    data[offset + 1] = static_cast<quiky::byte>(value & 0xff);
}

quiky::Area makeBieneArea() {
    quiky::Bytes data(0x14e8, 0);
    writeU16BEAt(data, 0x0e, 2);
    writeU16BEAt(data, 0x10, 1);
    writeU16BEAt(data, 0x160, 0x1388);
    writeU16BEAt(data, 0x162, 0xffff);
    data.push_back(0x00);
    data.push_back(0x03);
    data.push_back(0x00);
    data.push_back(0x10);
    data.push_back(0x00);
    data.push_back(0x10);
    data.push_back(0xff);
    data.push_back(0xff);
    return quiky::Area::parse(data, "W1L1.ARE");
}

quiky::Map makeMap() {
    quiky::Map map;
    map.width = 16;
    map.height = 8;
    map.unknown = 0;
    map.cells.assign(static_cast<std::size_t>(map.width) * map.height, 0);
    return map;
}

void testBieneAcquiresAndStingsWithoutReplayTable() {
    const quiky::Map map = makeMap();
    const quiky::WorldCollisionView world(map);
    quiky::LevelSessionConfig config;
    config.hasSpawn = true;
    config.spawnX = 16;
    config.spawnY = 80;
    config.streamRadiusRegions = 1;
    config.enableEdgeExit = false;

    quiky::LevelSession session("W1L1.MAP", map, makeBieneArea(), config);
    quiky::Simulation simulation;
    quiky::SimulationOutput output;
    session.reset(simulation);
    session.updateStreaming(simulation, 16, 80);

    quiky::LevelEntity &biene = session.entitiesForSetup()[0];
    biene.x = 56;
    biene.y = 36;
    biene.positionX = quiky::Fixed16::fromPixels(56);
    biene.positionY = quiky::Fixed16::fromPixels(36);
    biene.enemyOriginY36 = biene.positionY.raw;

    quiky::PlayerRecord &player = simulation.stateForSetup().player;
    player.positionX = quiky::Fixed16::fromPixels(16);
    player.positionY = quiky::Fixed16::fromPixels(80);
    player.state2C = -10;
    player.verticalStepOrDirection2E = -40;
    player.state30 = 10;
    player.callbackState32 = 0;
    player.syncToRaw();

    session.tick(simulation, world, quiky::InputState(), output);
    assert(biene.enemyState == 1);

    bool stung = false;
    for (int frame = 0; frame < 80; ++frame) {
        session.tick(simulation, world, quiky::InputState(), output);
        while (session.hasPendingEvents()) {
            if (session.consumeEvent().type == quiky::LevelEventType::PlayerDamaged) {
                stung = true;
            }
        }
        if (stung) {
            break;
        }
    }
    assert(stung);
    assert(session.gameplayState().currentHealth8822 == 2);
    assert(player.timer34 == 0x00d2);
}

} // namespace

int main() {
    testBieneAcquiresAndStingsWithoutReplayTable();
    std::cout << "biene sight and sting passed\n";
    return 0;
}
