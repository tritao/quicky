#include "quiky/archive.h"
#include "quiky/level_runtime.h"
#include "quiky/player_update.h"
#include "quiky/simulation.h"

#include <cassert>
#include <exception>
#include <iostream>
#include <string>
#include <vector>

namespace {

void testNativeReloadClosure(const std::string &archivePath) {
    const quiky::Archive archive = quiky::Archive::load(archivePath);
    quiky::LevelSessionConfig config;
    config.enableEdgeExit = false;

    quiky::Simulation simulation;
    quiky::TraceClosedPlayerUpdate playerUpdater;
    simulation.setExperimentalPlayerUpdater(&playerUpdater);
    std::unique_ptr<quiky::LevelRuntime> current =
        quiky::LevelRuntime::load(archive, "W1L1.MAP", "QUIKYW1.BOB", config);
    current->reset(simulation);
    // Native post-completion state observed at 01D7:5010. These fields must
    // survive the target-level rebuild; puzzle/action state must not.
    current->session().gameplayStateForSetup().score881c = 2950;
    current->session().gameplayStateForSetup().lives880a = 4;
    current->session().gameplayStateForSetup().currentHealth8822 = 3;
    current->session().gameplayStateForSetup().maximumHealth8824 = 3;
    current->session().gameplayStateForSetup().puzzleMask60d8 = 0x007f;
    current->session().gameplayStateForSetup().ammo880c = 10;
    current->session().gameplayStateForSetup().invulnerabilityGate8810 = 0xffff;

    quiky::LevelReloadTrace trace;
    const std::vector<quiky::LevelReloadStage> expected = {
        quiky::LevelReloadStage::TransitionGate5010,
        quiky::LevelReloadStage::ObjectTeardown0908,
        quiky::LevelReloadStage::ResourceLookup18C7,
        quiky::LevelReloadStage::TransitionBufferCopy0D5A,
        quiky::LevelReloadStage::PlayerReposition1AAA,
        quiky::LevelReloadStage::AnimationLoader5D38,
        quiky::LevelReloadStage::CameraRebuild321F,
        quiky::LevelReloadStage::WorldDispatch313D,
        quiky::LevelReloadStage::Cleanup504F,
    };
    std::unique_ptr<quiky::LevelRuntime> next = current->reload(
        archive, "W1L4.MAP", simulation, config, &trace);

    assert(trace.targetMap == "W1L4.MAP");
    assert(trace.stages == expected);
    assert(trace.hasPlayerSpawn);
    assert(trace.playerSpawnX == 288 && trace.playerSpawnY == 144);
    assert(next->mapName() == "W1L4.MAP");
    assert(next->areaName() == "W1L4.ARE");
    assert(next->worldName() == "W1");
    assert(next->session().entities().size() > 0);
    assert(simulation.state().player.positionX.floorPixels() == 288);
    assert(simulation.state().player.positionY.floorPixels() == 144);
    assert(next->session().score() == 2950);
    assert(next->session().gameplayState().score881c == 2950);
    assert(next->session().gameplayState().lives880a == 4);
    assert(next->session().gameplayState().currentHealth8822 == 3);
    assert(next->session().gameplayState().maximumHealth8824 == 3);
    assert(next->session().gameplayState().puzzleMask60d8 == 0);
    assert(next->session().gameplayState().ammo880c == 0);
    assert(next->session().gameplayState().invulnerabilityGate8810 == 0);
    assert(simulation.state().queuedEvents.empty());
    assert(simulation.state().player.toBytes().size() == 0x78);
}

} // namespace

int main(int argc, char **argv) {
    if (argc != 2) {
        std::cerr << "usage: quiky-transition-reload-tests NESTLE.DAT\n";
        return 1;
    }
    try {
        testNativeReloadClosure(argv[1]);
    } catch (const std::exception &error) {
        std::cerr << "transition reload test failed: " << error.what() << "\n";
        return 1;
    }
    std::cout << "transition reload closure passed\n";
    return 0;
}
