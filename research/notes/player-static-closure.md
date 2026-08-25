# Player static closure

This is the primary static closure for the player object in segment 3
(`01f7`). It follows one callback from input through movement, MAP and
descriptor lookup, collision correction, and final state writes. The exact
offsets are preserved in every name so this note can be checked against the
raw segment image and NE relocations.

The machine-readable inventory is
[`player-static-closure.json`](../ghidra/player-static-closure.json), and the
direct pseudocode is
[`player-static-decomp.cpp`](player-static-decomp.cpp).

## Reproduce the result

The pinned input is `game/QUIKY.EXE`, SHA-256
`c9b2e59febd6fa0ea271bedf360459353f55c74444f026964b70988d6de1bca1`. Rebuild
the segment images and run the repository's pinned Ghidra import/decompile
pipeline with:

```sh
python3 research/tools/run_player_callback_baseline.py \
  --output /tmp/quiky-player-callback-baseline
python3 research/tools/ne_relocs.py game/QUIKY.EXE \
  --segment 3 --start 0x3376 --end 0x659c
python3 research/tools/verify_player_static_closure.py \
  --segment /tmp/quiky-player-callback-baseline/segments/QUIKY_SEG03.bin
```

The checked segment image is 62,013 bytes, SHA-256
`8d6c9554a0715935cf50b0fd6f1624941eaaf202cd5ed8f060f8f34f69f790cf`.
The existing Ghidra scripts can also regenerate a focused decompiler export:

```sh
analyzeHeadless research/build/ghidra-project QuikySegments \
  -process QUIKY_SEG03.bin -noanalysis \
  -postScript DumpQuikyDecomp.java /tmp/quiky-player-decomp \
  -scriptPath research/tools -commit 'Dump player closure'
```

The Ghidra output is not the canonical source of names. The executable
contains unresolved NE far-call relocations, and the input routine ends in a
byte table that the generic protected-mode x86 importer may decode as a
computed jump. The Ghidra project/export, relocation listing, JSON manifest,
and pseudocode together are the reproducible decompilation.

## Boundary and call-convention map

Offsets are relative to segment 3 and are hexadecimal. `near` means a normal
segment-3 `RET`; `far` means `lret` and a relocated `lcall`/far entry. A
`Flags` return means callers consume x86 flags directly; in the pseudocode,
`Flags::zf` is the recovered ZF and `Flags::cf` is the recovered CF.

| Offset | Renamed function | Convention and signature | Return contract |
| ---: | --- | --- | --- |
| `3376..3399` | `map_tile_id_lookup_16px` | far; `uint16_t(y, x)` | `AX = MAP word & 0x01ff` |
| `3971..3985` | `probe_vertical_10px_clear` | near; `Flags(Player*)` | flags from relocated `1c92` with `y - 0x0a - +0x72` |
| `3986..3997` | `probe_vertical_step_clear` | near; `Flags(Player*)` | flags from relocated `1c92` with `y - +0x72` |
| `3998..39fd` | `probe_forward_surface_clear` | near; `Flags(Player*)` | ZF=1 means the forward probes are clear |
| `3a1f..3a61` | `test_player_side_contact` | near; `Flags(Player*)` | side latch and x-5/x+5 probe result |
| `3a62..3a89` | `apply_action_contact_side_effect` | near; `void(Player*)` | none |
| `3a8a..3ab8` | `dispatch_special_tile_contact` | far; `void(Player*)` | none |
| `3ab9..3d01` | `integrate_horizontal_player_motion` | near; `void(Player*)` | none |
| `3d02..3df1` | `apply_descriptor_vertical_correction` | near; `Flags(Player*)` | early path has ZF=1; response path sets AL=1 |
| `3df2..3e2f` | `snap_player_y_on_side_response` | near; `void(Player*)` | none |
| `3e30..3e40` | `commit_external_player_y_and_gate` | far; `void(Fixed16 eax)` | resolves player through `DS:881a`, writes `+38=0xff` and `+06=EAX` |
| `3e41..3f26` | `compute_view_delta_after_player_motion` | near; `ViewDelta(Player*)` | EAX/EBX are consumed by `20af` |
| `3f27..3ff7` | `initialize_player_record` | near; `void(Player*)` | none |
| `3ff8..44fe` | `update_player_record` | near; `void(Player*)` | returns at `4415` or `44fe` |
| `5c27..5c9c` | `map_descriptor_quadrant_occupancy` | far; `Flags(y, x)` | ZF=1 means selected low-nibble bit is clear |
| `5cc3..5cff` | `read_map_descriptor_word` | far; `uint16_t(y, x)` | `DX = descriptor word` |
| `6370..6483` | `resolve_left_contact_tile` | near; `Flags(Player*)` | CF only for matching negative-mode contact |
| `6484..648d` | `resolve_left_contact_wrapper` | far; `Flags(Player*)` | forwards `6370` flags |
| `648e..659b` | `resolve_right_contact_tile` | far; `Flags(Player*)` | CF only for matching negative-mode contact |
| `f17f..f1a7` | `keyboard_irq1_ring_producer` | near; `void()` | writes the scan-code ring |
| `f1a8..f21a` | `poll_keyboard_ring_to_action_bits` | far; `void()` | updates `DS:88bc` |
| `f21b..f231` | `read_normalized_action_bits` | far; `uint16_t()` | `DS:88bc | DS:8196` |

`5937` is an external far call at callback entry. Its local state effects are
not needed to recover the action word, so it is retained as the mechanical
name `poll_auxiliary_input_state`. The callback's direct action source is
`DS:656c`, and when `DS:85da <= 0` the callback also calls `f21c`, the
normalized-input helper immediately after `f21b`, before storing the low byte
at `DS:4ff0`.

The call ABI is not interchangeable: local helpers use segment-3 `CALL rel16`
and `RET`, while MAP/descriptor, input, contact, animation, and view targets
use relocated `LCALL` and `LRET`. `6370` is a near helper whose CF is returned
through the far `6484` wrapper; the callback tests that CF immediately after
both `648e` and `6484`. `5c27` and `5cc3` are far helpers, so their return
flags/value are consumed at the relocated call sites. No near/far thunk or
selector conversion is implied by the pseudocode: the selector/offset pair
is the NE relocation's runtime target.

## Player record

The callback receives `ES:DI`. `DS:881a` retains the pool offset; the runtime
player selector is `0x027f`, and the record stride is `0x78` bytes. Position
and velocity are signed 32-bit 16.16 values. The high word is the integer
coordinate used for MAP probes; low words are retained during integration.

| Offset | Recovered name | Type | Static use |
| ---: | --- | --- | --- |
| `00` | `action_word` | `uint16_t` | action bits; written after input suppression and again at `41af` |
| `02` | `x_fixed_16_16` | `int32_t` | X integration |
| `04` | `x_pixel` | `int16_t` | probe coordinate |
| `06` | `y_fixed_16_16` | `int32_t` | Y integration |
| `08` | `y_pixel` | `int16_t` | probe and correction coordinate |
| `0a` | `x_velocity_fixed` | `int32_t` | horizontal integration; also the source operand in `3d02` response arithmetic |
| `0e` | `y_velocity_fixed` | `int32_t` | vertical integration |
| `12` | `status_word` | `uint16_t` | `&= 0x0fff`; timer can OR `0x8000` |
| `18` | `callback_offset` | `uint16_t` | initialized to `3ff8` |
| `28` | `direction_byte` | `uint8_t` | action `0x04` -> 1, action `0x08` -> `0xff` |
| `29` | `motion_direction_byte` | `uint8_t` | derived from `x_velocity + DS:8816` |
| `2a` | `action_counter_or_gate` | `uint8_t` | alternate/release branch gate |
| `2b` | `contact_scratch` | `uint8_t` | cleared before descriptor correction |
| `2c` | `state_2c` | `int16_t` | initialized to `-10` |
| `2e` | `vertical_step_or_direction` | `int16_t` | receives `+/-0x72` or `-(0x72 >> 1)` |
| `30` | `state_30` | `int16_t` | initialized to `+10` |
| `32` | `callback_state_word` | `uint16_t` | cleared by contact resolution |
| `34` | `timer_word` | `uint16_t` | decremented in common tail |
| `36` | `animation_state` | `uint8_t` | initialized `0xff`; grounded continuation sets 1 |
| `37` | `signed_callback_mode` | `int8_t` | branch selector: zero, positive, negative, or `0xff` |
| `38` | `collision_gate` | `uint8_t` | contact helper gate; cleared in common tail |
| `39` | `transition_pending` | `uint8_t` | converted to mode 1 and action `0x22` |
| `3a` | `vertical_response_latch` | `int8_t` | descriptor response: 0, 1, or `0xff` |
| `3b` | `side_response_latch` | `uint8_t` | side helper writes `0xff`; reset path clears it |
| `3e` | `reset_or_death_timer` | `uint16_t` | positive-path counter; reset values `0x03e7/0x03e8` |
| `40` | `action_frame_counter` | `uint16_t` | clear if no action `0x22`, then increment |
| `44` | `saved_y_fixed` | `int32_t` | saved before the input/motion block |
| `48` | `saved_x_fixed` | `int32_t` | saved before the input/motion block |
| `4a` | `view_anchor_x` | `int16_t` | X anchor used only by the `3e41` camera delta helper |
| `4c` | `acceleration_fixed` | `int32_t` | `0x2800` |
| `50` | `positive_y_acceleration_fixed` | `int32_t` | `0x2800` |
| `54` | `friction_fixed` | `int32_t` | `0x2000` |
| `58` | `negative_y_acceleration_fixed` | `int32_t` | `0x2000` |
| `5c` | `horizontal_speed_cap_fixed` | `int32_t` | `0x18000` |
| `60` | `positive_y_speed_cap_fixed` | `int32_t` | `0x40000` |
| `64` | `negative_y_speed_fixed` | `int32_t` | `0xfffb6000` (`-0x4a000`) |
| `72` | `vertical_step_pixels` | `uint16_t` | `0x28` (`40`) |

The initializer writes these fields and clears `DS:4fec`, `DS:8812`,
`DS:8810`, `DS:4fe2`, and `DS:4fe6`; it initializes `DS:4fee=0x00d3`.
It zeros both velocity dwords, mode, timer, action, and the vertical latch,
masks the incoming Y dword with `0xfff00000`, sets `+3b=1`, `+38=0xff`,
`+36=0xff`, `+28=1`, and `+29=1`, and installs callback `+18=0x3ff8`.

## Input and top-level branch order

The keyboard path is:

```text
IRQ1 f17f  ->  scan-code ring  ->  f1a8  ->  DS:88bc
                                           + DS:8196
                                           -> f21b/f21c -> callback action
```

The make/break mappings are `left=0x08`, `right=0x04`, `down=0x01`,
`up=0x02`, `alternate=0x10`, and `jump/confirm=0x20`. The callback order is:

1. Far-call `5937` and test `DS:89ea`. Nonzero goes directly to the
   transition block at `4416`.
2. In ordinary mode, call `648e`, then `6484`, then `3a8a`; CF from either
   first contact helper goes to `41c1`.
3. Mask `+12 &= 0x0fff`, save Y to `+44`, save X to `+48`, and consume a
   deferred `DS:8812` Y delta as `y += DS:8812 + 1`, then clear it.
4. Read `DS:656c`; if signed `DS:85da <= 0`, call normalized input and copy
   its low byte to `DS:4ff0`.
5. If `+39 != 0`, clear it, set `+37=1`, clear `+3e`, and OR action `0x22`.
   If `DS:89e6 != 0`, force the action to zero. Store it at `+00`.
6. Clear `+40` if action `& 0x22` is zero; increment `+40` unconditionally.
7. If `+38 != 0 && +37 != 0`, jump to the grounded/contact correction at
   `427f`. Otherwise process action bit `0x01` and horizontal accumulators.
8. Store the resulting action at `+00` again, then dispatch by signed `+37`:
   zero -> `42b4`, negative -> `4323`, positive -> `41e8`.
9. All ordinary paths converge at `4384`; the transition path returns through
   `44fe`.

The action-bit-`0x01` branch is intentionally asymmetric. With bit `0x01`
set and mode zero, it clears horizontal action bits `0x08|0x04`, computes
`-(+72 >> 1)` into `+2e`, increments `DS:4fec`, ramps `DS:4fe8` by `0x1000`
up to `0x18000`, subtracts it from `DS:4fe2`, clamps `DS:4fe2` at zero, and
adds `DS:4fe8` into `DS:4fe2`. The clear/action-zero branch sets
`DS:4fec=0`, clears action bit `0x01`, writes `-+72` to `+2e`, and when
`+2a` is active adjusts `DS:4fe8` by `0x2000` toward zero. The common
`4159` tail subtracts `0x1000` from `DS:4fe8` while `DS:4fe2>0`, clamps it
at `-0x18000`, adds `0x2000` when `DS:4fe2<0x100000`, and accumulates again.

## Collision and correction coordinates

The MAP cell is 16 pixels. The descriptor occupancy helper uses the same
cell address but selects one of four 8-pixel quadrants. Coordinates below are
the integer high words, not the full fixed-point values.

| Caller | First probe | Retry/second probe | State write |
| --- | --- | --- | --- |
| `3a1f` | `(x - 5, y)` | `(x + 5, y)` only if the first is clear | `+3b = 0xff` if both are clear |
| `3df2` | `(x - 5, y)` | `(x + 5, y)` if the first is clear | if either occupied, `y_pixel &= 0xfff8` |
| `3998` | `(x +/- 10, y - 1)`; sign from `+29` | same X at `y - 0x11`, then `y - 0x21` if `+72 > 0x20` | flags only |
| `3971` | `(x, y - 0x0a - +72)` | relocated backend | flags only |
| `3986` | `(x, y - +72)` | relocated backend | flags only |
| `41f7` | `(x - 5, y)` | `(x + 5, y)` if the first is clear | positive-path correction branch |
| `4218` | `(x, y)` | relocated `1c6e` | if clear, `y_pixel &= 0xfff0` |
| `3d02` | descriptor at `(x, y)` | if `DX&0x30==0`, retry at `(x, y - 8)` | descriptor response and Y target |
| `44a0` | descriptor at `(x, y)` | descriptor at `(x, y - 16)` | transition collision gate |
| `6370` negative mode | tile at `(x + DS:5003, y - +72)` | match tile IDs `8,9,10` | contact factory and object reposition |
| `6370` ordinary mode | tile at `(x + DS:5003, y)` | first IDs `8,9,10`, then `5,6,7` | contact factory and object reposition |
| `648e` negative mode | tile at `(x + 5, y - +72)` | match `8,9,10` | contact factory; CF set |
| `648e` ordinary mode | tile at `(x + 5, y)` | first IDs `8,9,10`, then `5,6,7` | contact factory and object reposition |

The `3df2` write is to `+08`, the Y integer word. Earlier notes described
this as an X snap; the raw instruction is `AND word ptr [ES:DI+08], 0xfff8`,
so the corrected name and pseudocode use Y.

## MAP and descriptor semantics

The tile lookup at `3376` computes:

```c
cell = far_read_word(DS:657c,
    DS:657a + (uint16_t(y) >> 4) * DS:657e + (uint16_t(x) >> 4) * 2);
return cell & 0x01ff;
```

The descriptor helpers use the equivalent byte address
`DS:657a + (y >> 4) * DS:657e + ((x >> 3) & 0xfffe)` and then fetch the
descriptor record at:

```c
descriptor = far_read_word(DS:6584,
    DS:6582 + tile_id * DS:30d4 + 2);
```

`5c27` returns flags rather than a value. If the descriptor low nibble is
zero it returns ZF=1. Otherwise the coordinate quadrant selects exactly one
descriptor bit:

| `y bit 3` | `x bit 3` | descriptor bit tested |
| ---: | ---: | ---: |
| 0 | 0 | `0x08` |
| 0 | 1 | `0x04` |
| 1 | 0 | `0x01` |
| 1 | 1 | `0x02` |

Thus `ZF=0` is the occupied/blocking result. `5cc3` returns the full
descriptor in `DX`; `3d02` tests `DX&0x30`, then `DX&0x20`, then `DX&0x40`.
The controlled descriptor matrix is:

| Tile | Descriptor | `0x10` | `0x20` | `0x40` | Observed use |
| ---: | ---: | ---: | ---: | ---: | --- |
| `0x28` | `0x0010` | yes | no | no | suppresses retry |
| `0x29` | `0x0050` | yes | no | yes | alignment branch |
| `0x2a` | `0x0070` | yes | yes | yes | response, latch `0xff` |
| `0x2b` | `0x0030` | yes | yes | no | response, latch `1` |

The `0x10` bit is only observed through `DX&0x30`; it is not independently
named as a polarity or “negative” bit. `0x20` selects the response polarity
and latch value. `0x40` selects the alignment branch.

## `3d02` descriptor correction

The helper first requires `+3b != 0`, then clears `+3a` and reads `DX` at the
current `(x,y)`. If `DX&0x30` is clear, it subtracts 8 from the Y integer
word and reads again;
if that retry is also clear, it restores the Y word and returns the early
flags. Otherwise it applies the descriptor response:

```c
if (descriptor & 0x20) {
    if (mode == 0)
        p->y_velocity_fixed = arithmetic_shift_right(p->x_velocity_fixed, 1);
    p->vertical_response_latch = 0xff;
    phase = (p->x_pixel & 0x0f) >> 1;
} else {
    if (mode == 0)
        p->y_velocity_fixed = arithmetic_shift_right(-p->x_velocity_fixed, 1);
    p->vertical_response_latch = 1;
    phase = (0x0f - (p->x_pixel & 0x0f)) >> 1;
}

target_y = (p->y_pixel & 0xfff0) + phase;
if ((descriptor & 0x40) == 0)
    target_y += 8;                 // raw 8086: SUB BX,0xfff8

if (original_y < target_y) {
    p->vertical_response_latch = 0;
    return ZF;                      // early/correction return
}
if (original_y != target_y)
    p->y_pixel = target_y;
return AL_1;                        // response return; ZF=0
```

The source operand is deliberately recorded as `+0x0a`: that is what the
bytes load. It is not silently renamed to a vertical-input field.

## Motion and state writes

`3ab9` first sets direction from action bits `0x04`/`0x08`, integrates
`x += vx`, derives `+29` from `vx + DS:8816`, and runs the forward surface
probe. On blockage it zeros `vx`, may set `+36=1`, clears `+13`, and emits
the relocated animation helper. On a clear surface it consumes
`DS:8816` into X and clears that global. It then accelerates right or left
by `+4c`, clamps by `+5c`, applies friction `+54` when neither direction is
held, and updates the animation byte according to the `0x28000` speed
threshold. If the vertical latch indicates a reverse crossing and the Y
movement is upward, it calls `3986`, restores saved X/Y on failure, and
zeros horizontal velocity.

The signed-mode branches in `update_player_record` are:

* Positive `+37`: increment `+3e`; side probes; descriptor correction; add
  positive Y velocity by `+50`, cap at `+60`, integrate Y; on no collision,
  clear Y velocity and mode, possibly emit `3156`, and set `+36=1`.
* Zero `+37`: side test first; when no side response, run `3df2` and `3d02`;
  if action `&0x22` survives the gate/counter tests, call `3971`. A clear
  result enters reset/death state: `+3e=0x03e8`, `+3b=0`, `+3a=0`,
  `+37=0xff`, `+0e=+64`, then `3160`.
* Negative `+37`: call `3986`; if occupied, enter `41c1`; otherwise add
  `+58` to Y velocity, clamp it no lower than `-0x20000`, reject a
  nonnegative result, integrate Y, and call `3986` again. The second probe's
  clear result goes to `4384`; its occupied result goes to `41c1`. The
  raw `436b..437f` mode-1/contact fragment is not reached by this branch.

The common tail at `4384` calls `38ca`, `38ec`, `3ab9`, the relocated
animation/view helper, `3a62`, and `3e41`; publishes EAX/EBX through `20af`,
clears `+38`, decrements `+34`, and sets `+12` bit `0x8000` when the timer
has bit `0x02`. It checks the camera Y bound, updates the idle counter toward
`0xd2`, and emits the transition helper when `+37==0` and `DS:89e6==-1`.

The recovered constants are all integer/fixed-point values. The important
ones are:

| Constant | Use |
| ---: | --- |
| `0x2800` | horizontal acceleration and positive-Y acceleration fields |
| `0x2000` | friction and negative-Y acceleration fields |
| `0x18000` | horizontal speed cap and input accumulator cap |
| `0x40000` | positive-Y cap and `3e41` view-delta clamp magnitude |
| `0xfffc0000` | negative `3e41` view-delta clamp (`-0x40000`) |
| `0xfffe0000` | negative-mode velocity floor (`-0x20000`) |
| `0xfffb6000` | reset/death Y velocity (`-0x4a000`) |
| `0x1000` / `0x2000` | input accumulator ramp/release increments |
| `0x22` | transition/action gate mask |
| `0x3c` | input-run counter threshold |
| `0xd2` / `0xd3` | idle threshold / initializer value |
| `0x0a` / `0x0d` / `0x13` | vertical probe and action-frame gates |
| `0x82` / `0xbe` | `3e41` camera-relative X window thresholds |
| `0x96` / `0x5a` | `3e41` Y phase thresholds |
| `0x5000` | transition X correction |

The transition block at `4416` is separate from ordinary input. When
`DS:89ea == -1` it sets `+0e=-0x20000`, clears `DS:8822`, and calls `31a4`;
it then advances the animation descriptor, publishes a zero EAX/EBX view
delta, adds `0x1800` to Y velocity up to `0x20000`, queries current and Y-16
descriptors, and uses relocated `1bd1` (CF selects the contact branch) before
writing the transition position. A contact hit decrements `DS:89ea`; only
values below `-0x15d` set `DS:89ec=-1`.

## Relevant globals and external calls

The MAP/descriptor globals are `DS:657a` (MAP offset), `DS:657c` (MAP
selector), `DS:657e` (row stride), `DS:6582` (descriptor offset),
`DS:6584` (descriptor selector), and `DS:30d4` (descriptor record stride,
runtime value 4). The player identity is `DS:881a`; deferred movement uses
`DS:8812` and `DS:8816`.

The action sources are `DS:88bc` and `DS:8196`; `DS:656c` is the action word
read at the callback boundary, and `DS:4ff0` receives the normalized low byte.
`DS:89e6` suppresses actions when nonzero and is also compared with `-1` in
the transition tail. `DS:89ea` selects the transition callback branch.

Far-call target names in the pseudocode are intentionally mechanical when
the target body is outside this closure. The relocations prove the call
target and near/far semantics; they do not, by themselves, prove a gameplay
label for `0fcf`, `0e06`, `1bd1`, `1c6e`, `1c92`, `199d`, `20af`, or the
animation/effect helpers. Those are listed with their exact offsets in the
JSON manifest instead of being collapsed into guessed fields.
