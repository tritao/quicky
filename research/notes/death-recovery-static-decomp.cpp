// Focused Ghidra reconstruction of the player damage/death/recovery closure.
//
// Source executable SHA-256:
//   c9b2e59febd6fa0ea271bedf360459353f55c74444f026964b70988d6de1bca1
// Static source:
//   x86:LE:16:Protected Mode, raw NE segment import, no whole-program
//   analysis, with the Ghidra instruction listing and NE relocations checked
//   by run_player_external_closure.py.
//
// This is deliberately address-annotated C-like code.  It records instruction
// facts and contracts; it does not assign meanings to opaque far calls.

#include <cstdint>
#include <algorithm>

using Fixed16 = std::int32_t;

struct PlayerRecord {
    std::uint8_t raw[0x78];

    std::uint8_t u8(std::uint16_t offset) const;
    void u8(std::uint16_t offset, std::uint8_t value);
    std::uint16_t u16(std::uint16_t offset) const;
    void u16(std::uint16_t offset, std::uint16_t value);
    Fixed16 i32(std::uint16_t offset) const;
    void i32(std::uint16_t offset, Fixed16 value);
};

struct SpawnCoordinateRow {
    std::uint16_t x_word;
    std::uint16_t y_word;
};

// DS:8828 is the base of 32 interleaved coordinate rows, not an int32
// scalar.  Row zero is DS:8828/DS:882A; row n is at DS:8828 + 4*n.
extern SpawnCoordinateRow address_named_spawn_rows_8828[0x20];

struct DeathGlobals {
    std::uint16_t transition_effect_bits_8950; // DS:8950
    std::uint16_t transition_gate_89ea;        // DS:89EA
    std::uint16_t transition_event_89ec;       // DS:89EC
    std::uint16_t transition_counter_89f0;     // DS:89F0
    std::uint8_t transition_substate_85da;     // DS:85DA
    std::uint16_t action_suppressor_89e0;      // DS:89E0
    std::uint16_t transition_signal_89e6;      // DS:89E6
    std::uint16_t transition_row_85d4;         // DS:85D4
    std::uint16_t lives_880a;                  // DS:880A
    std::uint16_t health_8822;                 // DS:8822
    std::uint16_t maximum_health_8824;         // DS:8824
    std::uint16_t invulnerability_gate_8810;   // DS:8810
    std::uint16_t pending_event_612e;          // DS:612E
    std::uint16_t spawn_row_85d2;              // DS:85D2
    std::uint16_t player_offset_881a;          // DS:881A
    std::uint16_t resource_handle_97e2;       // DS:97E2
    std::uint16_t animation_descriptor_3156;   // SI immediate at 1AEC
};

extern DeathGlobals DS;

// All declarations below are intentionally address-named.  A far call whose
// target is not resolved by the NE relocation table is not treated as a C
// return value or as a gameplay semantic.
extern void address_named_far_call_199A();
extern void address_named_far_call_19FB();
extern void address_named_far_call_1A43();
extern void address_named_far_call_1A89();
extern void address_named_far_call_1AEF();
extern void address_named_far_call_1B01();
extern void address_named_far_call_1AF5_outer();
extern void address_named_far_call_1AAA_entry();
extern void address_named_far_call_1B01_to_1AAA();
extern void address_named_animation_loader_5D38(std::uint16_t descriptor);
extern std::uint16_t address_named_recovery_selector_85D4();
extern std::uint16_t address_named_transition_selector_5044();
extern void clear_are_event_queue_17D4();
extern void deactivate_object_outside_camera_1DEE(std::uint16_t object_offset);

// The resource/DOS calls inside 34C7 are retained as address-qualified
// helpers.  Their returned AX words are the only values needed to recover the
// spawn-table write; no caller-consumed flags are present on this path.
extern std::uint16_t address_named_resource_word_34C7_3586(
    std::uint16_t resource_handle);
extern std::uint16_t address_named_resource_word_34C7_359D(
    std::uint16_t resource_handle);

extern void address_named_lifecycle_call_4BBC();
extern void address_named_lifecycle_call_4BCE();
extern void address_named_lifecycle_call_4BDF();
extern void address_named_lifecycle_call_4BE4();
extern void address_named_lifecycle_call_4C1C();
extern void address_named_lifecycle_call_4C21();
extern void address_named_lifecycle_call_4C26();
extern void address_named_lifecycle_call_4C2B();
extern void address_named_lifecycle_call_4C30();
extern void address_named_lifecycle_call_4C87();
extern void address_named_lifecycle_call_4C8C();
extern void address_named_lifecycle_call_4C91();
extern void address_named_lifecycle_call_4C96();
extern void address_named_lifecycle_call_4C9B();
extern void address_named_lifecycle_call_4CA0();
extern void address_named_lifecycle_call_4CA5();
extern void address_named_lifecycle_call_4C64();
extern void address_named_lifecycle_call_4CC4();
extern void address_named_lifecycle_call_4CC7();
extern void address_named_lifecycle_call_4CD5();
extern void address_named_terminal_menu_4C79();
extern void address_named_transition_substate_4D0D(std::uint8_t substate);
extern void address_named_transition_loader_4EBA(std::uint16_t arg);
extern void address_named_transition_effect_4EEB();
extern void address_named_transition_pending_4EA0();

extern void address_named_map_loader_3861();
extern void address_named_world_rebuild_313D();

// 01F7:199D.  Inputs: ES:DI = persistent player record.  No caller flags
// are consumed and no stable return value is produced (RETF at 19E5).
// Writes: DS:8950, DS:89EA, DS:880A, DS:8822 and player +0x0E/+0x4C/
// +0x50/+0x5C/+0x60.  The far call at 19A9 is retained as an effect
// boundary; its result is not used by this body.
void player_instant_death_199D(PlayerRecord &player) {
    DS.transition_effect_bits_8950 = 0;       // 199D-19A2
    DS.transition_gate_89ea = 0xffff;         // 19A3-19A8
    address_named_far_call_199A();            // 19A9; opaque effect dispatch

    player.i32(0x0e, static_cast<Fixed16>(0xfffe0000U)); // 19AE-19B6
    player.i32(0x4c, static_cast<Fixed16>(0x00002000U)); // 19B7-19BF
    player.i32(0x50, static_cast<Fixed16>(0x00002000U)); // 19C0-19C8
    player.i32(0x5c, static_cast<Fixed16>(0x00018000U)); // 19C9-19D1
    player.i32(0x60, static_cast<Fixed16>(0x00040000U)); // 19D2-19DA
    DS.lives_880a = static_cast<std::uint16_t>(DS.lives_880a - 1); // 19DB
    DS.health_8822 = 0;                         // 19DF-19E5
}

// 01F7:19E6.  Inputs: ES:DI is loaded from DS:881A; BX is the caller's
// horizontal reference.  No stable caller flag is returned.  The first
// TEST/JNZ is an invulnerability gate on player +0x34.
//
// The terminal branch duplicates the 199D motion subset and then adds the
// callback-visible reset fields +0x0E/+0x3B/+0x3E/+0x37/+0x3A/+0x2B and
// clears DS:8950 bits 0x30.  The nonterminal branch only writes +0x34.
void player_damage_or_death_19E6(PlayerRecord &player,
                                 std::uint16_t incoming_bx) {
    if (player.u16(0x34) != 0)                         // 19EB-19F1
        return;                                       // 1A95-1A96

    DS.pending_event_612e = 1;                        // 19F5-19FA
    address_named_far_call_19FB();                    // 19FB; sound/effect
    DS.health_8822 = static_cast<std::uint16_t>(DS.health_8822 - 1); // 1A00

    if (DS.health_8822 != 0) {                        // 1A04 / 1A73
        player.u16(0x34, 0x00d2);                     // 1A73
    } else {
        DS.lives_880a = static_cast<std::uint16_t>(DS.lives_880a - 1); // 1A06
        DS.transition_effect_bits_8950 = 0;           // 1A0A
        player.i32(0x0e, static_cast<Fixed16>(0xfffe0000U)); // 1A10
        player.i32(0x4c, static_cast<Fixed16>(0x00002000U)); // 1A19
        player.i32(0x50, static_cast<Fixed16>(0x00002000U)); // 1A22
        player.i32(0x5c, static_cast<Fixed16>(0x00018000U)); // 1A2B
        player.i32(0x60, static_cast<Fixed16>(0x00040000U)); // 1A34
        DS.transition_gate_89ea = 0xffff;              // 1A3D
        address_named_far_call_1A43();                // 1A43; opaque effect

        player.i32(0x0e, static_cast<Fixed16>(0xfffe0000U)); // 1A48
        player.u8(0x3b, 0);                            // 1A51
        player.u16(0x3e, 0x03e8);                      // 1A56
        player.u8(0x37, 0xff);                         // 1A5C
        player.u8(0x3a, 0);                            // 1A61
        player.u8(0x2b, 0);                            // 1A66
        DS.transition_effect_bits_8950 &= 0xffcf;     // 1A6B
    }

    // 1A79-1A8C: signed 16-bit comparison of player integer X against BX;
    // the stored values are full 16.16 fixed-point constants.
    if (static_cast<std::int16_t>(player.u16(0x04)) -
            static_cast<std::int16_t>(incoming_bx) >= 0) {
        player.i32(0x0a, static_cast<Fixed16>(0x00018000U)); // 1A81
    } else {
        player.i32(0x0a, static_cast<Fixed16>(0xfffe8000U)); // 1A8C
    }
}

// 01D7:34C7.  The near caller passes a far pointer to the selected resource
// row and this function returns with RET 4.  The raw body first copies the
// length-prefixed row into a local 0x202-byte buffer, refreshes the map-backed
// resource state through address-named 0227/0207 helpers, and then performs
// the fixed 32-row coordinate publication below.  The resource helpers do
// not expose simulation flags at this boundary.
//
// 01D7:3861 computes the source offset as 0x3574 + 5*DS:85D4 and calls this
// body before the next scheduler pass.  The offset is a runtime resource
// pointer in DS; it must not be interpreted as code in the CS-relative raw
// segment import.
void map_postload_publish_spawn_rows_34C7() {
    // 357E-35B5: i is a 16-bit counter and the loop executes exactly 0x20
    // times.  Each source value is returned in AX and stored as a word after
    // the fixed authored offsets.  The additions wrap at 16 bits.
    for (std::uint16_t i = 0; i != 0x20; ++i) {
        address_named_spawn_rows_8828[i].x_word =
            static_cast<std::uint16_t>(
                address_named_resource_word_34C7_3586(
                    DS.resource_handle_97e2) + 0x20U); // 3582-3595
        address_named_spawn_rows_8828[i].y_word =
            static_cast<std::uint16_t>(
                address_named_resource_word_34C7_359D(
                    DS.resource_handle_97e2) + 0x30U); // 3599-35AC
    }
}

// 01F7:1AAA.  Inputs: DS:85D2 selects one pair in DS:8828..DS:88A7.
// No stable flags are returned.  The first and last far calls remain opaque;
// the NE relocation at 01F7:1B01 is resolved separately to this function.
void player_respawn_reinitialize_1AAA(PlayerRecord &player) {
    address_named_far_call_1AAA_entry();           // 1AAA; setup context
    const std::uint16_t row = DS.spawn_row_85d2;
    const SpawnCoordinateRow &spawn = address_named_spawn_rows_8828[row];
    player.i32(0x02, static_cast<Fixed16>(
        static_cast<std::uint32_t>(spawn.x_word) << 16)); // 1AB7-1ACA
    player.i32(0x06, static_cast<Fixed16>(
        static_cast<std::uint32_t>(spawn.y_word) << 16)); // 1ACF-1AD6
    player.u16(0x18, 0x3f27);                       // 1ADB; callback pointer
    player.u8(0x29, 1);                              // 1AE1
    DS.transition_gate_89ea = 0;                     // 1AE6
    address_named_animation_loader_5D38(0x3156);     // 1AEC-1AF4; SI=3156
}

// 01F7:1AF5.  This body has no player-record write and no stable flags.  The
// far call at 1B01 is mechanically resolved by the NE relocation table:
// 01F7:1B01 -> 01F7:1AAA.
void restore_health_and_checkpoint_1AF5() {
    DS.health_8822 = DS.maximum_health_8824;         // 1AF5-1AFA
    DS.spawn_row_85d2 = 0;                           // 1AFB-1B00
    address_named_far_call_1B01_to_1AAA();           // 1B01-1B06
}

// 01D7:4BA4-4C40.  This is the simulation-relevant branch skeleton.  The
// presentation/resource calls are not guessed; their original call sites are
// retained as address-named leaves.  The entry is reached when the outer
// main-loop state selects the lifecycle gate; it is not a per-player callback
// ABI.
void main_loop_lifecycle_gate_4BA4(PlayerRecord &player) {
    // 4BA4-4BB5: recovery/reload path is entered only when gate != 0 and
    // lives > 0.  The alternative path is 4C43, not a fall-through.
    if (DS.transition_gate_89ea != 0 && DS.lives_880a > 0) {
        // 4BB8-4BD5: suppress contact, mark lifecycle presentation state,
        // clear the recovery row, then select a MAP row only for these values.
        address_named_lifecycle_call_4BBC();         // 4BBC; opaque effect
        address_named_lifecycle_call_4BCE();         // 4BCE; opaque setup
        DS.spawn_row_85d2 = 0;                       // 4BD3

        if (DS.lives_880a != 0) {                    // 4BD8-4C1C
            address_named_lifecycle_call_4BDF();     // 4BDF; teardown/setup
            address_named_lifecycle_call_4BE4();     // 4BE4; resource stage
            // 4BE9-4C19: 85D4 selects the only five recovery rows.
            switch (address_named_recovery_selector_85D4()) {
            case 0x0002: case 0x0005: case 0x0008:
            case 0x000b: case 0x000e:
                address_named_map_loader_3861();     // 4BF1/4BFB/4C05/4C0F/4C19
                break;
            default:
                break;
            }
            address_named_lifecycle_call_4C1C();     // 4C1C
            address_named_lifecycle_call_4C21();     // 4C21
            address_named_lifecycle_call_4C26();     // 4C26
            address_named_lifecycle_call_4C2B();     // 4C2B
            address_named_lifecycle_call_4C30();     // 4C30
            address_named_world_rebuild_313D();      // 4C35
            goto common_504F;
        }

        DS.transition_event_89ec = 0xffff;            // 4C3A
        goto common_504F;                             // 4C40
    }

    // 01D7:4C43-4C5D.  The terminal condition is exact and intentionally
    // written in branch form because 4C43 has two independent gates:
    // (gate != 0 && lives == 0) OR (gate == 0 && counter != 0 && lives == 1).
    const bool terminal =
        (DS.transition_gate_89ea != 0 && DS.lives_880a == 0) ||
        (DS.transition_gate_89ea == 0 &&
         DS.transition_counter_89f0 != 0 &&
         DS.lives_880a == 1);
    if (terminal) {
        DS.invulnerability_gate_8810 = 0;             // 4C5F-4C63
        address_named_lifecycle_call_4C64();          // 4C64; state reset
        DS.lives_880a = 0;                            // 4C69-4C6B
        address_named_terminal_menu_4C79();           // 4C6E-4C79
        address_named_lifecycle_call_4C87();          // 4C87
        address_named_lifecycle_call_4C8C();          // 4C8C
        address_named_lifecycle_call_4C91();          // 4C91
        address_named_lifecycle_call_4C96();          // 4C96
        address_named_lifecycle_call_4C9B();          // 4C9B
        address_named_lifecycle_call_4CA0();          // 4CA0
        address_named_lifecycle_call_4CA5();          // 4CA5
        DS.transition_event_89ec = 0xffff;            // 4CA8
        goto common_504F;                             // 4CAE
    }

    // 01D7:4CB1-4CF9 is a distinct nonterminal transition branch.  It is
    // selected when DS:89F0 != 0 and DS:89EA == 0.  Its direct simulation
    // writes are limited to DS:8810=0 and DS:89EC=-1; the intervening calls
    // remain address-named because they may own the observed death-hold
    // timing but expose no statically recoverable callback contract here.
    if (DS.transition_counter_89f0 != 0 && DS.transition_gate_89ea == 0) {
        DS.invulnerability_gate_8810 = 0;             // 4CBF-4CC1
        address_named_lifecycle_call_4CC4();          // 4CC4; 44D0 boundary
        address_named_lifecycle_call_4CC7();          // 4CCC; opaque dispatch
        address_named_lifecycle_call_4CD5();          // 4CD1-4CEE; opaque path
        DS.transition_event_89ec = 0xffff;            // 4CF3
        goto common_504F;                             // 4CF9
    }

    // 01D7:4CFC-4EA0 is the remaining authored transition substate machine.
    // The 85DA values below are static selectors, not semantic animation
    // names.  Only the direct gate writes are modeled; visual/audio calls are
    // retained behind address-named leaves.
    if (DS.transition_substate_85da <= 1) {
        address_named_transition_pending_4EA0();      // 4CFC-4D03
        goto common_504F;
    }
    address_named_transition_substate_4D0D(
        DS.transition_substate_85da);                 // 4D06-4E8A
    if (DS.transition_substate_85da == 7 ||
        DS.transition_substate_85da == 0x32 ||
        DS.transition_substate_85da == 0x34) {
        DS.action_suppressor_89e0 = 0xffff;            // 4D? / 4E10/4E8A
    }
    DS.transition_substate_85da = static_cast<std::uint8_t>(
        DS.transition_substate_85da + 1);             // 4E? increment

common_504F:
    // 4C40/4CAE -> 504F.  The outer loop and the animation/resource waits
    // are outside this function contract; no delay constant is inferred here.
    return;
}

// 01D7:4EA0-4EFE.  This is the exact pending-transition gate reached when
// 85DA <= 1.  It is kept separate because it owns the only explicit local
// wait value visible in this branch: BP-0x3? is initialized to 0x014F at
// 4EBF.  That value is not the player +0x3E word and must not be substituted
// into the PlayerRecord.
void transition_pending_4EA0() {
    if (DS.transition_signal_89e6 == 0)                 // 4EA0-4EA7
        return;                                         // 4EA7 -> 504F

    std::uint16_t lifecycle_wait = 0x014f;              // 4EAA-4EC5
    if (DS.transition_row_85d4 == 0x000e)
        DS.action_suppressor_89e0 = 0xffff;             // 4EC5-4ECC

    // 4ED2-4EE6 selects 0x46 when DS:5044 is zero, otherwise the local
    // 0x014F value.  The selector and all following calls remain opaque.
    const std::uint16_t transition_arg =
        address_named_transition_selector_5044() == 0
            ? 0x0046 : lifecycle_wait;
    address_named_transition_loader_4EBA(transition_arg); // 4ED9/4EE6
    address_named_transition_effect_4EEB();              // 4EEB-4EFE
}

// Evidence boundary:
// - Static fact: 199D/19E6 publish the gate and terminal player writes;
//   1AF5 -> 1AAA is relocation-closed; 4BA4 selects recovery versus terminal.
// - Static fact: 4BA4 itself contains no direct death-animation countdown.
// - Static fact: 4CB1-4CF9 is a separate nonterminal gate path; 4CFC-4EA0
//   advances the 85DA transition substates and can publish DS:89E0.
// - Static fact: 4EA0-4EFE gates on DS:89E6, initializes a local 0x014F
//   wait, and selects 0x46 versus that local value through DS:5044.  The
//   timer/IRQ owner of the observed natural death hold remains unresolved.
// - Runtime fact: natural W1L1 holds the death record from sampled frames
//   1603..1813 and first shows recovery at 1816, while the player remains in
//   scheduler banks with callback 3FF8/object offset zero.
// - Unresolved: the opaque calls and timer/IRQ ownership that produce the
//   unsampled death hold, plus teardown/rebuild membership ordering.

// -------------------------------------------------------------------------
// Focused Ghidra v33 relocation expansion
// -------------------------------------------------------------------------
// The following contracts are recovered from the NE relocation targets called
// by 01D7:4BA4-4EFE.  They are deliberately kept separate from the lifecycle
// skeleton above: a far-call target can be statically closed even when its
// presentation/resource callees remain address-named.

struct LifecycleObject {
    std::uint8_t raw[0x78];
    std::uint16_t u16(std::uint16_t offset) const;
    void u16(std::uint16_t offset, std::uint16_t value);
};

struct LifecycleTargetGlobals {
    std::uint16_t scheduler_bank_7966;
    std::uint16_t scheduler_count_88c8;
    std::uint16_t player_object_881a;
    std::uint16_t health_8822;
    std::uint16_t maximum_health_8824;
    std::uint16_t spawn_row_85d2;
    std::uint16_t frame_gate_819e;
    std::uint16_t map_width_657e;
    std::uint32_t map_page_a6_81a6;
    std::uint32_t map_page_be_81be;
    std::uint16_t map_page_alias_817a;
    std::uint16_t map_page_alias_817c;
    std::uint8_t map_reload_signal_81d1;
    std::uint32_t score_digits_4ff2;
    std::uint16_t lives_digits_4ffa;
    std::uint16_t score_cursor_4ff6;
    std::uint16_t health_cursor_4ff8;
    std::uint16_t transition_resource_state_504c;
    std::uint16_t transition_resource_id_5064;
    std::uint16_t transition_effect_state_8954;
    std::uint8_t are_event_index_8960[0x80];
    std::uint16_t are_event_cursor_895e;
    std::int32_t camera_target_x_60dc;
    std::int32_t camera_target_y_60e0;
    std::uint32_t camera_scroll_x_81a6;
    std::uint32_t camera_scroll_y_base_81aa;
    std::uint32_t camera_scroll_x_prev_81ae;
    std::uint32_t camera_scroll_y_prev_81b2;
    std::uint32_t camera_scroll_x_base_81be;
    std::uint32_t camera_scroll_y_81c2;
    std::uint32_t camera_page_divisor_81ca;
    struct AREEventSlot {
        std::uint32_t pointer;
        std::uint32_t source;
        std::uint8_t event_record_high_byte;
    } are_event_slot_6586[0x80];
};

extern LifecycleTargetGlobals DS;

// 01F7:106A, called by 01D7:4BCE/4C9B/4CE9.  Its entry relocation calls
// 01F7:17D4 once; the loop relocation at 10A1 calls 01F7:1DEE for sourced
// objects.  The selected scheduler bank is then walked in eight-byte entries.
// Objects with +0x1A == FFFF have their callback pointer (+0x18) cleared.
// No stable flags are returned.
void clear_dead_scheduler_callbacks_106A() {
    clear_are_event_queue_17D4();
    const std::uint16_t bank = static_cast<std::uint16_t>(
        (DS.scheduler_bank_7966 + 0x0200U) & 0x0200U);
    for (auto entry = scheduler_bank_7566(bank);
         entry.callback != 0xffff; entry = entry.next()) {
        if (entry.object.u16(0x1a) == 0xffff)
            entry.object.u16(0x18, 0);
        else
            deactivate_object_outside_camera_1DEE(entry.object_offset);
    }
}

// 01F7:17D4, the only relocated target reached from 106A.  Inputs and return
// flags are unused.  It clears 0x80 eight-byte event slots at DS:6586; for
// every nonzero slot it clears the high byte of the pointed-to event record
// and then clears the slot pointer itself.  This is gameplay-relevant only
// through the pending ARE-event queue; it does not write the player record,
// descriptor table, or scheduler bank.
void clear_are_event_queue_17D4() {
    for (std::uint16_t index = 0; index != 0x80; ++index) {
        auto &slot = DS.are_event_slot_6586[index];
        if (slot.pointer != 0) {
            slot.event_record_high_byte = 0;
            slot.pointer = 0;
        }
    }
}

// 01F7:17AE, called by 321F before the page-copy loop.  It initializes the
// 0x80-byte event-slot index table at DS:8960 to 0..0x7F, clears DS:895E,
// and clears the 0x80 event-slot pointers at DS:6586.  No flags or player
// record fields are returned or written.
void initialize_are_event_slots_17AE() {
    for (std::uint16_t index = 0; index != 0x80; ++index)
        DS.are_event_index_8960[index] = static_cast<std::uint8_t>(index);
    DS.are_event_cursor_895e = 0;
    clear_are_event_queue_17D4();
}

// 01F7:20AF.  Inputs are EAX=target X delta and EBX=target Y delta.  The Y
// delta is clamped to -0x40000; the two deltas are published in DS:60DC/60E0
// for the camera scroll helper.  No player record or descriptor write occurs.
void publish_camera_target_delta_20AF(std::int32_t target_x,
                                      std::int32_t target_y) {
    if (target_y < -0x40000)
        target_y = -0x40000;
    DS.camera_target_x_60dc = target_x;
    DS.camera_target_y_60e0 = target_y;
}

// 01F7:31D1.  Inputs are EAX/EBX fixed-point camera coordinates.  It clamps
// both coordinates to nonnegative values after subtracting the authored
// offsets 0xA00000 and 0x320000, derives the camera page quotient/remainder
// using DS:81CA, and publishes DS:81A6/81AA/81BE/81C2.  The near return is
// AX = (adjusted_y - (DS:81CA >> 1)) / DS:81CA.
std::uint16_t set_camera_origin_31D1(std::int32_t x, std::int32_t y) {
    const std::int32_t adjusted_x = std::max<std::int32_t>(x - 0xA00000, 0);
    const std::int32_t adjusted_y = std::max<std::int32_t>(y - 0x320000, 0);
    const std::uint32_t page = DS.camera_page_divisor_81ca;
    DS.camera_scroll_y_81c2 = static_cast<std::uint32_t>(
        adjusted_y) - (page >> 1);
    DS.camera_scroll_x_81a6 = adjusted_x;
    DS.camera_scroll_x_base_81be = adjusted_x;
    DS.camera_scroll_y_base_81aa = DS.camera_scroll_y_81c2 % page;
    return static_cast<std::uint16_t>(DS.camera_scroll_y_81c2 / page);
}

// 01F7:1ED7.  This helper is the camera/map bridge called by 321F.  It
// snapshots 81A6/81AA into 81AE/81B2, adds the published 60DC/60E0 deltas,
// clamps X/Y against target bounds at 36FC..3702, and updates 81BE/81C2.
// It then emits one map-column or map-strip refresh per changed page index,
// updates 81A8/81AC and 370A, and finally publishes 81CE/81D0 plus the new
// 81A6/81AA positions.  The map rendering calls are presentation contracts;
// the camera globals are direct state that later player probes read.
void update_camera_scroll_1ED7();

// 01F7:3062 is the VGA page-copy consumer reached by 321F/1ED7.  It copies
// prepared map/render rows using 817A/817C and 5C9A, updates 8176, and calls
// address-named glyph/render helpers.  Its port state and return packing do
// not write player, descriptor, camera, or scheduler state; stop the static
// closure at this presentation contract.
void render_map_page_3062();

// 01F7:1AF5, called by the recovery path.  The body is fully direct except
// for its final relocated call, which is already resolved to 01F7:1AAA.
void restore_health_and_respawn_row_1AF5() {
    DS.health_8822 = DS.maximum_health_8824;
    DS.spawn_row_85d2 = 0;
    player_respawn_reinitialize_1AAA(/* ES:DI persistent player */);
}

// 01F7:321F, called by the common recovery/reload tail.  It advances the two
// fixed-point MAP page cursors by 0x02000000, performs one of two 0x200-entry
// page-copy loops, swaps the page aliases at 817A/817C, publishes 81D1=1,
// and restores the saved cursors.  The helper has no direct player-record
// write, but its map-page writes feed later descriptor probes.
void rebuild_map_pages_321F() {
    const std::uint32_t saved_be = DS.map_page_be_81be;
    const std::uint32_t saved_a6 = DS.map_page_a6_81a6;
    DS.map_page_be_81be += 0x02000000U;
    DS.map_page_a6_81a6 += 0x02000000U;
    if (static_cast<std::int32_t>((DS.map_page_be_81be >> 0x13) + 0x2c) <=
        static_cast<std::int32_t>(DS.map_width_657e)) {
        address_named_map_page_copy_321F(/* 0x200 entries */);
    } else {
        DS.map_page_be_81be -= 0x04000000U;
        DS.map_page_a6_81a6 -= 0x04000000U;
        address_named_map_page_copy_321F(/* 0x200 entries */);
    }
    DS.map_page_be_81be = saved_be;
    DS.map_page_a6_81a6 = saved_a6;
}

// 01F7:5BEF, called during recovery/terminal presentation setup.  These are
// the direct auxiliary-display resets used by 5937; they do not touch the
// player record, DS:89EA, the map, or the scheduler.
void reset_score_display_cursors_5BEF() {
    DS.score_digits_4ff2 = 0xffffffffU;
    DS.lives_digits_4ffa = 0xffff;
    DS.score_cursor_4ff6 = 0xffff;
    DS.health_cursor_4ff8 = 0;
    address_named_score_display_reset_5BEF();
}

// 0207:0002.  Inputs: far-call stack word count (the decompiler exposes the
// first word as param_1 and the count as param_2).  It clears DS:819E and
// waits for the timer IRQ to publish a nonzero value once per requested tick.
// No player/global simulation fields are directly modified.
void wait_for_timer_ticks_0207_0002(std::uint16_t count) {
    while (count-- != 0) {
        DS.frame_gate_819e = 0;
        address_named_timer_yield_0207_0002();
        while (DS.frame_gate_819e == 0)
            address_named_timer_yield_0207_0002();
    }
}

// 0207:022A.  This is a presentation/resource gate.  It sets 60A8 to states
// 7/8 and, on first entry, calls address-named buffer routines before latching
// 5C96=1.  It has no player or scheduler write.
void begin_transition_effect_0207_022A(std::uint8_t mode,
                                       std::uint16_t resource) {
    address_named_transition_effect_setup_022A(mode, resource);
}

// 0207:08D8 and 0207:17A0 program VGA/PIT/DOS services and update only the
// presentation/timer boundary.  They are closed contracts for player parity:
// neither body writes the player record or gameplay globals.
void transition_vga_flush_0207_08D8();
void transition_timer_yield_0207_17A0();

// 01E7:0C71 and 01E7:0CAA set the resource-state word 504C to 0x17 and 0x16
// respectively when 5044 is nonzero, then call address-named resource loops.
// 01E7:082D is the transition substate leaf used by 4D0F-4E7D; its visible
// body only forwards its two input bytes to address-named resource helpers.
// 01E7:0D18 returns its incoming CX/param_4 and updates 504C/5064/8954 while
// selecting a resource/audio path.  None of these four bodies writes the
// player record, DS:89EA, DS:89EC, MAP, or scheduler banks.
std::uint16_t transition_resource_leaf_0D18(/* incoming CX */);

// 01F7:0908 is a bounded 0..999 dispatch loop.  Its per-index target remains
// address-named; the direct body has no known player/global store.  Retain it
// as a contract until a parity mismatch attributes gameplay feedback to the
// indirect target.
void bounded_transition_dispatch_0908() {
    for (std::uint16_t index = 0; index <= 999; ++index)
        address_named_transition_index_callback_0908(index);
}

// Static closure result for 01D7:4BA4-4EFE:
//
//   simulation-affecting direct targets:
//       01F7:106A -> 01F7:17D4  scheduler callback/event-queue cleanup
//       01F7:17AE/1ED7/20AF/31D1  event initialization and camera state
//       01F7:1AF5  health/spawn reset -> 1AAA
//       01F7:321F  MAP page rebuild/publication
//       01F7:5BEF  auxiliary display cursor reset only
//       0207:0002 frame gate only
//   address-named contracts with no direct simulation write:
//       01E7:082D/0C71/0CAA/0D18, 01F7:0908/F07B/F111,
//       0207:022A/08D8/17A0.
//   Runtime boundaries still requiring traces:
//       the timer/IRQ owner of the observed death hold and scheduler
//       membership across teardown/reload.  The rendering leaves below 1ED7
//       and 3062 are closed presentation contracts.

// Focused runtime corroboration (natural W1L1, 2026-08-27):
//
//   research/evidence/player-dos-parity/
//       player-death-recovery-focused-v5.json
//
// The protected-mode trace watches 01F7:1AAA and samples the persistent
// player callback at 01F7:3FF8.  The recovery entry is observed at frame 1770
// (the debugger stop is at 01F7:1AE6); the first subsequent player callback
// sample is frame 1780.  That callback already contains position
// (0x06890000, 0x01700000), zero X/Y velocity, +0x37=0, +0x36=FF,
// +0x38=FF, +0x3B=1, +0x3E=0x03E8, callback 0x3FF8, and animation
// descriptor/cursor 0x316C.  Health is 3 and lives is 3 at the recovery
// watch and first recovered callback.
//
// This confirms the runtime order 1AAA -> next 3FF8 callback and agrees with
// the static 1AF5 -> 1B01 -> 1AAA relocation.  The watch stopped with ES=0,
// DI=0x0348, so its object snapshot is intentionally rejected; the recovered
// player record is taken only from the following callback barrier.  The
// trace does not identify the unsampled 19E6/199D setter, the timer/IRQ hold
// condition, or the exact scheduler/resource teardown order.  Those remain
// address-named contracts rather than a guessed native countdown.
