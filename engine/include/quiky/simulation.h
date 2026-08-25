#ifndef QUIKY_SIMULATION_H
#define QUIKY_SIMULATION_H

#include "quiky/player_record.h"
#include "quiky/scheduler.h"
#include "quiky/world_view.h"

#include <cstdint>
#include <deque>
#include <vector>

namespace quiky {

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

struct SimulationOutput {
    std::uint64_t tick;
    std::uint16_t inputFlags;
    RecoveredPlayerState player;
    std::vector<RenderObjectState> renderObjects;
    std::vector<SchedulerInvocation> schedulerCallbacks;
    std::vector<SimulationEvent> gameEvents;
    std::vector<AudioEvent> audioEvents;

    SimulationOutput();
    void clearForTick(std::uint64_t tickValue, const InputState &input);
};

// Mutable state owned by the simulation boundary. World data is supplied as a
// const view to tick(); renderer and audio code only receive SimulationOutput.
struct SimulationState {
    std::uint64_t tick;
    RecoveredPlayerState player;
    ObjectScheduler scheduler;
    std::deque<SimulationEvent> queuedEvents;

    explicit SimulationState(std::size_t schedulerCapacity = 64);
};

class Simulation {
public:
    explicit Simulation(std::size_t schedulerCapacity = 64);

    void reset();
    void enqueueEvent(const SimulationEvent &event);

    // One deterministic gameplay boundary. This first shell advances the tick
    // and scheduler and drains events; player behavior is intentionally not
    // called until its recovered callback stages have evidence-backed formulas.
    void tick(const InputState &input,
              const WorldCollisionView &world,
              SimulationOutput &output);

    const SimulationState &state() const { return _state; }
    SimulationState &stateForSetup() { return _state; }

private:
    SimulationState _state;
};

} // namespace quiky

#endif
