# Player callback follow-up

This is the focused follow-up for `01F7:3FF8` against
`QUIKY.EXE` SHA-256 `c9b2e59febd6fa0ea271bedf360459353f55c74444f026964b70988d6de1bca1`.
The machine-readable contract and evidence ledger is
[`player-contact-followup.json`](player-contact-followup.json); the
focused external-state closure is
[`player-external-state-closure.json`](player-external-state-closure.json); the
address-annotated C-like source remains
[`player-static-decomp.cpp`](../notes/player-static-decomp.cpp).

## Result

The landing and ordinary branch ordering is statically closed. The descriptor
helpers are closed mechanically, but their gameplay labels remain conservative:
descriptor bits `0x20` and `0x40` select response polarity/alignment, not a
proven one-way or floor/ceiling class. The reachable negative-mode path joins
the generic `41C1` response on a blocked vertical probe; a natural ceiling
trace is still required before naming that response “ceiling.”

The contiguous protected-mode listing for `3FF8–44F7`, together with the
unpatched W1L1 trace, closes an important attribution gap. The negative path
can reach `41C1` after the first probe blocks (`4326`), after the integrated
velocity becomes nonnegative at the ordinary apex (`4356`), or after the
second probe blocks (`4368`). The observed frame 256 event follows the apex
join: it has no `4363/4366` second-probe watches. Therefore `41C1/41CF`
reachability alone is not evidence of a ceiling. The static and trace record
is [`player-negative-mode-second-probe-v1.json`](../evidence/player-dos-parity/player-negative-mode-second-probe-v1.json).

The first unpatched landing/contact matrix now closes one authored descriptor
case. In W1L1, the route crosses tile `0x28` (tile 40) from below during the
jump and later descends onto the same cell at `(x=1049,y=395)`. The landing
callback runs `3D02 -> 5CC3`, reads descriptor word `0x0010` for MAP word
`0x2028`, and leaves the player in mode `0` with vertical velocity settled.
This is evidence that this authored case is traversable from below and
supports a descending contact response; it does not rename the generic
`41C1/41CF` event on the same route as a ceiling hit, nor generalize the
result to descriptor bits `0x20`/`0x40`. See
[`player-natural-flagged-contact-v1.json`](../evidence/player-dos-parity/player-natural-flagged-contact-v1.json).

A second unpatched route reaches the adjacent authored tile `0x29` (tile 41),
whose descriptor is `0x0050`. The player again crosses the structure upward,
then lands at `(x=1095,y=372)` after `3D02 -> 5CC3`; the descriptor word is
returned in `DX=0x0050`, and the post-contact mode and vertical velocity are
`0`/`0`. This confirms the aligned descriptor case as a natural landing
contract, while keeping the effect of `0x40` separate from the authored tile
geometry. Evidence:
[`player-natural-tile41-contact-v1.json`](../evidence/player-dos-parity/player-natural-tile41-contact-v1.json).

The input boundary is also closed at the word level. `F21B/F21C` returns
`DS:88BC | DS:8196` with bits `down=1`, `up=2`, `right=4`, `left=8`,
`alternate=0x10`, and `jump/confirm=0x20`. It has no edge detector or side
effect beyond the returned word. The callback, not the dispatch boundary,
provides the `+0x40` throttle and scripted-input suppression.

A targeted unpatched W1L2 rerun is archived as
`research/build/traces/player-w1l2-natural-ceiling-targeted-v2.json`. At the
candidate transition it observes `4326 -> 4356 -> 41C1`; `4368` is absent.
The player is at `(86,428)` with `vy=-0x2000` before the response. This is a
second ordinary-apex negative control, not natural-ceiling evidence. The
remaining runtime target is a trajectory that reaches `4326` or `4368`
without the `4356` apex join.

## Auditable contracts

| Address | Recovered name | Inputs / outputs | Player/global writes | Callees | Confidence / evidence |
| --- | --- | --- | --- | --- | --- |
| `01F7:3D02` | `apply_descriptor_vertical_correction_3D02` | `+3B`, integer `(x,y)`, descriptor; AL/ZF return sites `3DF1/3DE4/3D44` | `+08`, mode-zero `+0E`, `+3A` | `5CC3` | Confirmed from raw `3D02–3DF1` and descriptor matrix |
| `01F7:3DF2` | `snap_player_y_on_side_contact_3DF2` | `+3B/+3A`; ordered `(x-5,y)`, `(x+5,y)` probes | `+08 &= 0xFFF8` | `5C27` twice | Confirmed from raw and side trace |
| `01F7:3A1F` | `probe_player_side_clear_3A1F` | gate/mode; ordered side probes | `+3B=FF` only after both clear | `5C27` twice | Confirmed |
| `01F7:3971` | `probe_vertical_10px_3971` | `y-10-+72`, `x`; ZF | none | `1C92` | Confirmed |
| `01F7:3986` | `probe_vertical_step_3986` | `y-+72`, `x`; ZF | none | `1C92` | Confirmed |
| `01F7:5937` | `player_helper_5937` | `DS:85DA`, `DS:60D8/60DA`, score/health/lives and auxiliary globals; far return at `5BED` | direct writes `60DA`, `4FF2`, `4FF8`, `4FFA`; no direct player/`89EA` write | `386F -> 0442 -> address-named loaded callback` | Direct body closed; five ordinary level-start targets bounded non-simulation; other runtime targets unresolved |
| `01F7:0E06` | `object_pool_factory_0E06` | requested callback AX, caller word DX | first-free object header; scheduler bank | `1036` | Allocator contract confirmed; family is caller-selected |
| `01F7:6328` | `contact_child_callback_6328` | pooled child state and fixed-point position | child `+0x32/+0x2E`; callback clear at terminal state | address-named effect subhelper | Static lifetime confirmed by 40-sample protected-mode trace; no solid collision read proven |
| `01F7:F21B/F21C` | `read_normalized_action_bits_F21B__input_dispatch_F21C` | `DS:88BC`, `DS:8196` | none | none | Word contract confirmed; malformed computed tail stays address-named |
| `01F7:A075` | `platform_player_overlap_A075` | platform geometry/player coordinates | `DS:5006=-1` on strict overlap | none | Native platform evidence |
| `01F7:A0B2` | `platform_publish_player_carry_A0B2` | accepted overlap/platform motion | `DS:8812`, `DS:8816`, platform `+59/+5A` | none | Native platform evidence |

## Contact ordering and writes

The player callback’s relevant ordering is:

```text
3FF8
  5937 -> DS:89EA transition gate
  648E -> 6484 -> 3A8A
  input normalization / +39 override / suppression / +40 counter
  427F grounded response
      3D02 -> 3DF2 -> vy=0 -> mode=0 -> animation 3156
  42B4 ordinary response
      3A1F -> 3DF2 -> 3D02 -> 3971 -> jump writes
  4323 negative response
      3986 -> integrate -> 3986 -> 41C1 or 4384
  41E8 positive response
      side probes -> 1C6E -> 3D02 -> integrate -> 3A1F -> 427F
  4384 common tail
      38CA -> 38EC/4519 -> 3AB9 -> 5D60 -> 3A62 -> camera/timers
```

Static landing writes are `+0x0E=0`, `+0x37=0`, and `+0x36=1`, with
animation loader writes to `+0x12/+0x1E/+0x20/+0x22/+0x24` when the idle gate
loads sequence `3156`. A natural landing callback diff additionally writes
`+0x07/+0x08/+0x0F/+0x10/+0x3E/+0x45/+0x46`; these are recorded as complete
callback observations, not incorrectly attributed to the landing helper.

A dense player-scoped execute trace at `5D60` closes the adjacent animation
boundary. On the natural landing callback, the falling state is replaced by
sequence `3156` (`+0x1E/+0x20=4`, cursor `3158`) before `5D60` runs; the watch
sees delay `4` and the callback returns with delay `3`. On each following
ordinary no-input callback, the callback again loads `3156` while `DS:4FEE <
0xD2`, then `5D60` returns the delay to `3`. This explains the former
late-release candidate mismatch and is implemented in the C++ no-input branch
at the statically recovered `3AB9` load site. It does not close missing probe
arrays or external effect data in the archived replay fixtures.

The shared response at `41C1` writes `+0x3E=0x03E7`, then `41CF` writes
`+0x37=1`, `+0x0E=0`, and loads sequence `3186`. The unpatched W1L1 trace
confirms those writes for the ordinary apex join, not for a ceiling. A
natural ceiling still must be captured with its `4326` or `4368` predecessor;
the prior controlled low-Y run is not sufficient for that claim.

## Moving platforms

`A075` accepts only `object_x < player_x < object_x+width` and
`object_y <= player_y < object_y+12`, then `A0B2` publishes fixed-point carry:

```text
DS:8816 = platform-to-tracked-X fixed-point correction/carry
DS:8812 = platform_y - player_y + 1       // fixed point
```

The player callback consumes and clears `DS:8812` on entry and consumes and
clears `DS:8816` in the horizontal/contact path. This closes the publication
and consumption contract for subpixel carry. The static main-loop call-site
pairs also establish `0E96` before `0FA2`, so a platform callback in the phase
bank can publish carry before the later nonzero-state dispatch invokes the
player callback. A hash-pinned W4L1 follow-up now captures the complete
`0x78` player record before and after three successive `3FF8` calls and the
near return at `0F26`. Player-list membership, landing/jump detachment, and
crushing remain open. Off-camera removal is `A06F -> 1DEE`; two explicit
forced-latch runs now observe that chain followed by the next `3FF8` callback
with unchanged player position, velocity, and mode. This closes the ordinary
next-tick consequence of the lifetime mutation, but not retail attached-player
behavior after culling.

A second hash-pinned W4L1 experiment places the live platform at a controlled
player-relative position before `9DC7` and holds native `KBD_space+KBD_up`
through the first combined platform/player handoff. The sample records
`keyboard_flags=0x22`, `DS:5006=FFFF`, `DS:8816=1`, and
`DS:8812=0xFFF80001` before `3FF8`; the complete record then changes
`+0x37` from `0` to `0xFF`, while later samples have zero carry and remain in
the ascending mode. This confirms the input/carry ordering and a controlled
jump-detachment transition. It is not a retail-geometry landing trace, so
natural landing, inherited velocity, crushing, and culling remain explicit
boundaries. Evidence: `research/evidence/entity-platform-player-jump-detach-v1.json`
(`9b60c6a74d0abf47d203541587945254da15172e077e3bf98fbf7f35cac46387`).

## Focused external-state expansion

The companion ledger records the additional static contracts without widening
the decompilation to unrelated game code:

- `0E96` walks pooled callbacks in phase order and calls its `0FDC` tail;
  `0FA2` is a later nonzero-state pass. NE relocation records verify the three
  `0E96`/`0FA2` call-site pairs used by the main loop.
- `A075`, `39FE`, and `A0B2` close platform overlap, tracked-position reads,
  and carry publication. `A06F -> 1DEE` closes object callback removal. The
  controlled cull evidence also observes the next `3FF8` tick without a
  movement/mode change; retail attached-player behavior after culling remains
  address-qualified and unresolved.
- `1C6E`, `1C92`, `5C27`, `5CC3`, and `5DC3` have exact MAP addressing,
  masks, return-flag, and descriptor-word contracts. Descriptor gameplay
  classes remain address/data named; no one-way or floor/ceiling label is
  inferred from a bit alone.
- `1BD1` is now closed as the transition branch's final CF-only descriptor
  probe: `CX` is the Y offset, `DX` is the X offset, and the selected
  low-nibble bit is `0x1/0x2/0x4/0x8` by the `(Y&8,X&8)` quadrant. It gates
  the `44CA` position integration versus the `44DC` transition decrement.
- `5937 -> 386F -> 0442` closes the direct global writes and the temporary
  view-state publication. Automatic target-inclusive watches now bound the
  ordinary W1L1-W5L1 startup callbacks to unchanged dispatched objects,
  complete player records, and recovered original-DS simulation words. The
  generic `0442 -> 0598` target remains address-named for later/action-selected
  records.
- `41C1 -> 41CF -> 3186`, the contact-effect path, and `5D38/5D60` close the
  known player/effect and animation writes. Sound-only or presentation-only
  continuations are retained as contracts and are not expanded further.
- The `01F7:199D -> 01E7:0CE3` boundary is now similarly closed. `0CE3`
  calls the runtime stack guard `0227:05CD`, conditionally writes
  `DS:504C=0x18`, and calls `01E7:33D5` with `BX=8/CX=0x40/DX=0x3F`.
  `33D5` writes only `FFFF:2FE9/2FEB/2FEC`; `05CD` only updates the stack
  watermark on its fast path and branches to the runtime failure handler on
  exhaustion. Neither target writes the player record, MAP/descriptor state,
  scheduler state, or callback simulation globals. The complete static note
  and independent A/B export evidence are
  [`player-transition-effect-static-v1.json`](../evidence/player-dos-parity/player-transition-effect-static-v1.json)
  and [`player-transition-effect-static-decomp.cpp`](../notes/player-transition-effect-static-decomp.cpp).
- A focused unpatched W1L1 trace now closes the ordinary-gameplay use of the
  transition branch: natural `6DC4 -> 19E6` damage is followed in one frame by
  `3FF8 -> 4416 -> 44DC`. This proves `4416–44FE` is required for damage/death
  gameplay, while leaving any separate level-exit writer and the later death
  recovery scheduler outside this result. Evidence:
  [`player-transition-writer-callback-v1.json`](../evidence/player-dos-parity/player-transition-writer-callback-v1.json).
- The selector launch itself is now statically bounded at `01D7:4B18 ->
  01D7:48B5-48C0`: `4BA4` clears `DS:819E` and waits for the timer IRQ at
  `01F7:F049` to publish `1`. The same gate is polled by `0207:0002` and
  `0207:101F`; those helpers are now included in the reproducible Ghidra
  listing. Current platform attempts still stop at this harness/runtime
  boundary, not because the platform callback is absent.

The reproducible export command is
`python3 research/tools/run_player_external_closure.py`; its verifier is
`python3 research/tools/verify_player_external_closure.py`. These exports use
Ghidra's raw-segment `x86:LE:16:Protected Mode` pipeline and NE relocation
records, not a separate x86 disassembler.

The scheduler/carry edge now also has exact Ghidra instruction listings,
independently compared between project A and project B, for `0E96–0F38`,
`0FDC–1032`, `1036–1066`, `1DEE–1E03`, `9DC7–9EC6`, `A075–A0B1`, and
`A0B2-A100`, and the transition helper `1BD1-1C10`.
This preserves the phase predicates, bank toggles, callback argument setup,
registration terminator, culling writes, strict overlap inequalities, the
platform state-machine probes, and fixed-point carry publication that the
C-like export can obscure. The address-annotated representation is
[`platform-static-decomp.cpp`](../notes/platform-static-decomp.cpp). The
runner and range list are recorded in `player-external-state-closure.json` so
the listing cannot silently drift from the ledger.

## Held-out trace matrix

The JSON ledger records SHA-256 for every ignored raw trace. The current set
covers standing, early and late jump input, apex/free fall, landing, side
rising/falling, the 5937 entry/return, 0E06 factory observation, descriptor
controls, and archival platform carry. The ceiling and one-way rows are
explicitly controls or open cases, not claimed parity victories.

Full parity is gated on comparing, per callback, all `0x78` bytes, ordered
probe coordinates/flags, relevant global writes, and created effect records.
The current C++ updater contains the recovered grounded/contact/jump
orchestration, but platform retail-geometry contacts and the no-descriptor
fallback remain fail-closed boundaries until their native traces are captured.
