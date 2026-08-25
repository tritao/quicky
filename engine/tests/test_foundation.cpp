#include "quiky/fixed.h"
#include "quiky/player_record.h"
#include "quiky/player_animation.h"
#include "quiky/scheduler.h"
#include "quiky/simulation.h"
#include "quiky/trace.h"
#include "quiky/player_update.h"
#include "quiky/world_view.h"

#include <cassert>
#include <cstdint>
#include <iostream>
#include <sstream>

namespace {

quiky::Map makeMap() {
    quiky::Map map;
    map.width = 2;
    map.height = 1;
    map.unknown = 0;
    map.cells.push_back(static_cast<std::uint16_t>(0x0012 | (0x55 << 9)));
    map.cells.push_back(0x0003);
    return map;
}

void testFixedPointTables() {
    struct Row {
        std::int32_t raw;
        std::int32_t floor;
        std::int32_t trunc;
    };
    const Row rows[] = {{0, 0, 0},
                        {1, 0, 0},
                        {-1, -1, 0},
                        {0xffff, 0, 0},
                        {-0x10000, -1, -1},
                        {0x10000, 1, 1},
                        {-0x10001, -2, -1}};
    for (std::size_t index = 0; index < sizeof(rows) / sizeof(rows[0]); ++index) {
        assert(quiky::Fixed16::floorRaw(rows[index].raw) == rows[index].floor);
        assert(quiky::Fixed16::truncRaw(rows[index].raw) == rows[index].trunc);
    }

    assert(quiky::Fixed16::arithmeticShiftRight(-1, 1) == -1);
    assert(quiky::Fixed16::arithmeticShiftRight(0x7fffffff, 16) == 0x7fff);
    assert(quiky::Fixed16::wrapAddRaw(0x7fffffff, 1) ==
           static_cast<std::int32_t>(-2147483647 - 1));
    assert(quiky::Fixed16::wrapSubRaw(static_cast<std::int32_t>(-2147483647 - 1),
                                      1) == 0x7fffffff);
    assert(quiky::Fixed16::wrapNegRaw(static_cast<std::int32_t>(-2147483647 - 1)) ==
           static_cast<std::int32_t>(-2147483647 - 1));
    assert(quiky::Fixed16::wrapMulRaw(0x10000, 0x20000) == 0x20000);
    assert(quiky::Fixed16::wrapMulRaw(-0x10000, 0x20000) == -0x20000);
    assert((quiky::Fixed16(0x10000) * quiky::Fixed16(0x20000)).raw == 0x20000);
    assert(quiky::Fixed16::fromPixels(-1).raw == -0x10000);
    assert(quiky::Fixed16::clampRaw(-3, -2, 2) == -2);
    assert(quiky::Fixed16::clampRaw(3, -2, 2) == 2);
}

void testInputAndPlayerRecord() {
    const quiky::InputState input = quiky::InputState::fromActionFlags(0x3f);
    assert(input.left && input.right && input.up && input.down && input.jump &&
           input.alternate);
    assert(input.actionFlags() == 0x3f);

    quiky::Bytes bytes(quiky::PlayerRawRecord::kSize);
    for (std::size_t index = 0; index < bytes.size(); ++index) {
        bytes[index] = static_cast<quiky::byte>(index ^ 0xa5);
    }
    quiky::PlayerRawRecord raw = quiky::PlayerRawRecord::fromBytes(bytes);
    raw.setU16(0x00, 0x1234);
    raw.setS32(0x02, 0x00018000);
    raw.setS32(0x06, static_cast<std::int32_t>(-0x00008000));
    raw.setS32(0x0a, 0x00001000);
    raw.setU8(0x38, 0x7e);

    quiky::RecoveredPlayerState state =
        quiky::RecoveredPlayerState::fromRaw(raw);
    assert(state.actionWord == 0x1234);
    assert(state.positionX.raw == 0x00018000);
    assert(state.positionY.raw == -0x00008000);
    assert(state.velocityX.raw == 0x00001000);
    assert(state.gate38 == 0x7e);
    assert(state.raw.bytes[0x18] == bytes[0x18]);

    state.positionX = quiky::Fixed16::fromRaw(0x22220000);
    const quiky::PlayerRawRecord updated = state.toRaw();
    assert(updated.s32(0x02) == 0x22220000);
    assert(updated.bytes[0x18] == bytes[0x18]);
}

void testPlayerAnimationTables() {
    quiky::PlayerAnimation animation;
    quiky::PlayerRecord player;
    player.mode37 = 0;
    player.motionDirectionByte29 = 1;

    animation.reset();
    animation.advance(player);
    assert(animation.slot() == 0);

    player.mode37 = -1;
    animation.advance(player);
    assert(animation.slot() == 10);

    player.motionDirectionByte29 = 0xff;
    animation.advance(player);
    assert(animation.slot() == 60);

    player.mode37 = 1;
    animation.advance(player);
    assert(animation.slot() == 63);

    player.mode37 = 0;
    player.velocityX = quiky::Fixed16::fromPixels(1);
    animation.advance(player);
    assert(animation.slot() == 50);
}

void testWorldView() {
    const quiky::Map map = makeMap();
    quiky::PlayerDescriptorTable descriptors;
    descriptors.setWord(0x12, 0x0068);
    const quiky::WorldCollisionView world(map, &descriptors);

    const quiky::MapCell cell = world.cellAt(0, 0);
    assert(cell.inBounds);
    assert(cell.rawWord == static_cast<std::uint16_t>(0x0012 | (0x55 << 9)));
    assert(cell.tileId == 0x12);
    assert(cell.flags == 0x55);
    const quiky::TileDescriptor descriptor = world.descriptorFor(cell);
    assert(descriptor.valid && descriptor.tileId == 0x12);
    assert(descriptor.descriptorWord == 0x0068);
    assert(descriptor.runtimeFlags == 0x0068);
    assert(world.blocksProbeConfirmed(3, 3));
    assert(world.hasVerticalResponseConfirmed(3, 3));
    assert(world.alignsEightPixelsConfirmed(3, 3));

    const quiky::MapCell outside = world.cellAt(-1, 0);
    assert(!outside.inBounds);
    assert(!world.descriptorFor(outside).valid);
}

void testSchedulerAndBoundary() {
    quiky::ObjectScheduler scheduler(3);
    const quiky::SchedulerHandle first =
        scheduler.queueSpawn(quiky::CallbackIdentity(0x027f, 0x1111, "first"),
                             10, true);
    const quiky::SchedulerHandle second =
        scheduler.queueSpawn(quiky::CallbackIdentity(0x027f, 0x2222, "second"),
                             20, false);
    assert(first.valid() && second.valid());
    assert(scheduler.activeCount() == 0);
    scheduler.setCameraParticipation(second, false);
    scheduler.beginTick(1);
    assert(scheduler.activeCount() == 2);
    assert(scheduler.invocations().size() == 1);
    assert(scheduler.invocations()[0].slot == first.slot);
    assert(scheduler.invocations()[0].playerCallback);

    scheduler.queueRelease(first);
    scheduler.beginTick(2);
    assert(scheduler.activeCount() == 1);
    assert(scheduler.invocations().empty());
    const quiky::SchedulerHandle replacement =
        scheduler.queueSpawn(quiky::CallbackIdentity(0x027f, 0x3333), 30);
    scheduler.beginTick(3);
    assert(replacement.slot == first.slot);
    assert(scheduler.invocations().size() == 1);
    assert(scheduler.invocations()[0].callback.offset == 0x3333);

    const quiky::Map map = makeMap();
    const quiky::WorldCollisionView world(map);
    quiky::Simulation simulation(2);
    const quiky::SchedulerHandle object = simulation.stateForSetup().scheduler.queueSpawn(
        quiky::CallbackIdentity(0x027f, 0x4444), 44);
    assert(object.valid());
    simulation.enqueueEvent(quiky::SimulationEvent(
        quiky::SimulationEventKind::Game, 0, 44, 7, 8));
    simulation.enqueueEvent(quiky::SimulationEvent(
        quiky::SimulationEventKind::Audio, 0, 44, 9, 10));
    quiky::SimulationOutput output;
    simulation.tick(quiky::InputState::fromActionFlags(0x24), world, output);
    assert(output.tick == 1);
    assert(output.inputFlags == 0x24);
    assert(output.renderObjects.size() == 1);
    assert(output.gameEvents.size() == 1);
    assert(output.audioEvents.size() == 1);
}

void testTraceComparison() {
    quiky::TraceFrame frame;
    frame.tick = 4;
    frame.inputFlags = 0x20;
    frame.playerSelector = 0x027f;
    frame.playerOffset = 0;
    frame.player.setU32(0x02, 0x12345678);
    frame.stateWrites.push_back(quiky::TraceStateWrite{0x02, 4, 0x12345678});

    std::ostringstream expectedText;
    quiky::TraceWriter expectedWriter(expectedText);
    expectedWriter.writeHeader();
    expectedWriter.writeFrame(frame);

    std::istringstream expected(expectedText.str());
    std::istringstream actual(expectedText.str());
    assert(quiky::TraceComparator::compare(expected, actual).equal);

    frame.player.setU32(0x02, 0x12345679);
    std::ostringstream actualText;
    quiky::TraceWriter actualWriter(actualText);
    actualWriter.writeHeader();
    actualWriter.writeFrame(frame);
    std::istringstream expectedChanged(expectedText.str());
    std::istringstream actualChanged(actualText.str());
    const quiky::TraceDifference difference =
        quiky::TraceComparator::compare(expectedChanged, actualChanged);
    assert(!difference.equal);
    assert(difference.tick == 4);
    assert(difference.field == "raw_player_record");
}

} // namespace

int main() {
    testFixedPointTables();
    testInputAndPlayerRecord();
    testPlayerAnimationTables();
    testWorldView();
    testSchedulerAndBoundary();
    testTraceComparison();
    std::cout << "all quiky foundation tests passed\n";
    return 0;
}
