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

For a DOS capture that publishes the runtime-generated leaf ring, replay the
ring before the first camera stream. The callback sample publishes the cursor
after the six initial leaves have consumed two bytes each, so subtract twelve
modulo 256 when supplying the native pre-stream cursor:

```sh
ring=$(jq -r '.events[0].samples[0].globals.leaf_prng_ring_hex' \
  /tmp/quiky-leaf-prng-player.json)
post=$(jq -r '.events[0].samples[0].globals.leaf_prng_index' \
  /tmp/quiky-leaf-prng-player.json)
pre=$(( (post - 12) & 255 ))
build/engine/quiky-w1l1-trace game/NESTLE.DAT W1L1.MAP \
  /tmp/quiky-leaf-prng-native.json --frames 4 --action-flags 0 \
  --leaf-prng-index "$pre" --leaf-prng-ring-hex "$ring"
python3 research/tools/w1l1_session_compare.py \
  --original /tmp/quiky-leaf-prng-player.json \
  --candidate /tmp/quiky-leaf-prng-native.json
```

The comparison established the following boundary on 2026-08-25:

| Field | Result | Evidence/status |
| --- | --- | --- |
| Player pre/post records | exact for all four samples | recovered `3F27` initializer plus `3FF8` callback; verified by the session comparator and existing `player_callback_parity.py` fixture |
| Input word and camera | exact for all four samples | DOS globals and native W1L1 anchor `(0,262)` |
| Scheduler order | exact for all four samples after the focused closure | Ghidra `1CDA -> 1E04` edge scans plus the native callback pool establish descending region-X/ascending region-Y append order; camera-anchored C++ streaming now follows it |
| WERBE position | exact for all four samples | Ghidra `8BC2` initializer projection `(x+1,y-2)` now publishes DOS `(257,366)`, slot `607`, subtype `1` |
| Active leaf position/slot/animation state | exact for all four fresh seeded samples | Ghidra listing `4727`, `47E7`, `5D38`, and `5D60`; C++ consumes the captured ring in stream order, applies the signed table choice, fixed-point fall, delay countdown, loop-marker cursor, and slot publication |
| Probes/global writes/effects | DOS capture coverage gap | `player-frame-full-v1.json` does not publish those arrays; the comparator reports the gap instead of treating it as an empty equal list |

The first player-state mismatch was removed by applying the mechanically
recovered `01F7:3F27` initializer projection before the first native callback:
callback `+0x18=0x3FF8`, pool defaults, startup animation descriptor fields,
`+0x2C=-10`, `+0x2E=40`, `+0x30=10`, direction bytes, and contact gates. The
pool phase byte `+0x17=2` is published at the session boundary because the DOS
callback capture occurs after scheduler phase advancement.

The fresh seeded replay now reports `OK: W1L1 session parity fields` with zero
field mismatches. Its remaining coverage gaps are expected: the focused DOS
capture suppresses later pool/scheduler snapshots and does not publish the
player probe/effect arrays on those samples. The old unseeded fixture remains
diagnostic only for leaf bytes because `DS:646C` is process-generated and is
not present in that archived capture.

The diagnostic comparison reports those omissions explicitly. For a held-out
acceptance fixture, add `--require-complete`; that mode returns failure when
either side omits a comparable player, object, probe, global-write, or effect
field, so a short capture cannot accidentally qualify as exact parity.

The callback-level command accepts the same switch:

```sh
python3 research/tools/player_callback_parity.py \
  --original TRACE.json --archive game/NESTLE.DAT --map W1L1.MAP \
  --binary build/engine/quiky-player-trace --require-complete
```

The first seeded leaf replay is recorded in
[`w1l1-leaf-prng-replay-derivation-v1.json`](../evidence/player-dos-parity/w1l1-leaf-prng-replay-derivation-v1.json).
It supplies the six observed initializer byte pairs only as an explicit
replay input; the native default remains unchanged.

The W1L1 collectible callback boundary is now implemented from the focused
`8D31 -> 393C` decompilation. Native tests use the recovered signed player
intervals, 16-pixel object-Y alignment, `DS:89EA` gate, and strict edge
comparisons; subtype fixtures cover ammo, health, invulnerability, and puzzle
letter writes. These are static/controlled-contract checks, not a claim of
full natural-DOS frame parity for collectible presentation effects.
