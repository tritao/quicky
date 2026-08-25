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
- deterministic player simulation with provisional MAP collision masks;
- standalone descriptor/quadrant rules recovered from the DOSBox player probes
  (kept separate until streamed MAP cells are linked to descriptor entries);
- an explicit MAP-cell-to-descriptor-table query bridge for those rules;
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

The `engine/player-foundation` branch establishes the deterministic shell for
the faithful player implementation. Its mutable ownership is explicit:
`SimulationState` owns the tick counter, recovered player record, scheduler,
object-pool records, and queued events. `WorldCollisionView` borrows decoded
MAP and descriptor data read-only. `Simulation::tick(input, world, output)` is
the one fixed-step boundary; it advances scheduling and drains events into a
`SimulationOutput` snapshot. Rendering reads that snapshot, and audio reads
its emitted audio events instead of driving gameplay.

The foundation currently includes:

- `Fixed16`, the centralized signed 16.16 representation with explicit
  arithmetic shifts, wrapping, clamping, conversion, and multiplication;
- `RecoveredPlayerState`, a typed projection of the recovered `0x78`-byte
  record, with stable `fieldXX` names and lossless raw-record conversion;
- `WorldCollisionView`, which preserves raw MAP words, tile IDs, cell flags,
  descriptor words, runtime flags, and out-of-bounds status. It exposes only
  the confirmed descriptor predicates, not a guessed `isSolid()` rule;
- `ObjectScheduler`, which applies deferred spawn/release mutations, walks
  stable slot order, and records callback identity and invocation order;
- `quiky-trace-v1`, a normalized trace interchange containing the raw player,
  MAP lookups, scheduler callbacks, state writes, and emitted events. The
  `quiky-trace-compare` tool reports the first divergent tick and field.

The existing `PlayerSimulation` and SDL frontend remain compatibility code and
are still provisional. No movement, collision, grounded, facing, or transition
formula is promoted by this shell. `player_update.h` defines the replaceable
callback stages for the later evidence-backed implementation.

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
build/engine/quiky-calibrate game/NESTLE.DAT W1L1.MAP 100 100 /tmp/W1L1-calibration.csv
```

The calibration command emits a frame-by-frame CSV and a summary of the
current provisional movement constants. Its output is intended to be paired
with future DOSBox traces before those constants are treated as authoritative.

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
