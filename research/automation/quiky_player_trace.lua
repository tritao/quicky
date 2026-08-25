-- Capture the live object pool and MAP lookups around the gameplay update loop.
-- Loaded by research/tools/quikytrace.py with a structured TRACE_CONFIG table.
local trace_config = TRACE_CONFIG or {}
assert(type(trace_config) == "table", "TRACE_CONFIG must be a table")
local timeout_ms = trace_config.timeout_ms or 30000
local sample_count = trace_config.samples or 8
local frames_between = trace_config.frames_between or 30
local post_frames = trace_config.post_frames or 0
local post_space = trace_config.post_space or false
local focus_callback = trace_config.focus_callback or false
local focus_callback_offset = trace_config.focus_callback_offset or 0x3ff8
local map_focus = trace_config.map_focus or false
local collision_focus = trace_config.collision_focus or false
local property_focus = trace_config.property_focus or false
local property_helper_offset = trace_config.property_helper_offset or 0
local branch_focus = trace_config.branch_focus or false
local branch_patch_tile = trace_config.branch_patch_tile
local descriptor_census = trace_config.descriptor_census or false
local descriptor_count = trace_config.descriptor_count or 512
local map_width = trace_config.map_width or 270
local map_height = trace_config.map_height or 30
local input_key = trace_config.input_key or ""
local input_key_last = trace_config.input_key_last or ""
local input_key_secondary = trace_config.input_key_secondary or ""
local secondary_pulse_frames = trace_config.secondary_pulse_frames or 0
local secondary_start_sample = trace_config.secondary_start_sample or 1
local secondary_end_sample = trace_config.secondary_end_sample or 0
local input_frames = trace_config.input_frames or 0
local input_frames_last = trace_config.input_frames_last
local input_samples = trace_config.input_samples or 0
local state_events = trace_config.state_events or false
local state_event_start_sample = trace_config.state_event_start_sample or 1
local goal_probe = trace_config.goal_probe or false
local alternate_probe = trace_config.alternate_probe or false
local menu_probe = trace_config.menu_probe or false
local menu_probe_continue = trace_config.menu_probe_continue or false
local menu_auto_confirm = trace_config.menu_auto_confirm ~= false
local menu_exit_probe = trace_config.menu_exit_probe or false
local high_score_probe = trace_config.high_score_probe or false
local high_score_early_probe = trace_config.high_score_early_probe or false
local high_score_insert_only = trace_config.high_score_insert_only or false
local high_score_force_gate = trace_config.high_score_force_gate or false
local checkpoint_probe = trace_config.checkpoint_probe or false
local goal_force_player_ready = trace_config.goal_force_player_ready or false
local seed_health = trace_config.seed_health
local seed_lives = trace_config.seed_lives
local seed_score = trace_config.seed_score
local seed_position_x = trace_config.seed_position_x
local seed_position_y = trace_config.seed_position_y
local seed_camera_x = trace_config.seed_camera_x
local seed_camera_y = trace_config.seed_camera_y
local select_level = trace_config.select_level or ""
local selector_frames = trace_config.selector_frames or 60
local trace_event_counter = 0
local state_event_active = false
local descriptor_census_done = false
local alternate_probe_seen = {}
local alternate_probe_armed = false

local alternate_probe_targets = {
    [0x487f] = "alternate_completion_initializer",
    [0x489c] = "alternate_completion_callback",
    [0x4968] = "alternate_score_tally",
    [0xb82b] = "alternate_caller_world_state_b82b",
    [0xc0e2] = "alternate_caller_world_state_c0e2",
    [0xc933] = "alternate_caller_world_state_c933",
    [0xd2a8] = "alternate_caller_world_state_d2a8",
    [0xdbe9] = "alternate_caller_world_state_dbe9",
}
local main_completion_probe_targets = {
    [0x4968] = "main_completion_entry",
    [0x4cfc] = "transition_scene_branch",
    [0x4ea0] = "transition_signal_gate",
    [0x4eaa] = "transition_scene_setup",
    [0x4ef0] = "transition_scene_finalize",
    [0x4f03] = "transition_scene_finalize_calls",
    [0x4f0d] = "transition_scene_tick",
    [0x4f1a] = "progression_loop",
    [0x4fad] = "progression_updated",
    [0x4faf] = "progression_prior_copy",
    [0x5010] = "progression_group_map",
}
local main_completion_probe_seen = {}
local score_file_probe_targets = {
    [0x3471] = "score_table_initializer",
    -- 0x3565 is the SCORE.DAT string literal; the callable writer body
    -- starts at 0x356f (the 01E7 selector alias is what 01D7:08d1 calls).
    [0x356f] = "score_file_write_body",
    [0x36ed] = "score_file_load",
    -- High-score insertion restores the DOS interrupt-8 vector after the
    -- writer returns.  This is cleanup, not another file format routine.
    [0x39f0] = "score_file_timer_vector_restore",
}
local score_file_probe_seen = {}
local score_file_probe_segments = {0x01e7, 0x0207, 0x0227}
local high_score_probe_targets = {
    [0x0703] = "high_score_insert_entry",
    [0x0704] = "high_score_insert_prologue",
    [0x0a35] = "high_score_display_entry",
    [0x10a1] = "high_score_dispatch_gate_branch",
    [0x10ba] = "high_score_dispatch_insert_call_site",
    [0x10bd] = "high_score_dispatch_insert_skip",
}
local high_score_terminal_targets = {
    [0x0703] = true,
    [0x0704] = true,
    [0x0a35] = true,
    [0x3471] = true,
    [0x356f] = true,
    [0x36ed] = true,
    [0x39f0] = true,
}
local high_score_probe_seen = {}
local high_score_dispatch_armed = false
-- The insertion/display routines are in extracted segment 1 (runtime
-- selector 01D7).  Segment 2 has different code at the same offsets and is
-- the live GAME OVER/menu helper under selector 0207, so do not conflate the
-- two when interpreting same-offset breakpoints.
local high_score_code_segments = {0x01d7}
local menu_probe_targets = {
    [0x0470] = "game_menu_entry",
    [0x04ba] = "game_menu_input",
    [0x0c2c] = "game_over_screen_entry",
    [0x0d1f] = "post_game_menu_entry",
    [0x1084] = "game_over_menu_dispatcher",
    [0x50b1] = "restart_initializer_entry",
    [0x51a7] = "restart_dispatch_call",
    -- Runtime selector 0207 contains the distinct GAME OVER/menu helper.
    -- These offsets let a probe stop at its entry, restart decision, and
    -- return rather than misreading the same offsets in segment 1 as the
    -- high-score dispatcher.
    [0x10b0] = "game_over_menu_helper_entry",
    [0x15e7] = "game_over_menu_finish",
    [0x1648] = "game_over_menu_restart_check",
    [0x1685] = "game_over_menu_return",
    [0x168c] = "game_over_menu_lret",
}
local menu_probe_seen = {}
local menu_probe_armed = false
-- Complete static write-xref for DS:85D2.  These are the only executable
-- instructions whose ModR/M operand names that word in the extracted NE
-- segments; the 01F7:1ABA occurrence is a read and is intentionally absent.
local checkpoint_probe_targets = {
    {segment = 0x01d7, offset = 0x45db, label = "checkpoint_reset_initializer"},
    {segment = 0x01d7, offset = 0x4603, label = "checkpoint_reset_level_start"},
    {segment = 0x01d7, offset = 0x4b6b, label = "checkpoint_reset_restart"},
    {segment = 0x01d7, offset = 0x4bd5, label = "checkpoint_reset_transition"},
    {segment = 0x01f7, offset = 0x1afb, label = "checkpoint_reset_respawn"},
}
local checkpoint_probe_seen = {}
local checkpoint_probe_armed = false
local overlay_probe_segments = {0x01d7, 0x0207}

local function is_overlay_probe_segment(segment)
    return segment == 0x01d7 or segment == 0x0207
end

local function is_score_file_probe_segment(segment)
    return segment == 0x01e7 or segment == 0x0207 or segment == 0x0227
end

local function is_high_score_probe_segment(segment)
    for _, candidate in ipairs(high_score_code_segments) do
        if candidate == segment then return true end
    end
    return false
end

local function checkpoint_probe_target(segment, offset)
    if not checkpoint_probe then return nil end
    for _, target in ipairs(checkpoint_probe_targets) do
        if target.segment == segment and target.offset == offset then
            return target
        end
    end
    return nil
end

local collision_offsets = {0x6484, 0x648e, 0x3a8a, 0x3a1f, 0x3df2}

local function is_collision_target(offset)
    if not collision_focus then return false end
    for _, target in ipairs(collision_offsets) do
        if target == offset then return true end
    end
    return false
end

local function next_trace_event()
    trace_event_counter = trace_event_counter + 1
    return trace_event_counter
end

local function word(s, index)
    local lo, hi = string.byte(s, index, index + 1)
    return lo | (hi << 8)
end

local function dword(s, index)
    return word(s, index) | (word(s, index + 2) << 16)
end

local function selector_word(selector, offset)
    local raw = dosbox.mem_read_selector(selector, offset, 2)
    if not raw or #raw < 2 then
        error(string.format("short selector word read 0x%04x:0x%x",
                            selector, offset))
    end
    return word(raw, 1)
end

local function hex(s)
    return (s:gsub(".", function(c)
        return string.format("%02x", string.byte(c))
    end))
end

local function wait_hit(label)
    local hit, err = dosbox.wait_for_breakpoint(timeout_ms)
    if not hit then error(label .. ": " .. (err or "timeout")) end
    return hit
end

local function object_snapshot(raw, selector, offset, index)
    local x_fixed = dword(raw, 3)
    local y_fixed = dword(raw, 7)
    return {
        index = index,
        selector = selector,
        offset = offset,
        state_hex = hex(raw),
        position = {
            x_fixed = x_fixed,
            y_fixed = y_fixed,
            x = x_fixed >> 16,
            y = y_fixed >> 16,
        },
        action_word = word(raw, 1),
        velocity_x_fixed = dword(raw, 0x0a + 1),
        velocity_y_fixed = dword(raw, 0x0e + 1),
        kind = word(raw, 0x14 + 1),
        phase = string.byte(raw, 0x17 + 1),
        callback = word(raw, 0x18 + 1),
        callback_data = word(raw, 0x1a + 1),
        sprite_slot = word(raw, 0x12 + 1),
        lifetime = word(raw, 0x2c + 1),
        state_field = word(raw, 0x2e + 1),
        update_state = word(raw, 0x32 + 1),
        -- Shared player-object timer: damage grace is initialized to 0xD2,
        -- while pickup invulnerability uses 0x2BC.
        player_timer_0x34 = word(raw, 0x34 + 1),
        player_byte_0x36 = string.byte(raw, 0x36 + 1),
        player_byte_0x37 = string.byte(raw, 0x37 + 1),
        player_byte_0x38 = string.byte(raw, 0x38 + 1),
        player_byte_0x39 = string.byte(raw, 0x39 + 1),
        player_byte_0x3a = string.byte(raw, 0x3a + 1),
        player_byte_0x3b = string.byte(raw, 0x3b + 1),
        player_word_0x3e = word(raw, 0x3e + 1),
    }
end

local function pool_snapshot()
    local pointer_raw = dosbox.mem_read("ds", 0x755e, 4) or ""
    if #pointer_raw < 4 then
        return {error = "object pool pointer is truncated"}
    end
    local pointer = dword(pointer_raw, 1)
    local pool_offset = pointer & 0xffff
    local pool_selector = (pointer >> 16) & 0xffff
    local stride = dosbox.mem_read_word("ds", 0x30ce)
    local objects = {}
    local kind_0x64 = {}
    if pool_selector == 0 or stride == 0 then
        return {
            selector = pool_selector,
            offset = pool_offset,
            stride = stride,
            objects = objects,
            kind_0x64 = kind_0x64,
            error = "object pool pointer or stride is zero",
        }
    end
    for index = 0, 63 do
        local offset = pool_offset + index * stride
        local ok, raw_or_error = pcall(
            dosbox.mem_read_selector, pool_selector, offset, 0x40
        )
        if ok and raw_or_error and #raw_or_error >= 0x40 then
            local object = object_snapshot(raw_or_error, pool_selector, offset, index)
            if object.callback ~= 0 then
                objects[#objects + 1] = object
            end
            if object.kind == 0x64 then
                kind_0x64[#kind_0x64 + 1] = object
            end
        end
    end
    return {
        selector = pool_selector,
        offset = pool_offset,
        stride = stride,
        active_count = #objects,
        kind_0x64_count = #kind_0x64,
        objects = objects,
        kind_0x64 = kind_0x64,
    }
end

local function map_lookup_snapshot(hit)
    local registers = hit.registers or {}
    local y = (registers.eax or 0) & 0xffff
    local x = (registers.ebx or 0) & 0xffff
    local map_base = dosbox.mem_read_word("ds", 0x657a)
    local map_selector = dosbox.mem_read_word("ds", 0x657c)
    local row_stride = dosbox.mem_read_word("ds", 0x657e)
    local cell_offset = map_base + (y >> 4) * row_stride + (x >> 4) * 2
    local ok, value_or_error = pcall(selector_word, map_selector, cell_offset)
    local cell_read_error = nil
    if not ok then cell_read_error = tostring(value_or_error) end
    return {
        x = x,
        y = y,
        map_selector = map_selector,
        map_base = map_base,
        row_stride = row_stride,
        cell_offset = cell_offset,
        cell_word = ok and value_or_error or nil,
        tile_id = ok and (value_or_error & 0x1ff) or nil,
        cell_read_error = cell_read_error,
    }
end

local function map_property_snapshot(hit)
    local registers = hit.registers or {}
    local lookup = map_lookup_snapshot(hit)
    local tile_id = lookup.tile_id or 0
    local x = (registers.ebx or 0) & 0xffff
    local y = (registers.eax or 0) & 0xffff
    local stack_raw = dosbox.mem_read(
        "ss", (registers.esp or 0) & 0xffff, 4
    ) or ""
    local caller_return = nil
    if #stack_raw >= 4 then
        caller_return = {
            offset = word(stack_raw, 1),
            segment = word(stack_raw, 3),
        }
    end
    local descriptor_base = dosbox.mem_read_word("ds", 0x6582)
    local descriptor_selector = dosbox.mem_read_word("ds", 0x6584)
    local descriptor_stride = dosbox.mem_read_word("ds", 0x30d4)
    local descriptor_offset = descriptor_base + tile_id * descriptor_stride + 2
    local ok, descriptor_word_or_error = pcall(
        selector_word, descriptor_selector, descriptor_offset
    )
    local descriptor_read_error = nil
    if not ok then descriptor_read_error = tostring(descriptor_word_or_error) end
    local x_bit_3 = (x >> 3) & 0x01
    local y_bit_3 = (y >> 3) & 0x01
    local quadrant_flag_mask = nil
    if hit.offset == 0x5c27 then
        if y_bit_3 ~= 0 then
            quadrant_flag_mask = x_bit_3 ~= 0 and 0x02 or 0x01
        else
            quadrant_flag_mask = x_bit_3 ~= 0 and 0x04 or 0x08
        end
    end
    local descriptor_word = ok and descriptor_word_or_error or nil
    local descriptor_flag_set = nil
    local quadrant_bits = nil
    if descriptor_word ~= nil and quadrant_flag_mask ~= nil then
        descriptor_flag_set = (descriptor_word & quadrant_flag_mask) ~= 0
        quadrant_bits = descriptor_word & quadrant_flag_mask
    end
    return {
        helper_offset = hit.offset,
        breakpoint = {segment = hit.segment, offset = hit.offset},
        caller_return = caller_return,
        registers = registers,
        coordinates = {
            x = x,
            y = y,
            ax_bit_3 = y_bit_3,
            bx_bit_3 = x_bit_3,
        },
        map_lookup = lookup,
        map_property_field = lookup.cell_word and (lookup.cell_word >> 9) or nil,
        raw_cell_word = lookup.cell_word,
        tile_id = tile_id,
        descriptor_base = descriptor_base,
        descriptor_selector = descriptor_selector,
        descriptor_stride = descriptor_stride,
        descriptor_tile_id = tile_id,
        descriptor_offset = descriptor_offset,
        descriptor_word = descriptor_word,
        descriptor_low_nibble = descriptor_word and (descriptor_word & 0x0f) or nil,
        quadrant_flag_mask = quadrant_flag_mask,
        quadrant_bits = quadrant_bits,
        descriptor_flag_set = descriptor_flag_set,
        descriptor_read_error = descriptor_read_error,
    }
end

local function static_globals()
    return {
        input_action_flags = dosbox.mem_read_word("ds", 0x8196),
        keyboard_action_flags = dosbox.mem_read_word("ds", 0x88bc),
        last_scan_code = dosbox.mem_read_word("ds", 0x88ba),
        camera_x = dosbox.mem_read_word("ds", 0x81c0),
        camera_y = dosbox.mem_read_word("ds", 0x81c4),
        map_row_stride = dosbox.mem_read_word("ds", 0x657e),
        object_list_cursor = dosbox.mem_read_word("ds", 0x36e0),
        player_object_offset = dosbox.mem_read_word("ds", 0x881a),
        player_control_word = dosbox.mem_read_word("ds", 0x89ea),
    }
end

-- Authoritative game-state words used by the HUD, pickup handlers, damage
-- path, and level-transition code.  Keep these reads in the guest so every
-- trace records the selector actually active at the breakpoint (the game
-- normally selects DS=0x0237, but that is an observation rather than an
-- assumption the probe should bake in).
local function game_state_snapshot(selector_override)
    local selector = selector_override or dosbox.cpu_state().ds
    local function read_word(offset)
        if selector_override ~= nil then
            local raw = dosbox.mem_read_selector(selector, offset, 2) or ""
            return #raw >= 2 and word(raw, 1) or nil
        end
        return dosbox.mem_read_word("ds", offset)
    end
    local function read_bytes(offset, size)
        if selector_override ~= nil then
            return dosbox.mem_read_selector(selector, offset, size) or ""
        end
        return dosbox.mem_read("ds", offset, size) or ""
    end
    local score_raw = read_bytes(0x881c, 4)
    local checkpoint_raw = read_bytes(0x8828, 16)
    local player_object_offset = read_word(0x881a)
    local player_timer = nil
    local pool_pointer_raw = read_bytes(0x755e, 4)
    if player_object_offset ~= nil and #pool_pointer_raw >= 4 then
        local pool_pointer = dword(pool_pointer_raw, 1)
        local pool_selector = (pool_pointer >> 16) & 0xffff
        if pool_selector ~= 0 then
            -- During the ordinary (non-cheat) menu path the pool pointer can
            -- briefly contain a stale selector while the GAME OVER helper is
            -- rebuilding the pool.  Treat that interval as “timer unknown”
            -- instead of aborting the entire trace on an invalid selector.
            local ok, timer_raw = pcall(function()
                return dosbox.mem_read_selector(
                    pool_selector, player_object_offset + 0x34, 2
                ) or ""
            end)
            if ok and #timer_raw >= 2 then player_timer = word(timer_raw, 1) end
        end
    end
    local checkpoint_words = {}
    for index = 0, 7 do
        checkpoint_words[#checkpoint_words + 1] =
            read_word(0x8828 + index * 2)
    end
    return {
        data_selector = selector,
        lives = read_word(0x880a),
        ammo = read_word(0x880c),
        invulnerability_gate = read_word(0x8810),
        score = #score_raw >= 4 and dword(score_raw, 1) or nil,
        score_aux = read_word(0x880e),
        player_object_offset = player_object_offset,
        player_timer_0x34 = player_timer,
        health = read_word(0x8822),
        max_health = read_word(0x8824),
        checkpoint_index = read_word(0x85d2),
        progression = read_word(0x85d4),
        progression_prior = read_word(0x85d6),
        completion_route = read_word(0x85d8),
        -- Ending/cutscene phase byte used by the cloned 01F7:B/C/D state
        -- machines that feed 487F -> 489C -> 4968.
        ending_scene_stage = read_word(0x88ae),
        -- Nonzero selects the preserved-score/finalization branch in
        -- 01D7:50B1; zero takes the fresh-game initialization branch.
        session_gate_88af = read_word(0x88af),
        transition_signal = read_word(0x89e6),
        menu_gate_flag = read_word(0x89f2),
        session_words = {
            read_word(0x88b0), read_word(0x88b2), read_word(0x88b4),
            read_word(0x88b6), read_word(0x88b8),
        },
        checkpoint_table_words = checkpoint_words,
        checkpoint_table_raw_hex = #checkpoint_raw == 16 and hex(checkpoint_raw) or nil,
    }
end

local state_event_targets = {
    {segment = 0x01f7, offset = 0x199d, label = "instant_death"},
    {segment = 0x01f7, offset = 0x19e6, label = "damage"},
    {segment = 0x01f7, offset = 0x1a97, label = "invulnerability"},
    {segment = 0x01f7, offset = 0x1aaa, label = "respawn"},
    {segment = 0x01f7, offset = 0x487f, label = "alternate_completion_initializer"},
    {segment = 0x01f7, offset = 0x489c, label = "alternate_completion_callback"},
    {segment = 0x01f7, offset = 0x4968, label = "completion_entry"},
    {segment = 0x01f7, offset = 0x497c, label = "completion_score_applied"},
    {segment = 0x01f7, offset = 0x4996, label = "completion_transition"},
    {segment = 0x01f7, offset = 0x9254, label = "spawn_position_written"},
    {segment = 0x01d7, offset = 0x4f1a, label = "progression_loop"},
    {segment = 0x01d7, offset = 0x4fad, label = "progression_updated"},
}

local goal_callback_segments = {0x01d7, 0x01e7, 0x01f7, 0x0207,
                                0x0227, 0x0237, 0x1997}

local function is_state_event_target(hit)
    if not state_events or not state_event_active then return nil end
    if goal_probe and hit.offset == 0x9269 then
        return {label = "goal_callback"}
    end
    for _, target in ipairs(state_event_targets) do
        if target.segment == hit.segment and target.offset == hit.offset then
            return target
        end
    end
    return nil
end

local function arm_state_event_targets()
    if not state_events or not state_event_active then return end
    for _, target in ipairs(state_event_targets) do
        dosbox.breakpoint_set(target.segment, target.offset, {once = true})
    end
    if goal_probe then
        -- The object scheduler calls the callback through the near-call at
        -- 01F7:0ED1.  This dispatch barrier is useful even when the callback
        -- selector stored in the scheduler is an overlay alias.
        dosbox.breakpoint_set(0x01f7, 0x0ed1, {once = false})
        for _, segment in ipairs(goal_callback_segments) do
            dosbox.breakpoint_set(segment, 0x9269, {once = true})
        end
    end
end

local function is_goal_dispatch(hit)
    return goal_probe and hit.offset == 0x0ed1 and
        ((hit.registers and hit.registers.eax or 0) & 0xffff) == 0x9269
end

-- Read the table used by 5CC3 and the currently loaded MAP.  This is kept
-- inside the guest so the evidence records the actual runtime selectors and
-- offsets rather than assuming that the archive layout is identical to the
-- loaded segment layout.
local function descriptor_census_snapshot()
    local descriptor_base = dosbox.mem_read_word("ds", 0x6582)
    local descriptor_selector = dosbox.mem_read_word("ds", 0x6584)
    local descriptor_stride = dosbox.mem_read_word("ds", 0x30d4)
    local map_base = dosbox.mem_read_word("ds", 0x657a)
    local map_selector = dosbox.mem_read_word("ds", 0x657c)
    local map_pointer = {selector = map_selector, offset = map_base}
    local row_stride = dosbox.mem_read_word("ds", 0x657e)
    local descriptors = {}
    local descriptor_errors = 0
    for tile_id = 0, descriptor_count - 1 do
        local offset = descriptor_base + tile_id * descriptor_stride + 2
        local ok, value_or_error = pcall(
            selector_word, descriptor_selector, offset
        )
        local item = {
            tile_id = tile_id,
            offset = offset,
            word = ok and value_or_error or nil,
        }
        if not ok then
            descriptor_errors = descriptor_errors + 1
            item.read_error = tostring(value_or_error)
        end
        descriptors[#descriptors + 1] = item
    end

    local cells = {}
    local candidates = {}
    local cell_errors = 0
    if map_base ~= nil and map_selector ~= nil and row_stride ~= 0 then
        for y = 0, map_height - 1 do
            for x = 0, map_width - 1 do
                local offset = map_base + y * row_stride + x * 2
                local ok, cell_or_error = pcall(
                    selector_word, map_selector, offset
                )
                local cell = {
                    x = x,
                    y = y,
                    world_x = x * 16,
                    world_y = y * 16,
                    offset = offset,
                    cell = ok and cell_or_error or nil,
                }
                if ok then
                    local tile_id = cell_or_error & 0x1ff
                    local descriptor_offset =
                        descriptor_base + tile_id * descriptor_stride + 2
                    local d_ok, descriptor_or_error = pcall(
                        selector_word, descriptor_selector, descriptor_offset
                    )
                    cell.tile_id = tile_id
                    cell.property = (cell_or_error >> 9) & 0x7f
                    cell.descriptor = d_ok and descriptor_or_error or nil
                    if not d_ok then
                        cell.descriptor_read_error = tostring(descriptor_or_error)
                    end
                    if d_ok and (descriptor_or_error & 0x70) ~= 0 then
                        candidates[#candidates + 1] = cell
                    end
                else
                    cell_errors = cell_errors + 1
                    cell.read_error = tostring(cell_or_error)
                end
                cells[#cells + 1] = cell
            end
        end
    end
    return {
        map = {
            pointer = map_pointer,
            row_stride = row_stride,
            width = map_width,
            height = map_height,
            cells = cells,
            flag_candidates = candidates,
            read_errors = cell_errors,
        },
        descriptor_table = {
            base = descriptor_base,
            selector = descriptor_selector,
            stride = descriptor_stride,
            count = descriptor_count,
            entries = descriptors,
            read_errors = descriptor_errors,
        },
    }
end

local function arm_callback_targets()
    for _, segment in ipairs({0x01d7, 0x01e7, 0x01f7, 0x0207, 0x0227, 0x0237,
                              0x1997}) do
        dosbox.breakpoint_set(segment, focus_callback_offset, {once = true})
    end
end

local function arm_property_targets()
    if property_helper_offset == 0x5c27 or property_helper_offset == 0x5cc3 then
        dosbox.breakpoint_set(0x01f7, property_helper_offset, {once = true})
    else
        for _, offset in ipairs({0x5c27, 0x5cc3}) do
            dosbox.breakpoint_set(0x01f7, offset, {once = true})
        end
    end
end

local branch_entry_offset = 0x3d02
local branch_offsets = {0x3d1e, 0x3d36, 0x3d40, 0x3d45, 0x3dd0,
                        0x3de4, 0x3d44, 0x3df1}

local function arm_branch_targets(exclude_offset)
    for _, offset in ipairs(branch_offsets) do
        if offset ~= exclude_offset then
            dosbox.breakpoint_set(0x01f7, offset, {once = true})
        end
    end
end

local function is_branch_target(offset)
    if not branch_focus then return false end
    if offset == branch_entry_offset then return true end
    for _, target in ipairs(branch_offsets) do
        if target == offset then return true end
    end
    return false
end

local function is_branch_return(offset)
    return offset == 0x3d44 or offset == 0x3de4 or offset == 0x3df1
end

local function clear_branch_targets()
    dosbox.breakpoint_remove(0x01f7, branch_entry_offset)
    for _, offset in ipairs(branch_offsets) do
        dosbox.breakpoint_remove(0x01f7, offset)
    end
end

local callback_object_snapshot

local function patch_branch_probe_cell(sample, hit)
    if not branch_patch_tile then return nil end
    local object = callback_object_snapshot(hit)
    local position = object and object.position
    if not position then return nil end
    local x = position.x
    local y = position.y
    local map_base = dosbox.mem_read_word("ds", 0x657a)
    local map_selector = dosbox.mem_read_word("ds", 0x657c)
    local row_stride = dosbox.mem_read_word("ds", 0x657e)
    local offset = map_base + (y >> 4) * row_stride + ((x >> 3) & 0xfffe)
    local ok, original = pcall(selector_word, map_selector, offset)
    if not ok then return nil end
    local patched = (original & 0xfe00) | (branch_patch_tile & 0x1ff)
    dosbox.mem_write_selector(map_selector, offset,
                              string.char(patched & 0xff,
                                          (patched >> 8) & 0xff))
    local readback = selector_word(map_selector, offset)
    local patch = {
        selector = map_selector,
        offset = offset,
        x = x,
        y = y,
        original = original,
        tile_id = branch_patch_tile & 0x1ff,
        patched = patched,
        readback = readback,
    }
    sample.branch_patch = patch
    return patch
end

local function restore_branch_probe_cell(patch)
    if patch == nil then return end
    dosbox.mem_write_selector(patch.selector, patch.offset,
                              string.char(patch.original & 0xff,
                                          (patch.original >> 8) & 0xff))
end

local function is_property_target(offset)
    return property_focus and (offset == 0x5c27 or offset == 0x5cc3) and
           (property_helper_offset == 0 or offset == property_helper_offset)
end

local function arm_targets()
    arm_state_event_targets()
    if alternate_probe then
        if not alternate_probe_armed then
            for offset, _ in pairs(alternate_probe_targets) do
                dosbox.breakpoint_set(0x01f7, offset, {once = true})
            end
            -- Keep the broad transition labels for decoding a hit, but arm
            -- only the one-shot completion/save targets here.  The debugger
            -- build has a finite breakpoint budget while the normal frame
            -- barrier is also active.
            if not main_completion_probe_seen[0x4968] then
                dosbox.breakpoint_set(0x01d7, 0x4968, {once = true})
            end
            alternate_probe_armed = true
        end
    end
    if menu_probe then
        if not menu_probe_armed then
            for _, segment in ipairs(overlay_probe_segments) do
                for offset, _ in pairs(menu_probe_targets) do
                    dosbox.breakpoint_set(segment, offset, {once = true})
                end
            end
            menu_probe_armed = true
        end
    end
    if high_score_probe then
        for _, segment in ipairs(high_score_code_segments) do
            for offset, _ in pairs(high_score_probe_targets) do
                if not ((high_score_force_gate or high_score_insert_only) and
                        offset == 0x0a35) and
                   not high_score_probe_seen[offset] then
                    dosbox.breakpoint_set(segment, offset, {once = true})
                end
            end
        end
        -- The insertion entry calls the SCORE.DAT writer through selector
        -- 01E7.  Arm both selector aliases so a high-score trace can capture
        -- load/write/cleanup without requiring a separate probe mode.
        for _, segment in ipairs(score_file_probe_segments) do
            for offset, _ in pairs(score_file_probe_targets) do
                if not score_file_probe_seen[offset] then
                    dosbox.breakpoint_set(segment, offset, {once = true})
                end
            end
        end
        if high_score_force_gate and not high_score_dispatch_armed then
            -- The menu dispatcher can be reached while the guest is waiting
            -- for input, outside guarded_wait_frames.  Intercept its entry
            -- once so the debugger-only gate rewrite is applied immediately
            -- before the 0703 call-site branch is evaluated.
            for _, segment in ipairs(high_score_code_segments) do
                dosbox.breakpoint_set(segment, 0x1084, {once = true})
            end
            high_score_dispatch_armed = true
        end
    end
    if checkpoint_probe then
        if not checkpoint_probe_armed then
            for _, target in ipairs(checkpoint_probe_targets) do
                dosbox.breakpoint_set(target.segment, target.offset, {once = true})
            end
            checkpoint_probe_armed = true
        end
    end
    if focus_callback then
        arm_callback_targets()
    end
    if map_focus then
        dosbox.breakpoint_set(0x01f7, 0x3376, {once = true})
    end
    if collision_focus then
        for _, offset in ipairs(collision_offsets) do
            dosbox.breakpoint_set(0x01f7, offset, {once = true})
        end
    end
    if property_focus then
        arm_property_targets()
    end
    if descriptor_census and not descriptor_census_done then
        dosbox.breakpoint_set(0x01f7, 0x5cc3, {once = true})
    end
    if goal_probe then
        -- W1L1's goal/exit entity uses this callback.  Keep it armed
        -- alongside the ordinary player barrier so a traversal can stop
        -- on the handler before the level-transition code removes that
        -- barrier.
        for _, segment in ipairs(goal_callback_segments) do
            dosbox.breakpoint_set(segment, 0x9269, {once = true})
        end
    end
    if branch_focus then
        dosbox.breakpoint_set(0x01f7, branch_entry_offset, {once = true})
        arm_branch_targets()
    end
    if focus_callback or map_focus or collision_focus or property_focus or branch_focus or
       (descriptor_census and not descriptor_census_done) then
        return
    end
    dosbox.breakpoint_set(0x01f7, 0x0e96, {once = true})
    dosbox.breakpoint_set(0x01f7, 0x0f3c, {once = true})
    dosbox.breakpoint_set(0x01f7, 0x3376, {once = true})
    dosbox.breakpoint_set(0x01f7, 0x3f27, {once = true})
end

local function state_event_snapshot(hit, target)
    return {
        event_index = next_trace_event(),
        label = target.label,
        breakpoint = {segment = hit.segment, offset = hit.offset},
        registers = hit.registers,
        game_state = game_state_snapshot(),
    }
end

-- Advance exactly one guest frame while leaving state-event breakpoints
-- active.  The ordinary wait_frames API intentionally hides debugger stops,
-- so event-focused runs use the player update barrier as the frame boundary.
local function advance_state_event_frame()
    arm_targets()
    dosbox.debug_continue()
    local ok, hit_or_error = pcall(wait_hit, "state-event frame")
    if not ok then return nil, {}, hit_or_error end
    local hit = hit_or_error
    local events = {}
    if goal_probe and hit.offset == 0x0e96 then
        -- 0E96 is the pass-entry barrier.  The callback dispatches occur
        -- later in the same pass, at 0ED1, so scan forward before treating
        -- the entry hit as the completed frame.
        dosbox.debug_continue()
        ok, hit_or_error = pcall(wait_hit, "goal dispatch after pass entry")
        if not ok then return nil, events, hit_or_error end
        hit = hit_or_error
    end
    while goal_probe and hit.offset == 0x0ed1 and not is_goal_dispatch(hit) do
        dosbox.debug_continue()
        ok, hit_or_error = pcall(wait_hit, "goal dispatch scan")
        if not ok then return nil, events, hit_or_error end
        hit = hit_or_error
    end
    if is_goal_dispatch(hit) then
        events[#events + 1] = {
            event_index = next_trace_event(),
            label = "goal_dispatch",
            breakpoint = {segment = hit.segment, offset = hit.offset},
            registers = hit.registers,
            game_state = game_state_snapshot(),
        }
        dosbox.breakpoint_set(0x01f7, 0x9269, {once = true})
        dosbox.debug_continue()
        ok, hit_or_error = pcall(wait_hit, "goal callback entry")
        if not ok then return nil, events, hit_or_error end
        hit = hit_or_error
        if hit.offset == 0x9269 then
            events[#events + 1] = {
                event_index = next_trace_event(),
                label = "goal_callback_entry",
                breakpoint = {segment = hit.segment, offset = hit.offset},
                registers = hit.registers,
                game_state = game_state_snapshot(),
            }
            -- The permanent dispatcher barrier is useful while locating the
            -- callback, but it would win the next wait before the guarded
            -- transition write.  Remove it once the callback entry is known.
            dosbox.breakpoint_remove(0x01f7, 0x0ed1)
            -- Leave the entry breakpoint only after giving the handler a
            -- chance to reach its transition-signal write.  A short wait
            -- keeps a non-overlapping callback from stalling the trace.
            -- The branch at 9276 performs the four bounding-box compares.
            -- Walk each compare with only its possible successors armed so a
            -- miss is distinguishable from a write that was not reached.
            local goal_bounds_targets = {
                [0x9282] = "goal_bounds_x_lower",
                [0x9289] = "goal_bounds_x_upper",
                [0x9296] = "goal_bounds_y_lower",
                [0x929d] = "goal_bounds_y_upper",
                [0x92a7] = "goal_bounds_player_gate",
                [0x92a9] = "goal_signal_write_entry",
                [0x92af] = "goal_bounds_return_after_gate",
                [0x92b1] = "goal_bounds_return",
            }
            local goal_bounds_successors = {
                [0x9282] = {0x9289, 0x92b1},
                [0x9289] = {0x9296, 0x92b1},
                [0x9296] = {0x929d, 0x92b1},
                [0x929d] = {0x92a7, 0x92b1},
                [0x92a7] = {0x92a9, 0x92af},
            }
            local function arm_goal_bounds(offset)
                dosbox.breakpoint_clear()
                local successors = goal_bounds_successors[offset]
                if offset == 0x9276 then
                    successors = {0x9282, 0x92b1}
                end
                for _, successor in ipairs(successors or {}) do
                    dosbox.breakpoint_set(0x01f7, successor, {once = true})
                end
            end
            arm_goal_bounds(0x9276)
            dosbox.debug_continue()
            local signal_hit, signal_err = dosbox.wait_for_breakpoint(1000)
            while signal_hit and signal_hit.offset ~= 0x92a9 and
                  signal_hit.offset ~= 0x92af and
                  signal_hit.offset ~= 0x92b1 do
                events[#events + 1] = {
                    event_index = next_trace_event(),
                    label = goal_bounds_targets[signal_hit.offset] or
                        "goal_callback_bounds_checkpoint",
                    breakpoint = {segment = signal_hit.segment,
                                  offset = signal_hit.offset},
                    registers = signal_hit.registers,
                    game_state = game_state_snapshot(),
                }
                arm_goal_bounds(signal_hit.offset)
                dosbox.debug_continue()
                signal_hit = dosbox.wait_for_breakpoint(1000)
            end
            if signal_hit and signal_hit.offset ~= 0x92a9 then
                events[#events + 1] = {
                    event_index = next_trace_event(),
                    label = goal_bounds_targets[signal_hit.offset] or
                        "goal_callback_bounds_checkpoint",
                    breakpoint = {segment = signal_hit.segment,
                                  offset = signal_hit.offset},
                    registers = signal_hit.registers,
                    game_state = game_state_snapshot(),
                }
            end
            if signal_hit and signal_hit.offset == 0x92a9 then
                events[#events + 1] = {
                    event_index = next_trace_event(),
                    label = "goal_signal_write_entry",
                    breakpoint = {segment = signal_hit.segment,
                                  offset = signal_hit.offset},
                    registers = signal_hit.registers,
                    game_state = game_state_snapshot(),
                }
                dosbox.breakpoint_set(0x01f7, 0x92af, {once = true})
                dosbox.debug_continue()
                local signal_after = dosbox.wait_for_breakpoint(1000)
                if signal_after and signal_after.offset == 0x92af then
                    events[#events + 1] = {
                        event_index = next_trace_event(),
                        label = "goal_signal_written",
                        breakpoint = {segment = signal_after.segment,
                                      offset = signal_after.offset},
                        registers = signal_after.registers,
                        game_state = game_state_snapshot(signal_after.registers and
                                                         signal_after.registers.ds or nil),
                    }
                    -- Let the main loop consume DS:89E6 while tracing its
                    -- known checks.  This distinguishes a latched request
                    -- from an actual progression transition.
                    dosbox.breakpoint_clear()
                    -- DOSBox's debugger has a small hardware breakpoint
                    -- budget.  Walk this path one address at a time so the
                    -- signal consumer cannot evict the following breakpoint.
                    local transition_sequence = {
                        {offset = 0x48e6, label = "main_signal_consumer_check"},
                        {offset = 0x48eb, label = "main_signal_consumer_branch"},
                        {offset = 0x4968, label = "completion_entry"},
                        {offset = 0x497c, label = "completion_route_check"},
                        {offset = 0x4b8d, label = "completion_dispatch_branch"},
                        {offset = 0x4ba4, label = "main_transition_check_4ba4"},
                        {offset = 0x4c43, label = "main_transition_check_4c43"},
                        {offset = 0x4cfc, label = "transition_scene_branch"},
                        {offset = 0x4ea0, label = "transition_signal_gate"},
                        {offset = 0x4eaa, label = "transition_scene_setup"},
                        {offset = 0x4ef0, label = "transition_scene_finalize"},
                        {offset = 0x4f03, label = "transition_scene_finalize_calls"},
                        {offset = 0x4f0d, label = "transition_scene_tick"},
                        {offset = 0x4f1a, label = "progression_loop"},
                        {offset = 0x4fa9, label = "progression_increment"},
                        {offset = 0x4fad, label = "progression_updated"},
                        {offset = 0x4faf, label = "progression_prior_copy"},
                        {offset = 0x5010, label = "progression_group_map"},
                    }
                    local transition_index = 1
                    dosbox.breakpoint_set(0x01d7,
                                          transition_sequence[transition_index].offset,
                                          {once = true})
                    dosbox.debug_continue()
                    local transition_hit = dosbox.wait_for_breakpoint(3000)
                    local transition_steps = 0
                    while transition_hit ~= nil and transition_steps < 16 do
                        transition_steps = transition_steps + 1
                        local transition_target = transition_sequence[transition_index]
                        events[#events + 1] = {
                            event_index = next_trace_event(),
                            label = transition_target and transition_target.label or
                                "post_signal_breakpoint",
                            breakpoint = {segment = transition_hit.segment,
                                          offset = transition_hit.offset},
                            registers = transition_hit.registers,
                            game_state = game_state_snapshot(),
                        }
                        transition_index = transition_index + 1
                        dosbox.breakpoint_clear()
                        if transition_sequence[transition_index] == nil then break end
                        dosbox.breakpoint_set(0x01d7,
                                              transition_sequence[transition_index].offset,
                                              {once = true})
                        dosbox.debug_continue()
                        transition_hit = dosbox.wait_for_breakpoint(30000)
                    end
                    dosbox.breakpoint_clear()
                    dosbox.debug_continue()
                    -- The transition scene pauses for the same Space action
                    -- used by the startup selector.  Keep this pulse scoped
                    -- to the debugger-only goal probe so normal traces do not
                    -- synthesize a user confirmation.
                    if goal_probe then
                        dosbox.key("KBD_space", true)
                        dosbox.wait_frames(30)
                        dosbox.key("KBD_space", false)
                    end
                    -- The ordinary W1L1 exit does not execute the object-side
                    -- +5000 tally.  Keep a short post-transition probe armed
                    -- for the alternate/final-level object state that owns
                    -- 487F/489C/4968.
                    if goal_probe then
                        for offset, _ in pairs(alternate_probe_targets) do
                            dosbox.breakpoint_set(0x01f7, offset, {once = true})
                        end
                        dosbox.debug_continue()
                        local alternate_hit = dosbox.wait_for_breakpoint(5000)
                        if alternate_hit then
                            events[#events + 1] = {
                                event_index = next_trace_event(),
                                label = alternate_probe_targets[alternate_hit.offset] or
                                    "alternate_completion_breakpoint",
                                breakpoint = {segment = alternate_hit.segment,
                                              offset = alternate_hit.offset},
                                registers = alternate_hit.registers,
                                game_state = game_state_snapshot(
                                    alternate_hit.registers and
                                    alternate_hit.registers.ds or nil),
                            }
                        end
                        dosbox.breakpoint_clear()
                    end
                    dosbox.wait_frames(120)
                    events[#events + 1] = {
                        event_index = next_trace_event(),
                        label = "goal_post_transition_state",
                        breakpoint = {segment = signal_after.segment,
                                      offset = signal_after.offset},
                        registers = signal_after.registers,
                        game_state = game_state_snapshot(signal_after.registers and
                                                         signal_after.registers.ds or nil),
                    }
                    return signal_after, events
                elseif signal_after then
                    events[#events + 1] = {
                        event_index = next_trace_event(),
                        label = "goal_callback_followup",
                        breakpoint = {segment = signal_after.segment,
                                      offset = signal_after.offset},
                        registers = signal_after.registers,
                        game_state = game_state_snapshot(),
                    }
                end
            elseif signal_err ~= nil then
                -- The callback returned without reaching the guarded write.
                -- Preserve the entry event as the useful result.
            end
            return hit, events
        end
    end
    local target = is_state_event_target(hit)
    while target ~= nil do
        events[#events + 1] = state_event_snapshot(hit, target)
        if target.label == "goal_callback" then
            return hit, events
        end
        dosbox.debug_continue()
        ok, hit_or_error = pcall(wait_hit, "state-event sequence")
        if not ok then return nil, events, hit_or_error end
        hit = hit_or_error
        target = is_state_event_target(hit)
    end
    return hit, events
end

callback_object_snapshot = function(hit)
    local registers = hit.registers or {}
    local selector = registers.es
    local offset = (registers.edi or 0) & 0xffff
    if selector == nil then return nil end
    local ok, raw_or_error = pcall(
        dosbox.mem_read_selector, selector, offset, 0x40
    )
    if not ok or not raw_or_error or #raw_or_error < 0x40 then
        return {
            selector = selector,
            offset = offset,
            read_error = ok and "short object state" or tostring(raw_or_error),
        }
    end
    return object_snapshot(raw_or_error, selector, offset, -1)
end

local function record_map_lookup(sample, hit)
    local lookup = map_lookup_snapshot(hit)
    sample.map_lookups = sample.map_lookups or {}
    sample.map_lookups[#sample.map_lookups + 1] = lookup
    sample.map_lookup = lookup
end

local function record_property(sample, hit)
    local property = map_property_snapshot(hit)
    sample.map_properties = sample.map_properties or {}
    sample.map_properties[#sample.map_properties + 1] = property
    if sample.map_property == nil then sample.map_property = property end
end

local function record_state_event(sample, hit, target)
    sample.state_events = sample.state_events or {}
    sample.state_events[#sample.state_events + 1] = {
        event_index = next_trace_event(),
        label = target.label,
        breakpoint = {segment = hit.segment, offset = hit.offset},
        registers = hit.registers,
        game_state = game_state_snapshot(),
    }
end

local function record_branch(sample, hit)
    local registers = hit.registers or {}
    local dx = (registers.edx or 0) & 0xffff
    local event = {
        event_index = next_trace_event(),
        frame_index = sample.frame_index,
        offset = hit.offset,
        breakpoint = {segment = hit.segment, offset = hit.offset},
        registers = registers,
        dx = dx,
        dx_mask_0x30 = dx & 0x30,
        dx_mask_0x20 = dx & 0x20,
        dx_mask_0x40 = dx & 0x40,
        object = callback_object_snapshot(hit),
        globals = static_globals(),
    }
    sample.branch_events = sample.branch_events or {}
    sample.branch_events[#sample.branch_events + 1] = event
    sample.branch_event = event
end

local function capture_branch_sequence(sample, initial_hit)
    local patch = patch_branch_probe_cell(sample, initial_hit)
    local hit = initial_hit
    local guard = 0
    while true do
        if not is_branch_target(hit.offset) then
            error(string.format("unexpected collision branch breakpoint 0x%04x", hit.offset))
        end
        sample.related_breakpoints[#sample.related_breakpoints + 1] = {
            segment = hit.segment, offset = hit.offset,
        }
        record_branch(sample, hit)
        if is_branch_return(hit.offset) then
            sample.branch_return = {
                segment = hit.segment, offset = hit.offset,
                registers = hit.registers,
            }
            clear_branch_targets()
            restore_branch_probe_cell(patch)
            return hit
        end
        guard = guard + 1
        if guard > 32 then error("collision branch sequence exceeded 32 events") end
        arm_branch_targets(hit.offset)
        dosbox.debug_continue()
        hit = wait_hit("collision branch sequence")
    end
end

local function far_return_location(hit)
    local registers = hit.registers or {}
    local raw = dosbox.mem_read(
        "ss", (registers.esp or 0) & 0xffff, 4
    ) or ""
    if #raw < 4 then return nil end
    return {offset = word(raw, 1), segment = word(raw, 3)}
end

local function record_collision(sample, hit)
    local collision = {
        event_index = next_trace_event(),
        frame_index = sample.frame_index,
        helper_offset = hit.offset,
        breakpoint = {segment = hit.segment, offset = hit.offset},
        registers = hit.registers,
        object = callback_object_snapshot(hit),
        globals = static_globals(),
    }
    sample.collisions = sample.collisions or {}
    sample.collisions[#sample.collisions + 1] = collision
    sample.collision = collision
end

local function stop_for_capture()
    local current = dosbox.cpu_state()
    dosbox.breakpoint_set(current.cs, current.eip, {once = true})
    dosbox.debug_continue()
    return wait_hit("capture barrier")
end

local function begin_selected_level()
    local selector_indices = {
        W1L1 = 0, W1L2 = 1, W1L3 = 2,
        W2L1 = 3, W2L2 = 4, W2L3 = 5,
        W3L1 = 6, W3L2 = 7, W3L3 = 8,
        W4L1 = 9, W4L2 = 10, W4L3 = 11,
        W5L1 = 12, W5L2 = 13, W5L3 = 14,
        W1L4 = 15, W2L4 = 16, W3L4 = 17,
        W4L4 = 18, W5L4 = 19,
    }
    local selector_index = selector_indices[select_level]
    assert(selector_index ~= nil, "unsupported level selector target")
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
    dosbox.output.checkpoints = {cheat = cheat}
    dosbox.mem_write("ds", 0x89f2, "\x01")
    dosbox.mem_write("ds", 0x88ba, "\x05\x00")
    dosbox.debug_continue()
    dosbox.wait_frames(selector_frames)
    dosbox.breakpoint_set(0x01d7, 0x4ace, {once = true})
    local input_wait = wait_hit("selector input wait")
    dosbox.output.checkpoints.input_wait = input_wait
    dosbox.mem_write("ds", 0x85d4,
                     string.char(selector_index & 0xff, selector_index >> 8))
    dosbox.breakpoint_set(0x01d7, 0x4b18, {once = true})
    dosbox.mem_write("ds", 0x88bc, "\x20\x00")
    dosbox.debug_continue()
    dosbox.output.checkpoints.launch = wait_hit("selector Space dispatch")
end

local function scheduler_snapshot()
    local raw = dosbox.mem_read("ds", 0x7566, 0x200) or ""
    local entries = {}
    for index = 0, 63 do
        local base = index * 8 + 1
        if base + 7 > #raw then break end
        local callback_offset = word(raw, base)
        local callback_segment = word(raw, base + 2)
        local object_offset = word(raw, base + 4)
        local object_segment = word(raw, base + 6)
        if callback_offset == 0xffff and callback_segment == 0xffff then
            break
        end
        entries[#entries + 1] = {
            index = index,
            callback = {segment = callback_segment, offset = callback_offset},
            object = {selector = object_segment, offset = object_offset},
        }
    end
    return {base = 0x7566, stride = 8, entries = entries}
end

local function apply_high_score_force_gate()
    if not high_score_force_gate then return end
    dosbox.mem_write("ds", 0x89f2, "\x00")
    for _, offset in ipairs({0x88b0, 0x88b2, 0x88b4, 0x88b6, 0x88b8}) do
        dosbox.mem_write("ds", offset, "\x00\x00")
    end
end

local function guarded_wait_frames(frames)
    if not high_score_force_gate then
        dosbox.wait_frames(frames)
        return
    end
    for _ = 1, frames do
        apply_high_score_force_gate()
        dosbox.wait_frames(1)
    end
end

-- A normal player trace arms its semantic breakpoints only after the first
-- gameplay barrier.  The preserved-score high-score call is a one-time
-- finalization branch that can run before that barrier, so provide a narrow
-- debugger-only mode that catches it at 50B1 and walks the chain directly.
local function run_early_high_score_probe()
    assert(select_level == "", "early high-score probe requires the normal launch path")
    local events = {}
    local function put_word(offset, value)
        value = value & 0xffff
        dosbox.mem_write("ds", offset,
                         string.char(value & 0xff, (value >> 8) & 0xff))
    end
    local function put_dword(offset, value)
        value = value & 0xffffffff
        dosbox.mem_write("ds", offset,
                         string.char(value & 0xff, (value >> 8) & 0xff,
                                     (value >> 16) & 0xff,
                                     (value >> 24) & 0xff))
    end
    local function record(label, hit)
        events[#events + 1] = {
            label = label,
            breakpoint = {segment = hit.segment, offset = hit.offset},
            registers = hit.registers,
            game_state = game_state_snapshot(),
        }
    end

    -- Arm before the startup replay.  01D7:50B1 is part of the initial
    -- game-launch fall-through (the NE entry is 01D7:5089), so waiting for
    -- the ordinary 350-frame title barrier before arming it would miss it.
    dosbox.breakpoint_set(0x01d7, 0x5089, {once = true, deferred = true})
    dosbox.breakpoint_set(0x01d7, 0x50b1, {once = true, deferred = true})
    dosbox.wait_frames(350)
    dosbox.key("KBD_space", true)
    dosbox.debug_continue()
    local entry = wait_hit("early preserved-score finalization entry")
    if entry.offset == 0x5089 then
        record("program_entry", entry)
        dosbox.breakpoint_set(0x01d7, 0x50b1, {once = true})
        dosbox.debug_continue()
        entry = wait_hit("early preserved-score finalization entry after program entry")
    end
    dosbox.key("KBD_space", false)
    record("preserved_score_finalization_entry", entry)

    -- Force only the debugger-visible inputs needed to exercise the original
    -- preserved-score branch.  This does not patch code or the score file.
    put_word(0x88af, 1)
    put_word(0x89f2, 0)
    for _, offset in ipairs({0x88b0, 0x88b2, 0x88b4, 0x88b6, 0x88b8}) do
        put_word(offset, 0)
    end
    if seed_score ~= nil then put_dword(0x881c, seed_score) end

    -- Walk one instruction at a time so the finite debugger breakpoint
    -- budget never conflates 01D7's dispatcher with selector-0207's menu.
    dosbox.breakpoint_set(0x01d7, 0x1084, {once = true})
    local stage = "dispatcher_entry"
    local last_hit = entry
    while stage ~= nil do
        dosbox.debug_continue()
        local hit = wait_hit("early high-score " .. stage)
        last_hit = hit
        if stage == "dispatcher_entry" then
            record("dispatcher_entry", hit)
            dosbox.breakpoint_set(0x01d7, 0x10a1, {once = true})
            dosbox.breakpoint_set(0x01d7, 0x10ba, {once = true})
            dosbox.breakpoint_set(0x01d7, 0x10bd, {once = true})
            dosbox.breakpoint_set(0x01d7, 0x0703, {once = true})
            stage = "dispatcher_gate"
        elseif hit.offset == 0x10a1 then
            record("dispatcher_gate_branch", hit)
            stage = "dispatcher_gate_result"
        elseif hit.offset == 0x10ba then
            record("dispatcher_insertion_call_site", hit)
            stage = "insertion_entry"
        elseif hit.offset == 0x10bd then
            record("dispatcher_insertion_skip", hit)
            stage = nil
        elseif hit.offset == 0x0703 then
            record("high_score_insert_entry", hit)
            dosbox.breakpoint_set(0x01e7, 0x356f, {once = true})
            dosbox.breakpoint_set(0x01e7, 0x39f0, {once = true})
            dosbox.breakpoint_set(0x0227, 0x356f, {once = true})
            dosbox.breakpoint_set(0x0227, 0x39f0, {once = true})
            stage = "score_writer"
        elseif hit.offset == 0x356f then
            record("score_file_write_body", hit)
            stage = "score_vector_cleanup"
        elseif hit.offset == 0x39f0 then
            record("score_file_timer_vector_restore", hit)
            stage = nil
        else
            record("early_high_score_unexpected", hit)
            stage = nil
        end
    end

    dosbox.output.player_trace = {
        trace_schema_version = trace_config.schema_version or 1,
        samples = {},
        early_high_score = events,
        final_capture_registers = last_hit.registers,
        final_globals = static_globals(),
        final_game_state = game_state_snapshot(),
    }
end

-- Install the early entry breakpoint before publishing the replay barrier.
-- The host starts the startup recording as soon as this output field becomes
-- visible, so doing it afterward can race past the initial 5089->50B1 flow.
if high_score_early_probe then
    dosbox.breakpoint_set(0x01d7, 0x5089, {once = true, deferred = true})
    dosbox.breakpoint_set(0x01d7, 0x50b1, {once = true, deferred = true})
end
dosbox.output.awaiting_startup_replay = true
if high_score_early_probe then
    run_early_high_score_probe()
    return
end
dosbox.wait_frames(350)
if select_level ~= "" then
    begin_selected_level()
else
    dosbox.key("KBD_space", true)
    dosbox.wait_frames(4)
    dosbox.key("KBD_space", false)
    -- Let the normal W1L1 initializer finish before debugger-only state
    -- seeds are applied.  The cheat selector path already waits for its
    -- launch barrier above; the ordinary start needs this short settle.
    dosbox.wait_frames(60)
end

if seed_health ~= nil then
    dosbox.mem_write("ds", 0x8822,
                     string.char(seed_health & 0xff, (seed_health >> 8) & 0xff))
end
    if seed_lives ~= nil then
        dosbox.mem_write("ds", 0x880a,
                     string.char(seed_lives & 0xff, (seed_lives >> 8) & 0xff))
    end
    if seed_score ~= nil then
        dosbox.mem_write("ds", 0x881c,
                         string.char(seed_score & 0xff,
                                     (seed_score >> 8) & 0xff,
                                     (seed_score >> 16) & 0xff,
                                     (seed_score >> 24) & 0xff))
    end
    apply_high_score_force_gate()
local function seed_player_position()
    if seed_position_x == nil or seed_position_y == nil then return end
    local pointer_raw = dosbox.mem_read("ds", 0x755e, 4) or ""
    if #pointer_raw < 4 then error("player position seed: pool pointer is truncated") end
    local pool_pointer = dword(pointer_raw, 1)
    local pool_selector = (pool_pointer >> 16) & 0xffff
    local player_offset = dosbox.mem_read_word("ds", 0x881a)
    local x_fixed = (seed_position_x << 16) & 0xffffffff
    local y_fixed = (seed_position_y << 16) & 0xffffffff
    dosbox.mem_write_selector(
        pool_selector, player_offset + 2,
        string.char(x_fixed & 0xff, (x_fixed >> 8) & 0xff,
                    (x_fixed >> 16) & 0xff, (x_fixed >> 24) & 0xff))
    dosbox.mem_write_selector(
        pool_selector, player_offset + 6,
        string.char(y_fixed & 0xff, (y_fixed >> 8) & 0xff,
                    (y_fixed >> 16) & 0xff, (y_fixed >> 24) & 0xff))
    if seed_camera_x ~= nil and seed_camera_y ~= nil then
        dosbox.mem_write("ds", 0x81c0,
                         string.char(seed_camera_x & 0xff, seed_camera_x >> 8))
        dosbox.mem_write("ds", 0x81c4,
                         string.char(seed_camera_y & 0xff, seed_camera_y >> 8))
    end
end

local function force_goal_player_ready()
    if not goal_force_player_ready then return end
    local pointer_raw = dosbox.mem_read("ds", 0x755e, 4) or ""
    if #pointer_raw < 4 then return end
    local pool_pointer = dword(pointer_raw, 1)
    local pool_selector = (pool_pointer >> 16) & 0xffff
    local player_offset = dosbox.mem_read_word("ds", 0x881a)
    dosbox.mem_write_selector(pool_selector, player_offset + 0x37, "\x00")
end

local samples = {}
local experiment_frame = 0
for sequence = 1, sample_count do
    local hit
    state_event_active = state_events and sequence >= state_event_start_sample
    -- A route can require a final correction after a long held direction
    -- (the W1L1 goal object is just beyond the right-facing stopping point).
    -- Keep the override sample-local so earlier movement remains identical.
    local sequence_input_key = (sequence == sample_count and input_key_last ~= "")
        and input_key_last or input_key
    local sequence_input_frames = input_frames
    if sequence == sample_count and input_frames_last ~= nil then
        sequence_input_frames = input_frames_last
    end
    local interval_state_events = nil
    if state_event_active then
        interval_state_events = {}
        local interval_error = nil
        local goal_found = false
        local held_input = sequence > 1 and sequence_input_key ~= "" and
            sequence_input_frames > 0 and
            (input_samples == 0 or sequence <= input_samples + 1)
        local held_secondary = held_input and input_key_secondary ~= "" and
            sequence >= secondary_start_sample and
            (secondary_end_sample == 0 or sequence <= secondary_end_sample)
        local secondary_pressed = false
        if sequence == state_event_start_sample then
            seed_player_position()
        end
        if held_input then
            dosbox.key(sequence_input_key, true)
            -- Menu probes also keep the confirm key held during the gameplay
            -- drive so a death/summary screen can advance without a second
            -- runtime launch.  The secondary key remains available for
            -- movement/jump experiments.
            if menu_auto_confirm and (menu_probe or high_score_probe) then
                dosbox.key("KBD_space", true)
            end
            if held_secondary and secondary_pulse_frames == 0 then
                dosbox.key(input_key_secondary, true)
                secondary_pressed = true
            end
        end
        local frame_count = sequence > 1 and
            ((held_input and sequence_input_frames or 0) + frames_between) or 1
        for frame = 1, frame_count do
            if held_secondary and secondary_pulse_frames > 0 then
                local chunk = math.floor((frame - 1) / secondary_pulse_frames)
                local desired = (chunk % 2) == 0
                if desired ~= secondary_pressed then
                    dosbox.key(input_key_secondary, desired)
                    secondary_pressed = desired
                end
            end
            force_goal_player_ready()
            local frame_hit, frame_events = advance_state_event_frame()
            hit = frame_hit
            for _, event in ipairs(frame_events) do
                interval_state_events[#interval_state_events + 1] = event
                if event.label == "goal_dispatch" or
                   event.label == "goal_callback_entry" or
                   event.label == "goal_signal_write_entry" or
                   event.label == "goal_signal_written" or
                   event.label == "goal_callback" then
                    goal_found = true
                end
            end
            if goal_found then break end
            if hit == nil then
                interval_error = frame_events[#frame_events] and
                    frame_events[#frame_events].registers or nil
                break
            end
        end
        if held_input then
            if held_secondary and secondary_pressed then
                dosbox.key(input_key_secondary, false)
            end
            if menu_auto_confirm and (menu_probe or high_score_probe) then
                dosbox.key("KBD_space", false)
            end
            dosbox.key(sequence_input_key, false)
        end
        experiment_frame = experiment_frame + frame_count
        if hit == nil and #interval_state_events > 0 then
            local last_event = interval_state_events[#interval_state_events]
            hit = {
                segment = last_event.breakpoint.segment,
                offset = last_event.breakpoint.offset,
                registers = interval_error or last_event.registers,
            }
        end
        if hit == nil then
            local cpu = dosbox.cpu_state()
            hit = {segment = cpu.cs, offset = cpu.eip, registers = cpu}
        end
    else
        if sequence > 1 then
            if sequence_input_key ~= "" and sequence_input_frames > 0 and
               (input_samples == 0 or sequence <= input_samples + 1) then
                dosbox.key(sequence_input_key, true)
                local menu_pressed = false
                if input_key_secondary ~= "" and sequence >= secondary_start_sample and
                   (secondary_end_sample == 0 or sequence <= secondary_end_sample) then
                    if secondary_pulse_frames > 0 then
                        local remaining = sequence_input_frames
                        local pressed = true
                        while remaining > 0 do
                            dosbox.key(input_key_secondary, pressed)
                            local chunk = math.min(secondary_pulse_frames, remaining)
                            if menu_auto_confirm and (menu_probe or high_score_probe) then
                                menu_pressed = not menu_pressed
                                dosbox.key("KBD_space", menu_pressed)
                            end
                            guarded_wait_frames(chunk)
                            remaining = remaining - chunk
                            pressed = not pressed
                        end
                        if pressed == false then
                            dosbox.key(input_key_secondary, false)
                        end
                    else
                        dosbox.key(input_key_secondary, true)
                        local remaining = sequence_input_frames
                        while remaining > 0 do
                            local chunk = math.min(20, remaining)
                            if menu_auto_confirm and (menu_probe or high_score_probe) then
                                menu_pressed = not menu_pressed
                                dosbox.key("KBD_space", menu_pressed)
                            end
                            guarded_wait_frames(chunk)
                            remaining = remaining - chunk
                        end
                        dosbox.key(input_key_secondary, false)
                    end
                else
                    local remaining = sequence_input_frames
                    while remaining > 0 do
                        local chunk = math.min(20, remaining)
                        if menu_auto_confirm and (menu_probe or high_score_probe) then
                            menu_pressed = not menu_pressed
                            dosbox.key("KBD_space", menu_pressed)
                        end
                        guarded_wait_frames(chunk)
                        remaining = remaining - chunk
                    end
                end
                if menu_auto_confirm and (menu_probe or high_score_probe) and menu_pressed then
                    dosbox.key("KBD_space", false)
                end
                dosbox.key(sequence_input_key, false)
                experiment_frame = experiment_frame + sequence_input_frames
            end
            guarded_wait_frames(frames_between)
            experiment_frame = experiment_frame + frames_between
        end
        arm_targets()
        dosbox.debug_continue()
        hit = wait_hit("player/object update breakpoint")
    end
    if sequence == 1 and (not state_events or state_event_start_sample == 1) then
        seed_player_position()
    end
    if sequence == 1 and select_level == "" then
        -- The ordinary start path can finish its initializer after the first
        -- callback barrier; reapply debugger-only session seeds at that safe
        -- point so lives/score are not overwritten by startup code.
        if seed_health ~= nil then
            dosbox.mem_write("ds", 0x8822,
                             string.char(seed_health & 0xff,
                                         (seed_health >> 8) & 0xff))
        end
        if seed_lives ~= nil then
            dosbox.mem_write("ds", 0x880a,
                             string.char(seed_lives & 0xff,
                                         (seed_lives >> 8) & 0xff))
        end
        if seed_score ~= nil then
            dosbox.mem_write("ds", 0x881c,
                             string.char(seed_score & 0xff,
                                         (seed_score >> 8) & 0xff,
                                         (seed_score >> 16) & 0xff,
                                         (seed_score >> 24) & 0xff))
        end
        apply_high_score_force_gate()
    end
    local high_score_dispatch_events = {}
    if high_score_probe and high_score_force_gate and
       is_overlay_probe_segment(hit.segment) and hit.offset == 0x1084 then
        high_score_dispatch_events[#high_score_dispatch_events + 1] = {
            label = "dispatcher_entry_before_gate",
            breakpoint = {segment = hit.segment, offset = hit.offset},
            registers = hit.registers,
            game_state = game_state_snapshot(),
        }
        apply_high_score_force_gate()
        high_score_dispatch_events[#high_score_dispatch_events + 1] = {
            label = "dispatcher_entry_after_gate_rewrite",
            breakpoint = {segment = hit.segment, offset = hit.offset},
            registers = hit.registers,
            game_state = game_state_snapshot(),
        }
        -- Stop at the two conditional branches rather than waiting for the
        -- menu renderer.  10ba is immediately before the 0703 call; 10bd
        -- is the skip path when either gate condition remains nonzero.
        dosbox.breakpoint_set(hit.segment, 0x10a1, {once = true})
        dosbox.breakpoint_set(hit.segment, 0x10ba, {once = true})
        dosbox.breakpoint_set(hit.segment, 0x10bd, {once = true})
        dosbox.debug_continue()
        local branch_hit = wait_hit("high-score dispatcher gate branch")
        for _ = 1, 3 do
            high_score_dispatch_events[#high_score_dispatch_events + 1] = {
                label = (branch_hit.offset == 0x10a1 and
                         "dispatcher_gate_flag_branch" or
                         branch_hit.offset == 0x10ba and
                         "dispatcher_insertion_call_site" or
                         branch_hit.offset == 0x10bd and
                         "dispatcher_insertion_skip" or
                         "dispatcher_unexpected_branch_stop"),
                breakpoint = {segment = branch_hit.segment,
                              offset = branch_hit.offset},
                registers = branch_hit.registers,
                game_state = game_state_snapshot(),
            }
            if branch_hit.offset ~= 0x10a1 then break end
            dosbox.breakpoint_set(branch_hit.segment, 0x10ba, {once = true})
            dosbox.breakpoint_set(branch_hit.segment, 0x10bd, {once = true})
            dosbox.debug_continue()
            branch_hit = wait_hit("high-score dispatcher sum branch")
        end
        hit = branch_hit
    end
    local sample = {
        sequence = sequence,
        frame_index = experiment_frame,
        breakpoint = {segment = hit.segment, offset = hit.offset},
        registers = hit.registers,
        globals = static_globals(),
        game_state = game_state_snapshot(),
        pool = pool_snapshot(),
        scheduler = scheduler_snapshot(),
        related_breakpoints = {},
    }
    if #high_score_dispatch_events > 0 then
        sample.high_score_dispatch = high_score_dispatch_events
    end
    local alternate_label = alternate_probe and alternate_probe_targets[hit.offset]
    local checkpoint_target = checkpoint_probe_target(hit.segment, hit.offset)
    if alternate_probe and hit.segment == 0x01d7 and
       main_completion_probe_targets[hit.offset] ~= nil then
        alternate_label = main_completion_probe_targets[hit.offset]
        main_completion_probe_seen[hit.offset] = true
    elseif (alternate_probe or high_score_probe) and
           is_score_file_probe_segment(hit.segment) and
           score_file_probe_targets[hit.offset] ~= nil then
        alternate_label = score_file_probe_targets[hit.offset]
        score_file_probe_seen[hit.offset] = true
    elseif (alternate_probe or high_score_probe) and
           is_high_score_probe_segment(hit.segment) and
           high_score_probe_targets[hit.offset] ~= nil then
        alternate_label = high_score_probe_targets[hit.offset]
        high_score_probe_seen[hit.offset] = true
    elseif (alternate_probe or menu_probe) and
           is_overlay_probe_segment(hit.segment) and
           menu_probe_targets[hit.offset] ~= nil then
        alternate_label = menu_probe_targets[hit.offset]
        menu_probe_seen[hit.offset] = true
    elseif alternate_label ~= nil then
        alternate_probe_seen[hit.offset] = true
    end
    if alternate_label ~= nil then
        sample.alternate_probe = {
            label = alternate_label,
            breakpoint = {segment = hit.segment, offset = hit.offset},
            registers = hit.registers,
            game_state = game_state_snapshot(),
        }
    end
    if checkpoint_target ~= nil then
        local key = string.format("%04x:%04x", hit.segment, hit.offset)
        checkpoint_probe_seen[key] = true
        sample.checkpoint_probe = {
            label = checkpoint_target.label,
            breakpoint = {segment = hit.segment, offset = hit.offset},
            registers = hit.registers,
            checkpoint_index_before = sample.game_state.checkpoint_index,
            -- The breakpoint is on the xor/mov instruction, so this read is
            -- the pre-write value.  The next frame/sample records the value
            -- after the instruction has executed.
            game_state = game_state_snapshot(),
        }
    end
    if goal_probe and hit.offset == 0x9269 then
        sample.goal_probe = {
            breakpoint = {segment = hit.segment, offset = hit.offset},
            game_state = game_state_snapshot(),
        }
    end
    if state_event_active then
        if #interval_state_events > 0 then
            sample.state_events = interval_state_events
        end
    end
    local initial_hit = hit
    local descriptor_census_result = nil
    if descriptor_census and initial_hit.offset == 0x5cc3 then
        descriptor_census_result = descriptor_census_snapshot()
        descriptor_census_done = true
        sample.descriptor_census = descriptor_census_result
        local census_return = far_return_location(initial_hit)
        if census_return ~= nil then
            dosbox.breakpoint_set(census_return.segment, census_return.offset,
                                  {once = true})
            dosbox.debug_continue()
            hit = wait_hit("descriptor census helper return")
        end
    elseif property_focus and (initial_hit.offset == 0x5c27 or
                           initial_hit.offset == 0x5cc3) then
        sample.related_breakpoints[#sample.related_breakpoints + 1] = {
            segment = initial_hit.segment, offset = initial_hit.offset,
        }
        record_property(sample, initial_hit)
    elseif initial_hit.offset == 0x3376 and map_focus then
        sample.related_breakpoints[#sample.related_breakpoints + 1] = {
            segment = initial_hit.segment, offset = initial_hit.offset,
        }
        record_map_lookup(sample, initial_hit)
    elseif is_collision_target(initial_hit.offset) then
        sample.related_breakpoints[#sample.related_breakpoints + 1] = {
            segment = initial_hit.segment, offset = initial_hit.offset,
        }
        record_collision(sample, initial_hit)
    end
    if branch_focus and is_branch_target(initial_hit.offset) then
        hit = capture_branch_sequence(sample, initial_hit)
    end
    if focus_callback and initial_hit.offset ~= focus_callback_offset and
       initial_hit.offset ~= 0x3f27 then
        arm_callback_targets()
        dosbox.debug_continue()
        hit = wait_hit("player callback after related breakpoint")
        sample.related_breakpoints[#sample.related_breakpoints + 1] = {
            segment = hit.segment, offset = hit.offset,
        }
        sample.breakpoint = {segment = hit.segment, offset = hit.offset}
        sample.registers = hit.registers
        sample.globals = static_globals()
        sample.game_state = game_state_snapshot()
        sample.pool = pool_snapshot()
        sample.scheduler = scheduler_snapshot()
    end
    if hit.offset == 0x3f27 or (focus_callback and hit.offset == focus_callback_offset) then
        local callback_object = callback_object_snapshot(hit)
        sample.player_callback = {
            breakpoint = {segment = hit.segment, offset = hit.offset},
            callback_offset = hit.offset,
            registers = hit.registers,
            stack_hex = hex(dosbox.mem_read(
                "ss", (hit.registers.esp or 0) & 0xffff, 12) or ""),
            object = callback_object,
        }
        local stack = dosbox.mem_read(
            "ss", (hit.registers.esp or 0) & 0xffff, 4) or ""
        if #stack >= 4 and callback_object ~= nil then
            local return_offset = word(stack, 1)
            -- The scheduler calls the callback through a near code pointer;
            -- the next stack word is the DS argument, not a far return
            -- selector. Return to the callback's current CS.
            local return_segment = hit.segment
            sample.player_callback.return_expected = {
                segment = return_segment, offset = return_offset,
            }
            local returned = nil
            local property_return = nil
            local collision_return = nil
            while returned == nil do
                dosbox.breakpoint_set(return_segment, return_offset, {once = true})
                if property_return ~= nil then
                    dosbox.breakpoint_set(property_return.segment,
                                          property_return.offset, {once = true})
                elseif collision_return ~= nil then
                    dosbox.breakpoint_set(collision_return.segment,
                                          collision_return.offset, {once = true})
                elseif property_focus then
                    arm_property_targets()
                elseif collision_focus then
                    for _, offset in ipairs(collision_offsets) do
                        dosbox.breakpoint_set(0x01f7, offset, {once = true})
                    end
                end
                dosbox.debug_continue()
                local candidate = wait_hit("player callback return")
                if property_return ~= nil and
                   candidate.segment == property_return.segment and
                   candidate.offset == property_return.offset then
                    property_return = nil
                elseif collision_return ~= nil and
                       candidate.segment == collision_return.segment and
                       candidate.offset == collision_return.offset then
                    collision_return = nil
                elseif candidate.segment == return_segment and candidate.offset == return_offset then
                    returned = candidate
                else
                    sample.related_breakpoints[#sample.related_breakpoints + 1] = {
                        segment = candidate.segment, offset = candidate.offset,
                    }
                    if is_property_target(candidate.offset) then
                        record_property(sample, candidate)
                        property_return = far_return_location(candidate)
                    elseif candidate.offset == 0x3376 and map_focus then
                        record_map_lookup(sample, candidate)
                    elseif is_collision_target(candidate.offset) then
                        record_collision(sample, candidate)
                        collision_return = far_return_location(candidate)
                    end
                end
            end
            sample.player_callback.return_actual = {
                segment = returned.segment, offset = returned.offset,
            }
            local ok, raw_or_error = pcall(
                dosbox.mem_read_selector,
                callback_object.selector, callback_object.offset, 0x40
            )
            if ok and raw_or_error and #raw_or_error >= 0x40 then
                sample.player_callback.post_object = object_snapshot(
                    raw_or_error, callback_object.selector, callback_object.offset, -1
                )
            else
                sample.player_callback.post_object_read_error =
                    ok and "short object state" or tostring(raw_or_error)
            end
        end
        if map_focus and sample.map_lookup == nil then
            dosbox.breakpoint_set(0x01f7, 0x3376, {once = true})
            dosbox.debug_continue()
            local related = wait_hit("MAP lookup after player callback")
            sample.related_breakpoints[#sample.related_breakpoints + 1] = {
                segment = related.segment, offset = related.offset,
            }
            record_map_lookup(sample, related)
        elseif collision_focus and sample.collision == nil then
            for _, offset in ipairs(collision_offsets) do
                dosbox.breakpoint_set(0x01f7, offset, {once = true})
            end
            dosbox.debug_continue()
            local related = wait_hit("collision helper after player callback")
            sample.related_breakpoints[#sample.related_breakpoints + 1] = {
                segment = related.segment, offset = related.offset,
            }
            record_collision(sample, related)
        end
    elseif hit.offset == 0x0f3c then
        sample.kind_scan = {
            cursor = dosbox.mem_read_word("ds", 0x36e0),
            target_kind = 0x64,
        }
    end
    -- Optional post-GAME-OVER exit probe.  The normal menu probe can stop at
    -- the live 0207:10B0 helper, but it historically left the third menu
    -- choice untested.  Select it explicitly (Up advances by two modulo
    -- three, Space confirms) and walk the preserved-score chain if the exit
    -- path reaches segment 1.
    if menu_exit_probe and menu_probe and
       ((hit.segment == 0x0207 and hit.offset == 0x10b0) or
        (hit.segment == 0x01d7 and hit.offset == 0x0d1f)) then
        local chain = {}
        -- A missing chain is useful evidence, but must not consume the
        -- caller's full trace timeout (which can be several minutes).
        local exit_timeout = math.min(timeout_ms, 5000)
        local function chain_record(chain_hit)
            chain[#chain + 1] = {
                breakpoint = {segment = chain_hit.segment,
                              offset = chain_hit.offset},
                registers = chain_hit.registers,
                game_state = game_state_snapshot(),
            }
        end
        dosbox.breakpoint_clear()
        dosbox.breakpoint_set(0x01d7, 0x50b1, {once = true})
        dosbox.breakpoint_set(0x01d7, 0x51a7, {once = true})
        dosbox.breakpoint_set(0x01d7, 0x1084, {once = true})
        dosbox.breakpoint_set(0x01d7, 0x10bd, {once = true})
        dosbox.key("KBD_up", true)
        dosbox.wait_frames(4)
        dosbox.key("KBD_up", false)
        dosbox.wait_frames(4)
        dosbox.key("KBD_space", true)
        dosbox.wait_frames(4)
        dosbox.key("KBD_space", false)
        dosbox.debug_continue()
        local chain_hit = dosbox.wait_for_breakpoint(exit_timeout)
        local chain_steps = 0
        while chain_hit ~= nil and chain_steps < 8 do
            chain_steps = chain_steps + 1
            chain_record(chain_hit)
            dosbox.breakpoint_clear()
            if chain_hit.segment == 0x01d7 and chain_hit.offset == 0x50b1 then
                dosbox.breakpoint_set(0x01d7, 0x1084, {once = true})
                dosbox.breakpoint_set(0x01d7, 0x10bd, {once = true})
            elseif chain_hit.segment == 0x01d7 and chain_hit.offset == 0x1084 then
                dosbox.breakpoint_set(0x01d7, 0x0703, {once = true})
                for _, segment in ipairs(score_file_probe_segments) do
                    dosbox.breakpoint_set(segment, 0x356f, {once = true})
                    dosbox.breakpoint_set(segment, 0x39f0, {once = true})
                end
            elseif chain_hit.segment == 0x01d7 and chain_hit.offset == 0x0703 then
                for _, segment in ipairs(score_file_probe_segments) do
                    dosbox.breakpoint_set(segment, 0x356f, {once = true})
                    dosbox.breakpoint_set(segment, 0x39f0, {once = true})
                end
            else
                break
            end
            dosbox.debug_continue()
            chain_hit = dosbox.wait_for_breakpoint(exit_timeout)
        end
        sample.menu_exit_probe = {
            selection_action = "up_then_space",
            chain = chain,
            chain_timeout = chain_hit == nil,
        }
        if #chain > 0 then
            local last = chain[#chain]
            sample.breakpoint = last.breakpoint
            sample.registers = last.registers
            sample.game_state = last.game_state
            hit = {segment = last.breakpoint.segment,
                   offset = last.breakpoint.offset,
                   registers = last.registers}
        end
    end
    samples[#samples + 1] = sample
    if (alternate_probe or menu_probe or high_score_probe or checkpoint_probe) and
       (alternate_probe_targets[hit.offset] ~= nil or
        (alternate_probe and hit.segment == 0x01d7 and
         main_completion_probe_targets[hit.offset] ~= nil) or
        ((alternate_probe or high_score_probe) and
         is_high_score_probe_segment(hit.segment) and
         high_score_probe_targets[hit.offset] ~= nil and
         high_score_terminal_targets[hit.offset]) or
        ((alternate_probe or menu_probe) and not menu_probe_continue and
         is_overlay_probe_segment(hit.segment) and
         menu_probe_targets[hit.offset] ~= nil) or
        ((alternate_probe or high_score_probe) and
         is_score_file_probe_segment(hit.segment) and
         score_file_probe_targets[hit.offset] ~= nil)) then
        break
    end
    if goal_probe and hit.offset == 0x9269 then
        break
    end
end

-- A final player barrier is often the exact moment at which the goal object
-- latches DS:89E6.  Allow a bounded, breakpoint-free continuation so the
-- main loop can consume that signal and expose progression/scene state.
if post_frames > 0 then
    dosbox.breakpoint_clear()
    if post_space then dosbox.key("KBD_space", true) end
    dosbox.debug_continue()
    dosbox.wait_frames(post_frames)
    if post_space then dosbox.key("KBD_space", false) end
end

local capture = stop_for_capture()
local result = {
    trace_schema_version = trace_config.schema_version or 1,
    samples = samples,
    final_capture_registers = capture.registers,
    final_globals = static_globals(),
    final_game_state = game_state_snapshot(),
    final_pool = pool_snapshot(),
}
for _, sample in ipairs(samples) do
    if sample.descriptor_census ~= nil then
        result.descriptor_census = sample.descriptor_census
        break
    end
end
dosbox.output.player_trace = result
