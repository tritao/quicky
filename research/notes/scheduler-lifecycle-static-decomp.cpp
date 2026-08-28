// Focused Ghidra reconstruction of the scheduler reset/rebuild closure used
// by player recovery and the selector handoff.
//
// Source executable SHA-256:
//   c9b2e59febd6fa0ea271bedf360459353f55c74444f026964b70988d6de1bca1
// Static source:
//   x86:LE:16:Protected Mode, raw NE segment import, no whole-program
//   analysis, with the independent Ghidra projects and instruction listing
//   export from player-external-state-closure-recovery-v59.
//
// This is address-annotated C-like contract text.  It preserves the exact
// scheduler words and ordering that can feed player simulation; it does not
// assign meanings to unresolved far calls.

#include <cstdint>

struct SchedulerEntry {
    std::uint16_t phase_callback_offset;      // +0x00
    std::uint16_t secondary_callback_offset;  // +0x02
    std::uint16_t object_offset;              // +0x04
    std::uint16_t entry_word_06;              // +0x06, not consumed here
};

struct SchedulerGlobals {
    std::uint16_t insert_cursor_7966;
    std::uint16_t scheduler_bank_a_7566;
    std::uint16_t scheduler_bank_b_7766;
    std::uint16_t object_stride_30ce;
    std::uint16_t player_object_offset_881a;
};

extern SchedulerGlobals DS;

struct PooledObject {
    std::uint8_t raw[0x78];
    std::uint16_t u16(std::uint16_t offset) const;
    void u16(std::uint16_t offset, std::uint16_t value);
    std::uint8_t u8(std::uint16_t offset) const;
    void u8(std::uint16_t offset, std::uint8_t value);
};

extern PooledObject &pooled_object(std::uint16_t object_offset);
extern SchedulerEntry &scheduler_entry_at(std::uint16_t address);
struct AREEventSlot {
    std::uint16_t pointer;
    std::uint16_t source;
    std::uint16_t event_record_high_byte;
    std::uint16_t reserved;
};
extern AREEventSlot &are_event_slot(std::uint16_t index);
extern void call_near_callback(std::uint16_t callback_offset,
                               std::uint16_t object_offset);
extern void object_pool_factory_0E06(std::uint16_t callback_offset);
extern void address_named_preclean_106A();
extern void clear_are_event_queue_17D4(std::uint16_t selected_bank_bit);

// 01F7:0B56.  This is the player-object reconstruction used by the natural
// selector handoff and by the 01F7:1AAA recovery initializer.  The far call at
// 0B6F is 01F7:0E06; it returns ES:DI for the newly allocated object.  No
// caller-consumed return flags are established.
void player_object_reinitialize_0B56() {
    DS.insert_cursor_7966 = 0;                 // 0B56
    DS.scheduler_bank_a_7566 = 0xffff;         // 0B5C-0B62
    DS.scheduler_bank_b_7766 = 0xffff;         // 0B64-0B67

    // DX=0 and AX=0x3F27 are present immediately before the far call.
    object_pool_factory_0E06(0x3f27);          // 0B69-0B6F, ES:DI result
    const std::uint16_t player_offset = returned_DI; // ES:DI from 0E06
    DS.player_object_offset = player_offset;   // 0B74; DS:881A
    pooled_object(player_offset).u8(0x17, 2);   // 0B78; phase 2
}

// 01F7:1036.  Input: ES:DI pooled object.  The body has an early RETF when
// object +0x18 is zero.  Otherwise it appends one eight-byte entry to the
// bank selected by DS:7966, then writes the next entry's phase word as FFFF.
// The object's secondary callback is copied before its phase callback and
// object offset.  No stable caller-consumed return value or flags are used.
void register_object_scheduler_entry_1036(std::uint16_t object_offset) {
    PooledObject &object = pooled_object(object_offset);
    if (object.u16(0x18) == 0)                    // 1036-103E
        return;

    SchedulerEntry &entry = scheduler_entry_at(
        static_cast<std::uint16_t>(0x7566 + DS.insert_cursor_7966));
    entry.secondary_callback_offset = object.u16(0x1c); // 104A-104E
    entry.phase_callback_offset = object.u16(0x18);     // 1051-1055
    entry.object_offset = object_offset;                // 1057
    DS.insert_cursor_7966 = static_cast<std::uint16_t>(
        DS.insert_cursor_7966 + 8);                     // 105A
    scheduler_entry_at(static_cast<std::uint16_t>(
        0x7566 + DS.insert_cursor_7966)).phase_callback_offset = 0xffff;
                                                            // 105F
}

// 01F7:106A.  The first far call is retained as an address-named boundary.
// Its visible scheduler body toggles the bank selector and walks the bank
// selected by the old selector value.  It clears a callback when object +0x1A
// is FFFF; otherwise it calls 17D4 with the old bank bit in BX.  The body has
// no stable return flags and does not write the player record directly.
void clear_selected_scheduler_callbacks_106A() {
    address_named_preclean_106A();              // 106A; unresolved far call

    const std::uint16_t old_cursor = DS.insert_cursor_7966; // 1074
    DS.insert_cursor_7966 = static_cast<std::uint16_t>(
        (old_cursor + 0x0200U) & 0x0200U);      // 1077-107F
    const std::uint16_t old_bank_bit = old_cursor & 0x0200U;
    std::uint16_t entry_address = static_cast<std::uint16_t>(
        0x7566 + old_bank_bit);                // 1082-1088

    while (scheduler_entry_at(entry_address).phase_callback_offset != 0xffff) {
        const std::uint16_t object_offset =
            scheduler_entry_at(entry_address).object_offset; // 1093
        PooledObject &object = pooled_object(object_offset);
        if (object.u16(0x1a) == 0xffff)       // 1099-109E
            object.u16(0x18, 0);               // 10A9; remove phase callback
        else
            clear_are_event_queue_17D4(old_bank_bit); // 10A0-10A6
        entry_address = static_cast<std::uint16_t>(entry_address + 8);
    }
}

// 01F7:17D4.  Input BX is preserved by the caller but is not consumed by the
// visible body.  It clears 0x80 eight-byte ARE-event slots at DS:6586.  For a
// nonzero slot, the pointed record's byte +0x01 is cleared and the slot's
// pointer is then cleared.  It does not write the player record or scheduler
// entries and returns with RETF.
void clear_are_event_queue_17D4(std::uint16_t /*selected_bank_bit*/) {
    for (std::uint16_t index = 0; index != 0x80; ++index) {
        AREEventSlot &slot = are_event_slot(index);
        if (slot.pointer != 0) {
            slot.event_record_high_byte = 0;    // 17DF-17E7
            slot.pointer = 0;                   // 17EC
        }
    }
}

// 01F7:0FA2.  Input is the active scheduler bank selected by DS:7966 bit
// 0x0200.  It stops at an entry whose phase callback word is FFFF, skips a
// zero secondary callback, and invokes the +0x02 callback with near CALL AX.
// Its callbacks' return flags are not consumed and the pass has no direct
// player/global simulation write.
void dispatch_secondary_callbacks_0FA2() {
    std::uint16_t entry_address = static_cast<std::uint16_t>(
        0x7566 + (DS.insert_cursor_7966 & 0x0200U)); // 0FA2
    while (scheduler_entry_at(entry_address).phase_callback_offset != 0xffff) {
        const SchedulerEntry &entry = scheduler_entry_at(entry_address);
        if (entry.secondary_callback_offset != 0)
            call_near_callback(entry.secondary_callback_offset,
                               entry.object_offset); // 0FBA-0FCD
        entry_address = static_cast<std::uint16_t>(entry_address + 8);
    }
}

// Recovery order proven by the contiguous Protected Mode listing and the
// NE relocation table:
//
//   01D7:4BA4 gate
//       -> 01F7:106A at 4BCE
//       -> 01D7:4BD8 row selector / recovery branch
//       -> 01D7:4C2B -> 01F7:1AF5
//       -> 01F7:1B01 -> 01F7:1AAA
//       -> 01F7:1AAA first far call -> 01F7:0B56
//       -> 01F7:0E06 -> 01F7:1036
//
// The remaining 4BBC, 4BDF, 4BE4, 4C1C, 4C21, 4C26, 4C30, and 4C35 calls
// retain address-qualified contracts.  Their exact resource/teardown effects
// are not inferred from this scheduler closure.  Runtime evidence confirms
// the player identity (callback 3FF8, object offset zero) before and after the
// handoff, but does not promote the opaque calls to simulation contracts.
