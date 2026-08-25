#include "quiky/archive.h"
#include "quiky/level_runtime.h"
#include "quiky/player_update.h"
#include "quiky/simulation.h"

#include <cassert>
#include <cstdint>
#include <exception>
#include <iostream>
#include <map>
#include <string>

namespace {

struct FamilyContract {
    quiky::EntityKind kind;
    std::uint16_t callback;
    const char *resource;
    bool effectResource;
};

FamilyContract contractFor(std::uint16_t type) {
    if (type == 0x01) {
        return {quiky::EntityKind::Hazard, 0x6dc4, "WURM2.BOB", false};
    }
    if (type == 0x03 || type == 0x04) {
        return {quiky::EntityKind::Hazard, 0x68c0, "BIENE.BOB", false};
    }
    if (type == 0x1f || type == 0x20) {
        return {quiky::EntityKind::EnvironmentalEffect, 0x8e4b,
                "W1.ICO", true};
    }
    if (type == 0x28) {
        return {quiky::EntityKind::AmbientVisual, 0x9269, "WOLKE.BOB", false};
    }
    if (type >= 0x29 && type <= 0x2b) {
        return {quiky::EntityKind::AmbientVisual, 0x47e7, "BLATT.BOB", false};
    }
    if (type == 0x65 || type == 0x67) {
        return {quiky::EntityKind::AmbientVisual, 0x0000, "LOOP_W1.ICO", true};
    }
    if (type == 0x6f || type == 0x71 || type == 0x72 ||
        (type >= 0x79 && type <= 0x7f)) {
        return {quiky::EntityKind::Collectible, 0x8d20,
                type >= 0x79 ? "PUZZLE.BOB" : "WERBE.BOB", false};
    }
    return {quiky::EntityKind::Unknown, 0x0000, "", false};
}

void testW1L1Inventory(const std::string &archivePath) {
    const quiky::Archive archive = quiky::Archive::load(archivePath);
    quiky::LevelSessionConfig config;
    config.enableEdgeExit = false;

    quiky::Simulation simulation;
    quiky::TraceClosedPlayerUpdate playerUpdater;
    simulation.setExperimentalPlayerUpdater(&playerUpdater);
    std::unique_ptr<quiky::LevelRuntime> runtime =
        quiky::LevelRuntime::load(archive, "W1L1.MAP", "QUIKYW1.BOB", config);

    const std::map<std::uint16_t, std::size_t> expected = {
        {0x01, 10}, {0x03, 7},  {0x04, 3},  {0x1f, 1}, {0x20, 1},
        {0x28, 1},  {0x29, 5}, {0x2a, 12}, {0x2b, 39}, {0x65, 66},
        {0x67, 14}, {0x6f, 2}, {0x71, 4}, {0x72, 1}, {0x79, 1},
        {0x7a, 1},  {0x7b, 1}, {0x7c, 1}, {0x7d, 1}, {0x7e, 1},
        {0x7f, 1},
    };
    std::map<std::uint16_t, std::size_t> actual;
    for (std::size_t index = 0; index < runtime->session().entities().size();
         ++index) {
        const quiky::LevelEntity &entity = runtime->session().entities()[index];
        ++actual[entity.type];

        if (entity.type == 0x6f) {
            // 01F7:8BC2 is an initializer, not a renderer-time offset.
            assert(entity.x == static_cast<std::int32_t>(
                entity.initialX + 1));
            assert(entity.y == static_cast<std::int32_t>(
                entity.initialY - 2));
            assert(entity.spriteSlot == 607);
            assert(entity.contactSubtype == 1);
        }

        const FamilyContract contract = contractFor(entity.type);
        assert(contract.kind != quiky::EntityKind::Unknown);
        assert(entity.kind == contract.kind);
        assert(entity.updateCallback.offset == contract.callback);
        if (contract.effectResource) {
            assert(entity.effectResource == contract.resource);
        } else {
            assert(entity.spriteResource == contract.resource);
        }
    }
    assert(actual == expected);
    assert(runtime->session().entities().size() == 173);

    // Confirm that the native player start publishes the callback-bearing
    // subset through the deterministic scheduler, while child-only event
    // declarations remain outside the normal callback list.
    runtime->reset(simulation);
    assert(runtime->session().hasStreamAnchor());
    assert(runtime->session().streamAnchorX() == 0);
    assert(runtime->session().streamAnchorY() == 262);
    const quiky::PlayerRecord &startupPlayer = simulation.state().player;
    assert(startupPlayer.callbackOffset18 == 0x3ff8);
    assert(startupPlayer.field17 == 2);
    assert(startupPlayer.field1C == 0x1997);
    assert(startupPlayer.animationDelay20 == 0x000e);
    assert(startupPlayer.animationCursor22 == 0x316c);
    assert(startupPlayer.state2C == -10);
    assert(startupPlayer.state30 == 10);
    assert(startupPlayer.gate38 == 0xff);

    bool startupLeafPublished = false;
    std::size_t scheduledDeclarations = 0;
    for (std::size_t index = 0; index < runtime->session().entities().size();
         ++index) {
        const quiky::LevelEntity &entity = runtime->session().entities()[index];
        if (entity.type == 0x2b && entity.x == 128 && entity.y == 256) {
            startupLeafPublished = entity.active;
        }
        if (entity.active && entity.updateCallback.offset != 0) {
            ++scheduledDeclarations;
            assert(entity.schedulerHandle.valid());
        }
    }
    assert(startupLeafPublished);
    assert(scheduledDeclarations > 0);
    assert(simulation.state().scheduler.activeCount() == 0);
    simulation.stateForSetup().scheduler.beginTick(1);
    assert(simulation.state().scheduler.activeCount() == scheduledDeclarations);

    quiky::PlayerUpdateTrace startupTrace;
    simulation.setPlayerTraceSink(&startupTrace);
    quiky::SimulationOutput startupOutput;
    runtime->tick(simulation, quiky::InputState(), startupOutput);
    simulation.setPlayerTraceSink(0);
    assert(startupTrace.hasPreState);
    assert(startupTrace.preState.toBytes().size() == 0x78);
    assert(startupTrace.postState.toBytes().size() == 0x78);
    assert(startupOutput.player.toBytes().size() == 0x78);
}

} // namespace

int main(int argc, char **argv) {
    if (argc != 2) {
        std::cerr << "usage: quiky-w1l1-inventory-tests NESTLE.DAT\n";
        return 1;
    }
    try {
        testW1L1Inventory(argv[1]);
    } catch (const std::exception &error) {
        std::cerr << "W1L1 inventory test failed: " << error.what() << "\n";
        return 1;
    }
    std::cout << "W1L1 object inventory passed\n";
    return 0;
}
