# Quiky C++ engine core

This is the first standalone C++ slice of the Quiky engine recreation. It is
kept independent of SDL and ScummVM so the format and renderer code can be
tested independently before a ScummVM frontend is added.

The current iteration supports:

- checked binary reads with explicit endianness;
- direct reading of the `NESTLE.DAT` archive;
- `MAP` decoding;
- world `PCC` palette extraction;
- column-interleaved `ICO` tile decoding;
- indexed level rendering to an 8-bit BMP;
- ARE layout/entity parsing and optional debug overlays;
- BOB sprite decoding and indexed contact sheets;
- a deterministic fixed-step player simulation centered on the recovered
  `0x78`-byte record;
- recovered world-specific descriptor tables linked to streamed MAP cells;
- an explicit MAP-cell-to-descriptor query bridge using confirmed quadrant rules;
- an SDL3 interactive W1L1 frontend with fixed-step input and camera scrolling;
- a shared SDL audio mixer that combines gameplay music with confirmed pickup SFX;
- the recovered `01F7:4416–44FE` transition branch, including its ordered
  descriptor probes, live-EAX Y correction, `DS:89EA` decrement, terminal
  `DS:89EC` write, and zero `01F7:20AF` view publication;
- ARE-backed level streaming with the scoped W1L1 object-family inventory;
  the shared collectible callback `01F7:8D20`, recovered normal-enemy family
  callbacks, and animated world-ICO callback `01F7:8E4B` are now published
  through the scheduler, while unrelated outer transition contracts remain
  explicit at their recovered boundaries;

The default W1L1 session now starts the player at the native traced
initializer position `(128,400)`. Explicit start-coordinate overrides remain
available for fixtures and controlled experiments; levels without a closed
player declaration retain the conservative fallback.
- source-less high-effect rendering from the recovered `4B70 -> 4C74` chain:
  world-specific `PUFF.BOB`/`PUFFW2.BOB`, slots `611/612/613`, and the
  31-update terminal lifecycle;
- DOSBox-equivalent runtime VGA palette output while preserving raw PCC
  values for asset inspection;
- a deterministic 320x200 `quiky-frame` scene probe for matching DOSBox
  screenshots with explicit camera, input, and high-effect controls;
- synthetic unit tests for the format readers.

## Player foundation boundary

The deterministic shell is now the canonical runtime boundary. Its mutable
ownership is explicit:
`SimulationState` owns the tick counter, recovered player record, scheduler,
object-pool records, and queued events. `WorldCollisionView` borrows decoded
MAP and descriptor data read-only. `Simulation::tick(input, world, output)` is
the one fixed-step boundary; it advances scheduling and drains events into a
`SimulationOutput` snapshot. Rendering reads that snapshot, and audio reads
its emitted audio events instead of driving gameplay.

The foundation currently includes:

- `Fixed16`, the centralized signed 16.16 representation with explicit
  arithmetic shifts, wrapping, clamping, conversion, and multiplication;
- `PlayerRecord`, a typed projection of the recovered `0x78`-byte record, with
  stable `fieldXX` names and lossless raw-record conversion;
- `WorldCollisionView`, which preserves raw MAP words, tile IDs, cell flags,
  descriptor words, runtime flags, and out-of-bounds status. It exposes only
  the confirmed descriptor predicates, not a guessed `isSolid()` rule;
- `ObjectScheduler`, which applies deferred spawn/release mutations, walks
  stable slot order, and records callback identity and invocation order;
- canonical recorded-run parity, with strict state JSONL and explicit exact
  or lifecycle comparison profiles.

The active frontend and scene tools consume `SimulationOutput` and
`PlayerRecord`; they no longer have a second player-state or collision API.
`player_update.h` keeps the recovered callback stages and unresolved external
boundaries explicit. The runtime does not silently fall back to guessed
grounding or jump behavior.

Research closure ownership is indexed in
[`research/ghidra/closure-index.json`](../research/ghidra/closure-index.json).
Each behavioral area has one canonical ledger and verifier; files under
`research/build/` are generated evidence only. New static expansion should be
triggered by a parity mismatch or a named simulation edge reaching an
unclassified address.

## W1L1 scheduler status

The first gameplay-object slice is the recovered shared collectible callback
`01F7:8D20`. ARE records for ammo, health, invulnerability, lives, and puzzle
letters publish their address-qualified callback identity through
`ObjectScheduler` in stable slot order. On overlap, the level session records
the exact subtype-driven player/global writes (`DS:880A`, `DS:880C`,
`DS:8810`, `DS:881C`, `DS:8822`, `DS:8824`, `DS:60D8`, and `DS:612E`) in the
collected event and clears the scheduled object. The invulnerability path also
writes the recovered player-record timer at `+0x34`; subsequent countdown and
gate clearing remain owned by the player callback.

This slice is statically closed for callback identity, overlap geometry, and
state effects and is covered by native contract tests. The native
`collectibleOverlaps` predicate now mirrors `8D31 -> 393C`: it gates on
`DS:89EA`, uses the signed player bounds at `+0x2C/+0x30/+0x2E/+0x32`, aligns
the object Y word down to 16 pixels, and preserves the original strict edge
comparisons. The tests include both accepted subtype writes and held-out
equality-boundary cases. Natural puzzle completion/transition behavior and
script-created platforms are not implemented by this slice.

The corresponding focused static contracts are consolidated in
[`w1l1-object-callbacks-static-decomp.cpp`](../research/notes/w1l1-object-callbacks-static-decomp.cpp).
That note and the external-state ledger now cover every actual W1L1 leaf,
collectible, puzzle-letter, and cloud callback body. Opaque visibility,
descriptor/map backends, sound/effect dispatch, and resource-selected targets
remain address-named; they are expanded only if a parity mismatch reaches them.

The non-W1 normal-enemy callback boundary is now exported through the same
Ghidra process. [`normal-enemy-static-decomp.cpp`](../research/notes/normal-enemy-static-decomp.cpp)
and [`normal-enemy-static-closure.json`](../research/ghidra/normal-enemy-static-closure.json)
cover PENGO, KRABBE, FISCH, SCHNEE, FLIEGE, SPINNE, BUGGY, and UFO: exact
initializer writes, callback state fields, MAP/player probe contracts, scheduler
position, visibility removal, and contact-tail selection. Independent A/B
decompilation and instruction listings are retained under
`research/build/player-external-state-closure-normal-enemy-v1/`. This closes
the static evidence boundary; native integration and authored contact timing
remain separate work.

The pooled boss/end-stage boundary is now exported through the same bounded
Ghidra process. [`boss-static-decomp.cpp`](../research/notes/boss-static-decomp.cpp)
and [`boss-static-closure.json`](../research/ghidra/boss-static-closure.json)
cover the five world-specific constructors, damage-row consumers, stage
callbacks, child/effect edges, scheduler order, and completion handoff. The
ledger records the exact pre-increment hit thresholds and `DS:88AE` stage
writes; it also records that these callbacks do not directly write the player
record. Repeated authored damage timing, completion production, and
resource-selected presentation leaves remain explicit unresolved boundaries.
Independent A/B exports are retained under
`research/build/boss-static-closure-v1/`.

## Focused 5010 reload boundary

The recovered high-level reload orchestration is now implemented by
`LevelRuntime::reload`. It records the auditable stage sequence

```text
5010 -> 0908/0931 -> 18C7 -> 0D5A -> 1AAA -> 5D38
     -> 321F -> 313D -> 504F
```

The call releases the old simulation scheduler, loads the target MAP/ARE and
world resources, reconstructs the target player/session, and republishes its
streamed scheduler entries. `LevelReloadTrace` preserves the address-qualified
stage order for parity tests. The real-archive test exercises the W1L1 to W1L4
route observed in the native v23 trace. Presentation-only fade and renderer
helpers remain outside this API; natural selector routing and authored spawn
table selection remain explicit data boundaries.

The native normal-enemy bridge now publishes all ten statically recovered
families (WURM2, BIENE, FISCH, KRABBE, PENGO, SCHNEE, FLIEGE, SPINNE, BUGGY,
and UFO): exact callback identities, initializer Y offsets, signed fixed-point
direction seeds, family animation delays, descriptor/raw MAP probe routing, and
the `4AB3` versus `4BA0` contact-response identity. Their primary callbacks
are dispatched before the phase-2 player callback, matching the recovered
`0E06 -> 1036 -> 0E96` phase-1/phase-2 ordering. The admitted native scope is
tracked in [`normal-enemy-native-bridge.cpp`](../research/notes/normal-enemy-native-bridge.cpp);
the static ordering record is [`w1l1-primary-callback-scheduler-order-static-v1.json`](../research/evidence/player-dos-parity/w1l1-primary-callback-scheduler-order-static-v1.json).
The WURM2/BIENE paths retain their existing state-machine coverage and now
execute the statically closed pre-update player-contact route
`01F7:1B77 -> 393C -> 19E6`: exact signed rectangle tests, invulnerability and
timer gates, damage/lives writes, knockback, terminal mode fields, and the
recovered transition globals are published by `LevelSession`. The opaque
`19FB` effect/sound dispatch and delayed death/recovery consumer remain
explicit boundaries. The additional families currently execute only the statically closed portions of
their state-0 paths. FISCH's table-driven vertical phase and the other
family-specific runtime-table/effect phases remain address-named pending native table
and callback traces. No renderer sequence cursor is published for these
families until its loader contract is closed.
The BIENE callback's complete static state machine is now separately exported
at [`biene-static-decomp.cpp`](../research/notes/biene-static-decomp.cpp):
states `0..8`, runtime-table transition motion, exact `1B77`/`1C4D`/`1BD1`
probes, animation loads, and the shared `4AB3` contact tail. Native execution
of those nonzero BIENE states now accepts an explicit replay-time copy of the
startup-generated `DS:7974` table and is covered through the state-1 fixed-point
phase contract. The default runtime keeps that path disabled until the
DOS-time/software-float table builder is reproduced. `1C4D` remains the exact
raw-MAP `0x4000` probe, while the `DS:81C4` range-gate source remains explicit
until the camera publication is wired to this callback. Player contact records
the timed `4AB3/4C5D` response, publishes the recovered action word, and keeps
the three-child effect allocation as a traceable external boundary; it does not
invoke the provisional player-death path.

The state-zero BIENE branch is now polarity-locked to the Ghidra listing:
raw `+0x2F <= 0` follows `69DD` and integrates the existing horizontal
velocity, while positive `+0x2F` follows `6902` and applies the patrol/contact
response. The native regression drives both cases with the raw MAP `0x4000`
probe. This closes only state zero; nonzero states still require an injected
`DS:7974` table and remain an explicit runtime-data boundary.

The W1 animated-tile slice executes the shared `01F7:8E4B` closure for ARE
types `0x1F`-`0x21`. It preserves the initializer selector at `object+0x2E`,
advances the callback state at `object+0x32`, performs the exact five-cell
MAP/`DS:6986` probe order at states `4/6/8/10`, and creates three-tick ICO
children through the recovered `16CE -> 10B5` contract. State 10 clears the
parent and writes row zero of the 32-entry `DS:8828..DS:88A7` spawn table;
the indexed `01F7:1AAA` transition consumer is now modeled for the observed
W1L4 row `(288,144)`, while other resource-selected rows remain data
boundaries.

The remaining W1L1 authored visual callbacks now publish their closed static
contracts without being folded into generic gameplay behavior:

- Cloud `01F7:9256 -> 9269` is classified as an ambient visual, keeps its
  normal logical slot at `0xFFFF`, and latches `DS:89E6` only when the
  normalized 16-pixel player gate, `DS:89EA == 0`, and player `+0x37 == 0`
  all pass. The outer transition consumer and special `WOLKE.BOB` blitter are
  still address-qualified.
- Leaf types `0x29`-`0x2B` publish the `01F7:47E7` callback, `BLATT.BOB`
  slot families, the `0x13000` fixed-point base velocity, source position,
  timer `0x0C`, and the confirmed `DS:3312`/`DS:3326` animation-table
  selectors. The signed PRNG perturbation and pooled-child emission cadence
  remain unresolved rather than being replaced by guessed motion.
- Dedicated types `0x65`-`0x67` publish subtype bytes `0x00/0x08/0x10`,
  world-relative `LOOP_Wn.ICO` records, no normal BOB slot, and child callback
  `01F7:10B5`. The event-ring seed, exact child selector/lifetime, and human
  semantic names remain explicit external data.

The current-main native goal fixture exercises all seven authored W1L1 letter
callbacks and reaches `DS:60D8=0x007F` through their normal updates. A
separate adjacent-cell fixture also reaches the accepted cloud callback at
`01F7:92A9` and publishes `DS:89E6=0xFFFF`; the cloud writer is observed
before the native letter mask becomes complete. The immediate outer path then
reaches `01D7:4EA0/4EAA` with the complete mask and cloud latch. These are
dynamic validations of letter accumulation and the outer gate. Static Ghidra
closure now also proves the post-`4EAA` `DS:5044`-selected timer wait,
`01D7:14E1` completion consumer, `DS:85DB` selector handoff, and `DS:89E0`
reload gate. The native cloud fixture reaches `0207:0002` with the complete
mask and sentinel, and the extended post-input run reaches `01D7:14E1`, the
authored `01D7:01F0/01AC/01BD/01D1/01D6` input path, `01E7:0CAA`,
`0207:022A`, `01D7:5010`, `01D7:504F`, and the first post-cleanup renderer at
`01F7:35C7`. The trace records actual debugger hit addresses and generations;
the repeated `01D7:01D6` hit is retained as observed. Exact object/resource
side effects produced by helpers beyond the reload gate remain explicit
runtime boundaries rather than guessed behavior.

The reproducible v52 Ghidra export also closes the progression accounting
body itself: `01D7:14E1` is a near function ending at `1733`, `16C6` is an
internal all-seven branch, and the static contract records the exact
`DS:880C`, `DS:881C:DS:881E`, `DS:612E`, `DS:85DB`, selector, and `5010`
handoff writes. The remaining boundary is the runtime-selected resource and
presentation behavior behind those calls, not the completion arithmetic or
branch order. See
[`player-progression-static-v1.json`](../research/evidence/player-dos-parity/player-progression-static-v1.json)
and [`progression-static-decomp.cpp`](../research/notes/progression-static-decomp.cpp).

The moving-platform slice now executes the static `01F7:9C70` /
`9CF5/9D19/9D5E/9D82` initializer matrix and the `9DC7` / `A075` / `A0B2`
publication contract for ARE types `0x3D`-`0x40`. It preserves the signed
`+0x4C/+0x4E/+0x50` direction/edge bytes, the `+0x52` and `+0x54` waits, the
initializer `+0x59` gate from raw MAP bit `0x0200`, and the exact `0x0800`
motion stop probes, 16-pixel snaps, reversal increments, and `0x46` blocked
wait. It runs before the player callback, applies the strict horizontal
interval and 12-pixel contact band, publishes `DS:5006`, `DS:8816`, and
`DS:8812`, and consumes the carry in the same callback while preserving the
packed `0x78` record's subpixel state.
`SimulationOutput::playerDependencyOrder` records platform and implemented
phase-1 gameplay-object callbacks before the `01F7:3FF8` player callback; the
ordinary scheduler invocation list remains separate because the player update
is not a pooled object callback in this native boundary.
The confirmed `5DC3` raw-MAP `0x0800` stop/snap path and its `0x46` wait value
are covered by a native contract test. The hash-pinned W4L1 fixture
[`platform-player-current-v1.json`](../research/evidence/player-dos-parity/platform-player-current-v1.json)
uses the archived `QUIKY.EXE`/`NESTLE.DAT` pair and observes
`9DC7 -> A075 -> A0B2 -> 3FF8 -> 0F26` across eight combined samples. It
captures the complete `0x78` player record and proves the initial
`DS:8816=1`, `DS:8812=0xFFF80001` carry is consumed and cleared before the
player callback returns. A second controlled aligned-platform trace holds
native Space+Up input through the first combined handoff and confirms the
same carry ordering during the recovered ascending mode. Retail-geometry
landing, inherited velocity, and crushing remain explicit boundaries. The
hash-pinned controlled horizontal trace
[`platform-player-horizontal-motion-v1.json`](../research/evidence/player-dos-parity/platform-player-horizontal-motion-v1.json)
also observes platform motion `504 -> 506 -> 509 -> 511` under the recovered
fixed-point velocity and confirms that the first `0x00020001` carry is
consumed by the following `3FF8` callback. Later `0x14` recontact cooldown
behavior is recorded but not promoted to a general attachment rule.
The statically confirmed `A06F -> 1DEE` cull/re-stream lifecycle is modeled:
platform carry/wait state is discarded at release and the ARE anchor and
initializer state are restored on re-entry. The effect of that release on an
attached player remains an explicit runtime boundary.
The scheduler entry layout is now recorded exactly: each eight-byte entry
stores the phase callback offset at `+0x00`, the secondary near callback
offset at `+0x02`, the pooled-object offset at `+0x04`, and an unclassified
word at `+0x06`. `0E96` calls the phase word in phase order; `0FA2` calls the
secondary word with near `CALL AX`; `0FDC` tests the secondary word and then
passes visible object position/flags/mode through its separate far import.
The runtime-selected callback target remains address-named until parity shows
it can feed back into player simulation.

The complete W1L1 ARE declaration inventory is now a native test fixture. It
checks all 173 declarations and the exact type counts for WURM2, BIENE,
animated world effects, cloud, falling leaves, dedicated LOOP effects, WERBE
pickups, and the seven `PUZZLE.BOB` letters. Each present type must resolve to
its recovered family kind, callback identity, and resource contract; dedicated
effect types intentionally remain callback-free parent declarations with their
`LOOP_W1.ICO` child path. The test also verifies that the callback-bearing
subset at the native player start is published through the scheduler. This is
an inventory gate, not a claim that every family has complete gameplay parity:
dedicated-event seed/lifetime and the transition presentation/menu handoff
remain explicit boundaries above. The falling-leaf
`01F7:4727/47E7` closure is now implemented: a replay may inject the captured
`DS:6468/646C` ring, and the native session publishes the exact signed table
choice, fixed-point fall, animation delay/cursor, and sprite slot.

Camera streaming is now an explicit runtime input. In camera mode the ARE
gate uses the recovered pixel window `X: camera-0x80..camera+0x1c0` and
`Y: camera-0x80..camera+0x130`; direct setup callers still use the former
player-coordinate fallback. W1L1 reset seeds the observed native startup
anchor `(0,262)`, and the SDL frontend publishes its settled camera for the
following tick. The native session also replays the Ghidra-derived
`01F7:1CDA -> 01F7:1E04` incremental stream cursor for camera-owned sessions;
W1L1 reset reproduces the observed selector settle from `x=511` to `x=0`.
Parity replay consumes the exact per-frame camera history from its required
canonical `--input-jsonl` stream. The scanner now accounts for
the six dedicated `0x65` records (one `01F7:5C11` byte each) and the later
phase-1 initialization of four leaf records (two bytes each), reproducing the observed W1L2 cursor
`DS:6468: 18 -> 32` from the captured ring. The complete boundary is recorded in
[`w1l2-startup-stream-v1.json`](../research/evidence/player-dos-parity/w1l2-startup-stream-v1.json);
the ring remains replay input rather than a hardcoded simulation rule.

Parity uses named recorded-run directories. Import accepts only the current
`quiky-player-dos-parity-v1` capture schema; validation and comparison consume
strict `quiky.parity-state-v2` JSONL on both sides:

For the normal interactive workflow, launch a visible window, play, and close
the window to finalize both the immutable capture and its processed run:

~~~sh
python3 research/tools/quiky.py capture w1l1-session --level W1L1
~~~

Capture and processing remain separate internally; pass `--capture-only` and
use `quiky capture process` when they should be run independently.

~~~sh
python3 research/tools/quiky.py run import research/runs/w1l1-session \
  --name w1l1-session --profile exact \
  --expected-trace research/evidence/player-dos-parity/w1l1-jump-property-v3.json
python3 research/tools/quiky.py run replay research/runs/w1l1-session \
  --binary build/engine/quiky-parity-replay \
  --archive game/NESTLE.DAT --map W1L1.MAP
python3 research/tools/quiky.py run verify research/runs/w1l1-session
~~~

`input.jsonl` is the only replay input format. Each
native sample records the complete callback pre/post record, ordered probes,
state/global/effect writes, camera anchor, scheduler/dependency order, active
entities/effects, gameplay globals, and queued events. The comparator is
fail-closed for player records and reports DOS capture coverage gaps instead
of treating an unrecorded probe or effect list as an empty equal list. The
unseeded archived fixture remains diagnostic for leaf bytes because its DOS
capture does not contain `DS:646C`. A fresh seeded four-frame comparison is
exact for player records, scheduler order, WERBE, and active leaf position,
velocity, animation cursor/delay, and sprite slot; the focused DOS capture's
missing later pool/probe/effect arrays are reported as coverage gaps.

For a capture requiring the replayable leaf ring, pass it to the canonical
replay command:

~~~sh
RING=$(python3 research/tools/derive_w1l1_leaf_ring.py \
  research/build/player-frame-full-v1.json)
python3 research/tools/quiky.py run replay research/runs/w1l1-session \
  --binary build/engine/quiky-parity-replay \
  --archive game/NESTLE.DAT --map W1L1.MAP \
  --leaf-prng-index 0 --leaf-prng-ring-hex "$RING"
python3 research/tools/quiky.py run verify research/runs/w1l1-session
~~~

This produces zero comparable mismatches for the complete player records,
scheduler order, WERBE object, and all six leaf callback identities, positions,
velocities, and sprite selectors. The exact seed prefix and the 60 honest
coverage gaps are recorded in
[`w1l1-seeded-session-object-parity-v1.json`](../research/evidence/player-dos-parity/w1l1-seeded-session-object-parity-v1.json).

The verifier fails on comparable mismatches and writes separate parity and
coverage reports. Comparison mode is selected explicitly by the run's
`exact` or `lifecycle` profile, never inferred from sample counts.

The first W1L2 difference family is now integrated from the focused
protected-mode closure: ARE type `0x34` runs `01F7:9BEE` initialization and
the phase-1 `01F7:9C0C` callback before `01F7:3FF8`. The native path preserves
the word-width `(x+0x10,y+0x20)` initializer shifts, the object `+0x20`
countdown and `400/402/403/401` descriptor cycle, the strict `39FE` player
gate, and the confirmed `1B5D -> 1B07` writes to the player record plus
`DS:8950` and `DS:612E=4`. BUMP contact is deliberately excluded from the
generic death reset path; the remaining `DS:3568` resource words, `1DCA`
camera inputs, and `0FCF` audio internals remain explicit external contracts.
The native contract test is `testRecoveredBumpCallbackContract` in
`tests/test_formats_faithful.cpp`.

The first W1L2 normal-enemy movement contract is also native: WURM2 types
`0x01/0x02` dispatch through `01F7:6D5F/6DC4`, publish the recovered signed
state-0 fixed-point movement, and evaluate the callback's exact descriptor
probe order. The bridge models `01F7:1C4D -> 5C27` at
`(object_x +/- 0x28, object_y - 0x28)` followed by the direct `5C27` side
probe at `(object_x +/- 0x26, object_y)`, including the original second-probe
branch polarity. When no descriptor table is available this remains an
explicit input boundary and does not fall back to a guessed occupancy rule.
The `01F7:707B` target-ring tail is now represented in the native level state:
it preserves the signed strict window, wraps `+0x30` against `DS:8808`, clears
only the matching `DS:87DE` X word, and publishes the address-qualified
`4AB3` response callback. The player-side `4519` producer and the `4AB3`
child/effect lifetime remain explicit contracts; a missing target ring is
represented by `DS:8806 == 0`. The renderer-owned `5D60` cursor remains
outside the simulation model. The focused tests are
`testRecoveredWurm2DescriptorProbeContract` and
`testRecoveredWurm2TargetTailContract` in `tests/test_formats_faithful.cpp`.
The 32-frame native scheduler trace and its fail-closed checker are recorded in
[`w1l2-native-scheduler-trace-v1.json`](../research/evidence/player-dos-parity/w1l2-native-scheduler-trace-v1.json)
and [`verify_w1l2_native_trace.py`](../research/tools/verify_w1l2_native_trace.py).
An independent natural DOS W1L2 trace now validates the ordinary WURM2
state-zero boundary: raw `+0x2F=0xFF` takes the `6F16` existing-velocity path,
advances `+0x2A`, and reaches the `707B`/`5D60` tail. The three valid `6DC4`
samples match the recovered fixed-point X integration and animation timing;
the positive-latch contact path remains an explicit natural-data boundary;
the target-tail mechanics are covered by the native contract test. The
evidence and fail-closed verifier are
[`w1l2-wurm2-natural-callback-v1.json`](../research/evidence/player-dos-parity/w1l2-wurm2-natural-callback-v1.json)
and [`verify_wurm2_dos_trace.py`](../research/tools/verify_wurm2_dos_trace.py).

## Unified player callback status

The unified `TraceClosedPlayerUpdate` is the C++ implementation of the
statically recovered `01F7:3FF8–44FE` callback path. Its branch order and
field writes are audited against
`research/notes/player-static-decomp.cpp` and
`research/ghidra/player-callback-focused-audit.json`.

Implemented from static evidence:

- callback entry, input normalization, action-counter updates, and the
  `+0x39`, `DS:89EA`, `DS:89E6`, and `DS:8812` gates;
- ordinary, positive, and negative mode dispatch;
- signed 16.16 acceleration, release clamp, gravity, apex, and terminal
  velocity arithmetic;
- ordered side/vertical probes, `3D02` descriptor correction, and `3DF2`
  alignment;
- jump initiation, grounded response, blocked ascent, landing-state writes,
  common-tail timer/global writes, the known pending jump sound dispatch, and
  the resident player animation loads/advancement at `5D38/5D60` for the
  recovered callback sequences, including the statically recovered turn and
  high-speed tables at `QUIKY_SEG06:3142` and `QUIKY_SEG06:3190`;
- the direct `01F7:5937` auxiliary publication contract: `DS:60DA`, the
  score/count/lives aliases at `DS:4FF2/4FF8/4FFA`, signed display-count
  stepping, and the address-qualified replay state needed before the
  transition gate. Nested `386F/0598` resource callbacks remain external;
- complete raw `0x78` pre/post records and ordered trace publication.
- the outer lifecycle-to-animation handoff: the interactive and frame
  frontends select the measured death table only when the callback record is
  in mode `-1`, health is zero, and the signed `DS:89EA` gate is negative.
  This keeps ordinary ascent and action word `4` on their normal tables.
- terminal `01F7:19E6` damage now publishes one `PlayerDied` event at the
  recovered state boundary; it does not reset the player or object scheduler,
  leaving the measured death hold available to the eventual `4BA4` consumer.

Dynamically parity-validated:

- the existing horizontal/free-space formula fixtures;
- the C++ callback's synthetic jump, grounded, and blocked-ascent branch
  vectors;
- the standing DOS replay's complete `0x78` post-records and observed global
  mutations across all eight callbacks;
- a player-scoped dense `5D60` trace through natural landing and the following
  ordinary callbacks, confirming the `3156` idle reload before `5D60` and
  exact post-record parity through that animation boundary;
- two unpatched W1L1 natural descriptor contacts: tile `0x28`/descriptor
  `0x0010` and tile `0x29`/descriptor `0x0050` are both traversed upward and
  later landed on through `3D02 -> 5CC3`, with grounded mode and zero vertical
  velocity after contact;
- candidate-to-candidate full-record/probe/global/effect comparison through
  canonical recorded runs;
- the complete v2 replay fixture, including all twelve `5937` inputs, exact
  player records, ordered property probes, and the direct `DS:4FF8` write.
- an eight-callback W1L2 input replay with a real `KBD_space+KBD_up` press and
  release: complete player records, normalized input, ordered property probes,
  and callback-global writes all match exactly.
- the natural W1L1 death trace's measured animation-state shape: action word
  `4` is retained while mode `-1`, the gate remains negative through the hold,
  and the death sequence uses the `20..28` table before checkpoint recovery.
- the terminal W1L1 damage fixture's event ordering: `PlayerDamaged` is
  followed by exactly one `PlayerDied` event while the terminal player record
  remains intact.
- a twenty-callback W1L2 replay through jump initiation, apex, free fall, and
  natural landing; complete records, ordered property probes, and global
  writes match exactly at every sampled callback.

The captured-DOS replay set is mixed: older standing and late-release fixtures
remain diagnostic when their source traces omit native probe arrays or opaque
helper outputs, while the complete W1L1 jump/property window is now a closed
fixture. It compares all ten callback records, normalized input words, ordered
`1C6E/1C92/5C27/5CC3` property probes, callback-global writes, and known
effects exactly. Dispatch-bearing captures retain all twelve auxiliary fields
at import; missing fields remain explicit coverage rather than defaults.

The first real W1L2 callback replay is also closed at the available capture
boundary: four one-frame-spaced DOS callbacks match the native implementation
for all `0x78` player bytes, ordered collision/property probes, and callback
global writes. An independent eight-callback rerun has the same zero
movement/probe/global mismatches. Its DOS capture does not publish
factory/effect arrays, so the strict parity mode correctly reports those as
missing coverage instead of declaring them equal. The result is recorded in
[`player-w1l2-session-parity-v1.json`](../research/evidence/player-dos-parity/player-w1l2-session-parity-v1.json).

The held-out W1L2 input replay extends that result beyond the stationary
startup window. The eight-callback `Space+Up` press/release capture reaches the
early ascent path and has zero complete-record, input, ordered-property, or
callback-global mismatches. Its capture and replay command are recorded in
[`player-w1l2-input-parity-v1.json`](../research/evidence/player-dos-parity/player-w1l2-input-parity-v1.json);
factory/effect arrays were not published by that DOS trace and remain explicit
coverage boundaries.

The held-out W1L2 landing replay extends the input fixture across the complete
short arc. It observes ordinary-to-ascent mode `0x00 -> 0xFF`, the apex
transition to `0x01`, and the natural landing transition back to `0x00` at
`y=512` with zero vertical velocity. The twenty-callback exact replay is
recorded in [`player-w1l2-landing-parity-v1.json`](../research/evidence/player-dos-parity/player-w1l2-landing-parity-v1.json).

The current W1L1 jump/landing replay closes the same arc at the main-repository
startup position: 20 callbacks match from jump initiation through apex, free
fall, and natural landing at `y=400`. It also validates the positive-mode
`41F7/4209` direct side-probe ordering and the repeated `3998` forward-probe
sequence. The evidence is recorded in
[`player-w1l1-jump-landing-parity-current-v1.json`](../research/evidence/player-dos-parity/player-w1l1-jump-landing-parity-current-v1.json).

The committed fixture and its candidate remain immutable evidence under
`research/evidence/player-dos-parity/`; import them into a named run before
comparison.

To include the callback's known pending-sound dispatch in a strict comparison,
add the Ghidra-addressed watch and request that field explicitly:

```sh
python3 research/tools/quikytrace.py --launch --headless --runtime-dir game \
  --select-level W1L1 --player-trace --player-focus-callback \
  --player-capture-record --player-parity-capture \
  --player-watch-execute 0x1e7:0xfcf \
  --output research/build/traces/player-effect-watch.json \
  --player-input-phase KBD_space+KBD_up:0 \
  --player-input-phase WAIT:1
python3 research/tools/quiky.py run import research/runs/player-effect-watch \
  --name player-effect-watch --profile exact \
  --expected-trace research/build/traces/player-effect-watch.json
```

The explicit watch publishes an empty effect array for callbacks where the
boundary was observed and records `01E7:0FCF` when it executes. This closes the
known effect field without treating the still-uninstrumented `0E06` factory
output as empty; use `--require-complete` only with a capture that also
publishes factory objects. The complete W1L1 jump/landing run with that watch
passes the strict effect comparison; its pinned result is
[`player-w1l1-effect-parity-v1.json`](../research/evidence/player-dos-parity/player-w1l1-effect-parity-v1.json).

The exact callback-selected animation words and the runtime-`DS` basis are
pinned in
[`player-animation-tables-static-v1.json`](../research/evidence/player-dos-parity/player-animation-tables-static-v1.json)
[`player-animation-tables-static.cpp`](../research/notes/player-animation-tables-static.cpp).

Explicit unresolved boundaries:

- whether the statically closed
  `648E/6484 -> 6370 -> 3376 -> 0E06(6328) -> 16CE` tile-effect path can
  alter a later retail solid or descriptor probe; its direct
  player/global/object writes, return-CF contract, pooled `6328` child
  lifetime/terminal callback clear, and controlled `16CE` MAP-word rewrite
  are recovered in [`player-contact-map-writer-v1.json`](../research/evidence/player-dos-parity/player-contact-map-writer-v1.json),
  and the direct controlled allocator selection is pinned in
  [`player-contact-factory-watch-v1.json`](../research/evidence/player-dos-parity/player-contact-factory-watch-v1.json),
  but natural feedback into a later callback remains open. The pre-input
  `DS:612E=7`/`01E7:0FCF` sound side effect is now implemented and pinned by
  [`player-contact-sound-watch-v1.json`](../research/evidence/player-dos-parity/player-contact-sound-watch-v1.json);
- runtime records outside the W1L1-observed `01F7:04DF`/`01F7:0517` target set
  at the `01F7:0598` indirect call site; the W1L1 low-byte matrix shows no
  bit-specific player simulation feedback, while non-W1L1/runtime-generated
  record targets remain address-qualified without a semantic rename;
- natural ceiling validation and a general descriptor semantic/one-way policy
  remain open. The two authored W1L1 contact cases above establish their
  shipped landing behavior without assigning global names to descriptor bits
  `0x20`/`0x40`; the focused branch-entry run is archived as an ordinary-apex
  negative control in [`player-natural-branch-entry-negative-control-v1.json`](../research/evidence/player-dos-parity/player-natural-branch-entry-negative-control-v1.json), not as ceiling evidence;
  a held-through-callback W1L1 jump now provides a second ordinary-apex and
  landing control in [`player-natural-apex-held-negative-control-v1.json`](../research/evidence/player-dos-parity/player-natural-apex-held-negative-control-v1.json).
  It observes the normal `4323 -> 3986` ascent probes and no `4368`, `41C1`, or
  `41CF`; it therefore does not close natural-ceiling behavior;
- moving-platform landing/inherited velocity, crushing, and the attached-player
  effect of culling; controlled motion, first-carry consumption, jump
  detachment, and complete post-`3FF8` record capture are validated, but
  retail-geometry contact and recontact policy remain open;
- the known static damage ownership routes are now explicit: rectangle contact
  `01F7:1B77 -> 393C -> 19E6`, and special-tile contact
  `01F7:3A8A -> 3376/1B07 -> 19E6`. The W1L1 WURM2/BIENE rectangle route is
  native and covered by a terminal/nonterminal contract fixture; `19FB` and
  delayed death/recovery remain external boundaries. This does not identify
  every natural hazard family or replace the runtime ownership fixture;
- death/recovery static writes are now closed (`01F7:199D`, `19E6`, `1AAA`,
  `1AE6`, `1AF5`, `01D7:34C7`, `01D7:3861`, and `01D7:4BA4`), including the
  recovery MAP/spawn-table rebuild contract. A focused natural W1L1 trace now
  confirms the runtime ordering `01F7:1AAA` -> next `01F7:3FF8` callback,
  including the indexed respawn position, reset motion, callback identity, and
  idle animation state. A follow-up lifecycle trace captures the raw row-zero
  resource words `(1673,374)` at `01F7:1AAA`; the first recovered callback's
  `(1673,368)` Y value is therefore retained as a callback-side correction,
  not folded into the resource declaration. A focused transition trace additionally observes the
  signed gate countdown through `-347`, entry at `01D7:4BA4` with `DS:89EA=-350`,
  health restoration at `4BD8`, gate clear at `01F7:1AE6`, and the next
  recovered `3FF8` callback. The same focused trace observes the player in
  both rotating banks before recovery and in the rebuilt `0x7766` bank at a
  different physical index afterward; callback `0x3ff8` plus object offset
  zero remains the identity. `LevelSession` still collapses a generic hazard
  hit into an immediate reset; the initial death setter, cross-level
  generality of the `-350` threshold, opaque timer/resource calls, delayed
  teardown, and exact bank-copy ordering remain explicit boundaries;
- the targeted `01D7:4BA4-4EFE` relocation expansion now classifies the concrete
  lifecycle targets: `01F7:106A` can remove dead scheduler callbacks,
  `01F7:1AF5` restores health/spawn selection before `1AAA`, and `01F7:321F`
  rebuilds MAP page aliases; its `17AE/17D4` targets initialize and clear the
  pending ARE-event queue. The camera bridge through `20AF -> 31D1 -> 1ED7`
  now has static fixed-point and page-refresh contracts. Timer/VGA/resource targets remain address-named
  contracts because they do not directly write player simulation state;
- the scheduler recovery edge is now separately pinned in
  [`player-scheduler-lifecycle-static-v1.json`](../research/evidence/player-dos-parity/player-scheduler-lifecycle-static-v1.json):
  `4BA4 -> 106A` first calls `17D4` to clear pending ARE-event ownership,
  then performs selected-bank cleanup; sourced entries call `1DEE` while
  source-less entries clear their phase callback before
  `1AF5 -> 1AAA -> 0B56 -> 0E06 -> 1036` resets and republishes the player
  entry, while `0FA2` consumes secondary callbacks afterward. The opaque
  resource/teardown calls around that sequence and post-culling membership
  remain unresolved.
- the no-descriptor-table fallback, which remains a deliberate research
  boundary.

The repeatable workflow is `quiky run import`, `quiky run replay`, then
`quiky run verify`. Missing records, probes, globals, or effect/factory data
are reported as coverage; unequal published values are mismatches.

## Horizontal player closure

The confirmed horizontal callback slice is integrated behind the explicit
player-updater seam. `PlayerRecord`
preserves all `0x78` bytes and exposes the static-closure fields, including
the overlapping pixel-word views and confirmed horizontal constants. The
trace-closed updater integrates old X velocity first, applies
right-before-left precedence, uses `0x2800` acceleration, `0x2000` release
friction, and the `+/-0x18000` cap through `Fixed16` helpers. For records
already in an airborne mode, it also applies the confirmed free-space vertical
branch, including release clamping, apex transition, gravity, and terminal
velocity.

`CollisionKernel` is a pure MAP/descriptor query layer for the recovered
quadrant mask, ordered `x-5`/`x+5` probes, `3DF2` Y alignment, and `3D02`
descriptor response. `TraceClosedPlayerUpdate` composes those helpers with
the statically recovered floor/ceiling/grounded branch writes. The separate
`updatePlayerVerticalFreeSpace` API remains a leaf for callers that already
have clear probes; it is not the unified callback.

The compact `tests/fixtures/player-horizontal-v1.tsv` contains the 1,261
formula samples represented by all 5,044 values checked by the Python model.
Regenerate it from the research ledgers with:

```sh
python3 engine/tools/generate_horizontal_fixture.py \
  research/evidence/player-horizontal \
  engine/tests/fixtures/player-horizontal-v1.tsv
```

The `quiky-horizontal-tests` target checks record round trips, all horizontal
held-out values, direct acceleration/reversal/cap vectors, Python collision
kernel parity vectors, snapshot isolation, and trace diagnostics. The
`quiky-vertical-tests` target checks the 15 trace-closed free-space rows and
the canonical updater integration. `quiky-player-callback-tests` checks the
focused jump, grounded, and blocked-ascent records and ordered probes.

## Runtime descriptor integration

`playerDescriptorTableForWorld()` reproduces the five world initializer
tables and `LevelRuntime` supplies the selected table alongside streamed MAP
cells. Descriptor-backed horizontal checks use the recovered forward probes
at `(x +/- 10, y - 1/-17/-33)`. Table boundaries, special descriptor values,
and probe coordinates have direct engine tests.

No compatibility player simulation remains. The C++ callback contains the
static landing, ceiling-response, jump, and recovered animation writes. The
two authored W1L1 descriptor contacts are now runtime evidence for the
collision contract; natural ceiling parity, general descriptor class names,
one-way behavior, and moving-platform contact remain explicit research
boundaries until targeted runtime traces close them.

Music playback is an optional subsystem because the bundled `.TFX`/`.SAM`
resources use TFMX, which needs a dedicated decoder. The engine includes an
in-tree four-voice TFMX Pro parser, sequencer, Paula-style mixer, and gameplay
SFX scheduler. It reads paired resources directly from `NESTLE.DAT`, renders
WAV, and can play through SDL3 when SDL3 is discoverable by CMake. No external
audio runtime is required.

Build it with the normal engine configuration:

```sh
cmake -S engine -B build/engine -DSDL3_DIR=/path/to/SDL3/cmake/config
cmake --build build/engine
build/engine/quiky-music game/NESTLE.DAT list
build/engine/quiky-music game/NESTLE.DAT render TITEL /tmp/TITEL.wav
build/engine/quiky-music game/NESTLE.DAT play TITEL
build/engine/quiky-sfx game/NESTLE.DAT list ONGAME2
build/engine/quiky-sfx game/NESTLE.DAT render ONGAME2 0 /tmp/sfx-0.wav
build/engine/quiky-sfx game/NESTLE.DAT play ONGAME2 0
```

`quiky-sfx` exposes all fourteen gameplay effect IDs. The `SfxModule` API in
`include/quiky/sfx.h` keeps TFX/SAM loading, four-voice priority scheduling,
macro interpretation, and PCM output separate. A frontend can call `trigger`,
advance the 50 Hz voice state, and mix PCM blocks, or use the deterministic
`render` helper for one-shot diagnostics. `quiky-play` now uses the shared
`AudioMixer` and triggers the confirmed pickup mappings, including the
puzzle-letter state-0 branch (ID 11). The event mapper exposes semantic
`GameplayCollectible` and `GameplaySfx` names in `include/quiky/sfx_events.h`,
so the frontend does not depend on raw entity-type or effect-slot literals.
Jump, player death/respawn, entity impact, tile interaction, alternate action,
and world-object events now use their confirmed named SFX roles. Exit events
and unresolved effect-table IDs remain silent until their dispatch semantics
are proven.

For short sequencer diagnostics, set `QUIKY_TFMX_TRACE` to the number of
50 Hz ticks to print before rendering:

```sh
QUIKY_TFMX_TRACE=50 build/engine/quiky-music game/NESTLE.DAT render \
  TITEL /tmp/TITEL-trace.wav 2>/tmp/TITEL-trace.log
```

Build and test from the repository root:

```sh
cmake -S engine -B build/engine
cmake --build build/engine
ctest --test-dir build/engine --output-on-failure
```

Launch the default W1L1 session without naming the derived world resources:

```sh
build/engine/quiky-play game/NESTLE.DAT
```

Inspect the bundled archive:

```sh
build/engine/quiky-inspect game/NESTLE.DAT info W1L1.MAP
build/engine/quiky-render game/NESTLE.DAT W1L1.MAP /tmp/W1L1.bmp
build/engine/quiky-render game/NESTLE.DAT W1L1.MAP /tmp/W1L1-entities.bmp --overlay-are
build/engine/quiky-bob-sheet game/NESTLE.DAT QUIKYW1.BOB W1.PCC /tmp/QUIKYW1.bmp
build/engine/quiky-simulate game/NESTLE.DAT W1L1.MAP 100 100 120 0x04
```

For a controlled DOSBox scene comparison, `quiky-frame` writes the same
320x200 indexed frame size used by the automation screenshots. The high
effect probe below places the recovered effect at world `(300,510)`, advances
its cursor to 5, and fixes the camera to the captured DOSBox position:

```sh
build/engine/quiky-frame game/NESTLE.DAT W1L3.MAP /tmp/W1L3-frame.bmp \
  --frames 5 --high-effect 300 500 --camera-x 0 --camera-y 358 \
  --no-player --no-entities --high-effect-only
PYTHONPATH=research/tools python3 research/tools/scene_frame_compare.py \
  /tmp/W1L3-frame.bmp research/build/high-effect/w1l3-visual-forced-5-v2-raw.png \
  --region 280 110 40 50
```

The probe can include the player and active ARE objects for scene-level
comparisons; `--no-player`, `--no-entities`, and `--high-effect-only` isolate
the recovered effect from the player, ARE sprites, and ARE transient queue.

The renderer output is an indexed BMP using the palette stored in the matching
world PCC resource. The C++ W1L1 output matches the existing Python renderer
pixel-for-pixel when ARE overlays are disabled. The ARE overlay is a debug
visualization; it does not assign gameplay meanings to entity types.
