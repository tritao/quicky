#include "quiky/collision_kernel.h"

#include "quiky/fixed.h"

namespace quiky {
namespace {

std::uint16_t wrap16(std::int32_t value) {
    return static_cast<std::uint16_t>(value);
}

std::int16_t signed16(std::uint16_t value) {
    if (value <= 0x7fff) {
        return static_cast<std::int16_t>(value);
    }
    return static_cast<std::int16_t>(-32768 + (value - 0x8000));
}

void addProbeSummary(CollisionDecision &decision,
                     const CollisionProbe &probe) {
    decision.probes.push_back(probe);
    decision.alignmentResponse =
        decision.alignmentResponse || ((probe.descriptorWord & 0x0040) != 0);
    decision.verticalResponse =
        decision.verticalResponse || ((probe.descriptorWord & 0x0020) != 0);
}

} // namespace

CollisionProbe::CollisionProbe()
    : pixelX(0),
      pixelY(0),
      mapWord(0),
      tileId(0),
      descriptorWord(0),
      quadrantMask(0),
      inBounds(false) {
}

CollisionDecision::CollisionDecision()
    : occupied(false),
      alignmentResponse(false),
      verticalResponse(false),
      shortCircuited(false),
      firstProbeClear(false),
      secondProbeTested(false),
      ySnapped(false),
      correctedY(0),
      probes() {
}

DescriptorResponseDecision::DescriptorResponseDecision()
    : returnOffset(0),
      al(0),
      hasAl(false),
      retried(false),
      accepted(false),
      alignmentResponse(false),
      verticalResponse(false),
      descriptorWord(0),
      originalY(0),
      finalY(0),
      originalVelocityY(0),
      finalVelocityY(0),
      finalVerticalResponse(0),
      probes() {
}

std::uint8_t CollisionKernel::quadrantMask(std::int16_t pixelX,
                                            std::int16_t pixelY) {
    const std::uint16_t x = static_cast<std::uint16_t>(pixelX);
    const std::uint16_t y = static_cast<std::uint16_t>(pixelY);
    const bool xBit3 = (x & 0x0008) != 0;
    const bool yBit3 = (y & 0x0008) != 0;
    if (yBit3) {
        return static_cast<std::uint8_t>(xBit3 ? 0x02 : 0x01);
    }
    return static_cast<std::uint8_t>(xBit3 ? 0x04 : 0x08);
}

CollisionProbe CollisionKernel::probeAt(const WorldCollisionView &world,
                                         std::int16_t pixelX,
                                         std::int16_t pixelY) {
    const std::uint16_t x = static_cast<std::uint16_t>(pixelX);
    const std::uint16_t y = static_cast<std::uint16_t>(pixelY);
    const MapCell cell = world.cellAt(static_cast<std::int32_t>(x >> 4),
                                      static_cast<std::int32_t>(y >> 4));
    const TileDescriptor descriptor = world.descriptorFor(cell);
    CollisionProbe result;
    result.pixelX = pixelX;
    result.pixelY = pixelY;
    result.mapWord = cell.rawWord;
    result.tileId = cell.tileId;
    result.descriptorWord = descriptor.valid ? descriptor.descriptorWord : 0;
    result.quadrantMask = quadrantMask(pixelX, pixelY);
    result.inBounds = cell.inBounds && descriptor.valid;
    return result;
}

bool CollisionKernel::occupied(const CollisionProbe &probe) {
    return (probe.descriptorWord & 0x000f) != 0 &&
           (probe.descriptorWord & probe.quadrantMask) != 0;
}

CollisionDecision CollisionKernel::sideProbePair(
    const WorldCollisionView &world,
    std::int16_t playerX,
    std::int16_t playerY) {
    CollisionDecision result;
    result.correctedY = playerY;

    const CollisionProbe left = probeAt(
        world, signed16(wrap16(static_cast<std::int32_t>(playerX) - 5)),
        playerY);
    addProbeSummary(result, left);
    if (occupied(left)) {
        result.occupied = true;
        result.shortCircuited = true;
        return result;
    }
    result.firstProbeClear = true;

    const CollisionProbe right = probeAt(
        world, signed16(wrap16(static_cast<std::int32_t>(playerX) + 5)),
        playerY);
    addProbeSummary(result, right);
    result.secondProbeTested = true;
    result.occupied = occupied(right);
    return result;
}

CollisionDecision CollisionKernel::snapYOnSideContact(
    const WorldCollisionView &world,
    std::int16_t playerX,
    std::int16_t playerY,
    std::uint8_t sideResponse,
    std::int8_t verticalResponse) {
    CollisionDecision result;
    result.correctedY = playerY;
    if (sideResponse == 0 || verticalResponse != 0) {
        return result;
    }

    result = sideProbePair(world, playerX, playerY);
    if (result.occupied) {
        result.correctedY = signed16(
            static_cast<std::uint16_t>(static_cast<std::uint16_t>(playerY) &
                                       0xfff8));
        result.ySnapped = true;
    }
    return result;
}

DescriptorResponseDecision CollisionKernel::resolveDescriptorResponse(
    const WorldCollisionView &world,
    const DescriptorResponseInput &input) {
    DescriptorResponseDecision result;
    result.originalY = input.pixelY;
    result.finalY = input.pixelY;
    result.originalVelocityY = input.velocityY;
    result.finalVelocityY = input.velocityY;

    if (input.sideResponse == 0) {
        result.returnOffset = 0x3df1;
        result.finalVerticalResponse = 0;
        return result;
    }

    const CollisionProbe current =
        probeAt(world, input.pixelX, input.pixelY);
    result.probes.push_back(current);
    result.descriptorWord = current.descriptorWord;
    std::int16_t workingY = input.pixelY;
    CollisionProbe selected = current;

    if ((selected.descriptorWord & 0x0030) == 0) {
        result.retried = true;
        workingY = signed16(wrap16(static_cast<std::int32_t>(input.pixelY) - 8));
        selected = probeAt(world, input.pixelX, workingY);
        result.probes.push_back(selected);
        result.descriptorWord = selected.descriptorWord;
        if ((selected.descriptorWord & 0x0030) == 0) {
            result.returnOffset = 0x3d44;
            result.hasAl = true;
            result.al = static_cast<std::uint8_t>(
                static_cast<std::uint16_t>(input.pixelY));
            result.finalY = input.pixelY;
            result.finalVerticalResponse = 0;
            return result;
        }
    }

    result.verticalResponse = (selected.descriptorWord & 0x0020) != 0;
    result.alignmentResponse = (selected.descriptorWord & 0x0040) != 0;
    result.finalVerticalResponse = result.verticalResponse
                                       ? static_cast<std::int8_t>(-1)
                                       : static_cast<std::int8_t>(1);
    if (input.callbackMode == 0) {
        // 01F7:3D52/3D8F reads player +0x0A, not +0x0E, and publishes its
        // signed half into +0x0E. Keep both velocity inputs explicit so a
        // diagonal/jump callback cannot use vertical velocity as the source
        // of this contact response.
        if (result.verticalResponse) {
            result.finalVelocityY = Fixed16::arithmeticShiftRight(
                input.velocityX, 1);
        } else {
            result.finalVelocityY = Fixed16::arithmeticShiftRight(
                Fixed16::wrapNegRaw(input.velocityX), 1);
        }
    }

    const std::uint16_t x = static_cast<std::uint16_t>(input.pixelX);
    const std::uint16_t y = static_cast<std::uint16_t>(workingY);
    std::uint16_t phase = result.verticalResponse
                              ? static_cast<std::uint16_t>((x & 0x000f) >> 1)
                              : static_cast<std::uint16_t>(
                                    (0x000f - (x & 0x000f)) >> 1);
    std::uint16_t target = static_cast<std::uint16_t>((y & 0xfff0) + phase);
    if (!result.alignmentResponse) {
        target = static_cast<std::uint16_t>(target + 8);
    }

    if (static_cast<std::int16_t>(static_cast<std::uint16_t>(input.pixelY)) <
        static_cast<std::int16_t>(target)) {
        result.finalVerticalResponse = 0;
        result.finalY = workingY;
        result.hasAl = true;
        result.al = 0;
        result.returnOffset = 0x3de4;
        return result;
    }

    if (static_cast<std::uint16_t>(input.pixelY) != target) {
        result.finalY = signed16(target);
    } else {
        result.finalY = input.pixelY;
    }
    result.accepted = true;
    result.hasAl = true;
    result.al = 1;
    result.returnOffset = 0x3df1;
    return result;
}

} // namespace quiky
