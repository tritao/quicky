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
    assert(next->mapName() == "W1L4.MAP");
    assert(next->areaName() == "W1L4.ARE");
    assert(next->worldName() == "W1");
    assert(next->session().entities().size() > 0);
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
