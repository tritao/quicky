#ifndef QUIKY_SIMULATION_H
#define QUIKY_SIMULATION_H

#include "quiky/player_record.h"
#include "quiky/scheduler.h"
#include "quiky/world_view.h"

#include <cstdint>
#include <deque>
#include <vector>

namespace quiky {

class PlayerUpdateCallback;
struct PlayerUpdateTrace;

enum class SimulationEventKind {
    Game,
    Audio
};

struct SimulationEvent {
    SimulationEventKind kind;
    std::uint64_t tick;
    std::uint32_t sourceId;
    std::uint16_t code;
    std::int32_t value;

    SimulationEvent(SimulationEventKind kindValue = SimulationEventKind::Game,
                    std::uint64_t tickValue = 0,
                    std::uint32_t sourceValue = 0,
                    std::uint16_t codeValue = 0,
                    std::int32_t valueValue = 0)
        : kind(kindValue),
          tick(tickValue),
          sourceId(sourceValue),
          code(codeValue),
          value(valueValue) {}
};

struct AudioEvent {
    std::uint64_t tick;
    std::uint32_t sourceId;
    std::uint16_t code;
    std::int32_t value;
};

// This is the read-only snapshot consumed by a renderer. It contains no
// mutable simulation references and has no methods that can alter state.
struct RenderObjectState {
    std::uint16_t slot;
    std::uint16_t generation;
    std::uint32_t sourceId;
    bool playerCallback;
    bool cameraParticipating;
};

// Narrow ordering trace for callbacks that can feed player simulation. It is
// separate from schedulerCallbacks because the player callback is invoked by
// the simulation boundary while moving-platform callbacks are currently
// executed by LevelSession before that boundary.
enum class SimulationCallbackPhase {
    MovingPlatformBeforePlayer,
    PlayerUpdate,
};

struct SimulationCallbackStep {
    SimulationCallbackPhase phase;
    std::uint32_t sourceId;
    CallbackIdentity callback;

    SimulationCallbackStep(
        SimulationCallbackPhase phaseValue =
            SimulationCallbackPhase::PlayerUpdate,
        std::uint32_t sourceIdValue = 0,
        const CallbackIdentity &callbackValue = CallbackIdentity())
        : phase(phaseValue), sourceId(sourceIdValue), callback(callbackValue) {}
};

struct SimulationOutput {
    std::uint64_t tick;
    std::uint16_t inputFlags;
    PlayerRecord player;
    std::vector<RenderObjectState> renderObjects;
    std::vector<SchedulerInvocation> schedulerCallbacks;
    std::vector<SimulationCallbackStep> playerDependencyOrder;
    std::vector<SimulationEvent> gameEvents;
    std::vector<AudioEvent> audioEvents;

    SimulationOutput();
    void clearForTick(std::uint64_t tickValue, const InputState &input);
};

// Mutable state owned by the simulation boundary. World data is supplied as a
// const view to tick(); renderer and audio code only receive SimulationOutput.
struct SimulationState {
    std::uint64_t tick;
    PlayerRecord player;
    ObjectScheduler scheduler;
    std::deque<SimulationEvent> queuedEvents;

    explicit SimulationState(std::size_t schedulerCapacity = 64);
};

class Simulation {
public:
    explicit Simulation(std::size_t schedulerCapacity = 64);

    void reset();
    void enqueueEvent(const SimulationEvent &event);
    void setExperimentalPlayerUpdater(PlayerUpdateCallback *updater);
    // Optional caller-owned sink for the recovered player callback trace. The
    // simulation still owns the player state; this only exposes the exact
    // callback-local evidence at the session boundary.
    void setPlayerTraceSink(PlayerUpdateTrace *sink);
    PlayerUpdateCallback *playerUpdater() const {
        return _experimentalPlayerUpdater;
    }

    // One deterministic gameplay boundary. It advances the recovered player
    // callback (when installed), scheduler, and queued events, then exposes a
    // read-only output snapshot to rendering and audio.
    void tick(const InputState &input,
              const WorldCollisionView &world,
              SimulationOutput &output);

    const SimulationState &state() const { return _state; }
    SimulationState &stateForSetup() { return _state; }

private:
    SimulationState _state;
    PlayerUpdateCallback *_experimentalPlayerUpdater;
    PlayerUpdateTrace *_playerTraceSink;
};

} // namespace quiky

#endif
