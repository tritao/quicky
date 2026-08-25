#include "quiky/player_update.h"

#include <cassert>
#include <cstdint>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

namespace {

std::int32_t value(const std::string &text) {
    return static_cast<std::int32_t>(std::strtol(text.c_str(), 0, 10));
}

quiky::PlayerRecord playerFor(std::int8_t mode, std::uint16_t action,
                              std::uint8_t scratch, std::int32_t y,
                              std::int32_t velocity) {
    quiky::PlayerRecord player;
    player.initializeConfirmedHorizontalFields();
    player.mode37 = mode;
    player.actionWord = action;
    player.contactScratch2B = scratch;
    player.positionY = quiky::Fixed16::fromRaw(y);
    player.velocityY = quiky::Fixed16::fromRaw(velocity);
    player.syncToRaw();
    return player;
}

void testEvidenceFixture(const std::string &path) {
    std::ifstream stream(path.c_str());
    assert(stream);
    std::string line;
    std::size_t rows = 0;
    while (std::getline(stream, line)) {
        if (line.empty() || line[0] == '#') {
            continue;
        }
        std::istringstream fields(line);
        std::string name;
        std::int32_t mode, action, scratch, yBefore, velocityBefore;
        std::int32_t yAfter, velocityAfter, modeAfter;
        fields >> name >> mode >> action >> scratch >> yBefore >> velocityBefore
               >> yAfter >> velocityAfter >> modeAfter;
        assert(fields);
        quiky::PlayerRecord player = playerFor(
            static_cast<std::int8_t>(mode), static_cast<std::uint16_t>(action),
            static_cast<std::uint8_t>(scratch), yBefore, velocityBefore);
        const quiky::VerticalFreeSpaceResult result =
            quiky::updatePlayerVerticalFreeSpace(player);
        assert(result.status == quiky::VerticalFreeSpaceStatus::Applied);
        assert(result.postState.s32(0x06) == yAfter);
        assert(result.postState.s32(0x0e) == velocityAfter);
        assert(result.postState.s8(0x37) == modeAfter);
        ++rows;
    }
    assert(rows == 15);
}

void testReleaseClampAndHoldGate() {
    quiky::PlayerRecord released = playerFor(-1, 0, 0, 0x01000000,
                                             -0x4a000);
    quiky::updatePlayerVerticalFreeSpace(released);
    assert(released.velocityY.raw == -0x20000);

    quiky::PlayerRecord held = playerFor(-1, 0x22, 0, 0x01000000,
                                         -0x4a000);
    quiky::updatePlayerVerticalFreeSpace(held);
    assert(held.velocityY.raw == -0x48000);

    quiky::PlayerRecord contact = playerFor(-1, 0, 1, 0x01000000,
                                            -0x4a000);
    quiky::updatePlayerVerticalFreeSpace(contact);
    assert(contact.velocityY.raw == -0x48000);
}

void testTerminalVelocityAndOrdinaryBoundary() {
    quiky::PlayerRecord falling = playerFor(1, 0, 0, 0x01000000,
                                            0x3f000);
    quiky::updatePlayerVerticalFreeSpace(falling);
    assert(falling.velocityY.raw == 0x40000);
    assert(falling.positionY.raw == 0x01040000);

    quiky::PlayerRecord ordinary = playerFor(0, 0x22, 0, 0x01000000, 0);
    const quiky::PlayerRawRecord before = ordinary.toRaw();
    const quiky::VerticalFreeSpaceResult result =
        quiky::updatePlayerVerticalFreeSpace(ordinary);
    assert(result.status == quiky::VerticalFreeSpaceStatus::OrdinaryModeRequiresContactResolution);
    assert(result.postState.bytes == before.bytes);
}

void testTraceClosedUpdaterIntegratesAirborneBranch() {
    quiky::Map map;
    map.width = 1;
    map.height = 64;
    map.unknown = 0;
    map.cells.assign(map.width * map.height, 0);
    const quiky::WorldCollisionView world(map);
    quiky::TraceClosedPlayerUpdate updater;

    quiky::PlayerRecord ascent = playerFor(-1, 0, 0, 25100288, -8192);
    const quiky::PlayerRawRecord ascentBefore = ascent.toRaw();
    quiky::PlayerUpdateTrace ascentTrace;
    updater.updatePlayer(ascent, quiky::InputState(), world, &ascentTrace);
    assert(ascent.positionY.raw == 25100288);
    assert(ascent.velocityY.raw == 0);
    assert(ascent.mode37 == 1);
    assert(ascentTrace.preState.s32(0x06) == ascentBefore.s32(0x06));
    assert(ascentTrace.postState.s32(0x06) == ascent.positionY.raw);
    assert(ascentTrace.postState.s32(0x0e) == ascent.velocityY.raw);

    quiky::PlayerRecord descent = playerFor(1, 0, 0, 25100288, 0);
    updater.updatePlayer(descent, quiky::InputState(), world, 0);
    assert(descent.positionY.raw == 25110528);
    assert(descent.velocityY.raw == 10240);

    quiky::PlayerRecord ordinary = playerFor(0, 0, 0, 25100288, 0);
    const quiky::PlayerRawRecord ordinaryBefore = ordinary.toRaw();
    updater.updatePlayer(ordinary, quiky::InputState(), world, 0);
    assert(ordinary.positionY.raw == ordinaryBefore.s32(0x06));
    assert(ordinary.velocityY.raw == ordinaryBefore.s32(0x0e));
    assert(ordinary.mode37 == 0);
}

} // namespace

int main(int argc, char **argv) {
    assert(argc == 2);
    testEvidenceFixture(argv[1]);
    testReleaseClampAndHoldGate();
    testTerminalVelocityAndOrdinaryBoundary();
    testTraceClosedUpdaterIntegratesAirborneBranch();
    std::cout << "vertical free-space tests passed\n";
    return 0;
}
