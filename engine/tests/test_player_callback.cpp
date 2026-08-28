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
    assert(player.field1E == 8);
    assert(player.animationDelay20 == 7);
    assert(player.animationCursor22 == 0x3162);
    assert(player.field24 == 0x3162);
    assert(player.statusWord12 == 10);
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
    updater.globalsForSetup().idleCounter4FEE = 0;
    landing.mode37 = 1;
    // The positive-mode callback's initial 41F7/4209 probes do not publish
    // +0x3B.  A descending contact reaches 3D02 with the latch already set
    // by the preceding 4278 side test, so model that incoming state here.
    landing.sideResponse3B = 0xff;
    landing.velocityY.raw = 0x1000;
    landing.syncToRaw();
    updater.updatePlayer(landing, quiky::InputState(), landingWorld, 0);
    assert(landing.mode37 == 0);
    assert(landing.velocityY.raw == 0);
    assert(landing.verticalResponse3A == -1);
    assert(landing.field1E == 4);
    assert(landing.animationDelay20 == 3);
    assert(landing.animationCursor22 == 0x3158);
    assert(landing.field24 == 0x3158);
    assert(landing.statusWord12 == 0);

    // 3AB9 reloads the idle descriptor before 5D60 while the idle counter is
    // below its long-idle boundary. The following ordinary callback therefore
    // preserves the native post-landing delay of 3.
    updater.updatePlayer(landing, quiky::InputState(), landingWorld, 0);
    assert(landing.field1E == 4);
    assert(landing.animationDelay20 == 3);
    assert(landing.animationCursor22 == 0x3158);

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
    assert(ceiling.field1E == 20);
    assert(ceiling.animationDelay20 == 19);
    assert(ceiling.animationCursor22 == 0x3188);
    assert(ceiling.field24 == 0x3188);
    assert(ceiling.statusWord12 == 13);
}

void testPostStepContactUsesCurrentRecordCoordinates() {
    quiky::Map map = makeMap();
    setCell(map, 2, 25, 45);
    quiky::PlayerDescriptorTable descriptors;
    descriptors.setWord(45, 0x000c);
    const quiky::WorldCollisionView world(map, &descriptors);

    quiky::TraceClosedPlayerUpdate updater;
    quiky::PlayerRecord player = playerAt();
    player.positionY.raw = (397 << 16) + 0x4000;
    player.mode37 = 1;
    // Model the incoming side-contact latch consumed by 3DF2 after the
    // post-step probe; 41F7/4209 itself does not create this latch.
    player.sideResponse3B = 0xff;
    player.velocityY.raw = 0x00039800;
    player.syncToRaw();
    updater.updatePlayer(player, quiky::InputState(), world, 0);

    // The post-step side probe must see the newly integrated Y (around 401),
    // then apply the native eight-pixel contact correction to 400.
    assert(player.mode37 == 0);
    assert(player.yPixel() == 400);
    assert(player.velocityY.raw == 0);
}

void testCommonTailClosedAnimationAndViewCopy() {
    quiky::Map map = makeMap();
    setCell(map, 2, 25, 45);
    quiky::PlayerDescriptorTable descriptors;
    descriptors.setWord(45, 0x000c);
    const quiky::WorldCollisionView world(map, &descriptors);

    quiky::TraceClosedPlayerUpdate updater;
    updater.globalsForSetup().viewStateA4FE4 = 0x1234;
    updater.globalsForSetup().viewStateB4FE6 = 0;
    updater.globalsForSetup().idleCounter4FEE = 0x00d3;

    quiky::PlayerRecord player = playerAt();
    player.field1E = 3;
    player.animationDelay20 = 3;
    player.animationCursor22 = 0x3162;
    player.field24 = 0x3162;
    player.syncToRaw();
    quiky::PlayerUpdateTrace trace;
    updater.updatePlayer(player, quiky::InputState(), world, &trace);

    assert(player.animationDelay20 == 2);
    assert(player.field24 == 0x3162);
    assert(updater.globals().viewStateB4FE6 == 0x1234);
    assert(updater.globals().idleCounter4FEE == 0x00d4);
    assert(trace.globalWrites.size() == 2);
    assert(trace.globalWrites[0].address == 0x4fe6);
    assert(trace.globalWrites[0].before == 0);
    assert(trace.globalWrites[0].after == 0x1234);
    assert(trace.globalWrites[1].address == 0x4fee);
    assert(trace.globalWrites[1].before == 0x00d3);
    assert(trace.globalWrites[1].after == 0x00d4);
}

void testAnimationStreamContinuesPast3142DescriptorLabel() {
    quiky::Map map = makeMap();
    quiky::PlayerDescriptorTable descriptors;
    const quiky::WorldCollisionView world(map, &descriptors);

    quiky::TraceClosedPlayerUpdate updater;
    quiky::PlayerRecord player = playerAt();
    player.mode37 = 1;
    player.velocityY.raw = 0;
    player.positiveYAcceleration50.raw = 0;
    player.field1E = 4;
    player.animationDelay20 = 0;
    player.animationCursor22 = 0x3144;
    player.field24 = 0x3150;
    player.syncToRaw();

    updater.updatePlayer(player, quiky::InputState(), world, 0);

    // 01F7:5D60 reads the current cursor word at 3150, advances to 3152,
    // and reloads the descriptor delay.  The 3152 word is part of the raw
    // 3142 stream even though 3156 is the next loader entry point.
    assert(player.statusWord12 == 7);
    assert(player.animationDelay20 == 4);
    assert(player.animationCursor22 == 0x3144);
    assert(player.field24 == 0x3152);
}

void testStaticHorizontalAnimationDescriptors() {
    quiky::Map map = makeMap();
    quiky::PlayerDescriptorTable descriptors;
    const quiky::WorldCollisionView world(map, &descriptors);

    quiky::TraceClosedPlayerUpdate updater;
    quiky::PlayerRecord turning = playerAt();
    turning.animationState36 = 1;
    turning.verticalResponse3A = 1;
    turning.velocityX.raw = 0x00001000;
    turning.syncToRaw();
    updater.updatePlayer(turning, quiky::InputState::fromActionFlags(0x0004),
                         world, 0);
    assert(turning.field1E == 4);
    assert(turning.animationCursor22 == 0x3144);
    assert(turning.field24 == 0x3144);
    assert(turning.statusWord12 == 0);

    quiky::PlayerRecord running = playerAt();
    running.verticalResponse3A = 1;
    running.velocityX.raw = 0x00028000;
    running.horizontalSpeedCap5C.raw = 0x00030000;
    running.syncToRaw();
    updater.updatePlayer(running, quiky::InputState::fromActionFlags(0x0004),
                         world, 0);
    assert(running.field1E == 2);
    assert(running.animationCursor22 == 0x3192);
    assert(running.field24 == 0x3192);
    assert(running.statusWord12 == 0);
}

void test5937PublishesAddressQualifiedCountState() {
    quiky::Map map = makeMap();
    quiky::PlayerDescriptorTable descriptors;
    const quiky::WorldCollisionView world(map, &descriptors);

    quiky::TraceClosedPlayerUpdate updater;
    updater.globalsForSetup().dispatchDisplayCount8822 = 3;
    updater.globalsForSetup().dispatchPublishedCount4FF8 = 1;
    updater.globalsForSetup().dispatchWord60D8 = 0x0020;
    updater.globalsForSetup().dispatchPreviousWord60DA = 0;

    quiky::PlayerRecord player = playerAt();
    quiky::PlayerUpdateTrace trace;
    updater.updatePlayer(player, quiky::InputState(), world, &trace);

    // 01F7:5937 runs before the movement callback and publishes one display
    // count step plus the changed dispatch word. Nested 386F/0598 calls are
    // intentionally outside this simulation trace.
    assert(updater.globals().dispatchPublishedCount4FF8 == 2);
    assert(updater.globals().dispatchPreviousWord60DA == 0x0020);
    assert(trace.globalWrites.size() >= 2);
    assert(trace.globalWrites[0].address == 0x60da);
    assert(trace.globalWrites[0].before == 0);
    assert(trace.globalWrites[0].after == 0x0020);
    assert(trace.globalWrites[1].address == 0x4ff8);
    assert(trace.globalWrites[1].before == 1);
    assert(trace.globalWrites[1].after == 2);
}

} // namespace

int main() {
    testJumpInitiationAndProbeOrder();
    testLandingAndCeilingResponses();
    testPostStepContactUsesCurrentRecordCoordinates();
    testCommonTailClosedAnimationAndViewCopy();
    testAnimationStreamContinuesPast3142DescriptorLabel();
    testStaticHorizontalAnimationDescriptors();
    test5937PublishesAddressQualifiedCountState();
    std::cout << "player callback contact tests passed\n";
    return 0;
}
