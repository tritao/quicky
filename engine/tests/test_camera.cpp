#include "quiky/camera.h"

#include <cassert>
#include <iostream>

namespace {

void testResetPreservesNativeAnchor() {
    quiky::GameplayCamera camera(320, 176);
    camera.reset(0, 262, 1024, 640);
    assert(camera.x() == 0);
    assert(camera.y() == 262);

    camera.follow(128, 400, 1024, 640);
    assert(camera.x() == 0);
    assert(camera.y() == 262);
}

void testHorizontalDeadZoneLeadsPlayer() {
    quiky::GameplayCamera camera(320, 176);
    camera.reset(0, 262, 1024, 640);

    camera.follow(190, 400, 1024, 640);
    assert(camera.x() == 0);
    camera.follow(191, 400, 1024, 640);
    assert(camera.x() == 1);
    assert(191 - camera.x() == 190);

    camera.follow(100, 400, 1024, 640);
    assert(camera.x() == 0);
}

void testVerticalDeadZoneMatchesJumpScroll() {
    quiky::GameplayCamera camera(320, 176);
    camera.reset(0, 262, 1024, 640);

    // The captured startup player remains at screen Y=138 until the ascent
    // crosses the top dead-zone edge.
    camera.follow(128, 351, 1024, 640);
    assert(camera.y() == 262);
    camera.follow(128, 348, 1024, 640);
    assert(camera.y() == 260);
    assert(348 - camera.y() == 88);

    camera.follow(128, 336, 1024, 640);
    assert(camera.y() == 248);
    camera.follow(128, 336, 1024, 640);
    assert(camera.y() == 248);
}

void testCameraClampsToSmallWorld() {
    quiky::GameplayCamera camera(320, 176);
    camera.reset(100, 100, 200, 100);
    assert(camera.x() == 0);
    assert(camera.y() == 0);
    camera.follow(-100, -100, 200, 100);
    assert(camera.x() == 0);
    assert(camera.y() == 0);
}

} // namespace

int main() {
    testResetPreservesNativeAnchor();
    testHorizontalDeadZoneLeadsPlayer();
    testVerticalDeadZoneMatchesJumpScroll();
    testCameraClampsToSmallWorld();
    std::cout << "camera tests passed\n";
    return 0;
}
