#include "quiky/player_update.h"

#include "quiky/fixed.h"

namespace quiky {
namespace {

void reportGlobalWrite(PlayerTraceSink *trace,
                       std::uint16_t address,
                       std::uint8_t width,
                       std::uint32_t before,
                       std::uint32_t after) {
    // The DOS trace records mutations, not stores whose value is unchanged.
    // Keeping no-op stores out of the publication is important for comparing
    // the callback's observable global state rather than its instruction
    // count.
    if (trace != 0 && before != after) {
        trace->onGlobalWrite(PlayerGlobalWrite(address, width, before, after));
    }
}

void writeGlobal8(std::uint16_t address,
                  std::uint8_t &field,
                  std::uint8_t value,
                  PlayerTraceSink *trace) {
    const std::uint8_t before = field;
    field = value;
    reportGlobalWrite(trace, address, 1, before, field);
}

void writeGlobal16(std::uint16_t address,
                   std::uint16_t &field,
                   std::uint16_t value,
                   PlayerTraceSink *trace) {
    const std::uint16_t before = field;
    field = value;
    reportGlobalWrite(trace, address, 2, before, field);
}

void writeGlobal32(std::uint16_t address,
                   std::int32_t &field,
                   std::int32_t value,
                   PlayerTraceSink *trace) {
    const std::int32_t before = field;
    field = value;
    reportGlobalWrite(trace, address, 4,
                      static_cast<std::uint32_t>(before),
                      static_cast<std::uint32_t>(field));
}

void writeGlobalU32(std::uint16_t address,
                    std::uint32_t &field,
                    std::uint32_t value,
                    PlayerTraceSink *trace) {
    const std::uint32_t before = field;
    field = value;
    reportGlobalWrite(trace, address, 4, before, field);
}

void writeGlobal16Signed(std::uint16_t address,
                         std::int16_t &field,
                         std::int16_t value,
                         PlayerTraceSink *trace) {
    const std::uint16_t before = static_cast<std::uint16_t>(field);
    field = value;
    reportGlobalWrite(trace, address, 2, before,
                      static_cast<std::uint16_t>(field));
}

void captureWrites(const PlayerRawRecord &before,
                   const PlayerRawRecord &after,
                   PlayerTraceSink *trace) {
    if (trace == 0) {
        return;
    }
    for (std::size_t offset = 0; offset < PlayerRawRecord::kSize; ++offset) {
        if (before.bytes[offset] == after.bytes[offset]) {
            continue;
        }
        trace->onStateWrite(offset, 1, before.bytes[offset],
                            after.bytes[offset]);
    }
}

void captureProbes(const CollisionDecision &decision, PlayerTraceSink *trace) {
    if (trace == 0) {
        return;
    }
    for (std::size_t index = 0; index < decision.probes.size(); ++index) {
        trace->onCollisionProbe(decision.probes[index],
                                CollisionKernel::occupied(decision.probes[index]));
    }
}

void captureProbe(const CollisionProbe &probe,
                  bool occupied,
                  PlayerTraceSink *trace) {
    if (trace != 0) {
        trace->onCollisionProbe(probe, occupied);
    }
}

struct AnimationSequenceWords {
    std::uint16_t address;
    const std::int16_t *words;
    std::size_t count;
};

// Static Ghidra data export from QUIKY_SEG06, where DS points at runtime in
// the player callback.  These are selected by the 3AB9 speed/turn paths.
// 01F7:5D60 walks the stream by cursor, not by the loader's descriptor
// length.  The 3142 stream therefore includes its two trailing words at
// 3152/3154: the first is the next frame and the second is its signed loop
// displacement back into the stream.  A natural held-jump trace reaches
// 3152 from a cursor initially at 3150.
const std::int16_t kAnimation3142[] = {
    4, 0, 1, 2, 3, 4, 5, 6, 7, -8
};
const std::int16_t kAnimation3156[] = {4, 0, 0, 0, -3};
const std::int16_t kAnimation3160[] = {8, 10, 11, 12, -1};
const std::int16_t kAnimation316a[] = {
    14, 0, 16, 17, 18, 18, 19, 19, 19, 18, 17, 16, 0, -1};
const std::int16_t kAnimation3186[] = {20, 13, 14, 15, -1};
const std::int16_t kAnimation3190[] = {2, 0, 1, 2, 3, 4, 5, 6};
const std::int16_t kAnimation31a4[] = {
    14, 20, 21, 22, 23, 24, 25, 26, 27, 28, -3};
const std::int16_t kAnimation31ba[] = {
    15, 30, 31, 32, 33, 33, 33, 33, 34, 34, 34, 35, 36, 37};

const AnimationSequenceWords kAnimationSequences[] = {
    {0x3142, kAnimation3142,
     sizeof(kAnimation3142) / sizeof(kAnimation3142[0])},
    {0x3156, kAnimation3156,
     sizeof(kAnimation3156) / sizeof(kAnimation3156[0])},
    {0x3160, kAnimation3160,
     sizeof(kAnimation3160) / sizeof(kAnimation3160[0])},
    {0x316a, kAnimation316a,
     sizeof(kAnimation316a) / sizeof(kAnimation316a[0])},
    {0x3186, kAnimation3186,
     sizeof(kAnimation3186) / sizeof(kAnimation3186[0])},
    {0x3190, kAnimation3190,
     sizeof(kAnimation3190) / sizeof(kAnimation3190[0])},
    {0x31a4, kAnimation31a4,
     sizeof(kAnimation31a4) / sizeof(kAnimation31a4[0])},
    {0x31ba, kAnimation31ba,
     sizeof(kAnimation31ba) / sizeof(kAnimation31ba[0])},
};

bool animationWord(std::uint16_t address, std::int16_t &value) {
    for (std::size_t sequenceIndex = 0;
         sequenceIndex < sizeof(kAnimationSequences) /
                              sizeof(kAnimationSequences[0]);
         ++sequenceIndex) {
        const AnimationSequenceWords &sequence =
            kAnimationSequences[sequenceIndex];
        const std::uint16_t end = static_cast<std::uint16_t>(
            sequence.address + sequence.count * 2);
        if (address < sequence.address || address >= end ||
            ((address - sequence.address) & 1) != 0) {
            continue;
        }
        value = sequence.words[(address - sequence.address) / 2];
        return true;
    }
    return false;
}

bool loadAnimationDescriptor(PlayerRecord &player,
                              std::uint16_t sequenceAddress,
                              PlayerTraceSink *trace) {
    std::int16_t delay = 0;
    std::int16_t frame = 0;
    if (!animationWord(sequenceAddress, delay) ||
        !animationWord(static_cast<std::uint16_t>(sequenceAddress + 2),
                       frame)) {
        if (trace != 0) {
            trace->onStage(PlayerUpdateStage::UnresolvedBoundary);
        }
        return false;
    }

    player.field1E = static_cast<std::uint16_t>(delay);
    player.animationDelay20 = static_cast<std::uint16_t>(delay);
    player.animationCursor22 = static_cast<std::uint16_t>(sequenceAddress + 2);
    player.field24 = static_cast<std::uint16_t>(sequenceAddress + 2);
    if (player.directionByte28 == 0xff) {
        frame = static_cast<std::int16_t>(frame + 0x32);
    }
    player.statusWord12 = static_cast<std::uint16_t>(frame);
    return true;
}

void stage(PlayerTraceSink *trace, PlayerUpdateStage value) {
    if (trace != 0) {
        trace->onStage(value);
    }
}

} // namespace

PlayerCallbackGlobals::PlayerCallbackGlobals()
    : deferredY8812(0),
      externalXDelta8816(0),
      timerClear8810(0),
      inputRunCounter4FEC(0),
      horizontalAccumulator4FE2(0),
      viewStateA4FE4(0),
      viewStateB4FE6(0),
      horizontalAccel4FE8(0),
      idleCounter4FEE(0x00d3),
      actionLowCopy4FF0(0),
      pendingEvent612E(0),
      cameraX81C0(0),
      cameraY81C4(0),
      cameraYLimit81CC(0x7fff),
      actionSource656C(0),
      activationState85DA(0),
      specialSpeedCapMode88B6(0),
      actionSuppressor89E6(0),
      collisionTransitionMode89EA(0),
      transitionState89EC(0),
      publishedViewDeltaX60DC(0),
      publishedViewDeltaY60E0(0),
      dispatchWord60D8(0),
      dispatchPreviousWord60DA(0),
      dispatchScoreLow881C(0),
      dispatchScoreHigh881E(0),
      dispatchLives880A(0),
      dispatchAmmo880C(0),
      dispatchDisplayCount8822(0),
      dispatchPublishedScore4FF2(0),
      dispatchPublishedAmmo4FF6(0),
      dispatchPublishedCount4FF8(0),
      dispatchPublishedLives4FFA(0) {
}

PlayerUpdateTrace::PlayerUpdateTrace()
    : stages(), inputFlags(0), hasPreState(false), preState(), postState(),
      stateWrites(), collisionProbes(), collisionOccupied(), globalWrites(),
      effectDispatches() {
}

void PlayerUpdateTrace::onStage(PlayerUpdateStage stageValue) {
    stages.push_back(stageValue);
}

void PlayerUpdateTrace::onStateWrite(std::uint16_t offset,
                                     std::uint8_t width,
                                     std::uint32_t before,
                                     std::uint32_t after) {
    stateWrites.push_back(TraceStateWrite{offset, width, after});
    (void)before;
}

void PlayerUpdateTrace::onCollisionProbe(const CollisionProbe &probe,
                                         bool occupiedValue) {
    collisionProbes.push_back(probe);
    collisionOccupied.push_back(occupiedValue);
}

void PlayerUpdateTrace::onGlobalWrite(const PlayerGlobalWrite &write) {
    globalWrites.push_back(write);
}

void PlayerUpdateTrace::onEffectDispatch(
    const PlayerEffectDispatch &dispatch) {
    effectDispatches.push_back(dispatch);
}

void PlayerUpdateTrace::onPreState(const PlayerRawRecord &state) {
    if (!hasPreState) {
        preState = state;
        hasPreState = true;
    }
}

void PlayerUpdateTrace::onInputFlags(std::uint16_t flags) {
    inputFlags = flags;
}

void PlayerUpdateTrace::onPostState(const PlayerRawRecord &state) {
    postState = state;
}

PlayerUpdateResult updatePlayerHorizontal(PlayerRecord &player,
                                           const InputState &input,
                                           const WorldCollisionView &world,
                                           PlayerTraceSink *trace) {
    stage(trace, PlayerUpdateStage::CapturePreState);
    player.syncToRaw();
    const PlayerRawRecord preState = player.toRaw();
    if (trace != 0) {
        trace->onPreState(preState);
    }

    stage(trace, PlayerUpdateStage::SampleInput);
    const std::uint16_t inputFlags = input.actionFlags();
    if (trace != 0) {
        trace->onInputFlags(inputFlags);
    }
    player.actionWord = inputFlags;

    stage(trace, PlayerUpdateStage::ResolveContactGates);

    stage(trace, PlayerUpdateStage::IntegrateOldHorizontalVelocity);
    const std::int32_t oldVelocity = player.velocityX.raw;
    player.positionX.raw = Fixed16::wrapAddRaw(player.positionX.raw,
                                               oldVelocity);

    stage(trace, PlayerUpdateStage::UpdateHorizontalVelocity);
    const std::int32_t acceleration = player.acceleration4C.raw;
    const std::int32_t friction = player.friction54.raw;
    const std::int32_t speedCap = player.horizontalSpeedCap5C.raw;
    const std::int32_t negativeCap = Fixed16::wrapNegRaw(speedCap);
    const std::int32_t negativeFriction = Fixed16::wrapNegRaw(friction);

    // 3AB9 tests right first, then left. This is the recovered precedence for
    // simultaneous horizontal bits.
    if ((inputFlags & 0x0004) != 0) {
        player.velocityX.raw = Fixed16::clampRaw(
            Fixed16::wrapAddRaw(player.velocityX.raw, acceleration),
            negativeCap, speedCap);
        player.directionByte28 = 1;
    } else if ((inputFlags & 0x0008) != 0) {
        player.velocityX.raw = Fixed16::clampRaw(
            Fixed16::wrapSubRaw(player.velocityX.raw, acceleration),
            negativeCap, speedCap);
        player.directionByte28 = 0xff;
    } else if (player.velocityX.raw > 0) {
        player.velocityX.raw =
            player.velocityX.raw > friction
                ? Fixed16::wrapSubRaw(player.velocityX.raw, friction)
                : 0;
    } else if (player.velocityX.raw < 0) {
        player.velocityX.raw =
            player.velocityX.raw < negativeFriction
                ? Fixed16::wrapAddRaw(player.velocityX.raw, friction)
                : 0;
    }

    // The callback derives the motion-facing byte from the velocity captured
    // before integration. A stationary/one-step velocity keeps the requested
    // direction; otherwise the old velocity supplies the facing sign.
    if (oldVelocity == 0 || oldVelocity == 1) {
        player.motionDirectionByte29 = player.directionByte28;
    } else {
        player.motionDirectionByte29 = oldVelocity < 0 ? 0xff : 1;
    }

    player.syncToRaw();

    stage(trace, PlayerUpdateStage::ProbeSideDescriptors);
    const CollisionDecision sideDecision = CollisionKernel::sideProbePair(
        world, player.xPixel(), player.yPixel());
    captureProbes(sideDecision, trace);

    stage(trace, PlayerUpdateStage::ApplyConfirmedCorrection);
    CollisionDecision appliedDecision = sideDecision;
    bool appliedYCorrection = false;
    if (player.gate38 == 0) {
        appliedDecision = CollisionKernel::snapYOnSideContact(
            world, player.xPixel(), player.yPixel(), player.sideResponse3B,
            player.verticalResponse3A);
        if (appliedDecision.ySnapped) {
            player.setYPixel(appliedDecision.correctedY);
            appliedYCorrection = true;
            captureProbes(appliedDecision, trace);
        }
    }

    player.syncToRaw();
    const PlayerRawRecord postState = player.toRaw();
    captureWrites(preState, postState, trace);
    stage(trace, PlayerUpdateStage::CapturePostState);
    if (trace != 0) {
        trace->onPostState(postState);
    }

    PlayerUpdateResult result;
    result.preState = preState;
    result.postState = postState;
    result.inputFlags = inputFlags;
    result.sideDecision = appliedDecision;
    result.appliedYCorrection = appliedYCorrection;
    return result;
}

VerticalFreeSpaceResult updatePlayerVerticalFreeSpace(
    PlayerRecord &player,
    PlayerTraceSink *trace) {
    player.syncToRaw();
    const PlayerRawRecord preState = player.toRaw();
    if (trace != 0) {
        trace->onPreState(preState);
        trace->onInputFlags(player.actionWord);
    }

    VerticalFreeSpaceResult result;
    result.preState = preState;
    result.status = VerticalFreeSpaceStatus::Applied;
    result.crossedApex = false;

    if (player.mode37 == 0) {
        result.status =
            VerticalFreeSpaceStatus::OrdinaryModeRequiresContactResolution;
    } else if (player.mode37 < 0) {
        stage(trace, PlayerUpdateStage::UpdateVerticalVelocity);
        std::int32_t nextVelocity = Fixed16::wrapAddRaw(
            player.velocityY.raw, player.negativeYAcceleration58.raw);
        const std::int32_t releaseFloor = -0x20000;
        if (player.contactScratch2B == 0 &&
            (player.actionWord & 0x0022) == 0 &&
            nextVelocity < releaseFloor) {
            nextVelocity = releaseFloor;
        }

        if (nextVelocity >= 0) {
            // The original contact-response target establishes the falling
            // state at the apex. Controlled traces show no Y movement here.
            stage(trace, PlayerUpdateStage::TransitionVerticalApex);
            player.velocityY.raw = 0;
            player.mode37 = 1;
            result.crossedApex = true;
        } else {
            player.velocityY.raw = nextVelocity;
            stage(trace, PlayerUpdateStage::IntegrateNewVerticalVelocity);
            player.positionY.raw = Fixed16::wrapAddRaw(
                player.positionY.raw, player.velocityY.raw);
        }
    } else {
        stage(trace, PlayerUpdateStage::UpdateVerticalVelocity);
        player.velocityY.raw = Fixed16::wrapAddRaw(
            player.velocityY.raw, player.positiveYAcceleration50.raw);
        if (player.velocityY.raw > player.positiveYSpeedCap60.raw) {
            player.velocityY.raw = player.positiveYSpeedCap60.raw;
        }
        stage(trace, PlayerUpdateStage::IntegrateNewVerticalVelocity);
        player.positionY.raw = Fixed16::wrapAddRaw(
            player.positionY.raw, player.velocityY.raw);
    }

    player.syncToRaw();
    result.postState = player.toRaw();
    captureWrites(preState, result.postState, trace);
    if (trace != 0) {
        trace->onPostState(result.postState);
    }
    return result;
}

namespace {

std::int16_t addPixel(std::int16_t value, std::int32_t delta) {
    return static_cast<std::int16_t>(static_cast<std::uint16_t>(value) +
                                     static_cast<std::uint16_t>(delta));
}

bool probeMapMask(const WorldCollisionView &world,
                  std::int16_t x,
                  std::int16_t y,
                  std::uint16_t mask,
                  PlayerTraceSink *trace) {
    const CollisionProbe probe = CollisionKernel::probeAt(world, x, y);
    const bool hit = (probe.mapWord & mask) != 0;
    captureProbe(probe, hit, trace);
    return hit;
}

bool probeVerticalStep(const PlayerRecord &player,
                       const WorldCollisionView &world,
                       PlayerTraceSink *trace,
                       std::int32_t extra = 0) {
    const std::int16_t y = addPixel(
        player.yPixel(),
        -static_cast<std::int32_t>(player.verticalStepPixels72) + extra);
    return probeMapMask(world, player.xPixel(), y, 0x1000, trace);
}

bool probeVerticalTenPixels(const PlayerRecord &player,
                            const WorldCollisionView &world,
                            PlayerTraceSink *trace) {
    const std::int16_t y = addPixel(
        player.yPixel(),
        -10 - static_cast<std::int32_t>(player.verticalStepPixels72));
    return probeMapMask(world, player.xPixel(), y, 0x1000, trace);
}

bool probeForwardSurface(const PlayerRecord &player,
                         const WorldCollisionView &world,
                         PlayerTraceSink *trace) {
    if (player.verticalResponse3A != 0) {
        return false;
    }

    const std::int32_t dx =
        static_cast<std::int8_t>(player.motionDirectionByte29) < 0 ? -10 : 10;
    const std::int16_t x = addPixel(player.xPixel(), dx);
    const std::int16_t y = player.yPixel();
    const CollisionProbe first = CollisionKernel::probeAt(
        world, x, addPixel(y, -1));
    captureProbe(first, CollisionKernel::occupied(first), trace);
    if (CollisionKernel::occupied(first)) {
        return true;
    }

    const CollisionProbe second = CollisionKernel::probeAt(
        world, x, addPixel(y, -0x11));
    captureProbe(second, CollisionKernel::occupied(second), trace);
    if (CollisionKernel::occupied(second)) {
        return true;
    }

    if (player.verticalStepPixels72 <= 0x20) {
        return false;
    }
    const CollisionProbe third = CollisionKernel::probeAt(
        world, x, addPixel(y, -0x21));
    captureProbe(third, CollisionKernel::occupied(third), trace);
    return CollisionKernel::occupied(third);
}

bool sideProbeClear(PlayerRecord &player,
                    const WorldCollisionView &world,
                    PlayerTraceSink *trace) {
    // Static 01F7:3A1F exits before either 5C27 probe while +0x38 is set.
    // The ordinary branch still evaluates its gate and may enter 3DF2, so
    // preserve that caller-visible decision without inventing side probes.
    if (player.gate38 != 0) {
        return true;
    }
    const CollisionDecision decision = CollisionKernel::sideProbePair(
        world, player.xPixel(), player.yPixel());
    captureProbes(decision, trace);
    if (!decision.occupied && player.gate38 == 0 && player.mode37 != -1) {
        player.sideResponse3B = 0xff;
    }
    return !decision.occupied;
}

bool positiveModeSideDescriptorsClear(
    const PlayerRecord &player,
    const WorldCollisionView &world,
    PlayerTraceSink *trace) {
    // Static 01F7:41F7/4209: positive mode performs the ordered x-5/x+5
    // quadrant tests directly.  Unlike 3A1F, this preliminary pair does not
    // publish +0x3B=FF; the later 01F7:4278 call is the latch-producing side
    // test after vertical integration.  Keeping this separate is observable
    // when 3D02 follows a clear positive-mode pair.
    const CollisionDecision decision = CollisionKernel::sideProbePair(
        world, player.xPixel(), player.yPixel());
    captureProbes(decision, trace);
    return !decision.occupied;
}

void applyDescriptorCorrection(PlayerRecord &player,
                               const WorldCollisionView &world,
                               PlayerTraceSink *trace) {
    const DescriptorResponseDecision decision =
        CollisionKernel::resolveDescriptorResponse(
            world,
            DescriptorResponseInput(player.xPixel(), player.yPixel(),
                                    player.velocityY.raw, player.mode37,
                                    player.sideResponse3B,
                                    player.velocityX.raw));
    for (std::size_t index = 0; index < decision.probes.size(); ++index) {
        captureProbe(decision.probes[index],
                     CollisionKernel::occupied(decision.probes[index]), trace);
    }
    player.verticalResponse3A = decision.finalVerticalResponse;
    player.velocityY.raw = decision.finalVelocityY;
    player.setYPixel(decision.finalY);
}

void advanceAnimationDescriptor(PlayerRecord &player,
                                PlayerTraceSink *trace) {
    // Static 01F7:5D60: the first path is fully closed.  When +0x20 is
    // nonzero, the helper decrements it and returns before touching the
    // resident animation table.  The zero path remains an explicit table
    // boundary; following it here would require inventing table contents.
    if (player.animationDelay20 != 0) {
        player.animationDelay20 = static_cast<std::uint16_t>(
            player.animationDelay20 - 1);
        return;
    }

    std::uint16_t cursor = static_cast<std::uint16_t>(player.field24 + 2);
    std::int16_t frame = 0;
    if (!animationWord(cursor, frame)) {
        if (trace != 0) {
            trace->onStage(PlayerUpdateStage::UnresolvedBoundary);
        }
        return;
    }
    while (frame < 0) {
        cursor = static_cast<std::uint16_t>(
            cursor + static_cast<std::int32_t>(frame) * 2);
        if (!animationWord(cursor, frame)) {
            if (trace != 0) {
                trace->onStage(PlayerUpdateStage::UnresolvedBoundary);
            }
            return;
        }
    }
    player.field24 = cursor;
    if (player.directionByte28 == 0xff) {
        frame = static_cast<std::int16_t>(frame + 0x32);
    }
    player.statusWord12 = static_cast<std::uint16_t>(frame);
    player.animationDelay20 = player.field1E;
}

void snapPlayerY(PlayerRecord &player,
                 const WorldCollisionView &world,
                 PlayerTraceSink *trace) {
    const CollisionDecision decision = CollisionKernel::snapYOnSideContact(
        world, player.xPixel(), player.yPixel(), player.sideResponse3B,
        player.verticalResponse3A);
    captureProbes(decision, trace);
    if (decision.ySnapped) {
        player.setYPixel(decision.correctedY);
    }
}

void updateHorizontalAccumulator(PlayerRecord &player,
                                 std::uint16_t &action,
                                 PlayerCallbackGlobals &globals,
                                 PlayerTraceSink *trace) {
    if ((action & 1) != 0 && player.mode37 == 0) {
        action = static_cast<std::uint16_t>(action & ~0x000cU);
        player.verticalStepOrDirection2E = static_cast<std::int16_t>(
            -(player.verticalStepPixels72 >> 1));
        writeGlobal16(0x4fec, globals.inputRunCounter4FEC,
                      static_cast<std::uint16_t>(globals.inputRunCounter4FEC + 1),
                      trace);
        if (globals.inputRunCounter4FEC >= 0x3c) {
            writeGlobal16(0x4fec, globals.inputRunCounter4FEC,
                          static_cast<std::uint16_t>(globals.inputRunCounter4FEC - 1),
                          trace);
            writeGlobal32(0x4fe8, globals.horizontalAccel4FE8,
                          Fixed16::wrapAddRaw(globals.horizontalAccel4FE8,
                                              0x1000), trace);
            if (globals.horizontalAccel4FE8 > 0x18000) {
                writeGlobal32(0x4fe8, globals.horizontalAccel4FE8, 0x18000,
                              trace);
            }
            if (globals.horizontalAccumulator4FE2 > 0x200000) {
                writeGlobal32(0x4fe8, globals.horizontalAccel4FE8,
                              Fixed16::wrapSubRaw(
                                  globals.horizontalAccel4FE8, 0x2000), trace);
                if (globals.horizontalAccel4FE8 < 0) {
                    writeGlobal32(0x4fe8, globals.horizontalAccel4FE8, 0,
                                  trace);
                }
            }
            writeGlobal32(0x4fe2, globals.horizontalAccumulator4FE2,
                          Fixed16::wrapAddRaw(
                              globals.horizontalAccumulator4FE2,
                              globals.horizontalAccel4FE8), trace);
        }
    } else {
        writeGlobal16(0x4fec, globals.inputRunCounter4FEC, 0, trace);
        action = static_cast<std::uint16_t>(action & ~1U);
        player.verticalStepOrDirection2E = static_cast<std::int16_t>(
            -static_cast<std::int32_t>(player.verticalStepPixels72));
        if (player.actionCounter2A != 0) {
            if (globals.horizontalAccel4FE8 != 0) {
                writeGlobal32(0x4fe8, globals.horizontalAccel4FE8,
                              Fixed16::wrapAddRaw(
                                  globals.horizontalAccel4FE8, 0x1000), trace);
                if (globals.horizontalAccel4FE8 > 0x18000) {
                    writeGlobal32(0x4fe8, globals.horizontalAccel4FE8, 0x18000,
                                  trace);
                }
                if (globals.horizontalAccumulator4FE2 > 0x200000) {
                    writeGlobal32(0x4fe8, globals.horizontalAccel4FE8,
                                  Fixed16::wrapSubRaw(
                                      globals.horizontalAccel4FE8, 0x2000), trace);
                    if (globals.horizontalAccel4FE8 < 0) {
                        writeGlobal32(0x4fe8, globals.horizontalAccel4FE8, 0,
                                      trace);
                    }
                }
                writeGlobal32(0x4fe2, globals.horizontalAccumulator4FE2,
                              Fixed16::wrapAddRaw(
                                  globals.horizontalAccumulator4FE2,
                                  globals.horizontalAccel4FE8), trace);
            } else {
                player.actionCounter2A = 0;
            }
        }
    }

    if (globals.horizontalAccumulator4FE2 > 0) {
        writeGlobal32(0x4fe8, globals.horizontalAccel4FE8,
                      Fixed16::wrapSubRaw(globals.horizontalAccel4FE8, 0x1000),
                      trace);
        if (globals.horizontalAccel4FE8 < -0x18000) {
            writeGlobal32(0x4fe8, globals.horizontalAccel4FE8, -0x18000,
                          trace);
        }
        if (globals.horizontalAccumulator4FE2 < 0x100000) {
            writeGlobal32(0x4fe8, globals.horizontalAccel4FE8,
                          Fixed16::wrapAddRaw(globals.horizontalAccel4FE8,
                                              0x2000), trace);
            if (globals.horizontalAccel4FE8 > 0) {
                writeGlobal32(0x4fe8, globals.horizontalAccel4FE8, 0, trace);
            }
        }
        writeGlobal32(0x4fe2, globals.horizontalAccumulator4FE2,
                      Fixed16::wrapAddRaw(globals.horizontalAccumulator4FE2,
                                          globals.horizontalAccel4FE8), trace);
    }
}

void integrateHorizontalMotion(PlayerRecord &player,
                               const WorldCollisionView &world,
                               PlayerCallbackGlobals &globals,
                               PlayerTraceSink *trace) {
    if ((player.actionWord & 0x0004) != 0) {
        player.directionByte28 = 1;
    } else if ((player.actionWord & 0x0008) != 0) {
        player.directionByte28 = 0xff;
    }

    const std::int32_t oldVelocity = player.velocityX.raw;
    player.positionX.raw = Fixed16::wrapAddRaw(player.positionX.raw,
                                               oldVelocity);
    // The original callback mutates the single ES:DI record.  Keep the raw
    // trace view synchronized before coordinate probes observe the updated X.
    player.syncToRaw();
    const std::int32_t motionSign = Fixed16::wrapAddRaw(
        oldVelocity, globals.externalXDelta8816);
    if (motionSign == 0 || motionSign == 1) {
        player.motionDirectionByte29 = player.directionByte28;
    } else {
        player.motionDirectionByte29 = motionSign < 0 ? 0xff : 1;
    }

    if (probeForwardSurface(player, world, trace)) {
        player.velocityX.raw = 0;
        if (player.animationState36 == 0 && player.mode37 == 0) {
            player.animationState36 = 1;
        }
        return;
    }

    player.positionX.raw = Fixed16::wrapAddRaw(
        player.positionX.raw, globals.externalXDelta8816);
    writeGlobal32(0x8816, globals.externalXDelta8816, 0, trace);

    if (player.verticalResponse3A < 0 &&
        static_cast<std::int8_t>(player.motionDirectionByte29) >= 0) {
        player.setYPixel(addPixel(player.yPixel(), 2));
    } else if (player.verticalResponse3A > 0 &&
               static_cast<std::int8_t>(player.motionDirectionByte29) < 0) {
        player.setYPixel(addPixel(player.yPixel(), 2));
    }

    // External carry and the two-pixel contact adjustment precede the
    // vertical probe below in the native record, so publish both fields first.
    player.syncToRaw();

    const std::int32_t cap = player.horizontalSpeedCap5C.raw;
    if ((player.actionWord & 0x0004) != 0) {
        const std::int32_t next = Fixed16::wrapAddRaw(
            player.velocityX.raw, player.acceleration4C.raw);
        player.velocityX.raw = next > cap ? cap : next;
        if (player.animationState36 != 0 && player.mode37 == 0) {
            loadAnimationDescriptor(player, 0x3142, trace);
            player.animationState36 = 0;
        }
    } else if ((player.actionWord & 0x0008) != 0) {
        const std::int32_t next = Fixed16::wrapSubRaw(
            player.velocityX.raw, player.acceleration4C.raw);
        player.velocityX.raw = next < Fixed16::wrapNegRaw(cap)
                                   ? Fixed16::wrapNegRaw(cap)
                                   : next;
        if (player.animationState36 != 0 && player.mode37 == 0) {
            loadAnimationDescriptor(player, 0x3142, trace);
            player.animationState36 = 0;
        }
    } else {
        // Static 01F7:3AB9: while the callback is ordinary and the idle
        // counter has not reached its long-idle boundary, reload sequence
        // 3156 before the common-tail 5D60 advance.  This is observable after
        // landing: native keeps +0x20 at 3 on each following no-input pass
        // instead of letting the countdown continue to 2, 1, ... .
        if (player.mode37 == 0 && globals.idleCounter4FEE < 0x00d2) {
            loadAnimationDescriptor(player, 0x3156, trace);
        }
        // 01F7:3AB9's idle-state publication belongs to ordinary mode. The
        // negative/jump path still traverses this common horizontal helper,
        // but DOS leaves +0x36 untouched there.
        if (player.mode37 == 0 && player.animationState36 == 0) {
            player.animationState36 = 1;
        }
        if (player.velocityX.raw > 0) {
            player.velocityX.raw = player.velocityX.raw > player.friction54.raw
                                       ? Fixed16::wrapSubRaw(
                                             player.velocityX.raw,
                                             player.friction54.raw)
                                       : 0;
        } else if (player.velocityX.raw < 0) {
            player.velocityX.raw =
                player.velocityX.raw < Fixed16::wrapNegRaw(player.friction54.raw)
                    ? Fixed16::wrapAddRaw(player.velocityX.raw,
                                          player.friction54.raw)
                    : 0;
        }
    }

    if (player.verticalResponse3A > 0 &&
        Fixed16::wrapSubRaw(player.positionY.raw, player.savedY44.raw) < 0 &&
        probeVerticalStep(player, world, trace)) {
        player.velocityX.raw = 0;
        player.positionY.raw = player.savedY44.raw;
        player.positionX.raw = player.savedX48.raw;
    }

    // +13 is the speed-animation state used by 3AB9. It is the high byte of
    // the confirmed status word, so preserve the low status byte verbatim.
    if (player.mode37 != 0) {
        player.statusWord12 = static_cast<std::uint16_t>(
            player.statusWord12 & 0x00ffU);
    } else {
        const std::int32_t speed = player.velocityX.raw < 0
                                       ? Fixed16::wrapNegRaw(player.velocityX.raw)
                                       : player.velocityX.raw;
        if (speed < 0x28000 && (player.statusWord12 & 0xff00U) != 0) {
            player.statusWord12 = static_cast<std::uint16_t>(
                player.statusWord12 & 0x00ffU);
            loadAnimationDescriptor(player, 0x3142, trace);
        }
        if (speed >= 0x28000 && (player.statusWord12 & 0xff00U) != 0xff00U) {
            player.statusWord12 = static_cast<std::uint16_t>(
                (player.statusWord12 & 0x00ffU) | 0xff00U);
            loadAnimationDescriptor(player, 0x3190, trace);
        }
    }
}

void contactResponse(PlayerRecord &player,
                     std::uint16_t timer,
                     bool setTimer,
                     PlayerTraceSink *trace) {
    // Static 01F7:41C1-41E5 and 01F7:4350-4368: the shared blocked/contact
    // response writes the mode, vertical velocity, and optional timer.
    (void)trace;
    if (setTimer) {
        player.resetDeathTimer3E = timer;
    }
    player.mode37 = 1;
    player.velocityY.raw = 0;
    loadAnimationDescriptor(player, 0x3186, trace);
}

void groundedContact(PlayerRecord &player,
                     const WorldCollisionView &world,
                     PlayerCallbackGlobals &globals,
                     PlayerTraceSink *trace) {
    // Static 01F7:427F-42B1: correction, side-contact snap, then grounded
    // mode/velocity/animation writes. Evidence: focused audit caller_order.
    if (player.gate38 == 0) {
        applyDescriptorCorrection(player, world, trace);
        snapPlayerY(player, world, trace);
    }
    player.velocityY.raw = 0;
    player.mode37 = 0;
    if (globals.idleCounter4FEE < 0x00d2) {
        loadAnimationDescriptor(player, 0x3156, trace);
    }
    player.animationState36 = 1;
}

void actionContactSideEffect(PlayerRecord &player) {
    if (player.mode37 == 0 && (player.actionWord & 0x0001) != 0) {
        player.actionCounter2A = 0xff;
        player.statusWord12 = static_cast<std::uint16_t>(
            static_cast<std::int8_t>(player.directionByte28) >= 0
                ? 0x0026
                : 0x0058);
    }
}

void updateAuxiliaryPlayerDispatch5937(PlayerCallbackGlobals &globals,
                                       PlayerTraceSink *trace) {
    // Static 01F7:5937-5BED. The first compare is byte-sized even though the
    // same DS:85DA word is consumed as a signed word by the input path.
    if (static_cast<std::uint8_t>(globals.activationState85DA) != 0) {
        return;
    }

    const std::uint16_t currentWord = globals.dispatchWord60D8;
    if (globals.dispatchPreviousWord60DA != currentWord) {
        const std::uint16_t changed = static_cast<std::uint16_t>(
            currentWord ^ globals.dispatchPreviousWord60DA);
        writeGlobal16(0x60da, globals.dispatchPreviousWord60DA,
                      currentWord, trace);

        // 5954-5A02 calls the external 386F/0442 resource dispatcher once
        // for each changed low-byte bit. Those calls are presentation-owned;
        // their selected far callback remains an explicit external contract
        // and is intentionally not synthesized here.
        (void)changed;
    }

    const std::uint32_t score =
        static_cast<std::uint32_t>(globals.dispatchScoreLow881C) |
        (static_cast<std::uint32_t>(globals.dispatchScoreHigh881E) << 16);
    if (globals.dispatchPublishedScore4FF2 != score) {
        writeGlobalU32(0x4ff2, globals.dispatchPublishedScore4FF2,
                       score, trace);
        // 5A03-5AF4 dispatches the six score digits through 386F. The
        // nested resource callbacks cannot feed player simulation in the
        // closed contract, so only the direct publication is represented.
    }

    // 5AF5-5B39. The source uses a signed comparison for the published and
    // current display counts, then stores exactly one 16-bit step.
    std::uint16_t publishedCount = globals.dispatchPublishedCount4FF8;
    const std::int16_t signedPublished =
        static_cast<std::int16_t>(publishedCount);
    const std::int16_t signedCurrent =
        static_cast<std::int16_t>(globals.dispatchDisplayCount8822);
    if (publishedCount != globals.dispatchDisplayCount8822) {
        if (signedPublished > signedCurrent) {
            writeGlobal16(0x4ff8, globals.dispatchPublishedCount4FF8,
                          static_cast<std::uint16_t>(publishedCount - 1),
                          trace);
        } else {
            publishedCount = static_cast<std::uint16_t>(publishedCount + 1);
            writeGlobal16(0x4ff8, globals.dispatchPublishedCount4FF8,
                          publishedCount, trace);
        }
    }

    // 5B3A-5B61. Negative lives are clamped only for the comparison and
    // stored publication; the visible digit uses the raw source word and is
    // capped at nine by the original unsigned compare.
    std::int16_t lives = static_cast<std::int16_t>(globals.dispatchLives880A);
    if (lives < 0) {
        lives = 0;
    }
    if (static_cast<std::uint16_t>(lives) !=
        globals.dispatchPublishedLives4FFA) {
        writeGlobal16(0x4ffa, globals.dispatchPublishedLives4FFA,
                      static_cast<std::uint16_t>(lives), trace);
    }

    // 5B96-5BEC compares ammo against 4FF6 and dispatches digits, but does
    // not write 4FF6. Keep the comparison state typed for future target
    // traces; no direct simulation state is changed by this body.
}

void commonCallbackTail(PlayerRecord &player,
                         const WorldCollisionView &world,
                        PlayerCallbackGlobals &globals,
                        PlayerTraceSink *trace) {
    // Static 01F7:4384-4415: common timer, horizontal, effect, and idle tail.
    if (player.mode37 == 0 && globals.specialSpeedCapMode88B6 == 1) {
        player.horizontalSpeedCap5C.raw = 0x30000;
    }

    if ((player.actionWord & 0x0010) == 0) {
        player.field3C = 0;
    } else if (player.field3C == 0) {
        player.field3C = 0xff;
        if (trace != 0) {
            trace->onStage(PlayerUpdateStage::UnresolvedBoundary);
        }
        // 38EC -> 4519 -> 0E06 is a known factory contract, but its child
        // family and lifetime are deliberately outside this implementation.
    }

    integrateHorizontalMotion(player, world, globals, trace);

    // Static 01F7:438F: 5D60 follows horizontal integration and precedes
    // 3A62.  Its nonzero-delay path is implemented above; its table reload
    // path remains address-qualified.
    advanceAnimationDescriptor(player, trace);
    actionContactSideEffect(player);

    // Static 01F7:3E41: the closed portion copies DS:4FE4 to DS:4FE6 before
    // the external view publisher.  The publisher's presentation globals are
    // not part of the player simulation contract, so only this relevant copy
    // is represented here.
    writeGlobal16Signed(0x4fe6, globals.viewStateB4FE6,
                        globals.viewStateA4FE4, trace);

    player.gate38 = 0;
    if (player.timer34 != 0) {
        player.timer34 = static_cast<std::uint16_t>(player.timer34 - 1);
        if (player.timer34 == 0) {
            writeGlobal16(0x8810, globals.timerClear8810, 0, trace);
        }
        if ((player.timer34 & 2) != 0) {
            player.statusWord12 = static_cast<std::uint16_t>(
                player.statusWord12 | 0x8000U);
        }
    }

    if (player.mode37 == 0 && player.actionWord == 0) {
        // The Ghidra decompilation at 43D5 increments on every ordinary idle
        // callback.  Only the equality-triggered 31BA table load is opaque;
        // it must not suppress the observed 4FEE write.
        writeGlobal16(0x4fee, globals.idleCounter4FEE,
                      static_cast<std::uint16_t>(
                          globals.idleCounter4FEE + 1), trace);
        if (globals.idleCounter4FEE == 0x00d2) {
            loadAnimationDescriptor(player, 0x31ba, trace);
        }
    } else {
        writeGlobal16(0x4fee, globals.idleCounter4FEE, 0, trace);
    }

    if (player.mode37 == 0 && globals.actionSuppressor89E6 == -1) {
        loadAnimationDescriptor(player, 0x316a, trace);
    }
}

void emitPendingSound(PlayerCallbackGlobals &globals,
                      PlayerTraceSink *trace) {
    writeGlobal16(0x612e, globals.pendingEvent612E, 0, trace);
    if (trace != 0) {
        trace->onEffectDispatch(PlayerEffectDispatch(0x01e70fcfU, 0));
    }
}

struct TransitionDescriptorProbeResult {
    bool contact;
    std::int32_t eaxAfterCall;

    TransitionDescriptorProbeResult(bool contactValue = false,
                                    std::int32_t eaxValue = 0)
        : contact(contactValue), eaxAfterCall(eaxValue) {}
};

bool transitionDescriptorPairClear(const PlayerRecord &player,
                                   const WorldCollisionView &world,
                                   PlayerTraceSink *trace) {
    // 01F7:447B-44BB: both branches execute the same two 5CC3 calls.  The
    // signed +0x29 branch is retained by the caller below because it is part
    // of the recovered control flow, while this helper preserves the exact
    // current/Y-16 probe order and TEST DX,0x70 polarity.
    const CollisionProbe current = CollisionKernel::probeAt(
        world, player.xPixel(), player.yPixel());
    captureProbe(current, (current.descriptorWord & 0x0070U) != 0, trace);
    const CollisionProbe previous = CollisionKernel::probeAt(
        world, player.xPixel(), addPixel(player.yPixel(), -0x10));
    captureProbe(previous, (previous.descriptorWord & 0x0070U) != 0, trace);
    return (current.descriptorWord & 0x0070U) == 0 &&
           (previous.descriptorWord & 0x0070U) == 0;
}

TransitionDescriptorProbeResult probeTransitionDescriptor1BD1(
    const PlayerRecord &player,
    const WorldCollisionView &world,
    std::int32_t callerEax,
    PlayerTraceSink *trace) {
    // 01F7:1BD1 is a far descriptor probe with CX=DX=0 at 44C5.  It restores
    // the original Y word into AX before testing the selected low-nibble bit;
    // the upper EAX word is not written by the 16-bit helper.  Consequently
    // 44CE consumes (caller EAX high word):(player Y pixel word), not the
    // fixed-point velocity value itself.
    const std::int16_t y = player.yPixel();
    const std::int16_t x = player.xPixel();
    const CollisionProbe probe = CollisionKernel::probeAt(world, x, y);
    const bool contact = CollisionKernel::occupied(probe);
    captureProbe(probe, contact, trace);
    const std::uint32_t preservedHigh =
        static_cast<std::uint32_t>(callerEax) & 0xffff0000U;
    const std::uint32_t restoredY = static_cast<std::uint16_t>(y);
    const std::int32_t eaxAfterCall = static_cast<std::int32_t>(
        preservedHigh | restoredY);
    return TransitionDescriptorProbeResult(contact, eaxAfterCall);
}

void transitionBranch4416(PlayerRecord &player,
                           const WorldCollisionView &world,
                           PlayerCallbackGlobals &globals,
                           PlayerTraceSink *trace) {
    stage(trace, PlayerUpdateStage::TransitionBranch);

    // Static 01F7:441D-4432.  8822 is the same address represented by the
    // 5937 health/display projection; the alias is intentional at this
    // callback boundary because both paths write the same DOS word.
    if (globals.collisionTransitionMode89EA == -1) {
        player.velocityY.raw = -0x20000;
        writeGlobal16(0x8822, globals.dispatchDisplayCount8822, 0, trace);
        loadAnimationDescriptor(player, 0x31a4, trace);
    }
    advanceAnimationDescriptor(player, trace);

    // Static 01F7:4439-4445 -> 01F7:20AF.
    writeGlobal32(0x60dc, globals.publishedViewDeltaX60DC, 0, trace);
    writeGlobal32(0x60e0, globals.publishedViewDeltaY60E0, 0, trace);

    player.velocityY.raw = Fixed16::wrapAddRaw(player.velocityY.raw, 0x1800);
    if (player.velocityY.raw > 0x20000) {
        player.velocityY.raw = 0x20000;
    }
    const std::int32_t callerEax = player.velocityY.raw;

    bool pairClear = false;
    if (static_cast<std::int8_t>(player.motionDirectionByte29) <= 0) {
        pairClear = transitionDescriptorPairClear(player, world, trace);
    } else {
        pairClear = transitionDescriptorPairClear(player, world, trace);
    }

    bool contact = !pairClear;
    if (pairClear) {
        const TransitionDescriptorProbeResult finalProbe =
            probeTransitionDescriptor1BD1(player, world, callerEax, trace);
        contact = finalProbe.contact;
        if (!contact) {
            // Static 01F7:44CE-44D3.  Keep both additions explicitly
            // wrapping at 32 bits, as the original writes the fixed-point
            // record with ADD/SUB rather than a host-language arithmetic
            // expression.
            player.positionY.raw = Fixed16::wrapAddRaw(
                player.positionY.raw, finalProbe.eaxAfterCall);
            player.positionX.raw = Fixed16::wrapSubRaw(
                player.positionX.raw, 0x5000);
        }
    }

    if (!contact) {
        return;
    }

    // Static 01F7:44DC-44F8.  This is a signed 16-bit global decrement.
    const std::uint16_t nextGate = static_cast<std::uint16_t>(
        static_cast<std::uint16_t>(globals.collisionTransitionMode89EA) - 1);
    writeGlobal16Signed(
        0x89ea, globals.collisionTransitionMode89EA,
        static_cast<std::int16_t>(nextGate), trace);
    if (globals.collisionTransitionMode89EA < -0x50 &&
        globals.collisionTransitionMode89EA < -0x15d) {
        writeGlobal16Signed(0x89ec, globals.transitionState89EC, -1, trace);
    }
}

} // namespace

TraceClosedPlayerUpdate::TraceClosedPlayerUpdate() : _globals() {
}

void TraceClosedPlayerUpdate::updatePlayer(
    PlayerRecord &player,
    const InputState &input,
    const WorldCollisionView &world,
    PlayerUpdateTrace *trace) {
    // Without the runtime descriptor table the C++ world boundary cannot
    // distinguish a clear descriptor from an absent descriptor. Preserve the
    // previously verified free-space behavior and expose this as the explicit
    // unresolved boundary instead of assigning semantics to zero descriptors.
    if (!world.hasDescriptorTable()) {
        if (trace != 0) {
            trace->onStage(PlayerUpdateStage::UnresolvedBoundary);
        }
        updatePlayerHorizontal(player, input, world, trace);
        if (player.mode37 != 0) {
            updatePlayerVerticalFreeSpace(player, trace);
        }
        return;
    }

    // Static 01F7:3FF8-4002: callback entry and transition gate.
    stage(trace, PlayerUpdateStage::CallbackEntry);
    player.syncToRaw();
    const PlayerRawRecord preState = player.toRaw();
    if (trace != 0) {
        trace->onPreState(preState);
    }

    const auto finishCallback = [&]() {
        player.syncToRaw();
        const PlayerRawRecord postState = player.toRaw();
        captureWrites(preState, postState, trace);
        stage(trace, PlayerUpdateStage::CapturePostState);
        if (trace != 0) {
            trace->onPostState(postState);
        }
    };

    // Static 01F7:3FF8 calls 5937 before testing DS:89EA. Its direct body
    // publishes only address-qualified auxiliary state; the nested 386F and
    // runtime-selected 0598 callbacks remain outside this player simulation
    // implementation until a trace demonstrates feedback into the record.
    updateAuxiliaryPlayerDispatch5937(_globals, trace);

    if (_globals.collisionTransitionMode89EA != 0) {
        // Static 01F7:4002 -> 01F7:4416.  This branch is also reached by
        // natural W1L1 damage (player-transition-writer-callback-v1), so it
        // is part of ordinary gameplay rather than a menu-only boundary.
        transitionBranch4416(player, world, _globals, trace);
        finishCallback();
        return;
    }

    // Static 01F7:401D-40A1: status/snapshot, deferred-Y, input dispatch,
    // transition consumption, suppression, and +0x40 action counter.
    stage(trace, PlayerUpdateStage::InputNormalization);
    player.statusWord12 = static_cast<std::uint16_t>(
        player.statusWord12 & 0x0fffU);
    player.savedY44 = player.positionY;
    player.savedX48 = player.positionX;

    if (_globals.deferredY8812 != 0) {
        player.gate38 = 0xff;
        player.positionY.raw = Fixed16::wrapAddRaw(
            player.positionY.raw,
            Fixed16::wrapAddRaw(_globals.deferredY8812, 1));
        // 01F7:4008 updates the record before the following contact helpers
        // read its packed pixel coordinates. Keep the raw view synchronized
        // so setYPixel/probe helpers preserve the published subpixel carry.
        player.syncToRaw();
        writeGlobal32(0x8812, _globals.deferredY8812, 0, trace);
    }

    std::uint16_t action = input.actionFlags();
    if (_globals.activationState85DA <= 0) {
        writeGlobal8(0x4ff0, _globals.actionLowCopy4FF0,
                     static_cast<std::uint8_t>(action), trace);
    } else {
        action = _globals.actionSource656C;
    }

    if (player.transition39 != 0) {
        player.transition39 = 0;
        player.mode37 = 1;
        player.resetDeathTimer3E = 0;
        action = static_cast<std::uint16_t>(action | 0x0022U);
    }
    if (_globals.actionSuppressor89E6 != 0) {
        action = 0;
    }
    player.actionWord = action;
    if ((action & 0x0022U) == 0) {
        player.callbackCounter40 = 0;
    }
    player.callbackCounter40 = static_cast<std::uint16_t>(
        player.callbackCounter40 + 1);
    if (trace != 0) {
        trace->onInputFlags(action);
    }
    stage(trace, PlayerUpdateStage::ActionCounterUpdate);

    // Static 01F7:4006-401D: 648E/6484/3A8A have a recovered
    // contact-object contract (6370 -> 3376 -> 0E06). The native scheduler
    // does not yet publish that runtime child/effect path, so no synthetic
    // contact is created here until a retail-contact parity fixture closes it.
    stage(trace, PlayerUpdateStage::UnresolvedBoundary);

    if (player.gate38 != 0 && player.mode37 != 0) {
        stage(trace, PlayerUpdateStage::VerticalContactGate);
        stage(trace, PlayerUpdateStage::GroundedContactResponse);
        groundedContact(player, world, _globals, trace);
        stage(trace, PlayerUpdateStage::CommonCallbackTail);
        commonCallbackTail(player, world, _globals, trace);
    } else {
        stage(trace, PlayerUpdateStage::HorizontalAccumulatorUpdate);
        updateHorizontalAccumulator(player, action, _globals, trace);
        player.actionWord = action;

        // Static 01F7:41AF-41BF: signed mode dispatch to 42B4, 4323, or 41E8.
        if (player.mode37 == 0) {
            // Static 01F7:42B4-4321: ordinary correction, jump gate, and
            // jump-initiation record/effect writes.
            stage(trace, PlayerUpdateStage::OrdinaryMode);
            const bool sideClear = sideProbeClear(player, world, trace);
            if (!sideClear || player.gate38 != 0) {
                snapPlayerY(player, world, trace);
                applyDescriptorCorrection(player, world, trace);
            } else if (player.verticalResponse3A == 0) {
                // Static 01F7:41C9 clears +0x3E before joining 41CF.  This
                // is the ordinary no-response contact path, distinct from
                // the already-timed early contact at 41C1.
                contactResponse(player, 0, true, trace);
            } else {
                snapPlayerY(player, world, trace);
                applyDescriptorCorrection(player, world, trace);
            }

            if ((action & 0x0022U) != 0 && player.gate38 == 0 &&
                player.callbackCounter40 <= 0x0d &&
                !probeVerticalTenPixels(player, world, trace)) {
                emitPendingSound(_globals, trace);
                player.resetDeathTimer3E = 0x03e8;
                player.sideResponse3B = 0;
                player.verticalResponse3A = 0;
                player.mode37 = -1;
                player.velocityY.raw = player.negativeYSpeed64.raw;
                loadAnimationDescriptor(player, 0x3160, trace);
                if (trace != 0) {
                    trace->onStage(PlayerUpdateStage::UnresolvedBoundary);
                }
            }

            stage(trace, PlayerUpdateStage::CommonCallbackTail);
            commonCallbackTail(player, world, _globals, trace);
        } else if (player.mode37 < 0) {
            // Static 01F7:4323-4368: ascent probe, release clamp, integrate,
            // post-step probe, and blocked-ascent response.
            stage(trace, PlayerUpdateStage::NegativeMode);
            if (probeVerticalStep(player, world, trace)) {
                contactResponse(player, 0x03e7, true, trace);
            } else {
                std::int32_t nextVelocity = Fixed16::wrapAddRaw(
                    player.velocityY.raw, player.negativeYAcceleration58.raw);
                if (player.contactScratch2B == 0 &&
                    (player.actionWord & 0x0022U) == 0 &&
                    nextVelocity < -0x20000) {
                    nextVelocity = -0x20000;
                }
                if (nextVelocity >= 0) {
                    contactResponse(player, 0x03e7, true, trace);
                } else {
                    player.velocityY.raw = nextVelocity;
                    player.positionY.raw = Fixed16::wrapAddRaw(
                        player.positionY.raw, player.velocityY.raw);
                    player.syncToRaw();
                    if (!probeVerticalStep(player, world, trace)) {
                        stage(trace,
                              PlayerUpdateStage::CommonCallbackTail);
                        commonCallbackTail(player, world, _globals, trace);
                        goto callback_complete;
                    }
                    contactResponse(player, 0x03e7, true, trace);
                }
            }
            stage(trace, PlayerUpdateStage::CommonCallbackTail);
            commonCallbackTail(player, world, _globals, trace);
        } else {
            // Static 01F7:41E8-42B1: falling/positive path, descriptor
            // correction, landing response, and post-step side probe.
            stage(trace, PlayerUpdateStage::PositiveMode);
            player.resetDeathTimer3E = static_cast<std::uint16_t>(
                player.resetDeathTimer3E + 1);
            if (positiveModeSideDescriptorsClear(player, world, trace) &&
                probeMapMask(world, player.xPixel(), player.yPixel(), 0x4000,
                             trace)) {
                player.setYPixel(static_cast<std::int16_t>(
                    static_cast<std::uint16_t>(player.yPixel()) & 0xfff0U));
            }
            if ((action & 0x0022U) != 0 &&
                player.callbackCounter40 <= 0x13 &&
                player.resetDeathTimer3E < 0x0a) {
                stage(trace, PlayerUpdateStage::GroundedContactResponse);
                groundedContact(player, world, _globals, trace);
            } else {
                player.contactScratch2B = 0;
                if (player.gate38 == 0) {
                    applyDescriptorCorrection(player, world, trace);
                }
                if (player.verticalResponse3A != 0) {
                    stage(trace, PlayerUpdateStage::GroundedContactResponse);
                    groundedContact(player, world, _globals, trace);
                } else {
                    player.velocityY.raw = Fixed16::wrapAddRaw(
                        player.velocityY.raw,
                        player.positiveYAcceleration50.raw);
                    if (player.velocityY.raw > player.positiveYSpeedCap60.raw) {
                        player.velocityY.raw = player.positiveYSpeedCap60.raw;
                    }
                    player.positionY.raw = Fixed16::wrapAddRaw(
                        player.positionY.raw, player.velocityY.raw);
                    player.syncToRaw();
                    if (!sideProbeClear(player, world, trace)) {
                        stage(trace,
                              PlayerUpdateStage::GroundedContactResponse);
                        groundedContact(player, world, _globals, trace);
                    }
                }
            }
            stage(trace, PlayerUpdateStage::CommonCallbackTail);
            commonCallbackTail(player, world, _globals, trace);
        }
    }

callback_complete:
    finishCallback();
}

void TraceClosedPlayerUpdate::publishPlatformCarry(
    std::int32_t xDelta8816, std::int32_t yDelta8812) {
    _globals.externalXDelta8816 = xDelta8816;
    _globals.deferredY8812 = yDelta8812;
}

void TraceClosedPlayerUpdate::publishTransitionGate(
    std::uint16_t transitionGate89ea) {
    _globals.collisionTransitionMode89EA =
        static_cast<std::int16_t>(transitionGate89ea);
}

void TraceClosedPlayerUpdate::publishGameplayCounters(
    std::uint32_t score881c,
    std::uint16_t lives880a,
    std::uint16_t ammo880c,
    std::uint16_t health8822,
    bool resetPublications) {
    _globals.dispatchScoreLow881C = static_cast<std::uint16_t>(score881c);
    _globals.dispatchScoreHigh881E = static_cast<std::uint16_t>(score881c >> 16);
    _globals.dispatchLives880A = lives880a;
    _globals.dispatchAmmo880C = ammo880c;
    _globals.dispatchDisplayCount8822 = health8822;
    if (resetPublications) {
        // Natural W1L2 startup enters 3FF8 with these display-side words
        // initialized by the outer setup path; 5937 then moves 4FF8 one
        // 16-bit step toward the current health on each callback.
        _globals.dispatchPreviousWord60DA = 0;
        _globals.dispatchPublishedScore4FF2 = 0;
        _globals.dispatchPublishedAmmo4FF6 = 0xffff;
        _globals.dispatchPublishedCount4FF8 = 1;
        _globals.dispatchPublishedLives4FFA = lives880a;
    }
}

std::int16_t TraceClosedPlayerUpdate::activationState85DA() const {
    return _globals.activationState85DA;
}

} // namespace quiky
