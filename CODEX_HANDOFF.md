# Codex handoff: Quiky reverse engineering

This file is a restart guide for a future Codex instance on another machine.
It describes the repository state, the evidence already collected, and the
next useful experiments. Treat `research/README.md` and the two detailed notes
as the source of truth when this summary is abbreviated:

- [research/README.md](research/README.md) — formats, tools, archive facts,
  renderer, and DOSBox workflow.
- [research/notes/cheat-trace.md](research/notes/cheat-trace.md) — debugger
  trace and cheat/level-selector evidence.
- [research/notes/ghidra-analysis.md](research/notes/ghidra-analysis.md) — raw
  NE-segment Ghidra setup, decompiler findings, relocations, and runtime status.

## Repository state

- Project: Tricky Quiky Games II / Nesquik / Quiky reverse engineering.
- Branch: `main`.
- The repository has no configured Git remote at the time this handoff was
  written. Configure one before pushing:

  ```sh
  git remote add origin <repository-url>
  git push -u origin main
  ```

- The tracked `game/` directory contains the runnable DKIA-distributed copy:
  `QUIKY.EXE`, `NESTLE.DAT`, `NESTLE.INI`, `SETUP.EXE`, `RTM.EXE`,
  `DPMI16BI.OVL`, and `SCORE.DAT`.
- The bundled executable is the DKIA copy, not a proven untouched original.
  The DOSGames `NESQUIK.EXE` distribution differs at 44 byte positions from
  `QUIKY.EXE`; see `research/README.md` for hashes and provenance.
- Download archives, extracted assets, build products, and Ghidra projects are
  intentionally not tracked. A fresh clone has the runtime under `game/`, but
  not the ignored `research/downloads/`, `research/extracted/`, or
  `research/build/` directories.

Check the starting state with:

```sh
git status --short
git log --oneline --decorate -8
```

## Required environment

Minimum for archive inspection and tests:

- Python 3
- a POSIX shell for the scripts

Useful optional tools:

- DOSBox (`dosbox`) for normal execution
- the debug DOSBox build (`dosbox-debug`) for protected-mode breakpoints
- GCC or Clang for the original/modern C tools
- Ghidra, Java, and `analyzeHeadless` for static decompilation
- ImageMagick only if converting the generated BMP tile sheet to PNG

Run the baseline checks after cloning:

```sh
python3 -m unittest discover -s research/tests -v
python3 research/tools/quikyctl.py archive-index game/NESTLE.DAT
python3 research/tools/quikyctl.py ne-info game/QUIKY.EXE
git diff --check
```

The expected test count at this handoff is 7 passing tests.

## Running the bundled game

The repository scripts mount only the `game/` directory:

```sh
./scripts/run-dosbox.sh setup
./scripts/run-dosbox.sh
```

The script starts at 16,000 cycles and runs `QUIKY.EXE`. If setup changes
`NESTLE.INI`, restore it before committing unrelated runtime changes. For a
manual DOSBox session:

```text
mount c /absolute/path/to/quicky/game
c:
dir
setup.exe
quiky.exe
```

Use Sound Blaster with base `220`, IRQ `7`, and DMA `1` if setup asks. DOSBox
uses `Ctrl+F11` to decrease cycles and `Ctrl+F12` to increase them.

For debugger work:

```sh
./scripts/run-dosbox-debug.sh
```

The debug launcher uses `dosbox-debug`, mounts `game/`, and defaults to
16,000 cycles. `QUIKY_CYCLES=16000` overrides the speed. In the debug build,
`Alt+Pause` enters the debugger; debugger commands are described in
`research/notes/cheat-trace.md`.

Do not modify the tracked archive for experiments. Use a copied runtime or
`scripts/run-are-variant.sh`, which substitutes only a test `NESTLE.DAT` in a
temporary directory.

## Existing inspection workflow

The dependency-free Python tool is `research/tools/quikyctl.py`:

```sh
python3 research/tools/quikyctl.py archive-list game/NESTLE.DAT
python3 research/tools/quikyctl.py archive-index game/NESTLE.DAT
python3 research/tools/quikyctl.py archive-extract game/NESTLE.DAT /tmp/quiky-assets
python3 research/tools/quikyctl.py map-info /tmp/quiky-assets/W1L1.MAP
python3 research/tools/quikyctl.py are-info /tmp/quiky-assets/W1L1.ARE
python3 research/tools/quikyctl.py level-render /tmp/quiky-assets/W1L1.MAP \
  --output /tmp/W1L1.png
```

The archive contains 142 payloads. `NESTLE.DAT` is uncompressed. Its useful
confirmed values are:

```text
file size             3,705,125 (0x3880cd)
directory offset      0x3880c7
directory end         file size - 8
trailer u32 @ end-8   0x3880c7
trailer u32 @ end-4   0x0000008d
directory entries     142 (stored count is entries minus one)
```

The directory entry is little-endian `u16 name_length`, raw name bytes, then
`u32 payload_offset`. There is no payload length; calculate it from the next
entry offset, or from the directory offset for the final payload.

## Confirmed format facts and open questions

Evidence labels matter: do not promote Simon's inferences or a renderer
convention to an engine fact without validating it.

- `MAP`: starts with `TLE1`; width, height, and the constant `0x0009` are
  big-endian `u16` values at offsets `0x04`, `0x06`, and `0x08`; cells follow
  at `0x0a` as big-endian `u16` values.
- MAP cell interpretation currently used by the viewer:
  `ABCD EFGX XXXX XXXX`; tile ID is `value & 0x01ff`, and the upper seven
  bits are properties. The exact collision meaning is still unknown.
- `ICO`: headerless indexed tiles, 256 bytes per 16x16 tile, with the
  column-interleaved VGA layout handled by `quikyctl.py` and the original
  `ico2bmp.c`.
- `PCC`: ZSoft PCX files; world PCC files provide the 768-byte RGB palette.
- `ARE`: mechanically decoded as a `0x160` unknown/header region, a `0x1380`
  region of 2,496 big-endian layout words, then variable records beginning at
  `0x14e0`. Entity semantics and placement are not fully confirmed.
- `BOB`: loaded as animated/still graphics; record semantics remain
  provisional.
- `SAM`/`TFX`: paired TFMX audio resources; structure is not fully recovered.
- `NESTLE.DAT` packing/extraction: the modern C tools are safe and portable;
  the modern packer reproduced the original archive byte-for-byte.

## Current static-analysis result

Ghidra's direct NE import fails on this executable with an invalid-index error.
The working workaround is to extract each file-backed NE segment as an
independent raw `x86:LE:16:Protected Mode` program. Segment 7 is uninitialized
BSS and is not imported.

Runtime selector mapping observed for this executable:

```text
segment 1 -> 0x01D7
segment 2 -> 0x01E7
segment 3 -> 0x01F7
segment 4 -> 0x0207
segment 5 -> 0x0227
segment 6 -> 0x0237 (DS)
```

Generate raw segments on any machine with:

```sh
python3 research/tools/ghidra_ne_segments.py \
  game/QUIKY.EXE /tmp/quiky-segments
```

Import the nonempty `QUIKY_SEG01.bin` through `QUIKY_SEG06.bin` files into a
Ghidra project as raw 16-bit protected-mode x86 programs at offset/base zero.
Then run the checked-in annotation script from Ghidra's `support/` directory:

```sh
GHIDRA=/path/to/ghidra
PROJECT=/absolute/path/to/quiky-ghidra-project

"$GHIDRA/support/analyzeHeadless" "$PROJECT" QuikySegments \
  -process \
  -postScript AnnotateQuiky.java \
  -scriptPath "$PWD/research/tools" \
  -commit 'Annotate Quiky segments'
```

The decompiler-report script can be run similarly with a report directory
argument after the script name:

```sh
"$GHIDRA/support/analyzeHeadless" "$PROJECT" QuikySegments \
  -process \
  -postScript DumpQuikyDecomp.java /tmp/quiky-decomp \
  -scriptPath "$PWD/research/tools" \
  -commit 'Dump Quiky decompilation'
```

The relocation reader is independent of Ghidra:

```sh
python3 research/tools/ne_relocs.py game/QUIKY.EXE --segment 1
```

Important static result: the ARE, both MAP, BOB, and ICO loaders all build a
Pascal path and call segment 4 offset `0x18c7` (runtime selector `0x0207`).
Decompiler evidence makes `0207:18c7` a strong, still explicitly inferred,
candidate for a common resource-entry lookup. It accepts a Pascal path and
updates shared resource-range state at `DS:0x97e4..0x97ee`. Segment 4 offset
`0x19ff` appears to check the current position against the shared end state.

Confirmed segment 5 helper meanings:

```text
0227:05cd  stack-space probe
0227:0e87  Pascal-string copy
0227:0f06  Pascal-string append
```

The remaining stream helpers are deliberately unnamed until more evidence is
available.

## Cheat and runtime trace

The full string `QUIKYSUPERHERO` was experimentally typed through the
emulated keyboard and set `DS:0x89f2 = 1`. The selector branch is at
`01d7:0x491d`. The debugger also confirmed the protected-mode selectors and a
shortcut path to the level selector.

The most recent attempt to break at `0207:0x18c7` injected debugger-only state
(`DS:0x89f2 = 1`, `DS:0x88ba = 5`, `DS:0x85d4 = 2`) during startup. It remained
on the splash screen and did not give a reliable breakpoint stop, so this is
not runtime confirmation of the resource lookup. No game files were changed.

## Recommended next steps

1. Start a fresh debug DOSBox session and reach the level selector/game path
   normally, using the documented cheat trace rather than injecting level
   state during startup.
2. Break at `0207:18c7` after an actual W1L3 load attempt. If it stops, dump
   `DS:0x85d4`, `DS:0x97e4`, `DS:0x97e8`, and `DS:0x97ec`, plus the relevant
   stack bytes and the Pascal path argument.
3. Repeat for a known PCC, MAP, ICO, and BOB load to distinguish archive
   directory lookup from stream read/position helpers.
4. Use `archive-extract` and `level-render` to correlate the runtime filename,
   MAP dimensions, ICO tile index, PCC palette, and ARE overlay.
5. Recover the archive-directory record and stream-helper semantics, then add
   tests before making any archive or engine changes.
6. Keep collision-bit meanings marked as inferred until an executable caller
   or controlled in-game experiment establishes them.

When continuing, begin by reading this file, then `research/README.md`, then
the relevant detailed note. Do not restart the investigation from the original
web research unless a provenance or licensing question needs rechecking.
