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
- an SDL3 interactive W1L1 frontend with fixed-step input and camera scrolling;
- ARE-backed level streaming with provisional collectible, hazard, and exit events;
- synthetic unit tests for the format readers.

Music playback is an optional subsystem because the bundled `.TFX`/`.SAM`
resources use TFMX, which needs a dedicated decoder. The engine includes an
in-tree four-voice TFMX Pro parser, sequencer, and Paula-style mixer. It reads
paired resources directly from `NESTLE.DAT`, renders WAV, and can play through
SDL3 when SDL3 is discoverable by CMake.

Build it with the normal engine configuration:

```sh
cmake -S engine -B build/engine -DSDL3_DIR=/path/to/SDL3/cmake/config
cmake --build build/engine
build/engine/quiky-music game/NESTLE.DAT list
build/engine/quiky-music game/NESTLE.DAT render TITEL /tmp/TITEL.wav
build/engine/quiky-music game/NESTLE.DAT play TITEL
```

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

The renderer output is an indexed BMP using the palette stored in the matching
world PCC resource. The C++ W1L1 output matches the existing Python renderer
pixel-for-pixel when ARE overlays are disabled. The ARE overlay is a debug
visualization; it does not assign gameplay meanings to entity types.
