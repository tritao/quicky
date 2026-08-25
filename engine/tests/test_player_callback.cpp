#include "quiky/player_update.h"

#include <cassert>
#include <cstdint>
#include <iostream>

namespace {

quiky::Map makeMap() {
    quiky::Map map;
    map.width = 4;
    map.height = 32;
    map.unknown = 0;
    map.cells.assign(static_cast<std::size_t>(map.width) * map.height, 0);
    return map;
}

void setCell(quiky::Map &map,
             std::uint16_t x,
             std::uint16_t y,
             std::uint16_t tile,
             std::uint16_t properties = 0) {
    map.cells[static_cast<std::size_t>(y) * map.width + x] =
        static_cast<std::uint16_t>(tile | properties);
}

quiky::PlayerRecord playerAt() {
    quiky::PlayerRecord player;
    player.initializeConfirmedHorizontalFields();
    player.positionX = quiky::Fixed16::fromPixels(32);
    player.positionY = quiky::Fixed16::fromPixels(400);
    player.syncToRaw();
    return player;
}

void testJumpInitiationAndProbeOrder() {
    quiky::Map map = makeMap();
    setCell(map, 2, 25, 1);
    quiky::PlayerDescriptorTable descriptors;
    descriptors.setWord(1, 0x0060);
    const quiky::WorldCollisionView world(map, &descriptors);

    quiky::TraceClosedPlayerUpdate updater;
    quiky::PlayerRecord player = playerAt();
    quiky::PlayerUpdateTrace trace;
    updater.updatePlayer(player, quiky::InputState::fromActionFlags(0x20),
                          world, &trace);

    assert(player.mode37 == -1);
    assert(player.velocityY.raw == player.negativeYSpeed64.raw);
    assert(player.resetDeathTimer3E == 0x03e8);
    assert(!trace.effectDispatches.empty());
    assert(trace.effectDispatches[0].address == 0x01e70fcfU);
    assert(trace.collisionProbes.size() >= 3);
    assert(trace.collisionProbes[0].pixelX == 27);
    assert(trace.collisionProbes[0].pixelY == 400);
    assert(trace.collisionProbes[1].pixelX == 37);
    assert(trace.collisionProbes[1].pixelY == 400);
}

void testLandingAndCeilingResponses() {
    quiky::Map landingMap = makeMap();
    setCell(landingMap, 2, 25, 1);
    quiky::PlayerDescriptorTable descriptors;
    descriptors.setWord(1, 0x0060);
    const quiky::WorldCollisionView landingWorld(landingMap, &descriptors);

    quiky::TraceClosedPlayerUpdate updater;
    quiky::PlayerRecord landing = playerAt();
    landing.mode37 = 1;
    landing.velocityY.raw = 0x1000;
    landing.syncToRaw();
    updater.updatePlayer(landing, quiky::InputState(), landingWorld, 0);
    assert(landing.mode37 == 0);
    assert(landing.velocityY.raw == 0);
    assert(landing.verticalResponse3A == -1);

    quiky::Map ceilingMap = makeMap();
    setCell(ceilingMap, 2, 22, 1, 0x1000);
    const quiky::WorldCollisionView ceilingWorld(ceilingMap, &descriptors);
    quiky::PlayerRecord ceiling = playerAt();
    ceiling.mode37 = -1;
    ceiling.velocityY.raw = -0x18000;
    ceiling.syncToRaw();
    updater.updatePlayer(ceiling, quiky::InputState(), ceilingWorld, 0);
    assert(ceiling.mode37 == 1);
    assert(ceiling.velocityY.raw == 0);
    assert(ceiling.resetDeathTimer3E == 0x03e7);
}

} // namespace

int main() {
    testJumpInitiationAndProbeOrder();
    testLandingAndCeilingResponses();
    std::cout << "player callback contact tests passed\n";
    return 0;
}
