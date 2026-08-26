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

The input boundary is also closed at the word level. `F21B/F21C` returns
`DS:88BC | DS:8196` with bits `down=1`, `up=2`, `right=4`, `left=8`,
`alternate=0x10`, and `jump/confirm=0x20`. It has no edge detector or side
effect beyond the returned word. The callback, not the dispatch boundary,
provides the `+0x40` throttle and scripted-input suppression.

## Auditable contracts

| Address | Recovered name | Inputs / outputs | Player/global writes | Callees | Confidence / evidence |
| --- | --- | --- | --- | --- | --- |
| `01F7:3D02` | `apply_descriptor_vertical_correction_3D02` | `+3B`, integer `(x,y)`, descriptor; AL/ZF return sites `3DF1/3DE4/3D44` | `+08`, mode-zero `+0E`, `+3A` | `5CC3` | Confirmed from raw `3D02–3DF1` and descriptor matrix |
| `01F7:3DF2` | `snap_player_y_on_side_contact_3DF2` | `+3B/+3A`; ordered `(x-5,y)`, `(x+5,y)` probes | `+08 &= 0xFFF8` | `5C27` twice | Confirmed from raw and side trace |
| `01F7:3A1F` | `probe_player_side_clear_3A1F` | gate/mode; ordered side probes | `+3B=FF` only after both clear | `5C27` twice | Confirmed |
| `01F7:3971` | `probe_vertical_10px_3971` | `y-10-+72`, `x`; ZF | none | `1C92` | Confirmed |
| `01F7:3986` | `probe_vertical_step_3986` | `y-+72`, `x`; ZF | none | `1C92` | Confirmed |
| `01F7:5937` | `player_helper_5937` | auxiliary globals; far return at `5BED` | direct writes `60DA`, `4FF2`, `4FF8`, `4FFA`, `4FF6`; no direct player/`89EA` write | address-named external dispatches | Direct contract closed; indirect effects unresolved |
| `01F7:0E06` | `object_pool_factory_0E06` | requested callback AX, caller word DX | first-free object header; scheduler bank | `1036` | Allocator contract confirmed; family is caller-selected |
| `01F7:6328` | `contact_child_callback_6328` | pooled child state and fixed-point position | child `+0x32/+0x2E`; callback clear at terminal state | address-named effect subhelper | Mechanical partial; no solid collision read proven |
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

The negative-mode blocked-probe response at `41C1` writes `+0x3E=0x03E7`,
then `41CF` writes `+0x37=1`, `+0x0E=0`, and loads sequence `3186`. The
controlled low-Y run reaches this path, but its coordinate patch makes it
unsuitable for naming the response as a natural ceiling hit.

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
near return at `0F26`. Player-list membership, landing/jump detachment,
crushing, and the effect of off-camera removal remain open. Off-camera
removal is `A06F -> 1DEE`; attached-player behavior after that removal is
unresolved.

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
  and carry publication. `A06F -> 1DEE` closes object callback removal, but
  not attached-player behavior after culling.
- `1C6E`, `1C92`, `5C27`, `5CC3`, and `5DC3` have exact MAP addressing,
  masks, return-flag, and descriptor-word contracts. Descriptor gameplay
  classes remain address/data named; no one-way or floor/ceiling label is
  inferred from a bit alone.
- `5937 -> 386F -> 0442` closes the direct global writes and the temporary
  view-state publication. The table-selected callback at `0442` remains an
  explicitly unresolved indirect boundary because its target can only be
  identified from runtime selector state.
- `41C1 -> 41CF -> 3186`, the contact-effect path, and `5D38/5D60` close the
  known player/effect and animation writes. Sound-only or presentation-only
  continuations are retained as contracts and are not expanded further.

The reproducible export command is
`python3 research/tools/run_player_external_closure.py`; its verifier is
`python3 research/tools/verify_player_external_closure.py`. These exports use
Ghidra's raw-segment `x86:LE:16:Protected Mode` pipeline and NE relocation
records, not a separate x86 disassembler.

## Held-out trace matrix

The JSON ledger records SHA-256 for every ignored raw trace. The current set
covers standing, early and late jump input, apex/free fall, landing, side
rising/falling, the 5937 entry/return, 0E06 factory observation, descriptor
controls, and archival platform carry. The ceiling and one-way rows are
explicitly controls or open cases, not claimed parity victories.

Full parity is gated on comparing, per callback, all `0x78` bytes, ordered
probe coordinates/flags, relevant global writes, and created effect records.
The current C++ updater intentionally stops before grounded/contact/jump
orchestration, so the parity checker must fail closed until a C++ callback
trace exists.
