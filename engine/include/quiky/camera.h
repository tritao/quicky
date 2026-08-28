#ifndef QUIKY_CAMERA_H
#define QUIKY_CAMERA_H

#include <cstdint>

namespace quiky {

// The retail game keeps the player inside a deliberately asymmetric screen
// window: horizontal motion leads the player, while vertical scrolling starts
// near the top of the gameplay viewport.  Keeping this policy explicit makes
// the presentation camera independent from the simulation coordinates and
// gives callers a stable place to refine it as more camera traces are closed.
struct CameraDeadZone {
    std::int32_t left;
    std::int32_t right;
    std::int32_t top;
    std::int32_t bottom;

    CameraDeadZone();
    CameraDeadZone(std::int32_t leftValue, std::int32_t rightValue,
                   std::int32_t topValue, std::int32_t bottomValue);
};

class GameplayCamera {
public:
    GameplayCamera(std::int32_t viewportWidth,
                   std::int32_t viewportHeight,
                   const CameraDeadZone &deadZone = CameraDeadZone());

    // Set an authored/native starting anchor.  Coordinates are clamped to the
    // current world, but are not recentered around the player.
    void reset(std::int32_t cameraX, std::int32_t cameraY,
               std::int32_t worldWidth, std::int32_t worldHeight);

    // Advance the camera only when the player leaves the dead-zone.  The
    // current anchor is retained otherwise, which avoids the one-frame jump
    // caused by recomputing a centered camera every render pass.
    void follow(std::int32_t targetX, std::int32_t targetY,
                std::int32_t worldWidth, std::int32_t worldHeight);

    std::int32_t x() const { return _cameraX; }
    std::int32_t y() const { return _cameraY; }

private:
    std::int32_t clampX(std::int32_t value, std::int32_t worldWidth) const;
    std::int32_t clampY(std::int32_t value, std::int32_t worldHeight) const;

    std::int32_t _viewportWidth;
    std::int32_t _viewportHeight;
    CameraDeadZone _deadZone;
    std::int32_t _cameraX;
    std::int32_t _cameraY;
};

} // namespace quiky

#endif
