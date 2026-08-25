# Boss behavior — first runtime pass

This investigation lives in worktree `/home/joao/dev/quicky-boss-behavior` on
branch `research/boss-behavior`. The DOSBox Automation debugger build is
reused from the main worktree:

```sh
export DOSBOX_AUTOMATION_BUILD_DIR=/home/joao/dev/quicky/research/build/dosbox-automation-debug
export DOSBOX_AUTOMATION_DEPS_PREFIX=/home/joao/dev/quicky/research/build/sdl-prefix
```

All runs below use the original `QUIKY.EXE` and `NESTLE.DAT`. The level cheat
and selector writes are the existing debugger-only setup; no executable or
archive mutation was used.

## Resource evidence and selector correction

The first run that looked like a W1 boss stage is archived as
`research/build/traces/w1l4-resource.json`. Its raw resource paths are the
important evidence because the selector label and archive naming are not
one-to-one:

| sequence | runtime path | return caller | size |
|---:|---|---|---:|
| 1 | `GAMEDATA\\W1IN.map` | `01D7:36D0` | 7210 |
| 2 | `GAMEDATA\\W1IN.are` | `01D7:3531` | 5352 |
| 6 | `GAMEDATA\\quikyw1.BOB` | `01D7:39ED` | 196056 |
| 10 | `GAMEDATA\\end1.BOB` | `01D7:39ED` | 102643 |
| 12 | `GAMEDATA\\doktor1.BOB` | `01D7:39ED` | 11560 |

`01D7:39ED` is the common BOB resource-load caller. It identifies the load
site, not yet the boss initializer. The old selector table labeled value 15
as `--select-level W1L4`, but that selector loads `W1IN.*`,
`END1.BOB`, and `DOKTOR1.BOB`. The rendered `W1IN` map is a small isolated
scene, not the ordinary scrolling W1L4 platforming map. The neighboring
selector value 16 loads the actual `W1L4.*` assets. The tracer tables now
name selector 15 `W1IN`, shift `W1L4` through `W4L4` to 16–19, and expose
selector 20 as `W5L4`.

This is the current context boundary: the live `END1`/`A234` object and its
speech child are confirmed runtime objects in the W1IN end/intermission
scene. Their fall, speech timing, phase byte, and transition flag are useful
cutscene evidence, but must not be treated as a playable W1L4 boss fight.

The corrected ordinary-level trace is
`research/build/traces/selector-16-w1l4-resource-v1.json`, captured with:

```sh
python3 research/tools/quikytrace.py --launch --headless \
  --select-level W1L4 --count 23 --tail-count 30 \
  --output research/build/traces/selector-16-w1l4-resource-v1.json
```

Selector 16 loads `GAMEDATA\\W1L4.map`,
`GAMEDATA\\W1L4.are`, and ordinary W1 assets only. Neither the initial
resource sequence nor the lazy-load tail contains `END1.BOB`. The selector
labels in the older filenames are therefore provisional; future traces must
record both the numeric selector and the raw loaded path.

Static archive inspection confirms the dedicated families:

| asset | records | slot range | relevant record |
|---|---:|---:|---|
| `END1.BOB` | 14 | 900–956 | record 2, slot 901, 103×57 |
| `END2.BOB` | 4 | 220–271 | family-specific, not yet live-matched |
| `END3.BOB` | 12 | 900–955 | family-specific, not yet live-matched |
| `END4.BOB` | 10 | 900–954 | family-specific, not yet live-matched |
| `END5.BOB` | 10 | 900–954 | family-specific, not yet live-matched |
| `END5_E.BOB` | 3 | 905–955 | auxiliary tall effects |
| `DOKTOR1–5.BOB` | 10 each | 940–994 | record 0, slot 940, 24×16 |
| `SCHROTT.BOB` | 32 | 310–341 | not yet live-matched |
| `MINIUFO.BOB` | 12 | 615–670 | not yet live-matched |

The different `END2.BOB` slot range is a useful warning against assuming that
all five bosses use one identical sprite-slot contract.

## First live object identity in W1IN

The object pool snapshot reports selector `027F`, stride `0x78`, and these
records during W1IN startup:

| pool offset | first callback | live callback | slots observed | position |
|---:|---|---|---|---|
| `0x0078` | `A12D` | `A234` | 901 | `(224,216)` → settles at `(224,326)` |
| `0x00F0` | `A1F8` | `A228` | 904–906 | `(230,187)` onward; state field advances |
| `0x0168` | `A213` | `A22E` | 940–944 | `(255,216)` onward |

Slot 901 resolves uniquely to `END1.BOB` record 2. Slot 940 resolves to
`DOKTOR1.BOB` record 0. The first pool sample sees the initializer-like
callbacks `A12D`, `A1F8`, and `A213` before the live callbacks appear, but the
initializer caller/return chain has not yet been captured.

The exact callback traces are:

- `w1l4-boss-focus-v1.json`: object `027F:0078`, callback `01F7:A234`.
- `w1l4-doktor-940.json`: object `027F:0168`, callback `01F7:A22E`.
- `w1l4-helper-904.json`: object `027F:00F0`, callback `01F7:A228`.
- `w1l4-boss-family-snapshot.json`: slot-range view of all boss-range records.

The constructor entry can now be captured before the selector launch dispatch
consumes its one-shot execution. In `w1l4-boss-initializer-a101.json`, the
breakpoint is `01F7:A101`; the register/stack snapshot has a far return at
`01D7:3159` (`stack_hex` begins `5931d701`). This is the runtime caller anchor
above the W1 boss initializer. In `w1l4-boss-initializer-a12d.json`, the
breakpoint is `01F7:A12D`; `ES:EDI` is `027F:0078`, the newly allocated main
boss object, and the stack begins `fd0e37027f0278`, carrying the constructor's
object selector/offset alongside the return address. These captures use the
debugger-only `--player-callback-no-return` mode because these entries are
constructors/labels rather than ordinary scheduler callbacks.

The 901 object has class/kind word `0`, phase byte `1`, callback data
`0xFFFF`, state field `0`, update-state field `0`, and vertical velocity
`0x00006000` while falling. Its captured bytes from `+0x40` through `+0x77`
are zero in the startup samples, so no health field is assigned yet.

The helper at `0x00F0` keeps callback `A228`, advances its state field from
`0x0A` through `0xE6` in the short trace, and cycles slots 904–906. The
`DOKTOR` record at `0x0168` keeps callback `A22E` and cycles slots 940–944.
Later pool snapshots also show records at offsets `0x0258`, `0x02D0`,
`0x0348`, and `0x03C0` with callbacks `A547`, `A600`, `A6B9`, and `A79F`.
Exact callback captures now identify these as ordinary animated objects rather
than boss-range actors: offsets `0x0258`, `0x02D0`, and `0x0348` use slots 766,
774, and 782 from `FR_W1.BOB`, while `0x03C0` uses slot 4 from `QUIKYW1.BOB`.
They move horizontally at y=386 and advance their local word `+0x38`; they
are not the boss damage or phase objects.

The callback disassembly resolves the primary W1 boss state machine. `A234`
uses object byte `+0x34` as a mode and object word `+0x38` as a timer:

| mode | observed behavior |
|---:|---|
| 0 | fall using `+0x0E` until y reaches camera y + `0x50` (y=326 in the baseline), then count to `0x25` |
| 1 | hold position for about `0x32` ticks; at the mode transition the shared byte `DS:85DA` becomes 1 |
| 2 | add `0x1000` to vertical velocity each callback and move upward |
| 3+ | clear the callback after leaving the camera and advance the shared encounter phase |

At the mode-0 timer threshold, `A234` calls relocation target `A889` with
`AX=0x152`, `BX=0x123`, `CX=0x15D`. This allocates a child at pool offset
`0x0438`, installs callback `A8FE`, and creates slot `349`. Archive lookup
resolves slot 349 to record 1 of `SPRECH.BOB` (122x45), so this child is
currently best described as the boss speech/cutscene object, not a projectile.
Its exact trace is `w1l4-boss-child-349.json`; it remains at `(338,291)` with
no movement fields changing.

The mode-3 path writes `DS:89EC = 0xFFFF`. The main loop checks that word at
`01D7:48DC` and branches to its transition path at `01D7:4968`, where the
word is cleared. Thus `DS:89EC` is a level/encounter completion signal, not a
boss health field. The slot-only timeline captures the boss assembly
disappearing around frame 600 and `DS:85DA` advancing through phases 2 and 3;
the transient `0xFFFF` is consumed by the main loop between samples.

## Controlled observations in W1IN

The experiments in this section are cutscene-context observations. They do
not establish that W1L4 has a boss, that slot 901 is damageable there, or that
the captured object owns a health field.

With no input, slot 901 falls from Y=216 to Y=326 and then remains stable for
the sampled 450-frame window. The boss callback and state fields do not change
in this phase.

A held-left run moved the player beyond the valid horizontal range and caused
coordinate wrap/underflow. The slot-901 record later disappeared from the
active pool, but this run is not valid evidence of a hit, damage event, or
victory condition. It is retained only as a control-path warning.

The recovered control mapping makes Alt the attack input: scan code `0x38`
sets action bit `0x10`; left Shift is scan code `0x2A` and is not the attack.
After establishing left-facing movement, a 10-frame `KBD_leftalt` hold creates
a `SCHUSS.BOB` object. In `w1l4-boss-left-then-alt-v2.json`, pool offset
`0x0258` uses callback `0x45AB` and cycles slots `631` and `632` while moving
left from x=240 through x=220. A jump-plus-Alt run produces the same family
at the same callback. The shots tested so far pass the boss x-coordinate at a
lower y than slot 901, and no boss-state change was observed; these are misses,
not evidence that boss damage is absent. The long no-input run also shows the
boss must descend to y=326 before a meaningful attack-position experiment.

A later phased run (`w1l4-boss-hit-plane-v2.json`) waited until the boss was
at y=326 and mode 1, then held left-facing Alt for one frame. It produced no
`SCHUSS.BOB`/`0x45AB` object at all, while the earlier frame-273 experiment did
produce one. This brackets an additional phase/control gate around the boss
scene; it is not evidence of a successful hit or of boss immunity.

No debugger-only health patch, collision patch, callback disable, projectile
mutation, or executable/archive mutation has been performed in this pass.

## Reproduction commands

W1IN/END1 resource batch (selector 15; harness label `W1IN`):

```sh
python3 research/tools/quikytrace.py --launch --headless \
  --select-level W1IN --count 20 --tail-count 40 \
  --output research/build/traces/w1l4-resource.json
```

Actual W1L4 resource batch (selector 16; harness label `W1L4`):

```sh
python3 research/tools/quikytrace.py --launch --headless \
  --select-level W1L4 --count 23 --tail-count 30 \
  --output research/build/traces/selector-16-w1l4-resource-v1.json
```

Exact W1IN END1 object callback:

```sh
python3 research/tools/quikytrace.py --launch --headless \
  --player-trace --player-object-focus \
  --player-object-offset 0x78 --player-object-callback 0xa234 \
  --select-level W1IN --player-samples 8 --player-frames-between 3 \
  --output research/build/traces/w1l4-boss-focus-v1.json
```

One-shot constructor entry:

```sh
python3 research/tools/quikytrace.py --launch --headless \
  --player-trace --player-focus-callback --player-callback-no-return \
  --player-callback-offset 0xa101 --select-level W1IN \
  --player-samples 1 --output research/build/traces/w1l4-boss-initializer-a101.json
```

Slot-range discovery without a known callback:

```sh
python3 research/tools/quikytrace.py --launch --headless \
  --player-trace --player-object-focus --select-level W1IN \
  --player-samples 8 --player-frames-between 5 \
  --output research/build/traces/w1l4-boss-family-snapshot.json
```

Phased movement/facing and attack input:

```sh
python3 research/tools/quikytrace.py --launch --headless \
  --player-trace --player-object-focus --select-level W1IN \
  --player-input-phase KBD_left:60 \
  --player-input-phase KBD_leftalt:10 \
  --player-samples 8 --player-frames-between 1 \
  --output research/build/traces/w1l4-boss-left-then-alt-v2.json
```

`--player-input-phase WAIT:N` is also available for a debugger-only wait
between controlled input phases. Slot-only focus reads the identification
fields needed for a range scan; supplying `--player-object-offset` restores a
full `0x78`-byte object capture and limits the pool read to that object.

The generic focus mode is intentionally still part of
`quiky_player_trace.lua`; a separate `quiky_boss_trace.lua` should wait until
the callback/encounter orchestration is understood.

The corrected W1L3 run is the first live playable-boss candidate. Selector 2
loads `W1L3.map`, `W1L3.are`, `END1.BOB`, and `DOKTOR1.BOB`. Its pool uses the
same three-role layout but a different callback family: main object
`027F:0078` has slot 951 and callback `01F7:B33B`, the helper at `0x00F0`
uses `B226`, and the Doktor at `0x0168` uses `B25D`. The main object moves
back and forth across the arena while its word `+0x38` advances and its
animation slots change; this is distinct from the W1IN `A234` cutscene
machine. The first W1L3 trace is
`research/build/traces/selector-w1l3-boss-family-v1.json`.

The corrected resource ledger is
`research/build/traces/selector-w1l3-resource-v2.json`. It records the
playable-level order as `W1L3.map`, `W1L3.are`, the W1 animation resources,
`END1.BOB`, `DOKTOR1.BOB`, and the ordinary W1 support assets. This is the
first runtime evidence that `END1` belongs to an actual W1L3 encounter rather
than only to the W1IN intermission.

The W1L3 arena render is `research/build/w1l3-map.png`. The player starts on
the isolated upper-left platform at approximately `(144,496)`; the boss
assembly starts near `(580,558)` over the central arena. A no-input run of
5,100 guest frames (`selector-w1l3-boss-phase-long-v1.json`) leaves
`DS:88AE` at 1, `DS:85DA` at 0, and `DS:89EC` at 0. The main boss, helper, and
Doktor remain alive throughout, so the first W1L3 phase does not advance on a
timer alone. The main object traverses roughly x=245..720 and the Doktor
continues cycling slots 990..994; an incidental child at pool offset `0x348`
appears later with callback `B84D`.

Static disassembly gives the first usable state-machine outline for the
playable boss:

| code | observed role |
|---|---|
| `B33B` | main movement/collision callback; gates on `DS:88AE`, applies bounded horizontal motion, bounces at a collision, and advances animation/timers |
| `B226` | helper callback; creates short-lived child objects and can write `DS:88AE = 2` after its local event count passes four |
| `B25D` | Doktor/helper callback; tests a shared coordinate/event table and spawns or clears pooled records |
| `B84D`/`B87B` | short-lived child callback created by the main phase logic |

`B33B` writes `object+0x3E = 1` when its collision probe succeeds, then uses
`object+0x40` as a direction/reaction sign and `object+0x42` as a countdown.
Those fields are not health fields: the matched no-hit control
`selector-w1l3-nohit-window-v1.json` shows the same transient values
`+0x3E=1`, `+0x40=1`, and `+0x42=0x1B` around frames 800–820 with no player
input. The earlier attack-window trace overlapped this normal movement
window, so its apparent hit signature is rejected. Slot 951→901 is also a
normal animation change during the same bounce cycle.

The projectile path is confirmed. A correctly faced Alt attack creates a
`SCHUSS.BOB` object at pool offset `0x01E0`, callback `01F7:45AB`, cycling
slots 631/632. The callback stores the projectile's integer X/Y pair in the
shared table at `DS:87DE`, with the active count at `DS:8806` and capacity at
`DS:8808` (W1 capacity is 4). The Doktor callback scans that table rather than
receiving a direct child-object collision. On overlap it clears the table
coordinate, increments Doktor `+0x2C`, advances its scan cursor `+0x2A`, and
creates a short-lived pooled effect. This distinguishes the real damage path
from the main callback's player/MAP collision fields: `B33B`'s relocation
targets `01F7:1B77` and `01F7:1C6E` are the player rectangle and map-cell
probes, respectively.

The exact `B25D` decode makes the collision window explicit. Each callback
in its damage-enabled mode checks one projectile-table entry selected by
`Doktor+0x2A`, wrapping the cursor against `DS:8808` and advancing it after
the check. A hit requires the projectile to satisfy the exclusive ranges
`boss_x-15 < projectile_x < boss_x+15` and
`boss_y-25 < projectile_y < boss_y+5`. This cursor matters for replay: a
missed projectile can remain in table entry zero while `+0x2A` scans entries
one through four, so geometric overlap alone does not guarantee a damage
event. A successful hit sets byte `+0x2E` to the invulnerability mode and
counts it with `+0x2F`; the damage scan is suspended until that timer exceeds
100 ticks.

The one-hit trace
`selector-w1l3-doktor-five-hits-v3.json` forces the projectile table entry to
the moving Doktor plane for a debugger-only window. At the overlap boundary,
Doktor `+0x2C` changes from 0 to 1 and the shared projectile count returns to
0; `DS:88AE` remains 1. This is the first direct runtime confirmation of a
boss damage counter. The trace name is retained from the original multi-shot
experiment; it does not prove five natural hits.

The threshold experiment
`selector-w1l3-doktor-phase-threshold-v1.json` starts with a reversible
debugger-only word patch setting Doktor `+0x2C` to 4, then forces one overlap.
At frame 789 the Doktor is still in callback `B25D` with slot 990 and the
forced projectile is active. At the next sample, frame 792, the shared
`DS:88AE` state is 3, the main slot is 950, the Doktor record has been removed
from the focused pool, and `DS:89EC` is still 0. Static `B25D` code writes
state 2 when the post-hit counter exceeds 4; the sampled state-3 value means
the phase-2 handoff is consumed between the B25D and B33B sampling barriers.
This is a state transition, not a health-field mutation. All object/table and
word patches are restored when the trace exits and are explicitly
debugger-only. The broader replay
`selector-w1l3-doktor-phase-broad-v1.json` shows the immediate assembly after
the handoff: the main record remains on `B33B` but changes slot 951 to 950,
the helper remains on `B226` and continues cycling slots 904–906, and the
Doktor record is absent. Across frames 792–825 the runtime state stays 3 and
the completion word stays 0; no ordinary timer-only transition follows in
that window.

## W1L3 post-hit lifecycle

The long-window trace
`selector-w1l3-state3-to-later-v1.json` establishes the remainder of the
W1L3 state ladder under the same reversible threshold experiment:

| sampled frame | runtime state | observed action |
|---:|---:|---|
| 789 | 1 → 3 | Doktor threshold is consumed; main slot changes 951 → 950 |
| 1192 | 4 | state-3 attack cycle has completed; the main record is still on `B33B` |
| 1595 | 5 | main record is reinitialized as callback `489C`, slot 715, at `(251,640)` |

Static `B33B` decoding explains state 3: it advances a waveform table at
`DS:7974`, creates two short-lived `4B70` → `4C74` objects using slot 611,
increments main `+0x44`, and eventually publishes runtime state 4 after the
repeated attack cycle. State 4 falls into callback initializer `487F`, which
installs `489C` and the slot-715 animation.

Callback `489C` is a player/map-contact gate. A debugger-only teleport to the
recorded `(251,640)` position in
`selector-w1l3-state5-player-contact-v4.json` changes the main record from
`489C`/715 to `9627`/724 on the next sample. The W1-specific child records
created at that boundary are:

| pool offset | callback | slot | role currently supported by evidence |
|---:|---:|---:|---|
| `0x168` | `4A5E` | 930 (`KAKAO.BOB`) | transition timer/flag object |
| `0x1E0` | `9313` | 766 (`FR_W1.BOB`) | paired transition animation/object |
| `0x258` | `993B` | 782 (`FR_W1.BOB`) | paired transition animation/object |

The broad contact trace is
`selector-w1l3-state5-contact-broad-v1.json`. The three children remain
active while runtime state 5 stays stable. Child `4A5E` advances its local
`+0x2C` counter to about 305 after 300 frames, but an unpatched continuation
allows the teleported player to die and resets the encounter before the
600-tick branch can be sampled.

The debugger-only child-threshold trace
`selector-w1l3-completion-forced-child-v1.json` sets `4A5E` `+0x2C` to 590
after the child is created. Within the next 15 frames, `DS:89E6` becomes
`0xFFFF`, the boss assembly is reinitialized to runtime state 1, and
`DS:89EC` remains 0. This identifies `DS:89E6` as the W1L3 transition/level
completion signal for this path. `DS:89EC` remains the separately observed
W1IN/intermission completion signal. The child counter and player position
patches are restored and are explicitly debugger-only.

Static call-site analysis locates the gameplay consumer at segment
`0x01D7:0x4EA0`. The loop first checks `DS:89E6`; when nonzero it sets the
transition/UI flag at its local state byte, renders the `0x4533` transition
text, and advances the script-state dispatcher through the `DS:85D4`
branches. The path clears `DS:89E6` at `0x01D7:0x504A`, then resumes through
`0x504F`; it does not directly write the final completion word. The same
routine also handles the ordinary `DS:89E6==0` path, so this word is consumed
by the main gameplay loop rather than by the boss callback itself. The
existing forced-child trace proves the write-to-consumption ordering:
`DS:89E6` is `0xFFFF` at the next scheduler sample immediately before the boss
assembly is rebuilt.

The static continuation at `01D7:4EA0` is now mapped through its dispatcher.
After rendering the transition text, it copies `DS:85D4` to `DS:85D6` and
maps selector/script states 16–20 through `DS:7A2C` values 1–14; the ordinary
states 0/1, 3/4, 6/7, 9/10, and 12/13 map to 0x10–0x14. The `4B8D` re-entry
helper is the branch that decides whether `DS:89E0` is set; after the rebuild
path runs, `DS:89E6` is cleared at `0x504A` and the normal loop resumes only
when `DS:89EC==0` and `DS:89E0!=-1`. Otherwise it enters the final
UI/progression calls. This identifies the completion gate without
mislabeling the `DS:89E6` consumer itself as the victory write.

The W1L3 resource ledger also resolves the other post-hit slots: state-3 slot
611 is `PUFF.BOB`, and state-5 slot 715 is `PAPIER.BOB`. These assignments use
the assets actually loaded by W1L3, avoiding archive-wide slot collisions.

## W1 natural-hit confirmation

`selector-w1l3-natural-five-shot-control-v1.json` is an unmodified five-shot
input run. It creates repeated `SCHUSS.BOB` projectiles and leaves
`DS:88AE=1`, `DS:85DA=0`, `DS:89E6=0`, and `DS:89EC=0`. The projectile table
coordinates are approximately Y=477–484 while the Doktor is at Y=551–565,
outside the statically decoded overlap window. A late Doktor `+0x2C=1` value is
visible, but no matching projectile-table overlap occurs at that boundary, so
it is not accepted as a natural damage hit. A separate plane-alignment attempt
(`selector-w1l3-natural-plane-attempt-v1.json`) produced no projectile because
the teleported player state is subject to an additional attack/input gate.

The player callback trace
`selector-w1l3-natural-hit-player-v1.json` resolves that gate without any
memory mutation: the player walks off the upper-left platform, reaches the
lower arena, uses `KBD_up` to rise through the Doktor plane, faces left, and
fires Alt. The exact Doktor callback trace
`selector-w1l3-natural-hit-b25d-v2.json` then records a real unpatched hit:
the table contains `(454,558)`, `(450,557)`, `(442,556)`, and `(438,555)` at
frames 254–260 while the Doktor is near `(441,551)`–`(440,552)`; `B25D`
increments `Doktor+0x2C` from 0 to 1 and the table clears on the next sample.

The same follow-up maneuver produces a second natural hit in
`selector-w1l3-natural-two-hit-follow-v1.json`. After following the boss left,
the table reaches `(386,556)` against the Doktor at `(382,554)` at frame 391,
and `Doktor+0x2C` changes from 1 to 2. A five-burst continuation
(`selector-w1l3-natural-five-hit-cycle-v1.json`) confirms the first two hits
but leaves later shots active during the post-hit cooldown. After waiting out
that cooldown and following farther left, the re-armed trace
`selector-w1l3-natural-three-hit-rearmed-v1.json` records a third hit at frame
623: table `(273,555)` overlaps the Doktor near `(284,560)`, changing
`Doktor+0x2C` from 2 to 3. The longer continuation
`selector-w1l3-natural-five-hit-complete-v1.json` does not produce hits four
and five because the player route stops emitting attacks after the third
phase. Three unpatched damage hits are now proven; the remaining work is
encounter pacing, not identifying the damage path. The extended cadence below
adds a fourth unpatched hit.

The extended cadence run
(`selector-w1l3-natural-four-five-attempt-v1.json`) advances the same
unmodified route through a fourth hit. The Doktor counter reaches 4 at frame
817, with the projectile and Doktor occupying the decoded overlap window.
After that hit the boss reverses its movement and the player enters the
nonzero `DS:89EA` transitional-control path; the encounter is rebuilt before
an unpatched fifth hit can be sampled. This is a real fourth hit, but not a
complete natural five-hit encounter.

The fifth-hit timing is now reproduced without memory mutation in
`selector-w1l3-natural-player-timed-fifth-v2.json`. After the fourth hit, the
route waits for the player transition to finish, enters the Doktor's vertical
plane, and delays Alt by two frames. Around frame 997 the projectile table
contains the final shot and then clears its active entry during the Doktor
callback; `01F7:B25D` increments `Doktor+0x2C` from 4 to 5 at frame 997. The
pool samples
are scheduler-barrier snapshots after callback ordering, so their nearby
projectile/boss coordinates should not be treated as the exact collision
instant. On the following samples
the object state advances from `DS:88AE=1` to 2 and then 3, while the Doktor
record changes its local state fields (`+0x2A`/`+0x3E`) and is no longer
present in the one-frame pool snapshot. This is the first complete natural
W1 damage cadence. It also confirms that the fifth hit is a boss death-phase
handoff, not a direct write of `DS:89E6` or a final level-completion signal.

The static callback explains the handoff: after `+0x2C > 4`, `B25D` writes
`DS:88AE=2`; `B33B` then changes the main object's animation/child state,
advances to state 3, emits the transient effect wave, runs the delayed
state-4 movement/visibility phase, and finally disables the object after the
off-screen check. The remaining W1 work is therefore to capture that complete
post-fifth ladder and its eventual handoff into the segment-1 transition
consumer, not to find another damage threshold.

The reconstructed natural route was replayed with the main boss record included
at pool offset `0x78` in
`selector-w1l3-natural-old-route-postfifth-v2.json`. At frame 1039 the live
main callback is still `B33B`, its sprite is in the late slot-900 family, and
`DS:88AE=3`; the scheduler contains two `4C74` helper records. By frame 1162
the scheduler shows fresh `4B70` records, while the main callback remains
active. This is unmodified post-fifth/late-phase evidence, but the sampled
route does not yet reach `487F` or the natural `489C` contact/progression
branch, so it does not close the terminal timing gap.

The same route was then replayed without debugger mutations with its late
`WAIT:40` phases subdivided into equal ten-frame observation intervals in
`selector-w1l3-natural-postfifth-dense-v3.json`. This preserves the original
elapsed timing while resolving the reset boundary: the main callback remains
`B33B` in slot 900 and the runtime state remains 3 through frame 1338, with
the expected recurring `4B70 -> 4C74` helper records. At frame 1358 the main
record is reinitialized as `B33B` in slot 951 and the Doktor records return to
slots 990--994; `DS:89E6` and `DS:89EC` remain zero throughout. The natural
route therefore ends in an encounter reload/death-style reset before
`487F`/`489C`, not in the boss terminal path. This is the strongest natural
evidence so far for the W1 post-fifth boundary; it still does not explain the
player-death trigger or provide a natural victory trace.

The follow-up lifecycle-watch replay
`selector-w1l3-natural-postfifth-lifecycle-watch-v4.json` completed with the
same input schedule but did not reproduce the fifth hit: the main record
remained in the pre-terminal slot-901 family through frame 1398, and none of
the staged death, `487F`/`489C`, or SEG01 transition watches fired. It is
therefore a route-reproducibility diagnostic, not contradictory boss
behavior evidence. The earlier lifecycle-watch attempts are likewise
discarded as instrumentation failures (watch stepping was enabled too early
or the callback barrier was disabled before a watched address was guaranteed
to execute).

The player-death trigger is now resolved by targeted Segment 3 decompilation.
The persistent player callback at `01F7:3FF8` calls `01F7:3971` after its
normal movement/collision work. That helper passes the player X coordinate
and `Y - 0x0A - object+0x72` to `01F7:1C92`. `1C92` converts those coordinates
to a 16-pixel MAP cell, reads the loaded cell word, tests `AH & 0x10`, and
returns with that flag in the CPU condition codes. When the bit is clear,
the callback enters the reset branch: it clears `DS:612E`, sets the player
timer at `+0x3E` to `1000`, clears the active/transition bytes, writes the
checkpoint velocity from `+0x32`, and continues through the normal player
cleanup. This matches the observed W1L3 sample at `(368,641)`: the query
coordinates are `(368,591)`, MAP cell `(23,36)`, and the archive cell is
`0x0001` (no `0x1000` flag). The reset is therefore a player fall/death
boundary, not a boss-side reload decision. The loader's separate `0x10`
operation only marks its initial row while normal gameplay cells retain the
archive property word.

### Targeted boss callback decompilation

A fresh headless Ghidra pass using `research/tools/DumpBossDecomp.java` now
decompiles the boss constructors, Doktor callbacks, main callbacks, late
completion callbacks, and world-specific support constructors in one report:
`/home/joao/dev/quiky-boss-ghidra-decomp-targeted-20260825/QUIKY_SEG03.bin-boss.c`.
It resolves two related boss frameworks rather than one universal callback:

- W1, W2, and W4 use the same late ladder. Their main callback leaves the
  damage phase by clearing the linked helper and setting `DS:88AE` to 3,
  performs a 26-update helper/spawn wave before setting it to 4, then runs a
  41-update exit phase before setting it to 5 and retiring off-screen. The
  constructors and movement offsets differ by world, but the `+0x38`/`+0x44`
  timer structure is shared.
- W3 and W5 use the six-state variant. The corresponding late ladder advances
  through `DS:88AE=4`, `5`, and `6`; it keeps the same 26-update wave, 16-step
  child-animation window, and 41-update off-screen exit shape, while adding
  world-specific child links and movement offsets. W3/W5 therefore need the
  same generic phase scheduler but different actor graphs and terminal caller
  states.

The damage callbacks also expose the family split directly. W1 and W3 advance
the world state after five accepted hits (`+0x2C > 4`), W2 and W4 after six
(`+0x2C > 5`), and W5 after four (`+0x2C > 3`), where W5 increments the world
state and clears the per-phase counter for repeated phases. All five use the
same projectile-table scan, the same `+0x2A` cursor, the same collision
rectangle, and the same 100-update rearm counter at `+0x2E/+0x2F`.

The terminal code is now concrete enough to implement as a separate system:
`487F` installs callback `489C`, clears its local mode, and seeds `+0x0E` with
`0x11000`. `489C` first advances the object toward the player. In contact mode
it writes `DS:612E=0x0C` and adds 5000 to `DS:881C`; worlds 1, 3, and 5 then
create the linked phase-2 follow-up objects and call the score/animation
continuation, while the other world IDs set `DS:89E6=0xFFFF` and enter the
same continuation through the alternate branch. Runtime traces still need to
show which world-specific caller reaches this code naturally and how SEG01
consumes the resulting transition.

The matching targeted SEG01 report now closes more of that consumer graph.
`4968` clears `DS:89EC` and the reset word, then either enters the reentry
helper at `4B8D` or rebuilds the level/session state. `4EA0` is the direct
`DS:89E6` consumer: it selects the `0x4533` transition effect, marks the final
world gate (`DS:89E0`) for selector `0x0E`, and transfers into the completion
effect consumer. `504A` clears `DS:89E6` and resumes the main loop only when
`DS:89EC` is clear and `DS:89E0` is not `-1`. `5165` then waits for the final
menu/finalization condition, clears the completion/reentry words, and enters
the high-score/session-save path. The remaining uncertainty is no longer the
static consumer identity; it is which runtime input/reentry branch reaches
these writes after a real boss contact and how the visible level transition
is sequenced.

The post-fourth control path is now correlated more precisely. Segment 3
contains two transition-entry writes, at `01F7:19A3` and `01F7:1A3D`, that set
`DS:89EA` to `0xFFFF`; the nearby player reinitialization path clears it again
at `01F7:1AE6`. The persistent player callback branches to `01F7:4416` when
the word is nonzero. Its transition routine at `01F7:44DC` subtracts one per
callback and tests signed thresholds before returning. The natural fourth-hit
trace enters this path between frames 662 and 687: `DS:89EA` starts near
`0xFFFF` and then counts down while the player is moved by the transition
logic. The boss globals remain `DS:88AE=1`, `DS:89E6=0`, and `DS:89EC=0`, so
this is a player/encounter transition rather than the boss damage handoff.

Raw decoding narrows the unresolved caller context. `01F7:19A3` is the first
`DS:89EA=0xFFFF` write inside a transition routine beginning at `01F7:199D`
and returning at `01F7:19E5`; `01F7:1A3D` is the corresponding write on a
branch inside the adjacent routine beginning at `01F7:19E6` and returning at
`01F7:1A8B`. Both routines initialize transitional player velocity/bounds
fields around the write. The nearby `01F7:1A97`/`1AAA` entries handle the
next player-state setup, and `01F7:1AE6` clears `DS:89EA` before restoring the
ordinary player callback path. This makes the remaining dynamic question the
state-transition dispatcher that enters this routine cluster, not an ARE
entity callback or the Doktor damage callback.

The new object-focus watch can restrict code-offset watches to `01F7` and can
step a selected input window through the focused callback. A normal-route
watch still does not capture the setter because the writes occur during the
input phase before the ordinary sample barrier; the stepped debugger window
changes the input timing and does not reproduce the transition. Its trace is
therefore diagnostic only, not additional natural-hit evidence.

## Targeted decompilation and caller graph

The raw-segment imports were reanalyzed with the targeted Ghidra script
`research/tools/DumpBossDecomp.java`. NE relocation records resolve the
transition-entry callers even though the decompiler cannot connect the
separate raw segment address spaces automatically:

| target | resolved caller(s) | interpretation |
|---|---|---|
| `01F7:199D` | `01F7:43D0` only | player-update camera/vertical-boundary transition entry |
| `01F7:19E6` | `01F7:1BC4`, `01F7:3AB3` | collision/overlap transition paths |
| `01F7:1A97` | `01F7:8DED` | later player transition-state setup |
| `01F7:1AAA` | `01D7:45DE`, `4B6E`, `503D`; `01F7:1B01` | player re-entry/setup; `01D7:503D` is in the post-`DS:89E6` path |

The `43D0` call is reached after comparing the player's Y position against
the camera boundary; it is not called by `B25D`. The `3AB3` path is reached
from the player collision helper after tile IDs `0x0B`–`0x0D` and then enters
`19E6`. In `19E6`, player field `+0x34` gates the transition: the routine
decrements `DS:8822`, sets `DS:89EA=0xFFFF` when that countdown expires, and
otherwise writes `+0x34=0x00D2`. This explains why the fourth-hit handoff is
a player/collision transition that follows the boss event rather than a
direct Doktor-to-victory call.

The decompiled W1 main callback confirms the runtime ladder: `DS:88AE` below
2 runs the moving/animation phase; state 2 removes or changes the child and
advances to state 3; state 3 constructs the transient effects; state 4 runs
the delayed movement/visibility phase; and state 5 disables the main callback
after the final off-screen check. The Doktor callback decomp independently
confirms the five-hit compare and the `DS:88AE=2` write.

The completion word at `DS:89E6` is now tied to the re-entry call at
`01D7:503D`, followed by the clear at `01D7:504A`. The separate gate is
`DS:89E0` (the address is `89E0`, not `E089`). The expanded multi-segment
Ghidra import resolves the previously truncated `01D7:3FAD` function: it is a
level rebuild/resource-loader routine, not a boss-state probe. It derives the
world ID from `DS:85D4`, loads the MAP/ICO resources, initializes the ARE
dispatch table, resets the shared projectile table through `01F7:44FF`, and
returns `1` on the analyzed path. The `DS:89E0` decision therefore belongs to
the surrounding re-entry state machine, not to an unresolved health return
from `3FAD`.

The expanded decompilation also makes `01D7:4B8D` the key re-entry gate. It
holds while `DS:89EA` and `DS:880A` indicate that player transition work is
still active; it clears the transition bookkeeping when the countdown has
finished; and it dispatches by `DS:85DA`. In the script-state branches it can
set `DS:89E0=0xFFFF` directly (including the `DS:85DA=7` and `4` cases), while
the early branch sets it when `DS:85D4=0x0E` and a boss transition word is
pending. After `01D7:503D` runs the rebuild helper and clears `DS:89E6`, the
normal loop is resumed only when `DS:89EC==0` and `DS:89E0!=-1`; otherwise the
flow goes to the downstream UI/progression path.

The same import shows `01F7:44FF` setting `DS:8808=4`, clearing `DS:8806`,
and zeroing the ten shared projectile-table entries. This confirms that the
projectile table is rebuilt as part of level initialization, rather than by a
boss callback.

## Constructor decompilation and completion writers

The constructor pass finds the initializer routines that install the boss
callbacks. They are ordinary same-segment immediate assignments, so they do
not appear as NE far-call relocation targets; the constructor signatures were
located by scanning for `ES:[DI+0x18] = callback` and then confirmed by Ghidra.

| world | main constructor | Doktor constructor | constructor evidence |
|---|---:|---:|---|
| W1 | `B142` → `B33B` | `B20B` → `B25D` | main initializes `+0x38/+0x44`, direction/timers, and child links at `+0x2A/+0x36`; Doktor zeros `+0x2A/+0x2C` |
| W2 | `B9F3` → `BBEC` | `BABC` → `BB0E` | same main shape; helper constructors `C104/C147/C18A` install `C1A0` with slots `0xED–0xEF` |
| W3 | `C28A` → `C40B` | `C30D` → `C328` | main uses `+0x46` in addition to the shared timer fields; Doktor still uses the shared cursor/hit-counter pair |
| W4 | `CC68` → `CE81` | `CD88` → `CDA3` | main links children at `+0x2A/+0x36`; helper `CD25` installs `CD40` |
| W5 | `D2F6` → `D63D` | `D53F` → `D55A` | main links records at `+0x2A/+0x36/+0x48`; effect constructors install `D3EE/D438/D4D9` and scrap constructors install `E0F5/E2BF` |

The Doktor constructors are the strongest generic-model evidence so far:
every world zeros `object+0x2A` (the projectile-table scan cursor) and
`object+0x2C` (the damage/phase counter), while the callback-specific damage
threshold remains in the update routine. Main constructors initialize
`object+0x3E`, `+0x40`, `+0x42`, and timer fields as movement/animation state;
none of these constructor fields is a health value.

The global-writer pass separates the completion signals:

- Playable W1 writes `DS:89E6=0xFFFF` from `01F7:4996`, the post-boss child
  timer at `01F7:4A5E`, and the effect path at `01F7:92A9`.
- Player transition code writes `DS:89EC=0xFFFF` from `01F7:44DC` after
  `DS:89EA` passes its negative countdown threshold. The `A2A5/A2FE/A82E/A874`
  writers belong to the W1IN cutscene family.
- `DS:89E0` is written by the SEG01 re-entry/progression routines. `4B82`
  copies an input value into it, `4ECC` sets it to `0xFFFF` while advancing
  the script state, and `50E7/50EC/50F1` perform the broader progression reset
  with `DS:85D4=0x0F`, `DS:880A=4`, and a call to the level rebuild routine.

This means the boss callback does not directly decide final victory. Its
playable W1 responsibility ends at the `DS:89E6` transition signal; the player
transition and SEG01 progression state machine decide whether that signal
becomes a level exit or a normal encounter rebuild.

The follow-up decompilation corrected one misleading intermediate label:
`01F7:49F2` is not the level-transition dispatcher. It calls the shared
`01F7:5D60` animation/object-state advance helper, which consumes the
object-local timer/animation descriptor at `+0x20/+0x24` and updates the
current sprite slot. The global transition remains the SEG01
`4EA0 -> 4C43 -> 504F` path, subject to the `DS:89EA`, `DS:89F0`, `DS:880A`,
and `DS:85DA` gates. This separates object animation cleanup from level
progression and removes `49F2` from the completion-dispatch model.

## Pooled-object allocator and constructor dispatch

The targeted SEG03 pass now resolves the allocator/scheduler boundary and binds
the boss-family NE far calls to their local targets in the disposable Ghidra
project. The raw executable is not changed. The generated report is
`/tmp/quiky-ghidra-boss-expanded-20260824/out/QUIKY_SEG03.bin-boss.c`.

`01F7:0E06` (`are_object_factory`) is the generic pooled-object allocator. It
accepts the constructor/update callback offset in `AX`, scans the 64 records
from the far pointer at `DS:755E` using the stride at `DS:30CE`, and selects the
first record whose `object+0x18` callback field is zero. It then writes:

- `object+0x18 = AX` and `object+0x1C = 0x1997` (the callback far-pointer
  pair observed in the live scheduler records);
- `object+0x28 = 1`, `object+0x17 = 1`, `object+0x12 = 0xFFFF`,
  `object+0x1A = 0xFFFF`, and `object+0x14 = 0`;
- an active scheduler entry through `01F7:1036`.

The allocator does not clear the whole 0x78-byte record and does not assign a
health field. The callback passed in `AX` is therefore a constructor state: the
first scheduler visit runs that constructor, which replaces `object+0x18` with
the steady-state callback and may allocate children.

`01F7:1036` (`object_scheduler_insert`) appends the active record to the
8-byte scheduler entries at `DS:7566`, using `DS:7966` as the write cursor. The
entry contains the callback offset/segment, the object far pointer, and a
`0xFFFF` terminator. `01F7:0E96` toggles the scheduler buffer and invokes
callbacks in `object+0x17` phase order `0`, `1`, then `2`; each callback is
reinserted through `1036`. `01F7:0FA2` is the nonzero-callback pass, while
`01F7:0E66` counts active records into `DS:88C8`.

The world-specific dispatch chain is now explicit:

| world | level setup calls factory with | constructor callback allocates |
|---|---|---|
| W1 | `B11B` → `B142` | `B142` → `B1F0` helper, `B20B` Doktor |
| W2 | `B9CC` → `B9F3` | `B9F3` → `BAA1` prop, `BABC` Doktor |
| W3 | `C264` → `C28A` | `C28A` → `C30D` Doktor |
| W4 | `CC41` → `CC68` | `CC68` → `CD25` helper, `CD88` Doktor |
| W5 | `D2D0` → `D2F6` | `D2F6` → `D3E1` effect, `D53F` Doktor, `D498` effect |

The constructor offsets are visible immediately before the bound factory
calls (`mov AX, constructor; xor DX, DX; call 01F7:0E06`). The setup routines
also place the first record relative to the camera: W1 `(+0x244,+200)`, W2
`(+0x1C2,+0xDC)`, W3 `(+0x244,-0x26)`, W4 `(+600,+0x96)`, and W5
`(+400,+0x14)`. This is level-script placement logic, not ARE declaration
dispatch. It explains why boss records can start at different pool offsets and
why a boss can occupy a non-900 sprite slot (W2).

This pass also corrects the interpretation of the constructor addresses:
`B142`, `B9F3`, `C28A`, `CC68`, and `D2F6` are not allocator entry points.
They are first-frame constructor callbacks installed by `0E06`; their later
callback writes (`B33B`, `BBEC`, `C40B`, `CE81`, `D63D`) are the actual main
boss update handlers. The Doktor/helper constructor records use the same
allocator and scheduler, confirming a generic object framework with
level-specific callback graphs.

## Targeted phase-helper decompilation

Binding the local far calls inside the boss callbacks removes most of the
remaining `func_0x0000ffff` noise. The important shared helpers are now clear:

- `01F7:1B77` is the boss/player overlap helper. It accepts four signed bounds
  arguments in `AX/BX/CX/DX`, obtains the player-derived bounds through
  `01F7:393C`, and tests the boss position plus the supplied offsets. The main
  callbacks call it with world-specific horizontal offsets (`0x32` in W1/W4,
  `0x41` in W5); this is contact/targeting logic, not damage.
- `01F7:1C6E` performs the 16-pixel MAP-cell read used by the boss movement
  probes.
- `01F7:5C11` is a random-table byte consumer, not a MAP query: it advances
  `DS:6468` and returns a signed byte from the table at `DS:646C`.
- `01F7:1BD1` is the W5 phase actor's MAP collision flag test. `01F7:5C27`
  is the lower-level tile descriptor/flag query used by that actor.

The phase-specific constructor/callback chains are also resolved:

| family | constructor chain | steady-state callback | role |
|---|---|---|---|
| W3 | `C955` → `C9F8` → `CA9B` | `CB11` | three random-velocity phase actors, sprite slots `0x389`, `0x387`, `0x388` |
| W5 | `DC09` → `DCAC` | `DD22` | two random-velocity phase actors, sprite slots `0x387`, `0x388` |
| all worlds shared | `4B70` | `4C74` | short-lived phase child; disables itself after 31 ticks |
| terminal shared | `487F` | `489C` | final post-boss contact/transition child used by all five worlds |

The W3 and W5 actor callbacks cull outside the camera, probe MAP collision,
integrate/clamp fixed-point velocities, and clear their callback on collision
or expiry. They are therefore phase/debris actors rather than additional health
owners. W5's `E44B` and `E1E0` callbacks similarly update auxiliary effects,
using the random table and animation counters. The main boss remains the
phase owner; Doktor remains the projectile-hit owner.

The main callbacks show a common late-phase choreography directly: after the
phase counter advances, they spawn repeated factory-created helper records,
wait on timer fields, move the main record off-screen, and finally enter the
terminal runtime state (`5` or `6`, depending on world). A raw 16-bit sweep
resolved the terminal constructor call sites that Ghidra had previously
treated as interior labels:

| world | terminal factory call | state written immediately before the call |
|---|---:|---:|
| W1 | `B82A -> 487F` | `DS:88AE=5` |
| W2 | `C0E1 -> 487F` | `DS:88AE=5` |
| W3 | `C932 -> 487F` | `DS:88AE=6` |
| W4 | `D2A7 -> 487F` | `DS:88AE=5` |
| W5 | `DBE7 -> 487F` | `DS:88AE=6` |

`487F` installs callback `489C`; that callback applies a short
falling/contact movement and then disables the record after its local counter
reaches two. The repeated `4B70` records are the earlier phase-child wave.
These factory calls create cleanup/transition records rather than changing a
health field. The remaining uncertainty is the exact asset assignment for
each late record, natural timing, and the downstream level-completion
consumer—not whether the worlds share a terminal object contract.

### Late-phase choreography comparison

The refreshed targeted decompilation makes the shared late-phase skeleton
more precise without collapsing the world-specific callbacks into one
implementation:

| world | late state handoff | helper cadence | helper spawn offset | departure | next-owner setup |
|---|---|---|---|---|---|
| W2 | `2 -> 3 -> 4` | every `0x1A`, 16 waves | roughly `(-0x20, 0)` | 41 ticks, then off-screen cull | allocates the next main record at the current position |
| W3 | `2/3 -> 4 -> 5` | every `0x1A`, 16 waves | roughly `(-0x20, -0x14)` | 41 ticks, then off-screen cull | allocates the next main record at the current position; enters state 6 |
| W4 | `2 -> 3 -> 4` | every `0x1A`, 16 waves | roughly `(-0x20, -0x1B)` | 41 ticks, then off-screen cull | allocates the next main record at the current position; enters state 5 |
| W5 | `2/3 -> 4 -> 5` | every `0x1A`, 16 waves | roughly `(-0x20, -0x1E)` | 41 ticks, then off-screen cull | allocates the next main record at fixed X `0x203`; enters state 6 |

The wave records are transient effects, not additional damage owners. The
main callbacks also differ in their movement limits and in the records they
reposition alongside the wave. This is enough to implement a generic phase
scheduler, but not enough to select faithful world assets or reproduce the
natural timing of the handoff. W1 remains the separate movement reference
case because its pre-terminal path differs, but its final object contract is
the same `487F -> 489C` family.

## Targeted terminal-path decompilation

The raw SEG01 bytes around `4EA0`–`504F` resolve the completion handoff more
precisely than the first Ghidra pass. When `DS:89E6` is zero, the consumer
returns through `01D7:504F`. When it is nonzero, it raises the local
transition/UI flag, presents the transition resource at `CS:4533`, starts a
`0x14f`-tick timer, and for level state `0x0e` sets `DS:89E0=0xffff`. It then
enters the resource/rebuild path; `01D7:503D` calls the SEG03 re-entry helper,
`01D7:5047` rebuilds the level state, and `01D7:504C` clears `DS:89E6`.

The final dispatch is explicit: `01D7:504F` resumes the ordinary game loop
only when `DS:89EC==0` and `DS:89E0!=-1`. Otherwise it enters the downstream
progression/UI calls. Thus `DS:89E6` is a boss-transition request consumed by
the level rebuild path, while `DS:89EC` is the later completion/progression
signal. The remaining dynamic question is which `DS:85DA` branch the natural
W1 fifth-hit route selects and which downstream routine turns that branch into
the visible level exit/cutscene.

The direct consumer is now runtime-confirmed with a debugger-only global patch:
`selector-w1l3-transition-consumer-watch-v13.json` sets `DS:89E6=0xffff` for
one sample and watches `01D7:4EA0`. Samples 3–8 stop at `4EA0` with the
transition word still set, `DS:89EC=0`, and the boss runtime state still `1`.
Sibling probes watching `01D7:4C43` and `01D7:4B8D` show the same request being
held by the common reentry gate. This validates the static consumer graph and
also validates the tracer's new preservation of intermediate watch hits; it
is not natural fifth-hit evidence because the global write is debugger-only.

The special branch inside the shared terminal callback is now decoded. On
player contact, `01F7:489C` writes event `DS:612E=0x0c` and adds `5000` to the
shared vertical-transition accumulator at `DS:881C`. For ordinary values of
`DS:85D8`, it sets the child state `+0x2a=2` and writes `DS:89E6=0xffff`.
The child then disables itself on the following callback pass. This is the
concrete boss-contact-to-transition handoff used by the normal terminal path.

For `DS:85D8` values `1`, `3`, or `5`, contact takes a different route. It
allocates a `49ff` record and a `92f2` record at the contact position; the
`49ff` constructor installs callback `4a5e`, sprite slot `0x3a2`, and itself
allocates `9614` and `991a` auxiliary records. Their constructor/callback
chains are:

| constructor | steady callback | key initialization |
|---|---|---|
| `92f2` → `92b3` | `9313` | direction `-1`, vertical velocity `0xffee0000`, counter `+0x2a=5` |
| `9614` → `95c7` | `9627` | direction `+1`, vertical velocity `0xffff8000`, counter `+0x2a=0` |
| `991a` → `98db` | `993b` | direction `-1`, vertical velocity `0xffff0000`, counter `+0x2a=10`, timer `+0x35=0x1e` |

The `9313`, `9627`, and `993b` callbacks are directional transition actors:
they use the shared `1c4d` collision helper and MAP descriptor checks, run
multi-stage movement using `+0x32/+0x33/+0x35`, consume the world random
table for timing, and finish through their own transition animation logic.
The `4a5e` callback tracks the special `DS:85DA=0x32` script phase and has a
long timer path that eventually writes `DS:89E6`. This explains why some
transition traces show several auxiliary records instead of an immediate
completion-word write; it is a scripted transition family, not another boss
health phase.

The raw `01F7:393C` implementation explains an important live-trace guard:
when `DS:89EA` is zero it returns four bounds built from the player record's
`x/y` plus fields `+0x2C/+0x30/+0x2E/+0x32`; when `DS:89EA` is nonzero it
returns zero bounds. Therefore a positioned player
can still fail the `489C` contact test if the transition-control word has
already been claimed by the player transition code.

This was resolved with the genuine terminal path in
`selector-w1l3-terminal-special-contact-v1.json`. The trace naturally reached
`B33B`, allocated `487F`, and installed the real `489C` callback. A reversible
position patch held the terminal child at `(1000,640)` and the player at
`(1000,650)`; a reversible `DS:89EA=0` patch let `393C` return live bounds.
The next scheduler passes captured the special constructors `49FF` and
`92F2`, followed by `9614` and `991A`. The resulting records then installed
the expected callbacks `4A5E`, `9313`, `9627`, and `993B`; the `49FF` record
used sprite slot `930`, while the directional records used slots `766` and
`782` in the observed animation phase. This is runtime confirmation of the
special transition family, but it is a controlled contact experiment: the
natural post-fifth route, exact BOB assignment for these constructors, and
the eventual `DS:89E6`/progression timing remain open.

A debugger-only global patch experiment (`--player-global-word-patch
0x89EA:0:26`) holds `DS:89EA` at zero immediately after the fourth-hit
window. The patch is recorded and restored on exit. With a reversible player
position patch keeping the player about 20 pixels to the right of the live
Doktor and Alt shots spaced beyond the 100-tick rearm period, the counter can
be driven from 2 through 3 and 4 while `B25D` remains active. This confirms
that the `DS:89EA` path is what prevents the ordinary route from continuing.
The separate unmodified timing trace now supplies the fifth hit; this patched
experiment remains useful only as a controlled comparison of the transition
guard, not as natural-hit evidence.

## Cross-world damage thresholds

Static decoding of the world-specific Doktor callbacks shows that the damage
scanner is a shared framework with world-specific phase thresholds:

| world | Doktor callback | hit-count compare | consequence after the next hit |
|---|---:|---:|---|
| W1 | `B25D` | `<= 4` | writes `DS:88AE=2` |
| W2 | `BB0E` | `<= 5` | writes `DS:88AE=2` |
| W3 | `C328` | `<= 4` | increments `DS:88AE`, resets `+0x2C` |
| W4 | `CDA3` | `<= 5` | writes `DS:88AE=2` |
| W5 | `D55A` | `<= 3` | increments `DS:88AE`, resets `+0x2C` |

The compare is performed after incrementing the object `+0x2C` counter, so
the effective hit counts are W1=5, W2=6, W3=5 per phase, W4=6, and W5=4 per
phase. Every callback uses the same projectile table at `DS:87DE`, the same
exclusive rectangle (`boss_x-15 < projectile_x < boss_x+15` and
`boss_y-25 < projectile_y < boss_y+5`), and the same `+0x2E/+0x2F` 100-tick
invulnerability/rearm sequence. W2's callback is also confirmed live by
`selector-w2l3-natural-hit-attempt-bb0e-v1.json`, where an unpatched shot
increments `+0x2C` from 0 to 1.

The phase-reset branches are also dynamically confirmed with reversible
counter/table alignment. In
`selector-w3l3-phase-complete-forced-v2.json`, setting Doktor `C328` `+0x2C`
to 4 and aligning the active projectile to the wrapped scan index produces
the next hit; the runtime state changes from 1 to 2. The corresponding W5
experiment, `selector-w5l3-phase-complete-forced-v1.json`, sets `D55A` `+0x2C`
to 3 and produces the same runtime-state 1 to 2 transition. Both mutations
are debugger-only and restored. Their static main callbacks then show the
next phase handlers: W3 begins at `C788`/`C7BB`, while W5 begins at
`DA3E`/`DA71`, with world-specific sprite and helper sequences.

The later handlers were also entered directly with a reversible
`DS:88AE=3` debugger patch. This is a control-flow probe, not a natural
completion claim. In `selector-w3l3-runtime3-phase-construction-v1.json`,
`C40B` takes the `C788` branch, the main slot changes `951 -> 950`, and a
phase object at `0x0348`/`C328` appears with slot 941. In
`selector-w5l3-runtime3-phase-construction-v1.json`, `D63D` takes the
`DA3E` branch, the main slot remains 950, and the trace shows the
world-specific auxiliary records `0x0348`/`D3EE` slot 955 and
`0x03C0`/`D55A` slot 941. These traces confirm that W3 and W5 share the
runtime-state ladder but construct different child/phase assets; they do not
yet identify the final state-5 consumer or victory write.

## Cross-family L3 sweep

The corrected L3 resource ledgers now establish the boss contexts for all
five worlds:

| level | boss-family resources observed | trace |
|---|---|---|
| W1L3 | `END1.BOB`, `DOKTOR1.BOB` | `selector-w1l3-resource-v2.json` |
| W2L3 | `END2.BOB`, `END2_W.BOB`, `PROP.BOB`, `DOKTOR2.BOB` | `selector-w2l3-resource-v2.json` |
| W3L3 | `END3.BOB`, `DOKTOR3.BOB` | `selector-w3l3-resource-v2.json` |
| W4L3 | `END4.BOB`, `DOKTOR4.BOB` | `selector-w4l3-resource-v5.json` |
| W5L3 | `END5.BOB`, `END5_E.BOB`, `DOKTOR5.BOB`, `SCHROTT.BOB` | `selector-w5l3-resource-v2.json` |

The live object traces show a shared 0x78-byte object stride and shared global
phase/completion words, but not a shared callback implementation. The first
main-family records are:

| level | main callback | pool offset | observed main slot | initial position |
|---|---:|---:|---:|---:|
| W1L3 | `B33B` | `0x078` | 951 | `(580,558)` |
| W2L3 | `BBEC` | `0x0F0` | 270 (`END2`) | `(450,354)` |
| W3L3 | `C40B` | `0x078` | 951 | `(580,288)` |
| W4L3 | `CE81` | `0x078` | 951 | `(600,316)` |
| W5L3 | `D63D` | `0x168` | 950 | `(720,298)` |

The corresponding exact-object traces are
`selector-w2l3-boss-focus-v2.json`,
`selector-w3l3-boss-focus-v2.json`,
`selector-w4l3-boss-focus-v2.json`, and
`selector-w5l3-boss-focus-v2.json`. All five begin with
`boss_runtime_state=1`, `boss_phase_counter=0`, and
`boss_completion_word=0`. Their main records use the same candidate field
locations (`+0x3E`, `+0x40`, `+0x42`, and `+0x44`), initially `0xFF`, `0xFF`,
`0x14`, and `0`; the callbacks then diverge into world-specific movement and
helper logic. W2 and W5 also confirm that the main boss need not occupy the
first allocated boss-looking record or a 900+ sprite slot. The ARE/entity
dispatch table therefore remains the wrong abstraction for bosses; the
level-specific constructors allocate a shared record shape with custom
callbacks and asset contracts.

The new broad ledgers are `selector-w2l3-boss-ledger-v1.json` through
`selector-w5l3-boss-ledger-v1.json`. They confirm that the common object stride
and global state words coexist with world-specific callback families:

| world | main object and asset | other live boss-family callbacks/assets observed |
|---|---|---|
| W1 | `0x78:B33B`, slot 951, `END1.BOB` | `B226` helper; `B25D`/slots 990–994 `DOKTOR1.BOB`; post-hit `PUFF`, `PAPIER`, `FR_W1`, `KAKAO` path |
| W2 | `0xF0:BBEC`, slot 270, `END2.BOB` | `C1A0`/slots 237–239 `END2_W.BOB`; `BAD7`/slots 207–209 `PROP.BOB`; `E6A4/E836`/slots 211–216 `BLASEN.BOB`; `BB0E`/slots 990–994 `DOKTOR2.BOB` |
| W3 | `0x78:C40B`, slot 951, `END3.BOB` | `EB1F/ECB1/EBE8`/slots 700–705 `FLOCKE.BOB`; `C328`/slots 992–994 `DOKTOR3.BOB` |
| W4 | `0x78:CE81`, slot 951, `END4.BOB` | `CD40`/slot 954 `END4.BOB`; `CDA3`/slots 992–994 `DOKTOR4.BOB` |
| W5 | `0x168:D63D`, slot 950, `END5.BOB` | `D3EE`/slot 955 and `D4D9`/slot 906 `END5_E.BOB`; `D438`/slot 904 `END5.BOB`; `E0F5/E2BF`/slots 318/322 `SCHROTT.BOB`; `D55A`/slots 992–994 `DOKTOR5.BOB` |

All four new world ledgers remain at `boss_runtime_state=1` during the short
no-input windows. They are identity and construction evidence, not yet damage
or victory traces. In particular, W2's main boss is below slot 900 and W5's
main boss is in a different pool position, reinforcing that slot-range focus is
only a discovery aid.

## Live allocator-focus results

The player tracer now has a dedicated `--player-factory-focus` mode. It
captures nested far calls to `01F7:0E06`, records constructor `AX`, the far
return, and the returned `ES:DI` object, then stops at the post-update `0F35`
barrier. This is separate from sprite-slot filtering and does not mutate the
executable.

The controlled W3 trace
`selector-w3l3-factory-forced-timer-v4.json` captures the initial constructor
chain (`C28A`, `E9ED`, `EA53`, `C30D`, `EAB9`, and `EAEC`) and, after a
debugger-only state/timer setup, two late calls with constructor `4B70`.
Those records are at `0x618` and `0x690`; after the scheduler pass both have
steady callback `4C74`, phase `2`, and sprite slot `611`. This directly
confirms the statically reconstructed W3 transient-helper wave.

The controlled W5 trace
`selector-w5l3-factory-forced-timer-v1.json` captures the initial chain
(`D2F6`, `DFB3`, `E01D`, `D3E1`, `D53F`, `D498`, `E087`, and `E0BE`), a
`D420` allocation at `0x618`, and two later `4B70` calls at `0x438` and
`0x618`. After the scheduler pass those two late records also become `4C74`
with slot `611`. The W5 result confirms that the same transient-helper
constructor is reused by a different world-specific main callback, while the
surrounding effect records remain W5-specific.

The state/timer patches in both traces are debugger-only and restored. They
prove constructor identity, pool allocation, and post-constructor callback
state, but not natural phase timing or final completion behavior.

The shorter initial-construction traces fill the remaining family baseline:

- W1 `selector-w1l3-factory-focus-v1.json`: `B142 → B1F0/B20B` at
  `0x78/0xF0/0x168`.
- W2 `selector-w2l3-factory-focus-v1.json`: `B9F3`, `E572/E5D8`,
  `BAA1/BABC`, and `E63E/E671` allocate the main, prop/effect, Doktor, and
  support records at `0x0F0` through `0x05A0`; the leading `8C08` callback is
  also observed at `0x78` but remains a setup-side constructor to classify.
- W4 `selector-w4l3-factory-focus-v1.json`: `CC68 → CD25/CD88` at
  `0x78/0xF0/0x168`.

W1's controlled state-3 timer trace
`selector-w1l3-factory-forced-timer-v1.json` adds two `4B70` calls at
`0x1E0/0x258`; after the same scheduler pass they are `4C74`, phase `2`, slot
`611`. The corresponding controlled traces
`selector-w2l3-factory-forced-timer-v1.json` and
`selector-w4l3-factory-forced-timer-v1.json` produce the same `4B70 → 4C74`
wave. W2 allocates at `0x078/0x618`; W4 allocates at `0x1E0/0x258`. After the
scheduler pass all four records use slot `611` and phase `2`. The transient
helper wave is therefore runtime-confirmed for all five worlds, while the
main constructors and first child graphs are also confirmed across the full
family.

### Controlled terminal-branch probe

The earlier debugger-only trace `selector-w1l3-terminal-special-forced-v2.json`
was insufficient because it replaced the callback word without reproducing the
constructor-side object context or the `393C` player-bounds gate. The later
`selector-w1l3-terminal-special-contact-v1.json` run keeps `489C` genuine and
only patches position, local contact state, runtime setup, and the `DS:89EA`
bounds gate. It reaches the special constructor family, so the remaining
terminal research is now timing, asset mapping, and the natural fifth-hit/
progression route rather than constructor discovery.

The bounded state probe
`selector-w1l3-postfifth-state3-factory-v3.json` independently reproduces the
late ladder with reversible one-sample counter/state patches. It records the
real sequence `DS:88AE=3` with main slot `950`, two `4B70` allocator calls,
then `DS:88AE=4`, followed by constructor `487F`; the next scheduler sample
installs callback `489C` and advances the runtime state to `5`. The narrow
probe observes terminal slot `711` during its first animation step; the longer
state-5 trace observes the same animation reaching slot `715` (`PAPIER.BOB`).
This confirms the complete controlled state-3→4→5 callback ladder and its
terminal constructor. The dense natural post-fifth trace now confirms the
timing of the two `4B70` waves and the subsequent reset, but it does not reach
terminal contact or the final progression branch.

### W1 special-transition asset correlation

The remaining W1 asset ambiguity can now be closed by combining the live slot
observations with the W1L3 resource batch and archive-wide `bob-find` results.
`selector-w1l3-resource-v2.json` loads `FR_W1.BOB`, `KAKAO.BOB`, and
`PAPIER.BOB`; the controlled terminal traces observe the following callback
and slot families:

| callback | observed slots | W1 asset mapping | evidence status |
|---|---:|---|---|
| `489C` | 711, 712, 715 | `PAPIER.BOB` records 1, 2, 5 | runtime slot + W1 resource context |
| `4A5E` | 930 | `KAKAO.BOB` record 0 | runtime slot + W1-only loaded candidate |
| `9313` | 716, 766 | `FR_W1.BOB` records 0, 1 | runtime animation slots + dimensions |
| `9627` | 724, 774 | `FR_W1.BOB` records 16, 17 | runtime animation slots + dimensions |
| `993B` | 782, 785 | `FR_W1.BOB` records 33, 39 | runtime animation slots + dimensions |

The archive contains cross-world slot collisions for some of these values
(`930` also exists in `KNUSPER.BOB` and `TRINK.BOB`, while the `FR_W1` slots
are shared by the W1/W3/W5 transition families), so the live resource context
is necessary. This is sufficient to assign the W1 special branch's concrete
BOB families; it does not yet establish the exact frame-by-frame transition
timing or the corresponding W2–W5 assignments.

The same static branch can now be classified by world. SEG01's world dispatch
uses `DS:85D8` as the five-world selector, and `489C` takes the auxiliary
transition branch only for values `1`, `3`, and `5`; W2 and W4 take the direct
`DS:89E6` path. The resource batches provide the corresponding candidate
families for the shared slot contracts:

| world | `4A5E` slot-930 resource | directional transition resource |
|---|---|---|
| W1 | `KAKAO.BOB` | `FR_W1.BOB` |
| W3 | `TRINK.BOB` | `FR_W3.BOB` |
| W5 | `KNUSPER.BOB` | `FR_W5.BOB` |

This is a static/resource-context mapping for W3 and W5, not a live terminal
trace. It narrows the remaining cross-world work to runtime timing, spawn
coordinates, and progression writes rather than asset discovery.

### W3/W5 state-6 terminal controls

The state-6 constructor path is now runtime-confirmed in both six-state
worlds with reversible `DS:88AE=6` debugger patches. The controls are not
natural damage/completion traces; they isolate the terminal factory contract
after the late phase has been entered.

| world | trace | terminal record | observed contract |
|---|---|---|---|
| W3 | `selector-w3l3-state6-terminal-control-v3.json` | slot `711`, callback `489C` | `487F` allocates at the main position, approximately `(578,288)`; `489C` then persists across scheduler passes |
| W5 | `selector-w5l3-state6-terminal-control-v1.json` | slot `711`, callback `489C` | `487F` allocates with X forced to `515` (`0x0203`) and inherited Y `298`; `489C` then moves vertically while retaining the shared slot family |

Both traces show the runtime state as `6` during the controlled window, a
`0x487F` constructor event, repeated `0x489C` updates, and
`boss_completion_word=0`. This confirms that W3/W5 share the same terminal
object and callback contract while retaining the world-specific W5 X write.
The absence of completion is expected: the player was not placed in contact,
so the world-1/3/5 auxiliary branch was not entered. A debugger-only contact
attempt crashed DOSBox in the protected-mode return path and produced no
usable trace; it is excluded from behavioral evidence.

### W3/W5 controlled terminal contact

The contact branch is now live-confirmed in both six-state worlds without
teleporting the player. The experiment kept the player at a valid in-level
position, moved the terminal record to the player, forced its local contact
mode, and cleared `DS:89EA`; all mutations were debugger-only and restored.

| world | trace | contact position | live children |
|---|---|---:|---|
| W3 | `selector-w3l3-terminal-special-contact-v4.json` | `(112,453)` | `4A5E`/slot `930`, `9313`/slot `766`, `9627`/slot `724`, `993B`/slot `782` |
| W5 | `selector-w5l3-terminal-special-contact-v2.json` | `(480,406)` | `4A5E`/slot `930`, `9313`/slot `766`, `9627`/slot `724`, `993B`/slot `782` |

The children remain positioned at the contact point while their directional
records animate and change slots. Both traces retain `boss_completion_word=0`,
which is expected for the W1/W3/W5 auxiliary family: `4A5E` must run its long
timer before it writes `DS:89E6`. This closes the cross-world contact-spawn
gap, but not the timer-to-SEG01 progression or natural encounter timing.

The archived natural post-fifth trace remains the strongest evidence for the
state-3 window, but an unchanged replay in the current automation session
diverged before that window and reset in state 1. The replay is therefore a
route-reproducibility diagnostic, not new behavioral evidence. Future natural
claims require a trace that reaches state 3 in the same run being analyzed.

### Direct terminal branch status

Targeted decompilation closes the structural W2/W4 question even though their
runtime contact probes remain timing-sensitive. `489C` is shared across all
five worlds; after the same contact rectangle and `DS:612E`/score writes, it
tests `DS:85D8`. World IDs `1`, `3`, and `5` allocate the linked `4A5E`,
`9313`, `9627`, and `993B` records. World IDs `2` and `4` instead set the
terminal record mode to `2`, write `DS:89E6=0xFFFF`, and call the shared
`49F2` animation/object-state helper. `49F2` is not the level-transition
dispatcher; SEG01 consumes the resulting word.

The native W2/W4 runtime task is now closed at the branch and effect-wait
boundary. Reversible position patches placed the already-live terminal record
on the player in the native selector: W2 used pool offset `0x78`, W4 used
`0xF0`. Both runs set `DS:89E6=0xFFFF` and reached raw `01D7:4EA0`,
`4EAA`, and `4F0D` with their original world IDs (`2` and `4`) still active:

| world | direct consumer | effect setup | wait/selector branch |
|---|---|---|---|
| W2L3 | `selector-w2l3-native-direct-watch-4ea0-v1.json` | `selector-w2l3-native-direct-watch-4eaa-v1.json` | `selector-w2l3-native-direct-watch-4f0d-v1.json` |
| W4L3 | `selector-w4l3-native-direct-watch-4ea0-v1.json` | `selector-w4l3-native-direct-watch-4eaa-v1.json` | `selector-w4l3-native-direct-watch-4f0d-v1.json` |

The controlled runs are not natural-route timing evidence: the terminal
positions were debugger-only mutations and the probes stop at the watched
SEG01 boundary. They do, however, close the W2/W4 callback-discovery gap and
show that both direct worlds share the same `DS:89E6` consumer sequence as
the W3 special branch.

## SEG01 lifecycle refinement

A later targeted lifecycle decompilation of SEG01
(`/home/joao/dev/quiky-ghidra-decomp-lifecycle1-20260827-b/QUIKY_SEG01.bin.c`)
resolves the raw branch structure around the boss transition more precisely.
The `01D7:4B8D` re-entry routine is a state machine, not a single completion
call:

- While `DS:89EA` and `DS:880A` indicate an active player transition, it
  clears effect/re-entry state and can reload a secondary MAP for selector
  values `2`, `5`, `8`, `0x0B`, and `0x0E` before entering the rebuild path.
- When the player transition has ended, it dispatches scripted branches from
  `DS:85DA`. Cases `2`, `4`, and `6` issue different scripted effect batches;
  cases `7` and `0x34` set `DS:89E0=-1`; case `0x32` runs a multi-step batch and
  changes the transition coordinates.
- The pending boss-transition branch at raw `01D7:4EA0` tests `DS:89E6` and,
  when nonzero, enters at `4EAA`, selects resource `CS:4533`, starts the local
  `0x14F` timer, and sets `DS:89E0=-1` for selector `0x0E`. The following
  `4F0D`/`4FAF` logic maps completed worlds to selectors `0x10`–`0x14` before
  `3FAD` rebuilds the level.
- Raw `4EA0`–`504F` bytes show `4FAD`/`4FAF` as the selector handoff,
  `4F0D` as the resource/effect wait path, `503D` as the post-transition
  re-entry call, `504A` as the clear of `DS:89E6`, and `504F` as the return to
  the normal game loop only when `DS:89EC==0` and `DS:89E0!=-1`.

The forced runtime probe
`selector-w1l3-transition-lifecycle-watch-v2.json` catches `01D7:4B8D` with
`DS:89E6=0xFFFF` on every sampled pass, confirming that the request reaches the
re-entry gate. After fixing the watch-resume barrier in
`quiky_player_trace.lua`, the diagnostic follow-up reaches raw `01D7:4EAA`
before the long transition wait; its timeout is a tooling boundary, not
evidence of final victory. No natural run has yet crossed the `0x14F` wait and
confirmed the visible progression branch, so that remains a runtime gap.

The transition boundary is now capturable without requiring the player object
to survive the handoff. The tracer supports debugger-only callback-identity
word patches, a stop-at-watch mode, and a watch-only barrier after a selected
sample. A W3L3 terminal-timer control uses those facilities to set the live
`4A5E` counter to `600` after the auxiliary child is created, then records:

| runtime label | trace | confirmed state |
|---|---|---|
| `01D7:4EA0` | `selector-w3l3-terminal-timer-callback-4ea0-v1.json` | `DS:89E6=0xFFFF` at the direct consumer; `DS:89EC=0` |
| `01D7:4EAA` | `selector-w3l3-terminal-timer-4eaa-v1.json` | transition request still set; effect setup receives the pending request |
| `01D7:4F0D` | `selector-w3l3-terminal-timer-4f0d-v1.json` | effect-wait/selector branch entered with `DS:89E6=0xFFFF` |

Raw bytes correct the old `4FAD/4FAF` shorthand. `4FA9` increments
`DS:85D4` for selectors that are not in the completed-world pairs; `4FAF`
loads the selector only on the mapped branch, and `5010` is the next
`DS:89E0` gate. W3L3 starts at selector `8`, so it takes the increment-and-
bypass route and does not execute `4FAF`. The post-`0x14F` runtime branch is
therefore still open, but the boss-to-SEG01 request and effect setup are now
live-confirmed rather than only statically inferred.

### Controlled W2 native terminal continuation

The W2 direct path is now traced beyond `4F0D` through the post-terminal
selector branch in `selector-w2l3-native-controlled-post-504a-v1.json`. This
is a debugger-controlled route, not natural encounter timing: the live
terminal record was held at the player, `DS:88AE`/`DS:89EA` were kept on the
known direct-terminal values, `DS:88BA=1` released the input wait at
`01D7:01D6`, and `DS:5044=0` released the resource-service wait at
`01E7:0CB8`. Every mutation was restored on exit.

The native call chain is now runtime-confirmed as:

```text
01D7:4F0D
  -> 01D7:14EC
  -> 01D7:1719
  -> 01D7:01F0
  -> 01D7:01D6   (waits for DS:88BA == 1)
  -> 01D7:171F
  -> 01E7:0CB8   (after the initial FFFF:0000 service call)
  -> 01D7:1724 -> 1728 -> 172D -> 1732
  -> 01D7:4F10 -> 4F1A -> 4FAD
  -> 01D7:5010 -> 5017 -> 3FAD -> 503D -> 5042 -> 504A -> 504F
```

At `4FAD`, W2's script selector advances from `5` to `6`; `5010` then enters
the `DS:89E0`/re-entry work rather than returning directly to the normal loop.
`504A` clears `DS:89E6`; the trace then reaches `3FAD` and the `504F` branch
boundary with `DS:89EC`, the re-entry words, and the completion word still
zero. Under this controlled state, `504F` is therefore poised to take the
normal-loop path rather than the visible completion path. This closes the
exact W2 post-effect code path through the `DS:89E6` clear and the rebuild
helper, but it does not yet prove the natural level-completion/reload result:
the write that makes `DS:89EC`/re-entry state select the visible exit, the
next-level selector, and natural timer/input timing still need runtime
confirmation.

The tracer now supports staged exact watches in the form
`SEGMENT:OFFSET[:START]` and reversible selector/real-mode word patches. This
was necessary because the same `01E7:0CAA` resource helper is also reached by
an earlier loader path; staged watches isolate the later terminal caller and
avoid treating that unrelated call as boss completion evidence.

### Controlled W4 native terminal continuation

The corresponding W4 controlled run is now captured in
`selector-w4l3-native-controlled-post-504f-v6.json`. It reaches the same
native terminal chain as W2:

```text
01D7:4FAD -> 5010 -> 5017 -> 3FAD -> 503D -> 5042 -> 504A -> 504F -> 4601
```

W4 advances its script state from `11` to `12`; the world/selector word then
advances from `4` to `5` at the `504A` boundary. As in W2, `504A` clears
`DS:89E6`, and the observed `504F` sample still has `DS:89EC == 0` and the
completion word unset. Therefore this is a controlled post-effect rebuild
handoff, not visible victory or proof of the natural next-level result.

The follow-up trace
`selector-w4l3-native-controlled-reentry-4b8d-v1.json` reaches `4601` after
`504F` and returns to the player callback; it does not enter `4B8D`. Targeted
static decompilation explains why: `504F` calls `4601` when `DS:89EC == 0`
and `DS:89E0 != -1`, while `4B8D` is called earlier from the completion gate
only when `DS:89F4 != -1`. The same decompilation shows `4B8D` has separate
branches for resource/map rebuild, scripted post-boss effects, and final
completion signaling. It is a conditional re-entry state machine, not the
universal continuation after every boss terminal callback.

The W4 trace required the ordinary initial callback barrier and skipped only
the generic pool/scheduler snapshot on transition-watch samples; all guest
state patches remained debugger-only and were restored. This closes the W2/W4
controlled post-effect handoff comparison while leaving natural timing and
the visible progression branch open.

### Controlled W3 special-branch continuation

The W3 special-branch continuation is now captured in
`selector-w3l3-special-controlled-post504f-v2.json`. Starting from the
debugger-only state-6/terminal setup with the auxiliary timer's `+0x2C` forced
to `600`, the trace reaches the full successor chain rather than stopping at
the first transition gate:

```text
01D7:4F0D -> 14EC -> 1719 -> 01F0 -> 01D6 -> 171F
 -> 01E7:0CB8 -> 1724 -> 1728 -> 172D -> 1732
 -> 4F10 -> 4F1A -> 4FAD -> 5010 -> 5017 -> 3FAD
 -> 503D -> 5042 -> 504A -> 504F
```

The run began with world/selector state `3`, script state `8`, and
`DS:89E6 == 0xFFFF`. `4FAD` advanced the script state to `9`; the rebuild
then advanced the world/selector shadow to `4`. `504A` still held
`DS:89E6 == 0xFFFF`, while `504F` observed it cleared and left `DS:89EC == 0`.
This is controlled evidence that the W3 special branch hands off through the
same cross-world rebuild machinery as W2/W4. It is not visible victory: the
run used a forced auxiliary timer and stopped at `504F`, where the normal-loop
condition is satisfied.

This also closes the earlier tracer-barrier ambiguity. The intermediate
`14EC`/`1719`/`01F0`/`01D6`/`171F`/`0CB8` gates are real successors, not an
unresolved stall. The remaining W1/W3/W5 question is the natural timer value
and the unmodified branch that produces the visible completion/intermission
state, not whether the `DS:89E6` signal can reach the next-world rebuild.

## Targeted decompilation: cross-world combat and phase matrix

The world-specific callback decompilation in
`/home/joao/dev/quiky-boss-ghidra-decomp-targeted-20260825-v2/QUIKY_SEG03.bin-boss.c`
closes several questions that live traces alone could only characterize. The
Doktor callbacks at `B25D`, `BB0E`, `C328`, `CDA3`, and `D55A` all use the same
projectile table and the same hit rectangle:

```text
boss_x - 15 < projectile_x < boss_x + 15
boss_y - 25 < projectile_y < boss_y + 5
```

On a hit they clear the projectile position, increment the Doktor hit count
at `+0x2C`, create the short hit-effect record, and set the per-hit lock at
`+0x2E`. The lock counter at `+0x2F` re-arms it only after `100` update
ticks. The shared cursor is `+0x2A`, wrapping against projectile capacity
`DS:8808`, and the active-projectile count is `DS:8806`.

The decompiled damage thresholds are intentionally not normalized into one
generic health value:

| world family | callback | threshold action | resulting phase behavior |
|---|---:|---|---|
| W1 | `B25D` | hit count `> 4` | writes `DS:88AE = 2` |
| W2 | `BB0E` | hit count `> 5` | writes `DS:88AE = 2` |
| W3 | `C328` | hit count `> 4` | increments `DS:88AE`, resets hit count |
| W4 | `CDA3` | hit count `> 5` | writes `DS:88AE = 2` |
| W5 | `D55A` | hit count `> 3` | increments `DS:88AE`, resets hit count |

The main callbacks confirm two phase families. W1/W2/W4 use the five-state
ladder: the post-hit state-3 helper wave runs for `0x1A` ticks, the next
state's animation/exit counter runs past `0x28`, and the callback then
publishes state 5. W3/W5 add an intermediate state: state 3 first removes
the previous helper and changes the main animation, state 4 runs the same
`0x1A` helper wave, state 5 runs the same past-`0x28` exit, and state 6
publishes the terminal record. The common transient constructor is still
`4B70 -> 4C74`; the surrounding actors and animation slots remain
world-specific.

The terminal callback is shared but its policy is split by `DS:85D8`. For
world IDs `1`, `3`, and `5`, `489C` creates the auxiliary transition graph
(`4A5E`, `9313`, `9627`, `993B`) and leaves the transition signal to the
timer path. For world IDs `2` and `4`, the same callback takes the direct
branch, sets its terminal mode, writes `DS:89E6 = 0xFFFF`, and calls `49F2`.
This is the strongest evidence so far for a generic boss runtime with
level-specific constructors, phase callbacks, and terminal policy rather
than an ARE entity type.

The decompilation also clarifies what it cannot prove. It does not establish
the natural frame on which a projectile becomes eligible, the exact player
contact-damage path, the animation frame-to-BOB-record mapping, or which
`DS:89EC`/`DS:89E0` combination is produced by an unmodified encounter.
Those remain runtime questions and should not be filled in from the
decompiler's unresolved helper calls.

## Archive-side ending BOB frame inventory

The archive parser was run against the ending assets themselves to separate
the available sprite records from the still-unresolved runtime animation
selection. The large ending families are stored as paired base/late slots:

| asset | records / slot families | dimensions and role evidence |
|---|---|---|
| `END1.BOB` | `900/950` through `906/956` | seven paired records; the first four are the large body, small hit/debris, and compact effect stages; W1 live main starts at `951` |
| `END2.BOB` | `220/270`, `221/271` | two large records; W2 live main starts at `270` |
| `END2_W.BOB` | `237`, `238`, `239` | `16x16`, `12x16`, and `8x10` helper/weapon-sized records |
| `END3.BOB` | `900/950` through `905/955` | six paired records; W3 live main starts at `951` |
| `END4.BOB` | `900/950` through `904/954` | five paired records; W4 live main starts at `951` |
| `END5.BOB` | `900/950` through `904/954` | five paired records; W5 live main starts at `950` |
| `END5_E.BOB` | `905/955`, `906` | two tall/small ending-effect records, distinct from `END5.BOB` |

All `DOKTOR1.BOB` through `DOKTOR5.BOB` files have the same ten-slot layout:
`940/990`, `941/991`, `942/992`, `943/993`, and `944/994`. The first record
is `24x16`; the remaining records are `24x15`, with the same origins across
all five files. This supports a shared Doktor renderer/animation contract
with world-specific art data rather than five different object layouts.

The auxiliary files also have regular grouped layouts. `SCHROTT.BOB` contains
32 records in eight four-frame groups at slots `310`–`341`; `MINIUFO.BOB`
contains six base/late pairs at `615/665` through `620/670`. These are now
asset-level candidates for W5 debris and helper actors, but their callback
ownership and natural spawn timing still require live confirmation.

This inventory is intentionally weaker than a runtime mapping: identical
slot numbers collide across ending files, and the archive does not say which
record a callback selects at each phase. The remaining presentation probe
must correlate the live `object+0x12` changes with the loaded world file and
the phase/state callbacks.

## Faithful-recreation boundary

The research is sufficient to define the shape of the boss subsystem, but not
yet sufficient to implement it faithfully. Confirmed pieces are the shared
`0x78`-byte pool/scheduler contract, the two world-family phase ladders,
level-specific constructors, Doktor hit counters and thresholds, the shared
transient helper wave, the terminal `487F -> 489C` family, and the SEG01
transition gates. The remaining fidelity work falls into four separate
categories:

- Natural timing: the W1 post-fifth reset is now explained as player fall/death,
  but we still need a no-death natural or minimally controlled route into
  terminal contact; then repeat the corresponding late phases in W2–W5.
- Combat semantics: exact projectile/boss collision geometry, damage-frame
  eligibility, invulnerability timing, player-contact damage, and reset/death
  behavior. The current hit counts are strong callback evidence, not a full
  collision specification.
- Presentation and assets: W1's special-branch BOB families are now mapped;
  W3/W5 now share the live special-child slot contract; still resolve the
  corresponding W2/W4 records, exact BOB frame assignments, sound/event
  writes, and the visible transition/cutscene sequence.
- Cross-world completion: determine whether W2–W5 share only the generic
  damage framework or also share terminal/progression policy; then validate
  retries, death, and level reload against the original traces. The native
  W2 and W4 terminal branches and post-effect handoff through `504F -> 4601`
  are now controlled-runtime confirmed; natural timing plus post-wait visible
  progression remain open.

Until those four areas are closed, a recreation can reproduce boss phases and
pool behavior but cannot claim frame-faithful encounters or faithful level
completion.

The tracer now preserves the complete guest-visible `trace_config` in each
player/object-pool ledger. That makes input phases, object focus, allocator
focus, pool offsets, watches, and debugger-only patch intervals auditable when
the natural timing passes are repeated; the short replay attempts that did
not reach the fifth hit are intentionally not treated as behavioral evidence.

## Next experiments

1. Obtain a reproducible no-death W1 route into `487F`/`489C`. The reset
   trigger is known, but the archived natural route did not reproduce in the
   latest session; the next valid run must preserve the exact callback/barrier
   conditions and record the route from the same process.
2. Run W2/W4 without debugger release patches if a stable natural terminal
   route can be found. The controlled code path is now known through
   `504F -> 4601`; the write that selects visible completion, next-level
   selector, and reload still requires runtime evidence.
3. Run one debugger-only trace through the confirmed `DS:85D8=1/3/5` special
   branch long enough to verify the `4a5e/9313/9627/993b` timer and
   `DS:89E6` timing. W3/W5 contact spawning is now closed; the long timer and
   progression handoff remain.
4. Follow W1/W3/W5 through the decoded `DS:89E6` consumer and the `0x4533`
   transition sequence. The static gate, timer, rebuild, and signal ordering
   are known; the runtime branch and final visible exit remain.
5. Validate the decompiled W2/W4 five-state and W3/W5 six-state ladders with
   clean late-phase runtime traces, including helper links, sprite-slot
   changes, child spawn coordinates, and world-specific completion callers.
   W3/W5 state-6 construction and contact-driven auxiliary spawning, plus the
   native W2/W4 direct terminal handoff, are now controlled-runtime confirmed;
   natural route/timing validation remains.
6. Close the combat/presentation/retry gaps: player-contact damage, exact
   projectile eligibility and rearm behavior, sound/event writes, final BOB
   mapping, death/retry/reload behavior, and cross-world progression.
7. Once at least W1 and one contrasting world are complete, implement the
   generic boss runtime model around level-specific constructors/callbacks:
   phase/state, damage counter, child/projectile spawning, death transition,
   and level completion.
