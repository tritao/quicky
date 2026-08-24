# Selective player/input static slice

This is the first static-analysis slice aimed at a compatible C engine. It
stays at the level of facts that can be tied to executable bytes or an existing
runtime trace; names such as “player” and “collision” are not assigned where
the code does not yet prove them.

## Input path

The protected-mode selectors are the existing runtime mapping: segment 1 is
`01D7` and segment 3 is `01F7`.

| Address | Evidence-backed role | Important state |
| --- | --- | --- |
| `01F7:F17F` | IRQ1 handler; reads port `0x60` and writes the 32-byte scan-code ring | `FFFF:501E`, head `DS:503E` |
| `01F7:F1A8` | Consumes the ring, handles make/break bytes, maps arrows and Space to action bits | `DS:88BA`, `DS:88BC` |
| `01F7:F21B` | Returns the OR of normalized keyboard actions and the other action-word source | `DS:88BC \| DS:8196` |
| `01D7:01AC` | Menu/game helper tests directional/confirm flags | `DS:8196`, `DS:88BC` |
| `01D7:0203` | Waits until both action words are clear | `DS:8196`, `DS:88BC` |
| `01D7:4AC2` | Runtime-confirmed hidden level-selector loop | flags `1`, `2`, `0x20` |

The keyboard mapping is directly visible in the `F1A8` comparisons:

```text
scan 4B -> action 08   left
scan 4D -> action 04   right
scan 50 -> action 01   down
scan 48 -> action 02   up
scan 38 -> action 10   left shift/alternate action
scan 39 -> action 20   space/confirm
```

The high bit of a scan byte is treated as a break event. Make events OR the
mapped bit into `DS:88BC`; break events clear it. `DS:88BA` retains the most
recent seven-bit scan code. The action words are therefore a better C-engine
interface than exposing raw DOS keyboard bytes.

## Pooled-object scheduler

Segment 3 uses a 64-entry object pool. Each list entry is eight bytes and
contains an object callback/list reference. The object record fields already
used by the entity traces are:

```text
+0x02/+0x06   16.16 X/Y position
+0x12          logical sprite slot
+0x14          kind field (one helper searches for 0x64; meaning unresolved)
+0x17          update phase/state byte
+0x18          update callback offset
+0x2C          lifetime/counter field for short-lived objects
+0x2E          type-specific state/index
+0x32          update-state word
```

The static routines are:

| Address | Role |
| --- | --- |
| `01F7:0E06` | Find a free pool entry and initialize it; returns the object in `ES:DI`. |
| `01F7:0E66` | Count occupied pool entries into `DS:88C8`. |
| `01F7:0E96` | Run callbacks grouped by phase values `0`, `1`, and `2` in `+0x17`. |
| `01F7:0FA2` | Run callbacks for list entries with a non-null callback pointer; the list/object state meaning is unresolved. |
| `01F7:0F3C` | Search for `+0x14 == 0x64`; this is a useful breakpoint candidate, not yet a player identification. |

This explains why a compatible engine should model a stable pooled-object
identity and callback phase, rather than treating each ARE declaration as a
permanent object.

## Camera and MAP access

`01F7:1ED7` updates the 16.16 scroll accumulators and clamps them against the
target bounds in `DS:36FC/36FE/3700/3702`. It publishes the integer camera at
`DS:81C0/81C4` and derives `DS:81CE/81D0`. `01F7:1CDA` then uses those camera
values to stream 64-pixel ARE regions.

`01F7:3376` converts the supplied coordinates to a MAP cell using four-bit
shifts and the far MAP buffer. Its exact expression is
`DS:657A + (AX >> 4) * DS:657E + (BX >> 4) * 2`, where `AX` is Y, `BX` is X,
and `DS:657E` is the byte stride of one MAP row. It reads the word through
the segment in `DS:657C` and masks the result to the low nine-bit tile ID. It
therefore proves the 16-pixel addressing used by the state-machine traces,
while also showing that this helper intentionally discards the high MAP
property bits. The player collision path is separate and is decoded below.

The targeted MAP-user survey adds a useful negative result. `01F7:20C8` and
`01F7:2CB2` are renderer paths: they read a MAP word, mask it with `0x01ff`,
and use the tile ID to select VGA pixels. `01F7:16CE` also masks its input to a
tile ID, but preserves the existing cell's high bits when it writes an effect
cell. No direct consumer of the seven high MAP property bits was found in
these segment-3 users, so they are not yet a collision specification.

The player collision helpers resolve the next layer. `01F7:5C27` and
`01F7:5CC3` compute the same 16-pixel MAP-cell address, read the raw cell, and
execute `AND AH,1`. In the 8086 word representation this is exactly
`cell_word & 0x01ff`: it preserves all nine tile-ID bits and discards the
seven upper MAP bits. The resulting tile ID indexes the descriptor table at
`DS:6582` with `DS:30D4` bytes per entry; both helpers read the descriptor
word at entry offset `+2`.

A clean W1L1 right-input trace captured, for example, tile `126` with
descriptor `0x0000` and tile `341` with descriptor `0x0032`, while their raw
MAP property fields were `2` and `69`. The independent `5CC3` trace reads the
same descriptor-table words, providing the runtime check for the static
addressing and mask.

`5C27` consumes the descriptor low nibble as four coordinate-quadrant tests.
Depending on `AX bit 3` and `BX bit 3`, it tests descriptor bits `0x02`,
`0x01`, `0x04`, or `0x08`, returning the result through the x86 flags. `5CC3`
returns the descriptor word in `DX`, allowing its callers to test a wider set
of descriptor flags. This is the first statically complete explanation of the
collision helper's MAP input: the archive's seven-bit property field is not
what these helpers use.

The `01F7:3D02` caller makes the next-level use explicit. It calls `5CC3`,
and, when `DX & 0x30` is clear, retries after shifting the player eight pixels
in X. It then uses `DX & 0x20` to select the sign of a halved vertical
velocity and `DX & 0x40` for the final eight-pixel alignment adjustment. Those
bit consumers are confirmed, but their gameplay names remain provisional
until the corresponding floor, ceiling, and side boundary cases are traced.

The new `--player-branch-focus` probe watches `3D02` entry, the three
`DX & 0x30` tests (`3D1E`, `3D36`, `3D40`), the `DX & 0x20` test (`3D45`),
the `DX & 0x40` test (`3DD0`), and both return sites. W1L1 runtime results
are consistent across repeated samples:

* Neutral, left-wall (`x=72`), and jump samples follow
  `3D02 -> 3D1E -> 3D36 -> 3D40 -> 3D44`, with `DX=0x000C` on the first
  query and all three masks clear.
* The right transition at `(1163,338)` follows
  `3D02 -> 3D1E -> 3D45 -> 3DD0 -> 3DF1`, with `DX=0x0050`, so
  `DX&0x30=0x10`, `DX&0x20=0`, and `DX&0x40=0x40`; the object returns with
  `AL=1` and `object+0x3A=1` before the later checkpoint reset.

This proves the right-side reset path is the descriptor-flag branch, while
the stable left wall is handled by a different collision path. The observed
`AL` at the `3D44` early return is caller-state residue and is not treated as
a boolean result. Descriptor-bit gameplay names remain provisional until a
controlled case exercises `DX&0x20`.

The callback-focused tracer now records each helper's far-return address, so
runtime observations can be tied to static call sites: `0x3A3E/0x3A50` are
the two `3A1F` probes, `0x3E10/0x3E22` the `3DF2` probes, `0x3D1E/0x3D36`
the `3D02` descriptor reads, and `0x41FC/0x420E` the vertical-path probes.
The call sites use a zero result from `5C27` to enter their correction path;
when the selected low-nibble bit is set, the branch skips that correction.

Boundary traces provide these correlations without over-naming them:

* At the stable left wall (`x=72`), both `3A1F` probes at `x=67,y=400`
  read tile `139` with descriptor `0`, producing zero results.
* Near `x=1165,y=338`, the two `3A1F` probes read descriptor `0x000f`
  (selected bit set) and descriptor `0` (selected bit clear), showing the
  directional low-nibble behavior in one movement update.
* On jump descent, the `41F7/420E` probes read tile `42`, descriptor
  `0xe803`/low nibble `3`, on one side and tile `190`, descriptor `0`, on the
  other as the player returns toward the ground. The exact floor-versus-
  ceiling name remains open.

The reproducible W1L1 property matrix now covers neutral, left, right, and
upward input. The independent `5CC3` rows show `(128,400)` reading raw cell
`0x08B8`/tile `0x0B8` with descriptor `0x0000`; the stable left wall at
`(72,400)` reads `0xEC8B`/tile `0x08B` with descriptor `0x0000`; the jump
samples read tiles `0x136` and `0x121` with descriptor `0x0000`; and the
rightward path reaches tiles `0x151`, `0x000`, and `0x104`, including a
nonzero descriptor (`0x000F`/`0xF700`) before the reset to `(1673,368)`.
The corresponding `5C27` rows carry the coordinate-selected low-nibble mask
and are kept separate from `5CC3`'s returned descriptor word. These are
runtime observations, not names for the descriptor bits.

The loader still mutates one runtime row by ORing `0x10` into each cell's high
byte, which corresponds to runtime property bit `0x08`. That mutation should
be tracked separately until another consumer proves its meaning.

The tile-effect callback's zero-state gate is separate from MAP access. Static
code at `01F7:1DCA` checks `object+0x04/+0x08` against the current camera with
128-pixel margins and returns carry for an outside object. `01F7:1DEE` then
deactivates the object. Inside the camera window, `01F7:393C` returns dynamic
X/Y bounds derived from the object at `DS:881A`; `01F7:8E4B` uses those bounds
to decide whether to start the phase counter. This explains why the runtime
probes must keep the camera centered on the synthetic object before testing
the MAP-derived effects.

The call-site probe now resolves that indirect object. In the controlled
visible run, `DS:881A` and `DS:89EA` were both zero, `ES` was `0x027F`, and
`01F7:393C` read `ES:0000`; the caller's state-machine object was at
`ES:0078`. The offset-zero record supplied base `(128,400)` and the patched
bound fields `(left=488, bottom=-260, right=1088, top=-60)`, producing
`(AX,BX,CX,DX)=(616,140,1216,340)`. The static cross-reference survey found
no direct write to `DS:881A`; another executable path around `01F7:69FF`
also reads the offset-zero object's position. This supports treating it as a
persistent object-record slot rather than a per-effect temporary allocation.

## Runtime follow-up

The existing `quikytrace --state-machine-samples` path now records the input
words, camera words, and camera target bounds at each `01F7:8E4B` update entry
and exit. That gives the static labels a repeatable runtime check without
guessing the player object. The player/object-pool probe now closes that gap:
`ES:0000` is initialized by `01F7:3F27`, transitions to callback `01F7:3FF8`,
and remains the record referenced by `DS:881A`. The next pass should inject
controlled left/right/up/down input and correlate the player position delta
with the collision helpers at `648E`, `6484`, and `3A8A`. The annotation and
targeted decompiler scripts were also rerun against disposable raw-segment
Ghidra imports; generated projects and C reports remain outside the repository.

That controlled input pass is now complete for right movement. Holding
`KBD_right` for 30 guest frames moved the offset-zero record from `(128,400)`
to `(170,400)` and then `(219,400)` while callback `01F7:3FF8` remained active.
The same run hit `01F7:648E` at both the baseline and post-input samples. A
separate MAP-focused run hit `01F7:3376` with lookup coordinates changing from
`(133,400)` to `(173,400)` and returned low-nine-bit tile IDs `0x0b8` and
`0x167`; the persistent record positions at those barriers were `(128,400)`
and `(168,400)`. This is runtime evidence that the right-input path reaches
both the candidate collision helper and the 16-pixel MAP consumer. The
remaining directions should be sampled before assigning floor/side semantics.

The same W1L1 harness gives the expected directional controls: a 30-frame
`KBD_left` hold moved X from `128` to `85`, `KBD_up` moved Y from `400` to
`317`, and a 10-frame `KBD_space` hold moved Y to `358`. These are movement
observations only; the input words and object fields still need per-frame
comparison at a known boundary before naming jump or grounded flags.

The longer boundary runs now separate a solid wall from a reset hazard. With
six 240-frame right holds, the callback barrier reaches `(2132,368)` with
`action_word=4`, `velocity_x=0x00018000`, mode byte `+0x37=0xff`, and auxiliary
word `+0x3e=0x03e8`; releasing the key then returns the record to checkpoint
`(1673,368)`. This is a reset/death transition, not a solid wall. Six 240-frame
left holds instead settle at `(72,400)` for every subsequent sample, with the
normal zero-velocity tail state and no position reset. The paired MAP probe at
that wall reads `(77,400)`, full cell `0xec8b`, tile ID `0x08b`.

During ordinary movement and jump updates, the callback call sequence reaches
`648e`, `6484`, then `3a8a`, and the callback returns to its expected near
return address. At the right reset transition only `648e` is reached before
the state changes, which gives us a useful branch discriminator without yet
assigning the helpers names such as wall or hazard. Jump/release samples return
to `y=400` with zero vertical velocity; the raw mode bytes are retained in the
trace rather than promoted to a grounded flag until the static writes are
fully mapped.
