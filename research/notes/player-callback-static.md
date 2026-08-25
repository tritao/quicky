# Player callback static control flow

This note records the segment-3 bytes around `01F7:3FF8` from
`research/build/segments/QUIKY_SEG03.bin`. The offsets below are preserved as
raw executable offsets; the `lcall 0xffff:0xffff` placeholders are NE
relocations and are not assigned a semantic name unless runtime evidence has
already identified the target.

## Initializer `01F7:3F27`

The initializer receives `ES:DI` and stores `DI` at `DS:881A`. It clears
`DS:4FEC`, `DS:8812`, `DS:8810`, `DS:4FE2`, and `DS:4FE6`, then clears the
player's velocity at `+0x0A/+0x0E`, byte `+0x37`, word `+0x34`, and word `+0`.
It writes the following fixed state before installing callback `+0x18 = 0x3FF8`:

```text
+0x72 = 0x0028
+0x4C = 0x00002800   +0x50 = 0x00002800
+0x54 = 0x00002000   +0x58 = 0x00002000
+0x5C = 0x00018000   +0x60 = 0x00040000
+0x64 = 0xFFFB6000   +0x2C = 0xFFF6
+0x30 = 0x000A       +0x2E = +0x72
+0x32 = 0            +0x3B = 1
+0x38 = 0xFF         +0x36 = 0xFF
+0x28 = 1            +0x29 = 1
+0x18 = 0x3FF8
```

If `DS:85DA` is not positive, it calls the initializer-side helper at
`0x316A` before returning. These writes establish the initial fixed-point
limits and state bytes; they do not by themselves prove gameplay names for
the fields.

## Callback entry and top-level blocks

`01F7:3FF8` starts with a call at `3FF8`, then tests `DS:89EA` (`3FFD`). A
nonzero value jumps to the transition block at `4416`; zero continues through
the ordinary update path:

```text
3FF8 -> 3FFD --nonzero--> 4416
             --zero-----> 4006 -> 400F -> 4018
```

The three calls at `4006`, `400F`, and `4018` are the runtime-observed
`648E`, `6484`, and `3A8A` chain. Carry from either first helper branches to
`41C1`; otherwise `401D` masks `+0x12` with `0x0FFF`, saves the current
16.16 Y/X at `+0x44/+0x48`, and enters the input/motion block.

At `4037`, a nonzero `DS:8812` sets `+0x38 = 0xFF`, adds
`DS:8812 + 1` to the 16.16 Y at `+0x06`, and clears `DS:8812`. The callback
then reads `DS:656C` and conditionally calls the helper at `0x316A`; its low
byte is stored at `DS:4FF0`.

The `+0x39` transition byte is consumed at `406D`: if nonzero, it is cleared,
`+0x37` becomes `1`, `+0x3E` becomes `0`, and `0x22` is ORed into the local
action word. `DS:89E6` is then tested at `4087`; nonzero forces the action word
at `+0` to zero. If the resulting action word has no `0x22` bits, `+0x40` is
cleared at `409B`; it is incremented unconditionally at `40A1`.

## Input and horizontal/vertical motion blocks

The first action branch tests bit `0x01` (`40B5`). If it is clear, control
goes to `4132`, which clears `DS:4FEC`, clears action bit `0x01`, negates
`+0x72` into `+0x2E`, and applies the release/alternate-action counter at
`+0x2A` before entering the common acceleration tail at `4159`.

If bit `0x01` is set and `+0x37 == 0`, `40C1` clears bits `0x08/0x04`, computes
`-(+0x72 >> 1)` into `+0x2E`, increments `DS:4FEC`, and then updates the
16.16 accumulator `DS:4FE8` in 0x1000 steps. It caps that accumulator at
`0x18000`, subtracts it from `DS:4FE2`, and clamps the latter at zero. The
common write at `41AF` stores the action word at `+0`.

The alternate `4132` branch instead maintains `DS:4FEC = 0`, subtracts the
full `+0x72` into `+0x2E`, and, while `+0x2A` is active, repeatedly adjusts
`DS:4FE8` by `0x2000` toward zero. The shared `4159` tail updates `DS:4FE8`
in the opposite direction when `DS:4FE2 > 0`, with limits `-0x17FFF` and
`0x10000`, then adds it into `DS:4FE2`.

After `41AF`, the signed byte `+0x37` selects three paths:

```text
+0x37 == 0       -> 42B4
+0x37 < 0        -> 4323
+0x37 > 0        -> 41E8
```

The positive path increments `+0x3E`, probes at `x-5` through `3A1F`, and
then tests the second-side `3DF2` path. It is the path used by the observed
reset/death transition. `41C1` is entered from the early helper carry path;
it writes `+0x3E = 0x03E7` (or zero at `41C9`), sets `+0x37 = 1`, clears
`+0x0E`, calls `0x3186`, and jumps to the common tail `4384`.

## Positive path `41E8–42B4`

`41E8` increments `+0x3E`, computes `BX = +0x04 - 5`, `AX = +0x08`, and
calls the first side helper. A zero result probes `+0x04 + 5`; if that also
returns zero, it probes the vertical helper at the current position. A
nonzero result snaps `+0x08 &= 0xFFF0` at `421F`.

The path then tests `+0` against `0x22` and applies the `+0x40`/`+0x3E`
counter gates at `4225–4238`. It clears `+0x2B`, and if `+0x38 == 0`, calls
`3D02` followed by `3DF2` (`4248–4289`). If `+0x3A` is still zero, it clamps
the positive 16.16 Y velocity `+0x0E` against `+0x60`, integrates it into
`+0x06`, calls `3A1F`, and exits to `4384` on a zero result.

The no-collision continuation clears `+0x0E` and `+0x37`, emits the timer
helper when `DS:4FEE < 0xD2`, sets `+0x36 = 1`, and exits through `4384`.

## Stationary/negative paths `42B4–4384`

At `42B4`, `3A1F` is called first. A nonzero result or nonzero `+0x38`
skips the `41C9` reset branch and proceeds to `3DF2` then `3D02` at
`42C9–42CC`. The `+0x22` test and the `+0x38/+0x3E` counter gate at
`42CF–42E4` control entry to `3971`. If `3971` returns zero, the callback
writes the death/reset state: `+0x3E = 0x03E8`, `+0x3B = 0`, `+0x3A = 0`,
`+0x37 = 0xFF`, copies `+0x64` into `+0x0E`, calls `0x3160`, and exits at
`4384`.

The negative path `4323` calls `3986`; a nonzero result returns to `41C1`.
Otherwise it adds `+0x58` to `+0x0E`, clamps the result at `-0x20000`,
rejects positive values, integrates Y at `435E`, and calls `3986` again.
The second clear result exits through `4384`; the occupied result returns to
`41C1`. The raw `436B..437F` mode/contact fragment is not reached by this
branch.

## Common tail and transition path

`4384` calls helpers at `38CA`, `38EC`, `3AB9`, an indirect/relocated helper,
`3A62`, and `3E41`, then clears `+0x38`. It decrements the word timer `+0x34`;
when it reaches zero, `DS:8810` is cleared. Timer bit `0x02` sets `+0x12` bit
`0x8000`. The Y position is compared against `DS:81C4`; outside the lower
bound it calls the camera/transition helper. If `+0x37 == 0` and `+0 == 0`,
`DS:4FEE` increments up to `0xD2`, emitting the timer helper at the limit.

The callback then emits the `DS:89E6 == 0xFFFF` transition helper when `+0x37`
is zero and returns at `4415`.

The nonzero-`DS:89EA` block `4416` distinguishes `DS:89EA == 0xFFFF`: it
sets `+0x0E = -0x20000`, clears `DS:8822`, emits helper `0x31A4`, then calls
the transition routine, accelerates `+0x0E` by `0x1800` per callback, and
caps it at `0x20000` before continuing at `4467`. The latter block tests
`+0x29`, queries `3D02` at current and Y-16 positions when `+0x29 <= 0`, and
then falls into the common transition/update logic.

## Helper contracts confirmed statically

* `3A1F`: checks `+0x38`; probes `(x-5,y)` and then `(x+5,y)` through `5C27`;
  on the second zero result writes `+0x3B = 0xFF` and returns the byte test.
* `3DF2`: requires `+0x3B != 0` and `+0x3A == 0`; probes the same two X
  positions and snaps the integer Y word `+0x08 &= 0xFFF8` when either probe
  is nonzero.
* `3D02`: clears `+0x3A`, calls `5CC3`, retries at `y-8` only when
  `DX&0x30 == 0`, and uses `DX&0x20`/`DX&0x40` for the exact response and
  alignment branches documented in `ghidra-analysis.md`.
* `648E`: handles the object's signed `+0x37` state and dispatches hazard or
  damage outcomes from the `5C27`/MAP result; `6484` records the helper
  result through the relocated call at `0x6370`.
* `3A8A`: when `+0x37 > 0`, calls the MAP tile lookup and dispatches tile IDs
  `0x0B`, `0x0C`, and `0x0D` to its two relocated handlers.

The arithmetic widths above are the instruction widths: position and velocity
updates use 32-bit operands, action/state/timer fields use 16-bit or byte
operands as shown, and the signed branches are the x86 `jl/jg/jle/jge` forms.
This is a control-flow contract, not yet a final gameplay naming pass.
