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

The primary routine seeks four bytes past the `TLE1` header and reads two
big-endian values from the resource stream, stores a doubled first value as a row byte count, and
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
the stream. Segment 3 consumes that buffer as a region-reference grid followed
by six-byte entity declarations, as confirmed by the runtime trace below.

### BOB and ICO loading

The following routines use the same path-construction pattern:

```text
0x399E  load_bob_resource  GAMEDATA\\ + .BOB
0x3BBD  load_ico_resource  GAMEDATA\\ + .ICO
```

The BOB routine reads a repeated record structure with a `0x2C` runtime stride.
Static loader analysis plus a parser over every bundled BOB confirms each disk
record as slot, X/Y origin, width/height, a monotonic code-offset table, and an
x86 VGA blitter. The runtime descriptor stores geometry at `+00/+02/+08/+0A`,
the blitter pointer/size at `+10/+0C`, and the offset-table pointer/size at
`+20/+28`. Segment 3's draw path subtracts the origins from world position and
uses width/height for clipping. The safe decoder reconstructs pixels from
immediate planar writes without executing archive code. The ICO routine
iterates records/data ranges and copies resource bytes into an existing buffer.

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
rather than fully connected decompiler thunks. Runtime tracing has now
confirmed the common archive lookup, its shared result fields, and the core
relative seek/tell, big-endian word, and buffered byte helpers. The next useful
target is to recover the archive-directory record layout and identify the
underlying DOS/Pascal file-runtime calls.

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

Segment 4 `0x18C7` is the confirmed common resource-entry lookup. It accepts a
far pointer to a Pascal path, scans the archive directory, and writes the
matched resource range to shared state. The runtime-confirmed fields are:

```text
DS:0x97E4  u32 resource end offset (exclusive)
DS:0x97E8  u32 resource start offset
DS:0x97EC  u32 resource size
```

Segment 4 `0x19FF` compares the current position with the shared end offset.
The archive-directory record layout and its runtime representation are now
confirmed below.

## Runtime validation status

On 24 August 2026, the debugger-enabled automation build entered the proven
level selector by setting `DS:0x89F2 = 1`, `DS:0x88BA = 5`, and then selected
Nature 3 with `DS:0x85D4 = 2`. Pressing Space produced a reliable API
breakpoint stop at `0207:0x18C7`.

At the first stop, `SS:ESP` held the far return address `01D7:36D0`, followed
by a pointer to a length-prefixed `GAMEDATA\\W1L3.map` string. Sampling the
shared fields after the return gave:

| Field | Value | Archive match |
| --- | ---: | --- |
| `DS:97E8` | `0x002F3BD3` (3,095,507) | `W1L3.MAP` start |
| `DS:97E4` | `0x002F55A5` (3,102,117) | `W1L3.MAP` end |
| `DS:97EC` | `0x000019D2` (6,610) | `W1L3.MAP` size |

The next stop held return address `01D7:3531` and a pointer to
`GAMEDATA\\W1L3.are`. Its post-return values were:

| Field | Value | Archive match |
| --- | ---: | --- |
| `DS:97E8` | `0x002F55A5` (3,102,117) | `W1L3.ARE` start |
| `DS:97E4` | `0x002F6B19` (3,107,609) | `W1L3.ARE` end |
| `DS:97EC` | `0x00001574` (5,492) | `W1L3.ARE` size |

These values exactly match the independently parsed `NESTLE.DAT` index. This
confirms both the helper's role and the shared field meanings without changing
the executable or archive; only live debugger memory was modified to reach the
selector.

### W1L3 MAP stream-helper trace

The same run traced the first MAP parser calls and correlated them directly
with bytes at archive offset `0x002F3BD3`:

```text
54 4C 45 31  00 37  00 3C  00 09 ...
T  L  E  1      55      60       9
```

| Address | Confirmed role | Runtime evidence |
| ---: | --- | --- |
| `0207:125B` | `resource_seek_relative` | Called from `01D7:36D9` with offset 4; computes `DS:97E8 + offset`. |
| `0207:1BDE` | `resource_read_be_u16` | Called from `01D7:36E2`; consumed `00 37` and returned AX = `0x0037`. |
| `0207:1A37` | `resource_tell_relative` | Called from `01D7:3726`; returned AX = 10 after the three words at offsets 4, 6, and 8. |

The decompilation agrees with the trace: `0x125B` adds the shared resource
start before invoking the underlying seek, while `0x1A37` subtracts that start
from the underlying position. This establishes that loader offsets are local
to an archive member rather than absolute `NESTLE.DAT` positions.

### MAP buffered-reader trace

A fresh run stopped while the primary loader was reading `W1L1.MAP`. At this
point the archive member had size `0x3F52`; after its 10-byte header, the loader
called `0207:170A` with byte count `0x3F48` and file handle 5. The helper filled
the previously allocated far buffer at `DS:8A8C` and reset the word cursor at
`DS:8A90` to zero. The first 32 buffer bytes exactly matched `NESTLE.DAT` at
`resource_start + 10`.

The loader then called `0207:1737` without arguments. Two consecutive traced
calls returned `0x00` and `0x01`, matching the first two buffered bytes, while
the cursor advanced from 0 to 1 and then 2. The confirmed labels are therefore:

| Address | Label | Behavior |
| ---: | --- | --- |
| `0207:170A` | `resource_buffer_fill` | Reads a supplied count from a supplied handle into `DS:8A8C`; resets `DS:8A90`. |
| `0207:1737` | `resource_buffer_read_u8` | Returns `buffer[cursor]` and increments the cursor. |

This also explains the MAP cell-copy loop: it consumes each big-endian cell as
two buffered byte reads and stores the bytes in swapped host order.

### NESTLE.DAT directory layout

The archive initialization code at segment 4 reads the final eight bytes as
two little-endian dwords:

```c
struct ArchiveTrailer {
    uint32_t directory_offset;
    uint32_t last_entry_index;  /* entry count minus one */
};
```

For the bundled archive these are `0x003880C7` and 141, giving 142 entries.
Starting at `directory_offset` and ending immediately before the trailer, each
on-disk record is variable length:

```c
struct DirectoryRecord {
    uint16_t name_length;
    char name[name_length];
    uint32_t payload_offset;
};
```

All integer fields are little-endian and payload offsets are absolute from the
start of `NESTLE.DAT`. Records do not contain payload sizes. For every entry
except the last, lookup computes the exclusive end from the next entry's
offset. The final-entry branch at `0207:19BB` obtains the physical file size
with DOS seek-from-end and subtracts the 8-byte trailer. Consequently, the
game's nominal final resource range includes the serialized directory between
`directory_offset` and the trailer; the structural extractor intentionally
uses `directory_offset` as the final payload boundary instead.

At startup the executable converts the disk records into parallel fixed-size
tables:

| Address | Type | Meaning |
| ---: | --- | --- |
| `DS:8A92` | `u32` | Last entry index from the trailer. |
| `DS:8A96` | `byte[count][13]` | Pascal names: one length byte, up to 12 name bytes, then unused padding. |
| `DS:94BE` | `u32[count]` | Absolute payload offsets. |
| `DS:97DE` | `u32` | Directory offset from the trailer. |
| `DS:97E2` | `u16` | Open archive file handle. |

Live inspection matched the serialized index: slot 0 contained
`QUIKYW1.BOB` with offset 0, while slot 141 at `DS:91BF` contained
`TITELD.SAM` and the parallel offset at `DS:96F2` was `0x0037A3A5`.
`resource_entry_lookup` compares its normalized Pascal name against these
slots, seeks to the matched absolute offset, and publishes start, end, and
size through `DS:97E8`, `DS:97E4`, and `DS:97EC` respectively.

### ARE entity placement trace

Segment 3 routine `01F7:1CDA` tracks the camera in 64-pixel increments and
looks up declarations for newly visible regions. A nonblank reference is added
to the base of the loaded ARE buffer and passed to the near routine at
`01F7:1E04`. Because the loader stores the ARE bytes beginning at disk offset
`0x160`, a runtime buffer offset maps to `file_offset = 0x160 + buffer_offset`.

`01F7:1E04` walks records in this confirmed host-order representation:

```c
struct LiveAREEntity {
    uint16_t type_and_state;
    uint16_t local_x;
    uint16_t local_y;
};
```

The low byte of `type_and_state` is the entity type. On first processing, the
routine changes its high byte from zero to one; subsequent region visits skip
that record. `0xFFFF` terminates the declaration. For normal entity types the
routine indexes a four-byte dispatch table at `DS:81D2 + type * 4`, invokes the
object factory, and writes 16.16 fixed-point positions as:

```text
object_x = (are_region_origin_x + local_x) << 16
object_y = (are_region_origin_y + local_y) << 16
```

The origins at `DS:3714` and `DS:3716` are aligned to 64 pixels. Types `0x65`,
`0x66`, and `0x67` take dedicated creation paths rather than the normal dispatch
table.

The live W1L1 sample stopped with `FS:BX = 037F:1632`, region origin `(768,192)`,
and record words `(type=0x2B, x=0, y=32)`. Since the loaded buffer starts at ARE
offset `0x160`, this is the record at disk offset `0x1792`; its archived bytes
are `00 2B 00 00 00 20`. The resulting world position is therefore `(768,224)`.
This proves that ARE X/Y values are pixel offsets within a streamed 64-pixel
region, not tile indices or normalized renderer coordinates.

The automated type `0x2B` vertical slice additionally stopped at the normal
dispatch path with table bytes `27 47 01 00`. Static factory analysis confirms
this layout is update callback `01F7:4727`, object class `1`, and reserved byte
`0`; it is not a far pointer through segment ordinal 1. The shared factory at
`01F7:0E06` returned object `027F:0078`;
after caller initialization its 16.16 position fields were exactly
`(768,224)`. An isolated archive variant changed only W1L1 ARE record `0x1792`
from type `0x2B` to inert type `0`. In aligned paired captures the animated
falling leaves were absent from the inert run while the cloud and static scene
remained unchanged. This supports the catalog name `falling_leaves` at
confirmed confidence.

The callback's first far helper returns at `01F7:474D` with the same live
object in `ES:DI` and a logical sprite slot in `object+0x12`; independent live
runs observed slots `700` and `703`. The renderer at `01F7:3529` resolves that
field through the `DS:6D8E` slot map. `BLATT.BOB` contains the two interleaved
leaf families `700` through `707` and `750` through `757`, all with origin
`(7,12)` and size `14x12`. Safe decoding of representative slot 700 with
`W1.PCC` produces the leaf shown in
[`type-2b-falling-leaf-slot-700.png`](type-2b-falling-leaf-slot-700.png).
This closes the live chain from ARE type `0x2B` through its callback and
runtime object to the compiled sprite family.

The relocated call at `01F7:4748` targets `01F7:5D38`. That helper reads an
animation table from `DS:SI`, stores its delay at object offsets `+1E/+20`,
its cursor at `+22/+24`, and its first slot at `+12`. The callback chooses
between two tables using a signed byte from the engine PRNG ring at `DS:646C`:
`DS:3312` is delay 8 with slots 700-707 in order, while `DS:3326` is delay 10
with slots 703-707 then 700-702. Both end in `-8`, which the relocated
`01F7:4879 -> 01F7:5D60` advance helper interprets as an eight-word loop-back.
Both helpers add 50 to a selected slot when object byte `+28` is `FF`, choosing
the visibly brighter 750-757 row in
[`type-2b-falling-leaves-sheet.png`](type-2b-falling-leaves-sheet.png).

Runtime callback sampling also corrects the ownership model: the original
ARE object seeds the effect, while subsequent leaves occupy pooled objects in
the same selector (observed offsets include `0168`, `01E0`, `02D0`, and
`03C0`). `quikytrace --lifetime-samples N` records object identity, slot,
delay, animation cursor, and the `+28` variant flag at each shared leaf update.

The pooled-object allocator is the shared factory at `01F7:0E06`. It starts at
`DS:755E`, advances by the runtime object stride `DS:30CE`, and scans exactly
`0x40` entries. Its free test is `ES:[DI+0x18] == 0`; the first entry with no
update callback receives the requested callback and default object fields
before the type-specific initializer runs. This makes the leaf recycle rule
explicit rather than inferred only from repeated offsets.

### Early normal enemy families

The remaining early normal dispatch entries were traced at their
post-initializer boundaries and paired against an inert type-0 declaration at
the same W1L1 streamed anchor. The direct slot evidence is:

| types | callbacks / post boundaries | slots | world asset selected from the executable resource list |
| --- | --- | --- | --- |
| `0x01`/`0x02` | `6DA3`/`6DB0`, `6DB1`/`6DC3` | 281/231 | W1 `WURM2.BOB` records 1/0 |
| `0x03`/`0x04` | `689F`/`68AC`, `68AD`/`68BF` | 276/226 | W1 `BIENE.BOB` records 1/0 |
| `0x05`/`0x06` | `7B50`/`7B5D`, `7B5E`/`7B70` | 254/204 | W2 `FISCH.BOB` records 3/2 |
| `0x07`/`0x08` | `776B`/`7778`, `7779`/`778B` | 200/250 | W2 `KRABBE.BOB` records 0/1 |
| `0x09`/`0x0A` | `713D`/`714A`, `714B`/`715D` | 250/200 | W3 `PENGO.BOB` records 1/0 |
| `0x0B`/`0x0C` | `6651`/`6698`, `6699`/`66E0` | 209/209 | W3 `SCHNEE.BOB` record 0 |
| `0x15`/`0x16` | `7ED7`/`7EE4`, `7EE5`/`7EF7` | 208/208 | W4 `FLIEGE.BOB` record 0 |
| `0x17`/`0x18` | `8451`/`845E`, `845F`/`8471` | 250/200 | W4 `SPINNE.BOB` records 1/0 |
| `0x19`/`0x1A` | `5050`/`505D`, `505E`/`5070` | 250/200 | W5 `BUGGY.BOB` records 1/0 |
| `0x1B`/`0x1C` | `5F07`/`5F14`, `5F15`/`5F27` | 264/214 | W5 `UFO.BOB` records 1/0 |

The type-specific callbacks set movement/orientation state before their
post-initializer return; the paired types in each row therefore share a BOB
family but are retained as separate catalog entries. `bob-find` shows the
same logical slots in several other BOB files. Selector-mode resource traces
directly capture the W1 WURM2/BIENE, W2 FISCH/KRABBE, W3 PENGO, W4
FLIEGE/SPINNE, and W5 BUGGY/UFO batches. The live W3L1/W3L2 type-0B/type-0C
traces write slot 209, while the corresponding selector-mode batches load
`SCHNEE.BOB` and not the archive-colliding `PROP.BOB`; both types are therefore
confirmed as W3 snow-family variants. The renderer independently resolves
slot 209 through `DS:6D8E` to map index 133 and a live descriptor with
dimensions `34x26` and origin `(17,26)`, matching `SCHNEE.BOB` record 0. The
authoritative per-type records are in `research/entity-types.json`;
representative paired manifests are under
`research/build/entity-*-evidence/`.

### Pickup type `0x6F`

The W1L1 declaration at disk offset `0x1838` (archive world sample
`(544,304)`) dispatches through `DS:81D2+0x1BC` to `01F7:8BC2`, class 1. The
callback writes the normal object update pointer `01F7:8D20`; at `01F7:8BCE`
the live object has slot `607` in `object+0x12`. `bob-find` resolves that slot
uniquely to record 0 of `WERBE.BOB`, a 26x34 opaque Nesquik-branded pickup/sign
sprite. A baseline versus inert-type-0 archive mutation removes the object,
confirming the catalog entry `ten_ammo_box`.

The adjacent pickup callbacks share the same object layout and runtime update
callback. `01F7:8BE5` writes slot 608 for type `0x70`, `01F7:8C08` writes
slot 609 for type `0x71`, and `01F7:8C2B` writes slot 610 for type `0x72`.
Their post-slot boundaries are `01F7:8BF1`, `01F7:8C14`, and `01F7:8C37`.
`WERBE.BOB` records 1-3 decode to 21x22, 22x22, and 15x25 respectively.
Target-typed archives with a one-record type-0 mutation confirm that all
three are live removable pickup objects; the catalog names follow the
previously established gameplay semantics.
Native W1/W2 renderer probes resolve slots 607-610 through map indices
185/186/187/144 and descriptor offsets 8140/8184/8228/6336. The live
geometries are `26x34`, `21x22`, `22x22`, and `15x25`, all with origin `(0,0)`,
matching WERBE.BOB records 0-3.

Normal type `0x28` uses dispatch entry `01F7:9256`, object class `0`, and
reserved byte `0`. Its initializer leaves `object+0x12` at `0xFFFF`; paired
W1L1 captures remove a large white cloud when the record is changed to inert
type 0. The corresponding four-frame `WOLKE.BOB` family is slots 413-416,
each 32x16 with origin `(0,0)`. Because the standard logical-slot field is
unused, the asset correlation is recorded as a special-renderer mapping, not
as a direct slot write.

The callback's relocated calls are only the shared camera gate `01F7:1DCA`,
removal `01F7:1DEE`, bounds helper `01F7:393C`, animation helper `01F7:5D38`,
and MAP helpers `01F7:1C4D`/`01F7:5C27`; it does not call the normal renderer.
The standard renderer at `01F7:3529` returns immediately when
`object+0x12 == 0xFFFF`. A controlled W1L1 probe with `DS:89EA=0`, a
synthetic 16x16 bounds rectangle, and player byte `+0x37=0` reaches the
accepted `01F7:9269` branch and records `DS:89E6: 0 -> FFFF` while the cloud
callback remains active. One-shot reader probes then hit both player-side
consumers `01F7:4087` and `01F7:4406` with `DS:89E6=FFFF`; the player-state
consumer path is therefore resolved. A controlled outer-state run also hits
the main-loop consumer `01D7:4EA0` repeatedly with `DS:89E6=FFFF`; the normal
object renderer is deliberately bypassed, so only the low-level WOLKE.BOB
pixel primitive remains outside the logical-slot queue.

Types `0x29` and `0x2A` use the same normal dispatch callback as `0x2B`:
`01F7:4727`, object class `1`, reserved byte `0`.
Types `0x65`, `0x66`, and `0x67` branch before the table and call
`01F7:178D`, `01F7:1798`, and `01F7:17A3`, respectively. Those wrappers set a
subtype byte to `0x00`, `0x08`, or `0x10` and converge on the common creator at
`01F7:1749`.
Controlled one-variable archives replacing the reachable W1L1 record `0x1792`
with each dedicated type independently confirmed all three runtime branches at
the unchanged calculated world position `(768,224)`. The trace ledgers are
`entity-65-handler-trace.json`, `entity-66-handler-trace.json`, and
`entity-67-handler-trace.json` under `research/build/`.

The dedicated path is an event producer rather than a normal sprite factory.
`01F7:1749` increments the pending count at `DS:895E`, takes a byte from the
`DS:8960` ring, and writes an 8-byte event at `DS:6586 + ring_slot * 8`:

```text
+0x00  dword event position after the common creator's fixed-point transform
+0x04  source ARE/runtime record word
+0x06  animation state, populated from the PRNG helper at 01F7:5C11
+0x07  dedicated subtype copied from DS:36EE
```

The event loop reaches the far call at `01F7:1892`, whose relocated target is
`01F7:16CE`. That routine allocates a pooled object, leaves its normal
`object+0x12` sprite slot at `0xFFFF`, stores a subtype/animation-derived
index in `object+0x2E`, and installs the short-lived update callback
`01F7:10B5`. The event loop increments the queued animation byte modulo eight,
adds the subtype byte and `0x1D6`, and passes the resulting index as the
`01F7:16CE` `CX` argument. The callback first calls `01F7:1693`, which rejects
objects outside the camera rectangle. For a visible object, the code at
`01F7:1186` derives the animation selector/offset from `DS:6570`, `DS:6574`,
and `DS:6576`, then calls the short ICO drawing helper four times through
`01F7:11B4`.

Camera-centered W1L1 runs set the debugger-only camera override to the event's
`(368,304)` object coordinates and restore `DS:81C0`/`DS:81C4` afterward. The
live `FS:BX` blocks are 256 bytes and match `LOOP_W1.ICO` records exactly:
type `0x65` produced records 1 and 2 in repeated runs, while types `0x66` and
`0x67` produced record 6 in the captured runs. The W1 runtime selector was
`FS=0x035F`; the resource trace independently records the lazy
`GAMEDATA\\loop_W1.ico` request. These results resolve the animation asset as
the world-specific `LOOP_Wn.ICO` family while leaving the gameplay name
conservative. The path ledgers are `entity-65-dedicated-trace.json` through
`entity-67-dedicated-trace.json`; the animation ledgers are the corresponding
`entity-65/66/67-dedicated-animation-v8.json` traces under `research/build/`.

The world-relative asset choice is independently confirmed with first-entity
mutations in other levels. The W2L1 type-`0x66` trace matches
`LOOP_W2.ICO` record 8 at `FS:0x035F:0xDE00`; W3L1 type-`0x67` matches
`LOOP_W3.ICO` record 6; W4L1 type-`0x67` matches `LOOP_W4.ICO` record 22;
and W5L1 type-`0x65` matches `LOOP_W5.ICO` record 4. Each comparison is an
exact 256-byte block match, and the live selector remains `0x035F` while the
loaded world resource changes. The cross-world traces are
`dedicated-animation-w3-v1.json`, `dedicated-animation-w4-v1.json`,
`dedicated-animation-w5-v1.json`, and
`entity-66-dedicated-animation-w2l1-first-v1.json` under `research/build/traces/`.

Controlled W1L1 anchor traces for `0x29` and `0x2A` stop at the common
post-initializer boundary `01F7:474D` and write slots 700/703 in
`object+0x12`. Both read the same `DS:3312`/`DS:3326` animation tables and
resolve to the `BLATT.BOB` leaf families 700-707 and 750-757. Their target vs
inert manifests are `research/build/entity-29-evidence/experiment.json` and
`research/build/entity-2a-evidence/experiment.json`; the traces are
`research/build/traces/entity-29-confirm.json` and
`research/build/traces/entity-2a-confirm.json`. They are cataloged as leaf
variants because the dispatch code converges before any type-specific branch.

Additional callback families now have runtime slot evidence. Type `0x2C`
(`01F7:8C4E`) writes slot 710 at the post-callback boundary `01F7:8C70`,
which resolves uniquely to the 18x15 record 0 of `PAPIER.BOB`. Types `0x33`
(`01F7:87D1`), `0x35` (`01F7:544C`), and `0x36` (`01F7:545A`) write slots
214/264/214 respectively. Direct selector-mode resource traces resolve the
archive-wide collisions: W4L1 loads `WIND.BOB` (slot 214), while W5L1 loads
`UFO.BOB` (slots 264 and 214); neither world loads `SCHNEE.BOB` in its initial
resource batch. Type `0x34` (`01F7:9BEE`) writes slot 400, matching the
world-specific `BUMP_W1.BOB`-`BUMP_W5.BOB` family; W1L2 resource tracing
directly captures `BUMP_W1.BOB`.

Types `0x3D`-`0x40` share update callback `01F7:9DC7` after their distinct
initializers. Their post-callback slots are 301, 300, 301, and 300, matching
the world-specific `PLATFW*.BOB` records 1, 0, 1, and 0. Selector-mode
resource traces directly capture `PLATFW4.BOB` in W4L1,
`PLATFW3.BOB` in W3L2, `PLATFW1.BOB` in W1L3, and `PLATFW2.BOB` in W2L1;
the catalog now marks this family confirmed.

Native-context renderer probes independently resolve the shared-slot effect
families: W4 type `0x33` reaches the slot-214 descriptor at map index 137 with
geometry `33x29`, origin `(16,29)`, matching `WIND.BOB`; W5 types `0x35` and
`0x36` reach map indices 138/137 with geometry `31x31`, origin `(15,31)`,
matching `UFO.BOB` records 1/0. This verifies the resource-context selection
at the live descriptor layer, not only from archive path ordering.

The same native-context probe now covers the remaining shared-slot families.
Type `0x34` in W1L2 reaches slot 400 at map index 149 and descriptor offset
6556, with geometry `32x23`, origin `(16,23)`, matching `BUMP_W1.BOB` record 0.
Types `0x3D`-`0x40` reach map indices 104, 109, 85, and 119 respectively;
their live descriptor offsets are 4576, 4796, 3740, and 5236. The resulting
geometries are `32x14`, `48x16`, `32x14`, and `48x16`, all with origin `(0,0)`,
matching the W4, W3, W1, and W2 `PLATFW*.BOB` records selected by the resource
batches. These checks tie the world-specific BOB choice to the renderer's
actual GS descriptor rather than relying only on slot or filename correlation.

Native probes also cover representative ordinary families. W1L1 types `0x01`
and `0x03` resolve WURM2/BIENE descriptors with geometries `52x16` and
`46x36`; W2L1 types `0x05` and `0x07` resolve FISCH/KRABBE as `40x19` and
`41x21`; W3L1 type `0x09` resolves PENGO as `36x35`; W4L1 types `0x15` and
`0x17` resolve FLIEGE/SPINNE as `37x30` and `36x29`; and W5L1 types `0x19`
and `0x1B` resolve BUGGY/UFO as `33x27` and `31x31`. Type `0x2C` in W2L2
resolves slot 710 to the `18x15` PAPIER descriptor. In every case the live
width, height, and origin equal the selected BOB record, extending the direct
renderer check beyond the previously tested shared-slot effects.

The normal types `0x1F`-`0x21` initialize `object+0x2E` to 1, 2, and 3 and
converge on update callback `01F7:8E4B`, but leave `object+0x12` at `0xFFFF`.
Controlled W1L1 probes redirect the traced object to world `(3072,272)`, where
state 4 queries five nearby cells through `01F7:3376`. The returned tile IDs
`201, 200, 202, 203, 204` become effect indices `121, 120, 122, 123, 124`
through `DS:6986`; each nonzero entry reaches `01F7:16CE` and creates a
three-tick object using update callback `01F7:10B5`, still with no standard BOB
slot. A post-selector probe at `01F7:1186` catches the created object's actual
animation lookup: effect index 121 selects `FS=0x0357`, offset `0x7900`, and
the 256-byte block is byte-identical to raw tile 121 in `W1.ICO`. The three ARE
types produce the same sequence, so their behavior is confirmed as a shared
animated world-ICO tile-effect state machine.

The callback trace was extended to record `10B5 -> 1693 -> 1186` for the
short-lived objects. In a synthetic W5L1 state-4 probe, forcing the five MAP
lookups to effect states 61-64 creates four objects with state fields 61-64;
their live lookups select `FS=0x0357` offsets `0x3D00`, `0x3E00`, `0x3F00`, and
`0x4000`, byte-identical to `W5.ICO` records 61-64. Equivalent probes match
W2 states 126-130 at `0x7E00`-`0x8200`, W3 states 400-404 at `0x9000`-`0x9400`,
and W4 states 240-244 at `0xF000`-`0xF400`. The first visibility run also
showed the game camera had moved back to `(0,262)` before `1693`, which
returned carry set; reapplying the debugger-only camera at `10B5` makes the
same objects pass `1693` and reach `1186`. This confirms both the visibility
gate and the world-relative ICO table across W1-W5.

The normalized nonzero `DS:6986` entries for all five worlds, together with a
matched W2L1 run of types `0x1F`, `0x20`, and `0x21`, are recorded in
[`research/effect-mappings.json`](../effect-mappings.json). The matched runs
all produce update states `0,1,2,3` and effect sequence `127,126,128,129,130`;
only the initial `object+0x2E` field changes from `1` to `2` to `3`.

### Static decode of `01F7:8E4B` and `01F7:3376`

The raw-segment disassembly and NE relocation records resolve the state-machine
control flow more precisely than the runtime summary alone. `01F7:8E4B`
increments `object+0x32` on every nonzero-state callback. Its zero-state path
calls `01F7:1DCA`; on carry it calls `01F7:1DEE` and returns, otherwise it
calls `01F7:393C` and applies the object/camera eligibility checks. An eligible
object is initialized to state 1. The subsequent exact state values are 4, 6,
8, and 10; values between those checkpoints simply return after the increment.

Each checkpoint performs five copies of the same sequence:

1. call `01F7:3376` with `AX = object+0x08 + y_offset` and
   `BX = object+0x04 + x_offset`;
2. use the returned low-nine-bit tile ID as an index into
   `DS:6986[tile_id]` (`word` stride 2);
3. if that effect entry is nonzero, call `01F7:16CE` with the queried X/Y
   coordinates and the effect value in both `CX` and `DX`.

The statically decoded lookup grid is:

| `object+0x32` | Y argument | X offsets, in call order |
| ---: | --- | --- |
| 4 | `object+0x08` | `+0x10, +0x00, +0x20, +0x30, +0x40` |
| 6 | `object+0x08 + 0x10` | `+0x10, +0x20, +0x30, +0x40, +0x50` |
| 8 | `object+0x08 + 0x20` | `+0x10, +0x20, +0x30, +0x40, +0x50` |
| 10 | `object+0x08 + 0x30` | `+0x10, +0x20, +0x30, +0x40, +0x50` |

At state 10 the callback clears `object+0x18`, ending the state-machine
object, and publishes `object+0x04 + 0x19` and `object+0x08 + 0x46` at
`DS:8828` and `DS:882A`. The purpose of those two published coordinates is
not assigned beyond this executable-level fact.

The MAP helper at `01F7:3376` is correspondingly:

```c
uint16_t map_tile_id_lookup_16px(uint16_t y, uint16_t x) {
    uint8_t *cell = DS_657A
        + (y >> 4) * DS_657E       /* byte stride of one MAP row */
        + (x >> 4) * 2;             /* two bytes per big-endian cell */
    return read_u16(ES, cell) & 0x01ff;
}
```

Ghidra's equivalent expression is
`*(uint16_t *)(DS:657A + (AX >> 4) * DS:657E + (BX >> 4) * 2) & 0x01ff`.
The helper loads `ES` from `DS:657C` before reading, so `DS:657A/DS:657C`
are the far MAP buffer and `DS:657E` is the loaded row-byte stride. The W2L1
trace's state-4 calls `(AX,BX) = (336,448), (336,432), (336,464),
(336,480), (336,496)` match the first table row exactly; the post-state
effects `127,126,128,129,130` match the `DS:6986` and ICO evidence above.

### Player MAP descriptor helpers

The previously unresolved player-side MAP reads at `01F7:5C27` and
`01F7:5CC3` are now decoded from raw 8086 bytes and cross-referenced from
`3A1F`, `3DF2`, and `3D02`. Both helpers calculate the same cell address as
`3376`, then load the raw word and execute `AND AH,1`. Because the low byte is
unchanged, this operation is `raw_cell & 0x01ff`, not a test of only bit 8.
It removes the seven upper MAP bits before indexing the tile descriptor table:

```c
uint16_t tile_id = raw_cell & 0x01ff;
uint16_t descriptor = read_word(
    DS_6584, DS_6582 + tile_id * DS_30D4 + 2);
```

`5C27` tests `descriptor & 0x0f` and then selects one of four low-nibble
bits based on `AX bit 3` and `BX bit 3`: `0x02`, `0x01`, `0x04`, or `0x08`.
It communicates the classification via return flags. `5CC3` returns the
descriptor word in `DX`; its caller at `3D02` tests descriptor flags directly.
That caller probes again after an eight-pixel X adjustment when `DX & 0x30`
is clear. The exact vertical part of `3D02` is:

```c
if (descriptor & 0x20) {
    object->vertical_response = object->vertical_input >> 1;
    object->vertical_state = 0xff;
    phase = (object->x & 0x0f) >> 1;
} else {
    object->vertical_response = (-object->vertical_input) >> 1;
    object->vertical_state = 1;
    phase = (0x0f - (object->x & 0x0f)) >> 1;
}

target_y = (object->y & 0xfff0) + phase;
if (!(descriptor & 0x40))
    target_y -= 8;
```

Thus `0x20` is precisely the vertical-response polarity/state selector and
`0x40` is precisely the eight-pixel vertical-alignment selector. They are not
independent MAP upper-property bits, and `0x40` is not itself a floor/ceiling
type. The source supports the usual y-down interpretation that the `0x20`
clear branch reverses the vertical response, but the gameplay names
“ceiling” and “floor” remain provisional until both boundary orientations
are observed.
The descriptor table is also used by the renderer, which indexes tile imagery
by the same nine-bit tile ID, but the collision helpers consume the descriptor
word's low flags instead of the MAP upper field. Runtime property-focused
traces now use protected-mode selector reads for the far MAP and descriptor
buffers. Earlier generated property artifacts used `mem_read_word` with a
protected selector and must not be treated as MAP evidence; the guest `DX`
branch registers and object-state captures from those runs remain valid.

The callback-focused boundary traces now include each helper's far-return
address, tying observations to static call sites: return offsets `0x3A3E` and
`0x3A50` are the two `3A1F` probes, `0x3E10`/`0x3E22` the `3DF2` probes,
`0x3D1E`/`0x3D36` the `3D02` descriptor reads, and `0x41FC`/`0x420E` the
vertical-path probes in the player callback.

The helper call-site and return-address evidence remains valid, but the tile
and descriptor values in the pre-selector-safe `3A1F`/`3DF2` property rows are
superseded and will be recaptured with selector reads. The first corrected
W1L1 `5CC3` sample at `(128,400)` reads raw cell `0x002e`, tile `0x02e`, and
descriptor `0x000c`. The selector-safe matrix also reads `0x002d`/tile
`0x02d` with descriptor `0x000c` at the corrected left-wall and right-input
samples; jump rows reach tiles `0x001` and `0x01a` with descriptor `0`.

The dedicated `3D02` branch trace also reaches a nonzero vertical descriptor:
at `(1163,338)` the first descriptor query is clear, the eight-pixel retry
returns `0x0050`, and the path is `DX&0x20 == 0`, `DX&0x40 != 0`, then success
with `AL=1` and `object+0x3A=1`. This is a runtime check of the two exact
bit-consumer branches above; it does not by itself rename the response as
floor or ceiling.

Debugger-only descriptor controls now provide a clean positive/negative pair.
Patching the live probe cell to tile `0x02a` (descriptor `0x0070`) reaches
`3D02 -> 3D1E -> 3D45 -> 3DD0 -> 3DF1`, with `DX&0x20` and `DX&0x40` set,
`AL=1`, and `object+0x3a=0xff`. Tile `0x02b` (descriptor `0x0030`) reaches
the same tests but returns through `3DE4`, with `AL=0` and the byte restored
to zero. The original MAP word is restored after each branch.

The long right run reaches the known reset state at `(2131,368)` with
`+0x37=0xff` and `+0x3e=1000`; its old property-word association is not used
until recaptured with selector-safe reads.

The zero-state gate is also now decoded. `01F7:1DCA` evaluates the current
object against the camera with unsigned comparisons:

```c
x_test = 0x80 + object->x - DS[0x81C0];
if (x_test > 0x240) return_carry_set();
y_test = 0x80 + object->y - DS[0x81C4];
if (y_test > 0x1B0) return_carry_set();
return_carry_clear();
```

Thus the accepted camera window is approximately
`camera_x - 0x80 <= object.x <= camera_x + 0x1C0` and
`camera_y - 0x80 <= object.y <= camera_y + 0x130`; unsigned underflow also
rejects objects too far to the left or above. When the gate rejects an object,
`01F7:1DEE` clears `object+0x18` and the byte at
`FS:[object+0x1A+1]`, deactivating the state-machine object.

For an object inside the camera window, `01F7:393C` supplies the second gate's
dynamic bounds. If `DS:89EA` is zero, it reads the object pointed to by
`DS:881A` and returns:

```text
AX = bounds_object+0x04 + bounds_object+0x2C
CX = bounds_object+0x04 + bounds_object+0x30
BX = bounds_object+0x08 + bounds_object+0x2E
DX = bounds_object+0x08 + bounds_object+0x32
```

If `DS:89EA` is nonzero it returns zero in all four registers. `8E4B` then
accepts the state-machine object only when its X range overlaps `(AX,CX)`
with a 0x50 extension and its 16-pixel-aligned Y range overlaps `(BX,DX)`
with a 0x40 extension; acceptance sets `object+0x32` to 1. The W2L1 sample
returned `(AX,BX,CX,DX) = (232,236,832,436)` for an object at `(432,336)`;
W3L1 returned `(648,12,1248,212)` for `(848,112)`, W4L1 returned
`(536,236,1136,436)` for `(736,336)`, and W5L1 returned
`(632,60,1232,260)`. These samples all advance to state 1.

A follow-up controlled probe captured the indirect lookup at the actual
`01F7:393C` entry. In that run `DS:881A = 0`, `DS:89EA = 0`, `DS = 0x0237`,
and `ES = 0x027F`; the helper therefore reads `ES:0000`, not the current
state-machine object at `ES:0078`. The entry `DI` was still `0x0078` before
the helper's `mov DI,[DS:881A]`, which independently confirms that overwrite.
The sampled `ES:0000` fields were `base_x=128`, `base_y=400`,
`x_left=488`, `y_bottom=-260`, `x_right=1088`, and `y_top=-60`; the returned
low words were `(AX,BX,CX,DX) = (616,140,1216,340)`. This resolves the
bounds-object identity: it is the persistent offset-zero record in the object
selector, while the transient tile-effect object is a separate pooled record.

The expanded `FindQuikyReferences.java` survey found no direct `DS:881A`
write in the raw segments. Besides `393C`, the executable code around `69FF`
also loads the offset-zero object's X/Y words before comparing them with the
current object's position; its higher-level behavior is not assigned yet.
The `DS:8828/882A` pair is published by the state-10 path through a pointer
loaded from `DS:8828`, so a literal `DS:882A` byte-reference is not expected.
The other `DS:89EA` users are real control flow: the segment-3 routine at
`44DC` decrements the word and tests signed thresholds, while the segment-1
main loop tests it at `4BA4`, `4C43`, and `4CB8`. Its exact gameplay role is
still open, but `393C`'s zero/nonzero bounds-mode gate is now connected to
that shared control word rather than treated as an isolated data flag.

The persistent record is now identified as the player. Static code at
`01F7:3F27` stores the incoming `ES:DI` offset into `DS:881A`, initializes the
record's movement/collision fields, sets its initial bounds to
`(+2C,+2E,+30,+32)=(-10,40,10,0)`, and installs callback `01F7:3FF8`.
The callback's nonzero-`DS:89EA` branch performs a transitional vertical-motion
path and decrements the shared control word at `01F7:44DC`. Its zero branch
updates input-driven state, calls the collision helpers reached at `648E`,
`6484`, and `3A8A`, and preserves position snapshots at object `+0x44/+0x48`.

The player tracer confirms this statically derived identity in W1L1: the pool
uses selector `0x027F`, offset `0`, and stride `0x78`; `DS:881A` remains zero,
the record starts with callback `3F27`, and subsequent samples show callback
`3FF8` at the same record and position `(128,400)`. This is the first direct
runtime correlation between the bounds helper's offset-zero record and the
player initializer.

The first controlled input pass now exercises this callback rather than only
observing initialization. Holding `KBD_right` for 30 guest frames moved the
record from `(128,400)` to `(170,400)` and `(219,400)` across repeated
`3FF8` barriers. The candidate collision breakpoint `01F7:648E` fired in the
same run, while a MAP-focused run reached `01F7:3376` at `(133,400)` and
`(173,400)`, returning tile IDs `0x0b8` and `0x167`. These observations tie
the right-input path to both the collision-helper call graph and the existing
16-pixel MAP lookup; they do not yet assign floor/side semantics.

The callback-focused probe now leaves all related breakpoints armed while the
callback runs, so the near-return check is no longer confused with a helper
entry. Ordinary W1L1 updates reach `648E -> 6484 -> 3A8A` and return to the
expected offset `0x0F26`. At a long right-input transition near `(2132,368)`
only `648E` is reached before the player record enters the `+0x3E=0x03E8`
checkpoint/reset state; releasing input returns it to `(1673,368)`. A long
left-input run instead remains at `(72,400)` with zero velocity, and its MAP
sample is `(77,400)`, cell `0xEC8B`. These are controlled wall-versus-reset
observations, not final gameplay names.

The neighboring normal dispatch range `0x79`-`0x7F` is a seven-piece puzzle
letter family. Static disassembly of `QUIKY_SEG03.bin` shows dispatch entries
at `DS:81D2+0x1E4` through `+0x1FC`: each initializer writes one consecutive
sprite slot (600 through 606) into `object+0x12`, then returns to the common
object setup. The callbacks are `01F7:8C71`, `8C8A`, `8CA3`, `8CBC`, `8CD5`,
`8CEE`, and `8D07`; their post-slot boundaries are `8C7D`, `8C96`, `8CAF`,
`8CC8`, `8CE1`, `8CFA`, and `8D13`. All use update callback `01F7:8D20` and
object class 1.

Runtime traces from controlled W1L1 anchor mutations reached each callback and
recorded the expected slot. `bob-find` resolves slots 600-606 to records 0-6
of `PUZZLE.BOB`, all 16x16 with origin `(0,0)`. Rendering them in slot order
produces `N`, `E`, `S`, `Q`, `U`, `I`, `K`, establishing the names
`puzzle_letter_N` through `puzzle_letter_K`. Types `0x73` and `0x74` have zero
occurrences in the bundled ARE archive. Controlled W1L1 injections reach the
common factory, but both dispatch slots are `0000:0000` with class `0`, callback
`0`, and sprite slot `0xFFFF`; they are confirmed unused/unimplemented types,
not unresolved sprite families.

Native-context renderer probes independently resolve map indices 80 through 86
and descriptor offsets 3520, 3564, 3608, 3652, 3696, 3740, and 3784. Every
live descriptor is `16x16` with origin `(0,0)`, matching PUZZLE.BOB records 0
through 6 and confirming the slot-to-descriptor stride.

The completion trigger is in the main selector `01D7`, not in the `01F7`
letter callback. The routine at `01D7:14E1` compares `DS:60D8` against
`0x007F` at `01D7:1670`; the taken branch `01D7:16C6-1704` renders the
`NESQUIK: 2000` and `BONUS-LEVEL!` strings, sets the sound action at `DS:612E`,
adds `0x07D0` to the score pair `DS:881C/DS:881E`, waits between messages, and
sets `DS:85DB=1`. Its caller immediately tests that flag at `01D7:4F10`, jumps
to `01D7:4FAF`, maps the selector state, and enters the reload/transition setup
at `01D7:5017` (relocated target `01F7:0908`) followed by the resource/object
handoff calls through `01D7:5047`. The final-letter synthetic run reaches the
mask and clears the object but does not execute this authored presentation
branch within its 1,800-frame window. A refined 300-frame probe armed only the
authored comparator, presentation, and transition addresses and also recorded
zero hits, so the static handoff is resolved while the fixture prerequisite or
exact timing remains open.
