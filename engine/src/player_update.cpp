#include "quiky/player_update.h"

#include "quiky/fixed.h"

namespace quiky {
namespace {

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

void stage(PlayerTraceSink *trace, PlayerUpdateStage value) {
    if (trace != 0) {
        trace->onStage(value);
    }
}

} // namespace

PlayerUpdateTrace::PlayerUpdateTrace()
    : stages(), inputFlags(0), preState(), postState(), stateWrites(),
      collisionProbes() {
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
    (void)occupiedValue;
}

void PlayerUpdateTrace::onPreState(const PlayerRawRecord &state) {
    preState = state;
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

void ExperimentalHorizontalPlayerUpdate::updatePlayer(
    PlayerRecord &player,
    const InputState &input,
    const WorldCollisionView &world,
    PlayerUpdateTrace *trace) {
    updatePlayerHorizontal(player, input, world, trace);
}

} // namespace quiky
