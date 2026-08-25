# Actual game state

This note records the first evidence-backed state map for `QUIKY.EXE`. The
addresses below are offsets in the runtime data selector (`DS = 0x0237` for
the bundled executable). Names are descriptive; they are not claims that the
original symbols survived.

## Confirmed state fields

| DS offset | Width | Working meaning | Evidence |
| --- | ---: | --- | --- |
| `0x880A` | word | lives | HUD reader at `01F7:5B3A`; the type `0x2C` pickup path increments it up to 9; death paths decrement it at `01F7:19DB`, `1A06` |
| `0x880C` | word | ammo | HUD reader at `01F7:5B9A`; ammo pickup `0x6F` adds 10; pickup path clamps at 99 (`01F7:1949`); firing path decrements it at `01F7:4545` |
| `0x881C:0x881E` | dword | score | HUD reader at `01F7:5A08`; decimal digits are rendered by repeated division; the low word is incremented with carry by the level-score tally path |
| `0x8822` | word | current health | HUD reader at `01F7:5AF9`; damage decrements it at `01F7:1A00`; death/reset writes zero; pickup callbacks restore or increment it |
| `0x8824` | word | maximum health | gameplay initialization writes 5 at `01D7:5100`; health-package callback increments it up to 5 and copies it to current health |
| `0x8810` | word | global invulnerability/transition gate | invulnerability pickup sets `-1` at `01F7:1AA2`; the player timer clears it when the player object's timer expires; death/respawn paths clear it |
| `0x85D4` | word | progression/level index | the level-state loop rewrites it through the grouped values `0..14` and `0x10..0x14`; level-loading UI derives displayed level text from it |
| `0x85D6` | word | prior/within-world progression value | used with `0x85D4` to choose the next index in the level-state loop |
| `0x85DA` | byte | transition/cutscene mode counter (working) | startup sets it to 1 at `01D7:50D0`; the transition scene increments it at `4E94`, branches on it at `4CFC`, and reset/finalization clears it at `5184` |
| `0x85D8` | word | completion-route/world flag (working) | completion handling at `01F7:497C` branches on values 1, 3, and 5 |
| `0x89E0` | word | pending transition-scene signal (working) | transition dispatch writes `-1` for selected scene events at `01D7:4E10`, `4E8E`, and `4ECC`; the main transition loop consumes it at `5010` and resets it during initialization/finalization |
| `0x89EA` | word | player-control/death gate (working) | checked by the player callback at `01F7:3FFD` and by main-loop life/death branches at `01D7:4BA4`, `4C43`, and `4CB8`; nonzero bypasses the ordinary zero-state path |
| `0x88AF` | byte/word | preserved-score/finalization selector (working) | `01D7:50B1` branches around fresh-game score reset when nonzero, then calls the `01D7:1084` high-score dispatcher |

The gameplay-start initializer at `01D7:50D0` sets maximum/current health to
5, lives to 4, score to 0, and ammo to 0. A separate menu/demo initializer at
`01D7:1135` uses maximum/current health 3 and leaves score, ammo, and several
session counters at zero. The later menu result sets lives from
`DS:88B0 + 4` (the alternate path at `01D7:1464` uses `+3`). These two
initializers must not be conflated when comparing traces.

## Pickup semantics

The confirmed pickup objects share update callback `01F7:8D20`. Their
initializers write a small subtype/count at object `+0x2C`:

| ARE type | Object `+0x2C` | State effect | Score effect | Static site |
| --- | ---: | --- | ---: | --- |
| `0x6F` | 1 | ammo `+= 10`, capped at 99 | `+50` | `01F7:8D70` |
| `0x70` | 2 | maximum health `+= 1` up to 5; current health becomes maximum | `+250` | `01F7:8D89` |
| `0x71` | 3 | current health `+= 1` if below maximum | `+100` | `01F7:8DB8` |
| `0x72` | 4 | call `01F7:1A97` (invulnerability) | `+150` | `01F7:8DDF` |
| `0x2C` | 5 | lives `+= 1` up to 9 | `+500` | `01F7:8E08` |

The temporary-invulnerability callback sets the player object's `+0x34`
timer to `0x2BC` (700 frames) and sets `DS:8810 = -1`. The same player-object
timer is used for post-damage grace periods, but damage sets it to `0xD2`
(210 frames), so pickup immunity and damage grace are distinguishable by
duration.

A completed-update traversal with seeded health/lives captured the damage
timer in the live player object (`+0x34`). The first hazard hit occurred
between probe frames 360 and 420, reduced health by one, and left the timer at
170; subsequent 60-frame samples read 110, 50, and 0. No additional health
loss occurred while the timer was nonzero. This is runtime confirmation of a
one-count-per-guest-frame countdown from the statically identified `0xD2`
grace value. The artifact is
`research/build/actual-state-damage-timer-segmented.json`.

The matching type-`0x72` overlap trace was seeded at the W1L1 pickup position
`(2240,272)`. It recorded the pickup award (`score += 150`), `DS:8810 =
0xFFFF`, and the shared player timer at 550 after 180 frames, 370 after 360,
190 after 540, and 10 after 720. At the next 180-frame sample the timer was
zero and `DS:8810` had cleared. The artifact is
`research/build/actual-state-invuln-pickup-expiry.json`; this confirms the
same one-count-per-guest-frame countdown with the statically identified
`0x2BC` (700-frame) pickup duration.

## Damage, death, and respawn

`01F7:19E6` is the shared damage path used by the player collision helpers.
It first checks the persistent player object (`DS:881A`); a nonzero object
word at `+0x34` returns without applying damage. Otherwise it plays the damage
effect and decrements `DS:8822`.

- If health remains nonzero, the routine writes `player+0x34 = 0xD2` and
  returns. This is confirmed damage grace/invulnerability.
- If health reaches zero, it decrements `DS:880A`, clears the death/reset
  session flags, moves the player into the death animation state, and leaves
  the player object in the reset path. `01F7:199D` is the parallel instant-death
  path used by the out-of-bounds/fall branch; it decrements lives and writes
  current health to zero directly.
- The main loop checks lives at `01D7:4BAE` and `01D7:4C43`. When the last life
  is exhausted it clears the life word and enters the game-over/menu path.

The player initializer at `01F7:3F27` installs callback `01F7:3FF8`. The
respawn helper at `01F7:1AAA` reads a position table at `DS:8828 + 4*DS:85D2`,
writes the selected fixed-point position into the persistent player object,
and reinstalls the initializer callback. `01F7:1AF5` restores current health
from maximum health and resets `DS:85D2` to zero. A complete static write-xref
found only five executable stores to that word: `01D7:45DB`, `4603`, `4B6B`,
and `4BD5`, plus the respawn reset at `01F7:1AFB`; `01F7:1ABA` is the only
non-store reference in the respawn reader. The new `--player-checkpoint-probe`
arms all five instruction addresses. A live W1L1 run hit only the startup/
restart reset at `01D7:4B6B`; the index stayed zero while the position table
mutated. Thus no gameplay checkpoint index-advance write exists in the
executable's direct code path; the observed checkpoint behavior is table
mutation with slot zero selected.

A separate world-object routine at `01F7:924B-9253` writes the first position
pair in that table from the object's position (`object+0x04 + 0x19`,
`object+0x08 + 0x46`). This is evidence of a spawn-position setter, but its
entity/type and whether it represents a checkpoint or only level-start setup
are not identified. No direct executable write to `DS:85D2` other than the
respawn reset was found in the static pass.

The player object's `+0x3E` field is used by the death/reset animation (the
reset path writes `0x3E8`). Existing runtime notes observe a long rightward
transition entering this value and returning the player to a prior position,
but the checkpoint identity is still provisional.

## Score and completion evidence

Pickup awards are the values above. A separate completion/goal path at
`01F7:4968-4996` adds `5000` to the score, then checks `DS:85D8`. Values 1, 3,
and 5 take a special continuation path; other values set `DS:89E6 = -1`, a
level-transition signal consumed by the main loop. The level-end entity
initializer at `01F7:9256` installs callback `0x9269`; that callback compares
the entity's 16-pixel box with player bounds and writes `DS:89E6 = -1` when
the overlap succeeds and player object byte `+0x37` is zero. Thus the score
tally and endpoint signal are separate, statically identified completion
branches. The main loop checks the endpoint signal at `01D7:48E6` and branches
to its transition setup at `01D7:4968`; the score/progression-producing tally
is the distinct object-side path at `01F7:4968`.

The level-state loop at `01D7:4F1A-5010` maps completed level indices into the
next world/level group. It handles the five grouped ranges explicitly:

```text
0/1 -> 0x10, 3/4 -> 0x11, 6/7 -> 0x12,
9/10 -> 0x13, 12/13 -> 0x14
```

The final-level/cutscene branches use `DS:85DA`, `DS:89E0`, and `DS:89E6`;
these are progression/session controls, not yet renamed as save-game flags.

`SCORE.DAT` is 130 bytes: 128 encoded data bytes followed by a little-endian
16-bit checksum. The targeted segment-2 decompile distinguishes three
routines: `0227:3471` initializes the eight in-memory records and default
scores, `0227:3565` encodes and writes `Score.DAT`, and `0227:36ED` loads and
decodes it. The same code is also addressable through selector `01E7`. For
byte index `i` in the first 128 bytes,
the reversible transform is:

```text
encoded[i] = decoded[i] XOR (0xC8 + (i mod 0x20))
```

The checksum is `0x002A + sum(encoded[0:128])`, truncated to 16 bits. After
decoding, each record is:

```text
+0x00  name length (byte)
+0x01  eight-byte name field (space/NUL padded as applicable)
+0x09  score (little-endian dword)
+0x0D  saved score/session auxiliary word (`DS:880E`, little-endian)
+0x0F  saved progression byte (`DS:85D4` low byte)
```

The bundled file validates with checksum `0x5884`. Its decoded records
include names such as `SIMONERI`, `ESRICMHO`, `ESTHER`, and `NAOMI`, and scores
that match the initialization scale (100000, 60000, 40000, etc.). The
executable's HUD score path is independent of this file; the remaining file
format question is the exact high-score insertion/save trigger.

The high-score insertion routine copies `DS:880E` into record offset `+0x0D`
and the low byte of `DS:85D4` into `+0x0F` before the record is written. The
three auxiliary bytes therefore preserve a session/status word plus the
progression index; they are not opaque padding.

The targeted decompile of `01D7:0703` closes the insertion mechanics: it
compares the current score dword against the eighth record's score at
`DS:8943:8945` (the last record in the eight-entry table), rejects lower
scores, scans for the first existing record not greater than the new score,
and rejects an insertion index of 7 or higher. It shifts 16-byte records in
the `DS:88CA` table, writes the score dword at record `+0x09`, copies
`DS:880E` at `+0x0D`, and stores the progression byte at `+0x0F`
(normalizing progression `0x16` to `0x15`). It then renders the name/score
screen and calls the file writer through `01E7:356F` (the bytes immediately
before that address are the `Score.DAT` string literal, so `3565` is not a
callable entry). The writer encodes the 128-byte table, computes the
checksum, and performs the file I/O. The insertion caller's following helper
at `01E7:39F0` restores the DOS interrupt-8 vector. `0227:36ED` performs the
inverse load/decode. The Lua score-file probe now arms the callable `356F`
body and labels the vector-restore separately.

## Runtime validation

The existing debugger-enabled DOSBox binary from the main worktree was used
through `quikytrace.py`; the player probe now records these state words at each
object-update sample. A menu-to-`W1L1` run, holding Right, produced these
observable transitions (frame counts are probe-relative):

| Run/sample | Player position | Health | Lives | Ammo | Score | Observation |
| --- | ---: | ---: | ---: | ---: | ---: | --- |
| short/1 | `(128,400)` | 3 | 4 | 0 | 0 | menu/demo initializer state |
| short/2 | `(257,400)` | 3 | 4 | 10 | 50 | ammo pickup `0x6F` |
| short/3 | `(405,400)` | 2 | 4 | 10 | 50 | damage applied |
| short/7 | `(1132,353)` | 1 | 4 | 12 | 50 | another ammo pickup and damage |
| short/10 | `(1635,368)` | 2 | 4 | 18 | 150 | health pickup `0x71` and score award |
| death/8 | `(1258,336)` | 0 | 3 | 12 | 50 | death decremented lives |
| death/10 | `(128,400)` | 3 | 3 | 12 | 50 | respawn restored health and position; score/ammo persisted |

The same probe selected later levels without gameplay input and observed the
runtime progression fields: `W1L3` = `0x02` (route 1), `W2L1` = `0x03`
(route 2), `W3L1` = `0x06` (route 3), and `W4L1` = `0x09` (route 4). This
matches the static grouped level-state loop.

A debugger-only traversal seeded current health to 1000 and lives to 99 so
the player could cross hazards without changing normal game logic. During the
500-frame segmented run, the spawn table stayed `(128,400)` through player
x=1519, then changed to `(1673,374)` between x=1519 and x=1670. A longer
Right+Space probe observed a second table mutation to `(3097,342)` around
x=3530. The value is the exact position-table write shape from `01F7:924B`
(object position plus `(0x19,0x46)`), and it is the table consumed by
`01F7:1AAA`; the
`checkpoint_index` word stayed zero. This confirms a live checkpoint/spawn
update, while also showing that the bundled level uses the first position-table
slot rather than advancing `DS:85D2`.

A seeded Right+Space traversal reached x≈4122. The live pool then contained
the level-end entity at `(4176,192)` with callback `0x9269`; a timed Up cadence
reached `(4176,200)` without changing `DS:89E6`, so the overlap/byte-`+0x37`
gate was not satisfied by that input pattern. A debugger-only hybrid probe
then frame-stepped the loaded endpoint and forced player byte `+0x37` to zero
for one controlled collision. It captured the complete endpoint path:

```text
01F7:0ED1  AX=0x9269, DI=0x0618  goal callback dispatch
01F7:9269                          goal callback entry
01F7:92A9                          transition-signal write entry
01F7:92AF  DS:89E6 = 0xFFFF       write completed
```

After 300 guest frames the signal remained latched (`DS:89E6=0xFFFF`), while
score (`250`), progression (`0`), and the level selector were unchanged. No
runtime `+5000` score tally or progression update was observed. A follow-up
probe also stopped at the main-loop signal check `01D7:48E6`; releasing that
forced endpoint state for 120 guest frames left the signal, score, and
progression unchanged. This confirms the consumer checkpoint but also shows
that the debugger-forced endpoint does not by itself reproduce the normal
level-transition/tally route.

The same seeded Right+Up traversal with the `01F7:4968` state-event
breakpoint armed reached x≈4194 without firing that tally breakpoint; score
remained 250, progression remained 0, and `DS:89E6` remained zero. This
separates the W1L1 endpoint entity from the object-side `+5000` tally path
rather than treating a missed score update as a debugger artifact.

Archive inspection identifies the W1L1 endpoint precisely: ARE record `0x180A`
(decimal `6154`), type `0x28`, at world position `(4176,192)`. Its dispatch
entry is `01F7:9256`, which installs callback `01F7:9269`; the `01F7:4968`
score-tally routine is therefore not the endpoint entity's normal update
callback and must belong to a later transition object/state.

A debugger-only overlap-seeded run then passed all endpoint bounds checks and
captured the corrected transition chain:

```text
01F7:9282, 9289, 9296, 929D, 92A7  endpoint bounds/gate checks
01F7:92A9, 92AF                         DS:89E6 = 0xFFFF
01D7:48E6, 48EB, 4968, 497C, 4B8D      main signal consumer/setup
01D7:4BA4, 4C43, 4CFC, 4EA0, 4EAA      transition scene path
```

The signal was now observed as `0xFFFF` at the main-loop checks, eliminating
the earlier tracer race. Without continuing the scene, score remained 250 and
progression remained 0; the distinct `01F7:4968` tally still did not run.

With a debugger-only Space continuation pulse, the post-scene snapshot was
read from the original game-state selector even after the CPU changed `DS`.
It showed `DS:85D4` advancing from 0 to 1, `DS:85D6` remaining 0, and route
`DS:85D8=1`; the selector/level context had advanced successfully. The final
HUD score sample was 810 after the transition activity, but no `+5000` write
at `01F7:4973` occurred. This establishes the W1L1 progression result and
separates it from the unused-in-this-route object tally.

The follow-up artifact `research/build/actual-state-goal-alternate-object.json`
armed `01F7:487F`, `01F7:489C`, and `01F7:4968` immediately after that Space
continuation. None of those alternate-object breakpoints fired before the
next-context snapshot; the W1L1 transition therefore does not instantiate the
object-side tally in the observed window. The result is negative evidence for
W1L1, not proof that the handlers are unreachable: their static code still
belongs to a later/final-level or cutscene state.

A fresh non-cheat W1L1 traversal (`research/build/actual-state-w1l1-goal-normal-window2.json`)
also reaches the real endpoint region without any debugger state injection:
the player advances from `(128,400)` to `(4236,208)`, the endpoint object is
present at `(4176,192)` with callback `0x9269`, and score/ammo/health continue
to update normally. In that run `DS:89E6` stays zero because the player passes
outside the endpoint's strict 16-pixel overlap window; this is a useful normal
route boundary, not evidence that the callback is dead. The earlier forced
overlap artifact remains the authority for the positive signal-write path.

Archive decoding puts the sole W5L4 type-`0x28` exit at `(1248,432)`
(`W5L4.ARE`, record offset `0x15A4`, layout cell `(19,6)`).
A debugger-only W5L4 probe added camera seeding and armed the alternate
handlers, but changing the player/camera coordinates alone did not cause the
engine's region loader to populate that distant exit. A normal diagonal
traversal was attempted with the same instrumentation, but exceeded the
debugger trace's instruction budget before reaching a completion event. The
tool now records `487F` and `489C` as state-event targets, so a later bounded
final-level traversal can identify the object without another code change.

The bounded follow-up artifacts
`research/build/actual-state-w5l4-completion-entry2.json` and
`research/build/actual-state-w5l4-score-save-probe.json` used normal 800-frame
movement chunks. The resulting failure captures visibly reach the game's
`BONUS LEVEL` screen; the debugger-only completion probe did not hit
`01F7:487F`, `489C`, or `4968`, and the W5L4 state remained on route `4`
without the separate object-side `+5000` tally. A second endpoint-seeded run,
`research/build/actual-state-w5l4-alternate-endpoint-probe2.json`, held the
player on the sole W5L4 type-`0x28` exit at `(1248,432)` for six samples and
again hit none of the three handlers. The same alternate-handler probe was
also armed during a W1L4 endpoint experiment; that level did not reach a
player-update barrier after selector launch, so it is treated as an
inconclusive harness run. A follow-up
`research/build/actual-state-w5l4-alternate-callers-probe.json` also armed the
five statically identified caller blocks (`B82B`, `C0E2`, `C933`, `D2A8`,
`DBE9`) and saw no caller, initializer, callback, or tally hit. Together with
the static census (the only direct
`DS:881C += 5000` is `01F7:4973`, reached from `01F7:4968`), this rules out
the ordinary W1L1/W5L4 endpoint as the later tally object but does not identify
the cutscene/bonus state that invokes it.

The targeted caller decompile narrows that state further. `01F7:487F` is not
an ARE type-`0x28` callback or a dispatch-table entry: it is reached from five
world-specific cloned movement/state blocks at `01F7:B82B`, `C0E2`, `C933`,
`D2A8`, and `DBE9`. Those blocks set `DS:88AE` to phase 5 or 6, then create
or reposition a follow-on object before returning to the pooled-object update.
`01F7:487F` installs callback `01F7:489C` and an initial velocity; `489C`
performs the movement/collision phases, and only its terminal overlap branch
enters `4968`/`4973`. This explains why endpoint seeding misses the tally: the
invoking object is a later state-machine product of those world-specific
blocks, not the visible cloud/exit entity. The five callers are now the
preferred static anchors for a future controlled object-state trace.

The caller families are confirmed as ending/cutscene assets by their sprite
slots and by the selector-driven BOB loader in `01D7:3CF8`. The five anchors
resolve as follows:

| caller | stage write | follow-on initializer | sprite slot | archive asset evidence |
| --- | ---: | --- | ---: | --- |
| `01F7:B82B` | `DS:88AE=5` | `B84D` / `B87B` | `0x386` (902) | `END1/3/4/5.BOB` family |
| `01F7:C0E2` | `DS:88AE=5` | `C104` / `C1A0` | `0x0ED` (237) | uniquely `END2_W.BOB` |
| `01F7:C933` | `DS:88AE=6` | `C955` / `CB11` | `0x389` (905) | `END1/3.BOB` or `END5_E.BOB` |
| `01F7:D2A8` | `DS:88AE=5` | `D2F6` / `D63D` | `0x3B6` (950) | `END1/3/4/5.BOB` family |
| `01F7:DBE9` | `DS:88AE=6` | `DC09` / `DD22` | `0x387` (903) | `END1/3/4/5.BOB` family |

The caller decompile was rerun with functions forced at the otherwise
unreferenced continuation addresses. The resulting report is outside the
worktree at `/home/joao/dev/quiky-ghidra-decomp-actual-state-20260825/`
(`QUIKY_SEG03.bin.c`). It confirms the late-scene object contract: `B824` is
the phase-5 continuation that calls `487F`; `487F` installs callback `489C`,
clears the object state byte, and seeds velocity `0x11000`; `489C` moves the
object in 16.16 coordinates and tests the collision probes; the terminal
overlap path enters `4968`, which adds `5000` and either spawns the
route-specific continuation (`85D8` in `{1,3,5}`) or latches
`DS:89E6=-1`. This is stronger than the earlier address-only census because
the branch and object-field writes are now represented in one decompiler
report.

The shared slots are intentional: the loader swaps the world-specific BOB
member before the scene starts. Its progression branches load `END1.BOB` at
selectors 2 and 15, `END2.BOB` plus `END2_W.BOB` at selector 5, `END3.BOB` at
8, `END4.BOB` at 11, and `END5.BOB` plus `END5_E.BOB` at 14. Thus `88AE` is
the scene phase byte, not a gameplay progression counter; the progression
counter remains `DS:85D4`, and the `+5000` tally is a late scene overlap side
effect rather than ordinary exit completion.

Static disassembly identifies the high-score insertion entry at
`01D7:0703`; its body writes the current score, `DS:880E`, and the low byte of
`DS:85D4` into the selected record before the file-write calls. A debugger-only
run with score `0x7FFFF000` and a last-life death reached `health=0`,
`lives=0`, but did not enter `01D7:0703` or the `0227:3565` score-file writer.
The high-score path is therefore gated by a later menu/game
over transition that remains to be driven explicitly.

The post-death menu renderer/input pair is statically identified at
`01D7:0470`/`01D7:04BA` (the visible `INFO`, `SOUND`, `QUICKY`, `EXIT` menu).
A separate segment-1 options menu at `01D7:0D1F` is also fully decoded:
`DS:3632` holds the selection (0, 1, or 2), Up tests action bit `0x02` and
advances by two modulo three, Down tests bit `0x01` and advances by one, and
Space/Shift (`0x30`) confirms. Selection 2 is the exit branch. This proves
the intended third-choice input sequence statically even though the live
post-death helper uses selector `0207` and does not execute this function in
the bounded run.
A narrowed menu probe now drives a seeded last-life run through `GAME OVER`
and into that menu; the failure capture is
`research/build/actual-state-w1l1-menu-right-up-space-pulse.json.failure.png`.
The initial probes missed those entries by one or more bytes because the
breakpoints were placed after the `push bp` prologues.  The probe now arms the
exact game-over/overlay entries `01D7:0C2C`, `0D1F`, and `1084`, plus the
corrected high-score insertion entry `01D7:0703`, across the `01D7` and live
`0207` overlay selectors.  The corrected failure capture's CPU is stopped at
`0207:10C8`, inside the `1084` dispatcher body, while the `INFO/SOUND/QUICKY/
EXIT` menu is visible.  This identifies the runtime overlay even though its
entry breakpoint was passed before the final capture barrier.  That dispatcher
calls the high-score insertion entry only when `DS:89F2 == 0` and the five
session words `DS:88B0/88B2/88B4/88B6/88B8` sum to zero; the seeded last-life
run does not satisfy that gate.  A separate high-score-only probe
(`research/build/actual-state-w1l1-highscore-space-pulse.json.failure.png`)
reached the menu; the forced-gate follow-up hit the preceding live display
entry `0207:0A35` in
`research/build/actual-state-w1l1-highscore-force-gate.json` but still did not
reach `0207:0703`, so insertion is a later branch than the first high-score
screen.

The caller that prepares this gate is `01D7:50B1`: its reset/finalization
block at `5190-51A7` clears all five `88B*` words and then calls `1084`.
At `50C6`, `DS:88AF` selects whether the fresh-game score reset is skipped;
the nonzero/preserved-score branch is the only route that can make `0703`
see the previous run's score. The normal last-life menu is therefore
insufficient; the remaining run must capture this one-time finalization call
before the restart replaces the session context.

The post-finalization return contract is now clear from the same static
block: after the `01D7:1084` menu call, `50B1` reads the result byte in
`DS:3644`, derives the next lives value as `DS:880A = DS:88B0 + 4`, and
loops back through `1084` until result `3`; it then calls the save/cleanup
routine at `01D7:347A`. Result `2` resets `DS:85D4` to zero before re-entering
the loop. This makes the missing dynamic evidence precise: we need the
result-3 exit branch (and the preceding `50B1` call), not another ordinary
QUICKY restart capture.

The score-file probe was corrected after the static pass: `01D7:08D1` targets
`01E7:356F`, while `01E7:3565` is the preceding `Score.DAT` string literal.
`--player-high-score-probe` now arms `36ED` (load), `356F` (write), and
`39F0` (interrupt-vector cleanup) in both selector aliases. A short launch
validation (`research/build/actual-state-score-file-probe-validation.json`)
still stops at the earlier `0207:0A35` display entry, confirming that the
writer remains downstream of the display/menu branch rather than being an
initialization side effect.

The successful W5L4 score-save probe also captures the real bonus-scene
boundary: after the normal traversal reaches `0207:10B0` (`bonus_scene_entry`)
with `DS:89E6=0xFFFF`, the object pool is cleared and the subsequent sample
returns to the gameplay barrier with route `4`, progression changing from
`19` to `11`, and score preserved at `2250`. No `487F`, `489C`, or `4968`
breakpoint fires in that scene window. This confirms the bonus transition and
progression reset, while leaving the late `+5000` object caller as a separate
ending/cutscene path.

That input cadence is now reproduced: holding Right with intermittent Up only
through sample 4, then pulsing Space, reaches the `QUICKY` choice after
`health=0/lives=0`; the final snapshot resets to `health=3`, `lives=4`, and
`score=0` in `research/build/actual-state-w1l1-restart-menu-downshift.json`.
The matching insertion-only probe still did not hit `0207:0703`, even with
the gate override; the remaining trace should break at `50B1`/`51A7` and read
the gate words immediately before the `1084` call.

The gate snapshot also resolves `DS:89F2`: the selector-driven runs report
`89F2=1`, because the `QUIKYSUPERHERO` cheat phrase itself sets that byte at
`01D7:134F`; the five `88B*` words are zero.  A normal (non-selector) launch
reports `89F2=0`, so high-score insertion tests must use that path rather than
the convenient cheat-level launcher.

The selector distinction is now confirmed dynamically. The segment-2 helper
at `0207:10B0` is the live GAME OVER/menu entry; its confirm/restart path
returns to gameplay with health 3, lives 4, and (in the cheat-selector path)
score reset to zero. It is not the segment-1 high-score dispatcher despite
sharing offsets such as `10BA`/`10BD`. The focused helper artifact is
`research/build/actual-state-gameover-menu-path.json`.

The non-cheat path (`DS:89F2=0`) preserves a seeded high score across the
restart and reaches the score-runtime interrupt-vector helper at
`01E7:39F0`, but the disposable `SCORE.DAT` comparison remained byte-identical
and no `01D7:0703` insertion entry fired. The artifact is
`research/build/actual-state-highscore-normal-gate-path-diff.json`. This is
evidence that the ordinary GAME OVER/QUICKY restart is not the high-score
submission trigger; the later completion/exit flow still must be driven to
exercise the insertion screen.

The normal restart snapshot also reads `DS:88AF=1` before and after the menu
helper (`research/build/actual-state-normal-restart-gate-field.json`). This
confirms that the preserved-score branch is selected, but the one-time
`50B1 -> 1084` call occurs outside the post-restart player barrier; a faithful
recreation must run that finalization call before clearing or replacing the
session score.

A disposable-SCORE.DAT high-score probe with debugger-forced gate clearing
reached `health=0/lives=0` after the seeded last-life traversal and then
entered the segment-2 GAME OVER/menu helper at `0207:10CB`; this is not the
segment-1 high-score dispatcher at `01D7:1084`. It did not reach `0703` or
`356F` before the menu input wait timed out, and the source
`SCORE.DAT` was restored from a byte-for-byte backup afterward. This narrows
the remaining problem to the exact post-game menu input/branch timing rather
than the score threshold or file format. The failure capture is
`research/build/actual-state-highscore-focused-death10.json.failure.json`.

A follow-up same-offset probe reached `0207:10BA` and `0207:10BD`, but the
segment-2 disassembly shows those addresses belong to the GAME OVER/menu
helper, not the segment-1 high-score gate. Therefore these samples provide
negative selector evidence only; they do not prove that the high-score gate
opened. The actual gate targets are now restricted to `01D7:1084`/`0703`,
while the SCORE.DAT aliases include segment-2 selector `0207`. The misleading
same-offset samples remain archived in
`research/build/actual-state-highscore-insertion-entry.json` and the
prologue-target variant is in
`research/build/actual-state-highscore-insertion-prologue.json`.

The explicit third-choice exit follow-up is also bounded. With automatic
Space confirmation disabled, the normal last-life run reaches `0207:10B0`,
the QUICKY/restart helper, but the injected Up-then-Space sequence produces
no `01D7:50B1`, `1084`, `0703`, or SCORE.DAT writer hit; the disposable score
file remains byte-identical. This is a negative selector result: the live
helper is already the restart path, not the segment-1 `INFO/SOUND/EXIT` menu
at `01D7:0D1F`. The Lua probe now caps this negative branch's wait at five
seconds so a missing chain cannot consume the full trace timeout.

## Remaining runtime work

The static and current runtime evidence are sufficient to instrument the next
probes:

1. Pickup immunity and ordinary damage grace are now runtime-confirmed: the
   shared player timer counts down from 700 frames for type `0x72` and from
   210 frames after damage, clearing `DS:8810` when the former expires.
2. The checkpoint-index write-xref is complete: the five reset-only stores
   are covered by `--player-checkpoint-probe`; the two live spawn updates
   mutate `DS:8828` but leave `DS:85D2=0`.
3. The final/bonus route pass is bounded but negative: W1L1, W5L4's normal
   traversal, W5L4's exit seed, and startup traces for W2L3/W3L3/W4L3/W5L3
   do not execute `01F7:4968`. The targeted decompile is now complete for the
   five static callers; the next dynamic pass should drive a third-level
   completion far enough to observe `DS:88AE` entering phases 5/6.
4. The ordinary GAME OVER/QUICKY restart is now shown not to submit a high
   score: it preserves the seeded score in the non-cheat path, but does not
   enter `01D7:0703` or change `SCORE.DAT`. The remaining trigger is the later
   completion/exit flow; capture `01D7:0703` there and compare the file bytes
   before/after insertion while confirming the game-facing meaning of
   `DS:880E`.

The attempted early-finalization probe also establishes a tooling boundary:
the debugger Lua script is attached after the NE process has already executed
the initial `01D7:5089` entry flow, and later gameplay launches do not call
`01D7:50B1` again. Arming `01D7:5089` and `01D7:50B1` before the host's replay
barrier therefore still produces no hit; the run times out at the live
gameplay selector with `SCORE.DAT` unchanged. This does not invalidate the
static `50B1 -> 1084 -> 0703` chain, but capturing its one-time entry requires
starting DOSBox at a command prompt (or attaching a debugger before the EXE is
launched), rather than loading a Lua trace after auto-exec.

A bounded command-prompt attempt was made with
`research/automation/quiky_manual_highscore_probe.lua` and
`research/tools/manual_highscore_probe.py`. It successfully launched the
bundled executable from an isolated DOS prompt (the failure capture shows the
normal `INFO/SOUND/QUICKY/EXIT` screen) and preserved `SCORE.DAT` byte-for-byte,
but no `01D7:50B1` hit occurred during the menu/startup window. This is useful
negative evidence: `50B1` is not ordinary program-entry code, so the next
dynamic run must reach the post-completion finalization state rather than just
launching the executable manually.

The finalization menu helpers were also forced through the Ghidra annotation
pass. The `01D7:0703` decompile now directly shows the eligibility comparison
against the eighth record, first insertion-index scan, 16-byte record shift,
copy of `DS:880E` and low `DS:85D4`, progression `0x16 -> 0x15` normalization,
name-entry rendering, and the downstream writer call. The surrounding
`01D7:0470/04BA` routines are only the post-death INFO/SOUND/QUICKY/EXIT
renderer/input helper; they do not themselves prove the high-score submission
trigger. The remaining uncertainty is therefore the dynamic caller and menu
result that enter `50B1`, not the record format or insertion algorithm.

An independent W2L1 sanity pass (`research/build/actual-state-w2l1-checkpoint-
sanity.json`) shows that checkpoint storage is world-local: its table begins
`(192,496)` rather than W1L1's `(128,400)`, the player reaches progression 3,
and `DS:85D2` remains zero throughout. The four sampled states include an
ordinary damage decrement and an ammo pickup, with no executable checkpoint
index advance.

The extended W1L3 pass (`research/build/actual-state-w1l3-ending-space-
pass.json`) reaches progression 2 and moves from `(144,496)` to the first
vertical obstacle around `(790,640..880)`, but does not enter `01F7:487F`,
`489C`, or `4968`; `DS:88AE` stays `0x0101`. A longer goal-probe variant
timed out waiting for the W1L1-specific goal callback, so it is negative
evidence only—not proof that W1L3 lacks a completion trigger. The remaining
ending work is a route-aware input trace (or direct start-at-command-prompt
debugger run) that clears this obstacle and observes the phase-5/6 caller.

Two bounded follow-ups refine that boundary. `actual-state-w1l3-ending-
spacepulse.json` reproduces the obstacle at `(791,880)` after 4800 guest
frames with normal score/ammo/health updates and no alternate-handler hit;
the obstacle is therefore an input-route limitation, not a missing state
transition. An eight-sample W5L4 continuation,
`actual-state-w5l4-ending-extended.json`, reaches a stable gameplay barrier
after the ordinary traversal (`x=1751`, progression `19`, score `2100`) and
also produces no phase-5/6 caller. The earlier four-sample W5L4 artifact
remains the stronger bonus-scene boundary because its input cadence reaches
`0207:10B0`; extending a different cadence does not supersede that result.

The research harness and SCORE.DAT codec pass the final audit: Python bytecode
compilation, all 50 unit tests, and `git diff --check` succeed. The added
menu-control serialization test covers the no-auto-confirm/exit-probe flags.

### Controlled closure of the late score paths

The late-object probe now executes the original alternate completion handler
under debugger control, rather than reimplementing its scoring. After a real
W5L4 post-level barrier, one pooled object is retargeted to `01F7:489C` and
its terminal state is seeded. The two route variants produce:

```text
route 1: 489C -> 4968 -> 4973 -> 497C (score 0 -> 5000) -> 499E
route 2: 489C -> 4968 -> 4973 -> 497C (score 0 -> 5000) -> 4996 -> 499C (89E6=FFFF)
```

These are controlled-entry artifacts, not proof of the natural level-ending
trigger. They do prove the authoritative `+5000` write, the route set that
continues through `499E`, and the alternate route's transition signal at
`DS:89E6`. The artifacts are
`research/build/actual-state-ending-object-route1-v3.json` and
`research/build/actual-state-ending-object-route2-v3.json`; the reusable
driver is `research/tools/ending_object_probe.py`.

The natural GAME OVER/QUICKY restart still does not enter `01D7:50B1` or
`01D7:0703`; that negative result remains valid. A controlled far-call probe
does, however, execute the original `01E7:356F` SCORE.DAT writer on an
isolated runtime (`research/build/actual-state-score-writer-v4.json`). The
runtime breakpoint is `01E7:356F` (decimal selector 487, offset 13679), and
the writer reads the live 128-byte table at `DS:88CA`, applies the executable's
codec/checksum path, and leaves the isolated `SCORE.DAT` byte-identical for an
unchanged table. This closes the writer/codec dynamic question; the only
remaining high-score uncertainty is the natural caller/menu result that
reaches `50B1` in a completed session.

The high-score finalizer probe also reached `01D7:50B1 -> 50C6 -> 518E ->
5190 -> 51A7 -> 1084 -> 10A1 -> 0703` with a seeded eligible score and
rendered the inserted `AAAAAAA` row. Enter (`0x1C`) is the name terminator;
Space is an ordinary name character. The writer was not reliably caught on
that natural indirect path because the breakpoint is crossed while the
post-name menu is running, so that trace is retained as insertion evidence
while the direct writer artifact above is the authoritative persistence
proof.
