#include "quiky/simulation.h"

#include "quiky/player_update.h"

namespace quiky {

SimulationOutput::SimulationOutput()
    : tick(0),
      inputFlags(0),
      player(),
      renderObjects(),
      schedulerCallbacks(),
      gameEvents(),
      audioEvents() {
}

void SimulationOutput::clearForTick(std::uint64_t tickValue,
                                    const InputState &input) {
    tick = tickValue;
    inputFlags = input.actionFlags();
    renderObjects.clear();
    schedulerCallbacks.clear();
    gameEvents.clear();
    audioEvents.clear();
}

SimulationState::SimulationState(std::size_t schedulerCapacity)
    : tick(0), player(), scheduler(schedulerCapacity), queuedEvents() {
}

Simulation::Simulation(std::size_t schedulerCapacity)
    : _state(schedulerCapacity), _experimentalPlayerUpdater(0) {
}

void Simulation::reset() {
    _state.tick = 0;
    _state.player = PlayerRecord();
    _state.scheduler.reset();
    _state.queuedEvents.clear();
}

void Simulation::enqueueEvent(const SimulationEvent &event) {
    _state.queuedEvents.push_back(event);
}

void Simulation::setExperimentalPlayerUpdater(PlayerUpdateCallback *updater) {
    _experimentalPlayerUpdater = updater;
}

void Simulation::tick(const InputState &input,
                      const WorldCollisionView &world,
                      SimulationOutput &output) {
    ++_state.tick;
    _state.scheduler.beginTick(_state.tick);
    if (_experimentalPlayerUpdater != 0) {
        PlayerUpdateTrace trace;
        _experimentalPlayerUpdater->updatePlayer(_state.player, input, world,
                                                  &trace);
    }
    output.clearForTick(_state.tick, input);
    output.player = _state.player;
    output.player.syncToRaw();
    output.schedulerCallbacks = _state.scheduler.invocations();

    const std::vector<SchedulerObject> &objects = _state.scheduler.objects();
    for (std::size_t index = 0; index < objects.size(); ++index) {
        const SchedulerObject &object = objects[index];
        if (!object.active) {
            continue;
        }
        RenderObjectState render;
        render.slot = static_cast<std::uint16_t>(index);
        render.generation = object.generation;
        render.sourceId = object.sourceId;
        render.playerCallback = object.playerCallback;
        render.cameraParticipating = object.cameraParticipating;
        output.renderObjects.push_back(render);
    }

    while (!_state.queuedEvents.empty()) {
        SimulationEvent event = _state.queuedEvents.front();
        _state.queuedEvents.pop_front();
        if (event.tick == 0) {
            event.tick = _state.tick;
        }
        if (event.kind == SimulationEventKind::Audio) {
            AudioEvent audio;
            audio.tick = event.tick;
            audio.sourceId = event.sourceId;
            audio.code = event.code;
            audio.value = event.value;
            output.audioEvents.push_back(audio);
        } else {
            output.gameEvents.push_back(event);
        }
    }
}

} // namespace quiky
