#include "quiky/player_update.h"

#include "quiky/fixed.h"

namespace quiky {
namespace {

void reportGlobalWrite(PlayerTraceSink *trace,
                       std::uint16_t address,
                       std::uint8_t width,
                       std::uint32_t before,
                       std::uint32_t after) {
    if (trace != 0) {
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
      collisionTransitionMode89EA(0) {
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
    const CollisionDecision decision = CollisionKernel::sideProbePair(
        world, player.xPixel(), player.yPixel());
    captureProbes(decision, trace);
    if (!decision.occupied && player.gate38 == 0 && player.mode37 != -1) {
        player.sideResponse3B = 0xff;
    }
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
                                    player.sideResponse3B));
    for (std::size_t index = 0; index < decision.probes.size(); ++index) {
        captureProbe(decision.probes[index],
                     CollisionKernel::occupied(decision.probes[index]), trace);
    }
    player.verticalResponse3A = decision.finalVerticalResponse;
    player.velocityY.raw = decision.finalVelocityY;
    player.setYPixel(decision.finalY);
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

    const std::int32_t cap = player.horizontalSpeedCap5C.raw;
    if ((player.actionWord & 0x0004) != 0) {
        const std::int32_t next = Fixed16::wrapAddRaw(
            player.velocityX.raw, player.acceleration4C.raw);
        player.velocityX.raw = next > cap ? cap : next;
        if (player.animationState36 != 0 && player.mode37 == 0) {
            player.animationState36 = 0;
        }
    } else if ((player.actionWord & 0x0008) != 0) {
        const std::int32_t next = Fixed16::wrapSubRaw(
            player.velocityX.raw, player.acceleration4C.raw);
        player.velocityX.raw = next < Fixed16::wrapNegRaw(cap)
                                   ? Fixed16::wrapNegRaw(cap)
                                   : next;
        if (player.animationState36 != 0 && player.mode37 == 0) {
            player.animationState36 = 0;
        }
    } else {
        if (player.animationState36 == 0) {
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
        }
        if (speed >= 0x28000 && (player.statusWord12 & 0xff00U) != 0xff00U) {
            player.statusWord12 = static_cast<std::uint16_t>(
                (player.statusWord12 & 0x00ffU) | 0xff00U);
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
    // The animation table loader at 5D38 is intentionally not represented by
    // a guessed table. Its address remains an unresolved trace boundary.
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
        // 5D38 is an unresolved animation-table read; the state write at
        // +36 below is independently established by the callback bytes.
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

    if (trace != 0) {
        trace->onStage(PlayerUpdateStage::UnresolvedBoundary);
    }
    actionContactSideEffect(player);

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
        if (globals.idleCounter4FEE < 0x00d2) {
            writeGlobal16(0x4fee, globals.idleCounter4FEE,
                          static_cast<std::uint16_t>(
                              globals.idleCounter4FEE + 1), trace);
        } else if (trace != 0) {
            trace->onStage(PlayerUpdateStage::UnresolvedBoundary);
        }
    } else {
        writeGlobal16(0x4fee, globals.idleCounter4FEE, 0, trace);
    }

    if (player.mode37 == 0 && globals.actionSuppressor89E6 == -1 &&
        trace != 0) {
        trace->onStage(PlayerUpdateStage::UnresolvedBoundary);
    }
}

void emitPendingSound(PlayerCallbackGlobals &globals,
                      PlayerTraceSink *trace) {
    writeGlobal16(0x612e, globals.pendingEvent612E, 0, trace);
    if (trace != 0) {
        trace->onEffectDispatch(PlayerEffectDispatch(0x01e70fcfU, 0));
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

    if (_globals.collisionTransitionMode89EA != 0) {
        stage(trace, PlayerUpdateStage::UnresolvedBoundary);
        player.syncToRaw();
        const PlayerRawRecord postState = player.toRaw();
        captureWrites(preState, postState, trace);
        if (trace != 0) {
            trace->onPostState(postState);
        }
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

    // Static 01F7:4006-401D: 648E/6484/3A8A are retained as a mechanical
    // boundary until their runtime
    // contact-object initialization is integrated. No synthetic contact is
    // created here.
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
                contactResponse(player, 0, false, trace);
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
            if (sideProbeClear(player, world, trace) &&
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
    player.syncToRaw();
    const PlayerRawRecord postState = player.toRaw();
    captureWrites(preState, postState, trace);
    stage(trace, PlayerUpdateStage::CapturePostState);
    if (trace != 0) {
        trace->onPostState(postState);
    }
}

} // namespace quiky
