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
- ARE-backed level streaming with provisional collectible, hazard, and exit events;
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
  common-tail timer/global writes, and the known pending jump sound dispatch;
- complete raw `0x78` pre/post records and ordered trace publication.

Dynamically parity-validated:

- the existing horizontal/free-space formula fixtures;
- the C++ callback's synthetic jump, grounded, and blocked-ascent branch
  vectors;
- candidate-to-candidate full-record/probe/global/effect comparison through
  `player_parity_compare.py`.

These checks do not yet constitute full DOS callback parity. The first
captured-DOS replay set is intentionally reported as a diagnostic until its
pre-state globals and descriptor observations are complete.

Explicit unresolved boundaries:

- natural `648E/6484` contact behavior and the `0E06` contact-object family;
- the direct gameplay contract of `5937` when the `DS:89EA` transition gate is
  active;
- animation table reads at `5D38/5D60` and presentation-only callbacks;
- natural ceiling validation, descriptor semantic labels/one-way policy, and
  moving-platform scheduler/culling behavior;
- the no-descriptor-table fallback, which remains a deliberate research
  boundary.

The repeatable callback replay workflow is:

```sh
python3 research/tools/player_callback_parity.py \
  --original research/build/player-followup-standing-v1.json \
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
static landing, ceiling-response, and jump writes, while natural ceiling
parity, descriptor class names, one-way behavior, and moving-platform contact
remain explicit research boundaries until targeted runtime traces close them.

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
