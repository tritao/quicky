# Ordinary render-owner contracts

This note isolates the callbacks that own the ordinary records around the
transient high-effect draw. It is deliberately a render/lifecycle contract;
the helper calls inside `B33B` are not renamed as collision routines until
their callees and return flags are decoded.

## Shared queue evidence

The render pass at `01F7:3587` flushes an eight-byte record queue. In the
controlled W1L3 checkpoint (camera `(0,358)`) the queue is:

| queue index | pool index | callback | slot(s) | observed role |
| ---: | ---: | ---: | --- | --- |
| 0 | 1 | `B33B` | `951` | END1 ordinary owner |
| 1 | 2 | `B226` | `904`, `905`, `906` | END1 visibility/animation owner |
| 2 | 4 | `4C74` | `611`, `612`, `613` | transient PUFF effect |
| 3 | 3 | `B25D`/`4B70` | `994`, `993` | DOKTOR/effect owner |
| 4 | 0 | `3FF8` | `0`, `16` | player owner |

The ordinary owners therefore enter the shared queue before the transient
effect. This ordering is observed dynamically, but it is still only a
single-level sample; the recreation should not hard-code it as a universal
sort rule until another level and a camera transition confirm it.

A corrected W2L3 capture (camera `(0,134)`) shows why the full pool order must
remain data-driven. That level has thirteen queue records, with slots
`270, 207, 611, 994, 0, 214, 211, 214, 211, 214, 214, 214, 214`. The effect
is still record 2, DOKTOR slot `994` is record 3, and the player is record 4;
the additional ordinary records follow afterward. The W2 effect is `PUFFW2`
slot `611`, with world position `(300,210)`. Thus the cross-level invariant
currently supported is “ordinary scheduler owners, transient effect, DOKTOR,
player, then additional queued owners,” while the exact ordinary prefix is
level/pool dependent.

The W1L3 camera-motion run reaches camera `(103,490)` after controlled right
input and captures `[951, 906, 613, 994, 0]`. The effect remains queue record 2
while `B226` has advanced to slot `906`; the owner order and effect position
are unchanged as screen coordinates move. This confirms that the W1L3 order
is scheduler insertion order, not a fixed screen-space sort. It also shows
that the ordinary prefix is emitted before the transient effect on a moving
camera, while the player remains the final record in this five-owner scene.

## `B226`: visibility gate plus animation state

Static code at `01F7:B226` is complete enough to model exactly:

```text
if DS[0x88AE] < 4:
    call local continuation at B257
    return

screen_x = object.x - DS[0x81C0] + 0x20
screen_y = object.y - DS[0x81C4] + 0x20

if screen_x > 0x180 or screen_y > 0xF0:
    object.callback = 0
    set carry
else:
    clear carry

call local continuation at B257
return
```

The comparisons are unsigned and strict: positions exactly on the right or
bottom limits remain accepted. The callback does not itself select the BOB;
it leaves the current slot in `+0x12` for the common renderer. The observed
W1L3 sequence is:

```text
slot 904, state +0x2E: 0, 10, ..., 90
slot 905, state +0x2E: 100, 110, ..., 180
slot 906, state +0x2E: 190, 200, ...
```

The callback moves the object by subpixel/fixed-point state held in its
record; the integer samples show roughly one pixel per callback. The slot
change is an animation boundary, not a visibility transition. The exact
slot-duration table and the initial fixed-point values still need a shorter
cadence trace from object creation.

The gate is conditional on the global phase byte. A controlled debugger probe
with `DS:88AE=4` confirms the strict boundary at the live initial camera:

| forced object X | result | queue effect index |
| ---: | --- | ---: |
| `camera_x + 0x160` (`352`) | `B226` remains installed; slot `904` is queued | 2 |
| `camera_x + 0x161` (`353`) | `B226` writes callback `0`; slot `904` is omitted | 1 |

The effect's own record is unchanged; only the culled ordinary record drops
out of the shared queue. The same probe without forcing `DS:88AE=4` leaves
both positions active, confirming the early phase bypass at `B226:B22B`.

## `B33B`: phased animated owner

`01F7:B142` initializes this owner with slot `0x03B7` (`951`) and callback
`B33B`. `B33B` is not a simple draw callback. It uses `DS:0x88AE` as a global
phase gate and mutates the following record fields:

| field | confirmed use in `B33B` |
| --- | --- |
| `+0x02` | 32-bit fixed-point X position |
| `+0x06` | 32-bit fixed-point Y position |
| `+0x0A` | 32-bit X velocity/step |
| `+0x0E` | 32-bit Y velocity/step in the later phase |
| `+0x12` | alternates slots `0x0385`/`0x03B7`, then later `0x0384`/`0x03B6` |
| `+0x28` | signed direction/mode bit used to choose X offsets |
| `+0x29` | signed direction byte used to build velocity |
| `+0x2A` | linked pool-object index/offset used by the draw handoff |
| `+0x2C` | vertical step/delay value |
| `+0x2E` | animation/table cursor in the first phase |
| `+0x34` | transition flag into the next phase |
| `+0x36` | linked child object pointer |
| `+0x38` | phase timer |
| `+0x3E` | collision/helper result flag |
| `+0x40` | movement mode/direction flag |
| `+0x42` | bounded movement timer |
| `+0x44` | later-phase emission counter |

### Phase 0/1 (`DS:0x88AE < 2`)

The callback first saves a four-register probe context through relocated
helper `01F7:1B77`. That helper stores `AX/BX/CX/DX` at `DS:36F4..36FA`,
checks the player's current bounds through `01F7:393C`, and may enter the
action sink `01E7:0FCF` through `01F7:19E6` when its context overlaps. This
is a side-effect/action path; it is not a direct MAP read.

It then performs three directional MAP probes through relocated helper
`01F7:1C6E`:

```text
(x + signed_direction*0x32, y - 1)
(x + signed_direction*0x32, y - 0x11)
(x + direction-dependent 0x32, y - 0x0c)
```

`1C6E` computes the MAP cell at `(AX >> 4, BX >> 4)` using `DS:657A/657C`
and row stride `DS:657E`, reads the raw word, and tests bit `0x4000`. The
`B33B` `JNE` branches treat that bit as contact and set `+0x3E = 1`. The
remaining open question is the game-level name/coverage of bit `0x4000`,
not the helper identity or its raw behavior.

A one-shot phase probe at owner offset `0x0078` forcing `DS:0x88AE = 2`
confirms the first linked-record boundary. B33B advances the global phase to
`3`; its `+0x36` record (pool offset `0x0168`, callback B25D) runs once in
the same scheduler window and then stops receiving callback entries. The
`+0x2A` record (pool offset `0x00F0`, callback B226) remains independently
installed and continues its animation/render callbacks through at least the
first 27 phase-3 passes. The trace now records `DS:0x88AE` in every frame so later
phase transitions can be separated from renderer queue effects.

The movement branches are already exact at the instruction level:

- `+0x34 >= 1` enters the transition helper at `B84D`.
- With `+0x3E <= 0`, fixed-point X velocity is integrated, Y is adjusted by
  `+0x2C`, and a signed sample from the table at `DS:0x7974` is applied to Y.
  `+0x2E` advances by `0x20`; `+0x38` transitions after `0xDC`.
- With `+0x3E > 0` and `+0x40 < 0`, X velocity is clamped to
  `[-0x7000, +0x7000]`, `+0x42` decrements, and expiry flips direction
  fields, alternates the sprite slot, sets X velocity to `direction*0x200`,
  and reloads `+0x42 = 0x1E`.
- With `+0x40 >= 0`, the equivalent branch uses `direction*0x400` and a
  `0x14` timer; expiry sets `+0x3E = 0xFF` and flips `+0x40`.

At the end of the phase the callback copies position/animation state into the
linked object at `+0x2A` and `+0x36`. This explains why the queue sees the
ordinary owner before the transient effect even though the visible BOB may be
owned by a linked record.

### Later phases (`DS:0x88AE >= 2`)

The phase branches are also bounded:

- `< 3`: clear the linked object's callback, toggle the late-phase slot
  (`0x0385`/`0x03B7` to `0x03B6`/`0x0384`), and set the global phase to `3`.
- `< 4`: advance the later animation/motion path, emit a `4B70` child after
  the `+0x38 > 0x19` timer boundary, increment `+0x44`, and enter phase `4`
  after `+0x44 > 0x0F` while setting Y velocity to `-0x10000`.
- `< 5`: count to `0x28`, integrate the later Y step, update the linked
  object's position, and apply the camera gate `(x-camera+0x10 <= 0x160,
  y-camera+0x10 <= 0xD0)`. Outside that gate it clears its callback. Inside it
  calls `0x487F`, copies the returned position, and enters global phase `5`.
- `>= 5`: call `0x487F`, copy the returned position, and remain in the final
  phase path.

The linked-object and `0x487F` contracts are still open. They should be
traced as lifecycle/animation calls, not folded into the player collision
model.

### Controlled runtime handoff

A one-shot W1L3 probe forced `DS:0x88AE = 1` and the B33B owner byte
`+0x34 = 1` at owner offset `0x0078` (position `(300,500)`). This exercises
the real transition branch without waiting for the natural timer. The
observed sequence is:

1. B33B remains installed on the owner (slot `0x03B7`) and clears its
   transition byte after the call.
2. The B33B factory call allocates a child at pool offset `0x01E0` and B84D
   initializes it with slot `0x0386`, callback B87B, phase `2`, and
   `+0x3C = 0x30000`.
3. B87B runs for 31 callback passes in the fixed camera `(0,358)`. It moves
   the child from approximately `(269,500)` toward the right after its
   vertical path changes, while retaining callback B87B.
4. On the next pass the strict camera gate rejects the child at the upper
   Y boundary (`y=553`, so `y-camera+0x10 = 0xD3 > 0xD0`) and clears only
   `+0x18`. The record remains allocated with slot `0x0386` and its state
   fields intact.

This proves the B33B→B84D→B87B allocation and camera-gated deactivation
edge. It is a controlled transition, so it does not yet identify the
natural B33B trigger or prove whether a later pool pass reclaims/reactivates
the inactive record.

## Linked callback `B84D -> B87B`

The first transition helper is now statically constrained as well. `B84D`
changes the current record to slot `0x0386`, installs callback `B87B`, clears
`+0x3A`, initializes `+0x3C` to `0x30000`, and sets the later fixed-point
vertical step fields. `B87B` then:

- applies the strict camera gate
  `x-camera+0x10 <= 0x160` and `y-camera+0x10 <= 0xD0`, clearing `+0x18`
  outside it;
- uses `1BD1` and four `5C27` descriptor probes to test MAP descriptor
  flags around the moving object;
- reverses `+0x29` and `+0x0E` on a qualifying descriptor contact, increments
  `+0x3A`, and subtracts `0x5000` from the fixed-point bound at `+0x3C`;
- integrates and clamps the vertical step, updates Y, calls `1C4D` for the
  directional raw-MAP contact test, and applies the signed X adjustment;
- transitions through `0x487F` when the later state completes.

This closes the static callback edge but not its runtime lifetime. The next
trace should follow the same pool record from `B84D` entry through the first
`B87B` return and confirm whether the linked `+0x2A/+0x36` records are cleared
at the same boundary or one scheduler pass later.

## What remains before changing the recreation

1. Capture `B226` from creation at one-frame cadence to recover its exact
   initial fixed-point state, slot durations, and terminal callback behavior.
2. Trace the natural `B33B` action/context path (`1B77 -> 393C/19E6`) and sample
   MAP cells with raw bit `0x4000` to assign its gameplay meaning.
3. Trace the linked records at `+0x2A` and `+0x36`, including `B84D`, `B84C`,
   and `0x487F`, so object deletion and reactivation are not guessed.
4. Resolve the remaining full-frame palette/timing residuals after the
   queue-owned records are reproduced.
5. Once those pass, implement a queue-owned render pass in C++ and compare
   matched frames. The current frontend order remains provisional.
