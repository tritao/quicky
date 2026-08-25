-- Normal-lifecycle probe for the secondary MAP loader.
-- Unlike the descriptor-construction probe this does not stop at the primary
-- loader or world initializer; it lets the selected level finish transitioning
-- before watching the gameplay/update call sites.

local timeout_ms = TRACE_TIMEOUT_MS or 30000
local select_level = TRACE_SELECT_LEVEL or "W1L3"
local selector_frames = TRACE_SELECTOR_FRAMES or 80
local diagnostic = TRACE_SECONDARY_DIAGNOSTIC or false
local force_gate = TRACE_SECONDARY_FORCE_GATE or false
local diagnostic_steps = TRACE_SECONDARY_DIAGNOSTIC_STEPS or 48
local drive_wait = TRACE_SECONDARY_DRIVE_WAIT or false
local skip_wait_loop = TRACE_SECONDARY_SKIP_WAIT or false
local no_wait_break = TRACE_SECONDARY_NO_WAIT_BREAK or false
local post_input_key = TRACE_SECONDARY_POST_INPUT_KEY or ""
local post_input_hold_events = TRACE_SECONDARY_POST_INPUT_HOLD_EVENTS or 8
local gameplay_key = TRACE_SECONDARY_GAMEPLAY_KEY or ""
local gameplay_hold_events = TRACE_SECONDARY_GAMEPLAY_HOLD_EVENTS or 40
local gameplay_at_launch = TRACE_SECONDARY_GAMEPLAY_AT_LAUNCH or false
local force_event = TRACE_SECONDARY_FORCE_EVENT or false
local force_player_fall = TRACE_SECONDARY_FORCE_PLAYER_FALL or false
local pending_focus = TRACE_SECONDARY_PENDING_FOCUS or false
local writer_focus = TRACE_SECONDARY_WRITER_FOCUS or false
local event_before_secondary = TRACE_SECONDARY_EVENT_BEFORE_SECONDARY or false
local timer_audit = TRACE_SECONDARY_TIMER_AUDIT or false
local timer_state_trace = TRACE_SECONDARY_TIMER_STATE_TRACE or false
local timer_post_wait_audit = TRACE_SECONDARY_TIMER_POST_WAIT_AUDIT or false
local pending_loader_audit = TRACE_SECONDARY_PENDING_LOADER_AUDIT or false

local selector_indices = {
    W1L1 = 0, W1L2 = 1, W1L3 = 2,
    W2L1 = 3, W2L2 = 4, W2L3 = 5,
    W3L1 = 6, W3L2 = 7, W3L3 = 8,
    W4L1 = 9, W4L2 = 10, W4L3 = 11,
    W5L1 = 12, W5L2 = 13, W5L3 = 14,
    W1L4 = 15, W2L4 = 16, W3L4 = 17,
    W4L4 = 18, W5L4 = 19,
}

local function word(s, index)
    local lo, hi = string.byte(s, index, index + 1)
    return lo | (hi << 8)
end

local function wait_hit(label)
    local hit, err = dosbox.wait_for_breakpoint(timeout_ms)
    if not hit then error(label .. ": " .. (err or "breakpoint wait failed")) end
    return hit
end

local function ds_word(offset)
    return dosbox.mem_read_word("ds", offset)
end

local function map_state()
    return {
        base = ds_word(0x657a), selector = ds_word(0x657c),
        row_stride = ds_word(0x657e), height = ds_word(0x6580),
    }
end

local function map_word(map, offset)
    return word(dosbox.mem_read_selector(map.selector, map.base + offset, 2), 1)
end

local function map_effect_target(registers)
    local map = map_state()
    local ax = (registers and registers.eax or 0) & 0xffff
    local bx = (registers and registers.ebx or 0) & 0xffff
    -- 16CE's register convention is AX=column coordinate and BX=row
    -- coordinate (the opposite order from the generic helper's names).
    local cell_offset = map.base + (bx >> 4) * map.row_stride + (ax >> 4) * 2
    return {
        map = map,
        ax = ax,
        bx = bx,
        cx = (registers and registers.ecx or 0) & 0xffff,
        dx = (registers and registers.edx or 0) & 0xffff,
        cell_offset = cell_offset,
        before_word = map_word(map, cell_offset - map.base),
    }
end

local function hex(raw)
    if not raw then return nil end
    local bytes = {}
    for index = 1, #raw do
        bytes[#bytes + 1] = string.format("%02x", string.byte(raw, index))
    end
    return table.concat(bytes)
end

local function caller_snapshot(registers)
    local esp = (registers and registers.esp or 0) & 0xffff
    local stack = dosbox.mem_read("ss", esp, 8) or ""
    local result = {esp = esp, stack_hex = hex(stack)}
    if #stack >= 4 then
        result.return_address = {offset = word(stack, 1), segment = word(stack, 3)}
    end
    return result
end

assert(selector_indices[select_level] ~= nil, "unsupported level selector target")
dosbox.output.awaiting_startup_replay = true
dosbox.wait_frames(350)
dosbox.key("KBD_space", true)
dosbox.wait_frames(4)
dosbox.key("KBD_space", false)
dosbox.wait_frames(30)
dosbox.type("QUIKYSUPERHERO")
dosbox.wait_frames(3)
dosbox.breakpoint_set(0x01d7, 0x491d, {once = true})
dosbox.key("KBD_4", true)
local cheat = wait_hit("level selector branch")
dosbox.key("KBD_4", false)
dosbox.mem_write("ds", 0x89f2, "\x01")
dosbox.mem_write("ds", 0x88ba, "\x05\x00")
dosbox.debug_continue()
dosbox.wait_frames(selector_frames)
dosbox.breakpoint_set(0x01d7, 0x4ace, {once = true})
local input_wait = wait_hit("selector input wait")
dosbox.mem_write("ds", 0x85d4,
                 string.char(selector_indices[select_level] & 0xff,
                             selector_indices[select_level] >> 8))
dosbox.breakpoint_set(0x01d7, 0x4b18, {once = true})
dosbox.mem_write("ds", 0x88bc, "\x20\x00")

local call_sites = {0x4bf1, 0x4bfb, 0x4c05, 0x4c0f, 0x4c19}
dosbox.debug_continue()
local launch = wait_hit("selector Space dispatch")
-- Leave the transition state untouched: this probe is specifically the
-- unmodified lifecycle check. The focused descriptor probe contains the
-- separate, explicitly controlled trampoline call.
local controlled_patch = nil
local input_release_probe = false
local input_release_hit = nil
if event_before_secondary and post_input_key ~= "" then
    -- Controlled-input mode for the otherwise uninstrumented secondary wait:
    -- press the requested real key at the level-dispatch stop and release it
    -- at the actual DS:89E6 event-writer instruction. The loader call/entry
    -- breakpoints remain the only lifecycle observations after that release.
    dosbox.key(post_input_key, true)
    dosbox.breakpoint_set(0x01d7, 0x493e, {once = true})
    input_release_probe = true
end
local timer_audit_hits = {}
local timer_audit_pre_hits = {}
local timer_audit_post_hits = {}
local timer_audit_after_event = false
if timer_audit then
    dosbox.breakpoint_set(0x01f7, 0xf049, {once = true})
end
if skip_wait_loop then
    controlled_patch = {
        wait_loop = {segment = 0x01d7, offset = 0x48bb},
        original_wait_loop_bytes = (dosbox.mem_read_selector(0x01d7, 0x48bb, 7) or "")
            :gsub(".", function(c) return string.format("%02x", string.byte(c)) end),
    }
    -- Debugger-only: jump over CMP/JZ at 48BB to test whether the wait loop
    -- itself is the only barrier to the later gate checks.
    dosbox.mem_write_selector(0x01d7, 0x48bb, "\xeb\x05\x90\x90\x90\x90\x90")
    controlled_patch.patched_wait_loop_bytes = (dosbox.mem_read_selector(0x01d7, 0x48bb, 7) or "")
        :gsub(".", function(c) return string.format("%02x", string.byte(c)) end)
end
if force_gate then
    controlled_patch = {
        clear_instruction = {segment = 0x01f7, offset = 0x1ae6},
        original_clear_bytes = (dosbox.mem_read_selector(0x01f7, 0x1ae6, 6) or "")
            :gsub(".", function(c) return string.format("%02x", string.byte(c)) end),
        scheduler_before = ds_word(0x89ea),
        object_count_before = ds_word(0x880a),
    }
    -- 01f7:1ae6 clears DS:89EA after the state-machine transition. Keep the
    -- gate set for this probe only so the real 01d7:4bf1 caller can run.
    dosbox.mem_write_selector(0x01f7, 0x1ae6, "\x90\x90\x90\x90\x90\x90")
    controlled_patch.wait_clear_bytes = (dosbox.mem_read_selector(0x01d7, 0x48b5, 6) or "")
        :gsub(".", function(c) return string.format("%02x", string.byte(c)) end)
    dosbox.mem_write_selector(0x01d7, 0x48b5, "\x90\x90\x90\x90\x90\x90")
    dosbox.mem_write("ds", 0x89ea, "\x01\x00")
    dosbox.mem_write("ds", 0x819e, "\x01\x00")
    controlled_patch.scheduler_after = ds_word(0x89ea)
    controlled_patch.patched_clear_bytes = (dosbox.mem_read_selector(0x01f7, 0x1ae6, 6) or "")
        :gsub(".", function(c) return string.format("%02x", string.byte(c)) end)
end
if diagnostic then
    local watched = {
        {segment = 0x01d7, offset = 0x48b5, name = "wait_gate_clear"},
        {segment = 0x01d7, offset = 0x48bb, name = "wait_gate_test"},
        {segment = 0x01d7, offset = 0x48c2, name = "post_wait_entry"},
        {segment = 0x01d7, offset = 0x48cc, name = "post_wait_state_check"},
        {segment = 0x01d7, offset = 0x48dc, name = "post_wait_branch"},
        {segment = 0x01d7, offset = 0x48e6, name = "post_wait_state_gate"},
        {segment = 0x01d7, offset = 0x4968, name = "transition_dispatch"},
        {segment = 0x01d7, offset = 0x504f, name = "main_loop_dispatch"},
        {segment = 0x01d7, offset = 0x5080, name = "transition_return"},
        {segment = 0x01d7, offset = 0x4ba4, name = "scheduler_gate_test"},
        {segment = 0x01d7, offset = 0x4bae, name = "object_count_test"},
        {segment = 0x01d7, offset = 0x4bd8, name = "secondary_gate_test"},
        {segment = 0x01d7, offset = 0x4c43, name = "zero_gate_branch"},
        {segment = 0x01d7, offset = 0x4cfc, name = "main_loop_state_branch"},
        {segment = 0x01d7, offset = 0x4ea0, name = "pending_state_branch"},
        {segment = 0x01d7, offset = 0x4eaa, name = "pending_state_dispatch"},
        {segment = 0x01d7, offset = 0x5122, name = "write_880a_init", global = 0x880a, value = 4},
        {segment = 0x01f7, offset = 0x19e6, name = "state_update_entry"},
        {segment = 0x01f7, offset = 0x1bc5, name = "state_update_call_a"},
        {segment = 0x01f7, offset = 0x3aaf, name = "state_update_call_b"},
        {segment = 0x01f7, offset = 0x19a3, name = "write_89ea_start", global = 0x89ea, value = 0xffff},
        {segment = 0x01f7, offset = 0x1a3d, name = "write_89ea_state", global = 0x89ea, value = 0xffff},
        {segment = 0x01f7, offset = 0x1ae6, name = "write_89ea_clear", global = 0x89ea, value = 0},
        {segment = 0x01f7, offset = 0x199d, name = "write_89ea_callback", global = 0x89ea, value = 0xffff},
        {segment = 0x01f7, offset = 0x43d1, name = "callback_gate_call"},
        {segment = 0x01f7, offset = 0x44dc, name = "decrement_89ea", global = 0x89ea},
        {segment = 0x01f7, offset = 0x339a, name = "map_low_id_writer"},
        {segment = 0x01f7, offset = 0x340a, name = "map_property_writer"},
        {segment = 0x01f7, offset = 0x5c9d, name = "map_cell_store_helper"},
        {segment = 0x01f7, offset = 0x3ff8, name = "player_callback"},
        {segment = 0x01f7, offset = 0x43d0, name = "player_boundary_check"},
        {segment = 0x01d7, offset = 0x493e, name = "write_89e6_main", global = 0x89e6, value = 0xffff},
        {segment = 0x01f7, offset = 0x4996, name = "write_89e6_state_a", global = 0x89e6, value = 0xffff},
        {segment = 0x01f7, offset = 0x4aac, name = "write_89e6_state_b", global = 0x89e6, value = 0xffff},
        {segment = 0x01f7, offset = 0x92a9, name = "write_89e6_collision", global = 0x89e6, value = 0xffff},
        {segment = 0x01d7, offset = 0x3861, name = "secondary_loader_entry"},
    }
    if pending_focus then
        -- Keep the focused run below the debugger's simultaneous-breakpoint
        -- budget. The ordinary diagnostic list is useful for broad lifecycle
        -- scans, but the pending probe needs room for the state-word xrefs.
        for index = #watched, 1, -1 do watched[index] = nil end
        -- The post-event loop is a small state barrier rather than a single
        -- call site. Keep these breakpoints repeatable so one run shows the
        -- state words being produced, consumed, and cleared across passes.
        local pending_watched = {
            {segment = 0x01d7, offset = 0x48c2, name = "post_wait_entry"},
            {segment = 0x01d7, offset = 0x48cc, name = "post_wait_state_check"},
            {segment = 0x01d7, offset = 0x48dc, name = "post_wait_branch"},
            {segment = 0x01d7, offset = 0x48e6, name = "post_wait_state_gate"},
            {segment = 0x01d7, offset = 0x493e, name = "write_89e6_main", global = 0x89e6, value = 0xffff},
            {segment = 0x01d7, offset = 0x4968, name = "transition_dispatch"},
            {segment = 0x01d7, offset = 0x4ba4, name = "scheduler_gate_test"},
            {segment = 0x01d7, offset = 0x4bd8, name = "secondary_gate_test"},
            {segment = 0x01d7, offset = 0x3861, name = "secondary_loader_entry"},
            {segment = 0x01d7, offset = 0x4b82, name = "write_89e0_main_clear"},
            {segment = 0x01d7, offset = 0x4e10, name = "write_89e0_event7"},
            {segment = 0x01d7, offset = 0x4e8e, name = "write_89e0_event34"},
            {segment = 0x01d7, offset = 0x4ecc, name = "write_89e0_event14"},
            {segment = 0x01d7, offset = 0x4fb2, name = "write_85d6_from_85d4"},
            {segment = 0x01d7, offset = 0x4e94, name = "increment_85da"},
            {segment = 0x01d7, offset = 0x50d0, name = "write_85da_start"},
            {segment = 0x01d7, offset = 0x5184, name = "write_85da_clear"},
            {segment = 0x01d7, offset = 0x4b4f, name = "write_89ec_transition"},
            {segment = 0x01d7, offset = 0x4c3a, name = "write_89ec_zero_gate"},
            {segment = 0x01d7, offset = 0x4ca8, name = "write_89ec_state_a"},
            {segment = 0x01d7, offset = 0x4cf3, name = "write_89ec_state_b"},
            {segment = 0x01d7, offset = 0x5023, name = "write_89ec_reload"},
            {segment = 0x01d7, offset = 0x50ec, name = "clear_89ec_reload"},
            {segment = 0x01d7, offset = 0x5181, name = "clear_89ec_transition"},
            {segment = 0x01d7, offset = 0x520c, name = "clear_89ec_state"},
            {segment = 0x01d7, offset = 0x4b87, name = "clear_89e6_main"},
            {segment = 0x01d7, offset = 0x4b8a, name = "main_loop_after_init"},
            {segment = 0x01d7, offset = 0x5060, name = "main_loop_event_dispatch"},
            {segment = 0x01d7, offset = 0x4c43, name = "zero_gate_branch"},
            {segment = 0x01d7, offset = 0x4cfc, name = "main_loop_state_branch"},
            {segment = 0x01d7, offset = 0x4ea0, name = "pending_state_branch"},
            {segment = 0x01d7, offset = 0x4eaa, name = "pending_state_dispatch"},
            {segment = 0x01d7, offset = 0x504f, name = "main_loop_dispatch"},
            {segment = 0x01d7, offset = 0x4ebf, name = "pending_tail_after_call", tail = true, stage = 1},
            {segment = 0x01d7, offset = 0x4ec5, name = "pending_tail_state_test", tail = true, stage = 1},
            {segment = 0x01d7, offset = 0x4ecc, name = "pending_tail_set_e0", tail = true, stage = 1},
            {segment = 0x01d7, offset = 0x4ed2, name = "pending_tail_event_branch", tail = true, stage = 1},
            {segment = 0x01d7, offset = 0x4ed7, name = "pending_deep_flag_branch", tail = true, stage = 2},
            {segment = 0x01d7, offset = 0x4ed9, name = "pending_deep_call_a", tail = true, stage = 2},
            {segment = 0x01d7, offset = 0x4ee4, name = "pending_deep_call_b", tail = true, stage = 2},
            {segment = 0x01d7, offset = 0x4eeb, name = "pending_deep_call_c", tail = true, stage = 2},
            {segment = 0x01d7, offset = 0x4ef0, name = "pending_deep_call_d", tail = true, stage = 2},
            {segment = 0x01d7, offset = 0x4f0d, name = "pending_deep_call_e", tail = true, stage = 2},
            {segment = 0x01d7, offset = 0x4f10, name = "pending_deep_state_test", tail = true, stage = 2},
            {segment = 0x01d7, offset = 0x4f1a, name = "pending_deep_level_dispatch", tail = true, stage = 2},
            {segment = 0x01d7, offset = 0x4fad, name = "pending_deep_to_5010", tail = true, stage = 2},
            {segment = 0x01d7, offset = 0x5010, name = "pending_deep_error_gate", tail = true, stage = 2},
            {segment = 0x01d7, offset = 0x504c, name = "pending_deep_clear_e6", tail = true, stage = 2},
            {segment = 0x01d7, offset = 0x505d, name = "pending_deep_fallthrough", tail = true, stage = 2},
            {segment = 0x01d7, offset = 0x14e1, name = "pending_stage3_intro_entry", tail = true, stage = 3},
            {segment = 0x01d7, offset = 0x14f0, name = "pending_stage3_intro_call", tail = true, stage = 3},
            {segment = 0x01d7, offset = 0x150c, name = "pending_stage3_intro_draw", tail = true, stage = 3},
            {segment = 0x01d7, offset = 0x151f, name = "pending_stage3_intro_wait", tail = true, stage = 3},
            {segment = 0x01d7, offset = 0x1524, name = "pending_stage3_intro_call2", tail = true, stage = 3},
            {segment = 0x01d7, offset = 0x1529, name = "pending_stage3_intro_call3", tail = true, stage = 3},
            {segment = 0x01d7, offset = 0x153d, name = "pending_stage3_intro_return_path", tail = true, stage = 3},
            {segment = 0x01d7, offset = 0x1547, name = "pending_stage3_intro_map", tail = true, stage = 3},
            {segment = 0x01d7, offset = 0x154c, name = "pending_stage4_intro_copy", tail = true, stage = 4},
            {segment = 0x01d7, offset = 0x155a, name = "pending_stage4_intro_text_a", tail = true, stage = 4},
            {segment = 0x01d7, offset = 0x1566, name = "pending_stage4_intro_text_b", tail = true, stage = 4},
            {segment = 0x01d7, offset = 0x1587, name = "pending_stage4_intro_draw_a", tail = true, stage = 4},
            {segment = 0x01d7, offset = 0x158e, name = "pending_stage4_intro_wait_a", tail = true, stage = 4},
            {segment = 0x01d7, offset = 0x1593, name = "pending_stage4_intro_draw_b", tail = true, stage = 4},
            {segment = 0x01d7, offset = 0x15ae, name = "pending_stage4_intro_draw_c", tail = true, stage = 4},
            {segment = 0x01d7, offset = 0x15b5, name = "pending_stage4_intro_wait_b", tail = true, stage = 4},
            {segment = 0x01d7, offset = 0x15c5, name = "pending_stage4_intro_timer", tail = true, stage = 4},
            {segment = 0x01d7, offset = 0x1609, name = "pending_stage4_intro_timer_loop", tail = true, stage = 4},
            {segment = 0x0207, offset = 0x0002, name = "pending_stage4_timer_entry", tail = true, stage = 4},
            {segment = 0x0207, offset = 0x0017, name = "pending_stage4_timer_test", tail = true, stage = 4},
        }
        local pending_minimal = {
            post_wait_entry = true, post_wait_state_check = true,
            post_wait_branch = true, post_wait_state_gate = true,
            write_89e6_main = true, transition_dispatch = true,
            scheduler_gate_test = true, secondary_gate_test = true,
            write_85d6_from_85d4 = true, increment_85da = true,
            write_89ec_transition = true, write_89ec_zero_gate = true,
            zero_gate_branch = true,
            main_loop_state_branch = true, pending_state_branch = true,
            pending_state_dispatch = true, main_loop_dispatch = true,
            pending_tail_error_gate = true, pending_tail_clear_e6 = true,
            pending_tail_fallthrough = true,
            pending_tail_after_call = true, pending_tail_state_test = true,
            pending_tail_set_e0 = true, pending_tail_event_branch = true,
            pending_tail_call = true, pending_tail_flag_test = true,
            pending_tail_level_dispatch = true, pending_tail_to_5010 = true,
            pending_deep_flag_branch = true, pending_deep_call_a = true,
            pending_deep_call_b = true, pending_deep_call_c = true,
            pending_deep_call_d = true, pending_deep_call_e = true,
            pending_deep_state_test = true, pending_deep_level_dispatch = true,
            pending_deep_to_5010 = true, pending_deep_error_gate = true,
            pending_deep_clear_e6 = true, pending_deep_fallthrough = true,
            pending_stage3_intro_entry = true, pending_stage3_intro_call = true,
            pending_stage3_intro_draw = true, pending_stage3_intro_wait = true,
            pending_stage3_intro_call2 = true, pending_stage3_intro_call3 = true,
            pending_stage3_intro_return_path = true, pending_stage3_intro_map = true,
            pending_stage4_intro_copy = true, pending_stage4_intro_text_a = true,
            pending_stage4_intro_text_b = true, pending_stage4_intro_draw_a = true,
            pending_stage4_intro_wait_a = true, pending_stage4_intro_draw_b = true,
            pending_stage4_intro_draw_c = true, pending_stage4_intro_wait_b = true,
            pending_stage4_intro_timer = true, pending_stage4_intro_timer_loop = true,
            pending_stage4_timer_entry = true, pending_stage4_timer_test = true,
        }
        for index = #pending_watched, 1, -1 do
            if not pending_minimal[pending_watched[index].name] then
                table.remove(pending_watched, index)
            end
        end
        for _, item in ipairs(pending_watched) do watched[#watched + 1] = item end
    end
    if pending_loader_audit then
        -- Controlled diagnostic: once the real pending intro reaches its
        -- timed-wait test, bypass only that wait and continue tracing the
        -- original transition code toward the secondary loader.  This is
        -- explicitly not normal-lifecycle evidence; it isolates whether the
        -- loader gate is reachable after the presentation/timer barrier.
        for index = #watched, 1, -1 do watched[index] = nil end
        watched = {
            {segment = 0x01d7, offset = 0x4eaa, name = "pending_state_dispatch"},
            {segment = 0x01d7, offset = 0x4edd, name = "pending_wait_call_a"},
            {segment = 0x01d7, offset = 0x4ee6, name = "pending_wait_call_b"},
            {segment = 0x01d7, offset = 0x4f0d, name = "pending_intro_entry"},
            {segment = 0x01d7, offset = 0x4f1a, name = "pending_level_dispatch"},
            {segment = 0x01d7, offset = 0x4fad, name = "pending_to_main_event"},
            {segment = 0x01d7, offset = 0x5010, name = "transition_main_loop_event"},
            {segment = 0x01d7, offset = 0x504f, name = "main_loop_dispatch"},
            {segment = 0x01d7, offset = 0x4ba4, name = "scheduler_gate_test"},
            {segment = 0x01d7, offset = 0x4bae, name = "object_count_test"},
            {segment = 0x01d7, offset = 0x4bd8, name = "secondary_gate_test"},
            {segment = 0x01d7, offset = 0x3861, name = "secondary_loader_entry"},
            {segment = 0x01d7, offset = 0x394f, name = "secondary_loader_or"},
            {segment = 0x01d7, offset = 0x396d, name = "secondary_loader_normalizer"},
            {segment = 0x0207, offset = 0x0002, name = "timer_wait_entry"},
            {segment = 0x0207, offset = 0x0017, name = "timer_wait_test_flag"},
            {segment = 0x01d7, offset = 0x1609, name = "pending_intro_timer_loop"},
        }
    end
    if writer_focus then
        -- Let the selected level run without callback/transition breakpoints;
        -- only the three coordinate/full-word MAP writers are armed. This is
        -- a bounded normal-gameplay attribution window, not a proof that an
        -- unobserved writer can never execute later.
        for index = #watched, 1, -1 do watched[index] = nil end
        watched = {
            {segment = 0x01f7, offset = 0x16ce, name = "map_effect_tile_rewrite"},
            {segment = 0x01f7, offset = 0x339a, name = "map_low_id_writer"},
            {segment = 0x01f7, offset = 0x340a, name = "map_property_writer"},
            {segment = 0x01f7, offset = 0x5c9d, name = "map_cell_store_helper"},
        }
    end
    if timer_state_trace then
        -- The debugger has no data-watchpoint API.  Pair the known flag
        -- write/test instruction addresses with repeatable execution stops,
        -- and sample DS:819E at each stop.  The before/after pairs expose
        -- whether the timer IRQ write is visible to the wait loop without
        -- mutating either gate.
        for index = #watched, 1, -1 do watched[index] = nil end
        watched = {
            {segment = 0x01d7, offset = 0x48b5, name = "transition_wait_clear"},
            {segment = 0x01d7, offset = 0x48bb, name = "transition_wait_test"},
            {segment = 0x01d7, offset = 0x4edd, name = "pending_wait_call_a"},
            {segment = 0x01d7, offset = 0x4ee6, name = "pending_wait_call_b"},
            {segment = 0x01d7, offset = 0x4f0d, name = "pending_intro_entry"},
            {segment = 0x01d7, offset = 0x5010, name = "transition_finalize"},
            {segment = 0x01d7, offset = 0x504f, name = "main_loop_dispatch"},
            {segment = 0x01d7, offset = 0x4bd8, name = "secondary_gate_test"},
            {segment = 0x01d7, offset = 0x3861, name = "secondary_loader_entry"},
            {segment = 0x0207, offset = 0x0002, name = "timer_wait_entry"},
            {segment = 0x0207, offset = 0x0014, name = "timer_wait_clear_flag"},
            {segment = 0x0207, offset = 0x0017, name = "timer_wait_test_flag"},
            {segment = 0x0207, offset = 0x001e, name = "timer_wait_yield"},
            {segment = 0x0207, offset = 0x101f, name = "timer_routine_entry"},
            {segment = 0x0207, offset = 0x10a3, name = "timer_routine_recursive_call"},
            {segment = 0x0207, offset = 0x10a9, name = "pit_helper_entry"},
            {segment = 0x01f7, offset = 0xf049, name = "timer_irq_before_flag_write"},
            {segment = 0x01f7, offset = 0xf04f, name = "timer_irq_after_flag_write"},
        }
    end
    local events = {}
    local seen = {}
    local timer_post_wait_events = {}
    local timer_post_wait_timeout = nil
    local tail_stage = not pending_focus and 2 or 0
    local diagnostic_wait_ms = writer_focus and timeout_ms or
        (timer_state_trace and timeout_ms or (pending_focus and 15000 or 5000))
    local post_input_active = false
    local post_input_events = 0
    local gameplay_active = false
    local gameplay_events = 0
    local force_player_fall_done = false
    if gameplay_at_launch and gameplay_key ~= "" then
        dosbox.key(gameplay_key, true)
        gameplay_active = true
    end
    if post_input_key ~= "" then
        -- The selector launch breakpoint leaves the CPU stopped. Queue a real
        -- make event before resuming so the normal keyboard poll can consume
        -- it. Keep it held across a few diagnostic barriers; releasing it at
        -- the first barrier can race the input poll in the state update.
        dosbox.key(post_input_key, true)
        post_input_active = true
    end
    local function arm_diagnostic()
        for _, item in ipairs(watched) do
            if not seen[item.name] and (not item.stage or item.stage <= tail_stage) and
                    not ((skip_wait_loop or no_wait_break) and
                         (item.name == "wait_gate_clear" or item.name == "wait_gate_test")) and
                    not (force_gate and (item.name == "write_89ea_clear" or
                                   item.name == "wait_gate_clear" or
                                   item.name == "wait_gate_test")) then
                    dosbox.breakpoint_set(item.segment, item.offset, {once = true})
            end
        end
    end
    for sequence = 1, diagnostic_steps do
        arm_diagnostic()
        dosbox.debug_continue()
        local hit, err = dosbox.wait_for_breakpoint(diagnostic_wait_ms)
        if not hit then
            events[#events + 1] = {
                timeout = err or "timeout",
                cpu = dosbox.cpu_state(),
                globals = {
                    level_index = ds_word(0x85d4),
                    wait_gate = ds_word(0x819e),
                    scheduler_gate = ds_word(0x89ea),
                    object_count = ds_word(0x880a),
                    transition_event = ds_word(0x89e6),
                    timer_callback_segment = ds_word(0x8952),
                    timer_callback_offset = ds_word(0x8954),
                    transition_error = ds_word(0x89ec),
                    pending_state = ds_word(0x89e0),
                },
            }
            break
        end
        local name = "unknown"
        for _, item in ipairs(watched) do
            if item.segment == hit.segment and item.offset == hit.offset then
                name = item.name
                break
            end
        end
        events[#events + 1] = {
            sequence = sequence,
            name = name,
            breakpoint = {segment = hit.segment, offset = hit.offset},
            registers = hit.registers,
            caller = caller_snapshot(hit.registers),
            globals = {
                level_index = ds_word(0x85d4),
                wait_gate = ds_word(0x819e),
                scheduler_gate = ds_word(0x89ea),
                object_count = ds_word(0x880a),
                transition_state = ds_word(0x8810),
                last_key = ds_word(0x88ba),
                transition_event = ds_word(0x89e6),
                timer_callback_segment = ds_word(0x8952),
                timer_callback_offset = ds_word(0x8954),
                transition_error = ds_word(0x89ec),
                pending_state = ds_word(0x89e0),
                map_state = ds_word(0x85da),
                map_previous_state = ds_word(0x85d6),
            },
        }
        if timer_state_trace == false and name == "map_effect_tile_rewrite" then
            -- 16CE is the only identified normal-gameplay MAP writer with
            -- static direct callers.  Capture the exact selector/offset and
            -- the cell word before and after its write, using the real far
            -- return address as a temporary, non-mutating completion barrier.
            local effect = map_effect_target(hit.registers)
            local return_address = events[#events].caller.return_address
            if return_address then
                dosbox.breakpoint_set(return_address.segment, return_address.offset,
                                       {once = true})
                dosbox.debug_continue()
                local ok, returned = pcall(wait_hit, "16CE return")
                if ok and returned then
                    effect.after_word = map_word(effect.map,
                                                 effect.cell_offset - effect.map.base)
                    effect.return_breakpoint = {
                        segment = returned.segment, offset = returned.offset,
                    }
                else
                    effect.return_error = tostring(returned)
                end
            else
                effect.return_error = "missing far return address"
            end
            events[#events].map_effect = effect
        end
        post_input_events = post_input_events + 1
        if post_input_active and
                (post_input_events >= post_input_hold_events or
                 (hit.segment == 0x01d7 and hit.offset == 0x493e)) then
            dosbox.key(post_input_key, false)
            post_input_active = false
            events[#events].action = "release_" .. post_input_key
            if gameplay_key ~= "" and not gameplay_active then
                -- Continue with a genuine player action after the event key;
                -- this tests whether normal state updates restore DS:89EA.
                dosbox.key(gameplay_key, true)
                gameplay_active = true
                events[#events].action = events[#events].action ..
                    ";start_" .. gameplay_key
            end
        end
        if gameplay_active then
            gameplay_events = gameplay_events + 1
            if gameplay_hold_events > 0 and gameplay_events >= gameplay_hold_events then
                dosbox.key(gameplay_key, false)
                gameplay_active = false
                events[#events].action = (events[#events].action or "") ..
                    ";release_" .. gameplay_key
            end
        end
        if force_event and hit.segment == 0x01d7 and hit.offset == 0x48e6 then
            -- Controlled downstream check: 48E6 is the event gate's compare
            -- instruction. Set the event while stopped, then execute the
            -- unchanged branch into 4968. This is intentionally not gameplay
            -- evidence; it only validates the consumer path.
            dosbox.mem_write("ds", 0x89e6, "\xff\xff")
            events[#events].action = "force_DS_89E6=0xffff"
        end
        if pending_focus and hit.segment == 0x01d7 and hit.offset == 0x4eaa then
            -- All pre-handler breakpoints have served their purpose. Remove
            -- them before arming the tail so the debugger can expose the
            -- handler's return path without exhausting its breakpoint pool.
            for _, item in ipairs(watched) do
                if not item.tail then
                    dosbox.breakpoint_remove(item.segment, item.offset)
                end
            end
            tail_stage = 1
            events[#events].action = "arm_pending_tail"
        end
        if pending_focus and hit.segment == 0x01d7 and hit.offset == 0x4ed2 then
            for _, item in ipairs(watched) do
                if item.stage == 1 then
                    dosbox.breakpoint_remove(item.segment, item.offset)
                end
            end
            tail_stage = 2
            events[#events].action = "arm_pending_deep_tail"
        end
        if pending_focus and hit.segment == 0x01d7 and hit.offset == 0x4f0d then
            for _, item in ipairs(watched) do
                if item.stage == 2 then
                    dosbox.breakpoint_remove(item.segment, item.offset)
                end
            end
            tail_stage = 3
            events[#events].action = "arm_pending_intro_trace"
        end
        if pending_focus and hit.segment == 0x01d7 and hit.offset == 0x1547 then
            for _, item in ipairs(watched) do
                if item.stage == 3 then
                    dosbox.breakpoint_remove(item.segment, item.offset)
                end
            end
            tail_stage = 4
            events[#events].action = "arm_pending_intro_tail"
        end
        if pending_focus and hit.segment == 0x0207 and hit.offset == 0x0017 then
            -- The intro's timer wait is asynchronous and can outlive the
            -- diagnostic breakpoint window. Release this one wait explicitly
            -- so the trace can observe the post-intro return path; this is a
            -- debugger aid, not evidence of the natural timer trigger.
            dosbox.mem_write("ds", 0x819e, "\x01\x00")
            dosbox.mem_write_selector(0x0207, 0x0014, "\x90\x90\x90")
            dosbox.mem_write_selector(0x0207, 0x0017, "\xeb\x0c")
            events[#events].action = "diagnostic_release_DS_819E"
            events[#events].timer_patch = hex(dosbox.mem_read_selector(0x0207, 0x0014, 7))
        end
        if pending_loader_audit and hit.segment == 0x0207 and hit.offset == 0x0017 then
            -- Keep the patched wait short-circuit in place for all subsequent
            -- pending waits; this is the only guest-code mutation in this
            -- controlled reachability probe.
            dosbox.mem_write("ds", 0x819e, "\x01\x00")
            dosbox.mem_write_selector(0x0207, 0x0014, "\x90\x90\x90")
            dosbox.mem_write_selector(0x0207, 0x0017, "\xeb\x0c")
            events[#events].action = "diagnostic_release_pending_timer_wait"
            events[#events].timer_patch = hex(dosbox.mem_read_selector(0x0207, 0x0014, 7))
        end
        if force_gate and hit.segment == 0x01d7 and hit.offset == 0x4ba4 then
            -- The state callback can decrement DS:89EA again at 44DC before
            -- the main-loop test. Reassert the diagnostic scheduler gate at
            -- the actual consumer compare so 4BD8 can be observed.
            dosbox.mem_write("ds", 0x89ea, "\x01\x00")
            events[#events].action = "force_DS_89EA=1_at_4BA4"
        end
        if force_player_fall and hit.segment == 0x01f7 and hit.offset == 0x3ff8 then
            -- Controlled boundary test: the player callback's 43D0 check
            -- compares object Y against DS:81C4 + DS:81CC. Put the live
            -- player record below that boundary, then let the callback run.
            local registers = hit.registers or {}
            local selector = registers.es or 0
            local object_offset = (registers.edi or 0) & 0xffff
            dosbox.mem_write_selector(selector, object_offset + 8, "\xff\x7f")
            force_player_fall_done = true
            events[#events].action = "force_player_y=0x7fff"
        end
        for _, item in ipairs(watched) do
            if item.segment == hit.segment and item.offset == hit.offset then
                -- Keep one extra player-callback barrier armed after the
                -- initial fall injection; the first callback can still be on
                -- the DS:89EA-nonzero transition branch.
                if not item.rearm and not (force_player_fall and item.name == "player_callback" and
                        not force_player_fall_done) then
                    seen[item.name] = true
                end
                events[#events].watch = {
                    name = item.name, global = item.global, written_value = item.value,
                }
                break
            end
        end
        if drive_wait and hit.segment == 0x01d7 and hit.offset == 0x48bb then
            -- The original DOSBox lifecycle can remain in this asynchronous
            -- wait indefinitely. Advancing this flag is explicitly marked as
            -- a diagnostic aid; it does not claim a normal-gameplay trigger.
            dosbox.mem_write("ds", 0x819e, "\x01\x00")
            events[#events].action = "diagnostic_set_DS_819E=1"
        end
        if timer_post_wait_audit and hit.segment == 0x01d7 and hit.offset == 0x48bb then
            -- Remove every diagnostic stop and test the unmodified barrier in
            -- isolation.  If F049 never fires after 48BB, the level-loop
            -- wait is not merely being hidden by a competing breakpoint.
            for _, item in ipairs(watched) do
                dosbox.breakpoint_remove(item.segment, item.offset)
            end
            for sample = 1, 16 do
                dosbox.breakpoint_set(0x01f7, 0xf049, {once = true})
                dosbox.debug_continue()
                local ok, timer_hit = pcall(wait_hit, "timer IRQ after 48BB")
                if not ok then
                    timer_post_wait_timeout = tostring(timer_hit)
                    break
                end
                timer_post_wait_events[#timer_post_wait_events + 1] = {
                    sample = sample,
                    breakpoint = {segment = timer_hit.segment, offset = timer_hit.offset},
                    registers = timer_hit.registers,
                    wait_gate = ds_word(0x819e),
                    scheduler_gate = ds_word(0x89ea),
                    transition_event = ds_word(0x89e6),
                    timer_flag = ds_word(0x819e),
                    timer_transition_flag = ds_word(0x5014),
                    timer_callback_segment = ds_word(0x8952),
                    timer_callback_offset = ds_word(0x8954),
                }
            end
            dosbox.output.result = {
                schema = "quiky-secondary-timer-post-wait-audit-v1",
                level = select_level,
                timer_post_wait_audit = true,
                barrier_event = events[#events],
                timer_post_wait_events = timer_post_wait_events,
                timer_post_wait_timeout = timer_post_wait_timeout,
                events = events,
            }
            return
        end
        if hit.segment == 0x01d7 and hit.offset == 0x48bb and not force_gate then
            if not drive_wait and not no_wait_break then break end
        end
        if pending_loader_audit and hit.segment == 0x01d7 and hit.offset == 0x3861 then
            dosbox.output.result = {
                schema = "quiky-secondary-pending-loader-audit-v1",
                level = select_level,
                controlled = true,
                wait_patch = "0207:0014 NOP; 0207:0017 JMP 0025 after first observed wait test",
                secondary_loader_reached = true,
                loader_hit = events[#events],
                events = events,
            }
            return
        end
    end
    dosbox.output.result = {
        schema = "quiky-secondary-transition-diagnostic-v1",
        level = select_level,
        diagnostic_steps = diagnostic_steps,
        drive_wait = drive_wait,
        skip_wait_loop = skip_wait_loop,
        no_wait_break = no_wait_break,
        post_input_key = post_input_key,
        post_input_hold_events = post_input_hold_events,
        gameplay_key = gameplay_key,
        gameplay_hold_events = gameplay_hold_events,
        gameplay_at_launch = gameplay_at_launch,
        writer_focus = writer_focus,
        force_event = force_event,
        force_player_fall = force_player_fall,
        launch = launch,
        controlled_patch = controlled_patch,
        timer_audit_pre_event_hits = timer_audit_pre_hits,
        timer_audit_post_event_hits = timer_audit_post_hits,
        timer_state_trace = timer_state_trace,
        timer_post_wait_audit = timer_post_wait_audit,
        events = events,
    }
    return
end

for _, offset in ipairs(call_sites) do
    dosbox.breakpoint_set(0x01d7, offset, {once = true})
end
dosbox.breakpoint_set(0x01d7, 0x3861, {once = true})
dosbox.breakpoint_set(0x01d7, 0x3862, {once = true})
local function record_secondary_timeout(message)
    if not (event_before_secondary or timer_audit) then
        error(message)
    end
    dosbox.output.result = {
        schema = "quiky-secondary-uninstrumented-loader-audit-v1",
        level = select_level,
        timeout = true,
        error = message,
        controlled_input_key = event_before_secondary and post_input_key or nil,
        controlled_input_release = input_release_hit,
        timer_audit_hits = timer_audit_hits,
        timer_audit_pre_event_hits = timer_audit_pre_hits,
        timer_audit_post_event_hits = timer_audit_post_hits,
        launch = launch,
        cpu = dosbox.cpu_state(),
        globals = {
            level_index = ds_word(0x85d4),
            wait_gate = ds_word(0x819e),
            scheduler_gate = ds_word(0x89ea),
            object_count = ds_word(0x880a),
            transition_event = ds_word(0x89e6),
            transition_error = ds_word(0x89ec),
            pending_state = ds_word(0x89e0),
        },
    }
    return nil
end
local function wait_secondary_hit()
    while true do
        dosbox.debug_continue()
        local ok, hit_or_error = pcall(wait_hit, "secondary MAP loader call or entry")
        if not ok then
            return record_secondary_timeout(tostring(hit_or_error))
        end
        local hit = hit_or_error
        if timer_audit and hit.segment == 0x01f7 and hit.offset == 0xf049 then
            timer_audit_hits[#timer_audit_hits + 1] = hit
            local phase_hits = timer_audit_after_event and timer_audit_post_hits or timer_audit_pre_hits
            phase_hits[#phase_hits + 1] = hit
            if #phase_hits < 16 then
                dosbox.breakpoint_set(0x01f7, 0xf049, {once = true})
            else
                dosbox.breakpoint_remove(0x01f7, 0xf049)
            end
        elseif input_release_probe and hit.segment == 0x01d7 and hit.offset == 0x493e then
            input_release_hit = hit
            dosbox.key(post_input_key, false)
            dosbox.breakpoint_remove(0x01d7, 0x493e)
            input_release_probe = false
            if timer_audit then
                timer_audit_after_event = true
                dosbox.breakpoint_remove(0x01f7, 0xf049)
                dosbox.breakpoint_set(0x01f7, 0xf049, {once = true})
            end
        else
            return hit
        end
    end
end
local first = wait_secondary_hit()
if first == nil then return end
if first.offset ~= 0x3861 and first.offset ~= 0x3862 then
    dosbox.breakpoint_set(0x01d7, 0x3861, {once = true})
    dosbox.breakpoint_set(0x01d7, 0x3862, {once = true})
    first = wait_secondary_hit()
    if first == nil then return end
end
assert(first.segment == 0x01d7 and
       (first.offset == 0x3861 or first.offset == 0x3862),
       "secondary MAP loader was not reached")

local map = map_state()
dosbox.breakpoint_remove(0x01d7, 0x3861)
dosbox.breakpoint_remove(0x01d7, 0x3862)
for _, offset in ipairs(call_sites) do dosbox.breakpoint_remove(0x01d7, offset) end

dosbox.breakpoint_set(0x01d7, 0x394c, {once = true})
dosbox.debug_continue()
local before_hit = wait_hit("secondary MAP mutation read")
local before_regs = before_hit.registers
local before_word = map_word(map, before_regs.edi - 1)
dosbox.breakpoint_set(0x01d7, 0x394f, {once = true})
dosbox.debug_continue()
local or_hit = wait_hit("secondary MAP mutation OR")
dosbox.breakpoint_set(0x01d7, 0x3963, {once = true})
dosbox.debug_continue()
local after_hit = wait_hit("secondary MAP mutation write")
local after_regs = after_hit.registers
local after_word = map_word(map, after_regs.edi - 1)
dosbox.breakpoint_set(0x01d7, 0x396d, {once = true})
dosbox.debug_continue()
local done_hit = wait_hit("secondary MAP mutation end")

dosbox.output.result = {
    schema = "quiky-secondary-map-lifecycle-trace-v1",
    level = select_level,
    checkpoints = {cheat = cheat, input_wait = input_wait,
                   launch = launch, entry = first,
                   controlled_input_key = event_before_secondary and post_input_key or nil,
                   controlled_input_release = input_release_hit,
                   timer_audit_hits = timer_audit_hits},
    state = {
        map = map,
        level_index = ds_word(0x85d4),
        wait_gate = ds_word(0x819e),
        scheduler_gate = ds_word(0x89ea),
        object_count = ds_word(0x880a),
    },
    mutation = {
        offset = before_regs.edi - 1,
        before_word = before_word,
        after_word = after_word,
        delta = after_word - before_word,
        before_registers = before_regs,
        or_registers = or_hit.registers,
        after_registers = after_regs,
        done = done_hit,
    },
}
