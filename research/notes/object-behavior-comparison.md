# Frame contract comparison

`tools/object_behavior_compare.py` compares normalized DOSBox object traces
against the contracts recovered in this research track. It deliberately does
not emulate the whole callback. It checks exact, independently testable
contracts and leaves MAP-dependent type-`0x33` motion as an observation until
the pre-state produced by `1B77/1C4D` is modeled.

The comparator currently checks:

- descriptor initialization, timer countdown, signed relative sequence jumps,
  action selection, and mode-adjusted actions;
- the type-`0x34` `DS:85DA < 0x32` gate, strict proximity bounds, action word,
  and one-shot `1B5D/0FCF` action-chain entry;
- type-`0x33` callback persistence and the presence of frame-level movement /
  descriptor observations without inventing a MAP response model.

Example:

```sh
PYTHONPATH=research/tools python3 research/tools/object_behavior_compare.py \
  research/build/object-behavior/descriptor-34-sequence-s8.json \
  --output research/build/object-behavior/descriptor-34-comparison.json
```

The current DOSBox evidence passes the recovered contracts:

| Trace | Coverage | Result |
| --- | --- | --- |
| `descriptor-33-sequence-s8` | 7 steady descriptor frames, 7 type-0x33 movement observations | pass |
| `descriptor-34-sequence-s8` | 7 descriptor frames, one resolved sequence expiry | pass |
| `descriptor-34-force-cursor-s8` | 7 resolved sequence expiries | pass |
| `targeted-type34-state31-accepted` | 7 active-gate frames, 7 predicted proximity frames, action chain on first transition | pass |
| `targeted-type34-state32-accepted` | 7 inactive-gate frames | pass |
| `helper-33-s48-mapprobe` | 47 type-0x33 movement observations | pass |

The next comparator extension should consume the type-`0x33` MAP probe result
and return flags, then model the pre-state update that changes `+0x0A`,
`+0x2F`, and the directional motion branch. Until then, the comparator's
conservative behavior is intentional: an unresolved MAP branch is not
reported as a false mismatch.
