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

The callback first calls three helper sites with probes around the object:

```text
(x + signed_direction*0x32, y - 1)
(x + signed_direction*0x32, y - 0x11)
(x + direction-dependent 0x32, y - 0x0c)
```

If their return condition indicates a hit, it sets `+0x3E = 1`. This is the
only part that may be collision/MAP-related; the helper identities and exact
carry/zero meaning remain unresolved.

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

## What remains before changing the recreation

1. Capture `B226` from creation at one-frame cadence to recover its exact
   initial fixed-point state, slot durations, and terminal callback behavior.
2. Trace the three `B33B` helper callees far enough to identify their input
   map/geometry reads and carry/zero result contract. This is the only
   remaining collision-adjacent item in this render-owner slice.
3. Trace the linked records at `+0x2A` and `+0x36`, including `B84D`, `B84C`,
   and `0x487F`, so object deletion and reactivation are not guessed.
4. Repeat queue capture on a second level and across one camera movement to
   distinguish true insertion order from W1L3's current pool allocation.
5. Once those pass, implement a queue-owned render pass in C++ and compare
   matched frames. The current frontend order remains provisional.
