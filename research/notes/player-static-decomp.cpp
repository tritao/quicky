// Direct C++-style pseudocode for QUIKY.EXE segment 3, selector 0x01f7.
//
// This is deliberately offset-oriented.  It models the recovered bytes and
// flags, not a guessed modern gameplay API.  `near` functions return with RET;
// `far` functions return with LRET and are called through NE relocations.

#include <cstdint>

using Fixed16 = int32_t;                 // signed 16.16

struct Flags {
    bool zf;                             // true when x86 ZF=1
    bool cf;                             // true when x86 CF=1
};

struct ViewDelta {
    int32_t eax;
    int32_t ebx;
};

struct Globals {
    uint16_t player_offset;              // DS:881a
    uint16_t deferred_y;                 // DS:8812
    Fixed16 external_x_delta;            // DS:8816
    uint16_t timer_clear;                // DS:8810

    uint16_t input_run_counter;          // DS:4fec
    Fixed16 horizontal_accumulator;      // DS:4fe2
    Fixed16 horizontal_accel;            // DS:4fe8
    int16_t view_state_a;                // DS:4fe4
    int16_t view_state_b;                // DS:4fe6
    uint16_t idle_counter;               // DS:4fee
    uint8_t action_low_copy;             // DS:4ff0
    uint16_t special_speed_cap_mode;     // DS:88b6

    uint16_t map_offset;                 // DS:657a
    uint16_t map_selector;               // DS:657c
    uint16_t map_row_stride;             // DS:657e
    uint16_t descriptor_offset;          // DS:6582
    uint16_t descriptor_selector;        // DS:6584
    uint16_t descriptor_stride;          // DS:30d4; runtime value 4

    uint16_t camera_x;                   // DS:81c0
    uint16_t camera_y;                   // DS:81c4
    uint16_t camera_y_limit;             // DS:81cc

    uint16_t action_source;              // DS:656c
    uint16_t keyboard_actions;           // DS:88bc
    uint16_t secondary_actions;          // DS:8196
    uint16_t last_scan_code;             // DS:88ba
    int16_t activation_state;            // DS:85da

    int16_t collision_transition_mode;  // DS:89ea
    int16_t action_suppressor;           // DS:89e6
    int16_t transition_state;             // DS:89ec
    uint16_t transition_scratch;         // DS:8822
    int32_t transition_object_counter;   // DS:880a
    uint16_t transition_effect_bits;     // DS:8950
    Fixed16 published_view_x;             // DS:60dc
    Fixed16 published_view_y;             // DS:60e0
    uint16_t pending_event;              // DS:612e

    int16_t contact_y_scratch;           // DS:4ffe
    uint8_t contact_subtype;             // DS:5000
    uint8_t contact_code;                // DS:5001
    int16_t contact_x_offset;            // DS:5003
};

extern Globals DS;

// The real record is 0x78 bytes and has overlapping byte/word/dword views.
// These accessors are the least ambiguous C++ representation of that fact.
struct PlayerRecord {
    uint8_t bytes[0x78];                 // ES:DI, normally ES == 0x027f

    uint8_t  u8(uint16_t off) const;
    void     u8(uint16_t off, uint8_t value);
    uint16_t u16(uint16_t off) const;
    void     u16(uint16_t off, uint16_t value);
    int16_t  i16(uint16_t off) const;
    void     i16(uint16_t off, int16_t value);
    Fixed16  i32(uint16_t off) const;
    void     i32(uint16_t off, Fixed16 value);

    uint16_t action() const { return u16(0x00); }
    void action(uint16_t v) { u16(0x00, v); }
    Fixed16 x() const { return i32(0x02); }
    void x(Fixed16 v) { i32(0x02, v); }
    Fixed16 y() const { return i32(0x06); }
    void y(Fixed16 v) { i32(0x06, v); }
    Fixed16 vx() const { return i32(0x0a); }
    void vx(Fixed16 v) { i32(0x0a, v); }
    Fixed16 vy() const { return i32(0x0e); }
    void vy(Fixed16 v) { i32(0x0e, v); }
    int16_t x_pixel() const { return i16(0x04); }
    void x_pixel(int16_t v) { i16(0x04, v); }
    int16_t view_anchor_x() const { return i16(0x4a); }
    int16_t y_pixel() const { return i16(0x08); }
    void y_pixel(int16_t v) { i16(0x08, v); }
    int8_t mode() const { return static_cast<int8_t>(u8(0x37)); }
    uint8_t gate() const { return u8(0x38); }
};

// Relocated memory operations.  Arguments are AX=y, BX=x in the original.
extern uint16_t far_read_word(uint16_t selector, uint16_t offset);
extern void far_write_word(uint16_t selector, uint16_t offset, uint16_t value);

// Relocated targets outside the primary closure.  The names are mechanical;
// their exact offsets are in player-callback-closure.json.
extern void player_helper_5937();                   // 5937; unresolved input-side helper
extern int16_t animation_sequence_word(uint16_t sequence_offset); // raw DS:SI word
extern void dispatch_transition_effect_0CE3();       // 01e7:0ce3 contract
extern Flags probe_transition_descriptor(uint16_t cx, uint16_t dx); // 1bd1
extern Flags probe_map_word_bit_4000(int16_t y, int16_t x); // 1c6e
extern Flags probe_map_word_bit_1000(int16_t y, int16_t x); // 1c92
extern void dispatch_pending_sound_effect();        // 01e7:0fcf
// 0e06 is the shared first-free pool allocator. Its callback argument is
// selected by the caller; the allocator itself does not identify a gameplay
// object family. Keep the address in the name until every caller family is
// separately closed.
extern PlayerRecord* object_pool_factory_0E06(uint16_t ax, uint16_t dx);
extern void apply_tile_transition_1B07();            // 1b07
extern void apply_transition_reset_19E6();          // 19e6
extern PlayerRecord* spawn_contact_effect_entry(PlayerRecord* owner); // 4519 contract
extern uint16_t player_pool_offset(PlayerRecord*);  // representation of DI
extern PlayerRecord* player_from_pool_offset(uint16_t); // DS:881a -> ES:DI

// --------------------------- MAP / descriptors --------------------------

// Far entry 01f7:3376.  The raw helper takes AX=y and BX=x.
uint16_t map_tile_id_at_pixel(uint16_t y, uint16_t x) {
    uint16_t address = static_cast<uint16_t>(
        DS.map_offset + (static_cast<uint16_t>(y) >> 4) * DS.map_row_stride
        + (static_cast<uint16_t>(x) >> 4) * 2);
    uint16_t cell = far_read_word(DS.map_selector, address);
    return cell & 0x01ff;
}

uint16_t descriptor_word_at(uint16_t tile_id) {
    uint16_t address = static_cast<uint16_t>(
        DS.descriptor_offset + tile_id * DS.descriptor_stride + 2);
    return far_read_word(DS.descriptor_selector, address);
}

static uint16_t map_word_at_pixel(int16_t y, int16_t x) {
    uint16_t address = static_cast<uint16_t>(
        DS.map_offset + (static_cast<uint16_t>(y) >> 4) * DS.map_row_stride
        + ((static_cast<uint16_t>(x) >> 3) & 0xfffe));
    return far_read_word(DS.map_selector, address);
}

// Far entry 01f7:1c6e.  The raw MAP word is tested directly; the helper's
// meaningful ABI is ZF, not a C boolean return value.
Flags probe_map_word_bit_4000(int16_t y, int16_t x) {
    uint16_t word = map_word_at_pixel(y, x);
    return {(word & 0x4000) == 0, false};
}

// Far entry 01f7:1c92.  This is the corresponding 0x1000 test used by the
// vertical probes; it does not read the descriptor table.
Flags probe_map_word_bit_1000(int16_t y, int16_t x) {
    uint16_t word = map_word_at_pixel(y, x);
    return {(word & 0x1000) == 0, false};
}

// Far entry 01f7:5cc3.  The descriptor is returned in DX by the original.
uint16_t read_descriptor_word(int16_t y, int16_t x) {
    uint16_t cell = map_word_at_pixel(y, x);
    return descriptor_word_at(cell & 0x01ff);
}

// Far entry 01f7:5c27.  The low descriptor nibble is selected by coordinate
// quadrant.  `zf=true` is the nonblocking/clear result.
Flags probe_descriptor_quadrant(int16_t y, int16_t x) {
    uint16_t map_address = static_cast<uint16_t>(
        DS.map_offset + (static_cast<uint16_t>(y) >> 4) * DS.map_row_stride
        + ((static_cast<uint16_t>(x) >> 3) & 0xfffe));
    uint16_t cell = far_read_word(DS.map_selector, map_address);
    uint16_t descriptor = descriptor_word_at(cell & 0x01ff);
    uint16_t mask;

    if ((descriptor & 0x000f) == 0)
        return {true, false};                 // raw TEST/JZ clear return

    if ((y & 8) != 0) {
        mask = ((x & 8) != 0) ? 0x02 : 0x01;
    } else {
        mask = ((x & 8) != 0) ? 0x04 : 0x08;
    }
    bool occupied = (descriptor & mask) != 0;
    return {!occupied, false};
}

static bool occupied(int16_t y, int16_t x) {
    return !probe_descriptor_quadrant(y, x).zf;
}

// --------------------------- Input helpers -------------------------------

static uint16_t action_bit_for_scan_code(uint8_t scan) {
    switch (scan & 0x7f) {
    case 0x4b: return 0x0008; // left
    case 0x4d: return 0x0004; // right
    case 0x50: return 0x0001; // down
    case 0x48: return 0x0002; // up
    case 0x38: return 0x0010; // alternate / left Alt
    case 0x39: return 0x0020; // space / confirm
    default:   return 0;
    }
}

// The ring producer at f17f reads port 0x60.  f1a8 consumes its 32-byte
// ring, stores the last seven-bit scan in DS:88ba, and applies this make/
// break operation to DS:88bc.  The ring-head mechanics are intentionally
// left as the observed producer/consumer boundary rather than inventing a
// C representation for the FFFF:501e real-mode ring.
void poll_keyboard_ring_to_action_bits(uint8_t raw_scan) {
    uint16_t bit = action_bit_for_scan_code(raw_scan);
    if (bit == 0)
        return;
    DS.last_scan_code = raw_scan & 0x7f;
    if ((raw_scan & 0x80) != 0)
        DS.keyboard_actions &= static_cast<uint16_t>(~bit);
    else
        DS.keyboard_actions |= bit;
}

// Far entry 01f7:f21b.  f1a8 has already converted make/break scan codes.
uint16_t read_normalized_action_bits() {
    return DS.keyboard_actions | DS.secondary_actions;
}

// Direct callback entry at 01f7:f21c.  The raw body is the normalized-input
// expression above; the computed-jump tail remains address-qualified in the
// Ghidra export because the segment image ends before its speculative flows.
uint16_t input_dispatch_f21C() {
    return read_normalized_action_bits();
}

// --------------------------- Collision helpers ---------------------------

// Near entry 01f7:3971.
Flags probe_vertical_10px(PlayerRecord* p) {
    return probe_map_word_bit_1000(
        static_cast<int16_t>(p->y_pixel() - 0x0a - p->u16(0x72)),
        p->x_pixel());
}

// Near entry 01f7:3986.
Flags probe_vertical_step(PlayerRecord* p) {
    return probe_map_word_bit_1000(
        static_cast<int16_t>(p->y_pixel() - p->u16(0x72)),
        p->x_pixel());
}

// Near entry 01f7:3998.  The last probe's flags are returned.  When the
// configured step is <= 0x20 the third probe is replaced by CMP AL,AL.
Flags probe_forward_surface(PlayerRecord* p) {
    int16_t dx = (static_cast<int8_t>(p->u8(0x29)) < 0) ? -10 : 10;
    int16_t x = static_cast<int16_t>(p->x_pixel() + dx);

    if (p->u8(0x3a) == 0) {
        Flags first = probe_descriptor_quadrant(
            static_cast<int16_t>(p->y_pixel() - 1), x);
        if (!first.zf)
            return first;
        Flags second = probe_descriptor_quadrant(
            static_cast<int16_t>(p->y_pixel() - 0x11), x);
        if (!second.zf)
            return second;
        if (p->u16(0x72) <= 0x20)
            return {true, false};
        return probe_descriptor_quadrant(
            static_cast<int16_t>(p->y_pixel() - 0x21), x);
    }
    return {true, false};
}

// Near entry 01f7:3a1f.  This reproduces the flag result, including the
// side-latch fallback used when either side is occupied.
Flags probe_player_side_clear(PlayerRecord* p) {
    // 3a1f: the first quadrant probe is at 3a39/3a3e and the +5 retry is
    // at 3a47/3a50.  The second clear result writes +3b at 3a52.
    if (p->gate() == 0 && p->mode() != static_cast<int8_t>(-1)) {
        if (!occupied(p->y_pixel(), static_cast<int16_t>(p->x_pixel() - 5))) {
            if (!occupied(p->y_pixel(), static_cast<int16_t>(p->x_pixel() + 5))) {
                p->u8(0x3b, 0xff);
                return {true, false};
            }
        }
    }
    return {p->u8(0x3b) == 0, false};
}

// Near entry 01f7:3df2.  The write is to +08 (the integer Y word), not +04.
void snap_player_y_on_side_contact(PlayerRecord* p) {
    if (p->u8(0x3b) == 0 || p->u8(0x3a) != 0)
        return;
    if (occupied(p->y_pixel(), static_cast<int16_t>(p->x_pixel() - 5)) ||
        occupied(p->y_pixel(), static_cast<int16_t>(p->x_pixel() + 5))) {
        p->y_pixel(static_cast<int16_t>(p->y_pixel() & 0xfff8));
    }
}

// Near entry 01f7:3d02.  The source of the response arithmetic is the dword
// at +0x0a.  The original instruction performs an arithmetic 32-bit shift.
static Fixed16 arithmetic_shift_right_1(Fixed16 value) {
    int64_t wide = value;
    return static_cast<Fixed16>(wide >= 0 ? wide / 2 : -((-wide + 1) / 2));
}

Flags apply_descriptor_vertical_correction(PlayerRecord* p) {
    if (p->u8(0x3b) == 0)
        return {true, false};
    int16_t original_y = p->y_pixel();
    // 3d0c clears the transient latch before either descriptor read.
    p->u8(0x3a, 0);
    uint16_t descriptor = read_descriptor_word(p->y_pixel(), p->x_pixel());

    if ((descriptor & 0x0030) == 0) {
        p->y_pixel(static_cast<int16_t>(p->y_pixel() - 8));
        descriptor = read_descriptor_word(p->y_pixel(), p->x_pixel());
        if ((descriptor & 0x0030) == 0) {
            p->y_pixel(static_cast<int16_t>(p->y_pixel() + 8));
            return {(descriptor & 0x0030) == 0, false};
        }
    }

    int16_t phase;
    if ((descriptor & 0x0020) != 0) {
        if (p->mode() == 0)
            p->vy(arithmetic_shift_right_1(p->vy()));
        p->u8(0x3a, 0xff);
        phase = static_cast<int16_t>((p->x_pixel() & 0x000f) >> 1);
    } else {
        if (p->mode() == 0)
            p->vy(arithmetic_shift_right_1(-p->vy()));
        p->u8(0x3a, 1);
        phase = static_cast<int16_t>((0x000f - (p->x_pixel() & 0x000f)) >> 1);
    }

    int16_t target_y = static_cast<int16_t>((p->y_pixel() & 0xfff0) + phase);
    if ((descriptor & 0x0040) == 0)
        target_y = static_cast<int16_t>(target_y + 8); // SUB BX,0xfff8

    if (original_y < target_y) {
        // 3de0 cancels an otherwise selected response when the target is
        // beyond the original Y.  The clear return is 3de4.
        p->u8(0x3a, 0);
        return {true, false};
    }
    if (original_y != target_y)
        p->y_pixel(target_y);
    return {false, false};               // response path sets AL=1, ZF=0
}

// Near entry 01f7:3a62.
void apply_action_contact_side_effect(PlayerRecord* p) {
    if (p->mode() == 0 && (p->action() & 0x0001) != 0) {
        p->u8(0x2a, 0xff);
        p->u16(0x12, (static_cast<int8_t>(p->u8(0x28)) >= 0) ? 0x0026 : 0x0058);
    }
}

// Far entry 01f7:3a8a.
void dispatch_special_tile_contact(PlayerRecord* p) {
    if (p->mode() > 0) {
        uint16_t tile = map_tile_id_at_pixel(p->y_pixel(), p->x_pixel());
        if (tile == 0x0b || tile == 0x0c || tile == 0x0d) {
            apply_tile_transition_1B07();
            apply_transition_reset_19E6();
        }
    }
}

// Far entry 01f7:5d38.  SI is an offset into the resident animation table;
// animation_sequence_word is a raw segment-word read, not an engine API.
void load_animation_descriptor(PlayerRecord* p, uint16_t sequence_si) {
    uint16_t delay = static_cast<uint16_t>(animation_sequence_word(sequence_si));
    p->u16(0x1e, delay);
    p->u16(0x20, delay);
    p->u16(0x22, static_cast<uint16_t>(sequence_si + 2));
    p->u16(0x24, static_cast<uint16_t>(sequence_si + 2));

    int16_t frame = animation_sequence_word(static_cast<uint16_t>(sequence_si + 2));
    if (p->u8(0x28) == 0xff)
        frame = static_cast<int16_t>(frame + 0x32);
    p->u16(0x12, static_cast<uint16_t>(frame));
}

// Far entry 01f7:5d60.  Negative table words are signed cursor-relative
// jumps.  The signed loop is deliberately retained instead of being reduced
// to a modulo operation because the native tables contain a -3 terminal loop.
void advance_animation_descriptor(PlayerRecord* p) {
    if (p->u16(0x20) != 0) {
        p->u16(0x20, static_cast<uint16_t>(p->u16(0x20) - 1));
        return;
    }

    p->u16(0x24, static_cast<uint16_t>(p->u16(0x24) + 2));
    uint16_t cursor = p->u16(0x24);
    int16_t frame;
    do {
        frame = animation_sequence_word(cursor);
        if (frame < 0) {
            cursor = static_cast<uint16_t>(cursor + frame * 2);
            p->u16(0x24, cursor);
        }
    } while (frame < 0);

    if (p->u8(0x28) == 0xff)
        frame = static_cast<int16_t>(frame + 0x32);
    p->u16(0x12, static_cast<uint16_t>(frame));
    p->u16(0x20, p->u16(0x1e));
}

// --------------------------- Horizontal motion ---------------------------

// Near entry 01f7:3ab9.
void integrate_horizontal_motion(PlayerRecord* p) {
    if ((p->action() & 0x0004) != 0)
        p->u8(0x28, 1);
    else if ((p->action() & 0x0008) != 0)
        p->u8(0x28, 0xff);

    p->x(p->x() + p->vx());
    Fixed16 motion_sign = p->vx() + DS.external_x_delta;
    if (motion_sign == 0 || motion_sign == 1)
        p->u8(0x29, p->u8(0x28));
    else
        p->u8(0x29, motion_sign < 0 ? 0xff : 1);

    Flags surface = probe_forward_surface(p);
    if (!surface.zf) {
        p->vx(0);
        if (p->u8(0x36) == 0 && p->mode() == 0) {
            p->u8(0x36, 1);
            p->u8(0x13, 0);
            load_animation_descriptor(p, 0x3156); // animation_sequence_3156
        }
        goto animation_tail;
    }

    p->x(p->x() + DS.external_x_delta);
    DS.external_x_delta = 0;

    if (static_cast<int8_t>(p->u8(0x3a)) < 0 && static_cast<int8_t>(p->u8(0x29)) >= 0)
        p->y_pixel(static_cast<int16_t>(p->y_pixel() + 2));
    if (static_cast<int8_t>(p->u8(0x3a)) > 0 && static_cast<int8_t>(p->u8(0x29)) < 0)
        p->y_pixel(static_cast<int16_t>(p->y_pixel() + 2));

    if ((p->action() & 0x0004) != 0) {
        Fixed16 next = p->vx() + p->i32(0x4c);
        p->vx(next > p->i32(0x5c) ? p->i32(0x5c) : next);
        if (p->u8(0x36) != 0 && p->mode() == 0) {
            p->u8(0x36, 0);
            load_animation_descriptor(p, 0x3142); // animation_sequence_3142
        }
    } else if ((p->action() & 0x0008) != 0) {
        Fixed16 next = p->vx() - p->i32(0x4c);
        p->vx(next < -p->i32(0x5c) ? -p->i32(0x5c) : next);
        if (p->u8(0x36) != 0 && p->mode() == 0) {
            p->u8(0x36, 0);
            load_animation_descriptor(p, 0x3142); // animation_sequence_3142
        }
    } else {
        if (p->mode() == 0 && p->i16(0x0c) == 0 && DS.idle_counter < 0x00d2)
            load_animation_descriptor(p, 0x3156); // animation_sequence_3156
        if (p->u8(0x36) == 0)
            p->u8(0x36, 1);
        if (p->vx() > 0)
            p->vx(p->vx() > p->i32(0x54) ? p->vx() - p->i32(0x54) : 0);
        else if (p->vx() < 0)
            p->vx(p->vx() < -p->i32(0x54) ? p->vx() + p->i32(0x54) : 0);
    }

    if (static_cast<int8_t>(p->u8(0x3a)) > 0 &&
        p->y() - p->i32(0x44) < 0) {
        Flags below = probe_vertical_step(p);
        if (!below.zf) {
            p->vx(0);
            p->y(p->i32(0x44));
            p->x(p->i32(0x48));
            if (DS.idle_counter < 0x00d2)
                load_animation_descriptor(p, 0x3156); // animation_sequence_3156
        }
    }

animation_tail:
    if (p->mode() != 0) {
        p->u8(0x13, 0);
        return;
    }
    Fixed16 speed = p->vx() < 0 ? -p->vx() : p->vx();
    if (speed < 0x28000 && p->u8(0x13) != 0) {
        p->u8(0x13, 0);
        load_animation_descriptor(p, 0x3142); // animation_sequence_3142
    }
    if (speed >= 0x28000 && p->u8(0x13) != 0xff) {
        p->u8(0x13, 0xff);
        load_animation_descriptor(p, 0x3190); // animation_sequence_3190
    }
}

// Near entry 01f7:3e41.  The X part only contributes near the camera's
// horizontal window; the Y part follows the two raw 0x96/0x5a thresholds and
// advances DS:4fe6 toward DS:4fe4.  The returned words are promoted by the
// original code with a left shift of 16 and clamped to +/-0x40000.
static Fixed16 shift_word_to_fixed(uint16_t word) {
    return static_cast<Fixed16>(static_cast<uint32_t>(word) << 16);
}

static Fixed16 clamp_view_delta(Fixed16 value) {
    if (value >= 0x40000)
        return 0x40000;
    if (value <= static_cast<Fixed16>(0xfffc0000u))
        return static_cast<Fixed16>(0xfffc0000u);
    return value;
}

ViewDelta compute_view_delta(PlayerRecord* p) {
    Fixed16 x_delta = 0;
    int16_t camera_relative_x = static_cast<int16_t>(
        p->x_pixel() - static_cast<int16_t>(DS.camera_x));

    if (camera_relative_x >= 0x00be) {
        int16_t anchor_delta = static_cast<int16_t>(
            p->x_pixel() - p->view_anchor_x());
        if (anchor_delta >= 0)
            x_delta = shift_word_to_fixed(static_cast<uint16_t>(anchor_delta));
    } else {
        camera_relative_x = static_cast<int16_t>(
            p->x_pixel() - static_cast<int16_t>(DS.camera_x));
        if (camera_relative_x <= 0x0082) {
            int16_t anchor_delta = static_cast<int16_t>(
                p->x_pixel() - p->view_anchor_x());
            if (anchor_delta < 0)
                x_delta = shift_word_to_fixed(static_cast<uint16_t>(anchor_delta));
        }
    }

    int16_t y_delta = static_cast<int16_t>(
        p->y_pixel() - static_cast<int16_t>(DS.camera_y));
    int16_t phase = static_cast<int16_t>(0x0096 - DS.view_state_b);
    if (phase < y_delta) {
        y_delta = static_cast<int16_t>(y_delta - phase);
    } else {
        phase = static_cast<int16_t>(0x005a - DS.view_state_b);
        if (phase > y_delta)
            y_delta = static_cast<int16_t>(y_delta - phase);
        else
            y_delta = 0;
    }
    y_delta = static_cast<int16_t>(y_delta - DS.view_state_b + DS.view_state_a);
    DS.view_state_b = DS.view_state_a;

    Fixed16 y_fixed = shift_word_to_fixed(static_cast<uint16_t>(y_delta));
    return {clamp_view_delta(x_delta), clamp_view_delta(y_fixed)};
}

// Far entry 01f7:20af.  The caller already supplies the fixed-point pair;
// this leaf only applies the native lower Y bound before publishing it.
void publish_view_delta(Fixed16 eax, Fixed16 ebx) {
    if (ebx < static_cast<Fixed16>(-0x40000))
        ebx = static_cast<Fixed16>(-0x40000);
    DS.published_view_y = ebx;
    DS.published_view_x = eax;
}

// Far entry 01f7:199d.  The relocated 0ce3 call is retained as an external
// contract because it dispatches transition effects outside this closure.
void write_transition_gate_199D(PlayerRecord* p) {
    DS.transition_effect_bits = 0;
    DS.collision_transition_mode = -1;
    dispatch_transition_effect_0CE3();
    p->vy(-0x20000);
    p->i32(0x4c, 0x2000);
    p->i32(0x50, 0x2000);
    p->i32(0x5c, 0x18000);
    p->i32(0x60, 0x40000);
    --DS.transition_object_counter;
    DS.transition_scratch = 0;
}

// Far entry 01f7:3e30.  This adjacent setter is not called by the ordinary
// callback body, but it is part of the same player state-write closure.
void commit_external_player_y_and_gate(Fixed16 eax) {
    PlayerRecord* p = player_from_pool_offset(DS.player_offset);
    p->u8(0x38, 0xff);
    p->y(eax);
}

// --------------------------- Contact tile helpers ------------------------

static bool is_contact_tile_8_to_10(uint16_t tile) {
    return tile == 8 || tile == 9 || tile == 10;
}

static bool is_contact_tile_5_to_7(uint16_t tile) {
    return tile == 5 || tile == 6 || tile == 7;
}

// The common write sequence in 6370/648e.  It is intentionally separate so
// the two entry points can retain their different probe coordinates and CF.
static void commit_contact(PlayerRecord* p, bool negative_mode) {
    DS.pending_event = 7;
    dispatch_pending_sound_effect();
    object_pool_factory_0E06(0x6328, 0);
    p->u8(0x38, DS.contact_subtype);
    p->u8(0x2a, DS.contact_code);
    p->x(p->x() + (static_cast<Fixed16>(DS.contact_x_offset) << 16));
    p->y(static_cast<Fixed16>(DS.contact_y_scratch) << 16);
    p->u16(0x32, 0);
    p->i16(0x2e, 0);
    p->u8(0x36, 0);
    if (negative_mode)
        ; // original sets CF immediately before returning
}

// Near entry 01f7:6370.  6484 sets DS:5003=5 and forwards here.
Flags probe_contact_tile_offset(PlayerRecord* p) {
    bool negative_mode = p->mode() < 0;
    uint16_t tile;

    if (negative_mode) {
        tile = map_tile_id_at_pixel(
            static_cast<uint16_t>(p->y_pixel() - p->u16(0x72)),
            static_cast<uint16_t>(p->x_pixel() + DS.contact_x_offset));
        if (!is_contact_tile_8_to_10(tile))
            return {false, false};
    } else {
        tile = map_tile_id_at_pixel(
            p->y_pixel(), static_cast<uint16_t>(p->x_pixel() + DS.contact_x_offset));
        if (!is_contact_tile_8_to_10(tile)) {
            DS.contact_subtype = 0xff;
            if (!is_contact_tile_5_to_7(tile))
                return {false, false};
        }
    }

    commit_contact(p, negative_mode);
    return {false, negative_mode};
}

// Far entry 01f7:6484.
Flags probe_contact_plus5(PlayerRecord* p) {
    DS.contact_x_offset = 5;
    return probe_contact_tile_offset(p);
}

// Far entry 01f7:648e.  Its ordinary path uses literal x+5; its negative path
// uses the same x+5 and y-step arrangement before the ordinary fallback.
Flags probe_contact_right(PlayerRecord* p) {
    bool negative_mode = p->mode() < 0;
    uint16_t tile;

    if (negative_mode) {
        tile = map_tile_id_at_pixel(
            static_cast<uint16_t>(p->y_pixel() - p->u16(0x72)),
            static_cast<uint16_t>(p->x_pixel() + 5));
        if (!is_contact_tile_8_to_10(tile))
            return {false, false};
    } else {
        tile = map_tile_id_at_pixel(
            p->y_pixel(), static_cast<uint16_t>(p->x_pixel() + 5));
        if (!is_contact_tile_8_to_10(tile)) {
            DS.contact_subtype = 0xff;
            if (!is_contact_tile_5_to_7(tile))
                return {false, false};
        }
    }

    DS.contact_x_offset = 5;
    commit_contact(p, negative_mode);
    return {false, negative_mode};
}

// --------------------------- Initialization -------------------------------

// Near entry 01f7:3f27.
void initialize_player(PlayerRecord* p) {
    DS.player_offset = player_pool_offset(p); // raw code writes DI
    DS.input_run_counter = 0;
    DS.deferred_y = 0;
    DS.timer_clear = 0;
    DS.horizontal_accumulator = 0;
    DS.view_state_b = 0;
    DS.idle_counter = 0x00d3;

    p->vx(0);
    p->vy(0);
    p->u8(0x37, 0);
    p->u16(0x34, 0);
    p->action(0);
    p->u8(0x3a, 0);
    p->y(p->y() & static_cast<Fixed16>(0xfff00000u));
    p->u16(0x72, 0x28);

    p->i32(0x4c, 0x2800);
    p->i32(0x50, 0x2800);
    p->i32(0x54, 0x2000);
    p->i32(0x58, 0x2000);
    p->i32(0x5c, 0x18000);
    p->i32(0x60, 0x40000);
    p->i32(0x64, static_cast<Fixed16>(0xfffb6000));

    p->i16(0x2c, -10);
    p->i16(0x30, 10);
    p->i16(0x2e, static_cast<int16_t>(p->u16(0x72)));
    p->u16(0x32, 0);
    p->u8(0x3b, 1);
    p->u8(0x38, 0xff);
    p->u8(0x36, 0xff);
    p->u8(0x28, 1);
    p->u8(0x29, 1);
    p->u16(0x18, 0x3ff8);

    if (DS.activation_state <= 0)
        load_animation_descriptor(p, 0x316a); // animation_sequence_316A
}

// --------------------------- Callback ------------------------------------

static void update_horizontal_accumulators(PlayerRecord* p, uint16_t& action) {
    if ((action & 1) != 0 && p->mode() == 0) {
        action &= ~0x000c;
        p->i16(0x2e, static_cast<int16_t>(-(p->u16(0x72) >> 1)));
        ++DS.input_run_counter;
        if (DS.input_run_counter >= 0x3c) {
            --DS.input_run_counter;
            DS.horizontal_accel += 0x1000;
            if (DS.horizontal_accel > 0x18000)
                DS.horizontal_accel = 0x18000;
            if (DS.horizontal_accumulator > 0x200000) {
                DS.horizontal_accel -= 0x2000;
                if (DS.horizontal_accel < 0)
                    DS.horizontal_accel = 0;
            }
            DS.horizontal_accumulator += DS.horizontal_accel;
        }
    } else {
        DS.input_run_counter = 0;
        action &= ~1;
        p->i16(0x2e, static_cast<int16_t>(-p->u16(0x72)));
        if (p->u8(0x2a) != 0) {
            if (DS.horizontal_accel != 0)
                goto ramp_from_40e2;
            p->u8(0x2a, 0);
        }
    }

    goto common_accel_tail_4159;

ramp_from_40e2:
    DS.horizontal_accel += 0x1000;
    if (DS.horizontal_accel > 0x18000)
        DS.horizontal_accel = 0x18000;
    if (DS.horizontal_accumulator > 0x200000) {
        DS.horizontal_accel -= 0x2000;
        if (DS.horizontal_accel < 0)
            DS.horizontal_accel = 0;
    }
    DS.horizontal_accumulator += DS.horizontal_accel;

common_accel_tail_4159:
    if (DS.horizontal_accumulator > 0) {
        DS.horizontal_accel -= 0x1000;
        if (DS.horizontal_accel < -0x18000)
            DS.horizontal_accel = -0x18000;
        if (DS.horizontal_accumulator < 0x100000) {
            DS.horizontal_accel += 0x2000;
            if (DS.horizontal_accel > 0)
                DS.horizontal_accel = 0;
        }
        DS.horizontal_accumulator += DS.horizontal_accel;
    }
}

// Near entry 01f7:38ca.  The callback reaches this helper in the common
// tail.  The branch at 38e2 is an alternate entry in the surrounding raw
// code; it is not reached from 38ca and is not folded into this contract.
void apply_special_speed_cap_38CA(PlayerRecord* p) {
    if (p->mode() == 0 && DS.special_speed_cap_mode == 1)
        p->i32(0x5c, 0x30000);
}

// Near entry 01f7:38ec.  4519 leaves ES:DI pointing at the new child when a
// slot is allocated and leaves DI unchanged when allocation is rejected.
// That ABI detail is retained as an explicit pointer result rather than
// being represented as an engine-level effect abstraction.
void player_action_effect_38EC(PlayerRecord* p) {
    if ((p->action() & 0x0010) == 0) {
        p->u8(0x3c, 0);
        return;
    }
    if (p->u8(0x3c) != 0)
        return;

    p->u8(0x3c, 0xff);
    PlayerRecord* child = spawn_contact_effect_entry(p); // 4519
    if (child == p)
        return; // static fact: failed allocation preserves the owner DI

    child->u8(0x29, p->u8(0x28));
    child->u8(0x17, 1);
    child->y(p->y());
    child->x(p->x());
}

static void common_player_tail(PlayerRecord* p) {
    // 38ca and 38ec are local state helpers whose internal fields are outside
    // this primary closure; their calls and ordering are retained.
    apply_special_speed_cap_38CA(p);      // 38ca
    player_action_effect_38EC(p);        // 38ec
    integrate_horizontal_motion(p); // 3ab9
    advance_animation_descriptor(p);       // 5d60
    apply_action_contact_side_effect(p);  // 3a62

    ViewDelta delta = compute_view_delta(p);
    publish_view_delta(delta.eax, delta.ebx);
    p->u8(0x38, 0);

    if (p->u16(0x34) != 0) {
        p->u16(0x34, static_cast<uint16_t>(p->u16(0x34) - 1));
        if (p->u16(0x34) == 0)
            DS.timer_clear = 0;
        if ((p->u16(0x34) & 2) != 0)
            p->u16(0x12, p->u16(0x12) | 0x8000);
    }

    if (static_cast<int16_t>(p->y_pixel() - DS.camera_y) >=
        static_cast<int16_t>(DS.camera_y_limit))
        write_transition_gate_199D(p);     // 199d

    if (p->mode() == 0 && p->action() == 0) {
        if (DS.idle_counter < 0x00d2)
            ++DS.idle_counter;
        else
            load_animation_descriptor(p, 0x31ba); // animation_sequence_31BA
    } else {
        DS.idle_counter = 0;
    }

    if (p->mode() == 0 && DS.action_suppressor == -1)
        load_animation_descriptor(p, 0x316a); // animation_sequence_316A
}

// Near entry 01f7:3ff8.  This is the complete recovered branch ordering.
void update_player(PlayerRecord* p) {
    uint16_t action = 0;
    player_helper_5937();                   // 5937; unresolved

    if (DS.collision_transition_mode != 0)
        goto transition_block_4416;

    if (probe_contact_right(p).cf)
        goto early_contact_41c1;
    if (probe_contact_plus5(p).cf)
        goto early_contact_41c1;
    dispatch_special_tile_contact(p);       // 3a8a

    p->u16(0x12, p->u16(0x12) & 0x0fff);
    p->i32(0x44, p->y());                   // saved Y
    p->i32(0x48, p->x());                   // saved X

    if (DS.deferred_y != 0) {
        p->u8(0x38, 0xff);
        p->y(p->y() + (static_cast<Fixed16>(DS.deferred_y) + 1));
        DS.deferred_y = 0;
    }

    action = DS.action_source;
    if (DS.activation_state <= 0) {
        action = input_dispatch_f21C();
        DS.action_low_copy = static_cast<uint8_t>(action);
    }
    if (p->u8(0x39) != 0) {
        p->u8(0x39, 0);
        p->u8(0x37, 1);
        p->u16(0x3e, 0);
        action |= 0x0022;
    }
    if (DS.action_suppressor != 0)
        action = 0;
    p->action(action);

    // 406d..40a1: +39 is consumed before the action suppressor; +40 is
    // cleared at 409b only for an action without 0x22 and incremented at
    // 40a1 on every ordinary callback.
    if ((action & 0x0022) == 0)
        p->u16(0x40, 0);
    p->u16(0x40, static_cast<uint16_t>(p->u16(0x40) + 1));

    if (p->gate() != 0 && p->mode() != 0)
        goto grounded_contact_427f;

    update_horizontal_accumulators(p, action);
    p->action(action);

    if (p->mode() == 0)
        goto ordinary_mode_42b4;
    if (p->mode() < 0)
        goto negative_mode_4323;
    goto positive_mode_41e8;

early_contact_41c1:
    p->u16(0x3e, 0x03e7);
    goto contact_response_41cf;

early_contact_41c9:
    p->u16(0x3e, 0);

contact_response_41cf:
    p->u8(0x37, 1);
    p->vy(0);
    load_animation_descriptor(p, 0x3186);   // animation_sequence_3186 via 5d38
    goto common_tail_4384;

positive_mode_41e8:
    p->u16(0x3e, static_cast<uint16_t>(p->u16(0x3e) + 1));
    if (!occupied(p->y_pixel(), static_cast<int16_t>(p->x_pixel() - 5)) &&
        !occupied(p->y_pixel(), static_cast<int16_t>(p->x_pixel() + 5))) {
        Flags vertical = probe_map_word_bit_4000(p->y_pixel(), p->x_pixel());
        if (!vertical.zf)
            p->y_pixel(static_cast<int16_t>(p->y_pixel() & 0xfff0));
    }
    if ((action & 0x0022) != 0 && p->u16(0x40) <= 0x13 &&
        p->u16(0x3e) < 0x0a)
        goto grounded_contact_427f;
    p->u8(0x2b, 0);
    if (p->gate() == 0) {
        apply_descriptor_vertical_correction(p); // 3d02
    }
    if (p->u8(0x3a) != 0)
        goto grounded_contact_427f;
    p->vy(p->vy() + p->i32(0x50));
    if (p->vy() > p->i32(0x60))
        p->vy(p->i32(0x60));
    p->y(p->y() + p->vy());
    if (probe_player_side_clear(p).zf)
        goto common_tail_4384;
    goto grounded_contact_427f;

grounded_contact_427f:
    if (p->gate() == 0) {
        apply_descriptor_vertical_correction(p); // 3d02
        snap_player_y_on_side_contact(p);        // 3df2
    }
    p->vy(0);
    p->u8(0x37, 0);
    if (DS.idle_counter < 0x00d2)
        load_animation_descriptor(p, 0x3156); // animation_sequence_3156
    p->u8(0x36, 1);
    goto common_tail_4384;

ordinary_mode_42b4:
    if (!probe_player_side_clear(p).zf)
        goto ordinary_correction_42c9;
    if (p->gate() != 0)
        goto ordinary_correction_42c9;
    if (p->u8(0x3a) == 0)
        goto early_contact_41c9;

ordinary_correction_42c9:
    snap_player_y_on_side_contact(p);        // 3df2
    apply_descriptor_vertical_correction(p); // 3d02
    if ((action & 0x0022) == 0)
        goto common_tail_4384;
    if (p->gate() != 0 || p->u16(0x40) > 0x0d)
        goto common_tail_4384;
    if (!probe_vertical_10px(p).zf)
        goto common_tail_4384;

    DS.pending_event = 0;
    dispatch_pending_sound_effect();       // 01e7:0fcf
    p->u16(0x3e, 0x03e8);
    p->u8(0x3b, 0);
    p->u8(0x3a, 0);
    p->u8(0x37, 0xff);
    p->vy(p->i32(0x64));
    load_animation_descriptor(p, 0x3160);  // animation_sequence_3160
    goto common_tail_4384;

negative_mode_4323:
    if (!probe_vertical_step(p).zf)
        goto early_contact_41c1;
    {
        Fixed16 next = p->vy() + p->i32(0x58);
        // 4334..4348: release clamping is disabled while the callback still
        // sees action 0x22 or the contact scratch is nonzero.  The compare is
        // signed and clamps only values strictly below -0x20000.
        if (p->u8(0x2b) == 0 && (p->action() & 0x0022) == 0 &&
            next < -0x20000)
            next = -0x20000;
        if (next >= 0)
            goto early_contact_41c1;
        p->vy(next);
        p->y(p->y() + p->vy());
        if (probe_vertical_step(p).zf)
            goto common_tail_4384;
        goto early_contact_41c1;
    }
    // 0x436b..0x437f is a dead/unreferenced response fragment in the raw
    // closure; the reachable 0x4323 path branches to 4384 or 41c1 above.

common_tail_4384:
    common_player_tail(p);
    return;

transition_block_4416:
    if (DS.collision_transition_mode == -1) {
        p->vy(-0x20000);
        DS.transition_scratch = 0;
        load_animation_descriptor(p, 0x31a4); // animation_sequence_31A4 via 5d38
    }
    advance_animation_descriptor(p);       // 5d60
    publish_view_delta(0, 0);               // 20af, EAX=EBX=0 at 4439
    p->vy(p->vy() + 0x1800);
    if (p->vy() > 0x20000)
        p->vy(0x20000);

    if (static_cast<int8_t>(p->u8(0x29)) <= 0) {
        uint16_t d0 = read_descriptor_word(p->y_pixel(), p->x_pixel());
        uint16_t d1 = read_descriptor_word(
            static_cast<int16_t>(p->y_pixel() - 16), p->x_pixel());
        if ((d0 & 0x0070) == 0 && (d1 & 0x0070) == 0) {
            Flags transition = probe_transition_descriptor(0, 0);
            if (transition.cf)
                goto transition_hit_44dc;
            p->y(p->y() + p->vy());
            p->x(p->x() - 0x5000);
        } else {
            goto transition_hit_44dc;
        }
    } else {
        // The +0x29-positive branch performs the same current/Y-16 descriptor
        // pair before the transition query; only the signed input differs.
        uint16_t d0 = read_descriptor_word(p->y_pixel(), p->x_pixel());
        uint16_t d1 = read_descriptor_word(
            static_cast<int16_t>(p->y_pixel() - 16), p->x_pixel());
        if ((d0 & 0x0070) != 0 || (d1 & 0x0070) != 0)
            goto transition_hit_44dc;
        Flags transition = probe_transition_descriptor(0, 0);
        if (transition.cf)
            goto transition_hit_44dc;
        p->y(p->y() + p->vy());
        p->x(p->x() - 0x5000);
    }
    return;

transition_hit_44dc:
    --DS.collision_transition_mode;
    if (DS.collision_transition_mode >= -0x50) {
        if (DS.collision_transition_mode < -0x14)
            return;
        return;
    }
    if (DS.collision_transition_mode < -0x15d)
        DS.transition_state = -1;
    return;
}
