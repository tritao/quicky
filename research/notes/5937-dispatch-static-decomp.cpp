// Focused Ghidra reconstruction of the 5937 -> 386F -> 0442 dispatch body.
//
// Source executable SHA-256:
//   c9b2e59febd6fa0ea271bedf360459353f55c74444f026964b70988d6de1bca1
// Static source: Ghidra x86:LE:16:Protected Mode, raw NE segment import,
// no whole-program analysis. The instruction slices and this contract are
// generated/audited by run_player_external_closure.py.
//
// 0442, 04DF, and 0517 are not three independent C functions. 0442 creates
// the BP frame and local dispatch area; 04DF and 0517 are address-qualified
// entries into that live frame selected by runtime table data. The common
// return is RETF 0x6 at 059D.

#include <cstdint>

struct DispatchGlobals {
    std::uint16_t table_index_map_6d8e; // DS:6D8E, selector -> record index
    std::uint16_t table_stride_30d2;    // DS:30D2, record stride in bytes
    std::uint16_t view_base_x_81a8;     // DS:81A8
    std::uint16_t view_base_y_81ac;     // DS:81AC
    std::uint16_t camera_x_81c0;        // DS:81C0
    std::uint16_t camera_y_81c4;        // DS:81C4
    std::uint16_t view_state_817c;      // DS:817C, saved/restored by 386F
    std::uint16_t view_mask_6572;       // DS:6572
    std::uint16_t video_segment_5c9a;   // DS:5C9A
};

extern DispatchGlobals DS;

struct DispatchRecord {
    std::uint8_t bytes[0x2c];
    std::uint16_t u16(std::uint16_t offset) const;
};

// The far pointer at record +0x18/+0x1A is selected by runtime resource data.
// It is intentionally not given a gameplay or presentation semantic here.
extern void address_named_record_callback_far();

struct DispatchFrame {
    // Caller words at BP+06, BP+08, BP+0A. RETF 0x6 consumes all three.
    std::uint16_t selector;
    std::uint16_t view_x;
    std::uint16_t view_y;

    // BP-local bytes/words. Their meanings remain address-qualified.
    std::uint8_t local_25;
    std::uint8_t local_26;
    std::uint8_t local_27;
    std::uint16_t selected_record_index;
    std::uint16_t callback_offset;
    std::uint16_t callback_selector;
    std::uint16_t rotated_mask;
};

// Address-named operations preserve the portions whose runtime table/segment
// values are not executable constants in the raw segment image.
extern void address_named_frame_setup_0448();
extern const DispatchRecord &address_named_record_at_6d8a(
    std::uint16_t record_index);
extern std::uint16_t address_named_view_x_adjust_04a2(
    std::uint16_t input, std::uint16_t camera, std::uint16_t record_word,
    std::uint16_t base);
extern std::uint16_t address_named_view_y_adjust_04d3(
    std::uint16_t input, std::uint16_t camera, std::uint16_t record_word,
    std::uint16_t base);
extern std::uint16_t address_named_rol_1111(std::uint16_t count);
extern void address_named_view_dispatch_frame_setup_0551(DispatchFrame &);
extern void address_named_vga_port_write_0571();
extern void address_named_local_rotate_04df(DispatchFrame &, std::uint8_t,
                                             std::uint16_t);
extern std::uint16_t address_named_view_x_adjust_04df(
    std::uint16_t input, std::uint16_t base, std::uint16_t ax,
    std::uint16_t cx);
extern void address_named_dispatch_entry_0517(DispatchFrame &);
extern void address_named_dispatch_record_and_call_0533(DispatchFrame &);

// 01F7:0442. Exact direct contract: selector guard, DS:6D8E lookup, 0x2C
// record selection, view-coordinate preparation, three local dispatch bytes,
// VGA sequencer write, record callback pointer load, and the indirect call.
// No player-record or callback-global simulation field is directly written.
void address_named_dispatch_0442(DispatchFrame &frame) {
    address_named_frame_setup_0448();              // 0442-044D

    if (frame.selector > 0x03e7)                   // 0450-0457
        return;                                    // 059C-059D

    frame.selected_record_index =
        DS.table_index_map_6d8e + frame.selector * 2; // 045A-0463
    if (frame.selected_record_index == 0xffff)     // 0466-046C
        return;

    const DispatchRecord &record =
        address_named_record_at_6d8a(frame.selected_record_index); // 046F-049A
    frame.view_x = address_named_view_x_adjust_04a2(
        frame.view_x, DS.camera_x_81c0, record.u16(0x08),
        DS.view_base_x_81a8);                       // 049A-04C6
    frame.view_y = address_named_view_y_adjust_04d3(
        frame.view_y, DS.camera_y_81c4, record.u16(0x0A),
        DS.view_base_y_81ac);                       // 04C9-0502

    frame.local_25 = 0;                            // 0502-050B
    frame.local_26 = 0;
    frame.local_27 = 0;
    switch (frame.view_y & 3) {                     // 050E-0533
    case 1: frame.local_27 = 1; break;             // 0519-0522
    case 2: frame.local_26 = 1; break;             // 0524-052D
    case 3: frame.local_25 = 1; break;             // 052F
    default: break;
    }

    frame.rotated_mask = address_named_rol_1111(frame.view_y & 3); // 0533-0550
    address_named_view_dispatch_frame_setup_0551(frame);           // 0551-056E
    address_named_vga_port_write_0571();                            // 0571-0575

    frame.callback_offset = record.u16(0x18);        // 0575-0579
    frame.callback_selector = record.u16(0x1A);      // 057C-0580
    if ((frame.callback_offset | frame.callback_selector) == 0)
        return;                                     // 0583-059B

    // 058A-0598: AX receives the rotated mask with AH replaced by local -25;
    // BL/BH receive locals -26/-27; CH is cleared. The callback may still
    // alter simulation state and therefore remains address-named.
    address_named_record_callback_far();            // CALLF [BP-0x12]
}

// 01F7:04DF. Alternate entry into the live 0442 frame. It writes only the
// BP-local rotation/dispatch state, then joins the shared 0517/0533 path.
void address_named_dispatch_entry_04df(DispatchFrame &frame,
                                       std::uint8_t rotate_count,
                                       std::uint16_t ax,
                                       std::uint16_t cx,
                                       std::uint16_t di) {
    address_named_local_rotate_04df(frame, rotate_count, di); // 04DF-04FF
    frame.view_x = address_named_view_x_adjust_04df(
        frame.view_x, DS.view_base_y_81ac, ax, cx);            // 04E3-0502
    frame.local_25 = 0;
    frame.local_26 = 0;
    frame.local_27 = 0;
    address_named_dispatch_entry_0517(frame);                 // 0517 onward
}

// 01F7:0517. Shared indirect-entry join. It assumes the 0442 BP frame and
// has no independent prologue. Direct writes remain BP-local. The 0598
// instruction is a CALLF through the selected record's +0x18/+0x1A words,
// not a separate function body.
void address_named_dispatch_entry_0517(DispatchFrame &frame) {
    switch (frame.view_y & 3) {                               // 0517-0533
    case 1: frame.local_27 = 1; break;
    case 2: frame.local_26 = 1; break;
    case 3: frame.local_25 = 1; break;
    default: break;
    }
    address_named_dispatch_record_and_call_0533(frame);
}

// Static conclusion: 0442/04DF/0517 read runtime view/table state, write BP
// locals, and perform one VGA sequencer port 0x3C4 write. They do not directly write the
// player record or DS:89EA/89E6/8812/8816. 04DF and 0517 are shared linear
// body offsets, not independently proven runtime callback targets.
//
// The earlier selector-6/selector-5 observations indexed DS:6D8E from EBX.
// That was the middle 0442 argument, not the selector consumed at [BP+06].
// The corrected stack-argument trace reads [BP+06]=0x01FF and resolves the
// ordinary W1L1 record to a loaded far callback at 14A7:1258.  Its entry and
// far return were traced with the original player ES:DI and original data
// selector snapshotted across the call: the target object, player record, and
// recovered simulation globals were unchanged.  This closes that observed
// W1L1 target as a non-simulation callback, while records selected by other
// runtime inputs remain address-named at the 0598 call site.
