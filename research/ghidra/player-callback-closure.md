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
types and explicit fixed-point arithmetic, and leaves `player_helper_5937`
unresolved.

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
boundary without expanding unrelated game systems.

Current closure: 39 classified functions — 30 inline, 8 contract, and 1
unresolved — with 81 audited call-site edges. The closure includes the input
normalization boundary, action counters, horizontal integration, ascent and
falling paths, descriptor/side probes, jump initiation, landing/ceiling and
side-contact responses, animation selection/advancement, effect/sound
dispatch, transition helpers, view publication, and the common callback tail.

Static evidence for the main response paths is retained at these source
labels:

- jump initiation and animation selection: `ordinary_correction_42c9`;
- landing/grounded response: `grounded_contact_427f`;
- ascent/ceiling probe and response: `negative_mode_4323`;
- falling/side-contact response: `positive_mode_41e8`;
- transition path and common tail: `transition_block_4416` and
  `common_tail_4384`.

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

## Unresolved or partially resolved items

- `01F7:5937` (`player_helper_5937`) remains address-named. Its writes before
  the `DS:89EA` gate are not statically established and no dynamic evidence
  has closed that question.
- `01F7:0E06` (`object_pool_factory_0E06`) is a closed external contract for
  the contact factory's arguments and object/pool side effects, but the exact
  runtime object family remains only partially identified.
- `01F7:F21B/F21C` is retained as the input dispatch boundary. Ghidra emits
  malformed computed-jump warnings beyond the valid segment image; the raw
  bytes, function name, action-bit contract, and callback call site remain
  recorded without assigning a semantic target to the computed jump.

No enemy, boss, menu, renderer, or unrelated audio closure is included here.
