#include "quiky/camera.h"

#include "quiky/types.h"

#include <algorithm>

namespace quiky {

CameraDeadZone::CameraDeadZone()
    : left(128), right(190), top(88), bottom(144) {
}

CameraDeadZone::CameraDeadZone(std::int32_t leftValue,
                               std::int32_t rightValue,
                               std::int32_t topValue,
                               std::int32_t bottomValue)
    : left(leftValue), right(rightValue), top(topValue), bottom(bottomValue) {
}

GameplayCamera::GameplayCamera(std::int32_t viewportWidth,
                               std::int32_t viewportHeight,
                               const CameraDeadZone &deadZone)
    : _viewportWidth(viewportWidth),
      _viewportHeight(viewportHeight),
      _deadZone(deadZone),
      _cameraX(0),
      _cameraY(0) {
    if (_viewportWidth < 0 || _viewportHeight < 0) {
        throw FormatError("camera viewport dimensions must be non-negative");
    }
    if (_deadZone.left > _deadZone.right ||
        _deadZone.top > _deadZone.bottom) {
        throw FormatError("camera dead-zone bounds are inverted");
    }
}

std::int32_t GameplayCamera::clampX(std::int32_t value,
                                    std::int32_t worldWidth) const {
    const std::int32_t maximum = std::max<std::int32_t>(
        0, worldWidth - _viewportWidth);
    return std::max<std::int32_t>(0, std::min(maximum, value));
}

std::int32_t GameplayCamera::clampY(std::int32_t value,
                                    std::int32_t worldHeight) const {
    const std::int32_t maximum = std::max<std::int32_t>(
        0, worldHeight - _viewportHeight);
    return std::max<std::int32_t>(0, std::min(maximum, value));
}

void GameplayCamera::reset(std::int32_t cameraX, std::int32_t cameraY,
                           std::int32_t worldWidth,
                           std::int32_t worldHeight) {
    _cameraX = clampX(cameraX, worldWidth);
    _cameraY = clampY(cameraY, worldHeight);
}

void GameplayCamera::follow(std::int32_t targetX, std::int32_t targetY,
                            std::int32_t worldWidth,
                            std::int32_t worldHeight) {
    const std::int32_t screenX = targetX - _cameraX;
    if (screenX < _deadZone.left) {
        _cameraX = targetX - _deadZone.left;
    } else if (screenX > _deadZone.right) {
        _cameraX = targetX - _deadZone.right;
    }

    const std::int32_t screenY = targetY - _cameraY;
    if (screenY < _deadZone.top) {
        _cameraY = targetY - _deadZone.top;
    } else if (screenY > _deadZone.bottom) {
        _cameraY = targetY - _deadZone.bottom;
    }

    _cameraX = clampX(_cameraX, worldWidth);
    _cameraY = clampY(_cameraY, worldHeight);
}

} // namespace quiky
