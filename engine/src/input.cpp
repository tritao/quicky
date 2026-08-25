#include "quiky/runtime.h"

namespace quiky {

InputState InputState::fromActionFlags(std::uint16_t flags) {
    InputState input;
    input.left = (flags & 0x08) != 0;
    input.right = (flags & 0x04) != 0;
    input.up = (flags & 0x02) != 0;
    input.down = (flags & 0x01) != 0;
    input.jump = (flags & 0x20) != 0;
    input.alternate = (flags & 0x10) != 0;
    return input;
}

std::uint16_t InputState::actionFlags() const {
    std::uint16_t flags = 0;
    if (down) flags |= 0x01;
    if (up) flags |= 0x02;
    if (right) flags |= 0x04;
    if (left) flags |= 0x08;
    if (alternate) flags |= 0x10;
    if (jump) flags |= 0x20;
    return flags;
}

} // namespace quiky
