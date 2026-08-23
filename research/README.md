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
big-endian reads and is intentionally read-only:

~~~sh
python3 research/tools/quikyctl.py archive-list game/NESTLE.DAT
python3 research/tools/quikyctl.py archive-index game/NESTLE.DAT
python3 research/tools/quikyctl.py archive-extract game/NESTLE.DAT work/assets
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
expose the structures described below. The NE command is a static header
survey; it does not assign semantic names to code segments or claim that the
executable has been decompiled.

`level-render` is a dependency-free PNG renderer. It derives `W1.ICO` and
`W1.PCC` from `W1L1.MAP`, applies the Java viewer's four-way column
permutation, and renders the MAP at 16 pixels per tile. If `W1L1.ARE` is next
to the MAP, it adds colored entity markers. The marker transform is currently
a diagnostic normalized mapping from the 52x48 ARE reference grid and its
16-pixel local slots; it is intentionally reported as provisional until the
executable's ARE placement routine is traced.

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

### ARE — structure now mechanically decoded; semantics still inferred

There is no validated magic header. Simon's article reports a fixed-layout
file with zero padding and a variable final block. The new parser validates
the following layout across all 21 ARE payloads in the bundled archive:

~~~text
@0x0000  0x0160  unknown/header region
@0x0160  0x1380  2496 big-endian u16 layout words
@0x14e0  variable declaration records
~~~

In the layout region, `0xffff` is a blank marker and `0x0000` is also common;
the latter's exact game meaning remains unknown. Other nonzero words are
offset-like references. For each reference `r`, the referenced declaration
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

The parser reports the raw type and coordinate values, preserving the
uncertainty about whether coordinates are tiles, pixels, or another game
unit. Simon changed the byte at 0x14e9 while testing object types. The fixed
prefix and declaration records should therefore be treated as a structural
model, not yet as a stable gameplay object specification.

Blanking an ARE experimentally removes enemies, pickups, exits, elevators,
falling leaves, and other living objects while leaving some static geometry
and spikes. The tested object/reference values were:

~~~text
0x20 nothing?             0x29/0x2a/0x2b falling leaves
0x64 nothing?             0x65 one ammo (context-dependent)
0x67 one ammo             0x68/0x6d/0x6e nothing?
0x6f ten-ammo box         0x70 extra health package
0x71 health up            0x72 temporary invulnerability
0x73/0x74/0x7a nothing?
~~~

Use W1L3 for experiments; the article suggests entering QUIKYSUPERHERO in the
menu and pressing 4 to jump to a test level.

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

### BOB — partial

BOB files contain much of the animated and still graphics. Simon's current
inference is a little-endian height at offset 0x04 and width at offset 0x06;
the surrounding header, frame tables, compression/encoding, and pixel data
are not decoded. Arbitrary byte edits can crash the game when the graphic is
first displayed, so treat BOB as read-only until a decoder is validated.

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
