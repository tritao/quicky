# Player collision kernel and descriptor semantics

This is the exact segment-3 leaf model recovered for executable
c9b2e59febd6fa0ea271bedf360459353f55c74444f026964b70988d6de1bca1.
Addresses are segment-relative to runtime selector 01F7; the player record is
ES:DI. “Solid”, “floor”, and “wall” below describe an observed probe result,
not a claim that the executable has those independent masks.

## Result at a glance

The player callback does not read the seven upper MAP bits as collision flags.
The identified collision readers all mask a MAP cell to 0x01ff, use that value
as a tile ID, and read a separate four-byte descriptor record:

    tile_id    = MAP[(y >> 4) * DS:657E + ((x >> 3) & 0xfffe)] & 0x01ff
    descriptor = DS:6582 + tile_id * DS:30D4 + 2, read word

The descriptor record has a renderer field at +0 and the collision word at +2;
the recovered stride is DS:30D4 = 4. The low nibble is a four-way 8-pixel
occupancy selector. Bits 0x10, 0x20, and 0x40 are consumed by the full-word
response helper at 3D02 and, in the transition block, by tests of the
descriptor word left in DX by 5C27.

The ordinary callback prefix is:

    3FF8 -> 648E -> 6484 -> 3A8A

when DS:89EA == 0 and neither hazard helper returns carry. The normal
descriptor-side trace then observes:

    3A1F -> 3DF2

The later 3D02 call is state-path dependent; its exact branches are listed
below. 648E/6484/6370 are tile-ID hazard/effect probes, not solid geometry.

## Helper catalog and callers

| Entry | Final analysis name | Inputs / result | Important writes | Role / confidence |
| --- | --- | --- | --- | --- |
| 3FF8 | update_player_record | implicit ES:DI | whole player state | persistent callback; confirmed |
| 3376 | map_tile_id_at_pixel | AX=y, BX=x; returns AX=cell&1ff | none | raw tile-ID leaf; mechanically exact |
| 5C27 | map_descriptor_quadrant_test | AX=y, BX=x; selected occupancy test in flags | none | low-nibble quadrant leaf; mechanically exact |
| 5CC3 | map_descriptor_word_at_pixel | AX=y, BX=x; returns descriptor in DX | none | full descriptor leaf; mechanically exact |
| 3A1F | player_probe_side_clear | implicit ES:DI | +3B=FF only after two clear probes | ordered X-side probe; mechanically exact |
| 3DF2 | player_snap_y_on_side_contact | implicit ES:DI | +8 &= FFF8 | repeated side probe and Y snap on either hit; mechanically exact |
| 3D02 | player_resolve_descriptor_response | implicit ES:DI | +3A, +0E, sometimes +8; AL at exits | descriptor response/correction; mechanically exact |
| 3A8A | player_probe_transition_tiles | implicit ES:DI | calls transition handlers for tile IDs B..D | state/transition tile probe; mechanically exact |
| 648E | player_probe_hazard_right | implicit ES:DI | spawns effect object and writes hazard temporaries | x+5 hazard probe; mechanically exact, gameplay names qualified |
| 6484 | player_probe_hazard_plus5 | implicit ES:DI | DS:5003=5, then calls 6370 | wrapper; mechanically exact |
| 6370 | player_probe_hazard_offset | implicit ES:DI, DS:5003 | spawns effect object; writes +38, +2A on spawned record | tile-ID hazard probe; mechanically exact |
| 1B07 | player_begin_tile_transition | implicit player | clears response state, sets mode FF, copies +64 to Y velocity | transition-state helper; mechanically exact |
| 19E6 | player_apply_transition_reset | implicit player | resets motion/shared transition globals | transition reset; mechanically exact |
| 44DC | player_update_transition_motion | implicit player | transition motion/state | transition block helper; role confirmed, field names qualified |

Direct far-call relocations confirm 3A1F -> 5C27, 3DF2 -> 5C27,
3D02 -> 5CC3, 3A8A -> 3376, 648E -> 3376, and 6370 -> 3376.
The many other 5C27 callers are object/state paths; they use the same leaf,
but are not all player-solid paths. In particular, 4416 calls 5C27 at
447B/448C/44A0/44B1 and tests the returned descriptor in DX with 0x70 while
choosing transition motion.

## MAP and descriptor queries

3376 uses 16-pixel cells and returns only the low nine bits. 5C27 and 5CC3
use the equivalent byte offset written as an 8-pixel-aligned X term:

    uint16_t cell_word(uint16_t y, uint16_t x) {
        uint16_t offset = DS_657A
            + (uint16_t)(y >> 4) * DS_657E
            + (uint16_t)((x >> 3) & 0xfffe);
        return FS[offset];                 // FS = DS:657C
    }

    uint16_t descriptor_word(uint16_t y, uint16_t x) {
        uint16_t tile = cell_word(y, x) & 0x01ff;
        return descriptor_table[tile].word_at_plus_2;
    }

5C27 first tests descriptor & 0x000f. If the nibble is zero it returns
clear. Otherwise it selects one bit from the coordinate subquadrant:

| y bit 3 | x bit 3 | selected descriptor bit | geometric name for Y-down coordinates |
| ---: | ---: | ---: | --- |
| 0 | 0 | 0x08 | top-left |
| 0 | 1 | 0x04 | top-right |
| 1 | 0 | 0x01 | bottom-left |
| 1 | 1 | 0x02 | bottom-right |

The low-nibble bit names are high-confidence mechanically. Calling them
“solid occupancy” is a usage-based interpretation; the leaf itself only
returns a condition-code result: CF=1 when the selected bit is set and CF=0
when it is clear. The callers primarily consume the resulting ZF/NE condition
after the far call. The descriptor word remains in DX after the leaf restores
AX/BX, so a caller may also test descriptor high bits without calling 5CC3.

## Side probes: 3A1F and 3DF2

The exact probe points are integer high words, not the 16.16 fractions:

    left  = (object +0x04) - 5, (object +0x08)
    right = (object +0x04) + 5, (object +0x08)

3A1F does this:

    if (player->byte_38 != 0)
        return;
    if (player->byte_37 == 0xff)
        return;
    if (descriptor_quadrant_test(y, x - 5))
        return;                         // right probe is not called
    if (descriptor_quadrant_test(y, x + 5))
        return;
    player->byte_3b = 0xff;

It never clears +3B. Thus a left hit, right hit, inside corner, and ledge
are distinguished only by which ordered probe terminates the helper. A gap
or two clear probes is the only path that writes +3B=FF.

3DF2 is separately gated:

    if (player->byte_3b == 0 || player->byte_3a != 0)
        return;
    if (descriptor_quadrant_test(y, x - 5) ||
        descriptor_quadrant_test(y, x + 5))
        player->y_integer = player->y_integer & 0xfff8;

The second probe is reached only when the first is clear. The last write is to
object +0x08, the integer Y word, and occurs when either probe is occupied. It
is not an X snap.
This is why the current rectangle-based C++ controller remains a compatibility
model rather than a faithful implementation.

## Descriptor response: 3D02

The helper first checks +3B. If zero, it returns at 3DF1 without setting a new
AL value. Otherwise it clears +3A, saves the original integer Y, and reads
the full descriptor at (x,y).

    uint16_t original_y = player->y_integer;
    uint16_t probe_y = original_y;
    uint16_t d = descriptor_word(probe_y, player->x_integer);

    if ((d & 0x30) == 0) {
        player->y_integer -= 8;
        probe_y = player->y_integer;
        d = descriptor_word(probe_y, player->x_integer);
        if ((d & 0x30) == 0) {
            player->y_integer += 8;
            return { .return_site = 0x3D44, .al = (uint8_t)original_y };
        }
    }

    uint16_t phase;
    if (d & 0x20) {
        if (player->mode == 0)
            player->velocity_y = arithmetic_shift_right(player->velocity_y, 1);
        player->byte_3a = 0xff;
        phase = player->x_integer & 0x000f;
    } else {
        if (player->mode == 0)
            player->velocity_y = arithmetic_shift_right(-player->velocity_y, 1);
        player->byte_3a = 1;
        phase = 0x000f - (player->x_integer & 0x000f);
    }

    uint16_t target = (player->y_integer & 0xfff0) + (phase >> 1);
    if ((d & 0x40) == 0)
        target += 8;

    if ((int16_t)original_y < (int16_t)target) {
        player->byte_3a = 0;
        return { .return_site = 0x3DE4, .al = 0 };
    }
    if (original_y != target)
        player->y_integer = target;
    return { .return_site = 0x3DF1, .al = 1 };

The two velocity operations are the executable’s neg/shr/sign-repair and
shr/sign-repair sequences, equivalent to signed arithmetic shift right by
one for ordinary signed 32-bit values. The velocity write occurs before the
Y comparison, so a 3DE4 rejection can still leave the halved velocity when
+37 == 0. If the y−8 retry found the descriptor, the code keeps that
subtraction while calculating the target; the no-response retry alone is
restored at 3D40. At 3D44, the saved original Y is popped back into AX, so
AL is its low byte and ZF remains set from the clear DX&0x30 test. At 3DE4,
AL is explicitly zero and ZF is set by CMP BL,BL. At 3DF1, AL is one and ZF
is clear after TEST AL,1. The early +3B==0 return reaches 3DF1 without
assigning a new AL value.

### Mechanical flag matrix

| descriptor flags | retry at y−8 | response polarity / latch | target phase | observed meaning |
| ---: | --- | --- | --- | --- |
| 0x00 | yes; restore if retry also clear | none | none | clear return at 3D44 |
| 0x10 | no | -velocity/2, +3A=1 | 15-(x&F), plus 8 | retry suppression; acceptance depends on Y comparison |
| 0x20 | no | velocity/2, +3A=FF | x&F, plus 8 | response polarity alone; controlled run rejected |
| 0x30 | no | velocity/2, +3A=FF | x&F, plus 8 | controlled run rejected |
| 0x40 | yes | none if retry clear | none | alignment bit alone does not produce response |
| 0x50 | no | -velocity/2, +3A=1 | 15-(x&F), no 8 | 0x10 still suppresses retry |
| 0x60 | no | velocity/2, +3A=FF | x&F, no 8 | controlled positive return |
| 0x70 | no | velocity/2, +3A=FF | x&F, no 8 | natural positive descriptor |

The names “response polarity”, “retry suppression”, and “alignment selector”
are mechanical names. No isolated evidence proves that any one of these bits
means “one-way platform”, “floor”, or “ceiling” globally.

## Geometry matrix

The isolated geometry cases reduce to the following leaf outcomes. The
reference implementation and tests are in
research/tools/player_collision_kernel.py and
research/tests/test_player_collision_kernel.py.

| Case | Synthetic occupancy pattern | First leaf behavior | Exact consequence |
| --- | --- | --- | --- |
| floor-like contact | descriptor hit at sampled side/Y point | 3A1F stops at first hit | no side latch write; later state path decides whether 3D02 runs |
| ceiling-like contact | response descriptor at current point or at y−8 | 3D02 uses current point, then one y−8 retry | correction is descriptor target Y; no separate ceiling mask |
| left wall | (x−5,y) hit | one 5C27 call | right probe is skipped |
| right wall | left clear, (x+5,y) hit | two 5C27 calls | no +3B write; 3DF2 snaps Y on the right hit |
| inside corner | both side points hit | first left hit wins | the second side is not observed by that helper |
| outside corner | left clear, right hit | left then right | same observable path as a right-side ledge |
| one-tile gap | both side points clear | both probes complete | 3A1F writes +3B=FF; 3DF2 returns without snapping |
| ledge | one side hit, one side clear | order determines whether second probe runs | no independent ledge rule exists in these leaves |
| one-way candidate | descriptor 0x20/0x40/0x60 | 3D02 branch/target changes | one-way behavior is not proven; direction is not tested by 3D02 |

The probes do not form a general rectangle sweep. Consequently, a classic
floor/ceiling/wall taxonomy cannot be made exact without including the caller
state that chooses when to invoke these leaves.

## Transition-block descriptor gate

The nonzero-DS:89EA callback block at 4416 has a second descriptor use that is
easy to miss if the audit follows only 5CC3. Depending on +0x29, it calls the
quadrant helper at the current point and at Y−16, or in the opposite order:

    447B: 5C27(x, y);    test DX, 0x70
    448C: 5C27(x, y-16);  test DX, 0x70

The +0x29-positive branch uses 44A0 and 44B1 for the same two probes. Any of
the descriptor bits 0x10/0x20/0x40 sends control to 44DC. This is a transition
motion gate, not the 3D02 correction formula: it does not change +0x0E by the
3D02 half-velocity branches and it does not compute the 3D02 target Y. The
low-nibble condition result from 5C27 is not the gate at these call sites;
the caller explicitly tests the descriptor word still present in DX.

## Tile hazards, transitions, and objects are separate

648E, 6484, and 6370 call 3376, compare tile IDs 5..A, and create a
transient object through the object factory. For normal mode (+37==0), the
x+5 path recognizes 8..A first and then 5..7; negative mode has a different
Y offset and returns carry after a match. These helpers do not read descriptor
low-nibble occupancy and are not solid collision.

3A8A is gated by +37 > 0, reads the tile ID at (x,y), and dispatches 0B/0C/0D
to 1B07 and 19E6. Those helpers set transition/death/reset state, timers, and
vertical velocity; they are boundary/transition behavior, not a platform mask.

Moving platforms use a separate object callback path:

    platform callback 9DC7 -> overlap test A075 -> carry publisher A0B2
                             -> DS:8816 (X delta), DS:8812 (Y delta)
    player callback consumes those deltas

The native platform overlap is a strict horizontal overlap plus a 12-pixel
vertical band. It is not routed through 5C27, 5CC3, or 3D02. Normal
enemy/object MAP contact likewise uses object helpers around 1DCA, 1B77, 1C4D,
and 1C6E, not the player descriptor response leaf.

No generic “outside is solid” rule was isolated. The observed boundary and
transition behavior is handled by explicit callback state, camera bounds, and
transition helpers.

## Reversible MAP experiments and validation

The debugger trace tools patch only the queried MAP cell or descriptor-table
word, record the original bytes, and restore them on every return/guard path.
The controlled matrix in research/notes/descriptor-collision-evidence.json
includes:

* tile substitutions producing descriptor words 0x00, 0x10, 0x20, 0x30,
  0x40, 0x50, 0x60, and natural 0x70;
* direct validation of 3D44, 3DE4, and 3DF1 return paths;
* protected-selector MAP reads proving tile ID and descriptor dataflow;
* paired 3DF2 left/right/both side-probe patches.

The pure leaf model adds deterministic corner, gap, ledge, retry, velocity, and
simultaneous-axis tests. The focused Python suites pass, including the
existing descriptor, property, branch, and collision reports. The static
Ghidra annotation was applied to the external project
/home/joao/dev/quiky-ghidra-project-20260824 using
research/tools/AnnotatePlayerCollision.java; the decompiler now shows the
canonical names and QuikyPlayerRecord field names.

The remaining confidence boundary is intentional: a final gameplay label such
as “one-way platform” or “floor” would require evidence from a caller that
selects that state, not just the descriptor bit operations proven here.
