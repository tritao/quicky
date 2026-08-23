# Ghidra analysis

The DOS NE loader in Ghidra 12.1.3 rejects the bundled `QUIKY.EXE`, so the
file-backed NE segments are imported as separate raw programs using
`x86:LE:16:Protected Mode`. Segment-relative addresses therefore match the
offsets used by the DOSBox debugger.

The project is kept outside the repository:

```text
/home/joao/quiky-ghidra-project-20260823-clean/QuikySegments
```

The segment extractor and annotation/decompiler scripts are:

- `research/tools/ghidra_ne_segments.py`
- `research/tools/AnnotateQuiky.java`
- `research/tools/DumpQuikyDecomp.java`

Run the annotation pass with:

```sh
/home/joao/.local/opt/ghidra_12.1.3_PUBLIC/support/analyzeHeadless \
  /home/joao/quiky-ghidra-project-20260823-clean QuikySegments \
  -process -postScript AnnotateQuiky.java \
  -scriptPath /home/joao/dev/quicky/research/tools \
  -commit 'Update Quiky annotations'
```

Decompiler output from the current run is in
`/home/joao/quiky-ghidra-decomp-20260823/` and is intentionally outside Git.

## Confirmed from the decompiler output

### MAP loading

Segment 1 contains two MAP-loading routines:

| Address | Label | Evidence |
| ---: | --- | --- |
| `0x365B` | `load_map_resource_primary` | Builds the first `GAMEDATA\\` + `.MAP` path and reads the selected entry. |
| `0x3861` | `load_map_resource_secondary` | Builds the second `.MAP` path and reuses the MAP buffer/dimensions. |

Both routines index the level table at:

```c
0x3574 + (uint16_t)DS[0x85D4] * 5
```

This directly confirms that the observed selector value participates in a
five-byte-per-level table lookup. The surrounding table contains the visible
`Nature 1`, `Nature 2`, `Nature 3` labels and `W1L1`, `W1L2`, `W1L3` names.

The primary routine then reads two values from the resource stream through the
unresolved file helper, stores a doubled first value as a row byte count, and
uses the second as the other MAP dimension. It allocates a buffer based on
those dimensions and copies two-byte cell values into it. It also ORs `0x10`
into the high byte of one dimension's worth of cells. The latter is confirmed
behavior in the executable, but its semantic role still needs runtime/source
correlation before being called a collision bit.

The secondary routine performs the same cell-copy and `0x10` operation using
the already-established dimensions. Whether this is a second layer, reload,
or another MAP role remains unknown.

### ARE loading

Segment 1 function `0x34C8` (`load_are_resource`) constructs a path from the
Pascal fragments `GAMEDATA\\` and `.ARE`, reads a resource through the same
unresolved helper family, allocates a buffer, and copies paired values from
the stream. The exact ARE field meanings remain unknown.

### BOB and ICO loading

The following routines use the same path-construction pattern:

```text
0x399E  load_bob_resource  GAMEDATA\\ + .BOB
0x3BBD  load_ico_resource  GAMEDATA\\ + .ICO
```

The BOB routine reads a repeated record structure with a visible `0x2C` stride
and several 16-bit/32-bit fields. The ICO routine iterates records/data ranges
and copies resource bytes into an existing buffer. These observations are
confirmed from generated code; the record semantics are still provisional.

### SAM and TFX loading

Segment 2 function `0x085E` (`load_sam_tfx_resource`) uses the Pascal fragments
`GAMEDATA\\`, `.SAM`, and `.TFX`. It loads the two audio resources in stages,
using `DS:0x504C` as a visible stage/status value (`1` through `6`) and
`DS:0x8954` as temporary state. This confirms the paired SAM/TFX loading flow;
the audio structures remain to be documented.

## Current blocker and next target

The NE relocation tables have now been parsed and the important cross-segment
targets have been annotated. Ghidra still displays the raw programs as
separate address spaces, so the relocation targets are labels and evidence
rather than fully connected decompiler thunks. The next useful target is to
correlate the common helper with a controlled runtime breakpoint or with more
callers, then recover the archive-directory record layout.

The decompiler reports are useful for narrowing the target, but unresolved
helper calls are not being treated as identified APIs.

## NE relocation results

`research/tools/ne_relocs.py` parses the segment relocation tables without
modifying `QUIKY.EXE`. Relocation source type `0x03` entries whose preceding
byte is `0x9A` are far-call operands. For the resource loaders, these now map
to concrete targets, including the already-confirmed runtime selectors:

```text
segment 1 -> segment 5:0x05CD (selector 0x0227)
segment 1 -> segment 5:0x0E87 (selector 0x0227)
segment 1 -> segment 5:0x0F06 (selector 0x0227)
segment 1 -> segment 4:0x18C7 (selector 0x0207)
segment 1 -> segment 4:0x125B (selector 0x0207)
segment 1 -> segment 4:0x1BDE (selector 0x0207)
segment 1 -> segment 4:0x1A37 (selector 0x0207)
```

The same sequence appears in the ARE, MAP, BOB, and ICO loaders. Segment 5
`0x05CD` is a stack-space probe. Segment 5 `0x0E87` copies a length-prefixed
Pascal string, and `0x0F06` appends one. These three meanings are confirmed by
their generated code.

Segment 4 `0x18C7` is an inferred common resource-entry lookup: it accepts a
Pascal path, scans a table, and writes shared resource-range state at
`DS:0x97E4..0x97EE` (`start`, `end`, and derived size fields). Segment 4
`0x19FF` compares the current position with that shared end state, supporting
the same interpretation. The exact archive-directory record layout still
needs runtime or caller-level confirmation.

The remaining segment 4 targets (`0x125B`, `0x1BDE`, `0x1A37`, `0x170A`, and
`0x1737`) are now concrete, but their stream-read/position semantics remain
deliberately unnamed.

## Runtime validation status

A debugger-only experiment set `DS:0x89F2 = 1` (the known cheat flag),
`DS:0x88BA = 5`, and `DS:0x85D4 = 2`, then resumed with a breakpoint at
`0207:0x18C7`. The emulator remained on the startup splash and did not expose
a reliable breakpoint stop or register/memory dump. Therefore this run is not
counted as confirmation of the lookup routine or its state variables. No game
files were modified; the writes were made only in the debugger session.

The static result remains the strongest current evidence: all four resource
loaders construct a Pascal path, call the same `0207:0x18C7` target, and then
use the shared `DS:0x97E4..0x97EE` state. A future runtime pass should break
after the menu has reached an actual level-load path rather than injecting the
level state during startup.
