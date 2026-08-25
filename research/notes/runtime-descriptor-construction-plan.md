# Runtime descriptor construction research plan

## Objective

Determine, with reproducible static and runtime evidence, how a MAP cell's
low-nine-bit tile ID is mapped to a loaded descriptor record, what every field
in that record means, and which runtime systems mutate loaded MAP words.

The investigation starts from facts already established elsewhere in the
repository:

- `01F7:3376` resolves world coordinates to a word in the loaded MAP buffer.
- `01F7:5C27` and `01F7:5CC3` mask that word with `0x01ff`.
- The resulting tile ID indexes the far table at `DS:6584:DS:6582`, using
  `DS:30D4` as the record stride; collision code reads a word at record `+2`.
- The loader separately performs an observed `OR 0x10` mutation on one runtime
  MAP row. Static transition decompilation now distinguishes initial loading
  (`365B` from `4009`) from the W1L3–W5L3 secondary path (`3861` from the
  `48B5` scheduler); its byte/word effect and writer inventory are now
  established, while its gameplay purpose and any runtime-constructed callers
  remain under investigation.

These are starting constraints, not a substitute for tracing table creation.

## Questions to answer

1. Which loader allocates the descriptor table, sets `DS:6582/6584`, and sets
   `DS:30D4`?
2. Is descriptor index `n` always identical to ICO tile ID `n`, or is an
   index/remap table involved for any world, auxiliary tileset, or level?
3. What is the descriptor record size in each load path, and is it constant?
4. Which instruction writes each record field, and from what source data or
   derived computation?
5. What are the byte offsets, widths, signedness, and demonstrated consumers of
   every descriptor field?
6. Which descriptor bits affect collision, drawing, animation, or other
   behavior, and which names can be proven rather than guessed?
7. Which routines write the loaded MAP after file decoding, at what lifecycle
   stage, and whether they alter tile IDs, upper property bits, or both?
8. Does the runtime MAP retain archive dimensions/layout, or become a streamed,
   padded, transposed, or otherwise transformed buffer?

## Evidence standard

For each conclusion, record the evidence class:

- **Static:** instruction address, decoded operation, caller, and data flow.
- **Runtime:** breakpoint/watchpoint address, before/after values, selected
  level, frame/lifecycle stage, and protected-mode selector handling.
- **Archive correlation:** source payload, byte/record index, and exact value
  comparison against live memory.
- **Controlled mutation:** one changed input or debugger value, paired with a
  baseline and a repeatable observed consequence.

Field names remain neutral (`field_02`, `flags_02`, and so on) until at least a
consumer and a controlled contrast support a semantic name. A single
correlation is not sufficient.

## Work plan

### 1. Establish a loader call graph and lifecycle timeline

- Revisit the primary and secondary MAP loaders at `01D7:365B` and
  `01D7:3861`, then identify their callers and adjacent ICO/palette/resource
  setup.
- Find all static reads and writes of `DS:6582`, `DS:6584`, and `DS:30D4`.
- Trace allocation, initialization, population, publication, reuse, and free
  operations for the far descriptor table.
- Record when MAP decode, descriptor construction, and the first renderer and
  collision queries occur relative to one another.

Deliverable: an address ledger and a compact lifecycle diagram in the research
notes.

### 2. Capture descriptor-table construction

- Break on writes to the three descriptor globals and capture the allocating
  call stack, selector, offset, allocation size, and stride.
- Add a narrow runtime trace at the record-population loop: input index,
  destination address, source pointer/index, and the complete record bytes
  before and after each iteration.
- Capture at least W1L1 and one world whose ICO count differs (W2), plus a level
  using secondary MAP loading if that path is reachable.
- Compare populated record count against the relevant ICO tile count and the
  maximum tile ID present in the selected MAP.
- Test boundary IDs `0`, last referenced ID, and last available ICO record.

Deliverable: machine-readable construction traces and a report mapping
`tile_id -> descriptor offset -> source tile/resource`.

### 3. Recover the descriptor schema

- Infer the record extent from the construction loop and `DS:30D4`, then list
  every write within one iteration.
- Find all code references to each record offset, including renderer and
  collision consumers; distinguish reads from writes.
- Build a cross-world census of unique values at every byte/word offset.
- Correlate fields with ICO pixel data and known geometry. Check dimensions,
  origin/hotspot, flags, animation linkage, transparency/occupancy masks, and
  any cached pointers without assuming those interpretations.
- Use controlled tile substitutions or live descriptor patches to test fields
  that cannot be named from static data flow alone.

Deliverable: a schema table containing offset, size, signedness, producer,
consumers, value range, proposed meaning, confidence, and supporting artifacts.

### 4. Prove tile-ID mapping

- At descriptor construction, renderer lookup, `5C27`, and `5CC3`, log the
  same selected tile IDs and destination addresses.
- Verify the address formula against observed memory rather than only the
  decompilation:

  `descriptor = far(DS:6584, DS:6582 + (cell & 0x01ff) * DS:30D4)`

- Compare descriptor imagery/source identity against ICO record `tile_id`.
- Search for remap tables, range splits, auxiliary ICO sources, sentinel IDs,
  and out-of-range handling. Treat direct identity mapping as proven only for
  the tested resource families and ranges.

Deliverable: a mapping specification with explicit normal, boundary, and
exception cases.

### 5. Inventory and classify runtime MAP mutations

- Find all static writes through the loaded MAP far pointer, not just reads by
  `3376`. Classify bulk decode/copy separately from post-load mutations.
- Watch the loaded MAP memory from the end of file decode through gameplay and
  log writer address, old/new word, coordinates, and lifecycle timestamp.
- Reproduce the known loader `OR 0x10`; identify its loop bounds and whether
  `0x10` affects the low-nine-bit tile ID or the upper property field in the
  actual word representation.
- Compare the live MAP immediately after decode, after descriptor setup, after
  level initialization, and after representative gameplay events.
- Test whether moving platforms, collectibles, breakable tiles, triggers, or
  streaming boundaries rewrite MAP cells. Keep debugger-only patches clearly
  separated from engine writes.

Deliverable: a mutation ledger with writer routine, trigger, affected bit mask,
coordinate range, persistence, and hypothesized purpose.

### 6. Validate with controlled experiments

- Generate separate archive variants changing one MAP tile ID or upper-field
  bit at a time; never modify the reference archive.
- Pair each variant with a baseline on the same executable, level, position,
  and timing.
- For descriptor flags, patch one field/bit temporarily in live memory and
  restore it after the queried call; record both positive and negative cases.
- Repeat key observations at least twice and across two levels/worlds where
  practical.
- Add unit tests for any new trace normalization/reporting code and keep raw
  trace artifacts out of source-control paths unless intentionally curated.

Deliverable: a small reproducible experiment matrix connecting input change,
live MAP word, descriptor record, and observed consumer behavior.

## Proposed implementation artifacts

- `research/automation/quiky_descriptor_build_trace.lua` — focused loader and
  construction trace, separate from player behavior tracing.
- `research/tools/descriptor_build_report.py` — normalize traces, compare
  worlds, and emit the schema/mapping census.
- `research/build/traces/descriptor-build-*.json` — generated raw evidence.
- `research/build/descriptor-schema.json` — generated field census and
  evidence links.
- `research/notes/runtime-descriptor-construction.md` — final address ledger,
  mapping specification, schema, mutation ledger, and conclusions.

Names may be adjusted to fit discoveries, but construction evidence should not
be folded into the player tracer unless it genuinely shares the same lifecycle.

## Recommended execution order

1. Static xrefs and loader call graph.
2. Runtime writes to descriptor globals and allocation capture.
3. One complete W1L1 construction trace and raw-memory dump.
4. Record schema/xref census.
5. W2 cross-check and tile-ID boundary tests.
6. MAP write watchpoints and lifecycle snapshots.
7. Controlled field and MAP mutations.
8. Consolidated specification, tests, and README summary.

This ordering resolves the table's provenance before assigning meanings to its
contents and establishes normal construction before investigating mutations.

## Completion criteria

The research item is complete when:

- the allocation/population/publishing path is identified with addresses and a
  reproducible trace;
- the tile-ID-to-record formula is verified across at least two worlds, with
  boundary and exception behavior documented;
- every descriptor byte has a producer and known consumers, and semantic names
  carry explicit confidence/evidence;
- all observed post-decode MAP writers are inventoried, including the known
  `OR 0x10`, with before/after values and triggers;
- archive bytes, live MAP words, descriptor records, and consumer outcomes can
  be joined in one evidence trail; and
- supporting tools pass the repository test suite and the final notes clearly
  separate confirmed facts, strong inferences, and open questions.

## Risks and mitigations

- **Protected-mode pointer mistakes:** use selector-aware reads and validate a
  known descriptor value before trusting a trace.
- **Confusing MAP upper bits with descriptor flags:** always report the raw MAP
  word, `cell >> 9`, `cell & 0x01ff`, and descriptor fields separately.
- **Construction hidden in a shared resource loader:** trace from writes to the
  published globals backward, then correlate with loader call sites.
- **Watchpoint noise from bulk loading:** snapshot at lifecycle boundaries and
  enable fine-grained write logging only after decode completes.
- **World-specific layouts:** sample W1 and W2 before generalizing record count,
  stride, resource identity, or field semantics.

## Execution log (2026-08-24)

The static and runtime portions of the plan are now executed for W1 and W2.
The final findings and evidence boundaries are in
`research/notes/runtime-descriptor-construction.md`.

- Static xrefs identify the `01E7:382B` allocation/publication path, the
  `01D7:3808` world dispatch, and all five 512-record initializer ladders.
- `descriptor_static_report.py` emits a reproducible 512-record report and
  the primary/secondary MAP mutation ledger.
- Selector-safe DOSBox traces capture W1L1 and W2L1 construction checkpoints;
  both complete tables match their static initializer exactly (512/512).
- Selector-safe MAP traces capture the primary first-row mutation as
  `0x0001 -> 0x1001` (W1L1) and `0x005c -> 0x105c` (W2L1), both `+0x1000`.
- A normal W1L3 secondary-lifecycle probe reached the `01D7:48BB` transition
  wait but did not naturally call `3861`. A clearly marked debugger-only
  trampoline then called `3861` after descriptor publication;
  `394C/394F/3960/396D` confirmed the same `+0x1000` mutation on W1L3.
- A debugger-only call to `01F7:5C9D` captured its runtime write at
  `5CBE/5CC1`. Inputs `(x=0x45,y=0x123,CX=0xa55a)` produced the predicted
  offset `0x07c4` and changed `0x5001` to `0xa55a`.
- The controlled descriptor-flag pair was rerun: tile `0x02a`/flags `0x70`
  reaches `3DF1` with `AL=1`, while tile `0x02b`/flags `0x30` reaches `3DE4`
  with `AL=0`; each MAP patch is read back and restored.
- The four-record flag matrix (`0x10`, `0x50`, `0x70`, `0x30`) shows that the
  `3D02` positive branch requires both `0x20` and `0x40`.
- Static flag-use filtering closes the mechanical schema: `0x10` and `0x20`
  suppress the `3D02` y-minus-8 retry, `0x20` selects response polarity, and `0x40`
  selects the eight-pixel Y alignment. Only `3D19/3D31` call `5CC3`; direct
  `5C27` callers use its low-nibble condition result, while the transition
  block also consumes the full descriptor word left in `DX`.
- Transition diagnostics show `01F7:1AE6` repeatedly clears `DS:89EA` while
  `DS:880A` remains `4`; no unmodified `4BD8`/secondary call was reached.
  The relocation survey found no direct static caller of `01F7:5C9D`.
- The focused gate-write pass identified the launch chain `01D7:4B6E ->
  01F7:1AAA -> 01F7:1AE6 -> 01D7:4B73`: `1AE6` clears the scheduler gate
  before the main-loop test at `4BA4`. W1L3 and W2L3 both reproduce that
  sequence and then wait at `48BB`; the object-count gate is `4`, but the
  scheduler gate is `0`. The diagnostic now records stack return addresses
  and the identified writes to `8810/88BA/880A/89EA`.
- A longer diagnostic armed `19E6`/`1BC5`/`3AAF`, `199D`/`43D1`, `44DC`,
  `4BD8`, `3861`, and `5C9D`; none fired in the unmodified W1L3 launch. A
  debugger-only jump over the `48BB` compare/JZ also failed to reach those
  paths, so `DS:819E` is not the only asynchronous-transition barrier.
- The `DS:819E` owner is resolved: `01F7:F049` is the timer IRQ writer of
  `1`, while `0207:0014` and `0207:101F` clear/wait on the flag; `01D7:48B5`
  is the level-loop clear. W1L3 and W2L3 both capture the same runtime order.
  A normal run without a `48BB` breakpoint still times out at `48BB`, so the
  timer/transition sequence is not completing in this automated launch.
- Letting `48BB` run without a breakpoint reaches `48C2` and then `01F7:3062`
  (returning to `48C7`), but still not `4BA4`/`4BD8`/`3861`. Static callers of
  the timed-wait entry `0207:0002` include transition delay sites `4EDD/4EE6`;
  `0207:101F` is recursively called at `10A3`.
- The post-wait branch reaches `48E6`, where `DS:89E6=0` prevents the
  `4968` transition dispatch. Its static `0xffff` writers are `01D7:493E`
  and `01F7:4996/4AAC/92A9`; none fires during an unmodified level launch.
- An expanded W1L3 flow probe reaches `48E6 -> 493E -> 4968 -> 4BA4 ->
  4C43 -> 4CFC -> 4EA0 -> 4EAA` with `DS:89EA=0`, then times out without a
  state writer, secondary loader, or `5C9D` hit. This is the strongest current
  negative boundary for the normal post-wait restoration path.
- The segment-3 keyboard consumer at `01F7:F1A8` stores raw make bytes in
  `DS:88BA`; `0x03` is the `2` key. Queuing a real `KBD_2` after W1L3 launch
  reached `01D7:493E`, wrote `DS:89E6=0xffff`, and then reached `01D7:4968`.
  With a debugger-held scheduler gate, the same event reached
  `4BA4 -> 4BAE -> 4BD8 -> 3861`; the gate was reasserted at `4BA4` because
  the state callback decremented it at `44DC`. A separate forced-event run
  set `DS:89E6` at `48E6` and reproduced the downstream chain without input.
- Holding real `KBD_right` input from launch and after the `KBD_2` event did
  not fire `19E6`, `19A3`, `1A3D`, `199D`, or `44DC`; `DS:89EA` stayed zero
  and the unmodified run stopped at `4BA4`. This bounds the missing gate
  restoration more narrowly: it is not caused by the tested player-input
  path during automated W1L3 launch.
- NE relocation filtering resolves the direct state callers: `01F7:1BC4` and
  `3AB3` call `19E6`, while `01F7:43D0` calls `199D`. The `1BC4` path is an
  overlap test gated by `DS:8810`; `3AB3` follows tile IDs `0x0b/0x0c/0x0d`;
  and `43D0` is the player boundary check against `DS:81C4 + DS:81CC`.
  `199D` sets `DS:89EA=0xffff` and decrements `DS:880A`, making it the
  strongest level-boundary/death candidate. A controlled player-Y injection
  reached `3FF8` but not `43D0/199D` before the secondary-loader wait.
- The player-focused W1L1 trace confirms the normal overlap path at runtime:
  repeated `1BC4 -> 19E6` events occur under held `KBD_right`, but
  `DS:89EA` remains zero and no `19A3/1A3D/44DC/43D0/199D` writer fires.
  Downward and Space-input comparisons are also negative. This rules out the
  tested player collision/state path as the scheduler-gate restoration.
- A W1L1 controlled Y-boundary test now reaches `3FF8 -> 43D0 -> 199D ->
  19A3 -> 44DC`; `DS:89EA=0xffff` at `44DC`, and `DS:880A` drops from `4` to
  `3`. This proves the candidate gate-restoration path at runtime, while the
  forced Y value keeps it explicitly outside normal-gameplay attribution.
- A sustained unmodified W1L1 `KBD_right` run establishes a separate natural
  writer: after a 1409-frame warm-up it reaches `1BC4 -> 19E6 -> 1A3D`, where
  `DS:89EA` becomes `0xffff` at camera `x=1943` and `DS:880A` drops from `4`
  to `3`. Subsequent callbacks hit `44DC` and decrement the countdown. A
  longer natural-right run reaches `01D7:4BD8` with `DS:89EA=0xfea2`, proving
  the scheduler gate is naturally observed after this state-machine path.
  This does not turn the debugger-forced `43D0 -> 199D -> 19A3` boundary path
  into normal-gameplay evidence. Artifacts:
  `w1l1-transition-right-natural-warmup1409.json` and
  `w1l1-transition-right-natural-probe10.json`.
- Static MAP-writer audit finds the loader-return `33BF` whole-MAP pass, the
  23-call `16CE` tile-effect writer (two transient-event paths, one short
  animation path, and 20 state-machine calls), and the coordinate writers
  `339A`/`340A`.
  `339A`, `340A`, and `5C9D` have zero direct NE relocation callers; a normal-
  right W1L1 probe armed `5C9D` for 220 passes without a hit. Their indirect or
  runtime-generated caller status remains open.
- Descriptor-flag consumer audit finds 78 direct `5C27` calls and exactly two
  direct `5CC3` calls (`3D19`/`3D31`); no other descriptor `+2` consumer is
  present. `0x10` is mechanically an X-retry suppression bit, but no standalone
  gameplay name is justified.
- The targeted renderer/MAP-reader audit finds `3376`, `5C27`, `5CC3`, `20C8`,
  and `2CB2` masking raw MAP words to `0x01ff`; no identified direct reader
  interprets MAP bits 9..15. Selector-safe W1L1 collision traces confirm the
  tile-to-descriptor dataflow and the positive `0x70` versus negative `0x30`
  `3D02` branch pair. The curated evidence is in
  `research/notes/descriptor-collision-evidence.json`.
- Fresh current-worktree W1L1 controls isolate the X-retry gate: patched tile
  `0x028` (`0x10`) and `0x029` (`0x50`) both take
  `3D02 -> 3D1E -> 3D45 -> 3DD0 -> 3DE4`, while no-flag tile `0x160` takes
  `3D02 -> 3D1E -> 3D36 -> 3D40 -> 3D44`. The raw traces and hashes are in
  `research/notes/descriptor-collision-evidence.json`.
- Descriptor-only controls on unchanged tile `0x02e` complete the vertical
  bit matrix: `0x20` alone returns `3DE4`, `0x40` alone executes the retry and
  returns `3D44`, and `0x60` reaches the positive `3DF1` (`AL=1`) branch.
  The tracer now restores both the MAP cell and descriptor word after each
  controlled call; the new configuration is covered by the player-trace tests.
- A trajectory-matched descriptor patch closes the remaining motion confounder
  for `0x10`: on a descending natural W1L1 probe (`y=356`, vertical velocity
  `0x00009000`), tile `0x02a` changes from natural descriptor `0x70` to patched
  `0x10`; the post-query branch carries `0x10` through `3D1E/3D45/3DD0` and
  returns `3DE4`, whereas the unpatched trajectory reaches `3DF1`. The incoming
  `3D02` register is stale `0x70`, so the lookup record is the proof of the
  consumed value. Artifact and hash are in
  `notes/descriptor-collision-evidence.json`.
- Target-byte review of `5C27` fixes the low-nibble occupancy orientation:
  `AX.bit3/BX.bit3` pairs `11/10/01/00` test descriptor bits
  `0x02/0x01/0x04/0x08`; the zero-low-nibble case returns clear. This mapping
  is now emitted by the static report and construction evidence.
- Fresh W3L1/W4L1/W5L1 construction traces complete the runtime census: all
  five worlds match their static tables 512/512 and confirm each world’s MAP
  stride/height and `+0x1000` first-row mutation. W3 revealed duplicate
  `0xda/0xdb` compare cases; the static decoder now preserves the first
  reachable assignment and emits a duplicate-case report.
- The construction runner's `--secondary` switch now actually selects
  `01D7:3861`; a natural W1L3 run with a 128-lookup guard still timed out in
  the resource-lookup loop before that loader entry, strengthening the
  lifecycle-boundary negative result. The failure artifact is curated in
  `notes/descriptor-construction-evidence.json`.
- A less intrusive controlled-input W1L3 run held real `KBD_2` until the
  `01D7:493E` event writer, then waited 120 seconds with only secondary-loader
  breakpoints armed. The event fired, but the run remained in `0207:0002` at
  `0207:0023` and never reached `4BD8/3861`; this confirms the pending timer
  barrier after input consumption. It is controlled-input evidence, not
  normal-completion attribution, and is recorded in the construction evidence.
- The corrected timer-focused variant observed 16 `01F7:F049` IRQ hits before
  and 16 after `493E` but still failed to leave the pending wait; final CPU
  `0207:10C8` is inside the PIT-delay path. Segment-4 target decompilation now
  proves the `0002/101F/10A9` flag/PIT loop, ruling out absent IRQ delivery as
  the sole explanation and narrowing the gap to flag sampling/PIT sequencing.
- Follow-up target decompilation separates the timer IRQ's optional
  `DS:8952:DS:8954` callback from transition state: `382B` clears the pair,
  `36ED` handles `.\\Score.DAT`, `085E` temporarily masks the audio callback
  during SAM/TFX loading, and transition code disables the segment at
  `4853/496F`. The post-wait `FFFF:FFFF` sample is therefore audio/resource
  callback state, not missing `DS:89EA`/`DS:89E6` registration.
- A fresh unmodified W1L3 timer-state run sees `0x0000:0xffff` through the
  IRQ/PIT sequence and `0xffff:0xffff` at `48B5/48BB`, while `DS:89EA`,
  `DS:89E6`, and `DS:819E` are zero at the final wait test. The runtime pair
  ordering is curated as `timer_callback_pair_runtime_probe`.
- Target decompilation closes the object-state ambiguity beside `3D02`: the
  player byte at `+0x3a` is cleared on entry/rejection, set to `0x01` or
  `0xff` by the descriptor `0x20` branches, and consumed only by `3DF2` as a
  zero/nonzero gate for the integer-Y snap. The durable static report and curated
  evidence now call it a transient accepted-vertical-response latch rather
  than a surface-type field.
- Descriptor-constructor target decompilation now confirms that `01E7:382B`
  publishes and zero-fills the `DS:6582:DS:6584` `0x800`-byte table before
  allocating unrelated buffers; `01D7:3808` dispatches on `DS:85D8` and then
  clears a separate `DS:6D86:DS:6D88` `0x800`-byte buffer. A pending-audit stop
  at segment-5 `05D0` was decompiled as the interior of the stack-space probe
  `05CD`, not another MAP/descriptor routine.
- The indirect-capable MAP-writer audit was strengthened: `339A`, `340A`, and
  `5C9D` have zero NE relocation records of any source type and zero literal
  target-offset byte pairs in all file-backed segments. A caller can remain
  only runtime-constructed; the report and evidence JSON now retain the counts.
- Regenerated `research/build/entity-dispatch-table.json`; the complete test
  suite now passes (`59/59`).
- Transition-target decompilation now attributes the secondary loader: `4009`
  calls `365B` for initial MAP/assets, while `48B5/4BD8` calls `3861` only for
  zero-based third-level selectors `2/5/8/0x0b/0x0e`. Segment-3 decompilation
  also confirms the `DS:89EA` setters at `199D/19A3` and `19E6/1A3D`, plus
  the `1AE6` clearer.
- The new bounded writer-focus mode watches only `339A`, `340A`, and `5C9D`.
  W1L1 normal, W1L3 pre-gate, and W1L3 controlled post-gate runs all recorded
  zero hits; the W1L3 artifacts explicitly distinguish launch-barrier versus
  post-gate coverage.
- Additional 30-second held-right writer-focus windows on W2L1, W3L1, W4L1,
  and W5L1 also recorded zero hits for `339A`, `340A`, and `5C9D`. These are
  normal-input bounded negatives across all five world families, not a proof
  against a later event or a runtime-constructed caller.
- The call-form census found only two indirect CALL shapes in segment 3
  (`01F7:040F` and `01F7:0598`), both local `[BP-0x12]` stack-slot calls; no
  indirect target resolves to `339A`, `340A`, or `5C9D`.
- A controlled W1L1 archive variant now samples `01F7:8E4B` states 4, 6, and
  8. Each state queries five adjacent MAP cells, maps source IDs `0xc8..0xcc`
  through `DS:6986` to `0x78..0x7c`, and rewrites them through `16CE`; all
  fifteen writes are the expected `-0x50` low-ID replacement. Exact cells and
  hashes are in `state_machine_effect_probes` in the construction evidence.
- The regression suite now passes `61/61`, including schema checks for the
  call-form and state-machine evidence blocks.

## Optional follow-up plan

These questions do not block a faithful descriptor/MAP implementation. They
are retained as bounded follow-ups so extra tracing cannot silently promote a
debugger-only result into normal-gameplay attribution.

### O1. Give flag `0x10` a historical gameplay name

**Current fact:** the executable-level behavior is already proven: `0x10` (and
`0x20`) suppresses the `3D02` y-minus-8 retry; `0x20` additionally selects
vertical-response polarity, and `0x40` selects the eight-pixel Y alignment.

**Method:** enumerate every MAP tile ID carrying `0x10` in W1-W5, retain its
archive coordinates and neighboring descriptor words, then run paired
approach traces with descriptor values `0x10`, `0x20`, `0x40`, `0x30`, `0x50`,
and `0x70`. Capture `3D02`, `3D1E`, `3D36/3D45`, `3DD0`, `3DE4/3DF1`, and
`3DF2` with the incoming velocity and target-Y comparison.

**Acceptance:** a name is admissible only if the same directional/vertical
invariant appears in at least two worlds and two trajectories. Otherwise keep
the neutral implementation name `suppress_x_retry_bit_10`; do not label it
“floor,” “ceiling,” or “one-way” from tile placement alone.

**Artifact:** `research/build/optional/flag-0x10-matrix.json`, summarized in
the construction note with the existing collision-evidence hashes.

### O2. Resolve the natural W1L3-W5L3 secondary-loader trigger

**Current fact:** static control flow is complete: `4009 -> 365B` is initial
loading, while `48B5/4BD8` calls `3861` for selectors `2/5/8/0x0b/0x0e`.
The remaining uncertainty is which unmodified gameplay event reaches the
`DS:89EA`/`DS:89E6` setters before that gate.

**Method:** run unmodified W1L3, W2L3, and W3L3 sessions from the main-tree
DOSBox automation build with breakpoints only at `1BC4`, `3AB3`, `43D0`,
`19E6`, `199D`, `493E`, `4BA4`, `4BD8`, and `3861`. Record the first setter,
`DS:89EA` countdown, `DS:89E6` event, selector, and MAP mutation. Do not
patch `48BB`, `1AE6`, PIT flags, or scheduler globals in the natural matrix;
keep the existing controlled gate/trampoline traces as reachability controls.

**Acceptance:** one unmodified run reaches `3861` and its selector/MAP delta
matches the static prediction. If all three levels stop at the same timer or
pending-state boundary, record that shared boundary as the final natural-
lifecycle result and stop repeating equivalent timed waits.

**Artifact:** `research/build/optional/secondary-lifecycle-natural-matrix.json`
plus a curated entry in `descriptor-construction-evidence.json`.

### O3. Exclude runtime-generated callers of `339A`, `340A`, and `5C9D`

**Current fact:** all three helpers have zero NE relocation records, no literal
target-offset pairs, no indirect target hit in the segment-3 call-form census,
and no hits in bounded W1-W5 normal-input windows. This is strong negative
evidence, not a proof against a function pointer assembled from data.

**Method:** add a late-game/event-focused trace that arms the three helper
entries together with their far-return addresses and records caller return
segment:offset, MAP selector/offset, old/new word, and input registers. Run
normal input across all five worlds, then separately exercise known event and
transition paths without patching the helper targets.

**Acceptance:** a repeatable caller is promoted only when its return address and
MAP before/after pair recur in two runs. Otherwise classify the helpers as
implemented-but-unobserved runtime APIs and preserve the current negative
evidence; do not claim they are dead code.

**Artifact:** `research/build/optional/map-writer-runtime-caller-audit.json`.

### O4. Optional live confirmation of state-10 termination

**Current fact:** static `8E4B` decompilation already specifies state 10's five
MAP-derived effects, clearing of `object+0x18`, and publication at
`DS:8828/DS:882A`; states 4/6/8 are dynamically confirmed.

**Method:** reuse the controlled type-`0x20` archive variant, force entry at
state 9, and capture `3376`, `16CE`, `171C`, the terminal object fields, and
the two published coordinates. Treat instruction-limit or object-pool failure
as a tooling limitation, not evidence against the static path.

**Acceptance:** one trace showing five writes followed by object termination;
otherwise leave state 10 static-only and do not destabilize the proven state
4/6/8 artifacts.

**Artifact:** `research/build/optional/state-machine-state10.json`.

## Optional execution results

The four optional workstreams were executed with reproducible artifacts:

* **O1 — confirmed behavior, name unresolved.** The archive matrix contains
  `0x10` in W1/W3/W4/W5 (159/382/156/160 cells respectively) and none in W2.
  The controlled collision cases still establish only mechanical X-retry
  suppression, so the neutral implementation name remains in force.
  Artifact SHA-256: `7736888c52708f971c523d79e98e3eb250d460858ab22b49ea46e83f8bd97425`.
* **O2 — natural lifecycle boundary confirmed.** Fresh W1L3–W5L3 runs all
  reached the same timer/transition barrier and none reached `01D7:3861`.
  This closes the natural matrix without claiming an absent gameplay trigger.
  Artifact SHA-256: `392b0a437279ce34a8f0e6bf10f103d769d0543e4c8967afa3c32afe9279b123`.
* **O3 — bounded negative retained.** The event-focused W1L3 run produced no
  `339A`, `340A`, or `5C9D` hit; static call-form and existing W1–W5 windows
  agree. The helpers remain implemented-but-unobserved runtime APIs, while
  `16CE` retains its proven static/controlled callers.
  Artifact SHA-256: `e295704a6a7ef7291ca37dec09602ede22fcd751c70f578fc0ca8754f52303b4`.
* **O4 — tooling-limited.** The bounded forced-state-10 attempt hit the
  debugger instruction limit before a state-machine sample. State 10 therefore
  remains static-only: five MAP-derived writes, clear `object+0x18`, and
  publish `DS:8828/DS:882A`.
  Artifact SHA-256: `28d3cfd274aaebb79d323d97da03bc249f6a57139a69371aaf107c066a2fb37b`.

Remaining questions are limited to a user-facing gameplay name for flag
`0x10`, whether the statically identified `43D0 -> 199D` or `1BC4/3AB3 -> 19E6`
setter paths fire in an unmodified W1L3 launch, and whether `339A`, `340A`, or
`5C9D` has an indirect normal-gameplay caller. The W1L1 transition is
normal-gameplay evidence; debugger-controlled traces remain explicitly separate
from lifecycle attribution. The timer callback-pair audit no longer treats
`DS:8952:DS:8954` as an unresolved transition-registration channel.
