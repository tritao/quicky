# Audit of the current C++ player assumptions

This audit compares the current `engine/include/quiky/runtime.h` and
`engine/src/runtime.cpp` controller with the executable evidence in
`player-callback-static.md`, `player-input-static.md`, and the full-record
DOSBox captures. It is intentionally conservative: a field is not called
confirmed merely because its value looks like a familiar gameplay concept.

## Current public model

| C++ item | Status | Evidence-backed conclusion |
| --- | --- | --- |
| `PlayerState::x/y` | confirmed layout, incomplete semantics | The executable uses 32-bit 16.16 words at object `+0x02/+0x06`; C++ fixed-point storage is compatible. Exact integration order is not yet represented. |
| `PlayerState::velocityX/Y` | confirmed layout, incomplete semantics | The executable uses 32-bit words at `+0x0A/+0x0E`; signs and clamp constants are callback-path dependent. |
| `PlayerState::grounded` | unresolved/unsupported | No single executable byte has been proven to mean grounded. The callback uses signed mode `+0x37`, response state `+0x3A`, side state `+0x3B`, and several counters. |
| `PlayerState::facingRight` | unresolved/unsupported | The callback writes action/animation bytes at `+0x28/+0x29` and `+0x13`; no direct boolean equivalent has been established. |
| `PlayerConfig::width/height` | unresolved | The current `16x32` values are not recovered from the callback. Collision probes use explicit offsets such as `x±5`, `y-8`, and descriptor-dependent alignment. |
| `PlayerConfig::acceleration` | contradicted as an original constant | C++ uses `0x8000` per tick. The executable updates `DS:4FE8` in `0x1000`/`0x2000` steps and derives velocity from object constants; no `0x8000` acceleration write is present in the recovered path. |
| `PlayerConfig::maxHorizontalSpeed` | contradicted as an original constant | C++ hard-codes `3<<16`. The executable clamps through object fields and the `DS:4FE2/4FE8` accumulators; the recovered limits include `0x18000`, `0x10000`, and `-0x17fff` in different phases. |
| `PlayerConfig::friction` | unresolved | The release branch is coupled to `+0x2A`, `DS:4FEC`, and action bits; a constant linear friction rule has not been proven. |
| `PlayerConfig::gravity` | contradicted as a complete rule | The callback adds `+0x58` to vertical velocity on the negative path and applies descriptor/collision branches on other paths. A universal `+0x8000` gravity step is not supported. |
| `PlayerConfig::jumpVelocity` | unresolved/contradicted | The current `-8<<16` value is not in the static writes. Up-input traces show signed velocity changes, but jump eligibility and impulse are still entangled with mode/state bytes. |
| `CollisionRules::*Mask` | contradicted for original player collision | The player helpers mask the MAP cell to nine-bit tile ID and read descriptor-table record `+2`; they do not consume `value >> 9` as the collision decision. |
| `MapCollisionQuery` | useful provisional adapter, not original behavior | It is valid as a synthetic test query, but its upper-property masks must not be used as a claim about the DOSBox player. |
| `CollisionRules::outsideIsSolid` | unresolved | The executable has explicit camera/bounds/reset paths; no generic outside-solid rule has been isolated. |
| `PlayerDescriptorRules` | confirmed mechanical subset | Tile-ID lookup, quadrant mask, `0x20` response selector, `0x40` alignment selector, and low-word Y snapping are supported by static/runtime evidence. |
| `MapDescriptorQuery` | confirmed bridge, not integrated | The MAP-cell-to-descriptor lookup matches the executable formula. It intentionally does not assign final floor/ceiling names. |
| `PlayerSimulation::tick` order | contradicted as a faithful implementation | The callback updates action state, runs several helper chains, integrates 16.16 fields, then executes timer/camera/transition logic. The C++ tick order is only a provisional recreation. |

## Fields that the C++ model currently omits

The executable record is `0x78` bytes. The following offsets are directly
used by the initializer/callback and should be represented before a faithful
controller is attempted:

```text
+0x00  action word tested and rewritten by the callback
+0x02  16.16 X position
+0x06  16.16 Y position
+0x0A  16.16 horizontal velocity/accumulator input
+0x0E  16.16 vertical velocity
+0x12  word masked by callback and timer-bit writes
+0x18  callback offset
+0x28/+0x29  input/animation-related bytes
+0x2A/+0x2B  counters/gates
+0x2C/+0x2E  state/lifetime fields
+0x32  callback state word
+0x34  timer/invulnerability word
+0x36  animation/state byte
+0x37  signed callback mode byte
+0x38  callback gate byte
+0x39  transition byte
+0x3A  vertical response state
+0x3B  side/descriptor response state
+0x3E  reset/death timer
+0x40  callback counter
+0x44/+0x48  saved X/Y snapshots
+0x4C/+0x50/+0x54/+0x58/+0x5C/+0x60/+0x64  fixed-point constants/limits
+0x72  vertical step/response constant
```

The full-record tracer now captures all bytes and reports byte-level writes;
field names above remain confidence-qualified until read/write correlations
cover both normal and transition paths.

## Readiness decision

The current C++ player is **not implementation-ready**. A compatible engine
can use the descriptor bridge and the confirmed object layout, but it would
still invent movement constants, grounded/facing semantics, and transition
behavior. The next required evidence is a one-frame input matrix with the
callback’s helper returns and the signed fixed-point fields aligned on the
same frame barrier, followed by controlled release, landing, damage, and
respawn cases.
