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

The creation/animation helper edge is now decoded statically. B142 allocates
the B226 child through B1F0, then allocates the B25D child through B20B. Both
initializers call `01F7:5D38`, which reads the sequence at the supplied `SI`
cursor and initializes:

```text
+0x1E/+0x20 = sequence delay/reload
+0x22/+0x24 = sequence start/current cursors
+0x12       = first sprite slot (with the -1 direction bank offset)
```

The per-callback helper `01F7:5D60` decrements `+0x20`; when it reaches zero,
it advances `+0x24` by two bytes, skips negative table entries by rewinding
over their encoded span, selects the next slot, and reloads `+0x20` from
`+0x1E`. The controlled B226 record starts with delay/reload `8` and cursor
`0x3426`, so its slot boundaries are eight callback passes apart. This
replaces the earlier “roughly one pixel/slot” description with the actual
record-local animation contract; creation cadence and the table's semantic
frame names remain open.

A targeted Ghidra pass over the lifecycle callbacks narrows teardown further.
`B25D` handles target hits through `+0x2A/+0x2C`, advances its animation state,
and can publish global phase `2` after more than four hits. Its body has no
direct write clearing `+0x18`; the `+0x2E >= 1` branch only rolls its local
animation counter and calls an unresolved helper after the `+0x2F > 100`
boundary. `B33B` likewise has no phase-0/1 self-clear. Its explicit linked
clear is only in the later `< 3` phase branch, where it writes zero to the
record at `+0x36`. Therefore the natural sample's simultaneous disappearance
of B33B and B25D is an external scheduler/streaming/action teardown edge, not
an obvious direct clear in either callback; that edge needs one more runtime
pass with the general deactivation and helper-call sites armed.

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
`3`; its `+0x36` record (pool offset `0x0168`) is initially observed as the
B20B initializer and then as B25D. Direct pool samples show that B25D remains
active after the phase transition, so the earlier callback-only trace that
appeared to stop at this boundary was an ordering/breakpoint artifact. The
evidence is consistent with the already-queued B20B initializer completing
after B33B's linked-record clear and reinstalling B25D. The `+0x2A` record
(pool offset `0x00F0`, callback B226) remains independently installed through
phase 3. The lightweight trace records `DS:0x88AE` and the first pool records
between frame waits, avoiding debugger-stop artifacts in this lifecycle pass.

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

An unforced W1L3 pool sample now captures the natural edge. With the original
owner position and global state untouched, B33B drifts left from approximately
`(405,565)` at game frame `401` to `(386,553)` at frame `445`. The owner reaches
the handoff with its phase timer at `0xDC`; on the next callback it sets
`+0x34 = 1`, resets `+0x38`, and the first-free record `0x01E0` receives
initializer callback B84D. On frame `446` the same record is callback B87B with
slot `0x0386`. Its strict gate rejects `y=553` in camera `(0,358)`
(`y-camera+0x10 = 0xD3`), so by frame `447` the callback is cleared and the
same pool record is reused by `10B5`. The owner remains B33B and the global
phase remains `1`.

The lightweight sampler also reads the three B33B `1C6E` probe cells at each
frame. At the transition boundary the owner is `(386,553)`, direction mode is
`0xff`, and the probes are `(336,552) -> 0x018e`, `(336,536) -> 0x017a`, and
`(436,541) -> 0x0001`; none has raw bit `0x4000`. The same bit remains clear
through the surrounding 50-frame sample. This means the observed frame-445
handoff is timer-driven in this run, not a direct `0x4000` MAP hit at the
three B33B probe points. It does not yet name the bit or exclude an earlier
action/context side effect through `1B77 -> 393C/19E6`.

A longer unforced W1L3 sample resolves the ordinary lifetime of the linked
B25D record. It remains callback `B25D` through sample `127` (the owner is
`B33B`, `(350,565)`, with `+0x38 = 82`), then is absent at sample `128` in the
same observation in which the B33B owner is absent. The linked B226 record
remains active, and no B25D record is reactivated during the remaining 1,872
sampled frames. Thus the old `+0x36` record is reclaimed as part of the natural
B33B owner teardown rather than by an independently observed B25D terminal
callback; the exact clear ordering still needs an entry/return trace.

### Controlled late-phase boundary

The same lightweight sampler, forcing only the initial global phase to `2` and
then running ordinary frames, reaches the later phases without callback
breakpoints:

- global phase `3` begins within the first five sampled game frames;
- phase `3` reaches phase `4` at approximately game frame `420`, when the
  owner emission counter `+0x44` becomes `0x10`;
- phase `4` increments `+0x38` and moves the owner upward. At game frame
  `509`, the linked B226 record is still active at `y=330`, but B33B's strict
  gate (`y-camera+0x10`) rejects it because the unsigned result is above
  `0xD0`; its callback is cleared and the record is reused by `10B5` on the
  next frame;
- global phase `5` is reached at approximately frame `512`. The owner passes
  through initializer callback `487F`, then steady callback `489C` with slot
  `0x02C7`. The old B25D record at `+0x36` remains active through the sampled
  frame `650`, so final-phase owner conversion does not itself reclaim that
  linked record.

This closes the natural timing and deactivation edge for the controlled phase
sequence and the ordinary B25D lifetime. The remaining action/context path and
the exact teardown ordering remain open.

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

1. Associate the natural B226/B25D sequence-table entries with gameplay
   frames, and trace the terminal callback-clear ordering at the coupled B33B
   teardown boundary.
2. Trace the natural `B33B` action/context path (`1B77 -> 393C/19E6`) and
   deliberately capture a run in which one of the `1C6E` probes returns raw
   bit `0x4000`; the frame-445 sample shows that this bit is not the direct
   cause of the timer-driven handoff.
3. Trace the linked records at `+0x2A` and `+0x36`, including `B84D`, `B84C`,
   and `0x487F`, plus the general deactivation/scheduler sites, so the coupled
   teardown and any later object reactivation are not guessed.
4. Resolve the remaining full-frame palette/timing residuals after the
   queue-owned records are reproduced.
5. Once those pass, implement a queue-owned render pass in C++ and compare
   matched frames. The current frontend order remains provisional.
