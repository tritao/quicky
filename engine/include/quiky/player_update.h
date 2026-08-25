#ifndef QUIKY_PLAYER_UPDATE_H
#define QUIKY_PLAYER_UPDATE_H

#include "quiky/player_record.h"
#include "quiky/trace.h"

#include <cstdint>
#include <vector>

namespace quiky {

enum class PlayerUpdateStage {
    SampleInput,
    HorizontalIntent,
    VerticalIntent,
    IntegratePosition,
    ProbeWorld,
    ResolveCollision,
    UpdateActionAnimation,
    EmitEvents
};

struct PlayerUpdateTrace {
    std::vector<PlayerUpdateStage> stages;
    std::vector<TraceStateWrite> stateWrites;
    std::vector<TraceMapLookup> mapLookups;
    std::vector<SimulationEvent> emittedEvents;
};

// Phase-7 callback boundary. Implementations must keep uncertain stages
// replaceable and must not smuggle platform, renderer, or audio ownership
// into the player record.
class PlayerUpdateCallback {
public:
    virtual ~PlayerUpdateCallback() {}

    virtual void updatePlayer(RecoveredPlayerState &player,
                              const InputState &input,
                              const WorldCollisionView &world,
                              PlayerUpdateTrace *trace) = 0;
};

} // namespace quiky

#endif
