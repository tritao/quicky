# Runtime descriptor construction findings

## Scope and evidence

This note covers the MAP tile descriptor table in `QUIKY.EXE`, not the
separate `0x2c`-byte BOB sprite records. Static decoding was performed against
`game/QUIKY.EXE` SHA-256
`c9b2e59febd6fa0ea271bedf360459353f55c74444f026964b70988d6de1bca1`.
Runtime checkpoints used selector-aware DOSBox reads on W1L1, W1L3, and W2L1;
the two complete construction traces are generated as
`research/build/runtime-descriptor/construction-w1l1.json` and
`construction-w2l1.json`.

The reproducible tooling is:

- `research/tools/descriptor_static_report.py` — decodes the five static
  world initializers and the two MAP-loader mutations.
- `research/automation/quiky_descriptor_construction_trace.lua` and
  `research/tools/run_descriptor_trace.py` — capture loader, mutation,
  publication, and initializer checkpoints. The runner accepts
  `DOSBOX_AUTOMATION_BIN` so it can reuse a debugger build from another
  worktree.

## Construction path

1. Segment `01E7`, routine `01E7:382B`, allocates `0x800` bytes at
   `01E7:3874`, publishes the far pointer at `DS:6582`/`DS:6584` (`387C` and
   `387F`), and clears the same `0x800` bytes (`3883..388E`). This is exactly
   512 records at the runtime stride `DS:30D4 = 4`.
2. Segment `01D7:3808` dispatches on `DS:85D8` (world 1..5) to
   `1734`, `19E4`, `1BF1`, `28ED`, or `2D9F` respectively.
3. Each initializer loops `tile_id = 0..0x1ff`. Its destination is
   `far(DS:6584, DS:6582 + tile_id * 4)`. The loop writes the identity
   `tile_id` at record `+0`, then writes a world-specific word at record `+2`
   from a compare ladder; the fall-through value is zero.

The new target decompilation confirms that `382B` is a broader startup
constructor, not a descriptor-only helper: immediately after publishing and
zero-filling `DS:6582:DS:6584`, it allocates several unrelated runtime
buffers, including a second `0x800`-byte buffer at `DS:6D86:DS:6D88`. The
world dispatcher at `01D7:3808` selects the five initializers from
`DS:85D8`, then zero-fills that separate `6D86` buffer. This prevents the
second allocation from being mistaken for a remap or a second descriptor
table. The generated pseudocode is retained in
`/home/joao/dev/quiky-ghidra-decomp-descriptor-20260826-b/QUIKY_SEG02.bin.c`
and `.../quiky-ghidra-decomp-descriptor1-20260826-b/QUIKY_SEG01.bin.c`.

The record schema is therefore:

| Offset | Size | Neutral field | Proven producer/meaning | Proven consumers |
| ---: | ---: | --- | --- | --- |
| `+0` | `u16` | `tile_index` | loop counter identity (`tile_id`) | renderers `01F7:20C8`/`2CB2` use it as a resource index, shifting it by 8 for the `0x100`-byte tile image block |
| `+2` | `u16` | `flags` | world-specific compare ladder; default `0` | `01F7:5C27` low-nibble quadrant tests; `01F7:5CC3` returns the word to `3D02` |

The low nibble is a four-way occupancy mask: `5C27` selects `0x02`, `0x01`,
`0x04`, or `0x08` from coordinate bit 3. In the `3D02` caller, `0x20` is the
vertical-response polarity/state selector and `0x40` selects the eight-pixel
vertical alignment. `0x10` participates in the caller's `DX & 0x30` gate:
either `0x10` or `0x20` suppresses the eight-pixel X retry, while `0x20` also
selects the response polarity. Static relocation filtering finds only two
direct `5CC3` calls, both at `01F7:3D19/3D31` inside `3D02`; the many other
`5C27` callers consume only the low-nibble occupancy mask. Thus the
mechanical roles of every descriptor flag bit are proven, but a standalone
gameplay name for `0x10` remains unjustified. “Floor” and “ceiling” remain
provisional labels for the combined branches.

The static consumer audit is deliberately narrower than a raw byte search.
Filtering the NE relocation records for the descriptor-query entries finds 78
direct calls to `01F7:5C27` and exactly two direct calls to `01F7:5CC3`, at
`01F7:3D19` and `01F7:3D31` inside `3D02`. The former exposes only the low
nibble/quadrant result; the latter is the only file-backed path that receives
the complete `+2` word. The renderers at `01F7:20C8` and `01F7:2CB2` then load
record `+0`, zero-extend it, and shift it left by eight to select the
corresponding `0x100`-byte image block. Because the initializers write
`+0 = tile_id`, this proves the identity field is the direct ICO/image
resource index; it is not merely an unused copy of the lookup key. No second
descriptor-`+2` consumer was found. This rules out a hidden renderer-side
interpretation of `0x10` in the executable. The only mechanically supported
name for that bit is therefore “suppress the `3D02` eight-pixel X retry”; a
more specific gameplay name still requires a distinct normal branch.

Static flag histograms show world-specific content, including `0x04` and
`0x08` in W3 and the common `0x0c/0x10/0x30/0x50/0x70` combinations. The
report emits all 512 records, including unreferenced/sentinel-range entries.

## Runtime mapping checks

The selector-safe W1L1 construction trace reports `DS:6582 = 0`, descriptor
selector `DS:6584 = 631`, stride `4`, and 512 entries. Every post-initializer
record matches the static W1 table (512/512 records). The W2L1 trace reports
the same base/selector/stride and matches the static W2 table (512/512).
W2's pre-initializer table is byte-for-byte the prior W1 table, demonstrating
that the published allocation is reused and overwritten by the world-specific
initializer rather than remapped to a new index table.

### All-world runtime construction census

Fresh selector-aware construction traces now cover one level in every world.
Each table has 512/512 records matching the corrected static compare-ladder
model, and every run publishes the same base/selector (`0:0x0277`) and stride
4:

| World/level | MAP row stride | MAP height | Descriptor match | First-row delta |
| --- | ---: | ---: | ---: | ---: |
| W1L1 | 540 | 30 | 512/512 | `0x1000` |
| W2L1 | 540 | 40 | 512/512 | `0x1000` |
| W3L1 | 500 | 50 | 512/512 | `0x1000` |
| W4L1 | 490 | 52 | 512/512 | `0x1000` |
| W5L1 | 300 | 86 | 512/512 | `0x1000` |

The W3 run exposed an important static-analysis edge case. Its ladder contains
duplicate exact compares for tile IDs `0x0da` and `0x0db`: the first stores are
`0x000c`/`0x0008`, while later unreachable duplicates would store
`0x0010`/`0x0050`. The decoder now preserves the first matching assignment and
reports the duplicate cases explicitly; the live W3 table confirms the first
values. This rule is required for faithful reconstruction of generated
compare ladders, even though the duplicate cases are rare. Full paths and
hashes are in `notes/descriptor-construction-evidence.json`.

The consumer formula is:

```text
raw_cell = MAP[(y >> 4) * DS:657E + (x >> 4) * 2]
tile_id  = raw_cell & 0x01ff
record   = far(DS:6584, DS:6582 + tile_id * DS:30D4)
flags    = record[+2]
```

The W1L1 property checkpoint reads raw cell `0x002e`, tile `46`, descriptor
offset `46 * 4 + 2 = 186`, and flags `0x000c`. The same formula is confirmed
for W2 by the complete runtime/static census. No remap table was found in the
construction or lookup paths.

Archive correlation also shows the direct-ID range is sufficient for the
bundled worlds:

| World | ICO tile records | Maximum MAP tile ID |
| --- | ---: | ---: |
| W1 | 480 | 468/479 |
| W2 | 400 | 397/399 |
| W3 | 480 | 469/479 |
| W4 | 480 | 467/479 |
| W5 | 440 | 418/439 |

The table remains 512 entries even where the ICO has fewer records; no loaded
MAP in the archive references those tail IDs.

## Runtime MAP mutation ledger

The primary loader is `01D7:365B`. It copies the big-endian MAP cells into
the protected-mode MAP buffer at `DS:657A/DS:657C`, then runs a loop over
`DS:657E / 2` cells (the first row). At `01D7:37AD` it reads the byte at
`MAP + 2*cell + 1`, executes `OR AL,0x10` at `37B0`, and writes it back at
`37C0`. Because this is the high byte of the runtime word, a clear bit changes
the CPU-visible MAP word by `0x1000`, not by `0x0010` and not in the low-nine-bit
tile ID.

Runtime pairs:

| Level | MAP dimensions/stride | Before | After write | Delta |
| --- | --- | ---: | ---: | ---: |
| W1L1 | `270 x 30`, stride `540` | `0x0001` | `0x1001` | `0x1000` |
| W2L1 | `270 x 40`, stride `540` | `0x005c` | `0x105c` | `0x1000` |

The OR checkpoint sees `AL = 0x10`; the after-write checkpoints are `01D7:37C3`
and loop completion is `01D7:37CB`. The W1L1 trace shows all 270 first-row
cells carrying the mutation after the loop. The archive first cells are
`0x0001` and `0x005c`, so the copy and mutation are directly correlated.

The secondary loader is `01D7:3861`; it has the same first-row loop at
`394C/394F/3960` and the same `0x1000` word effect. Target decompilation of
the transition scheduler at `01D7:48B5` now identifies its role: after the
`4BA4` gate (`DS:89EA != 0`, `DS:880A > 0`), the `4BD8` branch calls `3861`
only for selectors `2`, `5`, `8`, `0x0b`, and `0x0e` (`4BF1/4BFB/4C05/4C0F/4C19`).
Because the selector table is zero-based, these are W1L3 through W5L3. The
primary setup callsite at `01D7:4009` calls `365B` before the level-specific
ICO/BOB asset loads. The distinction is therefore initial MAP construction
versus the third-level transition/reload path, not an unknown second layer.
A normal-lifecycle probe
for W1L3 did not reach the call sites: after the selected level transition the
engine repeatedly hit `01F7:1AE6`, which writes `DS:89EA = 0`, then remained
at the `01D7:48BB` wait loop. The object count was `DS:880A = 4`; no `4BD8`
gate or secondary call site was observed. A separate
controlled trace injected a same-segment trampoline at the stopped initializer
return (`01D7:19E3 -> 01D7:51E0`) and called `3861` with the already-published
MAP. This reached `01D7:3861` and observed W1L3 (`55 x 60`, stride `110`,
selector `887`) changing the first cell `0x0001 -> 0x1001` (`+0x1000`). The
injection is recorded in `construction-w1l3-secondary-controlled-call.json`
and is not normal-gameplay call attribution.

The construction runner's secondary switch was corrected to select `3861`
instead of silently reusing the primary loader. A fresh natural W1L3 run with
that switch enabled, a 128-lookup guard, and a 60-second wait still timed out
in the resource-lookup loop before the `3861` call/entry. This independently
reproduces the lifecycle boundary rather than relying on the earlier probe's
hard-coded primary path; its failure artifact is recorded in
`notes/descriptor-construction-evidence.json`.

### Complete static MAP-writer audit

The file-backed segments were searched for every use of the loaded MAP far
pointer (`DS:657A/DS:657C`, row stride `DS:657E`) and for every store through
the resulting `ES`/`FS` selector. Besides the two loaders, the following cell
writers are present:

| Entry | Store instructions | Operation | Coverage / caller evidence |
| --- | --- | --- | --- |
| `01F7:16CE` | `1706`, `170E` | `(MAP & 0xfe00) \| (DX & 0x01ff)`; skipped when `DX & 0x8000` | one coordinate-selected cell; 23 direct NE calls: two transient-event paths, one short animation path, and the 20-call `8E4B` tile-effect state machine |
| `01F7:33BF` | `33FA`, `33FF` | for low IDs `2..4`, reconstruct `(MAP & 0xfe00) \| ID` | whole MAP (`DS:657E / 2 * DS:6580` cells); called by `01D7:37CB` and `396D` immediately after both loader mutation loops; idempotent for a well-formed word |
| `01F7:339A` | `33B4`, `33B9` | `(MAP & 0xfe00) \| CX` | one coordinate-selected cell; no direct NE relocation caller; caller supplies the low-ID bits |
| `01F7:340A` | `3424`, `3429` | `(MAP & 0x01ff) \| CX` | one coordinate-selected cell; no direct NE relocation caller; caller supplies the upper-property bits |
| `01F7:5C9D` | `5CBE` | full-word `FS:[MAP + (y >> 4) * stride + ((x >> 3) & 0xfffe)] = CX` | one coordinate-selected cell; no direct NE relocation caller; controlled write only |

The first two rows of this table are easy to miss if the audit only watches
the known `OR 0x10`: `33BF` is called from both loader returns and performs a
post-copy whole-MAP pass, while `16CE` is the direct gameplay tile-effect
writer. The coordinate writers at `339A` and `340A` preserve the complementary
MAP bits but do not mask `CX` themselves; the caller must supply only the
intended low-ID or upper-property bits. They have complete static bodies but no
NE relocation record of any source type and no literal target-offset byte pair
in the file-backed segments. The same stronger negative applies to `5C9D`; an
indirect or runtime-generated caller remains possible only if it constructs the
target at runtime. No other `ES`/`FS` store was found that is preceded by the
loaded-MAP selector; the remaining selector stores target object, sprite, or
render buffers rather than `DS:657A/DS:657C`.

The analyzed segment-3 call-form census narrows that caveat. It finds only two
indirect `CALL` forms, at `01F7:040F` and `01F7:0598`, both through a local
`[BP-0x12]` stack slot; neither resolves to a MAP writer. The 23 `16CE` calls
remain NE-relocated far calls, while `339A`, `340A`, and `5C9D` have no direct
or indirect static call form. This still cannot exclude a pointer constructed
by runtime data, but it removes the remaining file-backed indirect-call shape
from the likely writer paths. The machine-readable result is
`map_writer_call_form_audit` in the evidence JSON.

### Controlled tile-effect state-machine MAP mutations

The 20-call `01F7:8E4B` path is now mechanically reproduced with a controlled
W1L1 archive variant. A type-`0x20` record at `0x1930` is redirected into the
startup stream, and a debugger-only MAP seed places tile IDs `0xc8..0xcc` in
the queried cells. In each sampled state, `01F7:3376` returns the source tile,
`DS:6986` supplies the effect value `0x78..0x7c`, and `01F7:16CE` writes that
value back to the same MAP cell before `01F7:171C` allocates a transient effect
object.

| State reached | Position / queried row | MAP offsets | Before → after |
| ---: | --- | --- | --- |
| 4 | `(768,224)`, `y=224` | `7656..7664` | `0x00c8..0x00cc → 0x0078..0x007c` |
| 6 | `(768,240)`, `y=256` | `8736..8744` | `0x00c8..0x00cc → 0x0078..0x007c` |
| 8 | `(768,256)`, `y=288` | `9816..9824` | `0x00c8..0x00cc → 0x0078..0x007c` |

All fifteen writes have delta `-0x50`; the writer preserves MAP bits 9..15
and replaces only the low-nine-bit tile ID, as established statically. The
three trace hashes and exact per-cell records are in
`state_machine_effect_probes` in `notes/descriptor-construction-evidence.json`.
These runs use a redirected archive record, forced state entry, and debugger
MAP patch, so they establish the effect/state/MAP mechanics without claiming
that this exact trigger occurs during an unmodified level.

### Upper MAP field read audit and collision confirmation

The targeted Ghidra output and the raw-segment pointer-reference survey close
the most important ambiguity around the seven upper MAP bits. Every identified
MAP reader masks the raw word with `0x01ff`: `3376` returns the tile ID,
`5C27` and `5CC3` index the descriptor table, and renderers `20C8` and `2CB2`
index tile imagery. The writers preserve or replace the upper field, but no
identified gameplay reader interprets it. This is a static-audit boundary:
an indirect or runtime-generated reader remains theoretically possible, but no
direct raw-segment consumer is evidenced.

Selector-safe W1L1 traces provide the corresponding runtime check. The stable
baseline reads raw MAP `0x002e`, tile `0x02e`, descriptor `0x000c`; corrected
left-wall and right-input samples read raw/tile `0x002d`/`0x02d`, also with
descriptor `0x000c`; jump samples reach tiles `0x001` and `0x01a`, both with
descriptor `0`. These values come from the protected-mode MAP and descriptor
selectors, not flat-selector reads.

Debugger-only descriptor substitutions then isolate the consumer behavior
without changing the archive. Replacing the live `3D02` probe tile with
`0x02a` (descriptor `0x0070`) reaches `3DF1` with `AL=1` and
`object+0x3a=0xff`; replacing it with `0x02b` (descriptor `0x0030`) reaches
`3DE4` with `AL=0` and `object+0x3a=0`. The MAP word is restored after each
probe. At the `3DF2` side probe, replacing the `(x-5,y)` cell with either tile
causes the helper to return `ZF=1` and permits the `(x+5,y)` probe; the
unpatched `0x02d` cell returns `ZF=0` and short-circuits the right probe. A
`both` run reaches both patched probes independently. This establishes tile
to descriptor dataflow and the helper's flag polarity, while the gameplay name
of the high descriptor bits remains deliberately conservative.

The raw source artifacts for this pass are retained in the main research tree
under `/home/joao/dev/quicky/research/build/`, including
`player-property-w1l1-selector-safe-{baseline,left,right,jump}.json`,
`player-branch-w1l1-patched-tile2a.json`,
`player-branch-w1l1-patched-tile2b-v2.json`, and the paired `0x2a`/`0x2b`
side-probe traces. Their executable hash is the same research binary used by
the construction traces. The curated static classification is also emitted by
`descriptor_static_report.py` under `map_read_audit`.

#### Fresh X-retry control pair

The current-worktree DOSBox build was then used for a three-way control at the
same W1L1 probe point (`x=128`, `y=400`, selector `0x0377`, offset `0x34cc`).
Each run patches only the live MAP cell and records the descriptor word at the
`3D02` branch; the original cell is restored by the tracer.

| Patched tile | Static W1 flags | `DX` at `3D1E`/`3D45` | Observed path | Result |
| ---: | ---: | ---: | --- | --- |
| `0x028` | `0x10` | `0x0010` | `3D02 -> 3D1E -> 3D45 -> 3DD0 -> 3DE4` | negative (`AL=0`, `object+3a=0`) |
| `0x029` | `0x50` | `0x0050` | `3D02 -> 3D1E -> 3D45 -> 3DD0 -> 3DE4` | negative (`AL=0`, `object+3a=0`) |
| `0x160` | `0x00` | `0x0000` | `3D02 -> 3D1E -> 3D36 -> 3D40 -> 3D44` | clear/negative return (`AL=0`) |

This isolates the mechanical meaning of the `0x10` gate: with `DX & 0x30`
nonzero, the helper does not enter the eight-pixel retry path (`3D36`/`3D40`).
The `0x29` run also shows that `0x40` does not override that suppression when
`0x10` is present. With a zero descriptor the retry executes and temporarily
updates the sampled Y from `400` to `392` before the `3D44` return. The three
raw traces and hashes are recorded in
`notes/descriptor-collision-evidence.json` as
`current_trace_flags_0x10`, `current_trace_flags_0x50`, and
`current_trace_flags_0x00`.

The tracer now supports a descriptor-only patch, leaving the MAP tile ID at
`0x02e` and changing only record `+2` (offset `0x00ba`). This isolates the two
vertical bits without relying on a world tile that happens to carry one bit:

| Patched descriptor | Retry path | Return | Interpretation |
| ---: | --- | --- | --- |
| `0x20` | suppressed (`3D45`) | `3DE4`, `AL=0` | alternate vertical response alone is insufficient |
| `0x40` | executed (`3D36`/`3D40`) | `3D44`, `AL=0` | Y alignment alone is insufficient |
| `0x60` | suppressed (`3D45`) | `3DF1`, `AL=1`, `object+3a=ff` | `0x20 | 0x40` is the positive branch pair |

This is stronger than substituting tiles `0x02a`/`0x02b`: the MAP word and
tile identity remain unchanged, so the result is a direct consumer test of
descriptor `+2`. The raw traces and hashes are
`direct_descriptor_flags_0x20`, `direct_descriptor_flags_0x40`, and
`direct_descriptor_flags_0x60` in `notes/descriptor-collision-evidence.json`.

A normal held-right W1L1 trace now reaches the same positive branch without a
debugger patch. At player position `(1570,354)`, the collision probe at
`(1575,354)` reads raw MAP `0x202a` (upper property field `0x0010`), masks to
tile `0x02a`, loads descriptor `0x0070`, and returns through `3DF1` with
`AL=1`/`object+3a=ff`. This is a live separation of the two fields: the MAP
upper bits are present in the raw word, but the direct collision data flow
still selects the descriptor solely from the low-nine-bit tile ID.

The same natural pass also reaches tile `0x028` at player `(1080,380)` and
probe `(1085,380)`: raw MAP `0x2028` (upper field `0x0010`) maps to descriptor
`0x0010` and returns through `3DF1` with `AL=1`/`object+3a=1`. This rules out
the over-strong interpretation that `0x10` intrinsically means “negative”;
its proven standalone role remains suppression of the eight-pixel X retry,
while the final correction depends on the target coordinate and other state.

The low-nibble helper's target disassembly also fixes the occupancy-bit
orientation. With `AX` as the 16-pixel row coordinate and `BX` as the
8-pixel-aligned column coordinate, `AX.bit3/BX.bit3 = 11, 10, 01, 00` selects
descriptor bits `0x02`, `0x01`, `0x04`, and `0x08`, respectively. A zero low
nibble returns clear; otherwise only the selected bit is tested. The static
report emits this mapping under `flag_consumers.low_nibble_query`.

A trajectory-matched control removes the remaining stationary-probe confounder.
At sequence 6/frame 1025 of the same held-right run, the player is descending
at `(1573,356)` with vertical velocity `0x00009000` into tile `0x02a`. The
natural descriptor is `0x70`; replacing only that live record with `0x10` leaves
the incoming `3D02` register stale at `0x70`, but the descriptor lookup and all
later branch events carry `0x10` and return through `3DE4`. The unpatched
trajectory returns through `3DF1`. This proves that the consumed descriptor
word, not the motion phase, controls the retry suppression. The exact artifact
and hash are the `vertical_motion_patch_flags_0x10` case in
`notes/descriptor-collision-evidence.json`.

The static body around `01F7:3D02` also gives the coordinate reconstruction
needed by a faithful collision implementation. Let `input_y` be the helper's
vertical input and `x`/`y` the object coordinates:

```text
if (descriptor & 0x20) {
    vertical_response =  input_y >> 1;
    vertical_state    = 0xff;
    phase             = (x & 0x0f) >> 1;
} else {
    vertical_response = (-input_y) >> 1;
    vertical_state    = 1;
    phase             = (0x0f - (x & 0x0f)) >> 1;
}
target_y = (y & 0xfff0) + phase;
if (!(descriptor & 0x40))
    target_y += 8;
```

The code is still a collision-helper reconstruction, not a semantic claim that
`0x20` means “ceiling” or that `0x40` means “floor”. Those labels require a
normal, non-patched gameplay trace with a known surface orientation. The
apparently counterintuitive `+8` is the raw `SUB BX,0xFFF8` encoding: the
8-bit immediate is sign-extended to `-8`, so subtracting it adds eight.
The instruction is `01F7:3DD6` (`83 EB F8`) in the segment-3 bytes.

Target decompilation closes the remaining ambiguity around player object
`+0x3a`. `3D02` clears it on entry, sets it to `0x01` or `0xff` while taking
the two `0x20` response-polarity branches, and clears it again when the
computed target is rejected (`original_y < target_y`). The only identified
consumer is `3DF2`, which tests the byte only for zero/nonzero before deciding
whether to perform the eight-pixel X snap. It is therefore a transient
accepted-vertical-response latch, not a persistent floor/ceiling or MAP
surface-type field; the `0x01` versus `0xff` values preserve the branch's
polarity for any later code that needs the byte value.

The focused gate-write diagnostic resolves why the unmodified probe stops.
The launch path at `01D7:4B6E` calls `01F7:1AAA`; its `01F7:1AE6` instruction
then writes `DS:89EA = 0` before returning to `01D7:4B73`. The main loop tests
that word at `01D7:4BA4`; zero branches directly to `4C43`, so the later
`01D7:4BD8` test and the five `3861` call sites (`4BF1/4BFB/4C05/4C0F/4C19`)
are not reached. `DS:880A` is `4` in the same transition, so the object-count
condition is present but the scheduler-gate condition is not. The other static
`DS:89EA` writers are the `19E6` state routine (`19A3`/`1A3D` set `0xffff`)
and `44DC` (decrement); their existence does not imply that either path runs
during this level-launch transition.

Runtime W1L3 and W2L3 diagnostics recorded the same sequence: `1AE6` with a
return address of `01D7:4B73`, `01F7:3F38` clearing `DS:8810` during player
initialization, then `01D7:48B5/48BB` clearing and waiting on `DS:819E`.
Neither run reached `4BD8` or a secondary call site. Artifacts are
`secondary-transition-writes-w1l3-v2.json` and
`secondary-transition-writes-w2l3.json`; the trace now records stack return
addresses and all identified writes to `DS:8810`, `DS:88BA`, `DS:880A`, and
`DS:89EA`.

An extended diagnostic also armed the `19E6` entry and callers (`1BC5`/`3AAF`),
the `199D`/`43D1` path, `44DC`, `4BD8`, `3861`, and the normal `5C9D` entry.
The new W1L3 flow trace reaches the natural post-wait sequence
`48E6 -> 493E -> 4968 -> 4BA4 -> 4C43 -> 4CFC -> 4EA0 -> 4EAA` with
`DS:89EA=0`, `DS:89E6` briefly `0xffff`, and `DS:880A=4`; it then times out
without any `19E6`, `19A3`, `1A3D`, `199D`, `44DC`, `4BD8`, `3861`, or `5C9D`
hit. A debugger-only patch replacing the `48BB` compare/JZ with a jump to
`48C2` likewise did not reach the later gate checks within the trace window
(`secondary-gate-skip-wait-w1l3.json`). This is a negative result: the tested
W1L3 post-wait path does not restore `DS:89EA`; the asynchronous transition
and its subsequent pending-state branch remain a barrier. The patch was not
used as lifecycle evidence. The unmodified flow artifact is
`research/artifacts/runtime-descriptor-construction/w1l3-next-natural-flow.json`.

The owner of `DS:819E` is now identified. Static file-backed writers are:
`01D7:48B5` (the level-loop clear), `01F7:F049` (timer interrupt writes
`1`), `0207:0014` (timed-wait clear), and `0207:101F` (timer routine clear;
its loop tests `DS:819E` before returning). The `F049` routine ends with an
interrupt return, confirming that the nonzero value is supplied by the timer
IRQ rather than by the level-state code. W1L3 and W2L3 runtime traces both
recorded `F049 -> 1AE6 -> 0207:101F -> 0207:0014 -> 48B5 -> 48BB`, with the
timer writer observed before the final wait test. The artifacts are
`secondary-gate-owner-w1l3.json` and `secondary-gate-owner-w2l3.json`.

A normal lifecycle run with no diagnostic breakpoint at `48BB` still timed out
at `48BB` without reaching `4BD8` or `3861`; this indicates the timer/transition
sequence is not completing in the current automated level-launch session even
though the timer writer itself is present.

The non-stopping post-wait trace clarifies the boundary: execution does reach
`01D7:48C2` when `48BB` is allowed to run, then calls `01F7:3062` (the first
post-wait helper, returning to `01D7:48C7`). It still does not reach the main
loop gate at `4BA4`, `4BD8`, or `3861` in the trace window. Static NE relocations
show the timed-wait entry `0207:0002` is called from several delay sites,
including `01D7:4EDD`/`4EE6`; `0207:101F` is called by its timer routine at
`0207:10A3`. The post-wait artifact is
`secondary-post-wait-w1l3-v3.json`.

The following state checks are now bounded as well. After the helper returns,
`01D7:48CC/48DC/48E6` runs; `48E6` tests `DS:89E6` and branches to `4968` only
when it is `0xffff`. The W1L3 trace reaches `48E6` with `DS:89E6 = 0`, so
`4968` is not taken and no secondary-loader gate is evaluated. Static writers
that can set this event flag to `0xffff` are `01D7:493E` (a main-loop input
branch) and `01F7:4996`, `4AAC`, and `92A9` (state/collision paths). None fires
during automated level launch. This shifts the remaining lifecycle question
from timer ownership to the gameplay/state event that sets `DS:89E6`.

The static helper `01F7:5C9D` is now exercised by a separate controlled trace.
It receives `(AX=y, BX=x, CX=value)`, computes
`(y >> 4) * DS:657E + ((x >> 3) & 0xfffe)`, adds `DS:657A`, loads `FS` from
`DS:657C`, and stores `CX`. With W1L3 inputs `(x=0x45, y=0x123,
value=0xa55a)`, `01F7:5CBE` writes selector `887`, offset `0x07c4` (1988),
changing `0x5001 -> 0xa55a`; the computed and observed offsets match. The
entry/write/return checkpoints are `5C9D/5CBE/5CC1`. This proves the helper's
address and write semantics, but the injected call does not establish a
normal-gameplay caller.

The NE relocation survey found zero direct far-call relocations targeting
`01F7:5C9D` in segments 1, 2, or 3, and no embedded `9d 5c` target bytes in
the file-backed segments. A normal-right W1L1 transition probe also armed
`5C9D` for 220 breakpoint passes (202 player callbacks, 16 overlap/state
updates, and the normal `4BD8` gate) without observing the helper. The W1L3
post-wait flow probe likewise saw no `5C9D` hit. An indirect/runtime-generated
caller therefore remains the only unexcluded possibility; no normal caller is
currently evidenced.

A dedicated writer-focus lifecycle run broadened the bounded runtime check to
all three uncalled entries. It launched W1L1 with real `KBD_right` held from
launch, armed only `339A`, `340A`, and `5C9D`, and allowed a 30-second window
without player/transition breakpoints. None fired. At timeout the run had
`DS:880A=3`, `DS:89EA=0xff41`, `DS:819E=0`, and `DS:89E6=0`, so the engine was
past the initial four-object state and the scheduler gate was active. This
strengthens the normal-gameplay negative for the window but still does not
exclude a later or runtime-constructed caller. The artifact is
`research/build/writer-focus-w1l1-right-30s-20260825.json`.

The same writer-focus mode was run for W1L3. No `339A`, `340A`, or `5C9D`
entry fired in 30 seconds; the selected level remained `DS:85D4=2` with
`DS:880A=4`, `DS:89EA=0`, and `DS:819E=1`, matching the known launch wait.
This is negative evidence for the pre-secondary-loader window only, not for
later W1L3 gameplay after the scheduler barrier. The artifact is
`research/build/writer-focus-w1l3-right-30s-20260825.json`.

As a path-control comparison, a 60-second W1L3 run held `DS:89EA=1` and
NOPed the launch clear at `01F7:1AE6`, then watched only the same three writer
entries. None fired; the run reached `DS:880A=3`, `DS:89EA=0xfef6`, and
`DS:819E=0`, showing that the gated transition advanced beyond the four-object
launch state. This is controlled evidence only, but it rules out those three
writers during the exercised post-gate window. The artifact is
`research/build/writer-focus-w1l3-force-gate-60s-20260825.json`.

Thirty-second normal held-right writer-focus windows on W2L1, W3L1, W4L1, and
W5L1 also recorded zero hits for `339A`, `340A`, and `5C9D`. These runs cover
one level in every world family and broaden the bounded normal-input negative;
later event-specific or runtime-generated callers remain unexcluded. Their
artifacts and hashes are listed under `writer_focus_other_world_l1_probes` in
`notes/descriptor-construction-evidence.json`.

The descriptor-flag matrix was repeated against W1L1 at MAP offset `0x34cc`
(coordinate `(128,400)`). The adjacent records produce:

| Tile | Descriptor flags | Branch return | AL | `object+0x3a` |
| ---: | ---: | ---: | ---: | ---: |
| `0x028` | `0x0010` | `3DE4` | `0` | `0` |
| `0x029` | `0x0050` | `3DE4` | `0` | `0` |
| `0x02A` | `0x0070` | `3DF1` | `1` | `0xFF` |
| `0x02B` | `0x0030` | `3DE4` | `0` | `0` |

This isolates the combined `0x20 | 0x40` requirement for this branch:
`0x10` alone, `0x40` without `0x20`, and `0x20` without `0x40` all reject.
Each run read back the patched tile and restored the original MAP word. Raw
artifacts are `branch-patch-tile-028.json`, `branch-patch-tile-029.json`,
`branch-patch-tile-02a.json`, and `branch-patch-tile-02b.json`.

The missing transition event is now reproduced with a real keyboard input. The
segment-3 keyboard consumer at `01F7:F1A8` copies the make scan byte to
`DS:88BA`; its arrow/Space translation only populates `DS:88BC`. Therefore
`DS:88BA == 3` is the raw make code for the `2` key. A W1L3 diagnostic queued
`KBD_2` after level launch and observed `last_key=3`, then the normal
`01D7:493E` branch wrote `DS:89E6=0xffff` and `01D7:4968` was reached. This is
normal input evidence for the event writer, although the surrounding launch
run still uses diagnostic breakpoints.

The same run with the scheduler gate held at its consumer reached the complete
downstream chain: `493E -> 4968 -> 4BA4 -> 4BAE -> 4BD8 -> 3861`. At
`4BA4`, the diagnostic had to reassert `DS:89EA=1` because the state callback
had decremented it at `44DC`; this is a controlled scheduler aid, not a claim
that an unmodified launch naturally reaches `3861`. The raw evidence is
`research/artifacts/runtime-descriptor-construction/w1l3-post-kbd2.json` and
`w1l3-post-kbd2-force-scheduler-v2.json`.

For an independent consumer-path check, a separate run forced only
`DS:89E6=0xffff` while stopped at the `01D7:48E6` compare, then reasserted the
scheduler gate at `4BA4`. It reached `4968`, `4BD8`, and `3861` without any
input writer firing. This confirms the event flag's downstream branch and is
explicitly controlled evidence; it is recorded in
`research/artifacts/runtime-descriptor-construction/w1l3-force-event.json`.

The follow-up normal-input test held `KBD_right` both from launch and after
the `KBD_2` event while watching `01F7:19E6`, `19A3`, `1A3D`, `199D`, and
`44DC`. None fired; `DS:89EA` stayed zero after `01F7:1AE6`, and the run
reached `4BA4` but not `4BD8` or `3861`. This rules out the tested player-input
path as the missing scheduler-gate restoration in the automated W1L3 launch.
The run is preserved as
`research/artifacts/runtime-descriptor-construction/w1l3-kbd2-gameplay-right.json`
and
`w1l3-kbd2-gameplay-right-at-launch.json`. The result is negative lifecycle
evidence, not proof that no other state or level-completion path can set the
gate.

The static caller audit now has exact gate semantics from target decompilation.
NE relocations show direct calls to `01F7:19E6` only at `01F7:1BC4` and
`01F7:3AB3`; the first is the overlap helper's `DS:8810 == 0` branch, and
the second follows the motion helper's tile-ID checks for `0x0b/0x0c/0x0d`.
The only direct call to `01F7:199D` is at `01F7:43D0`, inside the player
callback's boundary check `object+08 - DS:81C4 >= DS:81CC`. `199D` clears
`DS:8950`, sets `DS:89EA=0xffff`, resets player motion fields, and decrements
`DS:880A`; `19E6` sets the same gate after its overlap counter reaches zero.
These are the confirmed scheduler-gate setter paths, but none is reached by
the tested W1L3 launch before the secondary-loader wait.

A controlled W1L3 run armed `3FF8`, `43D0`, and `199D` and forced the live
player Y word to `0x7fff` at the first callback. It confirmed the player
record and injection point but still stopped in the post-event secondary
loader before a second callback or `199D`; therefore it does not claim a
positive boundary transition. The artifact is
`research/artifacts/runtime-descriptor-construction/w1l3-force-player-fall-v2.json`.

The player-focused trace was moved to W1L1, where the update loop runs without
the W1L3 loader barrier. Holding real `KBD_right` produced repeated runtime
`01F7:1BC4 -> 01F7:19E6` overlap/state-update pairs while the camera advanced
from `x=214` to `x=220`. Throughout those events `DS:89EA=0`; no `19A3`,
`1A3D`, `44DC`, `43D0`, or `199D` event occurred. Downward and Space-input
comparison runs likewise did not reach the boundary writer. The right-input
long trace is
`research/artifacts/runtime-descriptor-construction/w1l1-transition-right-repeat-v2.json`;
the shorter right, down, and Space traces are
`w1l1-transition-right.json`, `w1l1-transition-down.json`, and
`w1l1-transition-space.json`. This establishes the overlap caller as a real
player path but rules it out as the scheduler-gate restoration in the tested
W1L1 movement window.

The W1L1 controlled boundary test now reaches the candidate path directly.
At the first normal `01F7:3FF8` callback, the probe set the player Y word to
`0x7fff`; execution then hit `01F7:43D0`, `01F7:199D`, `01F7:19A3`, and
`01F7:44DC` in order. At the `44DC` breakpoint, `DS:89EA=0xffff` and
`DS:880A` had decreased from `4` to `3`. This proves the boundary/death path's
gate write and object-count effect at runtime, but the Y injection is
debugger-controlled and is not normal-gameplay attribution. The artifact is
`research/artifacts/runtime-descriptor-construction/w1l1-transition-force-fall.json`.

A sustained, unmodified W1L1 run with held `KBD_right` now reaches a normal
state-machine transition naturally. After a 1409-frame warm-up, the trace
observed `1BC4 -> 19E6`, then `01F7:1A3D` writing `DS:89EA=0xffff`; the next
player callback entered with `DS:89EA=0xffff` and `44DC` decremented it once
per callback. At the write, the camera was `x=1943` and `DS:880A` had changed
from `4` to `3`, establishing a natural `1A3D -> 44DC` countdown path without
debugger memory injection. This is distinct from the debugger-forced
`43D0 -> 199D -> 19A3` boundary path. Longer natural-right traces reached the
segment-1 `01D7:4BD8` secondary gate while the countdown was nonzero
(`DS:89EA=0xfea2`, camera `x=1943`, object count `3`). The exact-writer artifact is
`research/artifacts/runtime-descriptor-construction/w1l1-transition-right-natural-warmup1409.json`;
the gate observation is in
`research/artifacts/runtime-descriptor-construction/w1l1-transition-right-natural-probe10.json`.

The W1L3 pending-state barrier is now bounded statically and dynamically. The
segment-1 state words have these direct roles in the main loop:

| Word | Static producers | Main-loop consumers |
| --- | --- | --- |
| `DS:89E0` | clears at `4B82`, sets `0xffff` at `4E10`, `4E8E`, `4ECC`; later clears at `50E7` and `517C` | `5010`, `5056`, `5173`, `5203` |
| `DS:89E6` | clears at `4B87` and `504C`; input/event writer at `493E` | `48E6` and `4EA0` |
| `DS:89EC` | sets `0xffff` at `4B4F`, `4C3A`, `4CA8`, `4CF3`, `5023`; clears at `50EC`, `5181`, `520C` | `48DC` and `504F` state gates |
| `DS:85DA` | set/clear at `50D0`/`5184`, increment at `4E94` | byte tests at `4CFC`/`4D06` |
| `DS:85D6` | copied from `DS:85D4` at `4FB2` | transition-selection compares at `4F22`/`4F3E`/`4F5A`/`4F76`/`4F92` |

A focused W1L3 run with real `KBD_2` followed by held `KBD_right` reached the
natural event and pending chain:

`48C2 -> 48CC -> 48DC -> 48E6 -> 493E (DS:89E6=FFFF) -> 4968 -> 4BA4 -> 4C43 -> 4CFC -> 4EA0 -> 4EAA`.

The staged tail then observed `4EBF -> 4EC5 -> 4ED2 -> 4ED7 -> 4ED9`, where
`4EDD` calls timed wait `0207:0002`; after that wait it reached `4EEB -> 4EF0 -> 4F0D`, and `4F0D` entered local transition/intro routine
`14E1`. That routine ran through `14F0`, `150C`, `151F`, `1524`, `1529`,
`153D`, `1547`, `154C`, `155A`, `1566`, `1587`, `158E`, `1593`, `15AE`,
`15B5`, and `15C5`, then entered `0207:0002` again at timer test
`0207:0017`. It did not return to `504F` or reach `4BD8/3861` within the
diagnostic window. The raw staged trace is
`research/artifacts/runtime-descriptor-construction/w1l3-pending-focus-kbd2-right-v15.json`.

This separates the pending barrier from descriptor construction: the tested
W1L3 path is executing a transition presentation/timed-wait state machine
after `4EAA`, not immediately dispatching the secondary MAP loader. The
`--pending-focus` probe's timer-release action is debugger-only and remains a
diagnostic aid, not normal-gameplay evidence.

To remove breakpoint perturbation, a separate controlled-input run held real
`KBD_2` from the level-dispatch stop until `01D7:493E`, released it at that
event writer, and then waited 120 seconds with only the secondary-loader
call/entry breakpoints armed. The event writer was reached, but `4BD8/3861`
were not; the final CPU was inside the timed-wait routine at `01D7:0207:0023`.
This confirms that the pending timer barrier persists after the input is
consumed and is not merely an artifact of releasing the key at the first
player callback. The failure artifact and hash are recorded as
`secondary_event_uninstrumented_probe` in
`notes/descriptor-construction-evidence.json`.

The corrected timer-focused variant armed the IRQ entry `01F7:F049` while
waiting and captured 16 IRQ hits before and 16 after `493E`; it still did not
reach `4BD8/3861`. Its final CPU sample was `0207:10C8`, inside the PIT-delay
helper path. Targeted segment-4 decompilation now makes the wait mechanism
explicit: `0207:0002` clears `DS:819E` for each requested frame and yields
until it becomes nonzero; `0207:101F` clears the same flag and polls PIT
channel 0, while `0207:10A9` samples/reprograms the PIT divisor. The timer
audit therefore rules out a missing IRQ as the sole explanation; the
unresolved state is the timed-wait/PIT flag-sampling interaction. The artifact
and static source are recorded under `secondary_timer_audit_probe` and
`timer_wait_static_decomp` in the evidence JSON.

The IRQ entry was decompiled separately from segment 3. `01F7:F049` writes
`DS:819E=1`, writes `DS:5014=0xffff`, conditionally calls the optional far
callback at `DS:8952:DS:8954`, then sends the PIC EOI and returns with `IRET`.
The follow-up target pass identifies that pair as audio/resource-owned: the
startup constructor `01E7:382B` clears it before `01E7:36ED` validates the
`.\\Score.DAT` resource, and the SAM/TFX loader `01E7:085E` temporarily clears
and restores `DS:8954`. Transition code at `01D7:4853` disables the callback
segment with `0xffff`, while `01D7:496F` clears it during dispatch. The static
source is recorded as `timer_irq_static_decomp` and
`timer_callback_pair_static_decomp` in the evidence JSON.

A post-barrier audit then removed every diagnostic stop at `01D7:48BB` and
waited only for the real IRQ entry. W1L3 delivered 16 more `01F7:F049` hits;
each entry saw `DS:8952:8954 = FFFF:FFFF`, the disabled audio/resource callback
sentinel, while `DS:89EA` and `DS:89E6` stayed zero. Thus the timer is alive and
raises the frame flag, but this audit says nothing about a missing transition
callback: the pair is not the transition state channel. The remaining
lifecycle gap is the independent `89EA`/`89E6` setter and consumer path, not
absent IRQ delivery or an unregistered timer callback. This controlled result
is recorded as `timer_post_wait_audit` in the evidence JSON.

A fresh unmodified W1L3 timer-state trace records `DS:8952:DS:8954 =
0x0000:0xffff` through the IRQ/PIT sequence, then `0xffff:0xffff` at
`01D7:48B5/48BB` when the transition loop disables the callback segment. At
the final wait test `DS:89EA`, `DS:89E6`, and `DS:819E` are all zero. This
runtime ordering matches the static ownership result and is curated as
`timer_callback_pair_runtime_probe` in the evidence JSON.

The new unmodified state traces for W1L3 through W5L3 observe the same sequence:
`F049 -> F04F -> 504F -> 10A3 -> 101F -> 0002 -> 0014 -> 0017 -> 001E ->
48B5 -> 48BB`. At `0014` the flag is still `1` before the clear instruction;
at `0017` it is `0`, and the same before/after pattern appears at `48B5/48BB`.
None reaches `4BD8/3861`. These are unmodified lifecycle traces, not
gate-forced runs, and are recorded as `timer_state_trace_probes` plus
`timer_state_trace_additional_probes`.

The normal W1L1 writer audit was also widened to include `01F7:16CE`. Across
96 held-right transition-focus events, camera X ranged from 0 to 2309 and the
object count changed from 4 to 2; `16CE`, `339A`, `340A`, and `5C9D` all had
zero hits. This is bounded ordinary-movement evidence only; a later
event/effect campaign is still needed before treating the runtime MAP mutation
ledger as complete.

That probe was then repeated for all five W1L1 world families. Every normal
launch reached `16CE` from the common continuation `01F7:1897` and rewrote one
cell: W1 `(23,19)` `0x0002 -> 0x01d7`; W2 `(16,19)` `0x0003 -> 0x01e5`; W3
`(23,14)` `0x0002 -> 0x01d8`; W4 `(32,19)` `0x0002 -> 0x01d9`; W5 `(40,29)`
`0x0002 -> 0x01d8`. `16CE` uses AX as the column coordinate, BX as the row,
and writes `DX & 0x01ff` after preserving the upper MAP bits. These are
normal-gameplay mutation observations; the other writer families remain
bounded-negative only.

The target decompilation also resolves the `16CE` callsite inputs. `1892`
consumes visible `DS:6586` event records, advances the queued animation byte,
and passes `animation + subtype + 0x1d6` as the new tile ID. `1944` selects
`0x1ee`, `0x1ef`, or `0x1f0` from the event subtype. `6359` is a short
animation callback that rounds the object coordinates and derives the tile ID
from its animation offset/direction. This connects the observed W1L1–W5L1
mutations to their producers; the callsite decompilation is recorded in
`effect_writer_callsite_decomp`.

The static decompilation of that chain is now explicit. In neutral pseudocode
(`DS:` denotes the data selector):

```text
main_loop_4b8d:
    if DS:89EE == 0xffff: clear key state; goto 504f
    if DS:89EA == 0 or DS:880A <= 0: goto 4c43
    clear DS:8810; run transition helper; clear DS:85D2
    if DS:880A > 0 and DS:85D4 in {2,5,8,11,14}: call 01D7:3861
    run post-loader helpers; goto 504f

state_branch_4c43:
    if DS:89EA == 0 or DS:880A == 0: run reset branch at 4c5f
    else if DS:89F0 == 0 or DS:880A != 1: goto 4cfc
    else run the one-object reset branch; goto 504f

pending_update_4cfc:
    if byte DS:85DA <= 1: goto 4ea0
    dispatch byte DS:85DA (2, 4, 6, 7, 32, 34, ...), update the
    transition drawing coordinates, and increment DS:85DA at 4e94

pending_event_4ea0:
    if DS:89E6 == 0: goto 504f
    set the local transition-active flag; call 01E7:0D18; set a
    0x14f-frame delay; optionally set DS:89E0=0xffff for DS:85D4==0x0e;
    wait through 0207:0002 (or 0x46 frames on the alternate branch);
    call 01E7:0CAA, 0207:022A, 0207:17A0, 0207:08D8,
    01F7:F07B, 01F7:106A, then call the local intro routine 01D7:14E1
    continue at 4f10/4f1a/4faf and select the next DS:85D4 state

finalize_5010:
    if DS:89E0 == 0xffff: goto 504f
    run the resource/transition finalizer; on failure set DS:89EC=0xffff;
    copy the transition buffer; clear DS:89E6 at 504c

dispatch_504f:
    if DS:89EC != 0 or DS:89E0 == 0xffff: dispatch the active transition;
    otherwise jump to the ordinary loop at 4601
```

The direct relocation targets remove ambiguity from the far calls: `4EDD` and
`4EE6` call `0207:0002`; `4EEC` calls `01E7:0CAA`; `4EF4/4EF9/4EFE` call
`0207:022A/17A0/08D8`; and `4F03/4F08` call `01F7:F07B/106A`. The timed-wait
routine at `0207:0002` is itself statically simple:

```text
wait_frames(count):
    while count > 0:
        DS:819E = 0
        while DS:819E == 0:
            call scheduler/yield helper
        count--
```

Thus the unresolved W1L3 behavior is not an unknown branch: it is the timer
and transition state failing to return far enough for `504F` to dispatch the
secondary loader in the automated session. The static source for the ledger
is the extracted segment-1/segment-4 disassembly plus
`research/build/runtime-descriptor/segment1-relocations.json`.

## Conclusions and open points

Confirmed: direct nine-bit tile indexing; fixed four-byte records; renderer
resource index at `+0`; world-specific flags at `+2`; fixed 512-entry
allocation; selector-safe static/runtime agreement across W1–W5; and the
loader's first-row `OR 0x10` mutation as a `0x1000` word change. The W3
compare ladder requires first-match duplicate handling for `0xda/0xdb`. The
target decompilation also fixes the loader lifecycle: `365B` is the initial
MAP setup call from `4009`, while `3861` is the scheduler-selected W1L3–W5L3
transition path from `48B5`/`4BD8`.
The complete static MAP-write inventory also includes the loader-return `33BF`
pass, the tile-effect writer `16CE`, and the masked coordinate writers
`339A`/`340A`.
The direct MAP-reader audit finds all identified render/collision readers
masking to `0x01ff`; selector-safe controlled traces confirm the tile-to-
descriptor branch dataflow and `3DF2` probe polarity.

Open: a standalone semantic name for flag `0x10`, whether the statically
identified `43D0 -> 199D` or `1BC4/3AB3 -> 19E6` setter paths fire in an
unmodified W1L3 launch, and whether any runtime-constructed caller reaches
`01F7:339A`, `01F7:340A`, or `01F7:5C9D` during normal gameplay. Static evidence now excludes all NE
relocation/pointer references to those entries, and runtime probes have not
observed them. The W1L1 right-input run supplies normal-gameplay attribution
for the `1A3D -> 44DC` countdown path; the W1L3 `KBD_2` event writer and
controlled downstream path remain separate evidence. Those points should not
be inferred from the archive's upper MAP property field.
