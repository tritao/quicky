# W1L1 session parity boundary

The native session emitter is `engine/apps/quiky-w1l1-trace.cpp`. It runs the
loaded W1L1 runtime through the existing `LevelSession -> Simulation ->
TraceClosedPlayerUpdate` boundary and emits one JSON sample per input tick.
The sample contains the callback pre/post `0x78` record, ordered collision
probes, callback state/global/effect writes, camera anchor, dependency order,
scheduler callbacks, active entities/effects, gameplay globals, and queued
events. It does not create a second gameplay path.

Reproduce the four-frame native startup comparison with:

```sh
cmake -S engine -B build/engine
cmake --build build/engine --target quiky-w1l1-trace
build/engine/quiky-w1l1-trace game/NESTLE.DAT W1L1.MAP \
  /tmp/w1l1-session.json --frames 4 --action-flags 0
python3 research/tools/w1l1_session_compare.py \
  --original research/build/player-frame-full-v1.json \
  --candidate /tmp/w1l1-session.json
```

The comparison established the following boundary on 2026-08-25:

| Field | Result | Evidence/status |
| --- | --- | --- |
| Player pre/post records | exact for all four samples | recovered `3F27` initializer plus `3FF8` callback; verified by the session comparator and existing `player_callback_parity.py` fixture |
| Input word and camera | exact for all four samples | DOS globals and native W1L1 anchor `(0,262)` |
| Scheduler order | first divergence: DOS `47E7,47E7,8D20,47E7,47E7,47E7,47E7`; native `47E7,47E7,47E7,47E7,47E7,47E7,8D20` | native pool append order is observable; family-specific insertion/stream ordering remains to be closed |
| Active leaf position/slot | diverges at sample 1 | DOS positions include `y=257/209/225` and slots `700/703`; native retains declaration anchors and slot `700`; `47E7` PRNG perturbation and animation cadence are explicitly unresolved in the static closure |
| WERBE position | diverges at sample 1 | DOS `(257,366)` versus native `(256,368)`; `8D20` initializer/position publication remains a separate object-family boundary |
| Probes/global writes/effects | DOS capture coverage gap | `player-frame-full-v1.json` does not publish those arrays; the comparator reports the gap instead of treating it as an empty equal list |

The first player-state mismatch was removed by applying the mechanically
recovered `01F7:3F27` initializer projection before the first native callback:
callback `+0x18=0x3FF8`, pool defaults, startup animation descriptor fields,
`+0x2C=-10`, `+0x2E=40`, `+0x30=10`, direction bytes, and contact gates. The
pool phase byte `+0x17=2` is published at the session boundary because the DOS
callback capture occurs after scheduler phase advancement.

This is the current stopping boundary for the first replay slice: player
differences are no longer hidden by reset-state initialization; remaining
failures name object scheduling/initializer or leaf PRNG/animation contracts.
