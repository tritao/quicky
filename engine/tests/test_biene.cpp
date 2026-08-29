#include "quiky/area.h"
#include "quiky/level.h"
#include "quiky/map.h"
#include "quiky/simulation.h"
#include "quiky/world_view.h"

#include <array>
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

void testBieneUsesGeneratedRuntimeTable() {
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

    // 0A43's generated table drives the recovered state-1 path. The first
    // callback consumes phase 0x20 and then falls through the retail state
    // transition chain; no table-less attack behavior is involved.
    session.tick(simulation, world, quiky::InputState(), output);
    assert(biene.enemyAux3e == 0x20);
    assert(biene.enemyPhase34 == 2);
    assert(biene.enemyState == 7);
}

void testBieneRuntimeTableMatchesNativeCapture() {
    const std::array<std::int8_t, quiky::kBieneRuntimeTableSize> table =
        quiky::generateBieneRuntimeTable();
    const int expected[] = {
        0, 0, 1, 1, 90, 127, 127, 90,
        0, 0, 0, -127, -127, -90, -90, 0,
    };
    const std::size_t indices[] = {
        0x000, 0x001, 0x002, 0x003, 0x100, 0x1ff, 0x200, 0x300,
        0x3ff, 0x400, 0x401, 0x5ff, 0x600, 0x6ff, 0x700, 0x7ff,
    };
    for (std::size_t index = 0; index < sizeof(indices) / sizeof(indices[0]);
         ++index) {
        assert(table[indices[index]] == expected[index]);
    }
}

} // namespace

int main() {
    testBieneRuntimeTableMatchesNativeCapture();
    testBieneUsesGeneratedRuntimeTable();
    std::cout << "biene runtime table and state path passed\n";
    return 0;
}
