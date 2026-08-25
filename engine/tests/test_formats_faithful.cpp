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
    config.spawnY = 16;
    config.streamRadiusRegions = 0;
    config.enableEdgeExit = false;
    quiky::LevelSession session("W1L1.MAP", map, makeArea(0x6f, 0x79), config);
    quiky::Simulation simulation;
    quiky::TraceClosedPlayerUpdate updater;
    simulation.setExperimentalPlayerUpdater(&updater);
    quiky::SimulationOutput output;
    session.reset(simulation);
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

void testRecoveredCollectibleStateContracts() {
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

    quiky::LevelSession healthMax("W1L1.MAP", map, makeSingleArea(0x70), config);
    healthMax.reset(simulation);
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
    health.reset(simulation);
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
    invulnerability.reset(simulation);
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
    letter.updateStreaming(simulation, 16, 16);
    letter.tick(simulation, world, quiky::InputState(), output);
    event = letter.consumeEvent();
    assert(event.type == quiky::LevelEventType::Collected);
    assert(letter.gameplayState().puzzleMask60d8 == 1);
    assert(letter.score() == 100);
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
    assert(session.entities()[0].x == 14);
    assert(session.entities()[1].x == 78);
    assert(session.deaths() == 0);
    assert(session.entities()[0].enemyContactPending);
    assert(session.entities()[0].contactCallback.offset == 0x4ab3);
    assert(session.entities()[0].responseTimer == 0x28);
    assert(!session.entities()[1].enemyContactPending);
    assert(session.consumeEvent().type == quiky::LevelEventType::EntityCollisionImpact);

    for (int frame = 0; frame < 40; ++frame) {
        session.tick(simulation, world, quiky::InputState(), output);
    }
    assert(!session.entities()[0].active);
    assert(session.entities()[1].active);
    assert(session.entities()[0].streamSuppressed);
    assert(!session.entities()[1].streamSuppressed);

    session.updateStreaming(simulation, 400, 400);
    session.tick(simulation, world, quiky::InputState(), output);
    assert(output.schedulerCallbacks.empty());
    assert(!session.entities()[0].active);
    assert(!session.entities()[1].active);
    assert(session.entities()[1].streamSuppressed);

    session.updateStreaming(simulation, 16, 16);
    session.tick(simulation, world, quiky::InputState(), output);
    assert(output.schedulerCallbacks.empty());
    assert(!session.entities()[0].active);
    assert(!session.entities()[1].active);
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
        testRecoveredCollectibleStateContracts();
        testRecoveredW1L1EnemyFamilies();
    } catch (const std::exception &error) {
        std::cerr << "unexpected test failure: " << error.what() << "\n";
        return 1;
    }
    std::cout << "all faithful format tests passed\n";
    return 0;
}
