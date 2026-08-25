# Quiky C++ engine core

This is the first standalone C++ slice of the Quiky engine recreation. It is
kept independent of SDL and ScummVM so the format and renderer code can be
tested independently before a ScummVM frontend is added.

The current iteration supports:

- checked binary reads with explicit endianness;
- direct reading of the `NESTLE.DAT` archive;
- `MAP` decoding;
- world `PCC` palette extraction;
- column-interleaved `ICO` tile decoding;
- indexed level rendering to an 8-bit BMP;
- ARE layout/entity parsing and optional debug overlays;
- BOB sprite decoding and indexed contact sheets;
- a deterministic fixed-step player simulation centered on the recovered
  `0x78`-byte record;
- recovered world-specific descriptor tables linked to streamed MAP cells;
- an explicit MAP-cell-to-descriptor query bridge using confirmed quadrant rules;
- an SDL3 interactive W1L1 frontend with fixed-step input and camera scrolling;
- a shared SDL audio mixer that combines gameplay music with confirmed pickup SFX;
- ARE-backed level streaming with the scoped W1L1 object-family inventory;
  the shared collectible callback `01F7:8D20`, W1 normal-enemy callbacks
  `01F7:6DC4/68C0`, and animated world-ICO callback `01F7:8E4B` are now
  executed through the scheduler, while other transition contracts remain
  explicit at their recovered boundaries;

The default W1L1 session now starts the player at the native traced
initializer position `(128,400)`. Explicit start-coordinate overrides remain
available for fixtures and controlled experiments; levels without a closed
player declaration retain the conservative fallback.
- source-less high-effect rendering from the recovered `4B70 -> 4C74` chain:
  world-specific `PUFF.BOB`/`PUFFW2.BOB`, slots `611/612/613`, and the
  31-update terminal lifecycle;
- DOSBox-equivalent runtime VGA palette output while preserving raw PCC
  values for asset inspection;
- a deterministic 320x200 `quiky-frame` scene probe for matching DOSBox
  screenshots with explicit camera, input, and high-effect controls;
- synthetic unit tests for the format readers.

## Player foundation boundary

The deterministic shell is now the canonical runtime boundary. Its mutable
ownership is explicit:
`SimulationState` owns the tick counter, recovered player record, scheduler,
object-pool records, and queued events. `WorldCollisionView` borrows decoded
MAP and descriptor data read-only. `Simulation::tick(input, world, output)` is
the one fixed-step boundary; it advances scheduling and drains events into a
`SimulationOutput` snapshot. Rendering reads that snapshot, and audio reads
its emitted audio events instead of driving gameplay.

The foundation currently includes:

- `Fixed16`, the centralized signed 16.16 representation with explicit
  arithmetic shifts, wrapping, clamping, conversion, and multiplication;
- `PlayerRecord`, a typed projection of the recovered `0x78`-byte record, with
  stable `fieldXX` names and lossless raw-record conversion;
- `WorldCollisionView`, which preserves raw MAP words, tile IDs, cell flags,
  descriptor words, runtime flags, and out-of-bounds status. It exposes only
  the confirmed descriptor predicates, not a guessed `isSolid()` rule;
- `ObjectScheduler`, which applies deferred spawn/release mutations, walks
  stable slot order, and records callback identity and invocation order;
- `quiky-trace-v2`, a normalized trace interchange containing the raw player,
  MAP lookups, scheduler callbacks, state writes, and emitted events. The
  `quiky-trace-compare` tool reports the first divergent tick and field.

The active frontend and scene tools consume `SimulationOutput` and
`PlayerRecord`; they no longer have a second player-state or collision API.
`player_update.h` keeps the recovered callback stages and unresolved external
boundaries explicit. The runtime does not silently fall back to guessed
grounding or jump behavior.

## W1L1 scheduler status

The first gameplay-object slice is the recovered shared collectible callback
`01F7:8D20`. ARE records for ammo, health, invulnerability, lives, and puzzle
letters publish their address-qualified callback identity through
`ObjectScheduler` in stable slot order. On overlap, the level session records
the exact subtype-driven player/global writes (`DS:880A`, `DS:880C`,
`DS:8810`, `DS:881C`, `DS:8822`, `DS:8824`, `DS:60D8`, and `DS:612E`) in the
collected event and clears the scheduled object. The invulnerability path also
writes the recovered player-record timer at `+0x34`; subsequent countdown and
gate clearing remain owned by the player callback.

This slice is statically closed for callback identity and state effects and is
covered by native contract tests. The overlap predicate remains the explicit
`8D31 -> 393C` boundary until its exact fixed-point geometry is promoted from
the DOS trace evidence. Natural puzzle completion/transition behavior,
the remaining enemy vertical/PRNG branches, and script-created platforms are
not implemented by this slice.

The W1 normal-enemy slice now executes the closed initialization and state-zero
patrol behavior for WURM2 (`6DC4`) and BIENE (`68C0`): fixed-point direction,
the `+0x20` Y initializer offset, raw MAP `0x4000` probing through the
`1C6E` contract, animation-delay countdown, and shared off-camera removal.
Their `1C4D` object-helper polarity and vertical/PRNG phases remain
address-qualified. Player contact records the timed `4AB3/4C5D` response and
does not invoke the provisional player-death path.

The W1 animated-tile slice executes the shared `01F7:8E4B` closure for ARE
types `0x1F`-`0x21`. It preserves the initializer selector at `object+0x2E`,
advances the callback state at `object+0x32`, performs the exact five-cell
MAP/`DS:6986` probe order at states `4/6/8/10`, and creates three-tick ICO
children through the recovered `16CE -> 10B5` contract. State 10 clears the
parent and publishes the address-qualified terminal coordinates
`DS:8828/DS:882A`; the indexed `01F7:1AAA` transition consumer and authored
selector population remain explicit unresolved boundaries.

The remaining W1L1 authored visual callbacks now publish their closed static
contracts without being folded into generic gameplay behavior:

- Cloud `01F7:9256 -> 9269` is classified as an ambient visual, keeps its
  normal logical slot at `0xFFFF`, and latches `DS:89E6` only when the
  normalized 16-pixel player gate, `DS:89EA == 0`, and player `+0x37 == 0`
  all pass. The outer transition consumer and special `WOLKE.BOB` blitter are
  still address-qualified.
- Leaf types `0x29`-`0x2B` publish the `01F7:47E7` callback, `BLATT.BOB`
  slot families, the `0x13000` fixed-point base velocity, source position,
  timer `0x0C`, and the confirmed `DS:3312`/`DS:3326` animation-table
  selectors. The signed PRNG perturbation and pooled-child emission cadence
  remain unresolved rather than being replaced by guessed motion.
- Dedicated types `0x65`-`0x67` publish subtype bytes `0x00/0x08/0x10`,
  world-relative `LOOP_Wn.ICO` records, no normal BOB slot, and child callback
  `01F7:10B5`. The event-ring seed, exact child selector/lifetime, and human
  semantic names remain explicit external data.

The current-main native goal fixture exercises all seven authored W1L1 letter
callbacks and reaches `DS:60D8=0x007F` through their normal updates. A
separate adjacent-cell fixture also reaches the accepted cloud callback at
`01F7:92A9` and publishes `DS:89E6=0xFFFF`; the cloud writer is observed
before the native letter mask becomes complete. The immediate outer path then
reaches `01D7:4EA0/4EAA` with the complete mask and cloud latch. These are
dynamic validations of letter accumulation and the outer gate only. The
completion presentation and downstream resource reload remain explicitly
unresolved; no completion or level-exit behavior is inferred from them.

The moving-platform slice now executes the static `01F7:9DC7` / `A075` /
`A0B2` publication contract for ARE types `0x3D`-`0x40`. It runs before the
player callback, applies the strict horizontal interval and 12-pixel contact
band, publishes `DS:5006`, `DS:8816`, and `DS:8812`, and consumes the carry in
the same callback while preserving the packed `0x78` record's subpixel state.
`SimulationOutput::playerDependencyOrder` records the platform callback and
the `01F7:3FF8` player callback as an ordered pair; the ordinary scheduler
invocation list remains separate because the player update is not a pooled
object callback in this native boundary.
The confirmed `5DC3` raw-MAP `0x0800` stop/snap path and its `0x46` wait value
are covered by a native contract test. Full DOS scheduler provenance,
platform landing/jump detachment, and crushing remain explicit boundaries.
The statically confirmed `A06F -> 1DEE` cull/re-stream lifecycle is modeled:
platform carry/wait state is discarded at release and the ARE anchor and
initializer state are restored on re-entry. The effect of that release on an
attached player remains an explicit runtime boundary.

## Unified player callback status

The unified `TraceClosedPlayerUpdate` is the C++ implementation of the
statically recovered `01F7:3FF8–44FE` callback path. Its branch order and
field writes are audited against
`research/notes/player-static-decomp.cpp` and
`research/ghidra/player-callback-focused-audit.json`.

Implemented from static evidence:

- callback entry, input normalization, action-counter updates, and the
  `+0x39`, `DS:89EA`, `DS:89E6`, and `DS:8812` gates;
- ordinary, positive, and negative mode dispatch;
- signed 16.16 acceleration, release clamp, gravity, apex, and terminal
  velocity arithmetic;
- ordered side/vertical probes, `3D02` descriptor correction, and `3DF2`
  alignment;
- jump initiation, grounded response, blocked ascent, landing-state writes,
  common-tail timer/global writes, the known pending jump sound dispatch, and
  the resident player animation loads/advancement at `5D38/5D60` for the
  recovered callback sequences;
- complete raw `0x78` pre/post records and ordered trace publication.

Dynamically parity-validated:

- the existing horizontal/free-space formula fixtures;
- the C++ callback's synthetic jump, grounded, and blocked-ascent branch
  vectors;
- the standing DOS replay's complete `0x78` post-records and observed global
  mutations across all eight callbacks;
- a player-scoped dense `5D60` trace through natural landing and the following
  ordinary callbacks, confirming the `3156` idle reload before `5D60` and
  exact post-record parity through that animation boundary;
- candidate-to-candidate full-record/probe/global/effect comparison through
  `player_parity_compare.py`.

The captured-DOS replay set is mixed: older standing and late-release fixtures
remain diagnostic when their source traces omit native probe arrays or opaque
helper outputs, while the complete W1L1 jump/property window is now a closed
fixture. It compares all ten callback records, normalized input words, ordered
`1C6E/1C92/5C27/5CC3` property probes, callback-global writes, and known
effects exactly.

The committed fixture and its candidate are under
`research/evidence/player-dos-parity/`. Re-run the live replay with:

```sh
python3 research/tools/player_callback_parity.py \
  --original research/evidence/player-dos-parity/w1l1-jump-property-v3.json \
  --archive game/NESTLE.DAT --map W1L1.MAP \
  --binary build/engine/quiky-player-trace
```

Explicit unresolved boundaries:

- full natural `648E/6484` contact-effect creation and any gameplay feedback
  from the `0E06` object family;
- the ordinary-level transition behavior of `5937` beyond its recovered direct
  read/write and return contract;
- natural ceiling validation and descriptor semantic labels/one-way policy;
- moving-platform scheduler provenance beyond the pre-player publication,
  landing/jump detachment, crushing, and the attached-player effect of
  culling;
- the no-descriptor-table fallback, which remains a deliberate research
  boundary.

The repeatable callback replay workflow is:

```sh
python3 research/tools/player_callback_parity.py \
  --original research/evidence/player-dos-parity/w1l1-jump-property-v3.json \
  --archive game/NESTLE.DAT --map W1L1.MAP \
  --binary build/engine/quiky-player-trace
```

It materializes a pre-state replay manifest, runs `quiky-player-trace`, and
invokes the fail-closed comparator. Missing records, probes, globals, or
effect/factory data are reported as mismatches rather than skipped.

## Horizontal player closure

The confirmed horizontal callback slice is integrated behind the explicit
player-updater seam. `PlayerRecord`
preserves all `0x78` bytes and exposes the static-closure fields, including
the overlapping pixel-word views and confirmed horizontal constants. The
trace-closed updater integrates old X velocity first, applies
right-before-left precedence, uses `0x2800` acceleration, `0x2000` release
friction, and the `+/-0x18000` cap through `Fixed16` helpers. For records
already in an airborne mode, it also applies the confirmed free-space vertical
branch, including release clamping, apex transition, gravity, and terminal
velocity.

`CollisionKernel` is a pure MAP/descriptor query layer for the recovered
quadrant mask, ordered `x-5`/`x+5` probes, `3DF2` Y alignment, and `3D02`
descriptor response. `TraceClosedPlayerUpdate` composes those helpers with
the statically recovered floor/ceiling/grounded branch writes. The separate
`updatePlayerVerticalFreeSpace` API remains a leaf for callers that already
have clear probes; it is not the unified callback.

The compact `tests/fixtures/player-horizontal-v1.tsv` contains the 1,261
formula samples represented by all 5,044 values checked by the Python model.
Regenerate it from the research ledgers with:

```sh
python3 engine/tools/generate_horizontal_fixture.py \
  research/evidence/player-horizontal \
  engine/tests/fixtures/player-horizontal-v1.tsv
```

The `quiky-horizontal-tests` target checks record round trips, all horizontal
held-out values, direct acceleration/reversal/cap vectors, Python collision
kernel parity vectors, snapshot isolation, and trace diagnostics. The
`quiky-vertical-tests` target checks the 15 trace-closed free-space rows and
the canonical updater integration. `quiky-player-callback-tests` checks the
focused jump, grounded, and blocked-ascent records and ordered probes.

## Runtime descriptor integration

`playerDescriptorTableForWorld()` reproduces the five world initializer
tables and `LevelRuntime` supplies the selected table alongside streamed MAP
cells. Descriptor-backed horizontal checks use the recovered forward probes
at `(x +/- 10, y - 1/-17/-33)`. Table boundaries, special descriptor values,
and probe coordinates have direct engine tests.

No compatibility player simulation remains. The C++ callback contains the
static landing, ceiling-response, jump, and recovered animation writes, while
natural ceiling parity, descriptor class names, one-way behavior, and
moving-platform contact remain explicit research boundaries until targeted
runtime traces close them.

Music playback is an optional subsystem because the bundled `.TFX`/`.SAM`
resources use TFMX, which needs a dedicated decoder. The engine includes an
in-tree four-voice TFMX Pro parser, sequencer, Paula-style mixer, and gameplay
SFX scheduler. It reads paired resources directly from `NESTLE.DAT`, renders
WAV, and can play through SDL3 when SDL3 is discoverable by CMake. No external
audio runtime is required.

Build it with the normal engine configuration:

```sh
cmake -S engine -B build/engine -DSDL3_DIR=/path/to/SDL3/cmake/config
cmake --build build/engine
build/engine/quiky-music game/NESTLE.DAT list
build/engine/quiky-music game/NESTLE.DAT render TITEL /tmp/TITEL.wav
build/engine/quiky-music game/NESTLE.DAT play TITEL
build/engine/quiky-sfx game/NESTLE.DAT list ONGAME2
build/engine/quiky-sfx game/NESTLE.DAT render ONGAME2 0 /tmp/sfx-0.wav
build/engine/quiky-sfx game/NESTLE.DAT play ONGAME2 0
```

`quiky-sfx` exposes all fourteen gameplay effect IDs. The `SfxModule` API in
`include/quiky/sfx.h` keeps TFX/SAM loading, four-voice priority scheduling,
macro interpretation, and PCM output separate. A frontend can call `trigger`,
advance the 50 Hz voice state, and mix PCM blocks, or use the deterministic
`render` helper for one-shot diagnostics. `quiky-play` now uses the shared
`AudioMixer` and triggers the confirmed pickup mappings, including the
puzzle-letter state-0 branch (ID 11). The event mapper exposes semantic
`GameplayCollectible` and `GameplaySfx` names in `include/quiky/sfx_events.h`,
so the frontend does not depend on raw entity-type or effect-slot literals.
Jump, player death/respawn, entity impact, tile interaction, alternate action,
and world-object events now use their confirmed named SFX roles. Exit events
and unresolved effect-table IDs remain silent until their dispatch semantics
are proven.

For short sequencer diagnostics, set `QUIKY_TFMX_TRACE` to the number of
50 Hz ticks to print before rendering:

```sh
QUIKY_TFMX_TRACE=50 build/engine/quiky-music game/NESTLE.DAT render \
  TITEL /tmp/TITEL-trace.wav 2>/tmp/TITEL-trace.log
```

Build and test from the repository root:

```sh
cmake -S engine -B build/engine
cmake --build build/engine
ctest --test-dir build/engine --output-on-failure
```

Launch the default W1L1 session without naming the derived world resources:

```sh
build/engine/quiky-play game/NESTLE.DAT
```

Inspect the bundled archive:

```sh
build/engine/quiky-inspect game/NESTLE.DAT info W1L1.MAP
build/engine/quiky-render game/NESTLE.DAT W1L1.MAP /tmp/W1L1.bmp
build/engine/quiky-render game/NESTLE.DAT W1L1.MAP /tmp/W1L1-entities.bmp --overlay-are
build/engine/quiky-bob-sheet game/NESTLE.DAT QUIKYW1.BOB W1.PCC /tmp/QUIKYW1.bmp
build/engine/quiky-simulate game/NESTLE.DAT W1L1.MAP 100 100 120 0x04
```

For a controlled DOSBox scene comparison, `quiky-frame` writes the same
320x200 indexed frame size used by the automation screenshots. The high
effect probe below places the recovered effect at world `(300,510)`, advances
its cursor to 5, and fixes the camera to the captured DOSBox position:

```sh
build/engine/quiky-frame game/NESTLE.DAT W1L3.MAP /tmp/W1L3-frame.bmp \
  --frames 5 --high-effect 300 500 --camera-x 0 --camera-y 358 \
  --no-player --no-entities --high-effect-only
PYTHONPATH=research/tools python3 research/tools/scene_frame_compare.py \
  /tmp/W1L3-frame.bmp research/build/high-effect/w1l3-visual-forced-5-v2-raw.png \
  --region 280 110 40 50
```

The probe can include the player and active ARE objects for scene-level
comparisons; `--no-player`, `--no-entities`, and `--high-effect-only` isolate
the recovered effect from the player, ARE sprites, and ARE transient queue.

The renderer output is an indexed BMP using the palette stored in the matching
world PCC resource. The C++ W1L1 output matches the existing Python renderer
pixel-for-pixel when ARE overlays are disabled. The ARE overlay is a debug
visualization; it does not assign gameplay meanings to entity types.
