// Focused Ghidra reconstruction of the W1L1 object callbacks selected by the
// ARE inventory.  This file records the static contract; it is deliberately
// not a native implementation and does not assign meanings to opaque calls.
//
// Source executable SHA-256:
//   c9b2e59febd6fa0ea271bedf360459353f55c74444f026964b70988d6de1bca1
// Static source: Ghidra x86:LE:16:Protected Mode, raw NE segment import,
// no-analysis annotation, relocation-aware listing, export v43.
//
// Static fact and interpretation are separated in comments.  Interior
// addresses 474D, 8E42, and 92A9 are labels in containing functions, not
// additional function bodies.

#include <cstdint>

struct ObjectRecord {
    std::uint8_t bytes[0x78];

    std::uint8_t u8(std::uint16_t offset) const;
    void u8(std::uint16_t offset, std::uint8_t value);
    std::uint16_t u16(std::uint16_t offset) const;
    void u16(std::uint16_t offset, std::uint16_t value);
    std::uint32_t u32(std::uint16_t offset) const;
    void u32(std::uint16_t offset, std::uint32_t value);
    std::int32_t i32(std::uint16_t offset) const;
    void i32(std::uint16_t offset, std::int32_t value);
};

struct Flags {
    bool cf;
    bool zf;
};

struct ContactBounds {
    // 393C returns AX=left, BX=top, CX=right, DX=bottom in pixel units.
    std::int16_t left;
    std::int16_t top;
    std::int16_t right;
    std::int16_t bottom;
};

struct W1L1Globals {
    std::uint16_t random_cursor_6468;
    std::int8_t random_ring_646c[0x100];
    std::uint32_t score_881c;
    std::uint16_t ammo_880c;
    std::uint16_t lives_or_count_880a;
    std::uint16_t progress_current_8822;
    std::uint16_t progress_target_8824;
    std::uint16_t level_selector_85d4;
    std::uint16_t action_612e;
    std::uint16_t collected_mask_60d8;
    std::uint16_t transition_gate_89e6;
};

extern W1L1Globals DS;

// Address-named helpers preserve the unresolved flag/callback boundaries.
// Their declarations are contracts, not semantic guesses.
extern Flags address_named_visibility_gate();
extern void address_named_visibility_cleanup();
extern Flags address_named_leaf_map_probe(std::int16_t cx, std::int16_t dx);
extern void address_named_animation_or_effect_dispatch(std::uint16_t action);
extern void address_named_collectible_auxiliary_dispatch();
extern ContactBounds player_bounds_for_contact_393c();
extern void address_named_cloud_visual_dispatch();
extern void address_named_leaf_effect_dispatch();
extern void address_named_leaf_animation_loader(std::uint16_t selector);

// These are the relocated targets of the 47E7 callback.  They are named by
// address because the callback's branch polarity is closed statically, while
// the runtime MAP/descriptor contents still choose the branch.
extern Flags object_camera_visibility_gate_1dca(ObjectRecord &object);
extern void deactivate_object_outside_camera_1dee(ObjectRecord &object);
extern Flags probe_transition_descriptor_1bd1(
    ObjectRecord &object, std::int16_t cx, std::int16_t dx);
extern void advance_animation_descriptor_5d60(ObjectRecord &object);

void update_collectible_state_8d31(ObjectRecord &object);

static std::uint16_t next_random_index() {
    DS.random_cursor_6468 = static_cast<std::uint16_t>(
        (DS.random_cursor_6468 + 1U) & 0x00ffU);
    return DS.random_cursor_6468;
}

static std::int32_t signed_leaf_perturbation(std::uint16_t index) {
    return static_cast<std::int32_t>(DS.random_ring_646c[index]) << 7;
}

// 01F7:4727.  The requested 474D address is the callback-installation block
// in this same function, not a separate function entry.
//
// Static fact: consume one ring byte, choose selector 3312 or 3326 by its
// signed value, call that address-named loader, install 47E7, consume a second
// ring byte, and seed the object state.  No player record is accessed.
void initialize_falling_leaf_4727(ObjectRecord &object) {
    const std::uint16_t selector_index = next_random_index(); // 4727-4732
    const std::int8_t selector_value = DS.random_ring_646c[selector_index];
    address_named_leaf_animation_loader(
        selector_value > 0 ? 0x3312 : 0x3326);                 // 4738-4748
    object.u16(0x18, 0x47e7);                                  // 474D

    const std::uint16_t velocity_index = next_random_index(); // 4753-475E
    object.i32(0x0e, static_cast<std::int32_t>(0x13000) -
        signed_leaf_perturbation(velocity_index));             // 4764-4776
    object.i32(0x2a, object.i32(0x02));                        // 477B-4780
    object.i32(0x2e, object.i32(0x06));                        // 4785-478A
    object.u16(0x32, 0x000c);                                  // 478F
}

// 01F7:47E7.  The relocated callees are all mechanically identified:
// 1DCA is the camera gate, 1DEE is the object/ARE release, 1BD1 is the
// descriptor probe with CX=DX=0, and 5D60 advances the animation descriptor.
// Their runtime inputs can still choose the branch, but no callee remains
// structurally unresolved in this callback.
void update_falling_leaf_47e7(ObjectRecord &object) {
    if (object_camera_visibility_gate_1dca(object).cf) {        // 47E7-47EC
        deactivate_object_outside_camera_1dee(object);          // 47EE
        return;
    }

    if (probe_transition_descriptor_1bd1(object, 0, 0).cf) {   // 47F4-47FF
        const std::uint16_t timer = object.u16(0x32);
        const std::uint16_t next_timer = static_cast<std::uint16_t>(timer - 1);
        object.u16(0x32, next_timer);
        if (next_timer == 0) {                                 // 4809-481F
            object.i32(0x02, object.i32(0x2a));
            object.i32(0x06, object.i32(0x2e));
            object.u16(0x32, 0x000c);
            object.u16(0x12, static_cast<std::uint16_t>(
                object.u16(0x12) & 0x7fff));
            const std::uint16_t index = next_random_index();
            object.i32(0x0e, static_cast<std::int32_t>(0x13000) -
                signed_leaf_perturbation(index));
        } else {                                                // 4822-4835
            object.u16(0x12, static_cast<std::uint16_t>(
                object.u16(0x12) ^ 0x8000));
        }
    } else {                                                    // 4803-481B
        std::int32_t velocity = object.i32(0x0e);
        if (velocity > 0x4000)
            velocity -= 0x12c;
        object.i32(0x06, static_cast<std::int32_t>(
            static_cast<std::uint32_t>(object.i32(0x06)) +
            static_cast<std::uint32_t>(velocity)));
        object.i32(0x0e, velocity);
    }
    advance_animation_descriptor_5d60(object);                 // 4879
}

// The five numeric collectible initializers all install the same callback.
// Their word writes are retained as word writes because that is what the
// original instructions do, even where later code reads only a low byte.
void initialize_collectible_8bc2(ObjectRecord &object) {
    object.u16(0x18, 0x8d20); object.u16(0x12, 0x025f);
    object.i32(0x04, object.i32(0x04) + 1);
    object.i32(0x08, object.i32(0x08) - 2);
    object.u16(0x2c, 1); object.u16(0x2a, 0);                  // 8BC2
}

void initialize_collectible_8be5(ObjectRecord &object) {
    object.u16(0x18, 0x8d20); object.u16(0x12, 0x0260);
    object.i32(0x04, object.i32(0x04) + 5);
    object.i32(0x08, object.i32(0x08) + 10);
    object.u16(0x2c, 2); object.u16(0x2a, 0);                 // 8BE5
}

void initialize_collectible_8c08(ObjectRecord &object) {
    object.u16(0x18, 0x8d20); object.u16(0x12, 0x0261);
    object.i32(0x04, object.i32(0x04) + 5);
    object.i32(0x08, object.i32(0x08) + 10);
    object.u16(0x2a, 0); object.u16(0x2c, 3);                 // 8C08
}

void initialize_collectible_8c2b(ObjectRecord &object) {
    object.u16(0x18, 0x8d20); object.u16(0x12, 0x0262);
    object.i32(0x04, object.i32(0x04) + 3);
    object.i32(0x08, object.i32(0x08) + 7);
    object.u16(0x2c, 4); object.u16(0x2a, 0);                 // 8C2B
}

void initialize_collectible_8c4e(ObjectRecord &object) {
    object.u16(0x18, 0x8d20); object.u16(0x12, 0x02c6);
    object.i32(0x04, object.i32(0x04) + 3);
    object.i32(0x08, object.i32(0x08) + 7);
    object.u16(0x2c, 5); object.u16(0x2a, 0);                 // 8C4E
}

// 01F7:8C71..8D07.  Each puzzle letter uses subtype zero and only changes
// its progress mask/animation slot.  The wrappers have no return flags.
void initialize_puzzle_letter_8c71(ObjectRecord &o) {
    o.u16(0x18, 0x8d20); o.u16(0x12, 600); o.u16(0x2a, 1);  o.u16(0x2c, 0);
}
void initialize_puzzle_letter_8c8a(ObjectRecord &o) {
    o.u16(0x18, 0x8d20); o.u16(0x12, 0x259); o.u16(0x2a, 2); o.u16(0x2c, 0);
}
void initialize_puzzle_letter_8ca3(ObjectRecord &o) {
    o.u16(0x18, 0x8d20); o.u16(0x12, 0x25a); o.u16(0x2a, 4); o.u16(0x2c, 0);
}
void initialize_puzzle_letter_8cbc(ObjectRecord &o) {
    o.u16(0x18, 0x8d20); o.u16(0x12, 0x25b); o.u16(0x2a, 8); o.u16(0x2c, 0);
}
void initialize_puzzle_letter_8cd5(ObjectRecord &o) {
    o.u16(0x18, 0x8d20); o.u16(0x12, 0x25c); o.u16(0x2a, 0x10); o.u16(0x2c, 0);
}
void initialize_puzzle_letter_8cee(ObjectRecord &o) {
    o.u16(0x18, 0x8d20); o.u16(0x12, 0x25d); o.u16(0x2a, 0x20); o.u16(0x2c, 0);
}
void initialize_puzzle_letter_8d07(ObjectRecord &o) {
    o.u16(0x18, 0x8d20); o.u16(0x12, 0x25e); o.u16(0x2a, 0x40); o.u16(0x2c, 0);
}

// 01F7:8D20.  Visibility CF is consumed before the local state body.  The
// cleanup call is deliberately not interpreted; its object-lifetime effect
// is the only possible feedback at this boundary.
void update_collectible_8d20(ObjectRecord &object) {
    if (address_named_visibility_gate().cf) {
        address_named_visibility_cleanup();
        return;
    }
    // 8D2D is a direct near call to 8D31.
    update_collectible_state_8d31(object);
}

// 01F7:8D31.  8E42 is the common successful-contact label in this body.
// 393C's bounds are strict on all four edges; object Y is aligned down to a
// 16-pixel boundary before the Y comparisons.  The external dispatch after
// each state write is retained as address-named presentation/effect work.
void update_collectible_state_8d31(ObjectRecord &object) {
    const ContactBounds p = player_bounds_for_contact_393c();
    const std::int16_t x = static_cast<std::int16_t>(object.u16(0x04));
    const std::int16_t y = static_cast<std::int16_t>(
        object.u16(0x08) & 0xfff0U);
    if (!(x < p.right && x + 0x10 > p.left &&
          y < p.bottom && y + 0x10 > p.top))
        return;

    const std::uint8_t subtype = object.u8(0x2c);
    if (subtype == 0) {                                        // 8E29
        DS.score_881c += 100;
        DS.collected_mask_60d8 = static_cast<std::uint16_t>(
            DS.collected_mask_60d8 | object.u16(0x2a));
        DS.action_612e = 0x0b;
        address_named_animation_or_effect_dispatch(0x0b);
    } else if (subtype == 1) {                                 // 8D69
        DS.ammo_880c = static_cast<std::uint16_t>(DS.ammo_880c + 10);
        DS.score_881c += 0x32;
        DS.action_612e = 9;
        address_named_animation_or_effect_dispatch(9);
    } else if (subtype == 2) {                                 // 8D89
        DS.score_881c += 0xfa;
        if (DS.progress_target_8824 != 5)
            ++DS.progress_target_8824;
        DS.progress_current_8822 = DS.progress_target_8824;
        DS.action_612e = 9;
        address_named_animation_or_effect_dispatch(9);
    } else if (subtype == 3) {                                 // 8DB8
        DS.action_612e = 10;
        address_named_animation_or_effect_dispatch(10);
        DS.score_881c += 100;
        if (DS.progress_current_8822 != DS.progress_target_8824)
            ++DS.progress_current_8822;
    } else if (subtype == 4) {                                 // 8DDF
        if (DS.level_selector_85d4 < 0x10)
            address_named_collectible_auxiliary_dispatch();
        DS.score_881c += 0x96;
        DS.action_612e = 0x0c;
        address_named_animation_or_effect_dispatch(0x0c);
    } else {                                                   // 8E08
        DS.score_881c += 500;
        DS.action_612e = 0x0c;
        address_named_animation_or_effect_dispatch(0x0c);
        if (DS.lives_or_count_880a < 9)
            ++DS.lives_or_count_880a;
    }
    object.u16(0x18, 0);                                       // 8E42
}

// 01F7:8E4B.  The state-machine callback is shared by animated world-ICO
// declarations 0x1F..0x21.  8E42 is not another entry here; it is an
// interior success label in 8D31 above.
//
// The two coordinate conventions are intentionally explicit.  3376 is
// called with AX=Y and BX=X and returns the MAP low-nine-bit tile ID.  The
// following 16CE call receives the saved AX/BX pair in the same order; its
// own MAP writer uses BX as the row coordinate and AX as the column
// coordinate.  No semantic name is assigned to the selector-dependent
// contents of DS:6986.
extern Flags address_named_camera_gate_1dca();
extern void address_named_remove_object_1dee(ObjectRecord &object);
extern ContactBounds address_named_player_bounds_393c();
extern std::uint16_t address_named_map_tile_id_lookup_3376(
    std::uint16_t ax_y, std::uint16_t bx_x);
extern void address_named_map_effect_tile_rewrite_16ce(
    std::uint16_t ax_y, std::uint16_t bx_x,
    std::uint16_t cx_dx_effect);
extern std::uint16_t address_named_effect_table_6986(std::uint16_t tile_id);
extern std::uint16_t address_named_terminal_x_word_8828;
extern std::uint16_t address_named_terminal_y_word_882a;

static void emit_tile_effect_8e4b(ObjectRecord &object,
                                  std::uint16_t y_offset,
                                  std::uint16_t x_offset) {
    const std::uint16_t ax_y = static_cast<std::uint16_t>(
        object.u16(0x08) + y_offset);
    const std::uint16_t bx_x = static_cast<std::uint16_t>(
        object.u16(0x04) + x_offset);
    const std::uint16_t tile_id = address_named_map_tile_id_lookup_3376(
        ax_y, bx_x);
    const std::uint16_t effect = address_named_effect_table_6986(tile_id);
    if (effect != 0)
        address_named_map_effect_tile_rewrite_16ce(ax_y, bx_x, effect);
}

void update_tile_effect_state_machine_8e4b(ObjectRecord &object) {
    if (object.u16(0x32) == 0) {                                // 8E4B-8E78
        if (address_named_camera_gate_1dca().cf) {              // 8E79-8E85
            address_named_remove_object_1dee(object);
            return;
        }
        const ContactBounds p = address_named_player_bounds_393c(); // 8E86
        const std::int16_t x = static_cast<std::int16_t>(object.u16(0x04));
        const std::int16_t y = static_cast<std::int16_t>(object.u16(0x08));
        if (x < p.right && x + 0x50 > p.left &&
            static_cast<std::int16_t>(object.u16(0x08) & 0xfff0U) < p.bottom &&
            static_cast<std::int16_t>((object.u16(0x08) & 0xfff0U) + 0x40) > p.top)
            object.u16(0x32, 1);                                // 8EB5
        return;                                                  // 9255
    }

    const std::uint16_t state = static_cast<std::uint16_t>(
        object.u16(0x32) + 1U);                                  // 8E52
    object.u16(0x32, state);
    if (state == 4) {                                            // 8EBE
        emit_tile_effect_8e4b(object, 0x00, 0x10);
        emit_tile_effect_8e4b(object, 0x00, 0x00);
        emit_tile_effect_8e4b(object, 0x00, 0x20);
        emit_tile_effect_8e4b(object, 0x00, 0x30);
        emit_tile_effect_8e4b(object, 0x00, 0x40);
    } else if (state == 6) {                                     // 8F9D
        emit_tile_effect_8e4b(object, 0x10, 0x10);
        emit_tile_effect_8e4b(object, 0x10, 0x20);
        emit_tile_effect_8e4b(object, 0x10, 0x30);
        emit_tile_effect_8e4b(object, 0x10, 0x40);
        emit_tile_effect_8e4b(object, 0x10, 0x50);
    } else if (state == 8) {                                     // 907C
        emit_tile_effect_8e4b(object, 0x20, 0x10);
        emit_tile_effect_8e4b(object, 0x20, 0x20);
        emit_tile_effect_8e4b(object, 0x20, 0x30);
        emit_tile_effect_8e4b(object, 0x20, 0x40);
        emit_tile_effect_8e4b(object, 0x20, 0x50);
    } else if (state == 10) {                                    // 915B-9253
        emit_tile_effect_8e4b(object, 0x30, 0x10);
        emit_tile_effect_8e4b(object, 0x30, 0x20);
        emit_tile_effect_8e4b(object, 0x30, 0x30);
        emit_tile_effect_8e4b(object, 0x30, 0x40);
        emit_tile_effect_8e4b(object, 0x30, 0x50);
        object.u16(0x18, 0);                                     // 9237
        address_named_terminal_x_word_8828 = static_cast<std::uint16_t>(
            object.u16(0x04) + 0x19);                            // 9245-924E
        address_named_terminal_y_word_882a = static_cast<std::uint16_t>(
            object.u16(0x08) + 0x46);                            // 9250
    }
}

// 01F7:9256.  This is the initializer block for ARE type 0x28, not a
// separate update body.  It leaves the object in logical slot FFFF.
void initialize_cloud_9256(ObjectRecord &object) {
    object.u16(0x18, 0x9269);
    object.u16(0x12, 0xffff);
    object.u16(0x2a, 0);
}

// 01F7:9269.  92A9 is the successful-latch label in this same body.  The
// visibility gate and 393C bounds are ordered before the persistent-player
// +0x37 gate.  Only DS:89E6 is written on success.
void update_cloud_9269(ObjectRecord &object, const ObjectRecord &player) {
    if (address_named_visibility_gate().cf) {
        address_named_visibility_cleanup();
        return;
    }
    const ContactBounds p = player_bounds_for_contact_393c();
    const std::int16_t x = static_cast<std::int16_t>(object.u16(0x04));
    const std::int16_t y = static_cast<std::int16_t>(
        object.u16(0x08) & 0xfff0U);
    if (x < p.right && x + 0x10 > p.left &&
        y < p.bottom && y + 0x10 > p.top &&
        player.u8(0x37) == 0) {
        DS.transition_gate_89e6 = 0xffff;                      // 92A9
    }
}

// Static stopping point:
// - 8BC2..8D07, 8D20/8D31, and 9256/9269 are mechanically closed here.
// - 4727/47E7 remains open only at its address-named visibility/map/effect
//   calls and runtime PRNG contents; it has no direct player write.
// - The sound/effect dispatches in 8D31 are presentation contracts because no
//   player/MAP/callback-global write is present in their callers' body.
