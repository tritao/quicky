# Quiky C++ engine core

This is the first standalone C++ slice of the Quiky engine recreation. It is
kept independent of SDL and ScummVM so the format and renderer code can be
tested against the original data before a ScummVM frontend is added.

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
- synthetic unit tests for the format readers.

Music and interactive gameplay are intentionally deferred; the simulation
probe only exercises the current deterministic runtime core.

Build and test from the repository root:

```sh
cmake -S engine -B build/engine
cmake --build build/engine
ctest --test-dir build/engine --output-on-failure
```

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
