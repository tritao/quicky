# Player callback contract

This is the implementation contract from the selector-safe DOSBox pass. The
raw captures referenced here remain under `research/build/traces/`.

## Record and arithmetic

The player is `ES:DI`, normally selector `0x027f`, with a `0x78`-byte record.
The confirmed fixed-point fields are:

```text
+0x00 action word       +0x02/+0x06 X/Y (signed 16.16)
+0x0a/+0x0e X/Y velocity +0x18 callback
+0x28/+0x29 input/animation bytes  +0x2a/+0x2b counters
+0x2c/+0x2e state fields +0x32 callback state
+0x34 timer             +0x36 animation/state
+0x37 mode              +0x38 gate
+0x39 transition        +0x3a vertical response
+0x3b side response     +0x3e reset/death timer
+0x40 callback counter  +0x44/+0x48 saved X/Y
+0x4c..+0x64 fixed constants/limits; +0x72 vertical step
```

Input flags are `left=0x08`, `right=0x04`, `up=0x02`, `down=0x01`,
`alternate=0x10`, and `jump=0x20`. The one-frame matrix is in
`player-matrix-*.json`; it covers neutral, both directions, simultaneous
input, up, walking jump, release, and both reversals.

The implementation-relevant values are:

* X/Y use signed 32-bit 16.16 integration.
* Normal horizontal speed ramps in `0x2800` steps and caps at `0x18000`.
  Release uses the observed `0x2000` decrement; reversal applies the opposite
  `0x2800` step directly rather than first clearing the existing direction.
* The ascent path clamps at `-0x20000` and adds `0x2000` per callback.
  The jump capture starts with `+0x37=0xff` and the landing tail reaches
  `+0x37=1`; neither byte is promoted to the engine's `grounded` meaning.
* The long hold confirms `0x18000` as the normal cap and the release trace is
  `player-right-release-30frames.json`.

## Collision response

The callback chain is normally `648e -> 6484 -> 3a8a -> 3a1f -> 3df2`.
Near returns are used by `3a1f/3df2`; far returns by `648e/6484/3a8a`.
The tracer validates every return address and has event/repeat guards.

Descriptor values recovered from the live table and branch traces:

| tile | descriptor | `0x20` | `0x40` | observed branch |
|---:|---:|---:|---:|---|
| `0x28` | `0x0010` | no | no | clear response path |
| `0x29` | `0x0050` | no | yes | alignment branch |
| `0x2a` | `0x0070` | yes | yes | `AL=1`, response path |
| `0x2b` | `0x0030` | yes | no | response without alignment |

The low-nibble quadrant mask is `0x08/0x04/0x02/0x01` for the four
8-pixel quadrants. `3a1f` probes `(x-5,y)` then `(x+5,y)`; `3df2` repeats
that pair and snaps the integer Y word (`object+0x08`) to an 8-pixel boundary
when its gates permit it. The controlled property traces prove that a
blocking left probe short-circuits the right probe, while a non-blocking left
probe permits it.

```text
          x-5       x       x+5
           |---------|---------|
           3a1f/3df2 horizontal probes
                  y
       3d02 retries the descriptor lookup at y-8
```

## State and limits

The long right traversal reaches the known checkpoint/reset at approximately
`(2132,368)`, writes `+0x3e=0x03e8`, and later returns to `(1673,368)`.
The left boundary settles at `(72,400)` without the reset state. `DS:89e6`,
`DS:89ea`, and `DS:8812` are captured in `static_globals`; the transition
branch is documented in `player-callback-static.md`.

Damage/death and level-transition control flow is therefore represented in
the C++ session boundary, but the exact health/invulnerability producer and
goal write sequence remain unresolved. They must not be replaced with guessed
record fields. The runtime now exposes the recovered callback-mode,
transition, response, and death-timer projections and uses the confirmed
movement/descriptor arithmetic; the remaining state-machine fields stay
explicitly confidence-qualified.
