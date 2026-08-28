# Player callback closure

The authoritative, mechanically auditable representation is
[`player-callback-closure.json`](player-callback-closure.json). Each function
record contains its address, recovered name, signature, inputs/outputs,
player/global reads and writes, callees, classification, confidence, and
evidence. The address-qualified rename/import script is generated from that
ledger; it is not hand-maintained.

The authoritative C-like control-flow reconstruction is
[`player-static-decomp.cpp`](../notes/player-static-decomp.cpp). It keeps the
original callback labels and address-qualified helper calls, uses exact-width
types and explicit fixed-point arithmetic. The direct `5937` body and its
`386F/0442` dispatch entries are covered by the external-state ledger. The
`0598` instruction is an indirect call site rather than a separate function;
its W1L1-observed `04DF`/`0517` targets are already decompiled, while records
outside that observed target set remain address-named.

The focused branch/write supplement for the under-specified contact paths is
[`player-callback-focused-audit.json`](player-callback-focused-audit.json),
with a short review narrative in
[`player-callback-focused-audit.md`](player-callback-focused-audit.md). It
records every write to `+0x37`, `+0x3A`, `+0x3B`, `+0x3E`, and `+0x40`, and
makes the `3D02`/`3DF2` caller ordering explicit.

The focused external-state extension is
[`player-external-state-closure.json`](player-external-state-closure.json).
It covers scheduler/carry publication, contact/effect callbacks, `5937`
dispatch state, descriptor backends, animation loaders, and the transition
boundary without expanding unrelated game systems. Its latest focused
expansion also decompiles the death/recovery state writers at `199D`, `19E6`,
`1AAA`, `1AE6`, and `1AF5`, plus the post-load map helper at `01D7:34C7`, the
recovery map loader at `01D7:3861`, and the relevant main-loop lifecycle gate
at `01D7:4BA4`. The static writes are closed; natural hazard ownership,
delayed teardown, and resource values remain explicit runtime boundaries.

The targeted spawn-table census removes one of the remaining static
ambiguities: `DS:8828` is a 32-row interleaved `(x,y)` table. `01D7:34C7`
publishes all rows from the selected resource row, `01F7:8E4B` state 10
updates row zero, and `01F7:1AAA` consumes `DS:85D2` as the row index. Direct
code writes to `DS:85D2` at `01D7:4603`, `4B6B`, `4BD5`, and `01F7:1AFB` all
reset it to zero. The resource/DOS values and any indirect selector writes
remain address-named in
[`spawn-table-static-v1.json`](../evidence/player-dos-parity/spawn-table-static-v1.json).

The latest caller-focused expansion resolves the two known `19E6` ownership
routes: rectangle contact through `1B77 -> 393C -> 19E6`, and special-tile
contact through `3A8A -> 3376/1B07 -> 19E6`. A natural W1L1 trace then identified
the observed hazard owner as `01F7:6DC4`, with the concrete route
`6DC4 -> 1B77 -> 19E6`; `199D` was not executed for that contact. Ghidra now
exports `6DC4` and its descriptor probe helper `1C4D` in the same independent
project check.

The selector/initial-stream expansion is now also closed through the first
natural W1L1 declaration: `4B6E -> 1AAA -> 0B56 -> 0E06` reconstructs the
player object, then `321F -> 1CDA -> 1E04` walks the first ARE records. The
W1L1 special types are also closed through
`178D/1798/17A3 -> 1749 -> 181C/1892 -> 16CE`; their human-facing subtype
labels remain deliberately conservative.

The `5937` record-management edge now includes the normal loader at
`01D7:39ED-3BAB`. Its relocated `3A0A -> 01F7:05A0` call selects a `0x2c`
byte `DS:6D8A` record; the static write order for resource fields and the far
callback words is recorded in the loader note and evidence. Runtime resource
sizes, buffers, and callback selectors remain address-named boundaries.

The focused type-0x34 BUMP expansion now closes `01F7:9BEE -> 9C0C ->
1B5D -> 1B07`. The protected-mode listing records the raw word-width
initializer additions, the `DS:85DA` gate and strict overlap ordering, the
complete persistent-player response writes, and the `DS:612E` effect handoff.
Descriptor contents, camera inputs, and normal-level contact frequency remain
runtime boundaries; no broader enemy decompilation is implied.

The v34 relocation expansion follows the concrete far-call targets inside
`01D7:4BA4-4EFE`. `01F7:106A` can clear a dead object's scheduler callback;
its relocated `01F7:17D4` target clears the pending 0x80-entry ARE-event
queue;
`01F7:1AF5` restores health and the spawn-row selector before calling
`1AAA`; and `01F7:321F` rebuilds the two MAP page aliases and publishes the
`81D1` refresh signal. The same pass closes the camera bridge through
`17AE`, `20AF`, `31D1`, and `1ED7`: event-slot initialization, clamped target
deltas, camera page origin, target-bound scrolling, and page refresh ordering
are now address-annotated. The remaining segment-2/4 targets were classified as
resource, timer, VGA, or presentation contracts with no direct player-record,
`89EA`, MAP-descriptor, or scheduler write. The indirect callback invoked by
`0908`, timer ownership of the death hold, and cross-recovery bank membership
remain explicit runtime boundaries.

The transition-effect leaf reached by the player death writer is now closed
with the same protected-mode pipeline. `01E7:0CE3` calls the runtime stack
guard `0227:05CD`, conditionally writes `DS:504C=0x18`, and calls
`01E7:33D5` with `BX=8/CX=0x40/DX=0x3F`. `33D5` writes only
`FFFF:2FE9/2FEB/2FEC`; `05CD` only maintains the stack watermark or branches
to its runtime failure handler. These leaves have no player/MAP/descriptor/
scheduler writes or callback simulation flags, so the path stops at the
contract boundary documented in
[`player-transition-effect-static-decomp.cpp`](../notes/player-transition-effect-static-decomp.cpp).

Current closure: 113 address-qualified entries — 16 inline, 96 contract, and
one irrelevant presentation callback —
with the scheduler, callback, contact, lifecycle, and first-stream edges
recorded in the ledger. The direct `5937 -> 386F -> 0442/04DF/0517` body is
now statically closed. The `0598` call site has no W1L1 simulation feedback
through its observed targets; only runtime records outside that target set
remain address-named. The closure includes the input
normalization boundary, action counters, horizontal integration, ascent and
falling paths, descriptor/side probes, jump initiation, landing/ceiling and
side-contact responses, animation selection/advancement, effect/sound
dispatch, transition helpers, view publication, and the common callback tail.
The platform initializer closure now statically covers `9C70`, `9CF5`, `9D19`,
`9D5E`, `9D82`, and the relocated MAP word probe `5DA1`; all four variants
publish `9DC7` with the exact width/direction/sprite-slot matrix recorded in
the external-state ledger.
The latest focused protected-mode Ghidra expansion now preserves the exact descriptor backend
contracts at `1C6E/1C92/5C27/5CC3/5DC3` in
[`descriptor-backend-static-closure.json`](descriptor-backend-static-closure.json),
including the distinct `x>>4` map probes, `x>>3 & ~1` descriptor lookup, and
the `5C27` quadrant bit table. The `A06F -> 1DEE` cull
tail, and the three relocated scheduler pairs:
`44FA -> 0E96 -> 4518 -> 0FA2`, `47FC -> 0E96 -> 481A -> 0FA2`, and
`4872 -> 0E96 -> 4890 -> 0FA2`. This closes static ordering; attached-player
behavior after culling and natural platform contact remain runtime boundaries.
The scheduler tail is now also explicit: `0FDC` clears `DS:8174`, walks the
same eight-byte bank entries, suppresses callback-segment-zero and object
`+0x12` bit `0x8000` entries, and supplies object `+0x04/+0x08/+0x12/+0x16`
through the recovered AX/BX/CX/DX/CL register contract. The indirect callback
target remains address-named because its identity comes from runtime table
data; this is the stopping boundary for the scheduler static expansion.
The W1L1 WURM2 family is now also statically closed in the same protected-mode
export. `6D5F` seeds the pooled record, `6DA3/6DB1` select the two orientation
variants, and `6DC4` preserves the exact `1B77` rectangle call, `1C4D` carry
test, direct `5C27` side probe, four-state fixed-point movement, and
`707B -> 5D60` target/effect tail. Its static representation is
[`wurm2-static-decomp.cpp`](../notes/wurm2-static-decomp.cpp); the only player
feedback edge is the indirect `6DDD -> 1B77 -> 19E6` damage route.
The callback-tail effect boundary is also explicit: `38CA` publishes the
conditional `+0x5C` timer/state write, while `38EC` edge-latches `+0x3C` and
allocates the `4519` phase-1 contact effect through `0E06`.
The shared normal-enemy response is now closed as well: `4AB3` and `4BA0`
select action words `0x0D` and `0x02`, switch the source to `4C5D`, and
allocate three child effects through `4C8B` or `4EC9`. Those initializers and
their `4D44`/`4F82` children consume the exact `DS:6468`/`DS:646C` random
ring; `4DCE` is sealed as a presentation-only motion/lifetime callback after
its `1DCA`/`5D60` contract.

The bounded W1L1 object expansion is now recorded in
[`w1l1-object-callbacks-static-decomp.cpp`](../notes/w1l1-object-callbacks-static-decomp.cpp).
It closes the actual function bodies behind the remaining authored W1L1
families: leaf initializer/callback `4727/47E7`, the five numeric collectible
initializers, the seven puzzle-letter initializers, shared collectible
`8D20/8D31`, and cloud `9256/9269`. The listing proves the exact initializer
offsets, animation slots, masks, strict aligned overlap comparisons, subtype
global writes, cloud `DS:89E6` latch, and leaf timer/PRNG state. `474D`, `8E42`,
and `92A9` are interior labels in those containing bodies, not duplicated
functions. Visibility, map/effect, sound, renderer, and runtime resource
values remain address-named at their explicit stopping boundaries.

The focused leaf follow-up removes a stale structural gap from that closure:
`47E7` now names its relocated `1DCA` camera gate, `1DEE` release,
`1BD1` descriptor probe, and `5D60` animation advancement explicitly. The
remaining leaf question is only which branch the runtime MAP/descriptor data
selects; no callback callee is still unidentified. The contract is recorded in
[`falling-leaf-static-v1.json`](../evidence/player-dos-parity/falling-leaf-static-v1.json).

Static evidence for the main response paths is retained at these source
labels:

- jump initiation and animation selection: `ordinary_correction_42c9`;
- landing/grounded response: `grounded_contact_427f`;
- ascent/ceiling probe and response: `negative_mode_4323`;
- falling/side-contact response: `positive_mode_41e8`;
- transition path and common tail: `transition_block_4416` and
  `common_tail_4384`.

The transition branch's final descriptor probe is separately represented as
`probe_transition_descriptor_1BD1` in the external ledger and C-like source;
its CF-only contract is mechanically recovered from `1BD1-1C10`.

## Reproducible checks

From the repository root:

```text
python3 research/tools/verify_player_callback_closure.py
python3 research/tools/run_player_callback_baseline.py
python3 research/tools/verify_player_callback_closure.py \
  --callgraph research/build/player-callback-baseline/callgraph-a.json
```

The baseline runner extracts the exact NE segments, imports them with Ghidra
12.1.3 using `x86:LE:16:Protected Mode:default`, disassembles and constructs
the declared function ranges, applies generated ledger names/comments and
the typed player record, exports decompilation/body inventories and call
graphs, and repeats the import in two independent projects. It fails if the
two exports differ. The generated baseline lives under ignored
`research/build/` output.

The Ghidra-native graph export is retained as
`ghidra-callgraph-*.json`. It emits only direct near-call flow recovered from
the disassembled segment and records far/indirect calls as unresolved. The
verifier independently checks those near edges against raw bytes and checks
far edges against the executable's NE relocation table; the ledger-backed
`callgraph-*.json` remains the complete contract graph.

The PyGhidra audit view in `quiky-analysis.json` is generated from this ledger
by `generate_quiky_analysis.py`; the baseline fails if that projection drifts.

## Unresolved or partially resolved items

The W1L1 BIENE contact closure is now included in the same reproducible
Protected Mode export. `01F7:684A` initializes the shared object state,
`689F/68AD` select the two orientation variants, and `68C0` runs the complete
states `0..8` callback before the shared `6D01 -> 5D60` tail. The static
representation is [`biene-static-decomp.cpp`](../notes/biene-static-decomp.cpp)
and the machine-readable contract is the `01F7:68C0` entry in
[`player-external-state-closure.json`](player-external-state-closure.json).
It records the `1B77` player rectangle, oriented `1C4D` probes, `1BD1`
transition probe, `DS:7974` sine lookup, all callback field writes, and the
`+0x18=4AB3` contact-response handoff. This is a static closure increment;
the native implementation deliberately remains at the previously validated
state-zero boundary until a held-out BIENE callback trace is available.

The W1L1 WURM2 closure is represented by
[`wurm2-static-decomp.cpp`](../notes/wurm2-static-decomp.cpp) and the
`01F7:6D5F`, `6DA3`, `6DB1`, and `6DC4` ledger entries. The initializer trio
and callback are fully listed; native integration remains gated on a combined
object/player parity trace rather than being inferred from the static family
alone.

The focused contact-response representation is
[`contact-response-static-decomp.cpp`](../notes/contact-response-static-decomp.cpp).
Its source callbacks set the recovered action/global state and allocate the
three child objects; `4C5D` expires the source after the exact `+0x2A > 0x28`
test. The child `4DCE` callback writes no player, MAP, or callback-global
state, so it remains outside the gameplay closure beyond its explicit
lifetime contract.

- `01F7:5937` (`player_helper_5937`) has a statically recovered direct
  contract, including `DS:60DA` publication and auxiliary writes. The shared
  `0442/04DF/0517` BP-frame dispatch body is also closed, including its
  `RETF 0x6` and direct-write set. `0598` is the shared indirect call site;
  W1L1 observations select the already-decompiled `04DF`/`0517` entries and
  show no simulation feedback. Runtime records outside that target set remain
  address-named and are the only reason this edge stays open.
- `01F7:0E06` (`object_pool_factory_0E06`) is a closed external contract for
  the contact factory's arguments and object/pool side effects. The focused
  controlled-contact watch confirms the player route selects `AX=0x6328,
  DX=0` and produces the observed pooled `01F7:6328` child; unrelated caller
  families remain address-qualified.
- The pre-input `01F7:648E`/`6484` contact route now has a native contract
  for its direct effect boundary: the focused watch confirms
  `DS:612E=7` followed by `01E7:0FCF` with `EDX=0`. The native callback emits
  that dispatch and takes the statically recovered negative-mode `41C1`
  response; pooled `0E06` allocation and `6328 -> 16CE` feedback remain
  address-qualified.
- `01F7:F21B/F21C` is retained as the input dispatch boundary. Ghidra emits
  malformed computed-jump warnings beyond the valid segment image; the raw
  bytes, function name, action-bit contract, and callback call site remain
  recorded without assigning a semantic target to the computed jump.
- Death/recovery writes are statically closed in
  [`player-external-state-closure.json`](player-external-state-closure.json):
  `199D`/`19E6` publish the death or damage state, `1AAA` reconstructs the
  player from the indexed spawn table, `1AF5` restores health and resets the
  spawn-row selector, `01D7:34C7` publishes the map dimensions/storage and
  spawn table, and `01D7:3861` rebuilds the MAP backing store before the next
  gameplay pass. Natural W1L1 ownership is now closed for the observed
  `6DC4 -> 1B77 -> 19E6` route; remaining work is additional hazard-family
  coverage, delayed death animation/teardown, scheduler membership during
  recovery, and resource values.

No enemy, boss, menu, renderer, or unrelated audio closure is included here.
