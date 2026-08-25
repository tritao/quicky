#ifndef QUIKY_COLLISION_KERNEL_H
#define QUIKY_COLLISION_KERNEL_H

#include "quiky/world_view.h"

#include <cstdint>
#include <vector>

namespace quiky {

struct CollisionProbe {
    std::int16_t pixelX;
    std::int16_t pixelY;
    std::uint16_t mapWord;
    std::uint16_t tileId;
    std::uint16_t descriptorWord;
    std::uint8_t quadrantMask;
    bool inBounds;

    CollisionProbe();
};

struct CollisionDecision {
    bool occupied;
    bool alignmentResponse;
    bool verticalResponse;
    bool shortCircuited;
    bool firstProbeClear;
    bool secondProbeTested;
    bool ySnapped;
    std::int16_t correctedY;
    std::vector<CollisionProbe> probes;

    CollisionDecision();
};

struct DescriptorResponseInput {
    std::int16_t pixelX;
    std::int16_t pixelY;
    std::int32_t velocityY;
    std::int8_t callbackMode;
    std::uint8_t sideResponse;

    DescriptorResponseInput(std::int16_t x = 0,
                            std::int16_t y = 0,
                            std::int32_t velocity = 0,
                            std::int8_t mode = 0,
                            std::uint8_t side = 1)
        : pixelX(x),
          pixelY(y),
          velocityY(velocity),
          callbackMode(mode),
          sideResponse(side) {}
};

struct DescriptorResponseDecision {
    std::uint16_t returnOffset;
    std::uint8_t al;
    bool hasAl;
    bool retried;
    bool accepted;
    bool alignmentResponse;
    bool verticalResponse;
    std::uint16_t descriptorWord;
    std::int16_t originalY;
    std::int16_t finalY;
    std::int32_t originalVelocityY;
    std::int32_t finalVelocityY;
    std::int8_t finalVerticalResponse;
    std::vector<CollisionProbe> probes;

    DescriptorResponseDecision();
};

// Pure translation of the recovered 5C27/5CC3/3A1F/3DF2/3D02 leaves. It
// reads only through a const WorldCollisionView and returns all raw probe
// values needed by trace comparison.
class CollisionKernel {
public:
    static std::uint8_t quadrantMask(std::int16_t pixelX,
                                     std::int16_t pixelY);
    static CollisionProbe probeAt(const WorldCollisionView &world,
                                  std::int16_t pixelX,
                                  std::int16_t pixelY);
    static bool occupied(const CollisionProbe &probe);

    static CollisionDecision sideProbePair(const WorldCollisionView &world,
                                            std::int16_t playerX,
                                            std::int16_t playerY);
    static CollisionDecision snapYOnSideContact(
        const WorldCollisionView &world,
        std::int16_t playerX,
        std::int16_t playerY,
        std::uint8_t sideResponse,
        std::int8_t verticalResponse);

    static DescriptorResponseDecision resolveDescriptorResponse(
        const WorldCollisionView &world,
        const DescriptorResponseInput &input);
};

} // namespace quiky

#endif
