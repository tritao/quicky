#include "quiky/area.h"
#include "quiky/archive.h"
#include "quiky/binary_reader.h"
#include "quiky/bob.h"
#include "quiky/level.h"
#include "quiky/map.h"
#include "quiky/palette.h"
#include "quiky/player_animation.h"
#include "quiky/player_record.h"
#include "quiky/player_update.h"
#include "quiky/renderer.h"
#include "quiky/runtime.h"
#include "quiky/simulation.h"
#include "quiky/tileset.h"
#include "quiky/world_view.h"

#include <algorithm>
#include <cassert>
#include <cstdint>
#include <exception>
#include <iostream>
#include <sstream>

namespace {

void appendU16LE(quiky::Bytes &data, std::uint16_t value) {
    data.push_back(static_cast<quiky::byte>(value & 0xff));
    data.push_back(static_cast<quiky::byte>(value >> 8));
}

void appendU32LE(quiky::Bytes &data, std::uint32_t value) {
    appendU16LE(data, static_cast<std::uint16_t>(value));
    appendU16LE(data, static_cast<std::uint16_t>(value >> 16));
}

void appendU16BE(quiky::Bytes &data, std::uint16_t value) {
    data.push_back(static_cast<quiky::byte>(value >> 8));
    data.push_back(static_cast<quiky::byte>(value & 0xff));
}

void writeU16BEAt(quiky::Bytes &data, std::size_t offset,
                  std::uint16_t value) {
    data[offset] = static_cast<quiky::byte>(value >> 8);
    data[offset + 1] = static_cast<quiky::byte>(value & 0xff);
}

quiky::Bytes makeArchive() {
    const std::string firstName = "FIRST.MAP";
    const std::string secondName = "SECOND.ICO";
    quiky::Bytes data = {'a', 'b', 'c'};
    const std::size_t directoryOffset = data.size();
    appendU16LE(data, static_cast<std::uint16_t>(firstName.size()));
    data.insert(data.end(), firstName.begin(), firstName.end());
    appendU32LE(data, 0);
    appendU16LE(data, static_cast<std::uint16_t>(secondName.size()));
    data.insert(data.end(), secondName.begin(), secondName.end());
    appendU32LE(data, 3);
    appendU32LE(data, static_cast<std::uint32_t>(directoryOffset));
    appendU32LE(data, 1);
    return data;
}

quiky::Map makeMap(std::uint16_t width, std::uint16_t height,
                   std::uint16_t word = 0) {
    quiky::Map map;
    map.width = width;
    map.height = height;
    map.unknown = 0;
    map.cells.assign(static_cast<std::size_t>(width) * height, word);
    return map;
}

quiky::Area makeArea(std::uint16_t firstType, std::uint16_t secondType) {
    quiky::Bytes data(0x14e8, 0);
    writeU16BEAt(data, 0x0e, 2);
    writeU16BEAt(data, 0x10, 1);
    writeU16BEAt(data, 0x160, 0x1388);
    writeU16BEAt(data, 0x162, 0x1390);
    appendU16BE(data, firstType);
    appendU16BE(data, 16);
    appendU16BE(data, 16);
    appendU16BE(data, 0xffff);
    appendU16BE(data, secondType);
    appendU16BE(data, 16);
    appendU16BE(data, 16);
    appendU16BE(data, 0xffff);
    return quiky::Area::parse(data, "W1L1.ARE");
}

quiky::Area makeSingleArea(std::uint16_t type) {
    quiky::Bytes data(0x14e8, 0);
    writeU16BEAt(data, 0x0e, 2);
    writeU16BEAt(data, 0x10, 1);
    writeU16BEAt(data, 0x160, 0x1388);
    writeU16BEAt(data, 0x162, 0xffff);
    appendU16BE(data, type);
    appendU16BE(data, 16);
    appendU16BE(data, 16);
    appendU16BE(data, 0xffff);
    return quiky::Area::parse(data, "W1L1.ARE");
}

void forceCollectiblePlayerBounds(quiky::Simulation &simulation) {
    // The controlled DOS overlap fixtures publish these exact callback
    // bounds before entering 8D20. Keep the native fixture independent of
    // the ordinary movement setup while exercising the recovered predicate.
    quiky::PlayerRecord &player = simulation.stateForSetup().player;
    player.state2C = -10;
    player.verticalStepOrDirection2E = -40;
    player.state30 = 10;
    player.callbackState32 = 0;
    player.syncToRaw();
}

void testReader() {
    const quiky::Bytes data = {0x12, 0x34, 0x56, 0x78, 0x9a, 0xbc};
    quiky::BinaryReader reader(data, "reader test");
    assert(reader.readU16LE() == 0x3412);
    assert(reader.readU16BE() == 0x5678);
    assert(reader.readU16LE() == 0xbc9a);
    bool failed = false;
    try {
        reader.readU8();
    } catch (const quiky::FormatError &) {
        failed = true;
    }
    assert(failed);
}

void testArchive() {
    const quiky::Archive archive = quiky::Archive::fromBytes(makeArchive(), "test");
    assert(archive.entries().size() == 2);
    assert(archive.find("first.map").size == 3);
    assert(archive.read("FIRST.MAP") == quiky::Bytes({'a', 'b', 'c'}));
}

void testFormatsAndRenderer() {
    quiky::Bytes paletteData(128, 0);
    paletteData[0] = 0x0a;
    paletteData[3] = 0x08;
    paletteData.push_back(0x0c);
    for (std::size_t index = 0; index < 256; ++index) {
        paletteData.push_back(static_cast<quiky::byte>(index));
        paletteData.push_back(static_cast<quiky::byte>(index + 1));
        paletteData.push_back(static_cast<quiky::byte>(index + 2));
    }
    const quiky::Palette palette =
        quiky::Palette::parsePcx(paletteData, "test.PCC");
    assert(palette.colors[3].red == 3);
    assert(palette.colors[3].green == 4);
    assert(palette.colors[3].blue == 5);

    quiky::Bytes mapData = {'T', 'L', 'E', '1'};
    appendU16BE(mapData, 2);
    appendU16BE(mapData, 1);
    appendU16BE(mapData, 9);
    appendU16BE(mapData, 0);
    appendU16BE(mapData, 1);
    const quiky::Map map = quiky::Map::parse(mapData, "test.MAP");
    quiky::Bytes icoData(512, 0);
    for (std::size_t index = 0; index < 256; ++index) {
        icoData[index] = static_cast<quiky::byte>(index);
        icoData[256 + index] = 7;
    }
    const quiky::Tileset tileset = quiky::Tileset::parseIco(icoData, "test.ICO");
    quiky::IndexedSurface surface = quiky::renderMap(map, tileset);
    assert(surface.width == 32 && surface.height == 16);
    assert(surface.at(0, 0) == 0 && surface.at(1, 0) == 4);
    assert(surface.at(16, 0) == 7);
    quiky::drawIcoTile(surface, tileset, 1, 0, 0);
    assert(surface.at(0, 0) == 7);
}

void testGameplayFrameComposition() {
    quiky::IndexedSurface world(640, 352);
    for (std::uint32_t y = 0; y < world.height; ++y) {
        for (std::uint32_t x = 0; x < world.width; ++x) {
            world.at(x, y) = static_cast<quiky::byte>((x + y) & 0xff);
        }
    }
    quiky::IndexedSurface gamebar(320, 24);
    std::fill(gamebar.pixels.begin(), gamebar.pixels.end(), 0xa5);
    gamebar.at(0, 0) = 0x11;
    gamebar.at(319, 23) = 0x22;
    const quiky::IndexedSurface frame =
        quiky::composeGameplayFrame(world, gamebar, 100, 80);
    assert(frame.width == 320 && frame.height == 200);
    assert(frame.at(0, 0) == static_cast<quiky::byte>(180));
    assert(frame.at(319, 175) == static_cast<quiky::byte>(162));
    assert(frame.at(0, 176) == 0x11);
    assert(frame.at(1, 176) == 0xa5);
    assert(frame.at(319, 199) == 0x22);

    quiky::IndexedSurface zeroBar(320, 24);
    const quiky::IndexedSurface zeroFrame =
        quiky::composeGameplayFrame(world, zeroBar, 0, 0);
    assert(zeroFrame.at(10, 175) == 185);
    assert(zeroFrame.at(10, 176) == 0);
}

void testAreaAndBob() {
    quiky::Area area = makeSingleArea(0x2b);
    assert(area.layoutWidth == 2 && area.layoutHeight == 1);
    assert(area.references.size() == 1);
    assert(area.placements().size() == 1);
    assert(area.placements()[0].worldX == 16);

    const quiky::Bytes code = {
        0xee, 0xd0, 0xc0, 0xc6, 0x84, 0x00, 0x00, 0x01,
        0xee, 0xd0, 0xc0, 0xc6, 0x84, 0x00, 0x00, 0x02,
        0x58, 0x5e, 0xcb,
    };
    quiky::Bytes bobData;
    appendU16LE(bobData, 7);
    appendU16LE(bobData, 1);
    appendU16LE(bobData, 2);
    appendU16LE(bobData, 2);
    appendU16LE(bobData, 1);
    appendU16LE(bobData, 4);
    appendU16LE(bobData, 0);
    appendU16LE(bobData, 8);
    appendU16LE(bobData, static_cast<std::uint16_t>(code.size()));
    bobData.insert(bobData.end(), code.begin(), code.end());
    const quiky::Bob bob = quiky::Bob::parse(bobData, "TEST.BOB");
    assert(bob.records.size() == 1);
    const std::vector<std::int16_t> pixels = quiky::decodeBobRecord(bob.records[0]);
    assert(pixels.size() == 2 && pixels[0] == 1 && pixels[1] == 2);
}

void testFaithfulRecordWorldAndAnimation() {
    quiky::Bytes bytes(quiky::PlayerRawRecord::kSize);
    for (std::size_t index = 0; index < bytes.size(); ++index) {
        bytes[index] = static_cast<quiky::byte>((index * 37) ^ 0xa5);
    }
    const quiky::PlayerRecord record = quiky::PlayerRecord::fromBytes(bytes);
    assert(record.toBytes() == bytes);
    assert(record.statusWord() == record.raw.u16(0x12));
    assert(record.xPixel() == record.raw.s16(0x04));
    assert(record.viewAnchorX() == record.raw.s16(0x4a));

    quiky::Map map = makeMap(2, 1);
    map.cells[0] = static_cast<std::uint16_t>(0x0012 | (0x55 << 9));
    quiky::PlayerDescriptorTable descriptors;
    descriptors.setWord(0x12, 0x0068);
    const quiky::WorldCollisionView world(map, &descriptors);
    assert(world.cellAt(0, 0).flags == 0x55);
    assert(world.descriptorFor(world.cellAt(0, 0)).descriptorWord == 0x0068);
    assert(world.blocksProbeConfirmed(3, 3));
    assert(world.hasVerticalResponseConfirmed(3, 3));
    assert(world.alignsEightPixelsConfirmed(3, 3));

    const quiky::Map rawMap = makeMap(1, 1, 0x4000);
    const quiky::WorldCollisionView rawWorld(rawMap);
    assert(rawWorld.mapRawBit4000Confirmed(3, 3));

    quiky::PlayerRecord player;
    player.mode37 = 0;
    player.motionDirectionByte29 = 1;
    quiky::PlayerAnimation animation;
    animation.advance(player);
    assert(animation.slot() == 0);
    player.mode37 = -1;
    animation.advance(player);
    assert(animation.slot() == 10);
    player.motionDirectionByte29 = 0xff;
    animation.advance(player);
    assert(animation.slot() == 60);
}

void testLevelSessionUsesSimulationBoundary() {
    const quiky::Map map = makeMap(16, 8);
    quiky::LevelSessionConfig config;
    config.hasSpawn = true;
    config.spawnX = 16;
    config.spawnY = 32;
    config.streamRadiusRegions = 0;
    config.enableEdgeExit = false;
    quiky::LevelSession session("W1L1.MAP", map, makeArea(0x6f, 0x79), config);
    quiky::Simulation simulation;
    quiky::TraceClosedPlayerUpdate updater;
    simulation.setExperimentalPlayerUpdater(&updater);
    quiky::SimulationOutput output;
    session.reset(simulation);
    forceCollectiblePlayerBounds(simulation);
    session.updateStreaming(simulation,
                            simulation.state().player.positionX.floorPixels(),
                            simulation.state().player.positionY.floorPixels());
    assert(session.entities()[0].active);
    assert(session.entities()[0].spriteSlot == 607);
    assert(session.entities()[1].phase == quiky::EntityPhase::Dormant);

    const quiky::WorldCollisionView world(map);
    session.tick(simulation, world, quiky::InputState(), output);
    const quiky::LevelEvent collected = session.consumeEvent();
    assert(collected.type == quiky::LevelEventType::Collected);
    assert(collected.entityType == 0x6f);
    assert(collected.stateWrites.size() == 3);
    assert(collected.stateWrites[0].address == 0x880c);
    assert(collected.stateWrites[0].after == 10);
    assert(collected.stateWrites[1].address == 0x612e);
    assert(collected.stateWrites[1].after == 9);
    assert(collected.stateWrites[2].address == 0x881c);
    assert(collected.stateWrites[2].after == 50);
    assert(session.score() == 50);
    assert(session.gameplayState().ammo880c == 10);
    assert(session.gameplayState().pendingEvent612e == 9);
    assert(output.schedulerCallbacks.size() == 1);
    assert(output.schedulerCallbacks[0].callback.segment == 0x01f7);
    assert(output.schedulerCallbacks[0].callback.offset == 0x8d20);
    assert(output.player.toBytes() == simulation.state().player.toBytes());

    quiky::LevelSessionConfig exitConfig = config;
    exitConfig.spawnX = 232;
    exitConfig.enableEdgeExit = true;
    quiky::LevelSession exitSession("W1L1.MAP", map,
                                    makeArea(0x28, 0x28), exitConfig);
    exitSession.reset(simulation);
    exitSession.tick(simulation, world, quiky::InputState(), output);
    const quiky::LevelEvent exitEvent = exitSession.consumeEvent();
    assert(exitEvent.type == quiky::LevelEventType::LevelExit);
    assert(exitEvent.targetLevel == "W1L2.MAP");
}

void testW1L1NativeDefaultSpawn() {
    const quiky::Map map = makeMap(270, 30);
    quiky::LevelSessionConfig config;
    config.enableEdgeExit = false;
    quiky::LevelSession session("w1l1.map", map, makeSingleArea(0), config);
    quiky::Simulation simulation;

    session.reset(simulation);

    assert(simulation.state().player.positionX.floorPixels() == 128);
    assert(simulation.state().player.positionY.floorPixels() == 400);
}

void testW1L2NativeStartupState() {
    const quiky::Map map = makeMap(270, 40);
    quiky::LevelSessionConfig config;
    config.enableEdgeExit = false;
    quiky::LevelSession session("w1l2.map", map, makeSingleArea(0), config);
    quiky::Simulation simulation;
    quiky::PlayerDescriptorTable descriptors;
    const quiky::WorldCollisionView world(map, &descriptors);
    quiky::TraceClosedPlayerUpdate updater;
    simulation.setExperimentalPlayerUpdater(&updater);
    quiky::PlayerUpdateTrace trace;
    simulation.setPlayerTraceSink(&trace);

    session.reset(simulation);

    // Natural DOS startup capture: player callback entry at (240,512),
    // DS:8822=3 and DS:8824=3. Camera (80,374) remains an explicit stream
    // anchor supplied by the trace/frontend boundary.
    assert(simulation.state().player.positionX.floorPixels() == 240);
    assert(simulation.state().player.positionY.floorPixels() == 512);
    assert(session.gameplayState().currentHealth8822 == 3);
    assert(session.gameplayState().maximumHealth8824 == 3);

    quiky::SimulationOutput output;
    session.tick(simulation, world, quiky::InputState(), output);
    assert(trace.globalWrites.size() >= 1);
    assert(trace.globalWrites[0].address == 0x4ff8);
    assert(trace.globalWrites[0].before == 1);
    assert(trace.globalWrites[0].after == 2);
}

void testRecoveredCollectibleStateContracts() {
    const quiky::Map map = makeMap(16, 8);
    const quiky::WorldCollisionView world(map);
    quiky::LevelSessionConfig config;
    config.hasSpawn = true;
    config.spawnX = 16;
    config.spawnY = 32;
    config.streamRadiusRegions = 0;
    config.enableEdgeExit = false;

    quiky::Simulation simulation;
    quiky::TraceClosedPlayerUpdate updater;
    simulation.setExperimentalPlayerUpdater(&updater);
    quiky::SimulationOutput output;

    quiky::LevelSession healthMax("W1L1.MAP", map, makeSingleArea(0x70), config);
    assert(healthMax.entities()[0].x == 21);
    assert(healthMax.entities()[0].y == 26);
    healthMax.reset(simulation);
    forceCollectiblePlayerBounds(simulation);
    healthMax.gameplayStateForSetup().currentHealth8822 = 3;
    healthMax.gameplayStateForSetup().maximumHealth8824 = 3;
    healthMax.updateStreaming(simulation, 16, 16);
    healthMax.tick(simulation, world, quiky::InputState(), output);
    quiky::LevelEvent event = healthMax.consumeEvent();
    assert(event.type == quiky::LevelEventType::Collected);
    assert(healthMax.gameplayState().currentHealth8822 == 4);
    assert(healthMax.gameplayState().maximumHealth8824 == 4);
    assert(healthMax.score() == 250);

    quiky::LevelSession health("W1L1.MAP", map, makeSingleArea(0x71), config);
    assert(health.entities()[0].x == 21);
    assert(health.entities()[0].y == 26);
    health.reset(simulation);
    forceCollectiblePlayerBounds(simulation);
    health.gameplayStateForSetup().currentHealth8822 = 3;
    health.gameplayStateForSetup().maximumHealth8824 = 5;
    health.updateStreaming(simulation, 16, 16);
    health.tick(simulation, world, quiky::InputState(), output);
    event = health.consumeEvent();
    assert(event.type == quiky::LevelEventType::Collected);
    assert(health.gameplayState().currentHealth8822 == 4);
    assert(health.gameplayState().maximumHealth8824 == 5);
    assert(health.score() == 100);

    quiky::LevelSession invulnerability("W1L1.MAP", map,
                                        makeSingleArea(0x72), config);
    assert(invulnerability.entities()[0].x == 19);
    assert(invulnerability.entities()[0].y == 23);
    invulnerability.reset(simulation);
    forceCollectiblePlayerBounds(simulation);
    invulnerability.updateStreaming(simulation, 16, 16);
    invulnerability.tick(simulation, world, quiky::InputState(), output);
    event = invulnerability.consumeEvent();
    assert(event.type == quiky::LevelEventType::Collected);
    assert(invulnerability.gameplayState().invulnerabilityGate8810 == 0xffff);
    assert(invulnerability.gameplayState().playerTimer0034 == 0x02bc);
    assert(invulnerability.gameplayState().pendingEvent612e == 12);
    assert(simulation.state().player.timer34 == 0x02bc);
    assert(invulnerability.score() == 150);

    quiky::LevelSession letter("W1L1.MAP", map, makeSingleArea(0x79), config);
    letter.reset(simulation);
    forceCollectiblePlayerBounds(simulation);
    letter.updateStreaming(simulation, 16, 16);
    letter.tick(simulation, world, quiky::InputState(), output);
    event = letter.consumeEvent();
    assert(event.type == quiky::LevelEventType::Collected);
    assert(letter.gameplayState().puzzleMask60d8 == 1);
    assert(letter.score() == 100);
}

void testRecoveredCollectibleStrictBounds() {
    const quiky::Map map = makeMap(16, 8);
    const quiky::WorldCollisionView world(map);
    quiky::LevelSessionConfig config;
    config.hasSpawn = true;
    config.spawnX = 16;
    config.spawnY = 32;
    config.streamRadiusRegions = 0;
    config.enableEdgeExit = false;

    auto run = [&](std::int32_t objectX, std::int32_t objectY,
                   std::uint16_t transitionGate = 0) {
        quiky::Simulation simulation;
        quiky::TraceClosedPlayerUpdate updater;
        simulation.setExperimentalPlayerUpdater(&updater);
        quiky::SimulationOutput output;
        quiky::LevelSession session("W1L1.MAP", map,
                                    makeSingleArea(0x6f), config);
        session.reset(simulation);
        forceCollectiblePlayerBounds(simulation);
        session.gameplayStateForSetup().transitionGate89ea = transitionGate;
        session.updateStreaming(simulation, 16, 16);
        session.entitiesForSetup()[0].x = objectX;
        session.entitiesForSetup()[0].y = objectY;
        session.tick(simulation, world, quiky::InputState(), output);
        return session.consumeEvent();
    };

    // 8D31 uses JGE/JLE rejection, so equality at either horizontal edge
    // is not a hit. The player bounds are x=(6,26), y=(-8,32).
    assert(run(26, 14).type == quiky::LevelEventType::None);
    // 8D31 aligns the object Y down to a 16-pixel boundary and also rejects
    // equality at the upper edge of the returned player interval.
    assert(run(17, 32).type == quiky::LevelEventType::None);
    assert(run(17, 14, 0xffff).type == quiky::LevelEventType::None);
    assert(run(17, 14).type == quiky::LevelEventType::Collected);
}

void testRecoveredW1L1EnemyFamilies() {
    const quiky::Map map = makeMap(16, 8);
    const quiky::WorldCollisionView world(map);
    quiky::LevelSessionConfig config;
    config.hasSpawn = true;
    config.spawnX = 16;
    config.spawnY = 16;
    config.streamRadiusRegions = 1;
    config.enableEdgeExit = false;

    quiky::LevelSession session("W1L1.MAP", map, makeArea(0x01, 0x03), config);
    quiky::Simulation simulation;
    quiky::TraceClosedPlayerUpdate updater;
    simulation.setExperimentalPlayerUpdater(&updater);
    quiky::SimulationOutput output;
    session.reset(simulation);
    session.updateStreaming(simulation, 16, 16);

    assert(session.entities()[0].updateCallback.offset == 0x6dc4);
    assert(session.entities()[1].updateCallback.offset == 0x68c0);
    assert(session.entities()[0].y == 48);
    assert(session.entities()[1].y == 48);

    session.tick(simulation, world, quiky::InputState(), output);
    assert(output.schedulerCallbacks.size() == 2);
    assert(output.schedulerCallbacks[0].callback.offset == 0x6dc4);
    assert(output.schedulerCallbacks[1].callback.offset == 0x68c0);
    assert(output.playerDependencyOrder.size() == 3);
    assert(output.playerDependencyOrder[0].phase ==
           quiky::SimulationCallbackPhase::GameplayObjectBeforePlayer);
    assert(output.playerDependencyOrder[0].callback.offset == 0x6dc4);
    assert(output.playerDependencyOrder[1].phase ==
           quiky::SimulationCallbackPhase::GameplayObjectBeforePlayer);
    assert(output.playerDependencyOrder[1].callback.offset == 0x68c0);
    assert(output.playerDependencyOrder[2].phase ==
           quiky::SimulationCallbackPhase::PlayerUpdate);
    assert(output.playerDependencyOrder[2].callback.offset == 0x3ff8);
    assert(session.entities()[0].x == 14);
    assert(session.entities()[1].x == 78);
    assert(session.deaths() == 0);
    // WURM2/BIENE enter 01F7:1B77 before their movement callback. The
    // generic 4AB3/4C5D response belongs to the remaining normal families;
    // these two W1L1 families now use the recovered 19E6 damage contract.
    assert(!session.entities()[0].enemyContactPending);
    assert(session.entities()[0].contactCallback.offset == 0);
    assert(session.entities()[0].responseTimer == 0);
    assert(!session.entities()[1].enemyContactPending);
    assert(session.consumeEvent().type == quiky::LevelEventType::None);

    for (int frame = 0; frame < 40; ++frame) {
        session.tick(simulation, world, quiky::InputState(), output);
    }
    assert(session.entities()[0].active);
    assert(session.entities()[1].active);
    assert(!session.entities()[0].streamSuppressed);
    assert(!session.entities()[1].streamSuppressed);

    session.updateStreaming(simulation, 400, 400);
    session.tick(simulation, world, quiky::InputState(), output);
    assert(output.schedulerCallbacks.empty());
    assert(!session.entities()[0].active);
    assert(!session.entities()[1].active);
    assert(session.entities()[0].streamSuppressed);
    assert(session.entities()[1].streamSuppressed);

    session.updateStreaming(simulation, 16, 16);
    session.tick(simulation, world, quiky::InputState(), output);
    assert(output.schedulerCallbacks.empty());
    assert(!session.entities()[0].active);
    assert(!session.entities()[1].active);
}

void testRecoveredBumpCallbackContract() {
    const quiky::Map map = makeMap(16, 8);
    const quiky::WorldCollisionView world(map);
    quiky::LevelSessionConfig config;
    config.hasSpawn = true;
    config.spawnX = 16;
    config.spawnY = 44;
    config.streamRadiusRegions = 1;
    config.enableEdgeExit = false;

    quiky::LevelSession session("W1L2.MAP", map, makeSingleArea(0x34),
                                config);
    quiky::Simulation simulation;
    quiky::SimulationOutput output;
    session.reset(simulation);
    session.updateStreaming(simulation, 16, 44);
    assert(session.entities()[0].type == 0x34);
    assert(session.entities()[0].updateCallback.offset == 0x9c0c);
    assert(session.entities()[0].x == session.entities()[0].initialX + 0x10);
    assert(session.entities()[0].y == session.entities()[0].initialY + 0x20);
    assert(session.entities()[0].spriteSlot == 400);
    assert(session.entities()[0].bumpAnimationDelay20 == 6);

    // The BUMP range uses the persistent player's integer high words and
    // requires signed +0x37 > 0. No player updater is installed here so the
    // post-contact record can be checked before the next 3FF8 callback.
    quiky::PlayerRecord &player = simulation.stateForSetup().player;
    player.positionX = quiky::Fixed16::fromPixels(
        session.entities()[0].x);
    player.positionY = quiky::Fixed16::fromPixels(
        session.entities()[0].y - 4);
    player.mode37 = 1;
    player.negativeYSpeed64.raw = static_cast<std::int32_t>(0xfffb6000U);
    player.directionByte28 = 1;
    player.syncToRaw();

    session.tick(simulation, world, quiky::InputState(), output);
    const quiky::PlayerRecord &after = simulation.state().player;
    assert(after.mode37 == -1);
    assert(after.sideResponse3B == 0);
    assert(after.verticalResponse3A == 0);
    assert(after.resetDeathTimer3E == 0x03e8);
    assert(after.contactScratch2B == 0xff);
    assert(after.velocityY.raw ==
           quiky::Fixed16::wrapSubRaw(0xfffb6000, 0x0001b000));
    assert(after.animationDelay20 == 8);
    assert(after.animationCursor22 == 0x3162);
    assert(after.statusWord12 == 10);
    assert(session.gameplayState().pendingEvent612e == 4);
    const quiky::LevelEvent event = session.consumeEvent();
    assert(event.type == quiky::LevelEventType::EntityCollisionImpact);
    assert(event.entityType == 0x34);

    // The callback continues to run after contact; its descriptor reload
    // starts the seven-tick countdown again and does not remove the object.
    assert(session.entities()[0].active);
    assert(session.entities()[0].spriteSlot == 400);
    assert(session.entities()[0].bumpAnimationDelay20 == 6);

    quiky::LevelSession animationSession("W1L2.MAP", map,
                                         makeSingleArea(0x34), config);
    quiky::Simulation animationSimulation;
    quiky::SimulationOutput animationOutput;
    animationSession.reset(animationSimulation);
    animationSession.updateStreaming(animationSimulation, 16, 44);
    for (int frame = 1; frame <= 21; ++frame) {
        animationSession.tick(animationSimulation, world,
                               quiky::InputState(), animationOutput);
        const std::uint16_t expectedSlot =
            frame < 7 ? 400 : (frame < 14 ? 402 : (frame < 21 ? 403 : 401));
        assert(animationSession.entities()[0].spriteSlot == expectedSlot);
    }
}

void testRecoveredWurm2DescriptorProbeContract() {
    // 01F7:6DC4 uses the descriptor-backed 1C4D probe at (+0x28,-0x28)
    // followed by one direct 5C27 probe at X+/-0x26. The two descriptor
    // words below distinguish the oriented point from the direct side point
    // without relying on a host-side solid-tile abstraction.
    const quiky::Map map = makeMap(32, 16, 1);
    quiky::PlayerDescriptorTable descriptors;
    descriptors.setWord(1, 0x0008);
    const quiky::WorldCollisionView world(map, &descriptors);
    quiky::LevelSessionConfig config;
    config.hasSpawn = true;
    config.spawnX = 16;
    config.spawnY = 48;
    config.streamRadiusRegions = 1;
    config.enableEdgeExit = false;

    quiky::LevelSession session("W1L2.MAP", map, makeSingleArea(0x02),
                                config);
    quiky::Simulation simulation;
    quiky::SimulationOutput output;
    session.reset(simulation);
    session.updateStreaming(simulation, 16, 48);

    quiky::LevelEntity &wurm2 = session.entitiesForSetup()[0];
    wurm2.x = 128;
    wurm2.y = 128;
    wurm2.positionX = quiky::Fixed16::fromPixels(wurm2.x);
    wurm2.positionY = quiky::Fixed16::fromPixels(wurm2.y);
    // Keep the velocity below the clamp so the two static branches cannot
    // accidentally produce the same result.
    wurm2.velocityX.raw = 0x1000;
    session.tick(simulation, world, quiky::InputState(), output);
    // Descriptor 0x0008 leaves the oriented point clear (quadrant 0x0002)
    // and marks the direct side point occupied (quadrant 0x0008), so the
    // raw callback flag remains nonpositive and the ordinary 6F16 path is
    // taken, integrating the existing velocity.
    assert(wurm2.mapBlocked == 0);
    assert(wurm2.x == 128);
    assert(wurm2.velocityX.raw == 0x1000);

    descriptors.setWord(1, 0x0002);
    session.tick(simulation, world, quiky::InputState(), output);
    // The next descriptor word marks the oriented 1C4D point occupied. The
    // callback stores +0x2F=1 and takes its alternate state-0 path.
    assert(wurm2.mapBlocked == 1);
    assert(wurm2.x == 128);
    assert(wurm2.velocityX.raw == 0x1400);
}

void testRecoveredWurm2TargetTailContract() {
    // 01F7:707B scans the shared DS:87DE rows after every 6DC4 state path.
    // Use two rows so this covers a non-match, strict overlap, X-only clear,
    // cursor advancement, and the 4AB3 callback publication.
    const quiky::Map map = makeMap(32, 16, 0);
    const quiky::WorldCollisionView world(map);
    quiky::LevelSessionConfig config;
    config.hasSpawn = true;
    config.spawnX = 16;
    config.spawnY = 16;
    config.streamRadiusRegions = 1;
    config.enableEdgeExit = false;

    quiky::LevelSession session("W1L2.MAP", map, makeSingleArea(0x01),
                                config);
    quiky::Simulation simulation;
    quiky::TraceClosedPlayerUpdate updater;
    simulation.setExperimentalPlayerUpdater(&updater);
    quiky::SimulationOutput output;
    session.reset(simulation);
    session.updateStreaming(simulation, 16, 16);

    quiky::LevelEntity &wurm2 = session.entitiesForSetup()[0];
    wurm2.x = 100;
    wurm2.y = 100;
    wurm2.positionX = quiky::Fixed16::fromPixels(100);
    wurm2.positionY = quiky::Fixed16::fromPixels(100);
    wurm2.velocityX = quiky::Fixed16();
    wurm2.targetCursor30 = 0;
    quiky::LevelGameplayState &globals = session.gameplayStateForSetup();
    globals.sharedTargetActiveCount8806 = 1;
    globals.sharedTargetCapacity8808 = 2;
    globals.sharedTargetRows87de[0] = quiky::TargetCoordinateRow(300, 300);
    globals.sharedTargetRows87de[1] = quiky::TargetCoordinateRow(100, 100);

    session.tick(simulation, world, quiky::InputState(), output);
    assert(wurm2.targetCursor30 == 1);
    assert(globals.sharedTargetRows87de[0].x == 300);
    assert(wurm2.contactCallback.offset == 0);

    session.tick(simulation, world, quiky::InputState(), output);
    assert(wurm2.targetCursor30 == 2);
    assert(globals.sharedTargetRows87de[1].x == 0);
    assert(globals.sharedTargetRows87de[1].y == 100);
    assert(wurm2.contactCallback.offset == 0x4ab3);

    // A cursor equal to capacity wraps before selecting the row.
    wurm2.targetCursor30 = 2;
    globals.sharedTargetRows87de[0] = quiky::TargetCoordinateRow(100, 100);
    session.tick(simulation, world, quiky::InputState(), output);
    assert(wurm2.targetCursor30 == 1);
    assert(globals.sharedTargetRows87de[0].x == 0);
}

void testRecoveredBieneStateZeroMapPolarityContract() {
    // 01F7:68C0 -> 1C4D/1C6E publishes the state-zero MAP result in +0x2F.
    // The raw MAP bit is enough for this branch contract; no descriptor table
    // or startup-built BIENE table is involved.
    quiky::Map map = makeMap(16, 8);
    map.cells[1 * map.width + 1] = 0x4000;
    const quiky::WorldCollisionView world(map);
    quiky::LevelSessionConfig config;
    config.hasSpawn = true;
    config.spawnX = 16;
    config.spawnY = 16;
    config.streamRadiusRegions = 1;
    config.enableEdgeExit = false;

    quiky::LevelSession session("W1L1.MAP", map, makeSingleArea(0x03),
                                config);
    quiky::Simulation simulation;
    quiky::TraceClosedPlayerUpdate updater;
    simulation.setExperimentalPlayerUpdater(&updater);
    quiky::SimulationOutput output;
    session.reset(simulation);
    session.updateStreaming(simulation, 16, 16);

    quiky::LevelEntity &biene = session.entitiesForSetup()[0];
    biene.x = 48;
    biene.y = 48;
    biene.positionX = quiky::Fixed16::fromPixels(48);
    biene.positionY = quiky::Fixed16::fromPixels(48);
    biene.enemyState = 0;
    biene.enemyOrientation = -1;
    biene.enemySourceOrKind2c = -1;
    biene.enemyPatrolDirection = -1;
    biene.enemyTimer = 0x14;
    biene.enemyPhaseTimer = 0;
    biene.velocityX.raw = 0x1000;

    // 68F9 JLE is not taken for a positive latch: 6909 subtracts
    // orientation<<12 from the velocity and advances x.
    session.tick(simulation, world, quiky::InputState(), output);
    assert(biene.mapBlocked == 1);
    assert(biene.velocityX.raw == 0x2000);
    assert(biene.enemyPhaseTimer == 0);

    map.cells[1 * map.width + 1] = 0;
    biene.positionX = quiky::Fixed16::fromPixels(48);
    biene.positionY = quiky::Fixed16::fromPixels(48);
    biene.x = 48;
    biene.y = 48;
    biene.enemyTimer = 0x14;
    biene.enemyPhaseTimer = 0;
    biene.velocityX.raw = 0x1000;

    // 68F9 JLE is taken for a nonpositive latch: 69DD integrates the
    // existing velocity and increments +0x2A without patrol acceleration.
    session.tick(simulation, world, quiky::InputState(), output);
    assert(biene.mapBlocked == 0);
    assert(biene.velocityX.raw == 0x1000);
    assert(biene.enemyPhaseTimer == 1);
}

void testRecoveredBieneRuntimePhaseContract() {
    quiky::Map descriptorMap = makeMap(1, 1, 1);
    quiky::PlayerDescriptorTable descriptors;
    descriptors.setWord(1, 0x0008);
    const quiky::WorldCollisionView descriptorWorld(descriptorMap,
                                                    &descriptors);
    assert(descriptorWorld.transitionDescriptorProbeConfirmed(0, 0));
    assert(!descriptorWorld.transitionDescriptorProbeConfirmed(8, 0));

    const quiky::Map map = makeMap(16, 8);
    const quiky::WorldCollisionView world(map);
    quiky::LevelSessionConfig config;
    config.hasSpawn = true;
    config.spawnX = 16;
    config.spawnY = 16;
    config.streamRadiusRegions = 1;
    config.enableEdgeExit = false;
    config.hasBieneRuntimeTable = true;
    config.bieneRuntimeTable.fill(0);

    quiky::LevelSession session("W1L1.MAP", map, makeSingleArea(0x03),
                                config);
    quiky::Simulation simulation;
    quiky::TraceClosedPlayerUpdate updater;
    simulation.setExperimentalPlayerUpdater(&updater);
    quiky::SimulationOutput output;
    session.reset(simulation);
    session.updateStreaming(simulation, 16, 16);

    quiky::LevelEntity &biene = session.entitiesForSetup()[0];
    const std::int32_t oldX = biene.positionX.raw;
    const std::int32_t oldY = biene.positionY.raw;
    biene.enemyState = 1;
    biene.enemyAux3e = 0;
    biene.enemyVerticalOffset40 = 0;
    biene.enemyPhase34 = 0;
    // Keep the final state-7 exit arc above its origin so this fixture can
    // observe the state-1 -> state-3 -> state-7 fall-through in one callback.
    biene.enemyOriginY36 = quiky::Fixed16::fromPixels(-400).raw;

    session.tick(simulation, world, quiky::InputState(), output);

    // 01F7:6A69 masks the phase to 10 bits, consumes the injected signed
    // byte at phase+0x20, and adjusts the Y high word before subtracting the
    // fixed 0x1388 transition step. The X fixed-point correction is a raw
    // sign*0x2000 subtraction for the type-03 orientation.
    assert(biene.enemyAux3e == 0x20);
    // 01F7:6ACB and 01F7:6C12 each increment +0x34 after state 1 falls
    // through the same callback, so the post-callback word is two.
    assert(biene.enemyPhase34 == 2);
    assert(biene.enemyVerticalOffset40 == 0);
    const std::int32_t afterState3Y = quiky::Fixed16::wrapAddRaw(
        quiky::Fixed16::wrapSubRaw(oldY, 0x1388), 0x5014);
    assert(biene.positionY.raw == quiky::Fixed16::wrapAddRaw(
        afterState3Y, static_cast<std::int32_t>(0xffffee6cU)));
    assert(biene.positionX.raw == quiky::Fixed16::wrapAddRaw(
        quiky::Fixed16::wrapSubRaw(oldX, -0x2000), -0x15000 - 0x20000));
    assert(biene.enemyState == 7);
}

void testRecoveredNormalEnemyDamageContract() {
    const quiky::Map map = makeMap(16, 8);
    const quiky::WorldCollisionView world(map);
    quiky::LevelSessionConfig config;
    config.hasSpawn = true;
    config.spawnX = 16;
    config.spawnY = 48;
    config.streamRadiusRegions = 0;
    config.enableEdgeExit = false;

    struct Result {
        quiky::PlayerRecord player;
        quiky::LevelGameplayState gameplay;
        quiky::LevelEvent event;
        quiky::LevelEvent followup;
        bool enemyContactPending;
    };

    const auto run = [&](std::uint16_t type, std::uint16_t health,
                         std::uint16_t invulnerability,
                         std::uint16_t timer,
                         std::uint16_t transitionGate,
                         std::uint16_t effectBits,
                         bool installPlayerUpdater = false) {
        quiky::LevelSession session("W1L1.MAP", map, makeSingleArea(type),
                                    config);
        quiky::Simulation simulation;
        quiky::TraceClosedPlayerUpdate updater;
        if (installPlayerUpdater) {
            simulation.setExperimentalPlayerUpdater(&updater);
        }
        quiky::SimulationOutput output;
        quiky::PlayerDescriptorTable descriptors;
        const quiky::WorldCollisionView callbackWorld(
            map, installPlayerUpdater ? &descriptors : 0);
        session.reset(simulation);
        session.updateStreaming(simulation, 16, 48);

        // 01F7:393C publishes the exact four signed player bounds consumed
        // by 01F7:1B77. WURM2 uses object Y-10; BIENE uses object Y-20.
        quiky::PlayerRecord &player = simulation.stateForSetup().player;
        player.positionX = quiky::Fixed16::fromPixels(16);
        player.positionY = quiky::Fixed16::fromPixels(
            type == 0x03 ? 58 : 48);
        player.state2C = -10;
        player.verticalStepOrDirection2E = -40;
        player.state30 = 10;
        player.callbackState32 = 0;
        player.timer34 = timer;
        player.syncToRaw();

        session.gameplayStateForSetup().currentHealth8822 = health;
        session.gameplayStateForSetup().invulnerabilityGate8810 =
            invulnerability;
        session.gameplayStateForSetup().transitionGate89ea = transitionGate;
        session.gameplayStateForSetup().transitionEffectBits8950 = effectBits;

        session.tick(simulation, callbackWorld, quiky::InputState(), output);
        Result result;
        result.player = simulation.stateForSetup().player;
        result.gameplay = session.gameplayState();
        result.event = session.consumeEvent();
        result.followup = session.consumeEvent();
        result.enemyContactPending = session.entities()[0].enemyContactPending;
        return result;
    };

    const Result ordinary = run(0x01, 2, 0, 0, 0, 0);
    assert(ordinary.event.type == quiky::LevelEventType::PlayerDamaged);
    assert(ordinary.event.entityType == 0x01);
    assert(ordinary.event.stateWrites.size() == 2);
    assert(ordinary.event.stateWrites[0].address == 0x612e);
    assert(ordinary.event.stateWrites[0].before == 0);
    assert(ordinary.event.stateWrites[0].after == 1);
    assert(ordinary.event.stateWrites[1].address == 0x8822);
    assert(ordinary.event.stateWrites[1].before == 2);
    assert(ordinary.event.stateWrites[1].after == 1);
    assert(ordinary.gameplay.currentHealth8822 == 1);
    assert(ordinary.gameplay.lives880a == 4);
    assert(ordinary.gameplay.pendingEvent612e == 1);
    assert(ordinary.player.timer34 == 0xd2);
    assert(ordinary.player.velocityX.raw == 0x00018000);
    assert(!ordinary.enemyContactPending);

    const Result biene = run(0x03, 2, 0, 0, 0, 0);
    assert(biene.event.type == quiky::LevelEventType::PlayerDamaged);
    assert(biene.gameplay.currentHealth8822 == 1);
    assert(biene.player.timer34 == 0xd2);
    assert(!biene.enemyContactPending);

    const Result terminal = run(0x01, 1, 0, 0, 0, 0xffff, true);
    assert(terminal.event.type == quiky::LevelEventType::PlayerDamaged);
    assert(terminal.event.stateWrites.size() == 5);
    assert(terminal.event.stateWrites[2].address == 0x880a);
    assert(terminal.event.stateWrites[3].address == 0x8950);
    assert(terminal.event.stateWrites[3].after == 0xffcf);
    assert(terminal.event.stateWrites[4].address == 0x89ea);
    assert(terminal.event.stateWrites[4].after == 0xffff);
    assert(terminal.followup.type == quiky::LevelEventType::PlayerDied);
    assert(terminal.gameplay.currentHealth8822 == 0);
    assert(terminal.gameplay.lives880a == 3);
    assert(terminal.player.mode37 == -1);
    assert(terminal.player.sideResponse3B == 0);
    assert(terminal.player.verticalResponse3A == 0);
    assert(terminal.player.contactScratch2B == 0);
    assert(terminal.player.resetDeathTimer3E == 0x03e8);
    assert(terminal.player.velocityX.raw == 0x00018000);
    // 01F7:4416-44FE now runs after the 19E6 gate write.  The callback adds
    // the live EAX value left by 1BD1, so the clear-descriptor control moves
    // the integer Y word from 48 to 46 and leaves the post-4450 velocity at
    // -0x1e800.
    assert(terminal.player.xPixel() == 15);
    assert(terminal.player.yPixel() == 46);
    assert(terminal.player.velocityY.raw == static_cast<std::int32_t>(0xfffe1800U));
    assert(terminal.player.acceleration4C.raw == 0x00002000);
    assert(terminal.player.positiveYAcceleration50.raw == 0x00002000);
    assert(terminal.player.horizontalSpeedCap5C.raw == 0x00018000);
    assert(terminal.player.positiveYSpeedCap60.raw == 0x00040000);

    // The native death table remains active as the signed-negative gate is
    // decremented during the hold. Mode -1 alone must not classify ascent as
    // death, and a nonzero health word must not classify a transition as it.
    quiky::LevelSession lifecycleProbe("W1L1.MAP", map,
                                      makeSingleArea(0x01), config);
    quiky::PlayerRecord deathPlayer = terminal.player;
    lifecycleProbe.gameplayStateForSetup().transitionGate89ea = 0xfffe;
    lifecycleProbe.gameplayStateForSetup().currentHealth8822 = 0;
    assert(lifecycleProbe.playerDeathAnimationActive(deathPlayer));
    lifecycleProbe.gameplayStateForSetup().transitionGate89ea = 0;
    assert(!lifecycleProbe.playerDeathAnimationActive(deathPlayer));
    lifecycleProbe.gameplayStateForSetup().transitionGate89ea = 0xfffe;
    lifecycleProbe.gameplayStateForSetup().currentHealth8822 = 1;
    assert(!lifecycleProbe.playerDeathAnimationActive(deathPlayer));

    assert(run(0x01, 2, 0xffff, 0, 0, 0).event.type ==
           quiky::LevelEventType::None);
    assert(run(0x01, 2, 0, 0xd2, 0, 0).event.type ==
           quiky::LevelEventType::None);
    assert(run(0x01, 2, 0, 0, 0xffff, 0).event.type ==
           quiky::LevelEventType::None);
}

void testRecoveredAnimatedTileEffectStateMachine() {
    quiky::Map map = makeMap(16, 8);
    // W1 DS:6986 evidence: source tiles 200..204 select ICO effects
    // 120..124. State 4 has the special [16,0,32,48,64] order; states
    // 6/8/10 use [16,32,48,64,80].
    const std::uint16_t state4Tiles[] = {200, 201, 202, 203, 204};
    const std::uint16_t laterTiles[] = {200, 201, 202, 203, 204};
    const int rows[] = {1, 2, 3, 4};
    for (int rowIndex = 0; rowIndex < 4; ++rowIndex) {
        const std::uint16_t *tiles = rowIndex == 0 ? state4Tiles : laterTiles;
        for (int column = 0; column < 5; ++column) {
            map.cells[static_cast<std::size_t>(rows[rowIndex]) * map.width +
                      (rowIndex == 0 ? 1 : 2) + column] = tiles[column];
        }
    }

    const quiky::WorldCollisionView world(map);
    quiky::LevelSessionConfig config;
    config.hasSpawn = true;
    config.spawnX = 16;
    config.spawnY = 16;
    config.streamRadiusRegions = 0;
    config.enableEdgeExit = false;

    quiky::LevelSession session("W1L1.MAP", map, makeSingleArea(0x1f), config);
    quiky::Simulation simulation;
    quiky::TraceClosedPlayerUpdate updater;
    simulation.setExperimentalPlayerUpdater(&updater);
    quiky::SimulationOutput output;
    session.reset(simulation);
    session.updateStreaming(simulation, 16, 16);

    assert(session.entities()[0].kind == quiky::EntityKind::EnvironmentalEffect);
    assert(session.entities()[0].environmentSelector == 1);
    assert(session.entities()[0].environmentState == 0);
    assert(session.entities()[0].effectResource == "W1.ICO");

    session.tick(simulation, world, quiky::InputState(), output);
    assert(output.schedulerCallbacks.size() == 1);
    assert(output.schedulerCallbacks[0].callback.offset == 0x8e4b);
    assert(session.entities()[0].environmentState == 1);
    assert(session.effects().empty());

    for (int frame = 0; frame < 3; ++frame) {
        session.tick(simulation, world, quiky::InputState(), output);
    }
    assert(session.entities()[0].environmentState == 4);
    assert(session.effects().size() == 5);
    assert(session.effects()[0].effectSlot == 121);
    assert(session.effects()[1].effectSlot == 120);
    assert(session.effects()[2].effectSlot == 122);
    assert(session.effects()[4].x == 80);
    assert(session.effects()[0].effectResource == "W1.ICO");
    assert(session.consumeEvent().type == quiky::LevelEventType::TileInteraction);

    session.tick(simulation, world, quiky::InputState(), output);
    session.tick(simulation, world, quiky::InputState(), output);
    assert(session.entities()[0].environmentState == 6);
    assert(session.effects().size() == 10);
    assert(session.consumeEvent().type == quiky::LevelEventType::TileInteraction);

    session.tick(simulation, world, quiky::InputState(), output);
    session.tick(simulation, world, quiky::InputState(), output);
    assert(session.entities()[0].environmentState == 8);
    assert(session.consumeEvent().type == quiky::LevelEventType::TileInteraction);

    session.tick(simulation, world, quiky::InputState(), output);
    session.tick(simulation, world, quiky::InputState(), output);
    assert(session.entities()[0].environmentState == 10);
    assert(!session.entities()[0].active);
    assert(session.entities()[0].streamSuppressed);
    assert(session.gameplayState().terminalX8828 == 41);
    assert(session.gameplayState().terminalY882a == 86);
    assert(session.gameplayState().spawnRows8828[0].x == 41);
    assert(session.gameplayState().spawnRows8828[0].y == 86);
    // The state-8 children are still in their third native tick when state
    // 10 creates its children; they are removed at the following 10B5 pass.
    assert(session.effects().size() == 10);
    assert(session.effects()[5].effectSlot == 120);
    assert(session.effects()[9].effectSlot == 124);
    assert(session.consumeEvent().type == quiky::LevelEventType::TileInteraction);

    session.tick(simulation, world, quiky::InputState(), output);
    assert(output.schedulerCallbacks.empty());
    assert(!session.entities()[0].active);
    assert(session.effects().size() == 5);
}

void testRecoveredW1L1AmbientAndDedicatedContracts() {
    const quiky::Map map = makeMap(16, 8);
    const quiky::WorldCollisionView world(map);
    quiky::LevelSessionConfig config;
    config.hasSpawn = true;
    config.spawnX = 16;
    config.spawnY = 16;
    config.streamRadiusRegions = 0;
    config.enableEdgeExit = false;

    quiky::Simulation simulation;
    quiky::TraceClosedPlayerUpdate updater;
    simulation.setExperimentalPlayerUpdater(&updater);
    quiky::SimulationOutput output;

    quiky::LevelSession cloud("W1L1.MAP", map, makeSingleArea(0x28), config);
    cloud.reset(simulation);
    cloud.updateStreaming(simulation, 16, 16);
    assert(cloud.entities()[0].kind == quiky::EntityKind::AmbientVisual);
    assert(cloud.entities()[0].updateCallback.offset == 0x9269);
    assert(cloud.entities()[0].spriteSlot == 0xffff);
    assert(cloud.entities()[0].spriteResource == "WOLKE.BOB");
    cloud.tick(simulation, world, quiky::InputState(), output);
    assert(cloud.gameplayState().cloudSignal89e6 == 0xffff);
    const quiky::LevelEvent ordinaryGoal = cloud.consumeEvent();
    assert(ordinaryGoal.type == quiky::LevelEventType::LevelExit);
    assert(ordinaryGoal.targetLevel == "W1L2.MAP");

    quiky::LevelSession bonusCloud("W1L1.MAP", map,
                                   makeSingleArea(0x28), config);
    bonusCloud.reset(simulation);
    bonusCloud.updateStreaming(simulation, 16, 16);
    bonusCloud.gameplayStateForSetup().puzzleMask60d8 = 0x007f;
    bonusCloud.gameplayStateForSetup().score881c = 750;
    bonusCloud.gameplayStateForSetup().ammo880c = 10;
    bonusCloud.tick(simulation, world, quiky::InputState(), output);
    const quiky::LevelEvent bonusGoal = bonusCloud.consumeEvent();
    assert(bonusGoal.type == quiky::LevelEventType::LevelExit);
    assert(bonusGoal.targetLevel == "W1L4.MAP");
    assert(bonusCloud.score() == 2950);
    assert(bonusCloud.gameplayState().score881c == 2950);
    assert(bonusCloud.gameplayState().ammo880c == 0);
    assert(bonusCloud.gameplayState().pendingEvent612e == 12);
    assert(bonusGoal.stateWrites.size() == 3);
    assert(bonusGoal.stateWrites[0].address == 0x881c);
    assert(bonusGoal.stateWrites[0].before == 750);
    assert(bonusGoal.stateWrites[0].after == 2950);
    assert(bonusGoal.stateWrites[1].address == 0x880c);
    assert(bonusGoal.stateWrites[1].before == 10);
    assert(bonusGoal.stateWrites[1].after == 0);
    assert(bonusGoal.stateWrites[2].address == 0x612e);
    assert(bonusGoal.stateWrites[2].after == 12);

    quiky::LevelSession leaves("W1L1.MAP", map, makeSingleArea(0x2a), config);
    leaves.reset(simulation);
    leaves.updateStreaming(simulation, 16, 16);
    assert(leaves.entities()[0].kind == quiky::EntityKind::AmbientVisual);
    assert(leaves.entities()[0].updateCallback.offset == 0x47e7);
    assert(leaves.entities()[0].spriteSlot == 703);
    assert(leaves.entities()[0].spriteResource == "BLATT.BOB");
    assert(leaves.entities()[0].ambientVelocityY.raw == 0x13000);
    assert(leaves.entities()[0].ambientOriginX == leaves.entities()[0].x);
    assert(leaves.entities()[0].ambientOriginY == leaves.entities()[0].y);
    assert(leaves.entities()[0].ambientTimer == 0x000c);
    assert(leaves.entities()[0].ambientAnimationDelay == 10);
    assert(leaves.entities()[0].ambientAnimationCursor == 0x3328);

    quiky::LevelSessionConfig seededConfig = config;
    seededConfig.hasLeafPrngState = true;
    seededConfig.leafPrngRing.fill(0);
    seededConfig.leafPrngRing[0] = 1; // 4727 selects DS:3312.
    seededConfig.leafPrngRing[1] = 2; // 4727 subtracts 0x100 from 0x13000.
    quiky::LevelSession seededLeaves("W1L1.MAP", map,
                                    makeSingleArea(0x2b), seededConfig);
    seededLeaves.reset(simulation);
    seededLeaves.updateStreaming(simulation, 16, 16);
    assert(seededLeaves.entities()[0].spriteSlot == 700);
    assert(seededLeaves.entities()[0].ambientVelocityY.raw == 0x12f00);
    assert(seededLeaves.entities()[0].ambientAnimationDelay == 8);
    assert(seededLeaves.entities()[0].ambientAnimationCursor == 0x3314);
    seededLeaves.tick(simulation, world, quiky::InputState(), output);
    assert(seededLeaves.entities()[0].y == 17);
    assert(seededLeaves.entities()[0].ambientVelocityY.raw == 0x12dd4);
    assert(seededLeaves.entities()[0].ambientAnimationDelay == 7);
    assert(seededLeaves.entities()[0].spriteSlot == 700);

    quiky::LevelSession dedicated("W1L1.MAP", map, makeSingleArea(0x65), config);
    dedicated.reset(simulation);
    dedicated.updateStreaming(simulation, 16, 16);
    assert(dedicated.effects().size() == 1);
    assert(dedicated.effects()[0].sourceType == 0x65);
    assert(dedicated.effects()[0].effectResource == "LOOP_W1.ICO");
    assert(dedicated.effects()[0].effectSlot == 1);
    assert(dedicated.effects()[0].spriteSlot == 0xffff);
    assert(dedicated.effects()[0].updateCallback.offset == 0x10b5);
    assert(dedicated.effects()[0].eventSubtype == 0x00);
    assert(dedicated.effects()[0].eventAnimationState == 1);
    dedicated.tick(simulation, world, quiky::InputState(), output);
    assert(dedicated.effects().size() == 1);
}

void testRecoveredMovingPlatformCarryContract() {
    quiky::Map map = makeMap(16, 8);
    // An explicit (empty) descriptor table selects the trace-closed callback
    // path; the raw MAP bits used by the platform helper remain independent
    // of this table.
    quiky::PlayerDescriptorTable descriptors;
    const quiky::WorldCollisionView world(map, &descriptors);
    quiky::LevelSessionConfig config;
    config.hasSpawn = true;
    config.spawnX = 24;
    config.spawnY = 16;
    config.streamRadiusRegions = 0;
    config.enableEdgeExit = false;

    quiky::Simulation simulation;
    quiky::TraceClosedPlayerUpdate updater;
    simulation.setExperimentalPlayerUpdater(&updater);
    quiky::SimulationOutput output;

    quiky::LevelSession session("W1L1.MAP", map, makeSingleArea(0x3f), config);
    session.reset(simulation);
    session.updateStreaming(simulation, 24, 16);
    assert(session.entities()[0].kind == quiky::EntityKind::MovingPlatform);
    assert(session.entities()[0].updateCallback.offset == 0x9dc7);
    assert(session.entities()[0].spriteSlot == 301);
    assert(session.entities()[0].spriteResource == "PLATFW1.BOB");
    assert(!session.entities()[0].platformHorizontal4a);
    assert(session.entities()[0].platformDirectionY4c == -1);
    assert(session.entities()[0].platformDirectionX4e == 1);
    assert(session.entities()[0].platformEdgeLatch50 == -1);
    assert(session.entities()[0].platformAxisMarker4b == 0xff);
    assert(session.entities()[0].platformWait52 == 0x14);
    assert(session.entities()[0].platformMotionGate59);

    session.tick(simulation, world, quiky::InputState(), output);
    assert(output.schedulerCallbacks.size() == 1);
    assert(output.schedulerCallbacks[0].callback.offset == 0x9dc7);
    assert(output.playerDependencyOrder.size() == 2);
    assert(output.playerDependencyOrder[0].phase ==
           quiky::SimulationCallbackPhase::MovingPlatformBeforePlayer);
    assert(output.playerDependencyOrder[0].callback.offset == 0x9dc7);
    assert(output.playerDependencyOrder[0].sourceId ==
           session.entities()[0].id);
    assert(output.playerDependencyOrder[1].phase ==
           quiky::SimulationCallbackPhase::PlayerUpdate);
    assert(output.playerDependencyOrder[1].callback.offset == 0x3ff8);
    assert(session.gameplayState().platformLatch5006 == 0xffff);
    assert(session.gameplayState().platformCarryX8816 == 1);
    assert(session.gameplayState().platformCarryY8812 == 1);
    assert(session.entities()[0].platformCarryActive);
    // The player callback consumes DS:8816 and DS:8812 on this same frame;
    // the one-unit publication is retained in raw fixed-point state.
    assert(simulation.state().player.positionX.raw ==
           quiky::Fixed16::fromPixels(24).raw + 1);
    assert(simulation.state().player.positionY.raw ==
           quiky::Fixed16::fromPixels(16).raw + 2);

    quiky::LevelSessionConfig rejectedConfig = config;
    rejectedConfig.spawnX = 56;
    quiky::LevelSession rejected("W1L1.MAP", map,
                                 makeSingleArea(0x3f), rejectedConfig);
    rejected.reset(simulation);
    rejected.updateStreaming(simulation, 56, 16);
    rejected.tick(simulation, world, quiky::InputState(), output);
    assert(rejected.gameplayState().platformLatch5006 == 0);
    assert(!rejected.entities()[0].platformCarryActive);

    quiky::Map blockedMap = makeMap(16, 8);
    // The recovered 9C70 initializer keeps +0x59 set unless the initializer
    // cell has raw bit 0x0200. Use a horizontal 0x3D variant and force one
    // 16-pixel crossing so 9DC7 reaches the 0x0800 stop/snap probe at x=48.
    blockedMap.cells[17] = 0x0200;
    blockedMap.cells[19] = 0x0800;
    const quiky::WorldCollisionView blockedWorld(blockedMap, &descriptors);
    assert(blockedWorld.mapRawBit0800Confirmed(48, 16));
    quiky::LevelSession moving("W1L1.MAP", blockedMap,
                               makeSingleArea(0x3d), config);
    moving.reset(simulation);
    moving.updateStreaming(simulation, 24, 16);
    moving.entitiesForSetup()[0].velocityX = quiky::Fixed16(0x100000);
    moving.tick(simulation, blockedWorld, quiky::InputState(), output);
    assert(moving.entities()[0].x == 32);
    assert(moving.entities()[0].velocityX.raw == 0);
    assert(moving.entities()[0].platformWait54 == 0x46);
    assert(moving.entities()[0].platformHorizontal4a);
    assert(!moving.entities()[0].platformMotionGate59);

    quiky::Map horizontalMap = makeMap(16, 8);
    // The platform initializer's 5DA1 test clears +0x59 when the
    // initializer cell contains raw bit 0x0200.  With that gate closed, a
    // seeded +0x0A velocity reaches the horizontal integration path on the
    // first callback.  Spawn one pixel inside the strict A075 X interval so
    // the carry is published before the player callback runs.
    horizontalMap.cells[17] = 0x0200;
    const quiky::WorldCollisionView horizontalWorld(horizontalMap,
                                                     &descriptors);
    quiky::LevelSession horizontal("W1L1.MAP", horizontalMap,
                                   makeSingleArea(0x3d), config);
    quiky::Simulation horizontalSimulation;
    quiky::TraceClosedPlayerUpdate horizontalUpdater;
    horizontalSimulation.setExperimentalPlayerUpdater(&horizontalUpdater);
    horizontal.reset(horizontalSimulation);
    horizontal.updateStreaming(horizontalSimulation, 25, 16);
    horizontal.entitiesForSetup()[0].velocityX = quiky::Fixed16(0x28000);
    const std::int32_t horizontalPlayerXBefore =
        horizontalSimulation.state().player.positionX.raw;
    horizontal.tick(horizontalSimulation, horizontalWorld,
                    quiky::InputState(), output);
    assert(horizontal.entities()[0].x == 18);
    assert(horizontal.entities()[0].platformPreviousX == 16);
    assert(horizontal.gameplayState().platformLatch5006 == 0xffff);
    assert(horizontal.gameplayState().platformCarryX8816 ==
           quiky::Fixed16::fromRaw(0x00020001).raw);
    assert(horizontalSimulation.state().player.positionX.raw ==
           quiky::Fixed16::wrapAddRaw(horizontalPlayerXBefore,
                                      0x00020001));
    assert(horizontalSimulation.state().player.positionY.raw ==
           quiky::Fixed16::fromPixels(16).raw + 2);

    quiky::LevelSession horizontalNegative("W1L1.MAP", horizontalMap,
                                           makeSingleArea(0x3d), config);
    quiky::Simulation horizontalNegativeSimulation;
    quiky::TraceClosedPlayerUpdate horizontalNegativeUpdater;
    horizontalNegativeSimulation.setExperimentalPlayerUpdater(
        &horizontalNegativeUpdater);
    horizontalNegative.reset(horizontalNegativeSimulation);
    horizontalNegative.updateStreaming(horizontalNegativeSimulation, 25, 16);
    horizontalNegative.entitiesForSetup()[0].velocityX =
        quiky::Fixed16(static_cast<std::int32_t>(0xfffd8000U));
    const std::int32_t horizontalNegativePlayerXBefore =
        horizontalNegativeSimulation.state().player.positionX.raw;
    horizontalNegative.tick(horizontalNegativeSimulation, horizontalWorld,
                             quiky::InputState(), output);
    assert(horizontalNegative.entities()[0].x == 13);
    assert(horizontalNegative.entities()[0].platformPreviousX == 16);
    assert(horizontalNegative.gameplayState().platformLatch5006 == 0xffff);
    assert(horizontalNegative.gameplayState().platformCarryX8816 ==
           static_cast<std::int32_t>(0xfffd0001U));
    assert(horizontalNegativeSimulation.state().player.positionX.raw ==
           quiky::Fixed16::wrapAddRaw(horizontalNegativePlayerXBefore,
                                      static_cast<std::int32_t>(0xfffd0001U)));

    quiky::LevelSession culled("W1L1.MAP", map,
                               makeSingleArea(0x3f), config);
    culled.reset(simulation);
    culled.updateStreaming(simulation, 24, 16);
    culled.tick(simulation, world, quiky::InputState(), output);
    assert(culled.entities()[0].platformCarryActive);
    culled.updateStreaming(simulation, 256, 16);
    assert(!culled.entities()[0].active);
    assert(!culled.entities()[0].platformCarryActive);
    assert(!culled.entities()[0].schedulerHandle.valid());

    culled.updateStreaming(simulation, 24, 16);
    assert(culled.entities()[0].active);
    assert(culled.entities()[0].x == culled.entities()[0].initialX);
    assert(culled.entities()[0].y == culled.entities()[0].initialY);
    assert(culled.entities()[0].velocityX.raw == 0);
    assert(culled.entities()[0].platformWait54 == 0);
}

} // namespace

int main() {
    try {
        testReader();
        testArchive();
        testFormatsAndRenderer();
        testGameplayFrameComposition();
        testAreaAndBob();
        testFaithfulRecordWorldAndAnimation();
        testLevelSessionUsesSimulationBoundary();
        testW1L1NativeDefaultSpawn();
        testW1L2NativeStartupState();
        testRecoveredCollectibleStateContracts();
        testRecoveredCollectibleStrictBounds();
        testRecoveredW1L1EnemyFamilies();
        testRecoveredBumpCallbackContract();
        testRecoveredWurm2DescriptorProbeContract();
        testRecoveredWurm2TargetTailContract();
        testRecoveredBieneStateZeroMapPolarityContract();
        testRecoveredBieneRuntimePhaseContract();
        testRecoveredNormalEnemyDamageContract();
        testRecoveredAnimatedTileEffectStateMachine();
        testRecoveredW1L1AmbientAndDedicatedContracts();
        testRecoveredMovingPlatformCarryContract();
    } catch (const std::exception &error) {
        std::cerr << "unexpected test failure: " << error.what() << "\n";
        return 1;
    }
    std::cout << "all faithful format tests passed\n";
    return 0;
}
