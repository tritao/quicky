# Entity behavior families

This note is the interpretation guide for
[`entity-behavior-families.json`](../entity-behavior-families.json). The matrix
is deliberately conservative: a known sprite slot, BOB record, or initializer
address is not treated as proof of movement, collision, or death behavior.
`confirmed` means that the cited static/runtime evidence directly establishes
the claim; `partial` means the entry is a useful boundary but a required part
of the behavior is still missing; `unresolved` means that no durable evidence
currently establishes it.

## Inventory and family boundaries

The archive catalog contains 50 used types grouped into 21 families. The
normal enemy pairs are separate catalog types but share a sprite family; they
remain separate rows because their initializers and update callbacks differ.

| Family | Types | Used worlds | Current strongest identity evidence |
| --- | --- | --- | --- |
| WURM2 | 01/02 | W1 | callbacks 6DA3/6DB1, slots 281/231 |
| BIENE | 03/04 | W1 | callbacks 689F/68AD, slots 276/226 |
| FISCH | 05/06 | W2 | callbacks 7B50/7B5E, slots 254/204 |
| KRABBE | 07/08 | W2 | callbacks 776B/7779, slots 200/250 |
| PENGO | 09/0A | W3 | callbacks 713D/714B, slots 250/200 |
| SCHNEE | 0B/0C | W3 | callbacks 6651/6699, shared slot 209 |
| FLIEGE | 15/16 | W4 | callbacks 7ED7/7EE5, shared slot 208 |
| SPINNE | 17/18 | W4 | callbacks 8451/845F, slots 250/200 |
| BUGGY | 19/1A | W5 | callbacks 5050/505E, slots 250/200 |
| UFO enemy | 1B/1C | W5 | callbacks 5F07/5F15, slots 264/214 |
| animated tile effect | 1F/20/21 | W1-W5 | common 8E4B state machine and world ICO tables |
| cloud | 28 | W1-W5 | WOLKE.BOB special renderer, slots 413-416 |
| falling leaves | 29/2A/2B | W1 | common 4727 path and BLATT.BOB tables |
| paper | 2C | W1-W5 | callback 8C4E, slot 710/PAPIER.BOB |
| wind | 33 | W4 | callback 87D1, slot 214/WIND.BOB |
| bump | 34 | W1-W5 | callback 9BEE, slot 400/BUMP_Wn.BOB |
| UFO effect | 35/36 | W5 | callbacks 544C/545A, slots 264/214/UFO.BOB |
| moving platform | 3D/3E/3F/40 | W1-W5 | common 9DC7 path, slots 300/301/PLATFWn.BOB |
| dedicated event | 65/66/67 | W1-W5 | subtype wrappers and LOOP_Wn.ICO child path |
| pickup | 6F/70/71/72 | W1-W5 | slots 607-610/WERBE.BOB |
| puzzle letter | 79-7F | W1-W5 | slots 600-606/PUZZLE.BOB |

Types `0x73` and `0x74` are present in the confirmed dispatch survey but have
zero archive occurrences and zero dispatch entries. They are intentionally not
counted as used families.

## Confirmed state machines

### Animated tile effects (`0x1F`-`0x21`)

The three initializers differ only in `object+0x2E` (1, 2, 3) and converge on
`01F7:8E4B`; the normal sprite slot remains `0xFFFF`. The callback first gates
the object against the camera and the persistent player bounds object. Once
accepted it advances through states 1, 4, 6, 8, and 10. At states 4, 6, 8,
and 10 it performs five 16-pixel MAP lookups through `01F7:3376`, maps the
low-nine-bit tile IDs through `DS:6986`, and calls `01F7:16CE` for each nonzero
effect entry. State 10 clears `object+0x18` and publishes terminal coordinates
at `DS:8828/882A`.

The children are three-tick pooled objects with callback `01F7:10B5`.
`10B5 -> 1693 -> 1186` applies the visibility test and reads one 256-byte
record from the world ICO resource. Exact byte comparisons exist for W1
through W5 in [`effect-mappings.json`](../effect-mappings.json). This family is
the current reference implementation for all seven matrix dimensions.

### Dedicated events (`0x65`-`0x67`)

The three wrappers set subtype bytes `0x00`, `0x08`, and `0x10` before entering
the common creator at `01F7:1749`. The event ring stores a fixed-point position,
the source ARE record word, a PRNG-derived animation byte, and the subtype.
The event loop advances the animation byte modulo eight and calls `16CE`, which
allocates a pooled visual object with no normal BOB slot and callback `10B5`.
The callback rejects objects outside the camera rectangle, then draws from
`LOOP_Wn.ICO`. Live 256-byte blocks match the corresponding W1-W5 ICO records
exactly. These objects are visual-only and short-lived; no player collision or
enemy AI is on this path.

## Partially decoded families

The normal enemy families all use the common factory `01F7:0E06`, object class
1, and a family-specific callback that writes a logical sprite slot at
`object+0x12`. The callback and post-initializer boundaries are known, and
target-vs-inert ARE mutations prove that the seeded object is removable. The
near-camera callback pass below now establishes first-order direction,
fixed-point movement, and shared helper entry points for every pair. Complete
patrol/pursuit naming, exact collision rectangles, and gameplay death paths
remain intentionally conservative.

The first paired WURM2 run is now live-verified with the shared main-tree
debugger build. A redirected W1L1 anchor produced type `0x01` through dispatch
`01F7:6DA3`, post-initializer `01F7:6DB0`, slot 281, then object callback
`01F7:6DC4`; its first callback sample cleared `object+0x18`. The same anchor
mutated to type `0x02` produced dispatch `01F7:6DB1`, post-initializer
`01F7:6DC3`, slot 231, the same `01F7:6DC4` callback, and the same first-update
clear. Both paired type-0 fixtures reached the inert capture barrier without
allocating an object. These are removal/deactivation observations, not proof of
the gameplay death path; movement and collision remain open.

The paired FISCH run is likewise live-verified against the main-tree debugger
fixtures. Type `0x05` at the W1L1 anchor dispatches through `01F7:7B50`, returns
from its initializer at `01F7:7B5D`, installs slot 254, and reaches per-object
callback `01F7:7B71`; type `0x06` uses dispatch `01F7:7B5E`, post-initializer
`01F7:7B70`, slot 204, and the same `01F7:7B71` callback. The first callback
sample for each variant clears `object+0x18`, while the paired inert fixtures
reach the capture barrier without allocating an object. Existing native W2L1
selector evidence independently resolves FISCH.BOB geometry as 40x19 with
origin 20,19 (records 3 and 2 for the two variants). Movement, collision, and
the true gameplay death path remain open.

The corrected selector-mode tracer then sampled every remaining normal-enemy
pair in a common near-camera W1L2 fixture (the type-`0x34` probe archive,
record `0x1606`, with the target type substituted at declaration time). This
keeps the initializer/callback comparison controlled even though the native
sprite resources belong to other worlds. The observed callback and first-order
motion are:

| family | live callback | 32-tick observation | state/counter evidence |
| --- | --- | --- | --- |
| WURM2 `01/02` | `6DC4` | left `240->226` / right `240->253` | state 0; `+20` countdown and `+2A` phase advance |
| BIENE `03/04` | `68C0` | left `240->198` / right `240->282` | state 0; `+20` counts 14 to 0 and `+2A` advances |
| FISCH `05/06` | `7B71` | left/right with y `384->387` | states 1, 2, 3 observed; phase-driven vertical component |
| KRABBE `07/08` | `778C` | left `240->237` / right `240->242` | state 0; countdown/phase bytes advance |
| PENGO `09/0A` | `715E` | no x displacement; y `400->401` | animation/state cursor at `+32` advances in `0x100` steps |
| SCHNEE `0B/0C` | `66E1 -> 6757` | left/right with y `400->420` | state 0; shared callback changes after the initial path |
| FLIEGE `15/16` | `7EF8` | left `240->194` / right `240->286` | state 0; countdown/phase bytes advance |
| SPINNE `17/18` | `8472` | left `240->230` / right `240->249` | state 0; countdown/phase bytes advance |
| BUGGY `19/1A` | `5071` | left `240->226` / right `240->253` | state 0; countdown/phase bytes advance |
| UFO enemy `1B/1C` | `5F28` | left `240->198` / right `240->282` | state 0; countdown/phase bytes advance |

All ten callbacks remain active for the 32 samples, so these are movement
observations rather than off-camera deactivation artifacts. The paired
type-`0` controls allocate no object. They establish initial direction,
fixed-point integration, and the timer/state bytes, but do not by themselves
prove pursuit, player damage, or the gameplay death path. Type `0x08` is a
known resource-context caveat: the synthetic W1L2 run observed logical slot
200, while the native W2 selector/resource run resolves the catalog companion
as slot 250; both use the same callback and KRABBE geometry.

Static callback inspection fills in the shared interaction boundary. Each
normal enemy first calls visibility gate `01F7:1DCA`; rejection enters
`01F7:1DEE`, which clears `object+0x18` and the selector's active byte. The
movement callbacks then call `01F7:1B77` for object/player bounds overlap and
`01F7:1C4D`/`01F7:1C6E` for directional MAP collision (descriptor bit
`0x400`), recording a blocking result in object byte `+0x2F`. The BIENE and
FISCH disassemblies additionally show direction-dependent velocity clamps and
phase/timer transitions; the other families now have runtime direction and
counter evidence but still need a complete state-table decode. Rendered BOB
width/height/origin values remain sprite geometry, not proof of the gameplay
collision rectangle.

The falling-leaf family is further along: `01F7:5D38` reads one of two PRNG
selected tables (`DS:3312` delay 8 and `DS:3326` delay 10), uses slots 700-707,
and switches to the bright 750-757 row when `object+0x28 == 0xFF`. The seeded
object produces pooled child leaves, but the continuous trajectory, cadence,
collision, and recycling rules are not yet fully captured.

The remaining effect-family uncertainty is no longer object identity: all of
these callbacks and resource bindings are now bounded below. The open parts
are exact gameplay names for global effects, collision response details, and
level persistence/respawn semantics. Platforms retain a compatibility-only
one-way-floor hypothesis until an original-runtime player-carry trace confirms
it.

The shared effect/collectible callbacks are now statically bounded as well.
Cloud `0x28` initializes callback `9269`, leaves the logical slot at `FFFF`,
does not integrate position, and only performs the camera/player-range test
that updates `DS:89E6`. Paper `0x2C` initializes slot 710 and callback `8D20`
with subtype 5. The same `8D20` callback is used by pickups and puzzle letters:
it checks an x range extended by 16 pixels and a 16-pixel-aligned y range,
then dispatches `+0x2C` (pickup subtype) or ORs the letter bit in `+0x2A`
into `DS:60D8` before clearing `object+0x18`. This establishes fixed placement,
overlap geometry, and callback removal for those families; inventory/score
meaning, level completion, persistence, and respawn remain separate questions.

Wind `0x33` and UFO effects `0x35/0x36` are moving callback families rather
than inert visuals. Their initializers load animation tables, seed signed
fixed-point horizontal velocity, and converge on callbacks `882F` and `546D`;
both call `1DCA -> 1DEE`, `1B77`, `1C4D/5C27`, clamp velocity, and flip
direction on timer or map blocks. Bump `0x34` is the contrasting stationary
hazard: `9BEE` shifts the position and installs `9C0C`, whose player-range
branch triggers the hazard state at `DS:612E=4`; its off-camera path also uses
`1DCA -> 1DEE`. Moving platforms `0x3D`-`0x40` converge on `9DC7`, integrate
horizontal or vertical fixed-point velocity, snap to 16-pixel grid cells on
MAP blocks, and carry player-relative offsets through `A0B2`; their offscreen
branch reaches `1DEE`. These static facts close initializer, movement, and
deactivation boundaries while preserving the unresolved gameplay response
names in the matrix.

## Cross-world rules

World-specific resource selection is a separate axis from logical sprite slot:
the same slot can resolve to different BOB records depending on the resource
batch. Confirmed examples include slot 214 (`WIND.BOB` in W4 versus `UFO.BOB`
in W5), slot 200/250 across enemy families, and slot 400 for the
`BUMP_W1.BOB`-through-`BUMP_W5.BOB` family. The matrix therefore records both
the logical slot and the resource context. A cross-world claim is complete only
when the live descriptor geometry or exact ICO block has been matched in that
world; filename ordering alone remains partial evidence.

## Reproducible next experiments

The isolated object tracer now lives in
[`object_behavior_trace.py`](../tools/object_behavior_trace.py) and
[`quiky_object_behavior_trace.lua`](../automation/quiky_object_behavior_trace.lua).
It captures the object bytes before and after each live per-object callback,
the callback return site, changed offsets, camera globals, and the initial and
post-initializer object records. `--sprite-init-offset` identifies the
post-initializer boundary; this direct callback mode avoids obsolete scheduler
entry offsets and handles one-shot normal objects.

Run a representative trace from the repository root after starting the
debug-enabled automation build:

```text
python3 research/tools/object_behavior_trace.py \
  --launch --headless --entity-type 0x01 \
  --record-offset 0x1792 --samples 32 \
  --output research/build/traces/entity-01-behavior.json
```

When this worktree has no local automation build, the compatible binary can be
shared from the sibling main worktree. Set all three paths so the launcher
uses the main build directory and dependency prefix:

```sh
DOSBOX_AUTOMATION_BIN=/home/joao/dev/quicky/research/build/dosbox-automation-debug/dosbox_with_debugger \
DOSBOX_AUTOMATION_BUILD_DIR=/home/joao/dev/quicky/research/build \
DOSBOX_AUTOMATION_DEPS_PREFIX=/home/joao/dev/quicky/research/build/sdl-prefix \
python3 research/tools/quikytrace.py --launch --headless \
  --runtime-dir /tmp/entity-01-variant/baseline/game \
  --select-level W1L1 --entity-record-offset 0x177a --entity-type 0x01 \
  --sprite-init-offset 0x6db0 --output /tmp/entity-01-generic-init.json
```

This invocation was verified on the current executable/archive pair. It
captured type `0x01` at world position `(816,240)`, dispatch callback
`01F7:6DA3`, common factory `01F7:0E06`, post-initializer slot `281`, and the
live descriptor geometry `52x16` with origin `(26,16)`. The selector path is
important: the default menu replay can time out before the first resource
lookup even though the shared debugger binary is healthy.

For a level-specific representative, add `--select-level W2L1` and use the
record offset from `quikyctl.py entity-catalog`. Use paired traces for each
family variant, then run a one-record target-vs-inert mutation with
`research/tools/quikyentity.py` to separate object removal from a visual
change. Keep the generated traces under ignored `research/build/`; only
normalized conclusions and hashes belong in tracked notes/catalogs.

The broad family pass is complete. The next experiments should target only
the remaining partial claims, in this order:

1. Patch the player/bounds object and capture a controlled overlap for one
   pickup subtype and one puzzle letter. Record the global fields changed by
   `8D20`, then repeat for paper subtype 5.
2. Hold the camera on a wind/UFO-effect pair and a moving platform long enough
   to catch timer reversal, MAP blocking, and player-carry branches. This turns
   the static velocity bounds into named trajectories and confirms the platform
   one-way response.
3. Run the same visibility/deactivation probe across W1-W5 cloud, paper, and
   BUMP resources, and compare descriptor geometry plus resource filenames.
4. Extend the falling-leaf pool trace until a child recycles, then test whether
   the W1-only family has any hidden world-specific resource branch.
5. For normal enemies, repeat a long run through a MAP block and a player
   overlap to separate patrol reversal, damage, and death from the already
   confirmed off-camera `1DEE` removal path.

Each new trace should update the corresponding seven dimension statuses in the
JSON matrix and add a durable reference to this note. Do not promote a
dimension to `confirmed` solely because a callback or sprite slot is known.
