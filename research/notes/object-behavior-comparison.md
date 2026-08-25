# Frame contract comparison

`tools/object_behavior_compare.py` compares normalized DOSBox object traces
against the contracts recovered in this research track. It deliberately does
not emulate the whole callback. It checks exact, independently testable
contracts; when helper tracing is enabled it also consumes the `1C4D` carry
and directional `5C27` zero flag to validate the recovered type-`0x33` motion
state machine.

The comparator currently checks:

- descriptor initialization, timer countdown, signed relative sequence jumps,
  action selection, and mode-adjusted actions;
- the type-`0x34` `DS:85DA < 0x32` gate, strict proximity bounds, action word,
  and one-shot `1B5D/0FCF` action-chain entry;
- type-`0x33` callback persistence and frame-level motion fields; helper traces
  additionally validate the MAP-derived transition and state `0–3` branches.
- source-less high-effect `4B70` initialization, `4C74` cursor progression,
  sprite slots `611/612/613`, and terminal callback clear at cursor `31`.

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
| `type33-y400-state1-helper` | helper-derived state `1→2`, velocity, and descriptor transition | pass |
| `type33-tail-target-hit` | active `8AE5` target clear and cursor advance | pass |
| `type33-tail-target-miss` | active `8AE5` nonmatching target and cursor advance | pass |
| `w1l3-effect-60` | 1 factory + 31 `4C74` updates, slots `611/612/613`, terminal clear | pass |
| `w2l3-effect-60` | 1 factory + 31 `4C74` updates, W2 resource context | pass |

Traces without helper tracing remain conservative: they still check lifecycle,
descriptor, and type-specific observations, but do not infer an unrecorded MAP
decision as a mismatch.

High-effect traces use their separate event schema and can be checked with:

```sh
PYTHONPATH=research/tools python3 research/tools/object_behavior_compare.py \
  research/build/high-effect/w2l3-effect-60.json \
  --family high-effect
```

The comparator is intentionally callback-level. It does not yet compare the
renderer bitmap output or choose between `PUFF.BOB` and `PUFFW2.BOB`; those are
the scene-comparison inputs. The BOB decoder itself has now been cross-checked
between the C++ and Python renderers for every opaque pixel in both resources;
scene-level comparison remains open because the DOSBox captures contain other
active sprites and cloud layers.
