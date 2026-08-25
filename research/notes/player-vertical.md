# Player vertical motion

This note is the implementation-facing result of the vertical static closure
and focused validation matrix. Static bytes lead the equations and branch
order; traces validate arithmetic and expose the runtime input/collision
boundaries that the frozen tracer can observe.

The machine-readable constants are in
../evidence/player-vertical/constants.json, the validation ledger is in
../evidence/player-vertical/validation.json, and compact normalized rows are
in ../evidence/player-vertical/timelines.json. The catalog is
../experiments/player-vertical.json.

## Reproducibility and evidence scope

Pinned inputs:

- game/QUIKY.EXE, SHA-256 c9b2e59febd6fa0ea271bedf360459353f55c74444f026964b70988d6de1bca1
- game/NESTLE.DAT, SHA-256 84d5198b4afecb4a30bab1f502705d451efaaa3a88bbebb18987966fc89fa3b2
- segment 01F7, 62,013 bytes, SHA-256 8d6c9554a0715935cf50b0fd6f1624941eaaf202cd5ed8f060f8f34f69f790cf

The static closure verifier passes with 22 functions. The vertical analysis
tool is research/tools/player_vertical_analysis.py; it accepts current
raw/current normalized traces and the archival WIP trace shape without
importing the WIP tracer modifications.

The live catalog uses derivation traces for natural-fall attempt and 2-, 4-,
and 20-callback releases; held-out traces for 8 and 12 callbacks; release
boundary traces for 1, 2, 8, 12, and 20; and collision traces for side-contact
and a reversible descriptor patch.

The current phase interface releases a physical key before the sample
callback. Therefore it cannot capture the takeoff callback while the key is
held: the first post-phase state is already vy=-0x20000. The archival WIP
observation is used only for experiment intent and corroboration: it saw the
takeoff writes +0x0E=-0x4A000, +0x37=0xFF, +0x3E=0x03E8, and action 0x20.
No WIP shared tracer code is in this branch.

## Static vertical control flow

The callback is 01F7:3FF8-44FE; offsets are segment-relative.

| Path | Range | Result |
| --- | --- | --- |
| positive mode | 41E8-427E | side/descriptor correction, falling integration, contact/ground branch |
| grounded/contact | 427F-42B3 | correction, vy=0, mode 0 |
| ordinary mode | 42B4-4322 | ordinary correction and input-triggered jump initiation |
| negative mode | 4323-4383 | upward integration, release clamp, apex transition |
| common tail | 4384-44FE | horizontal integration, animation, view, latch/timer cleanup |

The graph is also in research/ghidra/player-static-closure.json under
vertical_control_flow:

~~~text
3FF8 mode==0 -> 42B4 -> 4384
3FF8 mode<0  -> 4323 -> 4384
3FF8 mode>0  -> 41E8 -> 4384 or 427F -> 4384
4323 next_vy>=0 -> 41C1 -> 4384
all ordinary paths -> 4384 -> 44FE
~~~

Local helpers use near CALL/RET. MAP, descriptor, input, contact, animation,
and view targets use relocated far LCALL/LRET. 3986 is a near wrapper around
far 1C92; 3D02, 3A1F, and 3DF2 are near; 5C27 and 5CC3 are far. The full
signatures and flag contracts are in the static JSON and
research/notes/player-static-decomp.cpp.

## Exact fixed-point equations

Position and velocity are signed 32-bit 16.16 values. Integer coordinates are
their signed high words. The original code performs a 32-bit add and updates
velocity before adding it to Y:

~~~cpp
y_next = signed32(y_old + vy_next);
~~~

The initializer at 3F27 loads:

~~~cpp
+0x50 = 0x00002800;  // positive acceleration, +10240
+0x58 = 0x00002000;  // negative-mode acceleration, +8192
+0x60 = 0x00040000;  // positive terminal velocity, +262144
+0x64 = 0xfffb6000;  // jump impulse, -303104 == -0x4a000
+0x72 = 0x0028;      // 40-pixel probe step
~~~

At 4323-436A, negative mode is:

~~~cpp
if (!probe_vertical_step_clear(p).zf)
    return early_contact_41c1(); // mode=1, vy=0, contact response

int32_t next = signed32(p->vy() + p->i32(0x58));
if (p->u8(0x2b) == 0 && (p->action() & 0x22) == 0 &&
    next < -0x20000)
    next = -0x20000;
if (next >= 0)
    return early_contact_41c1(); // apex: no Y add

p->vy(next);
p->y(signed32(p->y() + next));
return probe_vertical_step_clear(p).zf
    ? common_tail_4384() : early_contact_41c1();
~~~

The release clamp is conditional: it is disabled while +0x2B is nonzero or
action 0x22 is visible, and it clamps only values strictly below -0x20000.
The mode byte is 0xFF, which dispatches as signed -1.

At 41E8-427E, positive mode is:

~~~cpp
++p->u16(0x3e);
if (side_clear(p) && side_clear_right(p) &&
    !far_vertical_position_query(p->y_pixel(), p->x_pixel()).zf)
    p->y_pixel(p->y_pixel() & 0xfff0);

if ((action & 0x22) && p->u16(0x40) <= 0x13 &&
    p->u16(0x3e) < 0x0a)
    return grounded_contact_427f();

p->u8(0x2b, 0);
if (p->u8(0x38) == 0) {
    apply_descriptor_vertical_correction(p); // 3D02
    snap_player_y_on_side_response(p);       // 3DF2
}
if (p->u8(0x3a) != 0)
    return grounded_contact_427f();

int32_t next = signed32(p->vy() + p->i32(0x50));
if (next > p->i32(0x60))
    next = p->i32(0x60);
p->vy(next);
p->y(signed32(p->y() + next));
return test_player_side_contact(p).zf
    ? common_tail_4384() : grounded_contact_427f();
~~~

## Input ordering and jump initiation

The callback order is:

1. auxiliary poll 5937 and transition gate DS:89EA;
2. contact helpers 648E, 6484, and special contact dispatch 3A8A;
3. save Y/X to +0x44/+0x48 and consume deferred Y;
4. read DS:656C, or normalized keyboard bits when DS:85DA <= 0;
5. consume +0x39, apply DS:89E6 suppression, and store action;
6. clear/increment +0x40;
7. horizontal input accumulation;
8. dispatch by signed +0x37;
9. converge at common tail 4384.

The ordinary jump branch at 42B4-4322 requires the action word to contain
0x22, +0x38==0, +0x40<=0x0D, and a clear 3971 10-pixel probe after ordinary
side/descriptor correction. On success it performs:

~~~cpp
DS.pending_event = 0;
p->u16(0x3e, 0x03e8);
p->u8(0x3b, 0);
p->u8(0x3a, 0);
p->u8(0x37, 0xff);
p->vy(p->i32(0x64));       // -0x4a000
far_enter_reset_motion(); // 3160
~~~

This establishes the impulse, mode, timer, side latch, vertical latch, and
reset-motion callback before the common tail. Action bit 0x20 is the
jump/confirm input; 0x22 is the hold/jump gate used by the callback.

At the apex the callback immediately before transition has vy=-0x2000. The
next callback writes vy=0, changes mode 0xFF -> 1, and leaves Y unchanged.
The following callback writes vy=0x2800 and adds that new velocity to Y. This
is visible in all five jump timelines and matches the raw branches.

## State fields

| Offset | Type | Safe meaning |
| --- | --- | --- |
| +0x06 | signed int32_t | fixed-point Y |
| +0x08 | signed int16_t | integer Y used by probes/correction |
| +0x0E | signed int32_t | fixed-point vertical velocity |
| +0x2B | uint8_t | contact scratch; gates ascent release clamp |
| +0x36 | uint8_t | animation state; grounded continuation writes 1 |
| +0x37 | signed int8_t | ordinary 0, negative -1, positive 1 |
| +0x38 | uint8_t | correction/contact gate |
| +0x3A | signed int8_t | descriptor response latch, 0/1/0xFF |
| +0x3B | uint8_t | side response latch |
| +0x3E | uint16_t | reset/positive-path timer or counter |
| +0x40 | uint16_t | action-frame counter |
| +0x50 | signed int32_t | positive acceleration |
| +0x58 | signed int32_t | negative acceleration |
| +0x60 | signed int32_t | positive velocity cap |
| +0x64 | signed int32_t | jump impulse |
| +0x72 | uint16_t | 40-pixel vertical step |

The gameplay names of +0x3E, +0x3A, and the external far helpers remain
mechanical where the bytes do not prove a narrower meaning.

## Collision interactions

Probe coordinates use integer high words:

- 3971: (x, y - 0x0A - +0x72), jump-initiation probe;
- 3986: (x, y - +0x72), negative-mode probe before and after integration;
- 3A1F: (x-5,y) and (x+5,y), side latch/contact result;
- 3DF2: same side probes, then masks integer Y with 0xFFF8 when active;
- 3D02: descriptor at (x,y), retry at (x,y-8) only when DX&0x30==0.

5C27 selects low-nibble occupancy bits:

~~~text
(y&8,x&8) = (0,0): 0x08   (0,1): 0x04
             (1,0): 0x01   (1,1): 0x02
~~~

ZF=1 means the selected quadrant is clear. 5CC3 returns the full descriptor.
Controlled values are 0x28->0x10, 0x29->0x50, 0x2A->0x70, and 0x2B->0x30.
0x10 suppresses the Y-8 retry through the combined DX&0x30 test; 0x20
selects response polarity/latch; 0x40 selects an alignment branch. These
bits are not relabeled as generic floor, ceiling, or one-way semantics.

The callback-focused vertical-correction-observe experiment patches MAP cell
(8,25) from 0x2E to 0x2A, records selector 0x377, offset 13516, and restores
the original bytes. Watches reach 3D02 and 3DF1. The first callback changes
integer Y 400 -> 402 (fixed 0x01900000 -> 0x01920000), leaves velocity zero,
and changes +0x3A 0 -> 0xFF. This validates a controlled correction/latch
ordering, not a universal floor/ceiling interpretation.

The side-contact experiment reaches 648E repeatedly and is retained as
contact-helper evidence only. A combined branch/census attempt exceeded the
frozen Lua instruction limit while scanning MAP/descriptor cells. The precise
missing capability is a low-budget combined callback + branch-sequence +
descriptor-patch/census capture; no shared tracer file was changed.

The natural-fall attempt records a valid reversible patch and rightward motion,
but stays in mode 0 in the current matrix. It contributes zero free-space
checks and is reported as a negative reachability observation, not proof that
ledge falling is absent.

## Direct C++-style pseudocode

~~~cpp
void update_vertical(Player& p, Globals& ds, uint16_t action) {
    if (p.mode() > 0) {
        ++p.u16(0x3e);
        if (side_clear(p) && side_clear_right(p) &&
            !vertical_position_query(p.y_pixel(), p.x_pixel()).zf)
            p.y_pixel(p.y_pixel() & 0xfff0);
        if ((action & 0x22) && p.u16(0x40) <= 0x13 &&
            p.u16(0x3e) < 0x0a)
            return grounded(p, ds);
        p.u8(0x2b, 0);
        if (p.u8(0x38) == 0) {
            descriptor_vertical_correction(p); // 3D02
            snap_side_response_y(p);           // 3DF2
        }
        if (p.u8(0x3a) != 0) return grounded(p, ds);
        int32_t next = signed32(p.vy() + p.i32(0x50));
        if (next > p.i32(0x60)) next = p.i32(0x60);
        p.vy(next);
        p.y(signed32(p.y() + next));
        return side_contact(p).zf ? common_tail(p, ds) : grounded(p, ds);
    }
    if (p.mode() < 0) {
        if (!vertical_step_probe(p).zf) return early_contact(p, ds);
        int32_t next = signed32(p.vy() + p.i32(0x58));
        if (p.u8(0x2b) == 0 && (p.action() & 0x22) == 0 &&
            next < -0x20000)
            next = -0x20000;
        if (next >= 0) return early_contact(p, ds);
        p.vy(next);
        p.y(signed32(p.y() + next));
        return vertical_step_probe(p).zf
            ? common_tail(p, ds) : early_contact(p, ds);
    }
    descriptor_vertical_correction(p);
    snap_side_response_y(p);
    if ((action & 0x22) && p.u8(0x38) == 0 && p.u16(0x40) <= 0x0d &&
        vertical_probe_10px(p).zf) {
        p.u16(0x3e, 0x03e8);
        p.u8(0x3b, 0);
        p.u8(0x3a, 0);
        p.u8(0x37, 0xff);
        p.vy(p.i32(0x64));
        enter_reset_motion();
    }
    return common_tail(p, ds);
}

void grounded(Player& p, Globals& ds) {
    if (p.u8(0x38) == 0) {
        descriptor_vertical_correction(p);
        snap_side_response_y(p);
    }
    p.vy(0);
    p.u8(0x37, 0);
    p.u8(0x36, 1);
    if (ds.idle_counter < 0xd2) set_idle_animation();
}
~~~

## Confirmed, uncertain, and readiness

Confirmed from static bytes and live values: signed 16.16 storage;
new-velocity-first integration; the -0x4A000 impulse; guarded release
clamping at -0x20000; +0x2000 ascent; +0x2800 descent; +0x40000 positive
cap; mode/latch/timer write ordering; probe coordinates; quadrant masks;
descriptor masks; near/far contracts; exact apex rows; and zero mismatches
over 328 checked derivation/held-out values.

Remaining ambiguities are natural ledge falling, full ceiling velocity response
and corner ordering, exact flat/edge/one-way landing correction, one-way
behavior from below versus above, moving-platform landing, external callback
effects, and narrower gameplay names for mechanical fields.

The free-space equations and apex transition are ready for engine code. The
complete vertical subsystem is not yet ready for direct foundation
implementation because deterministic ceiling, landing, one-way, and ledge
fall evidence remains incomplete.
