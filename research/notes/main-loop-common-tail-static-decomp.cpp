// Focused Ghidra reconstruction of the common 01D7:504F main-loop tail.
//
// Source executable SHA-256:
//   c9b2e59febd6fa0ea271bedf360459353f55c74444f026964b70988d6de1bca1
// Static source: x86:LE:16:Protected Mode raw NE segment import, no-analysis
// Ghidra export from run_transition_reload_closure.py.
//
// This is an address-annotated contract, not native engine code.  The local
// BP variables are intentionally address-named: their ownership is inside the
// containing 01D7:5010 routine and they are not player-record fields.

#include <cstdint>

using U8 = std::uint8_t;
using U16 = std::uint16_t;

struct MainLoopGlobals {
    U16 transition_event_89ec;       // DS:89EC
    U16 transition_suppressor_89e0; // DS:89E0
    U16 spawn_row_85d2;              // DS:85D2
    U16 transition_selector_85d4;    // DS:85D4
    U16 previous_selector_85d6;     // DS:85D6
    U8 transition_substate_85da;    // DS:85DA
    U8 completion_done_85db;         // DS:85DB
    U16 frame_gate_8196;             // DS:8196
    U16 timer_gate_819e;             // DS:819E
    U16 map_page_a_817a;             // DS:817A
    U16 map_page_b_817c;             // DS:817C
    U8 map_refresh_81d1;              // DS:81D1
    U16 input_command_88ba;           // DS:88BA
    U16 input_aux_88b2;                // DS:88B2
    U16 input_aux_88b4;                // DS:88B4
    U16 input_state_88bc;             // DS:88BC
    U16 transition_signal_89e6;       // DS:89E6
    U16 transition_input_89ee;        // DS:89EE
    U16 transition_counter_89f0;     // DS:89F0
    U16 transition_command_89f4;     // DS:89F4
    U16 transition_callback_8952;     // DS:8952
    U16 current_health_8822;          // DS:8822
    U16 maximum_health_8824;          // DS:8824
    U16 ammo_880c;                    // DS:880C
    U16 lives_880a;                   // DS:880A
    U16 transition_gate_89ea;         // DS:89EA
    U8 ui_state_613f;                 // DS:613F
};

struct MainLoopLocals {
    U8 callback_rebuild_403;  // BP-0x403
    U8 callback_rebuild_407;  // BP-0x407
    U8 selector_refresh_408;   // BP-0x408
    U8 saved_ui_state_409;     // BP-0x409
    U16 transition_wait_406;   // BP-0x406
};

// The calls below are retained as contracts because their targets are either
// presentation/resource routines or a separate closure.  Their return flags
// are not used by the direct state writes listed here unless noted.
extern void address_named_504f_external_call();
extern U8 address_named_finalization_menu_update(); // 01E7:0D18 boundary
extern void address_named_selector_preview();
extern void address_named_timer_wait();
extern void address_named_input_dispatch();
extern void address_named_completion_dispatch();
extern void address_named_recovery_dispatch();
extern void address_named_transition_pending();

// 01D7:504F is an internal continuation of 01D7:5010, not a separately
// callable function.  It has no independent argument or return-register
// contract.  Control either returns from the containing loop at 505? or jumps
// back to 504F after a state transition; the terminal/presentation calls are
// not assigned semantic names here.
void common_main_loop_tail_504f(MainLoopGlobals &g, MainLoopLocals &local) {
    // 504F-505?; this guard is tested before the ordinary-loop reset writes.
    if (g.transition_event_89ec != 0 ||
        g.transition_suppressor_89e0 == 0xffff) {
        address_named_504f_external_call();
        g.ui_state_613f = local.saved_ui_state_409;
        return;
    }

    // 505?; every ordinary pass starts with the authored spawn row cleared.
    g.spawn_row_85d2 = 0;
    if (g.transition_substate_85da == 1) {
        local.callback_rebuild_407 = 0;
        local.selector_refresh_408 = 1;
    }

    // The selector/rebuild calls are ordered before the common scheduler
    // reset.  Their targets remain address-named; only the direct 504F writes
    // are represented here.
    if (local.callback_rebuild_407 != 0) {
        address_named_selector_preview();
        g.ui_state_613f = 0;
        local.transition_wait_406 = 0x0014;
        local.callback_rebuild_407 = 0;
    }
    if (local.selector_refresh_408 != 0) {
        local.selector_refresh_408 = 0;
        address_named_504f_external_call();
    }

    // 01D7:505? -> 48B5.  These writes precede the timer wait and the input
    // command ladder; the carry/zero flags of the external calls are not
    // consumed by this boundary.
    g.frame_gate_8196 = 0;
    g.input_state_88bc = 0;
    address_named_504f_external_call();
    g.transition_signal_89e6 = 0;
    g.transition_input_89ee = 0;
    if (local.callback_rebuild_403 != 0)
        address_named_504f_external_call();
    g.transition_callback_8952 = 0xffff;

    // 01D7:48B5-4968.  The map-page swap, refresh latch, and timer gate are
    // direct state writes.  The IRQ producer and frame-wait implementation
    // remain outside this contract.
    if (local.transition_wait_406 != 0) {
        local.transition_wait_406 = static_cast<U16>(
            local.transition_wait_406 - 1);
        if (local.transition_wait_406 == 0)
            g.ui_state_613f = local.saved_ui_state_409;
    }
    address_named_timer_wait();
    const U16 page = g.map_page_a_817a;
    g.map_page_a_817a = g.map_page_b_817c;
    g.map_page_b_817c = page;
    g.map_refresh_81d1 = 1;
    g.timer_gate_819e = 0;
    address_named_timer_wait();

    // The command ladder is a word comparison.  It can publish gameplay
    // globals before the outer transition state is consumed.
    if (g.input_command_88ba == 0x0019) {
        g.transition_input_89ee = 0xffff;
        return;                         // 4968 join
    }
    if (g.transition_event_89ec == 0xffff ||
        g.transition_signal_89e6 == 0xffff)
        return;                         // 4968 join
    if (g.input_aux_88b2 != 0)
        g.current_health_8822 = g.maximum_health_8824;
    if (g.input_aux_88b4 != 0)
        g.ammo_880c = 99;
    if (g.input_command_88ba == 1) {
        g.transition_counter_89f0 = 0xffff;
        return;                         // 4968 join
    }
    if (g.input_command_88ba == 2)
        g.current_health_8822 = g.maximum_health_8824;
    if (g.input_command_88ba == 3) {
        g.transition_signal_89e6 = 0xffff;
        return;                         // 4968 join
    }
    if (g.input_command_88ba == 4)
        g.ammo_880c = 99;
    if (g.input_command_88ba == 5)
        g.transition_command_89f4 = 0xffff;

    // 4968 is the shared join after the command loop.  The state resets are
    // direct and occur before the optional completion/recovery calls.
    g.transition_event_89ec = 0;
    g.transition_callback_8952 = 0;
    local.callback_rebuild_403 = 1;
    if (g.transition_command_89f4 == 0xffff) {
        g.transition_command_89f4 = 0;
        g.completion_done_85db =
            (g.transition_selector_85d4 >= 0x0010 &&
             g.transition_selector_85d4 <= 0x0014) ? 1 : g.completion_done_85db;
        address_named_completion_dispatch();
    }

    // The 89EE, 89EA, terminal, and pending-transition branches follow this
    // join in the containing 5010 body.  Their direct writes are recorded in
    // death-recovery-static-decomp.cpp; this note stops at the common-tail
    // boundary so the timer/resource calls are not misclassified as player
    // simulation.
}

// Static stopping rule:
// - closed here: branch order, direct global writes, map-page/timer-gate
//   publication, and command values that feed later transition handling;
// - address-named contract: timer wait, IRQ producer, selector/UI/resource
//   calls, and the outer 89EA recovery consumer;
// - unresolved: the exact delayed death-hold counter and scheduler rebuild
//   membership observed between 19E6/199D and 1AAA.
