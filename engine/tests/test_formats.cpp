#include "quiky/area.h"
#include "quiky/archive.h"
#include "quiky/bob.h"
#include "quiky/binary_reader.h"
#include "quiky/map.h"
#include "quiky/palette.h"
#include "quiky/renderer.h"
#include "quiky/tileset.h"

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
    appendU16BE(mapData, 0x0001);
    const quiky::Map map = quiky::Map::parse(mapData, "test.MAP");
    assert(map.width == 2 && map.height == 1);
    assert(quiky::Map::tileId(0x3812) == 0x012);
    assert(quiky::Map::properties(0x3812) == 0x1c);

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

    const quiky::IndexedSurface surface = quiky::renderMap(map, tileset);
    assert(surface.width == 32 && surface.height == 16);
    assert(surface.at(0, 0) == 0);
    assert(surface.at(1, 0) == 4);
    assert(surface.at(16, 0) == 7);
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
}

} // namespace

int main() {
    try {
        testReader();
        testArchive();
        testMapPaletteTilesetAndRenderer();
        testAreaAndOverlay();
        testBobParserDecoderAndSheet();
    } catch (const std::exception &error) {
        std::cerr << "unexpected test failure: " << error.what() << "\n";
        return 1;
    }
    std::cout << "all quiky format tests passed\n";
    return 0;
}
