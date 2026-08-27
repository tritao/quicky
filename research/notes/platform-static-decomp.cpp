// Focused Ghidra-derived C-like representation of the moving-platform
// closure in QUIKY.EXE segment 01F7.
//
// This file intentionally keeps the original offsets and byte fields. It is
// a source for the later C++ platform callback, not a modernized physics
// abstraction. Unknown far calls remain address-named and their flags are
// explicit at each call site.

#include <cstdint>

using Fixed16 = std::int32_t;

struct Flags {
    bool zf;
    bool cf;
};

struct PlatformRecord {
    std::uint8_t bytes[0x78];

    std::uint8_t u8(std::uint16_t off) const;
    void u8(std::uint16_t off, std::uint8_t value);
    std::uint16_t u16(std::uint16_t off) const;
    void u16(std::uint16_t off, std::uint16_t value);
    Fixed16 i32(std::uint16_t off) const;
    void i32(std::uint16_t off, Fixed16 value);
};

// The scheduler uses the same pooled 0x78-byte record layout. Keep this alias
// address-qualified instead of assigning a broader object-family type here.
using ObjectRecord = PlatformRecord;

struct PlatformGlobals {
    std::uint16_t map_row_stride;       // DS:657E
    std::uint16_t map_height_rows;      // DS:6580
    std::uint16_t camera_x;             // DS:81C0
    std::uint16_t camera_y;             // DS:81C4
    std::uint16_t contact_latch_5006;   // DS:5006
    Fixed16 player_x_carry_8816;        // DS:8816
    Fixed16 player_y_carry_8812;        // DS:8812
    std::uint16_t player_offset_881A;   // DS:881A
    std::uint16_t scheduler_view_state_8174; // DS:8174, cleared by 0FDC
};

extern PlatformGlobals DS;

// 01F7:39FE returns AX=tracked-player integer X, BX=integer Y, and CL=the
// signed player +0x37 mode byte. If DS:89EA is nonzero, AX and BX are zero.
struct TrackedPlayerPosition {
    std::int16_t x;
    std::int16_t y;
    std::int8_t mode;
};
extern TrackedPlayerPosition read_player_position_and_mode_39FE();

// 01F7:5DC3 returns only flags. Every platform call below consumes ZF, not a
// C/C++ boolean inferred from a decompiler return type.
extern Flags probe_map_raw_block_bit_5DC3(std::int16_t y, std::int16_t x);
extern void deactivate_platform_outside_camera_A06F(PlatformRecord &platform);

// Address-named access to the persistent player record used only by A0B2.
extern Fixed16 player_record_i32(std::uint16_t pool_offset,
                                 std::uint16_t field_offset);

// 01F7:A075. Strict inequalities are preserved exactly from the CMP/Jcc
// sequence: object_x < player_x < object_x+width and
// object_y <= player_y < object_y+12. A negative player mode rejects the
// overlap. The one-byte +0x58 cooldown is checked before calling 39FE.
void test_platform_player_overlap_A075(PlatformRecord &platform) {
    if (platform.u8(0x58) != 0) {
        platform.u8(0x58, static_cast<std::uint8_t>(platform.u8(0x58) - 1));
        return;
    }

    const TrackedPlayerPosition player = read_player_position_and_mode_39FE();
    if (player.mode < 0 ||
        player.x <= static_cast<std::int16_t>(platform.u16(0x04)) ||
        player.x >= static_cast<std::int16_t>(
            platform.u16(0x04) + platform.u16(0x3E)) ||
        player.y < static_cast<std::int16_t>(platform.u16(0x08)) ||
        player.y >= static_cast<std::int16_t>(platform.u16(0x08) + 0x0C)) {
        return;
    }
    DS.contact_latch_5006 = 0xFFFF;
}

// 01F7:A0B2. The X expression intentionally performs the original 16-bit
// subtraction before the 32-bit shift. The +1 is part of the DOS contract;
// it is not a rounding convenience. Y is a wrapping 32-bit subtraction
// against the persistent player record at DS:881A.
void publish_platform_carry_A0B2(PlatformRecord &platform) {
    if (DS.contact_latch_5006 != 0) {
        const std::uint16_t x_delta16 = static_cast<std::uint16_t>(
            platform.u16(0x04) - platform.u16(0x2A));
        DS.player_x_carry_8816 = static_cast<Fixed16>(
            (static_cast<std::uint32_t>(x_delta16) << 16) + 1U);

        // Mechanical form of A0CC-A0E0; ES:DI is temporarily replaced with
        // DS:881A, and the raw player accessor remains address-named.
        DS.player_y_carry_8812 = static_cast<Fixed16>(
            static_cast<std::uint32_t>(
                player_record_i32(DS.player_offset_881A, 0x06) -
                platform.i32(0x06)) + 1U);
        platform.u8(0x5A, 0xFF);
        platform.u8(0x59, 0x00);
        return;
    }

    if (platform.u8(0x5A) == 0xFF) {
        platform.u8(0x5A, 0x00);
        platform.u8(0x58, 0x14);
    }
}

// 01F7:9DC7. The body below preserves the static state-machine structure;
// labels are kept at the original response joins. The four platform motion
// fields are +0x02/+0x06 position, +0x0A/+0x0E velocity, with direction and
// latches at +0x4C/+0x4E/+0x50/+0x52/+0x54/+0x58/+0x59/+0x5A.
void update_platform_9DC7(PlatformRecord &platform) {
    // Static boundary: 9DC7 and its A06F -> 1DEE cull tail never write the
    // player record.  A platform cannot directly synthesize a player landing,
    // side/ceiling crush, or detachment here; those effects must arise from
    // DS:5006/DS:8812/DS:8816 consumed by 3FF8, or from scheduler membership
    // changes after the platform object is released.
    const std::int16_t x = static_cast<std::int16_t>(platform.u16(0x04));
    const std::int16_t y = static_cast<std::int16_t>(platform.u16(0x08));
    if (x < 0 || y < 0 ||
        static_cast<std::int16_t>(DS.map_row_stride * 8U - platform.u16(0x04)) < 0 ||
        static_cast<std::int16_t>(DS.map_height_rows * 16U - platform.u16(0x08)) < 0) {
        deactivate_platform_outside_camera_A06F(platform); // 9DC7:A06F
        return;
    }

    DS.contact_latch_5006 = 0;
    test_platform_player_overlap_A075(platform);           // 9DC7:9E03
    platform.u16(0x2C, platform.u16(0x08));                 // 9DC7:9E0E
    platform.u16(0x2A, platform.u16(0x04));                 // 9DC7:9E12

    if (platform.u8(0x59) == 1) {                           // 9DC7:9E16
        publish_platform_carry_A0B2(platform);              // 9DC7:A06B
        return;
    }

    if (platform.u8(0x4A) != 0) {
        // Horizontal variant; all motion and tile probes use the exact
        // 0xFFF0 snap masks and 0x46 blocked-motion delay from 9E20-9F63.
        if (static_cast<std::int16_t>(platform.u16(0x08) - DS.camera_y + 0x100U) >
            0x2B0) {
            deactivate_platform_outside_camera_A06F(platform); // 9E3A
            return;
        }
        if (platform.u16(0x54) != 0) {
            platform.u16(0x54, platform.u16(0x54) - 1);
        } else if (platform.i32(0x0A) != 0) {
            const std::uint16_t old_x = platform.u16(0x04);
            platform.i32(0x02, static_cast<Fixed16>(
                static_cast<std::uint32_t>(platform.i32(0x02)) +
                static_cast<std::uint32_t>(platform.i32(0x0A))));
            if (((platform.u16(0x04) ^ old_x) & 0xFFF0U) != 0) {
                if (platform.u8(0x4E) == 1) {
                    const Flags blocked = probe_map_raw_block_bit_5DC3(
                        static_cast<std::int16_t>(platform.u16(0x08)),
                        static_cast<std::int16_t>((platform.u16(0x04) & 0xFFF0U) +
                                                  platform.u16(0x3E) - 0x10));
                    if (blocked.zf) goto platform_tail_A06B;
                } else {
                    const Flags blocked = probe_map_raw_block_bit_5DC3(
                        static_cast<std::int16_t>(platform.u16(0x08)),
                        static_cast<std::int16_t>((platform.u16(0x04) & 0xFFF0U) + 0x10));
                    if (blocked.zf) goto platform_tail_A06B;
                }
                platform.i32(0x0A, 0);
                platform.u16(0x54, 0x46);
                platform.u16(0x04, platform.u16(0x04) & 0xFFF0U);
                if (platform.u8(0x4E) != 1) {
                    platform.u16(0x04, platform.u16(0x04) + 0x10);
                }
            }
        } else if (platform.u16(0x52) != 0) {
            platform.u16(0x52, platform.u16(0x52) - 1);
        } else {
            const Flags edge = probe_map_raw_block_bit_5DC3(
                static_cast<std::int16_t>(platform.u16(0x08)),
                static_cast<std::int16_t>(platform.u16(0x04) + platform.u16(0x3E)));
            if (platform.u8(0x50) == 0xFF) {
                platform.u8(0x50, 0);
                if (edge.zf) goto platform_forward_A04A;
                goto platform_reverse_A035;
            }
            const Flags behind = probe_map_raw_block_bit_5DC3(
                static_cast<std::int16_t>(platform.u16(0x08)),
                static_cast<std::int16_t>(platform.u16(0x04) - 0x10));
            if (!behind.zf) goto platform_forward_A04A;
            const Flags ahead = probe_map_raw_block_bit_5DC3(
                static_cast<std::int16_t>(platform.u16(0x08)),
                static_cast<std::int16_t>(platform.u16(0x04) + platform.u16(0x3E)));
            if (!ahead.zf || platform.u8(0x4E) != 0xFF) goto platform_forward_A04A;
            goto platform_reverse_A035;
        }
        goto platform_tail_A06B;
    }

    // Vertical variant; this is the same state machine with X/Y exchanged.
    if (static_cast<std::int16_t>(platform.u16(0x04) - DS.camera_x + 0x100U) >
        0x340) {
        deactivate_platform_outside_camera_A06F(platform); // 9D7C
        return;
    }
    if (platform.u16(0x54) != 0) {
        platform.u16(0x54, platform.u16(0x54) - 1);
    } else if (platform.i32(0x0E) != 0) {
        const std::uint16_t old_y = platform.u16(0x08);
        platform.i32(0x06, static_cast<Fixed16>(
            static_cast<std::uint32_t>(platform.i32(0x06)) +
            static_cast<std::uint32_t>(platform.i32(0x0E))));
        if (((platform.u16(0x08) ^ old_y) & 0xFFF0U) != 0) {
            const Flags blocked = probe_map_raw_block_bit_5DC3(
                static_cast<std::int16_t>((platform.u16(0x08) & 0xFFF0U) +
                                          (platform.u8(0x4C) == 1 ? 0x10 : 0)),
                static_cast<std::int16_t>(platform.u16(0x04)));
            if (!blocked.zf) {
                platform.i32(0x0E, 0);
                platform.u16(0x54, 0x46);
                platform.u16(0x08, platform.u16(0x08) & 0xFFF0U);
                if (platform.u8(0x4C) != 1) platform.u16(0x08, platform.u16(0x08) + 0x10);
            }
        }
    } else if (platform.u16(0x52) != 0) {
        platform.u16(0x52, platform.u16(0x52) - 1);
    } else {
        const Flags up = probe_map_raw_block_bit_5DC3(
            static_cast<std::int16_t>(platform.u16(0x08) + 0x10),
            static_cast<std::int16_t>(platform.u16(0x04)));
        if (!up.zf) goto platform_reverse_A051;
        const Flags down = probe_map_raw_block_bit_5DC3(
            static_cast<std::int16_t>(platform.u16(0x08) - 0x10),
            static_cast<std::int16_t>(platform.u16(0x04)));
        if (down.zf && platform.u8(0x4C) == 1) goto platform_forward_A03D;
        goto platform_reverse_A051;
    }
    goto platform_tail_A06B;

platform_forward_A04A:
    platform.i32(0x0A, 0x28000);
    platform.u16(0x04, platform.u16(0x04) + 1);
    platform.u8(0x4E, 1);
    goto platform_tail_A06B;
platform_reverse_A035:
    platform.i32(0x0A, static_cast<Fixed16>(0xFFFD8000));
    platform.u16(0x04, platform.u16(0x04) - 1);
    platform.u8(0x4E, 0xFF);
    goto platform_tail_A06B;
platform_forward_A03D:
    platform.i32(0x0E, 0x28000);
    platform.u16(0x08, platform.u16(0x08) + 1);
    platform.u8(0x4C, 1);
    goto platform_tail_A06B;
platform_reverse_A051:
    platform.i32(0x0E, static_cast<Fixed16>(0xFFFD8000));
    platform.u16(0x08, platform.u16(0x08) - 1);
    platform.u8(0x4C, 0xFF);

platform_tail_A06B:
    publish_platform_carry_A0B2(platform);
}

// Focused Ghidra audit v29: the platform/player ordering is a pair of
// relocated calls at each main-loop handoff.  These are kept as a separate
// representation because the raw segment import cannot resolve far-call
// slots by itself; research/tools/ne_relocs.py supplies the target identity.
struct SchedulerPassContract {
    std::uint16_t phase_pass;       // 01F7:0E96
    std::uint16_t nonzero_pass;     // 01F7:0FA2
};

constexpr SchedulerPassContract kMainLoopSchedulerOrder[] = {
    {0x0E96, 0x0FA2}, // 01D7:44FA -> 4518
    {0x0E96, 0x0FA2}, // 01D7:47FC -> 481A
    {0x0E96, 0x0FA2}, // 01D7:4872 -> 4890
};

// 01F7:0FA2. The entry is eight bytes, but it is not a far callback pointer.
// The protected-mode listing shows two independent near callback words:
// +0x00 is the phase callback consumed by 0E96, +0x02 is the secondary
// nonzero-state callback consumed by 0FA2, and +0x04 is the pooled-object
// offset reloaded into DI. The word at +0x06 is not read by 0E96, 0FA2, or
// 0FDC and therefore remains address-named. The next entry's +0x00 word is
// the FFFF terminator written by 1036.
struct SchedulerEntry {
    std::uint16_t phase_callback_offset;    // entry +0x00; 0E96 CALL AX
    std::uint16_t secondary_callback_offset;// entry +0x02; 0FA2 CALL AX
    std::uint16_t object_offset;            // entry +0x04; ES:DI object
    std::uint16_t entry_word_06;            // entry +0x06; not read by these passes
};

extern ObjectRecord &object_at(std::uint16_t object_offset);
extern std::uint16_t scheduler_entry_offset(const SchedulerEntry *entry);
extern std::uint8_t &address_named_fs_byte(std::uint16_t offset);

// Address-named indirect calls preserve the unresolved runtime target while
// making the static register contract explicit. Both 0E96 and 0FA2 call a
// near offset in the current code segment after PUSHA/PUSH ES/PUSH DS. 0FDC
// does not call the +0x02 word; it only tests that word and then invokes its
// separate fixed far presentation/import boundary.
extern void address_named_scheduler_callback_0FA2(
    std::uint16_t secondary_callback_offset,
    std::uint16_t object_offset,
    std::uint16_t scheduler_entry_offset,
    std::uint16_t scheduler_context_7560,
    std::uint16_t saved_bp,
    std::uint16_t callback_return_scratch,
    std::uint16_t inherited_bx,
    std::uint16_t inherited_dx,
    std::uint16_t inherited_cx);
extern void address_named_scheduler_callback_0FDC(
    std::uint16_t secondary_callback_offset,
    std::uint16_t object_offset,
    std::uint16_t scheduler_entry_offset,
    std::uint16_t scheduler_context_7560,
    std::uint16_t object_x_ax,
    std::uint16_t object_y_bx,
    std::uint16_t object_y_high_mode_cx,
    std::uint16_t object_flags_dx);

// The real call is near CALL AX after AX was loaded from entry +0x02. PUSHA,
// PUSH ES, and PUSH DS preserve the caller context; after return the scheduler
// restores DS/ES and advances exactly eight bytes. No callback flags are
// consumed by this pass. The phase callback at +0x00 is not consulted here.
void dispatch_nonzero_scheduler_entries_0FA2(
    SchedulerEntry *active_bank,
    std::uint16_t scheduler_context_7560,
    std::uint16_t saved_bp,
    std::uint16_t callback_return_scratch,
    std::uint16_t bx,
    std::uint16_t dx,
    std::uint16_t cx) {
    for (SchedulerEntry *entry = active_bank;
         entry->phase_callback_offset != 0xffff;
         ++entry) {
        if (entry->secondary_callback_offset == 0)
            continue;

        // Static fact: the callback receives the object offset in DI/ES
        // context and the scheduler entry offset as a separate argument. The
        // target remains address-named until runtime table data identifies it.
        address_named_scheduler_callback_0FA2(
            entry->secondary_callback_offset,
            entry->object_offset, scheduler_entry_offset(entry),
            scheduler_context_7560, saved_bp, callback_return_scratch,
            bx, dx, cx);
    }
}

// 01F7:0FDC, the tail invoked by 0E96. It has a distinct presentation
// contract: entry +0x00 is the sentinel word, entry +0x02 is tested for
// presence, and the fixed far import at 1024 receives object +0x12 in DX
// (bit 0x8000 suppresses the import), object +0x04/+0x08 in AX/CX and BX,
// and object +0x16 in CL. DS:8174 is cleared before walking the same
// eight-byte bank entries.
void dispatch_visible_scheduler_entries_0FDC(
    SchedulerEntry *active_bank,
    std::uint16_t scheduler_context_7560) {
    DS.scheduler_view_state_8174 = 0;
    for (SchedulerEntry *entry = active_bank;
         entry->phase_callback_offset != 0xffff;
         ++entry) {
        if (entry->secondary_callback_offset == 0)
            continue;

        ObjectRecord &object = object_at(entry->object_offset);
        const std::uint16_t object_flags = object.u16(0x12);
        if ((object_flags & 0x8000U) != 0)
            continue;

        const std::uint16_t object_y = object.u16(0x08);
        const std::uint16_t object_y_high_mode = static_cast<std::uint16_t>(
            (object_y & 0xff00U) | object.u8(0x16));
        address_named_scheduler_callback_0FDC(
            entry->secondary_callback_offset,
            entry->object_offset, scheduler_entry_offset(entry),
            scheduler_context_7560, object.u16(0x04), object_y,
            object_y_high_mode, object_flags);
    }
}

// 01F7:34BC, reached by the fixed relocated call at 01F7:1024 from 0FDC.
// This is a presentation queue append, not an object allocator or gameplay
// callback. Ghidra's Protected Mode listing preserves the exact write order
// and the far return. DS:6D86 is a far pointer; DS:8174 is a 16-bit cursor.
struct VisibleObjectQueueRecord {
    std::uint16_t x;
    std::uint16_t y;
    std::uint16_t flags;
    std::uint8_t reserved;
    std::uint8_t mode;
};

extern VisibleObjectQueueRecord *address_named_visible_queue_6D86();
extern std::uint16_t address_named_visible_queue_cursor_8174;

void append_visible_object_queue_34BC(
    std::uint16_t ax_object_x,
    std::uint16_t bx_object_y,
    std::uint16_t dx_object_flags,
    std::uint8_t cl_object_mode) {
    // 34C4/34C7: SHL AX,3 and ADD DI,AX are 16-bit address arithmetic.
    VisibleObjectQueueRecord *queue = address_named_visible_queue_6D86();
    const std::uint16_t cursor = address_named_visible_queue_cursor_8174;
    VisibleObjectQueueRecord *record = reinterpret_cast<VisibleObjectQueueRecord *>(
        reinterpret_cast<std::uintptr_t>(queue) +
        static_cast<std::uint16_t>(cursor << 3));
    address_named_visible_queue_cursor_8174 =
        static_cast<std::uint16_t>(cursor + 1);

    // 34CE, 34D2, 34D5, 34D9, 34DE: preserve the retail store order.
    record->flags = dx_object_flags;
    record->x = ax_object_x;
    record->y = bx_object_y;
    record->reserved = 0;
    record->mode = cl_object_mode;
}

// 01F7:1DEE. The off-camera cull is a lifetime mutation, not a collision
// response. ES:DI+0x18 is cleared first; then the declaration high byte at
// FS:[ES:DI+0x1A+1] is cleared, making the declaration eligible for a later
// stream pass. No player record, carry global, or callback-global is written.
// Two controlled W4L1 runs force the 9DC7 +0x59 latch clear, observe 9E1C ->
// 9E20 -> 9E3A -> A06F -> 1DEE, and then observe the next 3FF8 callback with
// unchanged player position, velocity, and mode. This closes the ordinary
// next-tick consequence of the lifetime mutation; it does not classify retail
// attached-player detachment, landing, inherited velocity, or crushing.
void deactivate_object_outside_camera_1DEE(ObjectRecord &object) {
    const std::uint16_t declaration_offset = object.u16(0x1a);
    object.u16(0x18, 0);
    address_named_fs_byte(declaration_offset + 1) = 0;
}

// Static fact: 0E96 toggles DS:7966 and dispatches the +0x00 phase callback
// in phase 0/1/2 order; 0FA2 then walks the same bank and dispatches the
// +0x02 secondary callback. Thus a 9DC7 phase callback can publish
// DS:5006/DS:8812/DS:8816 before the player 3FF8 phase callback consumes that
// publication. 1036 writes an object's +0x18 phase callback to +0x00 and its
// +0x1C secondary callback to +0x02, then writes the next-entry +0x00
// terminator. The remaining runtime questions are callback membership and
// contact behavior, not scheduler call order.

// The v29 listing also preserves the contracts that feed these branches.
// 1C6E indexes the descriptor table and tests word bit 0x4000; 1C92 indexes
// the raw MAP and tests word bit 0x1000; 5C27 returns selected descriptor
// occupancy in ZF; 5CC3 returns the descriptor word for callers that need its
// flags; and 5DC3 indexes the raw MAP and returns word bit 0x0800 in ZF for
// platform movement. None of these helpers writes the player record or
// callback globals directly.
