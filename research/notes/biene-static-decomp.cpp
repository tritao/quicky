// Focused Ghidra reconstruction of the BIENE normal-enemy closure in
// QUIKY.EXE segment 01F7.  This is intentionally address/field oriented: it
// records the exact state machine that can reach the player damage path,
// without assigning presentation-only meanings to the animation tables.
//
// Source executable SHA-256:
//   c9b2e59febd6fa0ea271bedf360459353f55c74444f026964b70988d6de1bca1
// Static source: Ghidra x86:LE:16:Protected Mode, raw NE segment import,
// no-analysis annotation, and NE relocation resolution.  The reproducible
// export is run_player_external_closure.py.

#include <cstdint>

struct Flags {
    bool cf;
    bool zf;
};

struct BieneRecord {
    std::uint8_t bytes[0x78];

    std::uint8_t u8(std::uint16_t offset) const;
    void u8(std::uint16_t offset, std::uint8_t value);
    std::uint16_t u16(std::uint16_t offset) const;
    void u16(std::uint16_t offset, std::uint16_t value);
    std::int32_t i32(std::uint16_t offset) const;
    void i32(std::uint16_t offset, std::int32_t value);
};

struct BieneGlobals {
    std::uint16_t camera_y_81c4;       // DS:81C4
    std::uint16_t active_effects_8806; // DS:8806
    std::uint16_t effect_capacity_8808; // DS:8808
    std::uint16_t effect_x_87de[16];   // DS:87DE, four-byte entries
    std::uint16_t effect_y_87e0[16];
    std::uint16_t player_offset_881a;  // DS:881A
};

extern BieneGlobals DS;

// These are flag contracts, not C++ booleans inferred from a decompiler.
extern Flags object_camera_visibility_1dca(BieneRecord &object);
extern void deactivate_object_1dee(BieneRecord &object);
extern Flags player_contact_damage_1b77(std::int16_t ax_offset,
                                       std::int16_t bx_offset,
                                       std::int16_t cx_extent,
                                       std::int16_t dx_extent,
                                       BieneRecord &object);
extern Flags descriptor_probe_oriented_1c4d(std::int16_t x_offset,
                                            std::int16_t y_offset,
                                            BieneRecord &object);
extern Flags transition_descriptor_probe_1bd1(BieneRecord &object,
                                              std::uint16_t y_offset,
                                              std::uint16_t x_offset);
extern void load_animation_descriptor_5d38(BieneRecord &object,
                                            std::uint16_t sequence);
extern void advance_animation_descriptor_5d60(BieneRecord &object);
// 01F7:0A43 writes this runtime byte table during startup.  The imported
// QUIKY image at DS:7974 is only the pre-runtime storage image; it is not the
// initialized table and must not be treated as a static waveform asset.
extern std::int8_t biene_runtime_byte_7974(std::uint16_t phase);

struct TrackedPlayer {
    std::int16_t x;
    std::int16_t y;
};
extern TrackedPlayer read_player_position_39fe();

static std::int32_t clamp(std::int32_t value,
                          std::int32_t low,
                          std::int32_t high) {
    return value < low ? low : (value > high ? high : value);
}

// 01F7:0A43 / initialize_game_state.  This startup closure is included here
// because BIENE state 1 consumes DS:7974 and the same startup pass also
// publishes the random ring used by object/effect factories.
//
// Static facts:
//   * 0227:19EE seeds the 32-bit state at 0227:35FA/35FC from DOS time.
//   * 0227:1B7E clears DS:6586 for 0x400 bytes.
//   * 0227:1959 advances that state through 0227:19B6 (multiplier 0x8405)
//     and scales the result for the DS:646C random ring.
//   * The table-building loops write DS:7974[index] for index 0..0x7ff.
//     Entries 0..0x3ff additionally reset DS:6D8E[index] to 0xffff.
//   * Each table entry uses the relocated helper sequence
//     14C0 -> 14B2 -> 14AC -> 15E5 -> 14AC -> 14CC.
//   * 15E5's large-value path calls 18F1; 18F1/190A consumes seven six-byte
//     constants at CS:1646, advancing the pointer by six bytes per term.
//     The exact exported words are retained in the runtime-table ledger and
//     are not assigned mathematical names here.
//
// The software-floating-point register/flag ABI in 0227:1185..18F1 is still
// address-qualified.  Therefore this note records the builder's control flow
// and side effects but does not invent native table bytes or call the result
// a sine wave.  Reproducing the initialized bytes requires either an exact
// port of that ABI or a runtime seed/table trace.

// 01F7:684A.  Shared BIENE initializer.  The incoming +0x29/+0x28 pair is
// supplied by the type wrapper.  The source Y is copied to +0x36 before the
// ARE +0x20 placement offset is applied.
void initialize_biene_684a(BieneRecord &object) {
    load_animation_descriptor_5d38(object, 0x33c0); // 684A:684A-6850
    object.u16(0x18, 0x68c0);                       // 684A:6852
    object.u16(0x2a, 0);                            // 684A:6858
    object.u16(0x34, 0);                            // 684A:685E
    object.u16(0x08, static_cast<std::uint16_t>(
        object.u16(0x08) + 0x20));                 // 684A:6864
    object.i32(0x0a, static_cast<std::int32_t>(0xfffeb000)); // 6869
    object.u16(0x30, 0);                            // 6872
    object.u16(0x32, 0);                            // 6878
    object.u16(0x40, 0);                            // 687E
    object.i32(0x36, object.i32(0x06));             // 6884
    object.u8(0x2c, 0xff);                          // 688E
    object.u16(0x2d, 0x14);                         // 6893
    object.u8(0x2f, 0xff);                          // 6899
}

// 01F7:689F, type 0x03 wrapper.  The paired type changes only the signed
// horizontal orientation and mirror byte before entering 684A.
void initialize_biene_left_689f(BieneRecord &object) {
    object.u8(0x29, 0xff);
    object.u8(0x28, 0xff);
    initialize_biene_684a(object);
}

// 01F7:68AD, type 0x04 wrapper.  NEG +0x0A is after the shared initializer,
// so it negates the seeded horizontal velocity including its fixed fraction.
void initialize_biene_right_68ad(BieneRecord &object) {
    object.u8(0x29, 1);
    object.u8(0x28, 1);
    initialize_biene_684a(object);
    object.i32(0x0a, static_cast<std::int32_t>(
        0U - static_cast<std::uint32_t>(object.i32(0x0a))));
}

// The callback's final target-overlap tail is shared with the other normal
// enemy families.  It can replace the callback with 4AB3, which is the
// subsequent player-contact response object; no stable flags are consumed.
static void consume_contact_effect_tail_6d01(BieneRecord &object) {
    if (DS.active_effects_8806 == 0)
        return;

    std::uint16_t index = object.u16(0x30);
    if (index >= DS.effect_capacity_8808)
        index = 0;
    const std::int16_t x = static_cast<std::int16_t>(object.u16(0x04));
    const std::int16_t y = static_cast<std::int16_t>(object.u16(0x08));
    const std::int16_t effect_x = static_cast<std::int16_t>(
        DS.effect_x_87de[index]);
    const std::int16_t effect_y = static_cast<std::int16_t>(
        DS.effect_y_87e0[index]);
    if (effect_x > static_cast<std::int16_t>(x - 0x0f) &&
        effect_x < static_cast<std::int16_t>(x + 0x0f) &&
        effect_y < static_cast<std::int16_t>(y + 5) &&
        effect_y > static_cast<std::int16_t>(y - 0x19)) {
        DS.effect_x_87de[index] = 0;
        object.u16(0x18, 0x4ab3); // 6D49
    }
    object.u16(0x30, static_cast<std::uint16_t>(object.u16(0x30) + 1));
}

// 01F7:68C0.  The object is an ES:DI pooled record.  Its RET returns no
// caller-consumed flags; only the nested helper flags control local branches.
void update_biene_68c0(BieneRecord &object) {
    // 68C0-68CC: visibility gate and callback removal.
    if (object_camera_visibility_1dca(object).cf) {
        deactivate_object_1dee(object);
        return;
    }

    // 68CD-68EB: exact outer contact probes. The first call is the player
    // rectangle (-10,-20,+20,+20); the second is the oriented descriptor
    // point (X-20,Y+20) and returns the inverted descriptor polarity.
    if (player_contact_damage_1b77(-10, -20, 20, 20, object).cf ||
        descriptor_probe_oriented_1c4d(20, -20, object).cf)
        object.u8(0x2f, 1);

    // 68F0-6A61: states below one are horizontal patrol/turnaround. All
    // signed arithmetic is fixed-point and wraps at the original dword.
    if (object.u16(0x32) < 1) {
        // 68F9-68FE: nonpositive +0x2F takes the ordinary 69DD path;
        // positive +0x2F takes the 6902 patrol/contact response.
        if (static_cast<std::int8_t>(object.u8(0x2f)) > 0) {
            if (static_cast<std::int8_t>(object.u8(0x2c)) < 0) {
                if (object.u16(0x2d) == 0x14)
                    load_animation_descriptor_5d38(object, 0x33c0);
                const std::int32_t sign =
                    static_cast<std::int8_t>(object.u8(0x29));
                const std::int32_t velocity = clamp(
                    object.i32(0x0a) - (sign << 12),
                    static_cast<std::int32_t>(0xfffeb000), 0x15000);
                object.i32(0x0a, velocity);
                object.i32(0x02, static_cast<std::int32_t>(
                    static_cast<std::uint32_t>(object.i32(0x02)) +
                    static_cast<std::uint32_t>(velocity)));
                const std::uint16_t timer = object.u16(0x2d);
                object.u16(0x2d, static_cast<std::uint16_t>(timer - 1));
                if (timer == 0) {
                    object.u8(0x29, static_cast<std::uint8_t>(
                        0 - object.u8(0x29)));
                    object.u8(0x28, static_cast<std::uint8_t>(
                        0 - object.u8(0x28)));
                    object.u8(0x2c, static_cast<std::uint8_t>(
                        0 - object.u8(0x2c)));
                    object.i32(0x0a,
                        static_cast<std::int8_t>(object.u8(0x29)) << 9);
                    object.u16(0x2d, 0x3c);
                }
            } else {
                const std::int32_t sign =
                    static_cast<std::int8_t>(object.u8(0x29));
                const std::int32_t velocity = clamp(
                    object.i32(0x0a) + (sign << 10),
                    static_cast<std::int32_t>(0xfffeb000), 0x15000);
                object.i32(0x0a, velocity);
                object.i32(0x02, static_cast<std::int32_t>(
                    static_cast<std::uint32_t>(object.i32(0x02)) +
                    static_cast<std::uint32_t>(velocity)));
                const std::uint16_t timer = object.u16(0x2d);
                object.u16(0x2d, static_cast<std::uint16_t>(timer - 1));
                if (timer == 0) {
                    object.u8(0x2c, static_cast<std::uint8_t>(
                        0 - object.u8(0x2c)));
                    object.u8(0x2f, 0xff);
                    object.u16(0x2d, 0x14);
                }
            }
        } else {
            object.i32(0x02, static_cast<std::int32_t>(
                static_cast<std::uint32_t>(object.i32(0x02)) +
                static_cast<std::uint32_t>(object.i32(0x0a))));
            object.u16(0x2a, static_cast<std::uint16_t>(
                object.u16(0x2a) + 1));
            if (object.u16(0x2a) > 0x96) {
                object.u16(0x2a, 0);
                object.u8(0x2f, 1);
            }
        }

        // 69FE-6A61: persistent-player range gate. The raw instruction order
        // is retained: the +0x78 add is overwritten by the DS:81C4 load.
        // A successful range test enters state 1 and loads 33C8; either way
        // this branch joins the common 6D01 tail.
        const TrackedPlayer player = read_player_position_39fe();
        const std::int16_t object_x = static_cast<std::int16_t>(
            object.u16(0x04));
        const std::int16_t object_y = static_cast<std::int16_t>(
            object.u16(0x08));
        bool in_x_range = false;
        if (static_cast<std::int8_t>(object.u8(0x29)) < 0) {
            const std::int16_t right = static_cast<std::int16_t>(
                player.x + 0x28);
            const std::int16_t inner = static_cast<std::int16_t>(right - 5);
            in_x_range = object_x < right && object_x > inner;
        } else {
            const std::int16_t left = static_cast<std::int16_t>(
                player.x - 0x28);
            const std::int16_t inner = static_cast<std::int16_t>(left + 5);
            in_x_range = object_x > left && object_x < inner;
        }
        if (in_x_range && object_y < player.y &&
            object_y > static_cast<std::int16_t>(DS.camera_y_81c4)) {
            object.u16(0x32, 1);
            load_animation_descriptor_5d38(object, 0x33c8);
        }
        goto callback_tail_6d01;
    }

    // 6A69-6AC5: state 1 is the 0x400-byte runtime-table vertical transition.
    if (object.u16(0x32) < 2) {
        object.u16(0x08, static_cast<std::uint16_t>(
            object.u16(0x08) - object.u16(0x40)));
        const std::uint16_t phase = static_cast<std::uint16_t>(
            (object.u16(0x3e) + 0x20) & 0x03ff);
        object.u16(0x3e, phase);
        const std::int16_t phase_delta = static_cast<std::int8_t>(
            biene_runtime_byte_7974(phase)) >> 5;
        object.u16(0x40, static_cast<std::uint16_t>(phase_delta));
        object.u16(0x08, static_cast<std::uint16_t>(
            object.u16(0x08) + phase_delta));
        object.i32(0x06, object.i32(0x06) - 0x1388);
        object.i32(0x02, object.i32(0x02) -
            (static_cast<std::int8_t>(object.u8(0x29)) << 13));
        object.u16(0x34, static_cast<std::uint16_t>(object.u16(0x34) + 1));
        if (object.u16(0x34) > 0x32) {
            object.u16(0x34, 0);
            object.u16(0x32, 2);
        }
    }

    // 6ACB-6C0F: states 2-4 perform bounded transition motion and call the
    // final descriptor probe with CX=DX=0. State 5 is the response join.
    if (object.u16(0x32) < 5) {
        object.i32(0x3a, object.i32(0x0a));
        if (object.u16(0x32) != 3 && object.u16(0x32) != 4) {
            object.u16(0x32, 3);
            object.i32(0x0e, 0x1f4);
            object.u16(0x2d, 0x3c);
        }
        if (object.u16(0x32) == 3) {
            object.i32(0x0e, clamp(object.i32(0x0e) + 0x4e20,
                                   static_cast<std::int32_t>(0xfffb0000),
                                   0x50000));
            const std::int32_t sign =
                static_cast<std::int8_t>(object.u8(0x29));
            object.i32(0x0a, clamp(object.i32(0x0a) + (sign << 16),
                                   static_cast<std::int32_t>(0xfffeb000),
                                   0x15000));
            object.i32(0x06, object.i32(0x06) + object.i32(0x0e));
            object.i32(0x02, object.i32(0x02) + object.i32(0x0a));
            const std::uint16_t timer = object.u16(0x2d);
            object.u16(0x2d, static_cast<std::uint16_t>(timer - 1));
            if (timer == 0) {
                object.u16(0x32, 4);
                object.u16(0x2d, 0x3c);
            }
        } else {
            object.i32(0x06, object.i32(0x06) + object.i32(0x0e));
        }
        if (transition_descriptor_probe_1bd1(object, 0, 0).cf) {
            object.u16(0x32, 5);
            load_animation_descriptor_5d38(object, 0x33d2);
        }
    }

    // 6C12-6C35: wait in state 5, then enter exit state 6 and reload 33C0.
    if (object.u16(0x32) < 6) {
        object.u16(0x34, static_cast<std::uint16_t>(object.u16(0x34) + 1));
        if (object.u16(0x34) > 0x6e) {
            object.u16(0x34, 0);
            object.u16(0x32, 6);
            load_animation_descriptor_5d38(object, 0x33c0);
        }
    }

    // 6C3A-6CFC: state 7/8 exit, including the +/-0x40000 Y bound and the
    // small oriented descriptor query at (X+10,Y-10).
    if (object.u16(0x32) < 7)
        ; // state 6 falls through to the state-7 setup below
    if (object.u16(0x32) != 7 && object.u16(0x32) != 8) {
        object.u16(0x32, 7);
        object.i32(0x0e, static_cast<std::int32_t>(0xfffffe0c));
        object.u16(0x2d, 0x46);
    }
    if (object.u16(0x32) == 7) {
        object.i32(0x0e, clamp(object.i32(0x0e) - 0xfa0,
                               static_cast<std::int32_t>(0xfffc0000),
                               0x40000));
        object.i32(0x06, object.i32(0x06) + object.i32(0x0e));
        if (descriptor_probe_oriented_1c4d(10, -10, object).cf) {
            object.u8(0x29, static_cast<std::uint8_t>(
                0 - object.u8(0x29)));
            object.u8(0x28, static_cast<std::uint8_t>(
                0 - object.u8(0x28)));
        }
        object.i32(0x02, object.i32(0x02) +
            (static_cast<std::int8_t>(object.u8(0x29)) << 17));
        const std::uint16_t timer = object.u16(0x2d);
        object.u16(0x2d, static_cast<std::uint16_t>(timer - 1));
        if (timer == 0) {
            object.u16(0x32, 8);
            object.u16(0x2d, 0x46);
        }
    } else if (object.u16(0x32) == 8) {
        object.i32(0x06, object.i32(0x06) + object.i32(0x0e));
    }

    if (object.i32(0x06) < object.i32(0x36)) {
        object.u16(0x32, 0);
        object.u16(0x2d, 0x14);
        object.i32(0x0a, object.i32(0x3a));
    }

callback_tail_6d01:
    consume_contact_effect_tail_6d01(object);
    advance_animation_descriptor_5d60(object);
}

// Static stopping rule: `player_contact_damage_1b77` is the only direct
// feedback edge into player health/death.  Renderer/audio continuations below
// 5D60 remain outside this closure because they cannot modify player state in
// the callback.
