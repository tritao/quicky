# Entity behavior families

This note is the interpretation guide for
[`entity-behavior-families.json`](../entity-behavior-families.json). The matrix
is deliberately conservative: a known sprite slot, BOB record, or initializer
address is not treated as proof of movement, collision, or death behavior.
`confirmed` means that the cited static/runtime evidence directly establishes
the claim; `partial` means the entry is a useful boundary but a required part
of the behavior is still missing; `unresolved` means that no durable evidence
currently establishes it.

## Inventory and family boundaries

The archive catalog contains 50 used types grouped into 21 families. The
normal enemy pairs are separate catalog types but share a sprite family; they
remain separate rows because their initializers and update callbacks differ.

| Family | Types | Used worlds | Current strongest identity evidence |
| --- | --- | --- | --- |
| WURM2 | 01/02 | W1 | callbacks 6DA3/6DB1, slots 281/231 |
| BIENE | 03/04 | W1 | callbacks 689F/68AD, slots 276/226 |
| FISCH | 05/06 | W2 | callbacks 7B50/7B5E, slots 254/204 |
| KRABBE | 07/08 | W2 | callbacks 776B/7779, slots 200/250 |
| PENGO | 09/0A | W3 | callbacks 713D/714B, slots 250/200 |
| SCHNEE | 0B/0C | W3 | callbacks 6651/6699, shared slot 209 |
| FLIEGE | 15/16 | W4 | callbacks 7ED7/7EE5, shared slot 208 |
| SPINNE | 17/18 | W4 | callbacks 8451/845F, slots 250/200 |
| BUGGY | 19/1A | W5 | callbacks 5050/505E, slots 250/200 |
| UFO enemy | 1B/1C | W5 | callbacks 5F07/5F15, slots 264/214 |
| animated tile effect | 1F/20/21 | W1-W5 | common 8E4B state machine and world ICO tables |
| cloud | 28 | W1-W5 | WOLKE.BOB special renderer, slots 413-416 |
| falling leaves | 29/2A/2B | W1 | common 4727 path and BLATT.BOB tables |
| paper | 2C | W1-W5 | callback 8C4E, slot 710/PAPIER.BOB |
| wind | 33 | W4 | callback 87D1, slot 214/WIND.BOB |
| bump | 34 | W1-W5 | callback 9BEE, slot 400/BUMP_Wn.BOB |
| UFO effect | 35/36 | W5 | callbacks 544C/545A, slots 264/214/UFO.BOB |
| moving platform | 3D/3E/3F/40 | W1-W5 | common 9DC7 path, slots 300/301/PLATFWn.BOB |
| dedicated event | 65/66/67 | W1-W5 | subtype wrappers and LOOP_Wn.ICO child path |
| pickup | 6F/70/71/72 | W1-W5 | slots 607-610/WERBE.BOB |
| puzzle letter | 79-7F | W1-W5 | slots 600-606/PUZZLE.BOB |

Types `0x73` and `0x74` are present in the confirmed dispatch survey but have
zero archive occurrences and zero dispatch entries. They are intentionally not
counted as used families.

## Confirmed state machines

### Animated tile effects (`0x1F`-`0x21`)

The three initializers differ only in `object+0x2E` (1, 2, 3) and converge on
`01F7:8E4B`; the normal sprite slot remains `0xFFFF`. The callback first gates
the object against the camera and the persistent player bounds object. Once
accepted it advances through states 1, 4, 6, 8, and 10. At states 4, 6, 8,
and 10 it performs five 16-pixel MAP lookups through `01F7:3376`, maps the
low-nine-bit tile IDs through `DS:6986`, and calls `01F7:16CE` for each nonzero
effect entry. State 10 clears `object+0x18` and publishes terminal coordinates
at `DS:8828/882A`.

The children are three-tick pooled objects with callback `01F7:10B5`.
`10B5 -> 1693 -> 1186` applies the visibility test and reads one 256-byte
record from the world ICO resource. Exact byte comparisons exist for W1
through W5 in [`effect-mappings.json`](../effect-mappings.json). This family is
the current reference implementation for all seven matrix dimensions.

### Dedicated events (`0x65`-`0x67`)

The three wrappers set subtype bytes `0x00`, `0x08`, and `0x10` before entering
the common creator at `01F7:1749`. The event ring stores a fixed-point position,
the source ARE record word, a PRNG-derived animation byte, and the subtype.
The event loop advances the animation byte modulo eight and calls `16CE`, which
allocates a pooled visual object with no normal BOB slot and callback `10B5`.
The callback rejects objects outside the camera rectangle, then draws from
`LOOP_Wn.ICO`. Live 256-byte blocks match the corresponding W1-W5 ICO records
exactly. These objects are visual-only and short-lived; no player collision or
enemy AI is on this path.

## Partially decoded families

The normal enemy families all use the common factory `01F7:0E06`, object class
1, and a family-specific callback that writes a logical sprite slot at
`object+0x12`. The callback and post-initializer boundaries are known, and
target-vs-inert ARE mutations prove that the seeded object is removable. The
near-camera callback pass below now establishes first-order direction,
fixed-point movement, and shared helper entry points for every pair. Complete
patrol/pursuit naming, exact collision rectangles, and gameplay death paths
remain intentionally conservative.

The first paired WURM2 run is now live-verified with the shared main-tree
debugger build. A redirected W1L1 anchor produced type `0x01` through dispatch
`01F7:6DA3`, post-initializer `01F7:6DB0`, slot 281, then object callback
`01F7:6DC4`; its first callback sample cleared `object+0x18`. The same anchor
mutated to type `0x02` produced dispatch `01F7:6DB1`, post-initializer
`01F7:6DC3`, slot 231, the same `01F7:6DC4` callback, and the same first-update
clear. Both paired type-0 fixtures reached the inert capture barrier without
allocating an object. These are removal/deactivation observations, not proof of
the gameplay death path; movement and collision remain open.

The paired FISCH run is likewise live-verified against the main-tree debugger
fixtures. Type `0x05` at the W1L1 anchor dispatches through `01F7:7B50`, returns
from its initializer at `01F7:7B5D`, installs slot 254, and reaches per-object
callback `01F7:7B71`; type `0x06` uses dispatch `01F7:7B5E`, post-initializer
`01F7:7B70`, slot 204, and the same `01F7:7B71` callback. The first callback
sample for each variant clears `object+0x18`, while the paired inert fixtures
reach the capture barrier without allocating an object. Existing native W2L1
selector evidence independently resolves FISCH.BOB geometry as 40x19 with
origin 20,19 (records 3 and 2 for the two variants). Movement, collision, and
the true gameplay death path remain open.

The corrected selector-mode tracer then sampled every remaining normal-enemy
pair in a common near-camera W1L2 fixture (the type-`0x34` probe archive,
record `0x1606`, with the target type substituted at declaration time). This
keeps the initializer/callback comparison controlled even though the native
sprite resources belong to other worlds. The observed callback and first-order
motion are:

| family | live callback | 32-tick observation | state/counter evidence |
| --- | --- | --- | --- |
| WURM2 `01/02` | `6DC4` | left `240->226` / right `240->253` | state 0; `+20` countdown and `+2A` phase advance |
| BIENE `03/04` | `68C0` | left `240->198` / right `240->282` | state 0; `+20` counts 14 to 0 and `+2A` advances |
| FISCH `05/06` | `7B71` | left/right with y `384->387` | states 1, 2, 3 observed; phase-driven vertical component |
| KRABBE `07/08` | `778C` | left `240->237` / right `240->242` | state 0; countdown/phase bytes advance |
| PENGO `09/0A` | `715E` | no x displacement; y `400->401` | animation/state cursor at `+32` advances in `0x100` steps |
| SCHNEE `0B/0C` | `66E1 -> 6757` | left/right with y `400->420` | state 0; shared callback changes after the initial path |
| FLIEGE `15/16` | `7EF8` | left `240->194` / right `240->286` | state 0; countdown/phase bytes advance |
| SPINNE `17/18` | `8472` | left `240->230` / right `240->249` | state 0; countdown/phase bytes advance |
| BUGGY `19/1A` | `5071` | left `240->226` / right `240->253` | state 0; countdown/phase bytes advance |
| UFO enemy `1B/1C` | `5F28` | left `240->198` / right `240->282` | state 0; countdown/phase bytes advance |

All ten callbacks remain active for the 32 samples, so these are movement
observations rather than off-camera deactivation artifacts. The paired
type-`0` controls allocate no object. They establish initial direction,
fixed-point integration, and the timer/state bytes, but do not by themselves
prove pursuit, player damage, or the gameplay death path. Type `0x08` is a
known resource-context caveat: the synthetic W1L2 run observed logical slot
200, while the native W2 selector/resource run resolves the catalog companion
as slot 250; both use the same callback and KRABBE geometry.

Static callback inspection fills in the shared interaction boundary. Each
normal enemy first calls visibility gate `01F7:1DCA`; rejection enters
`01F7:1DEE`, which clears `object+0x18` and the selector's active byte. The
movement callbacks then call `01F7:1B77` for object/player bounds overlap and
`01F7:1C4D`/`01F7:1C6E` for directional MAP collision (descriptor bit
`0x400`), recording a blocking result in object byte `+0x2F`. The BIENE and
FISCH disassemblies additionally show direction-dependent velocity clamps and
phase/timer transitions; the other families now have runtime direction and
counter evidence but still need a complete state-table decode. Rendered BOB
width/height/origin values remain sprite geometry, not proof of the gameplay
collision rectangle.

A controlled type `0x01` callback probe now confirms those shared helper edges
in the live executable. With the target aligned to the player and the active
player bounds forced to `f6 ff d8 ff 0a 00 00 00` (a debugger control), one
update enters `1DCA`, `1B77`, `1C4D`, and `1C6E`, changes object byte `+0x2F`
from `FF` to `1`, and keeps callback `6DC4` active. This establishes a real
MAP/bounds-block result and first movement step; it does not identify the
unmodified player damage rectangle or the gameplay death path. The normalized
ledger is [`entity-normal-enemy-collision-evidence.json`](../entity-normal-enemy-collision-evidence.json).

A native W1L1 run with the object aligned to the live player, without forcing
the player bounds, reaches the same `1DCA`, `1B77`, `1C4D`, and `1C6E` helpers;
the enemy moves one step (`x=128->126`) and sets `+0x2F` from 0 to 1. No
player action/global damage flag changes. This is the strongest unmodified
contact probe currently available, and it still deliberately stops short of a
player-death claim; see [`entity-normal-enemy-natural-contact-evidence.json`](../entity-normal-enemy-natural-contact-evidence.json).

The same type-`0x01` callback also has a live removal ledger: holding the
camera at `(0,0)` places the object at `(880,416)` outside the visibility
rectangle, and the first callback clears `object+0x18` (`6DC4 -> 0000`). This
confirms off-camera deactivation while keeping player-caused death, drops, and
respawn separate. See [`entity-normal-enemy-removal-evidence.json`](../entity-normal-enemy-removal-evidence.json).

The shared rule is independently exercised by BIENE type `0x03`: a native W1L1
probe at `(768,256)` with camera `(0,0)` clears callback `68C0` on its first
update. Together with the static `1DCA -> 1DEE` path in the other normal
callbacks, this confirms off-camera removal across the enemy families; it does
not turn the still-open player-caused death/drop path into a claim. See
[`entity-normal-enemy-family-removal-evidence.json`](../entity-normal-enemy-family-removal-evidence.json).

The strict family audit now has a static ledger for every normal-enemy row, not
just slot identity. [`entity-normal-enemy-static-evidence.json`](../entity-normal-enemy-static-evidence.json)
records the common object field layout, each initializer's velocity/orientation/
phase/timer writes, callback state bytes and transition ranges, MAP/player probe
constants, and animation delay/cursor plus BOB record mapping. This closes the
normal families' initializer, state-machine, movement, collision, and animation
dimensions. The remaining normal-enemy questions were semantic: what the
player-range branch does to gameplay state, and which authored level events
produce drops or respawn.

The semantic contact pass now resolves the first of those questions. Static
tails route WURM2, BIENE, PENGO, FLIEGE, SPINNE, BUGGY, and UFO enemies through
`4AB3 -> 4C5D`, which publishes sound action `DS:612E=13`, while FISCH and KRABBE use
`4BA0 -> 4C5D` with sound action `DS:612E=2`. SCHNEE has no equivalent `DS:8806` contact
tail in `66E1/6757`. A controlled native WURM2 probe reaches `70C9`, switches
the object to `4AB3`, then records the action-13 write and timed `4C5D`
response. None of these contact branches clears the enemy object or creates a
drop. The shared removal routine `1DEE` clears both `object+0x18` and the ARE
record high byte at `FS:[object+0x1A+1]`, so a later camera-region scan can
recreate the object; this is the normal re-stream respawn trigger. See
[`entity-normal-enemy-contact-response-evidence.json`](../entity-normal-enemy-contact-response-evidence.json).

The shared action word is now identified as an ONGAME2.TFX sound selector,
not an inventory, health, or visual-effect field. Segment `01E7:0FCF` reads
`DS:612E` and calls `01E7:3360`; the dispatcher indexes the TFX descriptor
table at `ES:[0x200 + action*8]` using `DS:504E`, then updates an audio voice
record selected through `DS:2F5C`. The source rows are exact matches in
`ONGAME2.TFX`: action 2 is `1502f30003620005` (macro byte `0x62`), action 4 is
`1504f30003680005` (`0x68`), and action 13 is `150df300036a0005` (`0x6A`).
The module has no human-readable sample names in these rows. See
[`entity-action-dispatch-evidence.json`](../entity-action-dispatch-evidence.json).

The falling-leaf family is now decoded through its child callback. `01F7:474D`
selects one of two PRNG tables (`DS:3312` delay 8 and `DS:3326` delay 10),
seeds `object+0x0E` with `0x13000-(signed_random_byte<<7)`, copies the source
position to `+0x2A/+0x2E`, and starts `+0x32=0x000C`. Child callback `47E7`
adds the fixed-point velocity to y, pauses and toggles the sprite high bit on a
MAP block, and restores/reseeds the source position when the timer expires. Its
visibility-false path reaches shared removal `1DEE`.

The 256-sample lifetime ledger closes the runtime side of that gap. After the initial pool
offsets `120, 240, 360, 480, 720, 840, 960, 1080`, later emissions reuse
`360, 480, 720, 840, 960, 1080`; animation delay counts down through zero and
the cursor advances through slots 700-704, while the alternate table emits slot
703. Static factory `01F7:0E06` scans 64 entries from `DS:755E` at stride
`DS:30CE` and selects the first entry whose `object+0x18` callback is zero.
This resolves the exact free predicate as well as direct pooled reuse and
animation rollover. The archive contains leaf declarations only in W1L1, so
the cadence claim is explicitly W1L1-scoped rather than an unobserved
cross-world comparison. The child callback has no persistent player/entity collision branch; BLATT geometry
is render-only, so collision is not applicable to this family. See
[`entity-leaf-pool-evidence.json`](../entity-leaf-pool-evidence.json) and
[`entity-leaf-state-evidence.json`](../entity-leaf-state-evidence.json).

The remaining effect-family uncertainty is no longer object identity: all of
these callbacks and resource bindings are now bounded below. The open parts
are exact gameplay names for global effects, collision response details, and
level persistence/respawn semantics. The native platform pass now confirms the
accepted carry band and its player-offset consumers; a paired native W4L1
probe also resolves horizontal approach polarity at fixed vertical offset.

The shared effect/collectible callbacks are now statically bounded as well.
Cloud `0x28` initializes callback `9269`, leaves the logical slot at `FFFF`,
does not integrate position, and only performs the camera/player-range test
that updates `DS:89E6`. Paper `0x2C` initializes slot 710 and callback `8D20`
with subtype 5. The same `8D20` callback is used by pickups and puzzle letters:
it checks an x range extended by 16 pixels and a 16-pixel-aligned y range,
then dispatches `+0x2C` (pickup subtype) or ORs the letter bit in `+0x2A`
into `DS:60D8` before clearing `object+0x18`. This establishes fixed placement
and overlap geometry. The accepted interaction branches join at `8E42` and do
not call `1DEE`, so they clear the live callback without clearing the ARE
re-stream claim; a collected object is not recreated by the normal off-camera
scan in the same loaded buffer. Level selection is separately observed to
reread the ARE resource. A stateful W1L1 type-0x6F collect-then-reload probe
now reconstructs the same declaration and reinstalls callback `01F7:8D20`, so
pickup reload persistence is resolved as reset/reconstruction behavior in the
tested fixture. A final-letter W1L1 probe then forced `DS:60D8=0x3F`, collected
type `0x7F`, reached `DS:60D8=0x7F`, emitted action `11`, and cleared the letter;
after 1,800 gameplay frames the mask and W1L1 resource span were unchanged and
no level transition occurred. Static disassembly of selector `01D7` now closes
the authored path: `01D7:1670` compares `DS:60D8` with `0x7F`,
`01D7:16C6-1704` renders `NESQUIK: 2000` and `BONUS-LEVEL!`, adds 2000 points,
and sets `DS:85DB=1`; the caller consumes that state at `01D7:4F10-4FAF` and
enters reload/transition setup at `01D7:5017`. The synthetic run simply did
not enter that presentation branch. See [`entity-collectible-persistence-evidence.json`](../entity-collectible-persistence-evidence.json)
and [`entity-puzzle-completion-evidence.json`](../entity-puzzle-completion-evidence.json).

The three adjacent pickup subtypes now have controlled native overlap ledgers.
Type `0x70` adds `250` to `DS:881C` and raises `DS:8822/DS:8824` together;
type `0x71` adds `100`; type `0x72` adds `150` and conditionally calls
`01F7:1A97` when `DS:85D4 <= 15`. They emit sound actions `9`, `10`, and `12`
respectively, then clear the live callback. See
[`entity-pickup-subtype-overlap-evidence.json`](../entity-pickup-subtype-overlap-evidence.json).

Wind `0x33` and UFO effects `0x35/0x36` are moving callback families rather
than inert visuals. Their initializers load animation tables, seed signed
fixed-point horizontal velocity, and converge on callbacks `882F` and `546D`;
both call `1DCA -> 1DEE`, `1B77`, `1C4D/5C27`, clamp velocity, and flip
direction on timer or map blocks. Bump `0x34` is the contrasting stationary
hazard: `9BEE` shifts the position and installs `9C0C`, whose player-range
branch emits sound action `DS:612E=4`; its off-camera path also uses
`1DCA -> 1DEE`. Moving platforms `0x3D`-`0x40` converge on `9DC7`, integrate
horizontal or vertical fixed-point velocity, snap to 16-pixel grid cells on
MAP blocks, and carry player-relative offsets through `A0B2`; their offscreen
branch reaches `1DEE`. These static facts close initializer, movement, and
deactivation boundaries while preserving the unresolved gameplay response
names in the matrix.

Current-build 64-sample direct traces add a bounded trajectory observation:
wind `0x33` moves horizontally from x `240` to `225` at constant y `544`, while
UFO effects `0x35` and `0x36` share callback `546D` and move from x `240` to
`157` and `322` respectively, also at constant y `544`. All callbacks remain
active in these near-camera runs. The runs establish signed horizontal motion
and opposite paired directions, but not timer reversal, MAP blocking, or
player-contact response; see [`entity-effect-motion-evidence.json`](../entity-effect-motion-evidence.json).

The callback state paths are now decoded in the segment as well. Wind `882F`
uses states 0-3: horizontal travel with directional MAP checks and a 0x51-tick
phase, bounded transition/vertical phases, and a return-to-state-0 clamp. Its
player-range band is `x-10..x+10`, `y-35..y+5`; the callback scans `DS:8806`
and clears a matching entry but emits no direct `DS:612E` sound action. UFO
effect `546D` uses states 0-6, direction-dependent `+/-20`/`+/-16` MAP probes,
the player-range transition, return/acceleration phases, and the camera-gated
clear at `58A0`; common setup seeds `+/-0x15000` velocity and the paired types
negate it. Its `DS:8806` scan either selects `4AB3` on a second match or writes
sound action `0x0D`. The normalized motion ledger now carries these static state
rules as well as the direct trajectories; the remaining questions are the
semantic meaning of the table entries and authored reset behavior.

## Targeted static pass

The targeted raw-segment decompile covers the cloud render handoff and the
remaining effect-family branches. In the cloud path, `01D7:4EA0 -> 4EAA`
consumes `DS:89E6`, performs its outer gameplay/effect calls, and reaches
`01F7:106A`; the normal object update/render queues at `0FDC` and `1024`
explicitly skip objects whose logical slot is `0xFFFF`. The ordinary queue
append/draw path is `34BC -> 3587 -> 0013`, where `0013` is a generic VGA/BOB
blitter. A 256-sample main-tree hardware trace hit `0013` only from return
site `35B8`, with identical ordinary queue parameters (logical slot 0,
position 128,400), and never observed a WOLKE slot 413-416 call. Thus the
generic primitive is identified, but the WOLKE-specific caller or direct record
selection after the outer consumer remains open; archive and cross-world
resource evidence still establish slots 413-416.

The same pass closes the branch-level behavior for the remaining effects:

- Wind `87D1 -> 882F` seeds `-0xB000` velocity and traverses states 0-3;
  it clears matching entries in the shared `DS:8806`/`DS:87DE` effect table
  and removes off camera through the shared `1DCA -> 1DEE` path.
- UFO effect `53F7 -> 546D` seeds `-0x15000` (negated for type `0x36`) and
  traverses states 0-6; its directional scan of the same table leads to `4AB3`
  on a second match or sound action `DS:612E=0x0D`, then it clears off camera
  at `58A0`. The shared table lifecycle is now recovered: `44FF` resets the
  table, `4519` allocates/spawns an entry, `45AB` updates one x/y row, and
  `470C` removes it; `DS:8806` is the active-entry count, `DS:8808` the
  capacity bound, and `DS:880C` the pending-effect count consumed by
  `01D7:14E1`.
- Paper `8C4E -> 8D20` shares the pickup overlap dispatcher: subtype 5 adds
  500 to `DS:881C`, emits `DS:612E=0x0C`, increments bounded `DS:880A` (only
  while below 9), and clears the live callback. `01D7:1084` initializes the
  counter from `DS:88B0+3`, while `01F7:5936` clamps and renders changed values
  and `01F7:5BEE` resets its display cache. The remaining uncertainty is the
  counter's human-facing label and authored reload persistence. A main-tree
  W1L1 trace captured `8C4E -> 8D20` with `DS:880A=4`; the controlled fixture
  did not enter the overlap effect branch, so the increment itself remains
  supported by the existing W1L2 overlap ledger plus the static consumer pass.
- Dedicated events `178D/1798/17A3 -> 1749` seed subtype bytes `0x00/0x08/0x10`;
  child callback `10B5` decrements its lifetime and clears at zero, while
  `1186 -> 11B4` renders world-specific ICO data without collision.

The durable ledger for this pass is
[`entity-targeted-decompile-evidence.json`](../entity-targeted-decompile-evidence.json).

The platform carry branch is also live-observed. Eight direct updates of type
`0x3D` in the synthetic W1L2 cell enter `A075` and `A0B2` every time; the
position remains `(240,512)` because this cell is MAP-blocked/waiting, while
phase/timer fields advance and the callback stays active. This confirms that
the original callback reaches its player-carry path without pretending the
blocked fixture is a free-running trajectory. The normalized result is
[`entity-platform-carry-evidence.json`](../entity-platform-carry-evidence.json).

The platform motion path is now live-observed in the native W4L1 fixture using
only debugger controls recorded with the trace. Clearing the carry latch and
injecting `0x28000` fixed-point x velocity produces 128 active `9DC7` updates
from `(752,336)` to `(1072,336)` while `A075` and `A0B2` remain reachable. A
second run with injected `0x100000` velocity reaches the native MAP stop at
sample 85: x snaps at `2112`, `object+0x0A` is zeroed, and `object+0x54` starts
its `0x46` wait phase. Static branches at `9F35/9F4A` and `A03D/A051` select
the signed `0x28000` directions. These traces prove integration and blocked
state mechanics; the separate native overlap ledger and static control-flow
decode below resolve carry acceptance and the absence of a platform-specific
terminal table. See
[`entity-platform-motion-evidence.json`](../entity-platform-motion-evidence.json).
The remaining W5 resource question is now closed by a native W5L1 selector
trace that requests `GAMEDATA\\platfw5.BOB`; the normalized resource ledger is
[`entity-platform-cross-world-evidence.json`](../entity-platform-cross-world-evidence.json).
The platform animation dimension is intentionally not applicable: the four
initializers choose static `PLATFW` records (slots 300/301), and callback
`9DC7` never advances an object animation cursor. A native W4L1 overlap probe
with the object placed eight pixels above and left of the live player reaches
`A075 -> A0B2` without forcing platform-ready state or player bounds. The
accepted branch writes `DS:5006=-1`, computes `DS:8816`/`DS:8812` carry offsets,
and changes object `+0x59/+0x5A` into its carry phase; player callbacks
`3A8A/3B44` and `3FF8` consume those offsets. The normalized result is
[`entity-platform-player-carry-evidence.json`](../entity-platform-player-carry-evidence.json).
Static `9DC7` control flow contains no separate authored terminal table:
`+0x52/+0x54` are wait counters and `9F35/9F4A`, `A03D/A051` reverse only
after MAP tests. Off-camera `A06F -> 1DEE` resets the ARE record claim for
re-stream respawn. The accepted/rejected native pair keeps the vertical offset
constant and moves the platform from x=-8 to x=+40 relative to the player:
`DS:5006` changes from `0 -> FFFF` to remaining `0`, directly resolving the
horizontal approach polarity.

BUMP `0x34` is now bounded across all requested dimensions. Its callback uses
the open player gate `object_x-25 < player_x < object_x+25` and
`object_y-8 < player_y < object_y`; the accepted branch emits sound action
`DS:612E=4`.
A 128-sample native run cycles slots `400,402,403,401` with a seven-tick
`object+0x20` counter, and the five `BUMP_Wn.BOB` tables are cataloged with
exact geometry. A camera-(0,0) run takes `1DCA -> 1DEE` and clears the callback.
A controlled native overlap with `DS:89EA=0`, player `+0x37=1`, and the object
four pixels below the player reaches every helper and the live `9C64` action
write. Eight qualifying samples repeat sound action 4 while callback `9C0C`
remains active, so there is no BUMP contact reset/removal state. See
[`entity-action-dispatch-evidence.json`](../entity-action-dispatch-evidence.json)
and [`entity-bump-evidence.json`](../entity-bump-evidence.json).

## Cross-world rules

World-specific resource selection is a separate axis from logical sprite slot:
the same slot can resolve to different BOB records depending on the resource
batch. Confirmed examples include slot 214 (`WIND.BOB` in W4 versus `UFO.BOB`
in W5), slot 200/250 across enemy families, and slot 400 for the
`BUMP_W1.BOB`-through-`BUMP_W5.BOB` family. The matrix therefore records both
the logical slot and the resource context. A cross-world claim is complete only
when the live descriptor geometry or exact ICO block has been matched in that
world; filename ordering alone remains partial evidence.

Cloud `0x28` is the cross-world exception to ordinary slot lookup. ARE parsing
finds it in W1, W2, W3, W4, and W5 contexts, while every archive contributes
the same global `WOLKE.BOB` table (slots 413-416, 32x16, origin 0,0); no
`WOLKE_Wn.BOB` files occur. The initializer keeps logical slot `FFFF`, which
matches the special renderer rather than a missing sprite. Native callback
`9269` is camera-gated, performs a 16x16 aligned player-bounds test, and writes
`DS:89E6=FFFF` on the accepted branch. A controlled positive probe points
`DS:881A` at the cloud object, sets `DS:89EA=0`, supplies a 16x16 bounds
rectangle, and confirms `DS:89E6: 0 -> FFFF` while callback `9269` remains
active and stationary; the static `+0x37 == 0` test is therefore resolved. The
standard renderer `01F7:3529` returns immediately for logical slot `FFFF`, and
`9269` has no direct draw/resource call. One-shot native probes hit both
player-side readers `01F7:4087` and `01F7:4406` with `DS:89E6=FFFF`; a separate
controlled run hits the outer main-loop consumer `01D7:4EA0` repeatedly with
that flag set. The exact low-level WOLKE.BOB pixel primitive is still outside
the normal logical-slot queue. See
[`entity-cloud-crossworld-evidence.json`](../entity-cloud-crossworld-evidence.json).

Raw-segment relocation scanning narrows that renderer gap: the only direct
reference to `01F7:0013` is the ordinary queue call at `01F7:35B3`, returning to
`35B8`. No additional WOLKE-specific caller of the generic blitter was found;
the remaining branch must either select a WOLKE descriptor through another
entry or render through the outer cloud consumer without calling `0013`.

A targeted queue-boundary trace now tests that distinction directly. Across 64
breakpoint samples, every hit was `01F7:34BC` with return site `01F7:1029`
(the ordinary `01F7:1024` object renderer), and every incoming record was
`x=128, y=400, logical_slot=0, flags=0`; `DS:89E6` remained `FFFF`. No
explicit WOLKE queue entry was observed. The auxiliary outer-branch targets
`0207:08D8` (mode-13h/framebuffer setup), `01F7:F07B` (DOS INT 21h vector-08h
installation), and `01F7:106A` (object update/list pass) likewise do not select
a WOLKE descriptor. The trace is recorded in
[`entity-targeted-decompile-evidence.json`](../entity-targeted-decompile-evidence.json)
with its output and script hashes; the remaining work is to find the alternate
descriptor/resource path or a fixture that reaches it.

## Reproducible next experiments

The isolated object tracer now lives in
[`object_behavior_trace.py`](../tools/object_behavior_trace.py) and
[`quiky_object_behavior_trace.lua`](../automation/quiky_object_behavior_trace.lua).
It captures the object bytes before and after each live per-object callback,
the callback return site, changed offsets, camera globals, and the initial and
post-initializer object records. `--sprite-init-offset` identifies the
post-initializer boundary; this direct callback mode avoids obsolete scheduler
entry offsets and handles one-shot normal objects.

Run a representative trace from the repository root after starting the
debug-enabled automation build:

```text
python3 research/tools/object_behavior_trace.py \
  --launch --headless --entity-type 0x01 \
  --record-offset 0x1792 --samples 32 \
  --output research/build/traces/entity-01-behavior.json
```

When this worktree has no local automation build, the compatible binary can be
shared from the sibling main worktree. Set all three paths so the launcher
uses the main build directory and dependency prefix:

```sh
DOSBOX_AUTOMATION_BIN=/home/joao/dev/quicky/research/build/dosbox-automation-debug/dosbox_with_debugger \
DOSBOX_AUTOMATION_BUILD_DIR=/home/joao/dev/quicky/research/build \
DOSBOX_AUTOMATION_DEPS_PREFIX=/home/joao/dev/quicky/research/build/sdl-prefix \
python3 research/tools/quikytrace.py --launch --headless \
  --runtime-dir /tmp/entity-01-variant/baseline/game \
  --select-level W1L1 --entity-record-offset 0x177a --entity-type 0x01 \
  --sprite-init-offset 0x6db0 --output /tmp/entity-01-generic-init.json
```

This invocation was verified on the current executable/archive pair. It
captured type `0x01` at world position `(816,240)`, dispatch callback
`01F7:6DA3`, common factory `01F7:0E06`, post-initializer slot `281`, and the
live descriptor geometry `52x16` with origin `(26,16)`. The selector path is
important: the default menu replay can time out before the first resource
lookup even though the shared debugger binary is healthy.

The stateful pickup reload probe uses the same tracer with the main-tree
debugger build and an isolated W1L1 fixture:

```sh
DOSBOX_AUTOMATION_BIN=/home/joao/dev/quicky/research/build/dosbox-automation-debug/dosbox_with_debugger \
DOSBOX_AUTOMATION_BUILD_DIR=/home/joao/dev/quicky/research/build \
DOSBOX_AUTOMATION_DEPS_PREFIX=/home/joao/dev/quicky/research/build/sdl-prefix \
python3 research/tools/object_behavior_trace.py --launch --headless \
  --runtime-dir /home/joao/dev/quicky/research/build/entity-6f/baseline/game \
  --entity-type 0x6f --record-offset 0x1838 --samples 6 \
  --select-level W1L1 --align-object-to-player --align-y-offset -32 \
  --force-active-player-bounds --trace-overlap --reload-after-collect \
  --reload-level W1L1 --output research/build/entity-6f-reload-after-collect.json
```

The first callback sequence is `01F7:8BC2 -> 01F7:8D20`; the controlled
overlap reaches `01F7:8E42` and leaves object+0x18 zero. The second selector
load sees the same `6f0020003000` declaration, runs the factory and `8BC2`
initializer again, and reinstalls `8D20` on the new live object. This closes
pickup reload persistence for the tested fixture as reset/reconstruction
behavior; it does not claim cross-session save semantics.

The final-letter completion probe uses the synthetic W1L1 type-`0x7F` fixture,
stops after initializer `01F7:8D13`, and seeds the other six bits:

```sh
DOSBOX_AUTOMATION_BIN=/home/joao/dev/quicky/research/build/dosbox-automation-debug/dosbox_with_debugger \
DOSBOX_AUTOMATION_BUILD_DIR=/home/joao/dev/quicky/research/build \
DOSBOX_AUTOMATION_DEPS_PREFIX=/home/joao/dev/quicky/research/build/sdl-prefix \
python3 research/tools/object_behavior_trace.py --launch --headless \
  --runtime-dir /home/joao/dev/quicky/research/build/entity-7f-native/baseline/game \
  --entity-type 0x7f --record-offset 0x1792 --samples 1 \
  --select-level W1L1 --sprite-init-offset 0x8d13 \
  --align-object-to-player --align-y-offset -32 --force-active-player-bounds \
  --trace-overlap --force-tile-mask 0x3f --trace-puzzle-completion \
  --puzzle-probe-frames 1800 --output research/build/entity-7f-puzzle-completion-1800.json
```

This reaches `0x7F`, action `11`, and callback clear, but remains in gameplay
with no resource reload or level transition. The direct `DS:60D8` consumer at
`01F7:5940` updates presentation state; see
[`entity-puzzle-completion-evidence.json`](../entity-puzzle-completion-evidence.json).

For a level-specific representative, add `--select-level W2L1` and use the
record offset from `quikyctl.py entity-catalog`. Use paired traces for each
family variant, then run a one-record target-vs-inert mutation with
`research/tools/quikyentity.py` to separate object removal from a visual
change. Keep the generated traces under ignored `research/build/`; only
normalized conclusions and hashes belong in tracked notes/catalogs.

The broad family pass is complete. The targeted probes now close the shared
collectible overlap and pickup reload, pooled-leaf reuse, normal-enemy helper,
and platform-carry boundaries. Remaining experiments are deliberately narrower:

1. Capture the fully authored all-seven-letter run if exact completion timing is
   needed. The static comparator and transition handoff are now identified, but
   the synthetic fixture did not execute the presentation branch.
2. If pixel-level renderer provenance is required, continue from the narrowed
   WOLKE boundary: identify the alternate descriptor/resource path (or a
   fixture that reaches it) after the outer `01D7:4EA0` state consumer. The
   generic `0013` VGA/BOB primitive, ordinary queue path, player-state gate,
   cross-world usage, removal path, and both player-side readers are confirmed;
   the 64-sample queue probe ruled out an explicit ordinary-queue injection.
3. If effect semantics are required beyond control flow, correlate authored
   producers/reset paths for the now-decoded `DS:8806` table, and assign the
   paper `DS:880A` bounded HUD counter its human-facing label.

Each new trace should update the corresponding seven dimension statuses in the
JSON matrix and add a durable reference to this note. Do not promote a
dimension to `confirmed` solely because a callback or sprite slot is known.
