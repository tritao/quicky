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
    CapturePostState,
    CallbackEntry,
    InputNormalization,
    ActionCounterUpdate,
    VerticalContactGate,
    HorizontalAccumulatorUpdate,
    OrdinaryMode,
    PositiveMode,
    NegativeMode,
    GroundedContactResponse,
    CommonCallbackTail,
    TransitionBranch,
    UnresolvedBoundary
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

struct PlayerGlobalWrite {
    std::uint16_t address;
    std::uint8_t width;
    std::uint32_t before;
    std::uint32_t after;

    PlayerGlobalWrite(std::uint16_t addressValue = 0,
                      std::uint8_t widthValue = 0,
                      std::uint32_t beforeValue = 0,
                      std::uint32_t afterValue = 0)
        : address(addressValue),
          width(widthValue),
          before(beforeValue),
          after(afterValue) {}
};

struct PlayerEffectDispatch {
    std::uint32_t address;
    std::uint16_t code;

    PlayerEffectDispatch(std::uint32_t addressValue = 0,
                         std::uint16_t codeValue = 0)
        : address(addressValue), code(codeValue) {}
};

// Typed projection of the callback globals used by the recovered 3FF8 path.
// Addresses are retained because these values are part of the trace contract;
// they are not presented as a modern engine singleton.
struct PlayerCallbackGlobals {
    std::int32_t deferredY8812;
    std::int32_t externalXDelta8816;
    std::uint16_t timerClear8810;
    std::uint16_t inputRunCounter4FEC;
    std::int32_t horizontalAccumulator4FE2;
    std::int16_t viewStateA4FE4;
    std::int16_t viewStateB4FE6;
    std::int32_t horizontalAccel4FE8;
    std::uint16_t idleCounter4FEE;
    std::uint8_t actionLowCopy4FF0;
    std::uint16_t pendingEvent612E;
    std::uint16_t cameraX81C0;
    std::uint16_t cameraY81C4;
    std::uint16_t cameraYLimit81CC;
    std::uint16_t actionSource656C;
    std::int16_t activationState85DA;
    std::uint16_t specialSpeedCapMode88B6;
    std::int16_t actionSuppressor89E6;
    std::int16_t collisionTransitionMode89EA;
    std::int16_t transitionState89EC;
    // 01F7:20AF publication pair. These are camera-facing globals, but the
    // transition branch writes the zero pair before it can feed the next
    // frame's camera/level boundary.
    std::int32_t publishedViewDeltaX60DC;
    std::int32_t publishedViewDeltaY60E0;

    // Address-qualified auxiliary state consumed by 01F7:5937. The
    // 881C/881E and 4FF2/4FF4 pairs are retained as a low/high word split at
    // the replay boundary; 5937 compares their reconstructed dwords.
    std::uint16_t dispatchWord60D8;
    std::uint16_t dispatchPreviousWord60DA;
    std::uint16_t dispatchScoreLow881C;
    std::uint16_t dispatchScoreHigh881E;
    std::uint16_t dispatchLives880A;
    std::uint16_t dispatchAmmo880C;
    std::uint16_t dispatchDisplayCount8822;
    std::uint32_t dispatchPublishedScore4FF2;
    std::uint16_t dispatchPublishedAmmo4FF6;
    std::uint16_t dispatchPublishedCount4FF8;
    std::uint16_t dispatchPublishedLives4FFA;

    PlayerCallbackGlobals();
};

// Public free-space leaf recovered from 4323/41E8. Mode 0 deliberately
// remains a leaf no-op: ordinary/contact orchestration belongs to the
// trace-closed 3FF8 callback below, where the descriptor table and callback
// globals are available. Keeping this distinction prevents callers from
// treating the leaf as a second grounded-policy implementation.
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
    virtual void onGlobalWrite(const PlayerGlobalWrite &write) {
        (void)write;
    }
    virtual void onEffectDispatch(const PlayerEffectDispatch &dispatch) {
        (void)dispatch;
    }
    virtual void onPreState(const PlayerRawRecord &state) { (void)state; }
    virtual void onInputFlags(std::uint16_t flags) { (void)flags; }
    virtual void onPostState(const PlayerRawRecord &state) { (void)state; }
};

struct PlayerUpdateTrace : public PlayerTraceSink {
    std::vector<PlayerUpdateStage> stages;
    std::uint16_t inputFlags;
    bool hasPreState;
    PlayerRawRecord preState;
    PlayerRawRecord postState;
    std::vector<TraceStateWrite> stateWrites;
    std::vector<CollisionProbe> collisionProbes;
    std::vector<bool> collisionOccupied;
    std::vector<PlayerGlobalWrite> globalWrites;
    std::vector<PlayerEffectDispatch> effectDispatches;

    PlayerUpdateTrace();

    void onStage(PlayerUpdateStage stage) override;
    void onStateWrite(std::uint16_t offset,
                      std::uint8_t width,
                      std::uint32_t before,
                      std::uint32_t after) override;
    void onCollisionProbe(const CollisionProbe &probe,
                          bool occupied) override;
    void onGlobalWrite(const PlayerGlobalWrite &write) override;
    void onEffectDispatch(const PlayerEffectDispatch &dispatch) override;
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

// Horizontal-only foundation API. TraceClosedPlayerUpdate composes this
// motion with the recovered callback ordering when a runtime descriptor table
// is present; an absent table remains an explicit unresolved boundary.
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

    // 01F7:A0B2 publishes these external carry globals before the player
    // callback consumes them. Other callback implementations may ignore the
    // contract until they model the recovered global boundary.
    virtual void publishPlatformCarry(std::int32_t xDelta8816,
                                      std::int32_t yDelta8812) {
        (void)xDelta8816;
        (void)yDelta8812;
    }

    // 01F7:3FF8 reads DS:89EA after phase-1 gameplay callbacks have run.
    // Session owners publish the recovered gate at that same callback
    // boundary; implementations that do not model the global may ignore it.
    virtual void publishTransitionGate(std::uint16_t transitionGate89ea) {
        (void)transitionGate89ea;
    }

    // The recovered 4416-44FE branch decrements DS:89EA in the player
    // callback. Session owners must read it back after the callback or the
    // next frame would republish the stale pre-callback gate.
    virtual bool hasTransitionState() const { return false; }
    virtual std::uint16_t transitionGate89EA() const { return 0; }
    virtual std::uint16_t transitionState89EC() const { return 0; }

    // 01F7:5937 consumes the persistent score/lives/ammo/health words from
    // the level callback boundary. The boolean is true only at level reset,
    // where the display-side publication words are rebuilt by the original
    // setup path.
    virtual void publishGameplayCounters(std::uint32_t score881c,
                                          std::uint16_t lives880a,
                                          std::uint16_t ammo880c,
                                          std::uint16_t health8822,
                                          bool resetPublications) {
        (void)score881c;
        (void)lives880a;
        (void)ammo880c;
        (void)health8822;
        (void)resetPublications;
    }

    // 01F7:9C0C reads the low byte of DS:85DA to decide whether its camera
    // and animation prefix runs. This accessor exposes that already-typed
    // callback global without assigning a broader transition meaning to it.
    virtual std::int16_t activationState85DA() const { return 0; }
};

class TraceClosedPlayerUpdate : public PlayerUpdateCallback {
public:
    TraceClosedPlayerUpdate();

    PlayerCallbackGlobals &globalsForSetup() { return _globals; }
    const PlayerCallbackGlobals &globals() const { return _globals; }

    void updatePlayer(PlayerRecord &player,
                      const InputState &input,
                      const WorldCollisionView &world,
                      PlayerUpdateTrace *trace) override;
    void publishPlatformCarry(std::int32_t xDelta8816,
                              std::int32_t yDelta8812) override;
    void publishTransitionGate(std::uint16_t transitionGate89ea) override;
    bool hasTransitionState() const override { return true; }
    std::uint16_t transitionGate89EA() const override {
        return static_cast<std::uint16_t>(_globals.collisionTransitionMode89EA);
    }
    std::uint16_t transitionState89EC() const override {
        return static_cast<std::uint16_t>(_globals.transitionState89EC);
    }
    void publishGameplayCounters(std::uint32_t score881c,
                                  std::uint16_t lives880a,
                                  std::uint16_t ammo880c,
                                  std::uint16_t health8822,
                                  bool resetPublications) override;
    std::int16_t activationState85DA() const override;

private:
    PlayerCallbackGlobals _globals;
};

} // namespace quiky

#endif
