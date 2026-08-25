#ifndef QUIKY_PLAYER_UPDATE_H
#define QUIKY_PLAYER_UPDATE_H

#include "quiky/collision_kernel.h"
#include "quiky/player_record.h"
#include "quiky/trace.h"

#include <cstdint>
#include <vector>

namespace quiky {

class PlayerTraceSink;

enum class PlayerUpdateStage {
    CapturePreState,
    SampleInput,
    ResolveContactGates,
    IntegrateOldHorizontalVelocity,
    UpdateHorizontalVelocity,
    ProbeSideDescriptors,
    ApplyConfirmedCorrection,
    UpdateVerticalVelocity,
    IntegrateNewVerticalVelocity,
    TransitionVerticalApex,
    CapturePostState
};

enum class VerticalFreeSpaceStatus {
    Applied,
    OrdinaryModeRequiresContactResolution
};

struct VerticalFreeSpaceResult {
    PlayerRawRecord preState;
    PlayerRawRecord postState;
    VerticalFreeSpaceStatus status;
    bool crossedApex;
};

// Exact trace-closed portion of the original vertical callback. The caller
// must already have established that the vertical probes are clear. Mode 0
// deliberately returns without mutation because grounded/jump initiation is
// inseparable from the unresolved contact path.
VerticalFreeSpaceResult updatePlayerVerticalFreeSpace(
    PlayerRecord &player,
    PlayerTraceSink *trace = 0);

class PlayerTraceSink {
public:
    virtual ~PlayerTraceSink() {}

    virtual void onStage(PlayerUpdateStage stage) = 0;
    virtual void onStateWrite(std::uint16_t offset,
                              std::uint8_t width,
                              std::uint32_t before,
                              std::uint32_t after) = 0;
    virtual void onCollisionProbe(const CollisionProbe &probe,
                                  bool occupied) = 0;
    virtual void onPreState(const PlayerRawRecord &state) { (void)state; }
    virtual void onInputFlags(std::uint16_t flags) { (void)flags; }
    virtual void onPostState(const PlayerRawRecord &state) { (void)state; }
};

struct PlayerUpdateTrace : public PlayerTraceSink {
    std::vector<PlayerUpdateStage> stages;
    std::uint16_t inputFlags;
    PlayerRawRecord preState;
    PlayerRawRecord postState;
    std::vector<TraceStateWrite> stateWrites;
    std::vector<CollisionProbe> collisionProbes;

    PlayerUpdateTrace();

    void onStage(PlayerUpdateStage stage) override;
    void onStateWrite(std::uint16_t offset,
                      std::uint8_t width,
                      std::uint32_t before,
                      std::uint32_t after) override;
    void onCollisionProbe(const CollisionProbe &probe,
                          bool occupied) override;
    void onPreState(const PlayerRawRecord &state) override;
    void onInputFlags(std::uint16_t flags) override;
    void onPostState(const PlayerRawRecord &state) override;
};

struct PlayerUpdateResult {
    PlayerRawRecord preState;
    PlayerRawRecord postState;
    std::uint16_t inputFlags;
    CollisionDecision sideDecision;
    bool appliedYCorrection;
};

// Horizontal-only callback implementation. It does not apply gravity,
// jump, floor/ceiling response, grounded state, or vertical descriptor
// response; those remain behind the pending vertical research interface.
PlayerUpdateResult updatePlayerHorizontal(
    PlayerRecord &player,
    const InputState &input,
    const WorldCollisionView &world,
    PlayerTraceSink *trace = 0);

class PlayerUpdateCallback {
public:
    virtual ~PlayerUpdateCallback() {}

    virtual void updatePlayer(PlayerRecord &player,
                              const InputState &input,
                              const WorldCollisionView &world,
                              PlayerUpdateTrace *trace) = 0;
};

class ExperimentalHorizontalPlayerUpdate : public PlayerUpdateCallback {
public:
    void updatePlayer(PlayerRecord &player,
                      const InputState &input,
                      const WorldCollisionView &world,
                      PlayerUpdateTrace *trace) override;
};

} // namespace quiky

#endif
