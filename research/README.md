# Tricky Quiky Games II reverse-engineering notes

This directory contains a reproducible inspection of Simon Laburda's 2011
reverse-engineering work. The primary source is the [DKIA article][dkia]. It
identifies the author as Simon Laburda and links the game and quiky-tools.zip.

The game archive was retrieved from DKIA because the article explicitly says
that its license permits private, non-commercial copying. A second copy was
retrieved from [DOSGames.com][dosgames], which labels the game freeware and
offers both a DOSBox-prepared archive and an original archive.

## Retrieved material

Downloads:

- downloads/quiky-tools.zip — SHA-256
  d8cc39c6c702335a3025a35600234b1bb833178c032a675733c5d71c54e7ca68.
- downloads/dkia-quiky.zip — SHA-256
  d62123cc60f39c3e5d6769607bd498cbe2334e2ac3cf666d01d695cf9d2bca53.
- downloads/dosbox-quiky.zip — SHA-256
  f7e19b159ade4eca244b9ca984e1f34e30e0291d1189950e9e2025f905660be6.
- downloads/dosgames-original-quiky.zip — SHA-256
  fc68c94f42b5bdc93e8c443ae05959e2f411bbb767859502b0386ac01236c14c.

The DKIA archive contains QUIKY.EXE, NESTLE.DAT, NESTLE.INI, SETUP.EXE,
RTM.EXE, DPMI16BI.OVL, and SCORE.DAT, matching the article. The DOSGames
prepared archive contains NESQUIK.EXE, the same-size NESTLE.DAT, NESQUIK.SET,
QUICKY.BAT, and HAUGLOGO.*. The two extracted NESTLE.DAT files are
byte-for-byte identical. The two 151,552-byte game executables differ at 44
byte positions, so NESQUIK.EXE is evidence of a closely related
distribution/build, not merely a filename rename.

The DOSGames quiky.zip download is a smaller six-file archive containing
setup/support material but not the executable or NESTLE.DAT; it appears to be
an installer/base distribution that needs setup. DKIA's patched archive is the
useful immediately runnable copy for this work.

## Tool source and exact syntax

Reference copies of the downloaded sources are in
[extracted/tools/](extracted/tools/). Only trailing whitespace was normalized;
the code and project structure are otherwise preserved. These syntaxes come from each main(),
not from assumptions:

| Program | Syntax from source | Behavior |
| --- | --- | --- |
| extract.c | extract [archive] | Extracts NESTLE.DAT by default, or the first argument. It writes payload files into the current directory. |
| pack.c | pack <target> <file to pack> ... | Concatenates the named files, then writes the directory and trailer. The argument order is the archive order. |
| ico2bmp.c | ico2bmp <Palette.PCC> <Tileset.ICO> <Output.BMP> | Reads a 256-colour PCX palette and turns the raw ICO tiles into an indexed BMP. |
| levelex.c | levelex <mapfile> | Reads a MAP and prints a lossy ASCII map, assigning characters to distinct 16-bit cell values. |
| Java viewer | java -jar QuikyLevelEditor.jar [MAP] | Opens a MAP, derives the matching world PCC/ICO names, and displays the level. |

The original C tools use host-endian reads/writes for archive fields. That is
correct for the original DOS/x86 environment, but is not a portable format
implementation. The original extract.c also has a real end-of-directory bug:
it reads the first four bytes of the trailer as a signed short filename
length. On current glibc this became a negative length and caused a
buffer-overflow abort after extracting all but TITELD.SAM.

[modern/extract.c](modern/extract.c) and [modern/pack.c](modern/pack.c) are
small safe variants with explicit little-endian handling, bounded names,
directory-boundary checks, and chunked I/O. They successfully extracted all
142 payloads and repacked them to a byte-for-byte identical NESTLE.DAT.

### Building

Linux/macOS with GCC or Clang:

~~~sh
cc -std=c17 -O2 -Wall -Wextra -o extract-modern research/modern/extract.c
cc -std=c17 -O2 -Wall -Wextra -o pack-modern research/modern/pack.c
cc -std=gnu17 -O2 -o ico2bmp research/extracted/tools/ico2bmp.c
cc -std=gnu17 -O2 -o levelex research/extracted/tools/levelex.c
~~~

On Windows, use the MinGW-w64 gcc commands above (or MSYS2's GCC). The
original files predate modern C prototypes and omit some portability headers;
the two modern/ programs avoid those issues. Native MSVC is not the target of
the original source and would need the usual _unlink/header adjustments.

### Modern inspection tool

`research/tools/quikyctl.py` is a dependency-free Python 3 inspector for the
archive, MAP files, and MZ/NE executable header. It uses explicit little- and
big-endian reads; its archive patch command always writes a separate output:

~~~sh
python3 research/tools/quikyctl.py archive-list game/NESTLE.DAT
python3 research/tools/quikyctl.py archive-index game/NESTLE.DAT
python3 research/tools/quikyctl.py archive-extract game/NESTLE.DAT work/assets
python3 research/tools/quikyctl.py archive-map-patch game/NESTLE.DAT \
  build/variant/NESTLE.DAT --map W1L1.MAP \
  --cell 27 21 120 --cell 28 21 121
python3 research/tools/quikyctl.py map-info path/to/W1L1.MAP
python3 research/tools/quikyctl.py are-info path/to/W1L1.ARE
python3 research/tools/quikyctl.py level-render path/to/W1L1.MAP \
  --output build/W1L1.png
python3 research/tools/quikyctl.py ne-info game/QUIKY.EXE
python3 research/tools/quikyctl.py archive-list game/NESTLE.DAT --json
python3 -m unittest discover -s research/tests -p 'test_*.py'
~~~

`archive-index` validates every MAP and ARE payload and reports per-extension
counts, byte totals, MAP dimensions, and ARE reference/entity counts.
`archive-extract` refuses path traversal, duplicate names, and overwrites
unless `--overwrite` is explicitly supplied. The archive and MAP commands
expose the structures described below. `archive-map-patch` validates the
selected MAP and changes only low-9-bit tile IDs in the separate output
archive. The NE command is a static header
survey; it does not assign semantic names to code segments or claim that the
executable has been decompiled.

`level-render` is a dependency-free PNG renderer. It derives `W1.ICO` and
`W1.PCC` from `W1L1.MAP`, applies the Java viewer's four-way column
permutation, and renders the MAP at 16 pixels per tile. If `W1L1.ARE` is next
to the MAP, it adds colored entity markers using the engine-confirmed transform:
each layout cell is a 64-pixel region and each record contains pixel offsets
relative to that region.

For a fresh extraction:

~~~sh
python3 research/tools/quikyctl.py archive-extract game/NESTLE.DAT /tmp/quiky-assets
python3 research/tools/quikyctl.py level-render /tmp/quiky-assets/W1L1.MAP \
  --output /tmp/W1L1.png
~~~

### Controlled ARE experiments

Generate isolated archives for the W1L3 experiment level:

~~~sh
python3 research/tools/quikyctl.py are-experiment \
  game/NESTLE.DAT research/notes/are-experiments
~~~

The command preserves the original archive and creates a manifest plus five
valid `NESTLE.DAT` variants: baseline, one entity X shift, one entity Y shift,
one type change, and one layout-cell move. By default it selects the first
ARE declaration reference and first entity in `W1L3.ARE`; use `--reference`
and `--entity-index` to select a different record. The generated manifest has
an `observation` field for recording what appears in-game.

Run one variant in an isolated DOSBox runtime:

~~~sh
./scripts/run-are-variant.sh \
  research/notes/are-experiments/entity-x-shift/NESTLE.DAT
~~~

The launcher copies the bundled runtime to a temporary directory, substitutes
only the selected `NESTLE.DAT`, mounts that temporary directory, and starts
`QUIKY.EXE` at 16,000 cycles. Use the game's `QUIKYSUPERHERO` cheat and level
selection described in the [DKIA article][dkia] to reach W1L3, then update the
variant's `observation` field after comparing it with the baseline. The
launcher removes its temporary runtime when DOSBox exits.

## Confirmed file formats

The labels below distinguish direct source/byte validation from Simon's
inferences.

### NESTLE.DAT — confirmed

This is an uncompressed concatenation of payloads followed by a directory and
an eight-byte trailer. For the retrieved archive:

~~~text
file size             3,705,125 (0x3880cd)
directory offset      0x3880c7 = 3,702,983
directory end         file size - 8
trailer u32 @ end-8   0x3880c7
trailer u32 @ end-4   0x0000008d = 141
directory entries     142 (the stored count is entries - 1)
~~~

Directory entry, in little-endian order:

~~~text
u16 name_length
u8  name[name_length]       # not NUL terminated
u32 payload_offset
~~~

There is no payload length. For entry i, use
offset[i+1] - offset[i]; the final payload ends at directory_offset.
The directory entries are in payload order. The article's pack.c writes the
last trailer word as argc - 3, i.e. number of packed files minus one.

### PCC — confirmed

PCC files are ordinary ZSoft PCX files. The world palette files (W1.PCC
through W5.PCC) are tiny PCX images whose useful content is the final 0x0c
marker plus a 768-byte RGB palette. The retrieved world files are 903, 901,
901, 901, and 899 bytes; their PCX bounding boxes are tiny rather than
320x200. The other PCC files are ordinary indexed PCX images and open in
GIMP/ImageMagick.

### ICO — confirmed / inferred rendering detail

ICO files are headerless and their sizes are multiples of 256. Each 256-byte
record is one 16x16 indexed tile. The main world tilesets contain:

~~~text
W1.ICO  480 tiles       W2.ICO  400 tiles       W3.ICO  480 tiles
W4.ICO  480 tiles       W5.ICO  440 tiles
~~~

W?ANI.ICO and LOOP_W?.ICO are additional tile-sized animation/loop sets.
The bytes are VGA-friendly column-interleaved rather than ordinary
left-to-right pixel order. ico2bmp.c uses the inverse lookup
raw_x = ((display_x * 4) & 0x0f) + (display_x >> 2); the Java viewer writes
raw pixels to display_x = ((raw_x * 4) & 0x0f) + (raw_x >> 2).

The converter arranges at most 20 tiles per row and writes an 8-bit indexed
BMP using the PCC palette. A validated output is
[build/output/W1-tiles.png](build/output/W1-tiles.png), converted from the
tool's BMP for convenient inspection.

### MAP — confirmed

The format is exactly the one described by levelex.c and the Java viewer:

~~~text
offset  size  field
0x00    4     ASCII "TLE1"
0x04    2     u16 big-endian width
0x06    2     u16 big-endian height
0x08    2     u16 big-endian unknown; all retrieved MAPs contain 0x0009
0x0a    ...   width * height u16 big-endian cells, rows then columns
~~~

The exact size is 10 + 2 * width * height. For example, W1L1.MAP is 270 x 30
and 16,210 bytes; W1L3.MAP is 55 x 60 and 6,610 bytes. MAP is the notable
big-endian format; the other inspected formats are generally little-endian
or byte-oriented.

Each cell is interpreted by the viewer as:

~~~text
ABCD EFGX XXXX XXXX
^^^^ ^^^
flags       tile = value & 0x01ff
~~~

Thus the lower nine bits select the tile and value >> 9 is a seven-bit
property field. The viewer masks 0x1ff; retrieved maps use tile IDs up to
469, so the article's old “maximum of 400” note is not a universal limit
(W2 has 400 tiles, while W1/W3/W4 have 480). Simon experimentally labelled B
as “solid left/right” and D as “hit on the head”; those meanings remain
inferences, not a completed collision specification. Other observed property
values include 0x04, 0x0a, 0x0b, 0x1c, 0x2c, and 0x38.

The player collision path now provides a stronger negative result for that
upper field. Helpers `01F7:5C27` and `01F7:5CC3` mask each raw cell to
`cell & 0x01ff`, use the full nine-bit tile ID to index the descriptor table
at `DS:6582` with stride `DS:30D4`, and read the descriptor word at `+2`.
`5C27` tests its low nibble against coordinate bit 3; `5CC3` returns the
descriptor word to its caller. Thus these collision decisions are driven by
per-tile descriptor flags, not by `value >> 9`. The loader's separate
`OR 0x10` into one runtime row remains a confirmed mutation whose higher-level
purpose is still open.

The targeted renderer audit agrees: `20C8` and `2CB2` also mask MAP words with
`0x01ff` before selecting imagery. The upper MAP field is therefore preserved
by mutation helpers but has no identified direct render/collision consumer;
indirect-reader status remains open. Selector-safe W1L1 traces confirm the
dataflow: `0x002e`/`0x002d` reach descriptor `0x000c`, while jump samples reach
tiles `0x001`/`0x01a` with descriptor `0`. Controlled tile substitutions give a
positive `0x02a`/`0x0070` branch through `3DF1` and a negative
`0x02b`/`0x0030` branch through `3DE4`; side-probe substitutions similarly
establish the `3DF2` short-circuit polarity. These are controlled evidence,
not final gameplay names for the high descriptor bits.

A fresh current-worktree DOSBox control pair makes the X-retry gate explicit:
W1 tile `0x028` (`flags=0x10`) and tile `0x029` (`flags=0x50`) both follow
`3D02 -> 3D1E -> 3D45 -> 3DD0 -> 3DE4`, while no-flag tile `0x160` follows
`3D02 -> 3D1E -> 3D36 -> 3D40 -> 3D44`. The `0x40` bit therefore does not
override `0x10` suppression. The exact traces and hashes are in the curated
evidence JSON.

Descriptor-only controls on the unchanged `0x02e` MAP tile further isolate the
vertical bits: `0x20` alone returns through `3DE4`, `0x40` alone executes the
retry and returns through `3D44`, and `0x60` reaches the positive `3DF1`
branch. The positive branch therefore requires the combined descriptor bits,
not a hidden MAP upper-field property.

Normal held-right W1L1 gameplay independently reaches tile `0x02a` at probe
`(1575,354)`: raw MAP `0x202a` (upper field `0x10`) maps to descriptor
`0x70` and returns through `3DF1`. This confirms the upper field remains
present in live MAP words without entering the direct collision lookup.
The same natural pass reaches tile `0x028` (`raw=0x2028`, descriptor `0x10`)
at `(1080,380)` and also returns through `3DF1`; `0x10` is therefore not
intrinsically a negative result.

The curated selector-safe cases and source hashes are recorded in
[`notes/descriptor-collision-evidence.json`](notes/descriptor-collision-evidence.json).

The descriptor flag roles are mechanically resolved: the low nibble is a
four-way occupancy mask; `0x10` or `0x20` suppresses the `3D02` eight-pixel X
retry; `0x20` selects the vertical response polarity; and `0x40` selects the
eight-pixel Y alignment. Only `01F7:3D19/3D31` directly call `5CC3`; the many
other `5C27` callers use only the low nibble. A standalone gameplay name for
`0x10` is still not justified.

The low-nibble helper's quadrant mapping is now explicit from its target
disassembly. With `AX` as the 16-pixel row coordinate and `BX` as the
8-pixel-aligned column coordinate, the tested subquadrant is selected by bit 3:
`AX.bit3/BX.bit3 = 11, 10, 01, 00` selects descriptor bits `0x02`, `0x01`,
`0x04`, and `0x08`, respectively. A zero low nibble returns clear; otherwise
only the selected occupancy bit is tested. This is the concrete bit-to-quadrant
mapping needed by a faithful collision implementation.

A trajectory-matched control strengthens the `0x10` result. At sequence 6 of
the held-right W1L1 run, the player is descending (`y=356`, vertical velocity
`0x00009000`) into tile `0x02a`; the natural descriptor `0x70` reaches `3DF1`.
Patching only that live descriptor record to `0x10` leaves the incoming `3D02`
register stale at `0x70`, but the descriptor lookup and subsequent branch
events carry `0x10` and reach `3DE4`. The retry suppression is therefore
attributable to the consumed descriptor word during vertical motion, not to a
stationary probe artifact. The exact trace and hash are in
[`notes/descriptor-collision-evidence.json`](notes/descriptor-collision-evidence.json).

Target decompilation also closes the nearby player-state ambiguity: `3D02`
clears object `+0x3a`, sets it to `0x01` or `0xff` for the two `0x20` response
branches, and clears it on a rejected target-Y comparison. `3DF2` is the only
identified consumer and tests the byte only as zero/nonzero before the X snap.
So `+0x3a` is a transient accepted-vertical-response latch, not a persistent
floor/ceiling or surface-type field.

### Runtime descriptor construction — confirmed

The descriptor allocation/publication path is `01E7:382B` (`0x800` bytes at
`01E7:3874`, published through `DS:6582/6584`), with runtime stride
`DS:30D4 = 4`. `01D7:3808` dispatches to one world-specific initializer; each
initializer fills 512 records at `DS:6582 + tile_id * 4`, writing the identity
`tile_id` at `+0` and a world-specific flags word at `+2` (default `0`). The
collision consumers in segment `01F7` use the direct mapping
`tile_id = raw_map_cell & 0x01ff`, then read the descriptor word at `+2`.
The renderers load `+0` and shift it by eight to select the corresponding
`0x100`-byte ICO/image block, proving that the identity field is a resource
index as well as a preserved tile ID.
Selector-safe DOSBox construction traces match the static W1 and W2 tables for
all 512 records. The full address ledger, flag evidence, and trace commands
are in [`notes/runtime-descriptor-construction.md`](notes/runtime-descriptor-construction.md).

The target decompiler confirms that `01E7:382B` is a startup constructor: it
publishes and zero-fills the descriptor table, then allocates unrelated runtime
buffers. The world dispatcher `01D7:3808` selects the initializer from
`DS:85D8` and clears a separate `DS:6D86:DS:6D88` `0x800`-byte buffer; that
buffer is not a descriptor remap. The generated pseudocode is retained outside
Git and its paths are recorded in the evidence JSON.

Fresh live traces now cover W3L1, W4L1, and W5L1 as well: all five worlds
match their corrected static tables 512/512, with MAP strides/heights
`540/30`, `540/40`, `500/50`, `490/52`, and `300/86`, respectively. W3 also
exposed duplicate compare cases for tile IDs `0xda`/`0xdb`; the first ladder
matches (`0x0c`/`0x08`) win over later unreachable (`0x10`/`0x50`) cases.
The machine-readable census is in
[`notes/descriptor-construction-evidence.json`](notes/descriptor-construction-evidence.json).

The primary MAP loader is `01D7:365B`; its post-copy loop covers the first row
(`DS:657E / 2` cells) and performs `OR byte [MAP + 2*cell + 1], 0x10`.
Runtime checkpoints show this changes the CPU-visible word by `0x1000`
(`0x0001→0x1001` on W1L1 and `0x005c→0x105c` on W2L1), leaving the low-nine-bit
tile ID unchanged. The secondary loader at `01D7:3861` has the same static
mutation pattern. Target decompilation now attributes it precisely: the
transition scheduler at `01D7:48B5` reaches `4BD8` only when `DS:89EA != 0`
and `DS:880A > 0`, then calls `3861` for selectors `2`, `5`, `8`, `0x0b`,
and `0x0e` (`4BF1/4BFB/4C05/4C0F/4C19`). These are the zero-based third
levels W1L3 through W5L3. The primary setup callsite at `01D7:4009` calls
`365B` before the level-specific ICO/BOB setup. A normal W1L3 lifecycle probe
still remains at the `01D7:48BB`
transition wait: `01F7:1AE6` repeatedly clears `DS:89EA` while `DS:880A` is
`4`, and its call sites are not claimed as naturally observed. A
corrected `--secondary` construction probe with a 128-lookup guard likewise
times out before entering `3861`; the negative artifact is listed in the
construction evidence JSON.
A controlled trampoline reaches `3861` and observes the same `0x0001→0x1001`
(`+0x1000`) first-cell mutation. The MAP writer helper `01F7:5C9D` is also
covered by a controlled W1L3 call: `(x=0x45,y=0x123,CX=0xa55a)` writes selector
`887`, offset `0x07c4`, changing `0x5001→0xa55a` at `5CBE`. These injected
calls prove code semantics, not normal-gameplay attribution.

The complete static MAP-write audit finds three additional cell-writer
families. `01F7:33BF` runs immediately after both loader returns (`37CB` and
`396D`) and conditionally rewrites every cell whose low-nine-bit ID is 2, 3,
or 4; the mask/reinsert operation is idempotent for a well-formed MAP word.
`01F7:16CE` rewrites one effect cell as `(word & 0xfe00) | (DX & 0x01ff)` and
has 23 direct NE callers: two transient-event paths, one short animation path,
and the 20 calls from the `8E4B` tile-effect state machine.
`01F7:339A` and `01F7:340A` are coordinate-selected low-ID and upper-property
writers, respectively, but have no NE relocation record or literal target-offset
reference in the file-backed segments. They preserve the complementary MAP bits
and OR the caller-supplied `CX` unmasked, so an indirect/runtime-generated caller
would have to construct the target at runtime. The same stronger static negative
applies to `01F7:5C9D`. No other store through the loaded `DS:657A/DS:657C`
pointer was found; the full table and static caller counts are in the
runtime-descriptor note and the JSON emitted by
`research/tools/descriptor_static_report.py`.

The focused transition diagnostic explains the wait: `01D7:4B6E` calls
`01F7:1AAA`, whose `01F7:1AE6` instruction clears `DS:89EA` before returning
to `01D7:4B73`; the main loop's `4BA4` zero branch therefore bypasses `4BD8`
and the five `3861` call sites. W1L3 and W2L3 both show this launch chain,
`DS:880A=4`, then `48B5/48BB` waiting on `DS:819E`. The diagnostic records
stack return addresses and all identified writes to the four transition
globals; the raw JSON artifacts are under `research/build/runtime-descriptor/`.
An extended W1L3 run also watched `19E6`/`199D`, `44DC`, `4BD8`, `3861`, and
`5C9D`; none fired. Skipping the `48BB` compare/JZ as a debugger-only test
still did not reach those paths, so the surrounding asynchronous transition is
also a barrier. That patched run is not treated as lifecycle evidence.
The wait flag's owner is now known: timer IRQ `01F7:F049` writes `DS:819E=1`,
while `0207:0014` and `0207:101F` clear/wait on it; `01D7:48B5` performs the
level-loop clear. W1L3 and W2L3 capture the same order, but an unmodified run
without a `48BB` breakpoint still times out there, so the automated transition
does not complete far enough to exercise `4BD8` or `3861`.
Allowing `48BB` to run reaches `01D7:48C2` and then `01F7:3062`, but not the
later main-loop gate; this post-wait boundary is recorded in
`secondary-post-wait-w1l3-v3.json`.
The post-wait branch is `48E6`, which checks `DS:89E6`; automated W1L3 launch
leaves it at `0`, so `4968` is skipped. Static `0xffff` writers are
`01D7:493E` and `01F7:4996/4AAC/92A9`. The input-triggered and controlled
consumer paths below distinguish the event flag from the separate scheduler
gate.

The segment-3 keyboard consumer at `01F7:F1A8` stores raw make bytes in
`DS:88BA`, so `DS:88BA=3` is the `2` key. Queuing a real `KBD_2` after W1L3
launch reaches `01D7:493E`, writes `DS:89E6=0xffff`, and enters `4968`. With
the scheduler gate held at its consumer, the same event reaches
`4BA4 -> 4BAE -> 4BD8 -> 3861`; a separate controlled run that forces
`DS:89E6` at `48E6` reproduces the downstream path without input. These
diagnostic gate holds are not unmodified lifecycle attribution. The raw runs
are under `research/artifacts/runtime-descriptor-construction/`.

Holding real `KBD_right` input from launch and after the `KBD_2` event did not
fire `01F7:19E6`, `19A3`, `1A3D`, `199D`, or `44DC`; `DS:89EA` stayed zero and
the unmodified run stopped at `4BA4`. This bounds the missing restoration more
narrowly: it is not caused by the tested player-input path during automated
W1L3 launch. The result is negative lifecycle evidence, not proof that no
other state or level-completion path can set the gate.

The expanded W1L3 flow probe reaches the natural post-wait sequence
`48E6 -> 493E -> 4968 -> 4BA4 -> 4C43 -> 4CFC -> 4EA0 -> 4EAA` with
`DS:89EA=0`, `DS:89E6` briefly `0xffff`, and `DS:880A=4`; it then times out
without `19E6`, `19A3`, `1A3D`, `199D`, `44DC`, `4BD8`, `3861`, or `5C9D`.
This narrows the current blocker to the pending-state/asynchronous path after
the post-wait branch. The artifact is
`w1l3-next-natural-flow.json` under
`research/artifacts/runtime-descriptor-construction/`.

The pending-focused trace bounds that path further. After `4EA0` sees
`DS:89E6=0xffff`, `4EAA` enters a transition handler that calls the timed wait
at `0207:0002` (`4EDD`), then `4F0D` enters the local intro routine at `14E1`.
The intro executes its draw/text/timer sequence through `15C5` and reaches a
second `0207:0002` wait (`0207:0017`) without returning to `504F`; no
`4BD8/3861` dispatch is observed. The staged artifact is
`w1l3-pending-focus-kbd2-right-v15.json`. The probe's timer-release memory
patch is debugger-only and is retained solely to locate the transition
handler's boundary.

Static decompilation now explains the whole pending branch: `4EA0` gates on
`DS:89E6`, `4EAA` performs transition setup, `4EDD/4EE6` call the frame-wait
routine `0207:0002`, `4F0D` enters local intro routine `14E1`, and `5010/504F`
consume `DS:89E0`/`DS:89EC` before either dispatching or returning to the
ordinary loop. The wait routine clears `DS:819E` and spins until the timer IRQ
sets it. The exact pseudocode and relocation ledger are in the research notes.

A less intrusive controlled-input run held real `KBD_2` from level dispatch
until the actual event writer `01D7:493E`, released it there, and then waited
120 seconds with only the secondary-loader call/entry breakpoints armed. The
event writer was reached, but `4BD8/3861` were not; the final CPU remained in
the timed-wait routine at `01D7:0207:0023`. This confirms that the pending timer
barrier survives input consumption and is not just a breakpoint-heavy trace
artifact. It remains controlled-input evidence rather than normal-completion
attribution; the hash is in `notes/descriptor-construction-evidence.json`.

The corrected timer-focused variant armed `01F7:F049` during the same wait and
observed 16 IRQ hits before and 16 after `493E`, yet still did not reach
`4BD8/3861`; its final CPU sample was `0207:10C8`, inside the PIT-delay path.
Targeted segment-4 decompilation shows `0207:0002` clearing `DS:819E` and
yielding until the IRQ makes it nonzero, while `0207:101F` clears the flag and
polls PIT channel 0 and `0207:10A9` samples/reprograms the PIT divisor. The
timer audit therefore rules out “no IRQ delivery” as the sole cause; the
remaining gap is the timed-wait/PIT flag-sampling interaction. Details are in
`notes/descriptor-construction-evidence.json`.

The IRQ target itself is also decompiled: `01F7:F049` writes
`DS:819E=1`, writes the transition timer flag `DS:5014=0xffff`, conditionally
calls the optional far callback at `DS:8952:DS:8954`, sends the PIC
end-of-interrupt, and returns with `IRET`. Follow-up target decompilation ties
that callback pair to the audio/resource startup (`36ED`/`SCORE.DAT` and the
SAM/TFX loader), while transition code disables its segment at `4853/496F`.
This makes the wait-flag producer unambiguous without misclassifying the audio
callback as the transition-state channel.

A post-`48BB` isolation run then waited only on `F049` and observed 16 real
IRQ entries. Every sample had `DS:8952:8954 = FFFF:FFFF`, the disabled
audio/resource callback sentinel, with `DS:89EA`/`DS:89E6` still zero. The
barrier therefore has live timer delivery; this sample does not identify a
missing transition callback. The remaining gap is the independent transition
state/setter path, not missing IRQs. This controlled result is
`timer_post_wait_audit` in the evidence JSON.

Unmodified instruction-level state traces now cover all five third-level
selectors. W1L3 through W5L3 all show
`F049 -> F04F -> 504F -> 10A3 -> 101F -> 0002 -> 0014 -> 0017 -> 001E ->
48B5 -> 48BB`; `DS:819E` is `0x0001` before the wait clear and `0x0000` at
the subsequent test. None reaches `4BD8/3861`. This is the strongest current
normal-lifecycle evidence for a shared timer/transition barrier; the raw
hashes are in the evidence JSON.

The player-focused W1L1 trace confirms the normal overlap path: repeated
`01F7:1BC4 -> 01F7:19E6` events occur under held `KBD_right`, while
`DS:89EA` remains zero. No `19A3`, `1A3D`, `44DC`, `43D0`, or `199D` writer
fires; downward and Space-input comparisons are also negative. This rules out
the tested player collision/state path as the scheduler-gate restoration.

The MAP-writer audit was extended to include gameplay effect writer
`01F7:16CE` in addition to `339A`, `340A`, and `5C9D`. A 96-event normal W1L1
held-right window traversed camera X `0..2309` while the object count changed
from 4 to 2, yet recorded no hits for any of the four writers. This is a
stronger bounded negative for ordinary movement; event-rich effect playback
and runtime-constructed callers remain open.

Those non-blocking follow-ups have explicit methods, acceptance criteria, and
stopping rules in the [optional follow-up plan](notes/runtime-descriptor-construction-plan.md#optional-follow-up-plan):
the historical `0x10` label, natural W1L3-W5L3 trigger, runtime-generated
callers for `339A/340A/5C9D`, and an optional live state-10 termination trace.

The optional follow-ups are now executed and curated in
`notes/descriptor-construction-evidence.json`: the `0x10` archive matrix keeps
the historical name unresolved, fresh W1L3-W5L3 runs confirm the shared natural
timer barrier, the three helper writers remain implemented-but-unobserved, and
the forced state-10 probe is explicitly tooling-limited before a sample.
Artifacts are under `research/build/optional/` with SHA-256 records in the
plan's [execution results](notes/runtime-descriptor-construction-plan.md#optional-execution-results).

The same four-writer probe also captured a normal `16CE` mutation in every
world's W1L1 launch. The common caller continuation is `01F7:1897`; the
rewritten cells are `(23,19) 0x0002→0x01d7` in W1, `(16,19) 0x0003→0x01e5`
in W2, `(23,14) 0x0002→0x01d8` in W3, `(32,19) 0x0002→0x01d9` in W4, and
`(40,29) 0x0002→0x01d8` in W5. The register convention is AX=column,
BX=row; `DX & 0x01ff` is the new tile ID. This confirms a real gameplay MAP
mutation path rather than only a static writer definition.

Target decompilation explains the trigger: `1892` consumes visible
`DS:6586` event records and calls `16CE` with event coordinates plus an
animation/subtype-derived tile ID; `1944` is a second event path selecting
IDs `0x1ee..0x1f0`; `6359` is a short animation callback that supplies a
rounded object coordinate and an animation-derived ID. The complete callsite
ledger is in `effect_writer_callsite_decomp`.

A W1L1 controlled Y-boundary test reaches `3FF8 -> 43D0 -> 199D -> 19A3 ->
44DC`; `DS:89EA=0xffff` at `44DC`, and `DS:880A` drops from `4` to `3`. This
proves the candidate gate-restoration path at runtime, while the forced Y
value keeps it explicitly outside normal-gameplay attribution.

A sustained unmodified W1L1 run with held `KBD_right` now reaches a normal
state-machine transition naturally. After a 1409-frame warm-up it observes
`1BC4 -> 19E6`, then `01F7:1A3D` writes `DS:89EA=0xffff` at camera `x=1943`;
the next player callback enters with `0xffff`, `44DC` decrements it, and
`DS:880A` is `3` (down from `4`). A longer natural-right trace reaches
`01D7:4BD8` while the countdown is still nonzero (`DS:89EA=0xfea2`, object
count `3`). This is distinct from the debugger-forced `43D0 -> 199D -> 19A3`
boundary path; it provides normal-gameplay evidence for the `1A3D -> 44DC`
countdown path. The exact writer and gate artifacts are
`w1l1-transition-right-natural-warmup1409.json` and
`w1l1-transition-right-natural-probe10.json` under
`research/artifacts/runtime-descriptor-construction/`. This closes the prior
uncertainty about whether the W1L1 state-machine countdown can occur without
debugger memory injection; the `43D0 -> 199D` boundary path remains separately
controlled, and the W1L3 launch still has a separate asynchronous barrier.

NE relocation filtering narrows the direct state callers: `01F7:1BC4` and
`3AB3` call `19E6`, while `01F7:43D0` calls `199D`. The `1BC4` path is an
overlap test gated by `DS:8810`; `3AB3` follows tile IDs `0x0b/0x0c/0x0d`;
and `43D0` is the player boundary check against `DS:81C4 + DS:81CC`.
Target decompilation confirms `199D` sets `DS:89EA=0xffff`, clears
`DS:8950`, resets motion fields, and decrements `DS:880A`; `19E6` sets the
same gate when its overlap counter reaches zero. These are the confirmed
setter paths. A controlled player-Y injection reached the player callback but
not `43D0/199D` before the secondary-loader wait.

The descriptor flag matrix was repeated under the player tracer. Tiles
`0x028/0x029/0x02a/0x02b` carry flags `0x10/0x50/0x70/0x30`; only `0x70`
returns `AL=1` through `3DF1`, while the other three return `AL=0` through
`3DE4`. This isolates the combined `0x20 | 0x40` requirement. The original
MAP word is restored after each run. Raw artifacts and exact breakpoint
metadata are in `research/build/runtime-descriptor/`.

The controlled `5C9D` write remains the only runtime proof for that helper.
Static relocations find no direct caller or embedded target, and a normal-right
W1L1 probe armed it for 220 passes without a hit; no W1L3 post-wait hit was
observed. The same no-direct-caller boundary applies to `339A` and `340A`,
while `16CE` and the loader-return `33BF` have static direct callers. Indirect
or runtime-generated callers remain possible for the three uncalled entries,
but none is currently evidenced by normal gameplay. A dedicated 30-second
W1L1 writer-focus run held real `KBD_right` from launch while watching only
`339A`, `340A`, and `5C9D`; none fired. At timeout the run had three active
objects, `DS:89EA=0xff41`, and no pending transition event. This is bounded
negative evidence, not a proof against later/runtime-generated calls; the raw
artifact is `research/build/writer-focus-w1l1-right-30s-20260825.json`.
The same 30-second writer-focus window on W1L3 also saw no hits, but ended at
the known launch barrier (`DS:85D4=2`, `DS:880A=4`, `DS:89EA=0`, `DS:819E=1`),
so it does not cover later third-level gameplay; its artifact is
`research/build/writer-focus-w1l3-right-30s-20260825.json`.
A controlled 60-second W1L3 comparison held `DS:89EA` open past the launch
clear and still saw no hits after the object count fell to three; this rules
out the three writers in that exercised post-gate window but is not normal
gameplay attribution. Artifact:
`research/build/writer-focus-w1l3-force-gate-60s-20260825.json`.
Thirty-second normal held-right writer-focus windows on W2L1, W3L1, W4L1, and
W5L1 also recorded zero hits for all three uncalled writers. This broadens the
bounded negative to one level in each world family; later event-specific and
runtime-generated callers remain open. The four artifact hashes are curated in
`notes/descriptor-construction-evidence.json`.

### ARE — structure now mechanically decoded; semantics still inferred

There is no validated magic header. Simon's article reports a fixed-layout
file with zero padding and a variable final block. The new parser validates
the following layout across all 21 ARE payloads in the bundled archive:

~~~text
@0x0000  0x0160  unknown/header region
@0x000e  u16     layout width
@0x0010  u16     layout height
@0x0160  width * height big-endian u16 layout words
@0x14e0  variable declaration records
~~~

In the layout region, `0xffff` is the blank marker. Earlier analysis mistakenly
treated the padding through `0x14e0` as part of a fixed 52x48 grid, producing
spurious zero cells and references. Other words are offset-like references.
For each reference `r`, the referenced declaration
starts at `0x0160 + r`; the first observed record starts at `0x14e8`, so the
usual first reference is `0x1388`. This offset relationship and the following
record terminator are confirmed experimentally from every bundled ARE file,
but the entity semantics are not yet confirmed by engine tracing:

~~~text
u16 entity_type
u16 x
u16 y
... repeated entity triples ...
u16 0xffff                 # end of one declaration record
~~~

Engine tracing confirms that the record coordinates are pixel offsets relative
to a 64-pixel-aligned streamed-region origin. The engine creates objects at
`region_origin + record_coordinate`, represented internally as 16.16
fixed-point positions. Simon changed the byte at 0x14e9 while testing object
types. Individual type meanings still require visual correlation.

Blanking an ARE experimentally removes enemies, pickups, exits, elevators,
falling leaves, and other living objects while leaving some static geometry
and spikes. The tested object/reference values were:

~~~text
0x20 nothing?             0x28 cloud     0x29/0x2a/0x2b falling leaves
0x64 nothing?             0x65/0x66/0x67 dedicated transient event variants
0x68/0x6d/0x6e nothing?
0x6f ten-ammo box         0x70 extra health package
0x71 health up            0x72 temporary invulnerability
0x73/0x74 unused/unimplemented (zero dispatch entries)
~~~

Use W1L3 for experiments; the article suggests entering QUIKYSUPERHERO in the
menu and pressing 4 to jump to a test level.

### Normal enemy families `0x01`-`0x1C`

The early normal dispatch range is now mapped through post-initializer debugger
stops. Each target reaches the shared factory at `01F7:0E06`, writes a logical
sprite slot into `object+0x12`, and is removed by the paired inert-type-0
archive mutation. The executable's resource-name table groups the matching BOB
files by world; this resolves the archive-wide slot collisions without claiming
that a slot is globally unique:

| ARE types | world | sprite family | slots | status |
| --- | --- | --- | --- | --- |
| `0x01`/`0x02` | W1 | `WURM2.BOB` | 281/231 | confirmed |
| `0x03`/`0x04` | W1 | `BIENE.BOB` | 276/226 | confirmed |
| `0x05`/`0x06` | W2 | `FISCH.BOB` | 254/204 | confirmed |
| `0x07`/`0x08` | W2 | `KRABBE.BOB` | 200/250 | confirmed |
| `0x09`/`0x0A` | W3 | `PENGO.BOB` | 250/200 | confirmed |
| `0x0B`/`0x0C` | W3 | `SCHNEE.BOB` | 209/209 | confirmed |
| `0x15`/`0x16` | W4 | `FLIEGE.BOB` | 208/208 | confirmed |
| `0x17`/`0x18` | W4 | `SPINNE.BOB` | 250/200 | confirmed |
| `0x19`/`0x1A` | W5 | `BUGGY.BOB` | 250/200 | confirmed |
| `0x1B`/`0x1C` | W5 | `UFO.BOB` | 264/214 | confirmed |

The exact callback, post-initializer offset, catalog placement, BOB record,
dimensions, and trace/experiment paths are recorded per decimal JSON type in
[`entity-types.json`](entity-types.json). Representative paired experiments
cover both W1 pairs and one pair in every later world family. Decoded sheets
used to establish the resource names are kept in the ignored research build
directory; the runtime traces and catalog are the durable evidence.

### Shared tile-effect state machine `0x1F`-`0x21`

Types `0x1F`, `0x20`, and `0x21` are now resolved as one confirmed state-machine
family. Their initializers set `object+0x2E` to 1, 2, and 3 respectively, then
converge on `01F7:8E4B`; the normal `object+0x12` sprite slot remains
`0xFFFF`. In a controlled W1L1 probe at world `(3072,272)`, state 4 queries
five nearby MAP cells through `01F7:3376`, returning tile IDs
`201, 200, 202, 203, 204`. The `DS:6986` table maps those IDs to effect indices
`121, 120, 122, 123, 124`, and `01F7:16CE` creates one transient object for
each mapped value.

The created objects have lifetime `3`, update callback `01F7:10B5`, and still
have no standard BOB slot. Static and runtime inspection of `10B5` shows that
it first calls the `01F7:1693` camera-visibility test and, when that test
passes, reaches `01F7:1186` for the ICO lookup. In the W1L1 runtime, effect
index 121 selects `FS:0x0357` at offset `0x7900`, and that 256-byte block is
byte-identical to raw tile 121 in `W1.ICO`. A camera-corrected W5L1 probe
captures the same path for effect states 61-64: `FS:0x0357` offsets `0x3D00`,
`0x3E00`, `0x3F00`, and `0x4000` match `W5.ICO` records 61-64 exactly.
Equivalent synthetic probes match W2 states 126-130 at offsets `0x7E00`-
`0x8200`, W3 states 400-404 at `0x9000`-`0x9400`, and W4 states 240-244 at
`0xF000`-`0xF400`. The world-relative ICO table is now directly confirmed
across W1-W5. The position, camera, bounds, and transient-lifetime overrides
are debugger-only and do not modify `QUIKY.EXE`, MAP files, or the archive.

The complete nonzero runtime tables and the matched W2 variant comparison are
in [`effect-mappings.json`](effect-mappings.json). In that comparison, types
`0x1F`, `0x20`, and `0x21` differ only in their initial `object+0x2E` values
(`1`, `2`, and `3`); their sampled state progression, MAP queries, emitted
effect sequence, and ICO lookups are identical.

The static decode of `01F7:8E4B` explains that progression. The callback
increments `object+0x32`; after the zero-state eligibility gate, it emits five
MAP-derived effects at each of states `4`, `6`, `8`, and `10`. The state-4
coordinates are `(Y, X) = (object+0x08, object+0x04 +
0x10/0x00/0x20/0x30/0x40)`. States 6, 8, and 10 use Y offsets `0x10`, `0x20`,
and `0x30`, respectively, and X offsets `0x10/0x20/0x30/0x40/0x50`.
Each tile ID returned by `01F7:3376` indexes `DS:6986`; zero entries are
skipped, while nonzero entries call `01F7:16CE` with the queried coordinates
and effect value. At state 10 the callback clears `object+0x18` and publishes
the terminal coordinates at `DS:8828/DS:882A`.

A controlled archive variant now captures the MAP mutation side of this path
directly. States 4, 6, and 8 each rewrite five adjacent cells: source IDs
`0xc8..0xcc` are looked up in `DS:6986` as `0x78..0x7c`, then written through
`16CE` with the upper MAP field preserved. This is debugger-seeded state/MAP
evidence rather than a normal-gameplay trigger; exact offsets and trace hashes
are recorded under `state_machine_effect_probes` in
[`notes/descriptor-construction-evidence.json`](notes/descriptor-construction-evidence.json).

The helper's exact address calculation is
`DS:657A + (AX >> 4) * DS:657E + (BX >> 4) * 2`, followed by a word read in
the segment selected by `DS:657C` and `& 0x01FF`. Here `AX` is Y, `BX` is X,
and `DS:657E` is the MAP row's byte stride. The static details and the
relocation-backed call sites are recorded in
[`notes/ghidra-analysis.md`](notes/ghidra-analysis.md).

The zero-state startup gate is now decoded as well. `01F7:1DCA` rejects an
object outside the camera-relative window
`camera_x - 0x80 .. camera_x + 0x1C0` by setting carry; its Y window is
`camera_y - 0x80 .. camera_y + 0x130`. Rejected objects go through
`01F7:1DEE`, which clears their update callback and list byte. Objects inside
the camera window call `01F7:393C`, which derives four dynamic bounds from the
object at `DS:881A` and its fields `+0x2C/+0x30/+0x2E/+0x32`. The state machine
advances to state 1 only when the current object overlaps those bounds. W2,
W3, W4, and W5 runtime samples match the decoded comparisons.
The call-site probe further resolves `DS:881A` to offset `0` in the live object
selector (`ES=0x027F` in the controlled run), while the transient effect
object occupied offset `0x78`; `DS:89EA` was zero. This makes the bounds source
a persistent object-record slot, not the effect object itself. The same
static survey finds a second position consumer around `01F7:69FF`, and shows
that `DS:89EA` is also decremented/tested by `01F7:44DC` and read by the
segment-1 main loop; its broader control meaning remains open.
The player initializer at `01F7:3F27` now closes the identity loop: it writes
`ES:DI` into `DS:881A` and installs callback `01F7:3FF8`. A W1L1 pool trace
confirms selector `0x027F`, offset `0`, stride `0x78`, and the callback
transition from `3F27` to `3FF8` at position `(128,400)`. The player probe now
supports repeated callback barriers and deterministic input holds:

~~~sh
python3 research/tools/quikytrace.py --launch --headless \
  --player-trace --player-focus-callback --player-callback-offset 0x3ff8 \
  --player-samples 3 --player-input-key KBD_right --player-input-frames 30 \
  --player-frames-between 5 --output research/build/traces/player-right.json
~~~

For scheduler-state callers, add `--player-transition-focus`; it watches
`19E6`, `199D`, `43D0`, `1BC4`, `3AB3`, and the `DS:89EA` state while holding
the selected input key across transition barriers.

To reproduce the bounded all-writer runtime check, use the secondary lifecycle
runner's writer-only mode:

~~~sh
python3 research/tools/run_secondary_lifecycle_trace.py \
  --level W1L1 --writer-focus --gameplay-key KBD_right \
  --gameplay-at-launch --timeout 30 \
  --output research/build/writer-focus-w1l1-right.json
~~~

`--writer-focus` arms only `339A`, `340A`, and `5C9D`; a timeout records the
global state at the end of the bounded window and is negative evidence only
for that window.

For long movement runs, `--player-transition-probe-frames` advances between
breakpoint passes; `--player-transition-probe-tail-frames` with
`--player-transition-probe-tail-camera-x` switches to a finer interval near a
chosen camera position, and `--player-transition-warmup-frames` lets a run
reach a natural transition before arming the writer probes.

The corresponding `--player-collision-focus` and `--player-map-focus` modes
break on the candidate collision helpers or `01F7:3376`; the structured output
keeps the helper registers, player object state, MAP coordinates, and tile IDs
together. `player_trace_report.py` joins `DS:881A` to the pool and reports
position deltas plus observed collision/MAP evidence. A W1L1 right-input run
moved `(128,400)` to `(170,400)` and `(219,400)`, reached `01F7:648E`, and a
separate MAP-focused run observed tile IDs `0x0b8` and `0x167`. Floor/side
semantics remain intentionally unassigned until the other directions and
boundary cases are sampled. Use `--player-input-samples N` to stop applying
the held key after N post-baseline samples and observe the release/reset path.
The current boundary evidence distinguishes a stable left wall at `x=72` from
a right-side reset near `(2132,368)`; it does not yet turn either case into a
hard-coded engine rule.

MAP and descriptor pointers are protected-mode selectors. Traces produced
before the selector-read correction used the real-mode `mem_read_word` helper
for those pointers; their tile/descriptor fields are archived but invalid.
Guest branch registers, object states, and movement positions from those runs
remain usable. New property and census traces use `mem_read_selector`.

For the descriptor layer, use `--player-property-focus` and optionally select
one helper with `--player-property-helper 0x5c27` or `0x5cc3`. Each property
row records the raw MAP cell, its upper property field, the masked nine-bit
tile ID, descriptor-table offset/word, and (for `5C27`) the coordinate-selected
low-nibble mask. Combine the traces with `player_property_report.py` to emit
machine-readable CSV/JSON evidence. The current W1L1 runs show the left wall
at `(128,400)` querying raw cell `0x002e`/tile `0x02e` with descriptor
`0x000c`; the corrected left/right rows query `0x002d`/tile `0x02d` with the
same descriptor, while jump samples query tiles `0x001` and `0x01a` with
descriptor `0`. The old wall/right/jump property rows remain archived but
invalid. The selector-safe `5C27` pass selects mask `0x04` at the baseline and
right positions (`x=123`), `0x08` at the left wall (`x=67`), and `0x02`/`0x04`
for the jump rows. Descriptor-bit gameplay names remain provisional.

To capture the caller's branch path, use `--player-branch-focus`. It watches
the `3D02` entry, the post-`5CC3` `DX&30` tests, and the `DX&20`/`DX&40`
branches, including the `3DE4` reject return, then records the return site,
`AL`, and `object+0x3A`:

~~~sh
python3 research/tools/quikytrace.py --launch --headless \
  --player-trace --player-branch-focus --select-level W1L1 \
  --player-input-key KBD_right --player-input-frames 240 \
  --player-input-samples 6 --output research/build/player-branch-right.json

python3 research/tools/player_branch_report.py \
  --label baseline --label left --label right --label up \
  research/build/player-branch-w1l1-baseline-v3.json \
  research/build/player-branch-w1l1-left.json \
  research/build/player-branch-w1l1-right.json \
  research/build/player-branch-w1l1-up.json \
  --json-output research/build/player-branch-calibration.json
~~~

The W1L1 branch matrix shows the right transition taking `DX=0x0050` through
the `DX&30`/`DX&40` path and returning with `AL=1`; neutral, left, and jump
cases keep those masks clear and return through `3D44`. This is evidence for
separating descriptor-driven reset behavior from the stable left wall.

The selector-safe debugger patch confirms both vertical branches without
altering the archive: tile `0x02a` (`0x0070`) reaches `3DF1` with `AL=1`, while
tile `0x02b` (`0x0030`) reaches the newly instrumented `3DE4` reject return
with `AL=0`.

For a loaded-table census, add `--player-descriptor-census`. The probe reads
the live descriptor table through `DS:6582/6584/30D4` and scans the loaded MAP
using the supplied dimensions (W1L1 defaults to `270x30`):

~~~sh
python3 research/tools/quikytrace.py --launch --headless \
  --player-trace --player-descriptor-census --select-level W1L1 \
  --player-samples 1 --output research/build/player-descriptor-w1l1.json

python3 research/tools/descriptor_census_report.py \
  research/build/player-descriptor-w1l1.json \
  --json-output research/build/player-descriptor-w1l1-candidates.json
~~~

The report retains exact MAP cell/world coordinates and emits candidates whose
descriptor contains `0x20`, `0x40`, or either side of `0x30`. Use
`--player-map-width/height` for a selected level with different dimensions.
These are the coordinates used by the live collision buffer; mapping its
streamed window back to archive-cell indices remains a separate step.
Collision focus now also records the `3A1F`, `3DF2`, `3A8A`, `6484`, and `648E`
helpers, with experiment-relative frame and event indices for later model
comparison.

To isolate descriptor semantics without depending on the MAP stream's active
window, `--player-branch-patch-tile 0x160` temporarily replaces only the
low-nine-bit tile at the live `3D02` probe, records the original word, and
restores it when the branch returns. This is debugger-only state and does not
modify the archive or executable.

For a bit-isolated control on the unchanged tile, pair
`--player-branch-focus` with `--player-branch-patch-flags 0x20`, `0x40`, or
`0x60`. The tracer patches only descriptor record `+2` and restores it after
the return; the resulting branch matrix is retained in
`notes/descriptor-collision-evidence.json`.

### Dedicated transient event types `0x65`-`0x67`

These three types do not use `DS:81D2`. Their wrappers set `DS:36EE` to
`0x00`, `0x08`, or `0x10` and call `01F7:1749`, which appends an 8-byte event
to the `DS:6586` table through the `DS:895E`/`DS:8960` ring. The event loop
dispatches through `01F7:1892 -> 01F7:16CE` and creates a short-lived object
whose update callback is `01F7:10B5`, whose normal sprite slot remains
`0xFFFF`. On each event-loop pass the animation byte is incremented modulo eight;
the loop adds the dedicated subtype and `0x1D6`, passes that index to `01F7:16CE`,
and stores it in `object+0x2E`. The update callback first calls the `01F7:1693`
visibility test. When the object is in the camera rectangle, `01F7:10B5` derives
an `FS:BX` block at `01F7:1186`; camera-centered W1L1 probes show that these
256-byte blocks match `LOOP_W1.ICO` records byte-for-byte. Type `0x65` selected
records 1 and 2 in repeated runs, while the captured `0x66` and `0x67` runs
selected record 6. The normal BOB slot remains unused, and the gameplay name
is still unresolved. The durable path ledgers are
`research/build/entity-65-dedicated-trace.json` through
`entity-67-dedicated-trace.json`; the animation ledgers are the corresponding
`entity-{65,66,67}-dedicated-animation-v8.json` traces under `research/build/`.
Cross-world first-entity probes independently match the same live blocks to
`LOOP_W2.ICO` record 8, `LOOP_W3.ICO` record 6, `LOOP_W4.ICO` record 22, and
`LOOP_W5.ICO` record 4. This establishes the world-relative `LOOP_Wn.ICO`
family rather than a W1-only coincidence; the gameplay names of the three
dedicated ARE types remain intentionally conservative.

### QUIKY.EXE — initial static survey

These values are confirmed by parsing the bundled executable's MZ and NE
headers; they say nothing yet about the meaning of individual routines:

~~~text
file size              151,552 bytes
NE header              0x0e3e
linker                 6.1
entry point            segment 1, offset 0x5089
initial stack          segment 7, offset 0x4000
segment count          7
segment table          NE + 0x0040 = file offset 0x0e7e
sector size            256 bytes
~~~

The segment table has five ordinary initialized segments, one segment with a
larger minimum allocation than its stored bytes, and a final zero-length
stored segment with a 0x4000 minimum allocation. The current parser reports
the raw table fields without assigning code/data meanings. The next
executable-analysis step is to map segment-relative addresses used by the
timer calibration and archive-loading code back to these segments, then
trace only those routines needed to validate format behavior.

### Selective static-analysis slice

The first targeted slice now ties the input, object-scheduling, camera, and MAP
paths to concrete segment-relative executable addresses. The annotated raw
segment project labels the keyboard IRQ/ring consumer (`01F7:F17F`/`F1A8`),
normalized action flags, menu selector loop, 64-entry pooled-object passes,
camera scroll update, and 16-pixel MAP-cell lookup. The corresponding runtime
probe records those input and camera globals at each state-machine update entry
and exit, so the labels remain hypotheses that can be checked against DOSBox
state rather than names inferred from a screenshot. The evidence and the
remaining deliberately unresolved semantics are summarized in
[`notes/player-input-static.md`](notes/player-input-static.md).
`FindQuikyReferences.java` provides the repeatable raw-segment data-user survey
used to distinguish MAP rendering from the still-unresolved collision path.

### BOB — compiled sprites decoded safely

BOB files are concatenations of little-endian sprite records with no global
header. Each record contains a logical slot, horizontal and vertical origins,
width and height, a length-prefixed table of 16-bit code offsets, and a
length-prefixed 16-bit x86 VGA blitter. Across the bundled archive, all 66 BOB
files parse exactly to EOF: 951 records populate 359 distinct slots between 0
and 994.

~~~text
u16 slot
u16 origin_x
u16 origin_y
u16 width
u16 height
u16 offset_table_bytes
u16 code_offsets[offset_table_bytes / 2]
u16 blitter_bytes
u8  blitter_code[blitter_bytes]
~~~

The offset table is monotonic and indexes the blitter. The second stream is
real executable code, not compressed pixel data. `quikyctl` never executes it:
the safe decoder recognizes only immediate byte/word writes to `[SI+disp]` and
maps their VGA planar addresses back to indexed pixels. Every recognized write
in all 951 records lies within its declared canvas.

Inspect and render an extracted sprite with:

~~~sh
python3 research/tools/quikyctl.py bob-info assets/QUIKYW1.BOB
python3 research/tools/quikyctl.py bob-render assets/QUIKYW1.BOB \
  --palette assets/W1.PCC --slot 0 --output quicky-slot-0.png
python3 research/tools/quikyctl.py bob-sheet assets/BLATT.BOB \
  --palette assets/W1.PCC --output falling-leaves.png --columns 8
python3 research/tools/quikyctl.py bob-find game/NESTLE.DAT \
  --slot 700 --slot 703 --json
~~~

For slot 0, this produces a 29x44 Quiky frame with origin `(14,44)`. Transparent
pixels are shown with a checkerboard. Arbitrary BOB editing remains unsafe
because changes to the blitter stream change machine instructions.

### SAM / TFX — partial

The TFX files begin with TFMX-SONG and are recognized as TFMX module sound
data. SAM files are the corresponding sample data. Simon reports that some
values have endianness issues relative to existing TFMX players; no complete
portable decoder is included in the surviving tools. Preserve both files as a
pair when experimenting.

## Java viewer status

The NetBeans metadata dates from 2010 and sets javac.source=1.5 and
javac.target=1.5. That old NetBeans/Ant build is not available here, but the
source itself compiles on OpenJDK 21 with ordinary javac; the only warnings
are raw generics/serialization warnings. The supplied JAR is Java 5 bytecode
and also runs on OpenJDK 21. A modern rebuilt copy is in
[build/output/QuikyLevelEditor-modern.jar](build/output/QuikyLevelEditor-modern.jar).

Build and run it:

~~~sh
javac -d research/build/java \
  research/extracted/tools/QuikyLevelEditor/src/at/sledv/qle/QuikyLevelEditor.java
jar --create --file research/build/output/QuikyLevelEditor-modern.jar \
  --main-class at.sledv.qle.QuikyLevelEditor -C research/build/java .
java -jar research/build/output/QuikyLevelEditor-modern.jar \
  research/extracted/dkia-assets/W1L1.MAP
~~~

The viewer derives W1.PCC and W1.ICO from W1L1.MAP, loads the palette,
decodes the tile interleave, masks the MAP tile index to nine bits, and draws
the map. It is a viewer rather than an editor: mouse motion reports the raw
cell value, but there is no save/edit path.

## DOSBox

Extract a runnable archive to C:\DOSGAMES\QUIKY, then use a dedicated mount:

~~~text
mount c c:\dosgames
c:
cd quiky
dir
setup.exe
~~~

Select Sound Blaster, base address 220, IRQ 7, DMA 1 for the DKIA copy.
Then run the executable that is actually present:

~~~text
quiky.exe
~~~

or, for the DOSGames prepared distribution:

~~~text
nesquik.exe
~~~

If timing is wrong, try:

~~~text
cycles 16000
~~~

Ctrl+F11 decreases emulation cycles and Ctrl+F12 increases them. The
[DOSBox manual][dosbox] documents the default Sound Blaster settings, the
cycle controls, and recommends mounting a dedicated DOS-games directory
instead of the entire Windows C: drive.

For executable-level keyboard tracing, install the debug build and run:

~~~sh
./scripts/run-dosbox-debug.sh
~~~

For the debugger-enabled dosbox-automation build, run:

~~~sh
./scripts/run-dosbox-automation.sh
~~~

This uses the project-local configuration, starts at 16,000 cycles, enables
the localhost REST API, and starts `game/QUIKY.EXE`; extra dosbox-automation
options are forwarded to it. Set `QUIKY_AUTOMATION_TARGET` to use another
executable, directory, or an empty value to start without a target. A
persistent API shell can be started with `QUIKY_AUTOMATION_TARGET=game`.

### Automated resource trace

With that debugger-enabled build running, the Quiky-specific Lua tracer
records calls to the confirmed resource lookup at `0207:18C7`. Lua handles
breakpoint waits, registers, protected-mode register-relative memory, and
continue operations synchronously inside the emulator. The Python wrapper
loads the script, collects its output, adds file hashes and emulator metadata,
and writes the JSON experiment ledger:

~~~sh
python3 research/tools/quikytrace.py \
  --output research/build/traces/w1l3.json \
  --count 2 --navigate-w1l3
~~~

`--navigate-w1l3` advances the opening sequence, launches the default level,
and rewrites matching `W1L1` Pascal paths to `W1L3` at the confirmed resource
lookup entry before the loader reads them. Each event records both
`original_path` and the effective `path`, making this deterministic live-memory
redirection explicit in the ledger.
`--navigate-level W4L1` generalizes that controlled path redirect to any
`W[1-5]L[1-4]` target. For the game's real world-resource context, use
`--select-level W4L1`; this enters the confirmed `QUIKYSUPERHERO` selector
path, writes the selector index at the documented input wait, and captures
the actual `W4L1.MAP`, `W4L1.ARE`, world ICO, and world BOB requests. Use
`--tail-count N` when checking BOBs that may be requested lazily after the
initial level-load batch. Resource ranges are sampled at the lookup routine's
far return, after `DS:97E4` has been updated, so they can be matched directly
against the parsed archive directory.
Alternatively, `--prepare-w1l3` continues from the documented `01D7:491D` cheat breakpoint,
performs the previously validated live-memory selector setup, and presses
Space. Omit it to trace normal manual play without changing guest memory. The
runner discovers the token generated by the project-local build,
or accepts `--token-file`/`DOSBOX_API_TOKEN`. It intentionally leaves the game
stopped at the final return breakpoint for inspection.

The debugger-enabled Lua API added for this workflow consists of
`breakpoint_set`, `breakpoint_remove`, `breakpoint_clear`,
`wait_for_breakpoint`, `debug_continue`, `cpu_state`, and
`mem_read_selector`. Lua-created
breakpoints are removed when the script completes, fails, or is stopped.

Entity probes use one structured `TRACE_CONFIG` Lua table. Python owns the
process, replay, screenshot, polling, and JSON-ledger work; Lua owns guest
timing, breakpoints, registers, and memory. State-machine-only controls are
nested under `TRACE_CONFIG.state_machine`, and the resulting entity record
includes `trace_schema_version`. This keeps debugger experiments explicit
without turning their flags into engine or gameplay API semantics.

### ARE entity catalog and controlled experiment

Catalog every unique ARE type, including exact world positions for every
reference-grid occurrence:

~~~sh
python3 research/tools/quikyctl.py entity-catalog game/NESTLE.DAT --json
python3 research/tools/quikyctl.py are-info path/to/W1L1.ARE --entities
~~~

`are-info` loads confidence-rated names from `research/entity-types.json`.
Unknown types remain numeric. To reproduce the complete confirmed W1L1 type
`0x2B` experiment—isolated baseline/inert runtimes, runtime object state, and
paired screenshots—run:

~~~sh
python3 research/tools/quikyentity.py game/NESTLE.DAT \
  research/build/entity-2b-experiment \
  --type 0x2b
~~~

When `--record-offset` is omitted, the tool deterministically selects the
matching W1L1 placement nearest the proven initial streaming anchor
`(768,224)`. Both disposable variants redirect the proven initial layout cell
to that declaration, so distant W1L1 records stream deterministically; the
baseline and inert archives still differ only in the selected entity's type.
Use `--dry-run` to inspect the choice without creating variants or launching
DOSBox. An explicit offset still overrides selection and preserves the
original layout.

Generate the batch-ready W1L1 queue, including callback groups and exact
commands, with:

```sh
python3 research/tools/quikyctl.py entity-experiment-plan game/NESTLE.DAT \
  --dispatch-ledger research/build/entity-dispatch-table.json --json
```

For synchronized animated evidence, request multiple frames. The tracer holds
the guest at a debugger barrier while Python captures each rendered frame and
acknowledges it through the debugger API:

```sh
python3 research/tools/quikyentity.py game/NESTLE.DAT \
  research/build/entity-2b-multiframe --type 0x2b \
  --capture-frames 6 --frame-step 30 --overwrite
```

Each experiment records a runtime-state timeline, per-frame masks, a union
mask, an intersection mask, changed-pixel counts, and a union bounding box.
The image analysis is dependency-free and accepts RGB or RGBA 8-bit PNGs;
ImageMagick is not required for evidence generation. `--capture-frames 1`
retains the original single-frame workflow.

The experiment never edits the source archive. Each runtime lives under its
own mount-policy-compatible `variant/game` directory and includes hashes and
an exact mutation manifest. Screenshot differences are supporting evidence;
initializer pointers, record identity, world position, and one-variable
archive mutation remain the primary evidence.

For confirmed type `0x2B`, the live callback trace also follows the object to
its renderer-facing field `object+0x12`: live runs observed logical slots 700
and 703. They belong to the `BLATT.BOB` leaf families 700-707 and 750-757
(14x12, origin 7,12). The decoded representative catalog preview is
[`notes/type-2b-falling-leaf-slot-700.png`](notes/type-2b-falling-leaf-slot-700.png).
The complete slot-ordered sheet is
[`notes/type-2b-falling-leaves-sheet.png`](notes/type-2b-falling-leaves-sheet.png):
the first row is 700-707 and the second is the brighter 750-757 family.
JSON `entity-catalog` output includes this structured evidence from
`research/entity-types.json`.

The same normal-object tracer handles pickup type `0x6F` with its
post-initializer breakpoint:

~~~sh
python3 research/tools/quikytrace.py --launch --headless \
  --runtime-dir research/build/entity-6f/baseline/game \
  --output research/build/traces/entity-6f-sprite.json \
  --entity-record-offset 0x1838 --entity-type 0x6f \
  --sprite-init-offset 0x8bce
~~~

That callback writes slot 607, which resolves uniquely to record 0 of
`WERBE.BOB` (26x34, origin 0,0). Its decoded preview is
[`notes/type-6f-slot-607.png`](notes/type-6f-slot-607.png), and the complete
four-record family is shown in
[`notes/type-6f-werbe-sheet.png`](notes/type-6f-werbe-sheet.png).

The adjacent pickup types use the same initializer/update layout and the next
three `WERBE.BOB` records:

| ARE type | meaning | initializer | slot | dimensions |
| --- | --- | --- | --- | --- |
| `0x70` | extra health package | `01F7:8BE5` | 608 | 21x22 |
| `0x71` | health upgrade | `01F7:8C08` | 609 | 22x22 |
| `0x72` | temporary invulnerability | `01F7:8C2B` | 610 | 15x25 |

Each slot is resolved by `bob-find`, and each target-typed archive was
compared against an inert type-0 mutation at the same streamed W1L1 anchor.

Normal type `0x28` is a cloud emitter. Its dispatch entry is
`DS:81D2+0x0A0 -> 01F7:9256` with object class 0. A target-vs-inert W1L1
capture removes the large white cloud. This object deliberately leaves
`object+0x12` at `0xFFFF`, so it does not follow the standard logical-slot
path; the rendered cloud matches the four 32x16 records in `WOLKE.BOB`, slots
413-416. The sheet is [`notes/type-28-wolke-sheet.png`](notes/type-28-wolke-sheet.png).

Types `0x29` and `0x2A` use the same `01F7:4727` leaf callback as confirmed
type `0x2B`, with class 1 and the same `BLATT.BOB` slots 700-707/750-757,
animation tables, and post-initializer boundary `01F7:474D`. Controlled
target-to-inert experiments at a common streamed anchor reached the expected
leaf slots for both types and removed their target contribution. They are
cataloged as `falling_leaves_variant_29` and `falling_leaves_variant_2a`; the
three types are behaviorally grouped because the executable dispatch callback
does not distinguish them after lookup.

The next resolved normal families are cataloged with explicit confidence:

| ARE types | runtime slot(s) | asset family | status |
| --- | --- | --- | --- |
| `0x2C` | 710 | `PAPIER.BOB` | confirmed paper effect |
| `0x33` | 214 | `WIND.BOB` in W4L1 context | confirmed wind effect |
| `0x35`, `0x36` | 264/214 | `UFO.BOB` in W5L1 context | confirmed UFO effect variants |
| `0x34` | 400 | `BUMP_W1.BOB` through `BUMP_W5.BOB` | confirmed bump effect |
| `0x3D`-`0x40` | 300/301 | `PLATFW1.BOB` through `PLATFW5.BOB` | confirmed moving-platform variants |

The slot evidence is from post-callback debugger stops, not the factory's
initial `0xFFFF` field. `bob-find` reports archive-wide collisions for some
shared logical slots, so the catalog records the level/world resource context.
Direct selector-mode resource traces now resolve the W4/W5 collision cases:
W4L1 loads `WIND.BOB` and `PLATFW4.BOB`, W5L1 loads `UFO.BOB` and
`PLATFW5.BOB`, W3L2 loads `PLATFW3.BOB`, W2L1 loads `PLATFW2.BOB`, and W1L3
loads `PLATFW1.BOB`. W1L2 also directly loads `BUMP_W1.BOB` for type `0x34`.
Native-context renderer probes independently confirm those choices: type `0x34`
resolves slot 400 through map index 149 to a `32x23` descriptor at offset 6556,
while types `0x3D`-`0x40` resolve map indices 104/109/85/119 and exact
`32x14`/`48x16`/`32x14`/`48x16` geometry in the W4/W3/W1/W2 contexts. All five
live descriptors match the selected BOB record dimensions and origins.
The same probe now covers representative ordinary families: W1L1 WURM2/BIENE,
W2L1 FISCH/KRABBE, W3L1 PENGO, W4L1 FLIEGE/SPINNE, W5L1 BUGGY/UFO, and W2L2
PAPIER all produce live descriptors whose dimensions and origins match the
catalogued BOB records. The per-type trace paths and descriptor offsets are
recorded in `research/entity-types.json`.
The four WERBE pickup types `0x6F`-`0x72` likewise resolve live descriptors
for slots 607-610; their dimensions are `26x34`, `21x22`, `22x22`, and `15x25`.
Types `0x1F`-`0x21` converge on update callback `01F7:8E4B`
but leave `object+0x12` unset; they are confirmed animated world-ICO
state-machine variants rather than standard BOB-slot objects.

The puzzle-letter family `0x79`-`0x7F` is also resolved. The seven normal
dispatch entries use consecutive callbacks at `01F7:8C71`-`01F7:8D07`, all
converge on update callback `01F7:8D20`, and write slots 600-606 in
`object+0x12`. Those slots are records 0-6 of `PUZZLE.BOB`, each 16x16; the
decoded sheet reads `N E S Q U I K` in slot order:

[`notes/type-79-slot-600.png`](notes/type-79-slot-600.png) is the N preview and
[`notes/type-7a-slot-601.png`](notes/type-7a-slot-601.png) is the E preview.
The complete family is shown in
[`notes/type-7a-puzzle-sheet.png`](notes/type-7a-puzzle-sheet.png). Each
mapping was checked with a controlled W1L1 record mutation to inert type 0;
the screenshot diff is supporting evidence, while the callback, object slot,
BOB record, and target-vs-inert runtime traces are the primary evidence.
Native-context probes independently resolve map indices 80-86 and descriptor
offsets 3520-3784 in `0x2C` strides; all seven live descriptors are `16x16`
with origin `(0,0)`, matching PUZZLE.BOB records 0-6.

Types `0x73` and `0x74` do not occur in any of the 21 archived ARE payloads.
Controlled injections into the W1L1 anchor reach the common factory, but their
dispatch entries are `0000:0000` with class `0` and no sprite assignment, so
they are cataloged as unused/unimplemented rather than assigned a meaning from
the neighboring puzzle range. The seven puzzle types occur ten times each
across the archive and are present in every sampled W1L1 stream.

The confirmed cheat comparison path and debugger addresses are documented in
[`research/notes/cheat-trace.md`](notes/cheat-trace.md). The debugger is
entered with `Alt+Pause`; it runs in the terminal while the game remains in
the SDL window.

## Safe continuation workflow

1. Make a pristine copy of NESTLE.DAT and record its SHA-256.
2. Work in a new directory and run `quikyctl archive-extract` there.
3. Identify a level by its WnLk.MAP/WnLk.ARE pair.
4. Derive its tileset and palette from Wn (W1L1 uses W1.ICO and W1.PCC).
5. Render the ICO with ico2bmp, or open the MAP in the Java viewer.
6. Preserve the full 16-bit MAP cell; edit only the tile or a deliberately
   tested property bit.
7. Compare ARE edits against a blank-ARE copy and record each changed offset.
8. Repack in a temporary output using the original payload names and order,
   never over the backup.
9. Replace only a disposable DOSBox test copy of NESTLE.DAT.
10. Test one change at a time, keeping SETUP.EXE/sound configuration and a
    known-good archive available for rollback.

The next useful research target is ARE: build a table of every 0x13 xx entry,
its position in the 0x160 region, the declaration bytes at 0x14e0, and the
corresponding in-game entity. For MAP collision work, compare otherwise
identical cells while changing one upper bit and test head/side/floor
interactions in DOSBox.

[dkia]: https://www.dkia.at/en/node/76
[dosgames]: https://www.dosgames.com/game/tricky-quiky-games/
[dosbox]: https://www.dosbox.com/DOSBoxManual.html
Capture every normal ARE dispatch entry in one initialized game run, then merge
the evidence into the static catalog or show update-callback/class groups:

```sh
python3 research/tools/quikytrace.py --launch --headless --dispatch-table \
  --output research/build/entity-dispatch-table.json
python3 research/tools/quikyctl.py entity-catalog game/NESTLE.DAT \
  --dispatch-ledger research/build/entity-dispatch-table.json
python3 research/tools/quikyctl.py entity-catalog game/NESTLE.DAT \
  --dispatch-ledger research/build/entity-dispatch-table.json --groups
```

The capture waits for the first ARE declaration after launching W1L1 because
the table is still zeroed at the menu. Normal entries contain a near update
callback in segment `01F7`, an object-class byte, and a reserved byte. Types
`0x65`, `0x66`, and `0x67` bypass this table and are reported as the distinct
runtime-confirmed handlers `01F7:178D`, `01F7:1798`, and `01F7:17A3`.
