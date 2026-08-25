# Player horizontal movement

This note is the native callback result for the player at `01F7:3FF8`, not a
frontend approximation. The capture target is normally `ES:DI = 0x027f:0x0000`;
the complete record is 120 bytes. Each sample in the evidence ledger contains
the record immediately before and after the callback (`pre_object` and
`post_object`) plus the callback writes.

## Exact free-space recurrence

The position and velocity fields are signed 32-bit 16.16 values:

```text
A = 0x00002800       acceleration step = 0.15625 px/frame^2
F = 0x00002000       release step      = 0.125 px/frame
V = 0x00018000       speed limit        = 1.5 px/frame

x_next = x + v_before

if right is held:
    v_next = min(+V, v_before + A)
elif left is held:
    v_next = max(-V, v_before - A)
else:
    v_next = toward_zero(v_before, F)
```

The update order is therefore position integration first, then horizontal
velocity update. There is no per-frame integer-X rounding: the fixed-point
fraction remains in the record. On the captured nonnegative range,
`x_integer = x_fixed >> 16`.

Opposite input applies the opposite `A` step directly. It does not first
clear the old direction. For example, after two right callbacks the measured
velocity is `0x3000`; the first left callback changes it to `0x0800`.

Starting from `x=0x00800000`, `v=0`, a held direction reaches the cap on hold
frame 10. The position at the end of an N-frame hold is:

```text
x_N = x_0 + sum(min(i*A, V) for i in range(N))
v_N = min(N*A, V)                 # right; negate for left
```

These values are visible directly in the complete ledgers. For example, a
16-frame right hold ends at `x=0x008ff800`, `v=0x00018000` before the release
tail.

The initializer stores the same constants in the record at `+0x4c/+0x50`
(`0x2800`), `+0x54/+0x58` (`0x2000`), and `+0x5c` (`0x18000`). The static
initializer evidence is in [player-callback-static.md](player-callback-static.md).

## Record fields and state changes

The machine-readable report decodes every field that changed in the captured
records. The relevant layout is:

| Offset | Width | Field | Observed horizontal/state use |
|---:|---:|---|---|
| `+0x00` | 16 | action word | neutral `0`; right `0x0004`; left `0x0008` |
| `+0x02` | 32 | X fixed | signed 16.16 position |
| `+0x06` | 32 | Y fixed | unchanged during free-space traces |
| `+0x0a` | 32 | X velocity | signed 16.16 velocity |
| `+0x0e` | 32 | Y velocity | unchanged during free-space traces |
| `+0x12` | 16 | sprite slot | right walk `0..7`; left walk `50..57`; idle table uses `0,16..19` |
| `+0x18` | 16 | callback | `0x3ff8` |
| `+0x20` | 16 | animation delay | walk delay is 4; raw countdown is captured |
| `+0x22` | 16 | animation cursor | raw animation-table state is captured |
| `+0x28..+0x29` | bytes | input/facing bytes | neutral/right `01/01`; left settles at `ff/ff`; reversal exposes intermediate values |
| `+0x2a` | 16 | action counter | unchanged in these free-space traces |
| `+0x2e` | 16 | state field | initial `0x0028`, then common idle value `0xffd8` |
| `+0x32` | 16 | update state | remains `0` in free-space traces |
| `+0x34` | 16 | callback timer | unchanged in these free-space traces |
| `+0x36..+0x3b` | bytes | animation/state bytes | direction/transition/response bytes; all raw values retained |
| `+0x3e` | 16 | reset/death timer | `1000` at the right checkpoint/reset state |
| `+0x40` | 16 | callback counter | increments from 0 to 1 on the first callback |
| `+0x44` | 32 | saved Y fixed | callback save slot; raw value retained |
| `+0x48` | 32 | saved X fixed | callback save slot; raw value retained |

The first neutral callback changes `+0x2e` from `0x0028` to `0xffd8` and
initializes the idle animation countdown. While a direction is held, the
action word is the corresponding input flag and the paired sprite table
identifies facing. On release the action word returns to zero and the idle
table takes over. The raw pre/post record—not a guessed semantic alias—is the
authoritative state representation; `timelines.json` preserves it for every
captured frame.

## Evidence matrix

The corpus under [evidence/player-horizontal/](../evidence/player-horizontal/)
contains:

- a 60-frame neutral baseline;
- right and left holds of 1, 2, 4, 8, 16, and 32 frames;
- low, medium, and maximum-speed reversals, including long opposite holds;
- long left/right boundary runs and collision-focused callback traces; and
- held-out right/left holds of 3, 5, 12, 24, and 48 frames.

The one-frame debugger barriers sometimes allow an extra guest callback in a
phase tail, and a long opposite-key phase has one direction-change boundary
sampling offset. This affects only which callback is labelled as the phase
endpoint; the local pre/post recurrence above remains unchanged. The analysis
tool accounts for that scheduling detail instead of changing the gameplay
formula.

## Boundaries and collision state

The left run settles at fixed X `0x00490000` (`72.96875`, integer X `72`) with
zero horizontal velocity. Its callback state is stable after the collision.

The right long run does not provide a symmetric solid-wall sample. It reaches
the checkpoint/reset path at fixed position `(0x08560000, 0x01700000)` —
integer `(2132, 368)` — with X velocity `0x18000`, Y velocity `0x20000`,
`+0x37=0xff`, `+0x3b=0`, and `+0x3e=1000`. A later collision-focused trace
observes the reset toward X `1673`. These are collision/level-state effects,
not inputs to the free-space horizontal recurrence.

## Machine-readable outputs and validation

[constants.json](../evidence/player-horizontal/constants.json) contains the
versioned constants, field layout, action words, animation tables, and boundary
checkpoints. [timelines.json](../evidence/player-horizontal/timelines.json)
contains every decoded pre/post record and expected motion row. The complete
changed-value inventory is in
[record-field-changes.json](../evidence/player-horizontal/record-field-changes.json)
contains named changed fields plus an exact changed-value map for every raw
record byte, including bytes whose semantic name is not yet settled.

[validation.json](../evidence/player-horizontal/validation.json) reports 28
formula traces with zero mismatches and 9 held-out traces with zero
mismatches. Collision/reset traces are retained as evidence and deliberately
reported separately from the free-space model.

The reproducible report command is:

```sh
python3 research/tools/player_horizontal_analysis.py \
  research/evidence/player-horizontal \
  --constants research/evidence/player-horizontal/constants.json \
  --timelines research/evidence/player-horizontal/timelines.json \
  --validation research/evidence/player-horizontal/validation.json \
  --field-changes research/evidence/player-horizontal/record-field-changes.json
```

The executable and archive hashes used for the capture, callback address,
record size, and fixed-point encoding are recorded in `constants.json` so a
future trace can be compared against the same native build.
