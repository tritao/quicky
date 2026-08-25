#include "quiky/area.h"
#include "quiky/archive.h"
#include "quiky/bob.h"
#include "quiky/binary_reader.h"
#include "quiky/map.h"
#include "quiky/palette.h"
#include "quiky/pcx.h"
#include "quiky/level.h"
#include "quiky/player_animation.h"
#include "quiky/renderer.h"
#include "quiky/runtime.h"
#include "quiky/tileset.h"

#include <algorithm>
#include <cassert>
#include <cstdint>
#include <exception>
#include <iostream>

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

void writeU16BEAt(quiky::Bytes &data, std::size_t offset, std::uint16_t value) {
    data[offset] = static_cast<quiky::byte>(value >> 8);
    data[offset + 1] = static_cast<quiky::byte>(value & 0xff);
}

quiky::Bytes makeArchive() {
    const std::string firstName = "FIRST.MAP";
    const std::string secondName = "SECOND.ICO";
    quiky::Bytes data;
    data.push_back('a');
    data.push_back('b');
    data.push_back('c');
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
    const quiky::Archive archive = quiky::Archive::fromBytes(makeArchive());
    assert(archive.entries().size() == 2);
    assert(archive.find("first.map").size == 3);
    assert(archive.read("FIRST.MAP") == quiky::Bytes({'a', 'b', 'c'}));
    bool failed = false;
    try {
        archive.find("missing.dat");
    } catch (const quiky::FormatError &) {
        failed = true;
    }
    assert(failed);
}

void testMapPaletteTilesetAndRenderer() {
    quiky::Bytes mapData = {'T', 'L', 'E', '1'};
    appendU16BE(mapData, 2);
    appendU16BE(mapData, 1);
    appendU16BE(mapData, 9);
    appendU16BE(mapData, 0x0000);
    appendU16BE(mapData, 0x3801);
    const quiky::Map map = quiky::Map::parse(mapData, "test.MAP");
    assert(map.width == 2 && map.height == 1);
    assert(quiky::Map::tileId(0x3812) == 0x012);
    assert(quiky::Map::properties(0x3812) == 0x1c);
    assert(quiky::Map::tileId(map.cell(1, 0)) == 0x001);
    assert(quiky::Map::properties(map.cell(1, 0)) == 0x1c);

    quiky::Bytes paletteData(128, 0);
    paletteData[0] = 0x0a;
    paletteData[3] = 0x08;
    paletteData.push_back(0x0c);
    for (int index = 0; index < 256; ++index) {
        paletteData.push_back(static_cast<quiky::byte>(index));
        paletteData.push_back(static_cast<quiky::byte>(index + 1));
        paletteData.push_back(static_cast<quiky::byte>(index + 2));
    }
    const quiky::Palette palette = quiky::Palette::parsePcx(paletteData, "test.PCC");
    assert(palette.colors[3].red == 3);
    assert(palette.colors[3].green == 4);
    assert(palette.colors[3].blue == 5);
    const quiky::Palette dacPalette =
        quiky::Palette::parsePcxDac(paletteData, "test.PCC");
    // Native palette staging stores source components as 6-bit DAC values
    // (component >> 2), then VGA expands them for presentation.
    assert(dacPalette.colors[3].red == 0);
    assert(dacPalette.colors[3].green == 4);
    assert(dacPalette.colors[3].blue == 4);

    quiky::Palette fadingPalette = palette;
    fadingPalette.blackoutDescending(5, 3);
    for (std::size_t index = 3; index <= 5; ++index) {
        assert(fadingPalette.colors[index].red == 0);
        assert(fadingPalette.colors[index].green == 0);
        assert(fadingPalette.colors[index].blue == 0);
    }
    assert(fadingPalette.colors[2].red == 2);
    fadingPalette.blackoutEntry(2);
    assert(fadingPalette.colors[2].red == 0);

    quiky::Bytes icoData(512, 0);
    for (std::size_t index = 0; index < 256; ++index) {
        icoData[index] = static_cast<quiky::byte>(index);
        icoData[256 + index] = 7;
    }
    const quiky::Tileset tileset = quiky::Tileset::parseIco(icoData, "test.ICO");
    assert(tileset.tiles.size() == 2);
    assert(tileset.tiles[0][0] == 0);
    assert(tileset.tiles[0][1] == 4);
    assert(tileset.tiles[0][4] == 1);
    assert(tileset.tiles[1][100] == 7);

    quiky::IndexedSurface surface = quiky::renderMap(map, tileset);
    assert(surface.width == 32 && surface.height == 16);
    assert(surface.at(0, 0) == 0);
    assert(surface.at(1, 0) == 4);
    assert(surface.at(16, 0) == 7);

    quiky::drawIcoTile(surface, tileset, 1, 0, 0);
    assert(surface.at(0, 0) == 7);
}

void testPcxAndIndexedLayerBlit() {
    quiky::Bytes data(128, 0);
    data[0] = 0x0a;
    data[1] = 0x05;
    data[2] = 0x01;
    data[3] = 0x08;
    data[4] = 0;
    data[5] = 0;
    data[6] = 0;
    data[7] = 0;
    data[8] = 2;
    data[9] = 0;
    data[10] = 1;
    data[11] = 0;
    data[65] = 1;
    data[66] = 4;
    data[67] = 0;
    data.insert(data.end(), {1, 2, 3, 0xc1, 0, 4, 5, 6, 0xc1, 0});
    data.push_back(0x0c);
    for (int index = 0; index < 256; ++index) {
        data.push_back(static_cast<quiky::byte>(index));
        data.push_back(static_cast<quiky::byte>(index + 1));
        data.push_back(static_cast<quiky::byte>(index + 2));
    }

    const quiky::PcxImage image = quiky::PcxImage::parse(data, "test.PCC");
    assert(image.width == 3 && image.height == 2 && image.bytesPerLine == 4);
    assert(image.pixels == quiky::Bytes({1, 2, 3, 4, 5, 6}));
    assert(image.palette.colors[5].red == 5);
    assert(image.surface().at(2, 1) == 6);

    quiky::IndexedSurface destination(4, 3);
    std::fill(destination.pixels.begin(), destination.pixels.end(), 9);
    quiky::blitIndexedSurface(destination, image.surface(), -1, -1);
    assert(destination.at(0, 0) == 5);
    assert(destination.at(1, 0) == 6);
    assert(destination.at(0, 1) == 9);

    quiky::IndexedSurface transparent(2, 2);
    transparent.pixels = {0, 7, 8, 9};
    quiky::blitIndexedSurface(destination, transparent, 2, 1,
                              quiky::SurfaceBlitMode::TransparentZero);
    assert(destination.at(2, 1) == 9);
    assert(destination.at(3, 1) == 7);
    assert(destination.at(2, 2) == 8);

    quiky::IndexedSurface screen(320, 200);
    std::fill(screen.pixels.begin(), screen.pixels.end(), 3);
    quiky::IndexedSurface gamebar(320, 24);
    std::fill(gamebar.pixels.begin(), gamebar.pixels.end(), 11);
    gamebar.at(0, 0) = 12;
    gamebar.at(319, 23) = 13;
    quiky::compositeGamebar(screen, gamebar);
    assert(screen.at(0, 175) == 3);
    assert(screen.at(0, 176) == 12);
    assert(screen.at(1, 176) == 11);
    assert(screen.at(319, 199) == 13);

    bool rejectedDimensions = false;
    try {
        quiky::compositeGamebar(destination, gamebar);
    } catch (const quiky::FormatError &) {
        rejectedDimensions = true;
    }
    assert(rejectedDimensions);
}

void testRendererClippingAndTransparency() {
    quiky::Tileset tileset;
    quiky::Tile tile;
    tile.fill(0);
    tile[0] = 9;
    tile[15] = 8;
    tile[15 * 16] = 7;
    tile[15 * 16 + 15] = 6;
    tileset.tiles.push_back(tile);

    quiky::IndexedSurface surface(8, 8);
    std::fill(surface.pixels.begin(), surface.pixels.end(), 3);
    quiky::drawIcoTile(surface, tileset, 0, -8, -8, true);
    assert(surface.at(0, 0) == 3);

    quiky::drawIcoTile(surface, tileset, 0, -15, -15, true);
    assert(surface.at(0, 0) == 6);
    assert(surface.at(1, 0) == 3);

    quiky::drawIcoTile(surface, tileset, 0, 7, 7, false);
    assert(surface.at(7, 7) == 9);

    // Native 01F7:11B4 writes each ICO plane unconditionally; indexed zero
    // therefore clears an earlier layer.  The explicit true mode remains a
    // utility for callers that need a masked composition operation.
    quiky::IndexedSurface nativeIco(8, 8);
    std::fill(nativeIco.pixels.begin(), nativeIco.pixels.end(), 3);
    quiky::drawIcoTile(nativeIco, tileset, 0, 0, 0);
    assert(nativeIco.at(0, 0) == 9);
    assert(nativeIco.at(1, 1) == 0);
}

void testAreaAndOverlay() {
    quiky::Bytes areaData(0x14e8, 0);
    writeU16BEAt(areaData, 0x0e, 2);
    writeU16BEAt(areaData, 0x10, 1);
    writeU16BEAt(areaData, 0x160, 0x1388);
    writeU16BEAt(areaData, 0x162, 0xffff);
    appendU16BE(areaData, 0x002b);
    appendU16BE(areaData, 0x0010);
    appendU16BE(areaData, 0x0020);
    appendU16BE(areaData, 0xffff);

    const quiky::Area area = quiky::Area::parse(areaData, "W1L1.ARE");
    assert(area.layoutWidth == 2 && area.layoutHeight == 1);
    assert(area.references.size() == 1);
    assert(area.references[0].value == 0x1388);
    assert(area.references[0].entities.size() == 1);
    assert(area.references[0].entities[0].recordOffset == 0x14e8);
    const std::vector<quiky::AreaPlacement> placements = area.placements();
    assert(placements.size() == 1);
    assert(placements[0].worldX == 16 && placements[0].worldY == 32);

    quiky::IndexedSurface surface(128, 128);
    quiky::Palette palette;
    quiky::overlayArea(surface, palette, area);
    const quiky::byte marker = static_cast<quiky::byte>(240 + (0x002b & 0x0f));
    assert(surface.at(16, 32) == marker);
    assert(surface.at(12, 28) == 0);
    assert(palette.colors[marker].red == static_cast<quiky::byte>(96 + ((0x002b * 73) % 160)));
}

void testBobParserDecoderAndSheet() {
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
    const quiky::BobRecord &record = bob.records[0];
    assert(record.slot == 7 && record.width == 2 && record.height == 1);
    const std::vector<std::int16_t> pixels = quiky::decodeBobRecord(record);
    assert(pixels.size() == 2 && pixels[0] == 1 && pixels[1] == 2);

    quiky::Palette palette;
    const quiky::IndexedSurface sheet = quiky::renderBobSheet(bob, palette, 1);
    assert(sheet.width == 6 && sheet.height == 5);
    assert(sheet.at(2, 2) == 1 && sheet.at(3, 2) == 2);

    quiky::IndexedSurface frame(16, 16);
    quiky::drawBobRecord(frame, record, 5, 5);
    assert(frame.at(4, 3) == 1 && frame.at(5, 3) == 2);

    quiky::IndexedSurface clipped(3, 3);
    std::fill(clipped.pixels.begin(), clipped.pixels.end(), 9);
    quiky::drawBobRecord(clipped, record, 2, 3);
    assert(clipped.at(1, 1) == 1);
    assert(clipped.at(2, 1) == 2);
    quiky::drawBobRecord(clipped, record, 0, 0);
    assert(clipped.at(0, 0) == 9);
}

quiky::Map makePhysicsMap() {
    quiky::Bytes data = {'T', 'L', 'E', '1'};
    appendU16BE(data, 8);
    appendU16BE(data, 6);
    appendU16BE(data, 9);
    for (std::uint16_t y = 0; y < 6; ++y) {
        for (std::uint16_t x = 0; x < 8; ++x) {
            const std::uint16_t value = y == 4 ? static_cast<std::uint16_t>(0x20 << 9) : 0;
            appendU16BE(data, value);
        }
    }
    return quiky::Map::parse(data, "physics.MAP");
}

class TraceCollisionQuery : public quiky::CollisionQuery {
public:
    bool blocksHorizontal(std::int32_t tileX, std::int32_t) const override {
        return tileX == 6;
    }

    bool blocksFloor(std::int32_t, std::int32_t tileY) const override {
        return tileY == 4;
    }

    bool blocksCeiling(std::int32_t, std::int32_t tileY) const override {
        return tileY == 1;
    }
};

quiky::Map makeLevelMap() {
    quiky::Bytes data = {'T', 'L', 'E', '1'};
    appendU16BE(data, 16);
    appendU16BE(data, 8);
    appendU16BE(data, 9);
    for (int index = 0; index < 16 * 8; ++index) {
        appendU16BE(data, 0);
    }
    return quiky::Map::parse(data, "level.MAP");
}

quiky::Area makeLevelArea(std::uint16_t firstType, std::uint16_t secondType) {
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

quiky::Area makeGoalMaskArea() {
    quiky::Bytes data(0x14e8, 0);
    writeU16BEAt(data, 0x0e, 1);
    writeU16BEAt(data, 0x10, 1);
    writeU16BEAt(data, 0x160, 0x1388);
    for (std::uint16_t type = 0x79; type <= 0x7f; ++type) {
        appendU16BE(data, type);
        appendU16BE(data, 16);
        appendU16BE(data, 16);
    }
    appendU16BE(data, 0xffff);
    return quiky::Area::parse(data, "W1L1.ARE");
}

void testLevelSession() {
    const quiky::Map map = makeLevelMap();
    quiky::Area area = makeLevelArea(0x6f, 0x79);
    quiky::LevelSessionConfig config;
    config.hasSpawn = true;
    config.spawnX = 16;
    config.spawnY = 16;
    config.streamRadiusRegions = 0;
    config.enableEdgeExit = false;
    quiky::PlayerConfig playerConfig;
    playerConfig.gravity = 0;
    playerConfig.acceleration = 0;
    playerConfig.friction = 0;
    quiky::PlayerSimulation simulation(playerConfig);
    quiky::PlayerState player;
    quiky::LevelSession session("W1L1.MAP", map, area, config);
    session.reset(player, simulation);
    session.updateStreaming(player.x.floorPixels(), player.y.floorPixels());
    assert(session.entities().size() == 2);
    assert(session.entities()[0].active);
    assert(session.entities()[0].phase == quiky::EntityPhase::Active);
    assert(session.entities()[0].spriteSlot == 607);
    assert(session.entities()[0].spriteResource == "WERBE.BOB");
    assert(!session.entities()[1].active);
    assert(session.entities()[1].phase == quiky::EntityPhase::Dormant);
    assert(session.entities()[1].spriteSlot == 600);
    assert(session.entities()[1].spriteResource == "PUZZLE.BOB");

    quiky::Area platformArea = makeLevelArea(0x3f, 0x34);
    quiky::LevelSession platformSession("W1L3.MAP", map, platformArea, config);
    assert(platformSession.entities()[0].kind == quiky::EntityKind::MovingPlatform);
    assert(platformSession.entities()[0].spriteSlot == 301);
    assert(platformSession.entities()[0].spriteResource == "PLATFW1.BOB");
    assert(platformSession.entities()[0].collisionWidth == 32);
    assert(platformSession.entities()[0].collisionHeight == 14);
    assert(platformSession.entities()[1].spriteSlot == 400);
    assert(platformSession.entities()[1].spriteResource == "BUMP_W1.BOB");

    const quiky::Area effectArea = makeLevelArea(0x28, 0x29);
    quiky::LevelSession effectSession("W1L1.MAP", map, effectArea, config);
    assert(effectSession.entities()[0].spriteSlot == 413);
    assert(effectSession.entities()[0].spriteResource == "WOLKE.BOB");
    assert(effectSession.entities()[1].spriteSlot == 700);
    assert(effectSession.entities()[1].spriteResource == "BLATT.BOB");

    const quiky::Area worldEffectArea = makeLevelArea(0x1f, 0x20);
    quiky::LevelSession worldEffectSession("W1L1.MAP", map,
                                           worldEffectArea, config);
    assert(worldEffectSession.entities()[0].effectResource == "WORLD");
    assert(worldEffectSession.entities()[0].effectSlot == 0xffff);

    quiky::Map worldEmissionMap = makeLevelMap();
    worldEmissionMap.cells[16 + 2] = 201;
    worldEmissionMap.cells[16 + 1] = 200;
    worldEmissionMap.cells[16 + 3] = 202;
    worldEmissionMap.cells[16 + 4] = 203;
    worldEmissionMap.cells[16 + 5] = 204;
    quiky::LevelSession worldEmissionSession("W1L1.MAP", worldEmissionMap,
                                             worldEffectArea, config);
    worldEmissionSession.reset(player, simulation);
    worldEmissionSession.updateStreaming(player.x.floorPixels(),
                                         player.y.floorPixels());
    for (int frame = 0; frame < 3; ++frame) {
        worldEmissionSession.tick(player, simulation, quiky::InputState());
    }
    assert(worldEmissionSession.effects().empty());
    for (int frame = 0; frame < 3; ++frame) {
        worldEmissionSession.tick(player, simulation, quiky::InputState());
    }
    const quiky::SpawnPoint publishedCheckpoint = worldEmissionSession.checkpoint();
    assert(publishedCheckpoint.x == 41 && publishedCheckpoint.y == 86);
    worldEmissionSession.tick(player, simulation, quiky::InputState());
    assert(worldEmissionSession.effects().size() == 5);
    assert(worldEmissionSession.effects()[0].effectSlot == 121);
    assert(worldEmissionSession.effects()[0].x == 32);
    assert(worldEmissionSession.effects()[1].effectSlot == 120);
    assert(worldEmissionSession.effects()[1].x == 16);
    assert(worldEmissionSession.effects()[4].effectSlot == 124);
    assert(worldEmissionSession.effects()[4].x == 80);
    for (int frame = 0; frame < 3; ++frame) {
        worldEmissionSession.tick(player, simulation, quiky::InputState());
    }
    assert(worldEmissionSession.effects().empty());

    const quiky::Area transientArea = makeLevelArea(0x65, 0x67);
    quiky::LevelSessionConfig transientConfig = config;
    transientConfig.streamRadiusRegions = 1;
    quiky::LevelSession transientSession("W1L1.MAP", map,
                                         transientArea, transientConfig);
    transientSession.reset(player, simulation);
    transientSession.updateStreaming(player.x.floorPixels(), player.y.floorPixels());
    assert(transientSession.effects().size() == 2);
    assert(transientSession.effects()[0].sourceType == 0x65);
    assert(transientSession.effects()[0].effectResource == "LOOP_W1.ICO");
    assert(transientSession.effects()[0].effectSlot == 1);
    assert(transientSession.effects()[1].sourceType == 0x67);
    assert(transientSession.effects()[1].effectSlot == 6);

    const quiky::Area dedicatedW2Area = makeLevelArea(0x65, 0x66);
    const quiky::LevelSession dedicatedW2Session("W2L1.MAP", map,
                                                dedicatedW2Area, config);
    assert(dedicatedW2Session.entities()[0].effectSlot == 1);
    assert(dedicatedW2Session.entities()[1].effectSlot == 8);
    const quiky::Area dedicatedW4Area = makeLevelArea(0x67, 0x65);
    const quiky::LevelSession dedicatedW4Session("W4L1.MAP", map,
                                                dedicatedW4Area, config);
    assert(dedicatedW4Session.entities()[0].effectSlot == 22);
    assert(dedicatedW4Session.entities()[1].effectSlot == 1);
    const quiky::Area dedicatedW5Area = makeLevelArea(0x65, 0x67);
    const quiky::LevelSession dedicatedW5Session("W5L1.MAP", map,
                                                dedicatedW5Area, config);
    assert(dedicatedW5Session.entities()[0].effectSlot == 4);
    assert(dedicatedW5Session.entities()[1].effectSlot == 6);

    transientSession.tick(player, simulation, quiky::InputState());
    assert(transientSession.effects()[0].animationFrame == 1);
    transientSession.tick(player, simulation, quiky::InputState());
    assert(transientSession.effects()[0].animationFrame == 2);
    transientSession.tick(player, simulation, quiky::InputState());
    assert(transientSession.effects().empty());

    session.tick(player, simulation, quiky::InputState());
    quiky::LevelEvent event = session.consumeEvent();
    assert(event.type == quiky::LevelEventType::Collected);
    assert(session.score() == 10);
    assert(session.entities()[0].collected);
    assert(session.entities()[0].phase == quiky::EntityPhase::Collected);
    assert(session.entities()[0].activeFrames == 1);
    assert(session.entities()[0].animationFrame == 1);

    session.reset(player, simulation);
    assert(session.score() == 0 && session.deaths() == 0);
    assert(!session.entities()[0].collected);
    assert(session.entities()[0].phase == quiky::EntityPhase::Dormant);
    session.updateStreaming(player.x.floorPixels(), player.y.floorPixels());
    assert(session.entities()[0].phase == quiky::EntityPhase::Active);
    assert(session.entities()[0].animationFrame == 0);

    quiky::LevelSessionConfig hazardConfig = config;
    hazardConfig.streamRadiusRegions = 2;
    quiky::Area hazardArea = makeLevelArea(0x01, 0x28);
    quiky::LevelSession hazardSession("W1L1.MAP", map, hazardArea, hazardConfig);
    hazardSession.reset(player, simulation);
    hazardSession.tick(player, simulation, quiky::InputState());
    event = hazardSession.consumeEvent();
    assert(event.type == quiky::LevelEventType::PlayerDied);
    assert(hazardSession.deaths() == 1);
    assert(player.x.floorPixels() == 16 && player.y.floorPixels() == 16);
    assert(hazardSession.playerDying());
    for (std::uint32_t frame = 0;
         frame + 1 < hazardConfig.deathRecoveryFrames; ++frame) {
        hazardSession.tick(player, simulation, quiky::InputState());
        event = hazardSession.consumeEvent();
        assert(event.type == quiky::LevelEventType::None);
        assert(hazardSession.playerDying());
    }
    hazardSession.tick(player, simulation, quiky::InputState());
    event = hazardSession.consumeEvent();
    assert(event.type == quiky::LevelEventType::PlayerRecovered);
    assert(!hazardSession.playerDying());
    assert(player.x.floorPixels() == 16 && player.y.floorPixels() == 16);

    quiky::LevelSessionConfig exitConfig = config;
    exitConfig.spawnX = 232;
    exitConfig.enableEdgeExit = true;
    quiky::Area exitArea = makeLevelArea(0x28, 0x28);
    quiky::LevelSession exitSession("W1L1.MAP", map, exitArea, exitConfig);
    exitSession.reset(player, simulation);
    exitSession.tick(player, simulation, quiky::InputState());
    event = exitSession.consumeEvent();
    assert(event.type == quiky::LevelEventType::LevelExit);
    assert(event.targetLevel == "W1L2.MAP");

    quiky::LevelSessionConfig bonusConfig = exitConfig;
    bonusConfig.spawnX = 16;
    bonusConfig.spawnY = 16;
    bonusConfig.streamRadiusRegions = 0;
    quiky::LevelSession bonusSession("W1L1.MAP", map,
                                     makeGoalMaskArea(), bonusConfig);
    bonusSession.reset(player, simulation);
    bonusSession.updateStreaming(player.x.floorPixels(), player.y.floorPixels());
    bonusSession.tick(player, simulation, quiky::InputState());
    assert(bonusSession.goalMask() == 0x007f);
    player.x = quiky::Fixed16::fromPixels(232);
    player.velocityX = quiky::Fixed16();
    player.grounded = true;
    bonusSession.tick(player, simulation, quiky::InputState());
    event = bonusSession.consumeEvent();
    assert(event.type == quiky::LevelEventType::LevelExit);
    assert(event.targetLevel == "W1L4.MAP");
}

void testLevelSessionCollisionQuery() {
    const quiky::Map map = makeLevelMap();
    const quiky::Area area = makeLevelArea(0x28, 0x28);
    quiky::LevelSessionConfig config;
    config.hasSpawn = true;
    config.spawnX = 16;
    config.spawnY = 16;
    config.streamRadiusRegions = 0;
    config.enableEdgeExit = false;

    quiky::PlayerConfig playerConfig;
    playerConfig.width = 12;
    playerConfig.height = 12;
    playerConfig.acceleration = 0;
    playerConfig.friction = 0;
    playerConfig.gravity = quiky::Fixed16::kOne;
    quiky::PlayerSimulation simulation(playerConfig);
    TraceCollisionQuery collision;
    quiky::PlayerState player;
    quiky::LevelSession session("W1L1.MAP", map, area, config);
    session.reset(player, simulation);

    for (int frame = 0; frame < 60; ++frame) {
        session.tick(player, simulation, collision, quiky::InputState());
        session.consumeEvent();
    }
    assert(player.grounded);
    assert(player.y.floorPixels() == 52);

    quiky::LevelSessionConfig platformConfig = config;
    platformConfig.spawnY = 0;
    const quiky::Area platformArea = makeLevelArea(0x3f, 0x28);
    quiky::LevelSession platformSession("W1L3.MAP", map, platformArea, platformConfig);
    platformSession.reset(player, simulation);
    for (int frame = 0; frame < 30; ++frame) {
        platformSession.tick(player, simulation, collision, quiky::InputState());
        platformSession.consumeEvent();
    }
    assert(player.grounded);
    assert(player.y.floorPixels() == 4);
}

void testPlayerSimulation() {
    const quiky::Map map = makePhysicsMap();
    quiky::PlayerConfig config;
    config.width = 12;
    config.height = 12;
    config.acceleration = quiky::Fixed16::kOne;
    config.maxHorizontalSpeed = 2 * quiky::Fixed16::kOne;
    config.friction = quiky::Fixed16::kOne;
    config.gravity = quiky::Fixed16::kOne;
    config.jumpVelocity = -4 * quiky::Fixed16::kOne;
    quiky::PlayerSimulation simulation(config);
    quiky::PlayerState player;
    simulation.reset(player, 16, 16);

    for (int frame = 0; frame < 60; ++frame) {
        simulation.tick(player, map, quiky::InputState());
    }
    assert(player.grounded);
    assert(player.y.floorPixels() == 52);

    const std::int32_t startX = player.x.floorPixels();
    quiky::InputState right;
    right.right = true;
    simulation.tick(player, map, right);
    assert(player.x.floorPixels() > startX);
    assert(player.facingRight);

    quiky::InputState jump;
    jump.jump = true;
    simulation.tick(player, map, jump);
    assert(!player.grounded);
    assert(player.velocityY.raw < 0);
    assert(player.y.floorPixels() < 52);

    const quiky::InputState actionFlags = quiky::InputState::fromActionFlags(0x2c);
    assert(actionFlags.left && actionFlags.right && actionFlags.jump);
}

void testPlayerInputTraceAndCollisionQuery() {
    TraceCollisionQuery collision;
    quiky::PlayerConfig config;
    config.width = 12;
    config.height = 12;
    config.acceleration = quiky::Fixed16::kOne;
    config.maxHorizontalSpeed = 2 * quiky::Fixed16::kOne;
    config.friction = quiky::Fixed16::kOne;
    config.gravity = quiky::Fixed16::kOne;
    config.jumpVelocity = -4 * quiky::Fixed16::kOne;
    quiky::PlayerSimulation simulation(config);
    quiky::PlayerState player;
    simulation.reset(player, 16, 16);

    struct TraceStep {
        std::uint16_t actionFlags;
        int frames;
    };
    const TraceStep trace[] = {
        {0x0000, 60}, // settle
        {0x0004, 30}, // right
        {0x0000, 20}, // brake
        {0x0008, 30}, // left
    };

    for (const TraceStep &step : trace) {
        const quiky::InputState input =
            quiky::InputState::fromActionFlags(step.actionFlags);
        for (int frame = 0; frame < step.frames; ++frame) {
            simulation.tick(player, collision, input);
        }
        if (step.actionFlags == 0x0000 && player.grounded) {
            assert(player.y.floorPixels() == 52);
        }
    }
    assert(player.x.floorPixels() < 16 + 30);
    assert(!player.facingRight);

    const quiky::InputState vertical =
        quiky::InputState::fromActionFlags(0x0003);
    assert(vertical.up && vertical.down);

    simulation.reset(player, 16, 52);
    player.grounded = true;
    simulation.tick(player, collision, quiky::InputState::fromActionFlags(0x0020));
    assert(!player.grounded);
    assert(player.velocityY.raw < 0);
    assert(player.y.floorPixels() < 52);

    simulation.reset(player, 16, 52);
    player.grounded = true;
    const quiky::InputState right =
        quiky::InputState::fromActionFlags(0x0004);
    for (int frame = 0; frame < 80; ++frame) {
        simulation.tick(player, collision, right);
    }
    assert(player.x.floorPixels() <= 84);
    assert(player.velocityX.raw == 0);
}

void testPlayerAnimationTables() {
    quiky::PlayerAnimation animation;
    quiky::PlayerState player;
    player.grounded = true;
    player.facingRight = true;

    assert(animation.action() == 0);
    assert(animation.slot() == 0);
    assert(animation.delay() == 14);
    for (int frame = 0; frame < 14; ++frame) {
        animation.advance(player);
    }
    assert(animation.slot() == 0);
    assert(animation.delay() == 0);
    animation.advance(player);
    assert(animation.slot() == 16);
    assert(animation.delay() == 14);

    player.velocityX = quiky::Fixed16::fromPixels(1);
    animation.advance(player);
    assert(animation.slot() == 0);
    assert(animation.delay() == 4);
    for (int frame = 0; frame < 5; ++frame) {
        animation.advance(player);
    }
    assert(animation.slot() == 1);

    animation.reset();
    player.grounded = false;
    player.velocityY = quiky::Fixed16::fromPixels(-1);
    animation.advance(player);
    assert(animation.slot() == 10);
    assert(animation.delay() == 8);
    player.velocityY = quiky::Fixed16::fromPixels(1);
    animation.advance(player);
    assert(animation.slot() == 13);
    assert(animation.delay() == 20);

    player.facingRight = false;
    animation.advance(player);
    assert(animation.slot() == 63);

    player.grounded = true;
    player.velocityX = quiky::Fixed16::fromPixels(1);
    animation.reset();
    player.facingRight = false;
    animation.setAction(4);
    animation.advance(player);
    assert(animation.action() == 4);
    assert(animation.slot() == 50);
    animation.setDeath(true);
    animation.advance(player);
    assert(animation.death());
    assert(animation.slot() == 70);
    // The native delay is 14, and the callback advances on the update after
    // the reload word reaches zero.
    for (int frame = 0; frame < 15; ++frame) {
        animation.advance(player);
    }
    assert(animation.slot() == 71);
    for (int frame = 0; frame < 7; ++frame) {
        for (int tick = 0; tick < 15; ++tick) {
            animation.advance(player);
        }
    }
    assert(animation.slot() == 78);
    for (int tick = 0; tick < 15; ++tick) {
        animation.advance(player);
    }
    assert(animation.slot() == 76);
    for (int tick = 0; tick < 15; ++tick) {
        animation.advance(player);
    }
    assert(animation.slot() == 77);
    for (int tick = 0; tick < 15; ++tick) {
        animation.advance(player);
    }
    assert(animation.slot() == 78);
    for (int tick = 0; tick < 15; ++tick) {
        animation.advance(player);
    }
    assert(animation.slot() == 76);
}

} // namespace

int main() {
    try {
        testReader();
        testArchive();
        testMapPaletteTilesetAndRenderer();
        testPcxAndIndexedLayerBlit();
        testRendererClippingAndTransparency();
        testAreaAndOverlay();
        testBobParserDecoderAndSheet();
        testPlayerSimulation();
        testPlayerInputTraceAndCollisionQuery();
        testPlayerAnimationTables();
        testLevelSession();
        testLevelSessionCollisionQuery();
    } catch (const std::exception &error) {
        std::cerr << "unexpected test failure: " << error.what() << "\n";
        return 1;
    }
    std::cout << "all quiky format tests passed\n";
    return 0;
}
