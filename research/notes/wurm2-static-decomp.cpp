// Focused Ghidra reconstruction of the WURM2 closure in QUIKY.EXE segment
// 01F7.  This is an address/field-oriented C-like record of the two ARE
// wrappers, their shared initializer, and the complete callback.  It is not
// a native object implementation and it deliberately keeps unknown fields
// address-named.
//
// Source executable SHA-256:
//   c9b2e59febd6fa0ea271bedf360459353f55c74444f026964b70988d6de1bca1
// Static source: Ghidra x86:LE:16:Protected Mode, raw NE segment import,
// no-analysis annotation, and NE relocation resolution.  The reproducible
// export is run_player_external_closure.py, v39 for the captured listing.

#include <cstdint>

struct Flags {
    bool cf;
    bool zf;
};

struct ContactResult {
    Flags flags;
    std::uint16_t ax_code; // 1B77 also returns AX=0/1/2; this call ignores it.
};

struct Wurm2Record {
    std::uint8_t bytes[0x78];

    std::uint8_t u8(std::uint16_t offset) const;
    void u8(std::uint16_t offset, std::uint8_t value);
    std::uint16_t u16(std::uint16_t offset) const;
    void u16(std::uint16_t offset, std::uint16_t value);
    std::int32_t i32(std::uint16_t offset) const;
    void i32(std::uint16_t offset, std::int32_t value);
};

struct Wurm2Globals {
    std::uint16_t random_cursor_6468; // DS:6468; word cursor, masked to FF
    std::int8_t random_bytes_646c[0x100]; // DS:646C
    std::uint16_t active_target_count_8806; // DS:8806
    std::uint16_t target_capacity_8808; // DS:8808
    std::uint8_t target_rows_79de[0x200]; // DS:79DE; four-byte rows

    std::uint16_t target_x(std::uint16_t index) const;
    std::uint16_t target_y(std::uint16_t index) const;
    void clear_target_x(std::uint16_t index);
};

extern Wurm2Globals DS;

// These declarations preserve machine contracts.  In particular, the
// caller-side CF uses are not inferred from a C/C++ boolean return type.
extern void load_animation_descriptor_5d38(Wurm2Record &object,
                                            std::uint16_t sequence);
extern void advance_animation_descriptor_5d60(Wurm2Record &object);
extern Flags object_camera_visibility_gate_1dca(Wurm2Record &object);
extern void deactivate_object_outside_camera_1dee(Wurm2Record &object);
extern ContactResult player_contact_damage_overlap_1b77(
    std::int16_t ax_offset, std::int16_t bx_offset,
    std::int16_t cx_extent, std::int16_t dx_extent,
    Wurm2Record &object);
extern Flags descriptor_probe_oriented_1c4d(
    std::int16_t x_offset, std::int16_t y_offset,
    Wurm2Record &object);
extern Flags map_descriptor_quadrant_test_5c27(
    std::int16_t y_pixels, std::int16_t x_pixels);

static std::int32_t wrap_add_i32(std::int32_t left, std::int32_t right) {
    return static_cast<std::int32_t>(
        static_cast<std::uint32_t>(left) +
        static_cast<std::uint32_t>(right));
}

static std::int32_t clamp_i32(std::int32_t value,
                              std::int32_t low,
                              std::int32_t high) {
    return value < low ? low : (value > high ? high : value);
}

static std::int8_t signed_byte(const Wurm2Record &object,
                               std::uint16_t offset) {
    return static_cast<std::int8_t>(object.u8(offset));
}

// 01F7:6D5F.  Shared WURM2 initializer.  The incoming +0x28/+0x29 and the
// source position are supplied by the ARE wrapper/caller.  The initializer
// has no caller-consumed return flags.
void initialize_wurm2_6d5f(Wurm2Record &object) {
    load_animation_descriptor_5d38(object, 0x33ee); // 6D5F:6D5F-6D62
    object.u16(0x18, 0x6dc4);                       // 6D65
    object.u16(0x2a, 0);                            // 6D69
    object.u16(0x33, 0);                            // 6D6F
    object.u16(0x08, static_cast<std::uint16_t>(
        object.u16(0x08) + 0x20));                       // 6D79
    object.u8(0x32, 0);                             // 6D7A
    object.i32(0x0a, static_cast<std::int32_t>(0xfffeb000)); // 6D7D
    object.u16(0x30, 0);                            // 6D83
    object.u8(0x2c, 0xff);                          // 6D89
    object.u16(0x2d, 0x14);                          // 6D8E
    object.u8(0x2f, 0xff);                          // 6D94
}

// 01F7:6DA3, ARE type 0x01 wrapper.  The wrapper only selects the signed
// orientation/mirror bytes before entering 6D5F.
void initialize_wurm2_type01_6da3(Wurm2Record &object) {
    object.u8(0x29, 0xff);                          // 6DA3
    object.u8(0x28, 0xff);                          // 6DA8
    initialize_wurm2_6d5f(object);                  // 6DAB
}

// 01F7:6DB1, ARE type 0x02 wrapper.  NEG is after the shared initializer,
// therefore the complete seeded fixed-point velocity at +0x0A is negated.
void initialize_wurm2_type02_6db1(Wurm2Record &object) {
    object.u8(0x29, 1);                             // 6DB1
    object.u8(0x28, 1);                             // 6DB6
    initialize_wurm2_6d5f(object);                  // 6DB9
    object.i32(0x0a, static_cast<std::int32_t>(
        0U - static_cast<std::uint32_t>(object.i32(0x0a)))); // 6DBC
}

static void consume_wurm2_target_tail_707b(Wurm2Record &object) {
    if (DS.active_target_count_8806 != 0) {
        std::uint16_t index = object.u16(0x30);
        if (index >= DS.target_capacity_8808) {
            object.u16(0x30, 0);                    // 708C-7092
            index = 0;
        }

        const std::int16_t object_x = static_cast<std::int16_t>(
            object.u16(0x04));
        const std::int16_t object_y = static_cast<std::int16_t>(
            object.u16(0x08));
        const std::int16_t target_x = static_cast<std::int16_t>(
            DS.target_x(index));
        const std::int16_t target_y = static_cast<std::int16_t>(
            DS.target_y(index));

        // 709E-70C1: strict target window.  The x bounds are object X-0x19
        // and object X+0x19; the y bounds are object Y-0x0F and Y+5.
        if (target_x > static_cast<std::int16_t>(object_x - 0x19) &&
            target_x < static_cast<std::int16_t>(object_x + 0x19) &&
            target_y < static_cast<std::int16_t>(object_y + 5) &&
            target_y > static_cast<std::int16_t>(object_y - 0x0f)) {
            DS.clear_target_x(index);                // 70C3
            object.u16(0x18, 0x4ab3);                // 70C9
        }
        object.u16(0x30, static_cast<std::uint16_t>(
            object.u16(0x30) + 1));                 // 70CF
    }
    advance_animation_descriptor_5d60(object);      // 70D3; flags unused
}

// 01F7:6DC4.  ES:DI is the pooled WURM2 record.  The callback has no stable
// scheduler return flags.  The first 1B77 call at 6DDD is deliberately not
// used for a branch: the next instructions overwrite the argument registers
// before the 1C4D call and the only branch after that call consumes 1C4D CF.
void update_wurm2_6dc4(Wurm2Record &object) {
    // 6DC4-6DD0.  CF=1 from 1DCA culls this object and returns.
    if (object_camera_visibility_gate_1dca(object).cf) {
        deactivate_object_outside_camera_1dee(object);
        return;
    }

    // 6DD1-6DDD.  Rectangle offsets are AX=-10, BX=-10, CX=+20, DX=+20.
    // 1B77's AX code and flags are not consumed at this call site, although
    // 1B77 may itself feed the player damage/death globals.
    (void)player_contact_damage_overlap_1b77(-10, -10, 20, 20, object);

    // 6DE2-6DE8.  1C4D receives AX=+0x28 and BX=-0x28, computes its oriented
    // point, and returns the inverted 5C27 occupancy in CF.
    if (descriptor_probe_oriented_1c4d(+0x28, -0x28, object).cf)
        object.u8(0x2f, 1);                         // 6DEF

    // 6DF4-6E23.  This second probe is a direct 5C27 call, not another 1C4D
    // call.  5C27 consumes AX=object Y and BX=object X +/- 0x26.  The raw
    // caller branch is JZ; preserve that flag condition without assigning a
    // semantic name to the selected descriptor polarity here.
    const std::int16_t y = static_cast<std::int16_t>(object.u16(0x08));
    const std::int16_t x = static_cast<std::int16_t>(object.u16(0x04));
    const std::int16_t side_x = signed_byte(object, 0x29) <= 0
        ? static_cast<std::int16_t>(x - 0x26)
        : static_cast<std::int16_t>(x + 0x26);
    if (map_descriptor_quadrant_test_5c27(y, side_x).zf)
        object.u8(0x2f, 1);                         // 6E0B/6E1F

    const std::int8_t state = signed_byte(object, 0x32);
    if (state < 1) {
        // 6E31-6E36: the signed MAP latch uses JLE for the ordinary path.
        // Nonpositive (+0x2F = the initializer value 0xFF, or a clear probe
        // result) falls through to 6F16 and integrates the existing velocity.
        // A positive latch is the descriptor-contact path at 6E3A. The
        // native mapBlocked boolean preserves this polarity.
        if (signed_byte(object, 0x2f) > 0) {
            if (signed_byte(object, 0x2c) < 0) {
                // 6E41-6EBA.  The animation load is conditional on the
                // pre-decrement timer being exactly 0x14.
                if (object.u16(0x2d) == 0x14)
                    load_animation_descriptor_5d38(object, 0x33ee);
                const std::int32_t velocity = clamp_i32(
                    object.i32(0x0a) -
                        (static_cast<std::int32_t>(signed_byte(object, 0x29)) << 12),
                    static_cast<std::int32_t>(0xfffeb000), 0x15000);
                object.i32(0x0a, velocity);
                object.i32(0x02, wrap_add_i32(object.i32(0x02), velocity));
                const std::uint16_t timer = object.u16(0x2d);
                const std::uint16_t next_timer =
                    static_cast<std::uint16_t>(timer - 1);
                object.u16(0x2d, next_timer);
                if (static_cast<std::int16_t>(next_timer) < 0) {
                    object.u8(0x29, static_cast<std::uint8_t>(
                        0 - object.u8(0x29)));
                    object.u8(0x28, static_cast<std::uint8_t>(
                        0 - object.u8(0x28)));
                    object.u8(0x2c, static_cast<std::uint8_t>(
                        0 - object.u8(0x2c)));
                    object.i32(0x0a,
                        static_cast<std::int32_t>(signed_byte(object, 0x29)) << 9);
                    object.u16(0x2d, 0x3c);
                }
            } else {
                // 6EBC-6F13.
                const std::int32_t velocity = clamp_i32(
                    object.i32(0x0a) +
                        (static_cast<std::int32_t>(signed_byte(object, 0x29)) << 10),
                    static_cast<std::int32_t>(0xfffeb000), 0x15000);
                object.i32(0x0a, velocity);
                object.i32(0x02, wrap_add_i32(object.i32(0x02), velocity));
                const std::uint16_t timer = object.u16(0x2d);
                const std::uint16_t next_timer =
                    static_cast<std::uint16_t>(timer - 1);
                object.u16(0x2d, next_timer);
                if (static_cast<std::int16_t>(next_timer) < 0) {
                    object.u8(0x2c, static_cast<std::uint8_t>(
                        0 - object.u8(0x2c)));
                    object.u8(0x2f, 0xff);
                    object.u16(0x2d, 0x14);
                }
            }
        } else {
            // 6F16-6F4D.  The PRNG read is sign-extended to AX and then
            // logically shifted right by one, exactly as CBW followed by SHR.
            object.i32(0x02, wrap_add_i32(
                object.i32(0x02), object.i32(0x0a)));
            object.u16(0x2a, static_cast<std::uint16_t>(
                object.u16(0x2a) + 1));
            if (object.u16(0x2a) > 0x96) {
                const std::uint16_t index = DS.random_cursor_6468;
                DS.random_cursor_6468 = static_cast<std::uint16_t>(
                    (DS.random_cursor_6468 + 1) & 0xff);
                const std::int16_t signed_random =
                    static_cast<std::int8_t>(DS.random_bytes_646c[index & 0xff]);
                const std::uint16_t shifted = static_cast<std::uint16_t>(
                    static_cast<std::uint16_t>(signed_random) >> 1);
                object.u16(0x2a, shifted);
                object.u8(0x32, 1);
            }
        }
    } else if (signed_byte(object, 0x2f) > 0) {
        // 6F50-6F62.  Positive contact latch exits the state machine before
        // the common target tail and leaves the 0x78 timer in +0x2A.
        object.u8(0x32, 0);
        object.u16(0x2a, 0x78);
    } else if (signed_byte(object, 0x32) == 2) {
        // 6FF3-7010.  State 2 waits through 0x4B inclusive; then it enters
        // state 3 and falls through to the state-3 integration below.
        object.u16(0x33, static_cast<std::uint16_t>(
            object.u16(0x33) + 1));
        if (object.u16(0x33) > 0x4b) {
            object.u16(0x33, 0);
            object.u8(0x32, 3);
            load_animation_descriptor_5d38(object, 0x33e2);
        } else {
            goto callback_tail_707b;
        }
        // Falls through to the state-3 velocity integration.
        goto state3_integration_7011;
    } else if (signed_byte(object, 0x32) != 3) {
        // 6F77-6FF0.  This is the state-1/other-state branch.  The exact
        // signed stop test is retained rather than assigning a semantic
        // “turn” label to the orientation byte.
        load_animation_descriptor_5d38(object, 0x33e2);
        const std::int32_t velocity = clamp_i32(
            object.i32(0x0a) -
                (static_cast<std::int32_t>(signed_byte(object, 0x29)) << 11),
            static_cast<std::int32_t>(0xfffeb000), 0x15000);
        object.i32(0x0a, velocity);
        object.i32(0x02, wrap_add_i32(object.i32(0x02), velocity));
        if ((signed_byte(object, 0x29) <= 0 && velocity < 0) ||
            (signed_byte(object, 0x29) > 0 && velocity > 0))
            goto callback_tail_707b;
        object.i32(0x0a, 0);
        object.u8(0x32, 2);
        load_animation_descriptor_5d38(object, 0x33fa);
        goto callback_tail_707b;
    } else {
state3_integration_7011:
        // 7011-7076.  State 3 accelerates in the orientation direction and
        // returns to state 0 only on the signed fixed-point bound.
        const std::int32_t velocity = clamp_i32(
            object.i32(0x0a) +
                (static_cast<std::int32_t>(signed_byte(object, 0x29)) << 11),
            static_cast<std::int32_t>(0xfffeb000), 0x15000);
        object.i32(0x0a, velocity);
        object.i32(0x02, wrap_add_i32(object.i32(0x02), velocity));
        if ((signed_byte(object, 0x29) > 0 && velocity >= 0x15000) ||
            (signed_byte(object, 0x29) <= 0 &&
             velocity <= static_cast<std::int32_t>(0xfffeb000))) {
            object.u8(0x32, 0);
            load_animation_descriptor_5d38(object, 0x33ee);
        }
    }

callback_tail_707b:
    consume_wurm2_target_tail_707b(object);
}

// Static boundary: 6D5F/6DA3/6DB1/6DC4 write only the pooled object and the
// 6468 PRNG cursor/79DE target row.  The player feedback edge is indirect:
// 6DDD -> 1B77 may reach 19E6.  The 1DCA/1DEE lifetime edge and the 5D38/
// 5D60 animation loaders are retained as explicit contracts.
