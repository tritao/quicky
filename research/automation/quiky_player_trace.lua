-- Capture the live object pool and MAP lookups around the gameplay update loop.
-- Loaded by research/tools/quikytrace.py with a structured TRACE_CONFIG table.
local trace_config = TRACE_CONFIG or {}
assert(type(trace_config) == "table", "TRACE_CONFIG must be a table")
local timeout_ms = trace_config.timeout_ms or 30000
local sample_count = trace_config.samples or 8
local frames_between = trace_config.frames_between or 30
local factory_focus = trace_config.factory_focus or false
local factory_start_sample = trace_config.factory_start_sample or 1
local focus_specs = trace_config.focus_objects
if type(focus_specs) ~= "table" or #focus_specs == 0 then
    focus_specs = {}
    if type(trace_config.focus_object) == "table" then
        focus_specs[1] = trace_config.focus_object
    end
end
local focus_object = focus_specs[1]
local object_focus_enabled = type(focus_object) == "table"
local focus_callback = trace_config.focus_callback or false
local focus_callback_offset = trace_config.focus_callback_offset or 0x3ff8
local callback_follow_return = trace_config.callback_follow_return ~= false
local callback_start_sample = trace_config.callback_start_sample or 1
local watch_offsets = trace_config.watch_offsets or {}
local watch_segments = trace_config.watch_segments or {}
local watch_targets = trace_config.watch_targets or {}
local stop_on_watch = trace_config.stop_on_watch or false
local stop_watch_offsets = trace_config.stop_watch_offsets or {}
local watch_only_start_sample = trace_config.watch_only_start_sample or 0
local pool_offsets = trace_config.pool_offsets or {}
if #watch_segments == 0 then
    watch_segments = {0x01f7}
end
local global_word_patches = trace_config.global_word_patches or {}
local selector_word_patches = trace_config.selector_word_patches or {}
local map_focus = trace_config.map_focus or false
local collision_focus = trace_config.collision_focus or false
local property_focus = trace_config.property_focus or false
local property_helper_offset = trace_config.property_helper_offset or 0
local branch_focus = trace_config.branch_focus or false
local branch_patch_tile = trace_config.branch_patch_tile
local collision_patch_tile = trace_config.collision_patch_tile
local descriptor_census = trace_config.descriptor_census or false
local descriptor_count = trace_config.descriptor_count or 512
local map_width = trace_config.map_width or 270
local map_height = trace_config.map_height or 30
local input_key = trace_config.input_key or ""
local input_key_2 = trace_config.input_key_2 or ""
local input_phases = trace_config.input_phases or {}
local input_frames = trace_config.input_frames or 0
local input_samples = trace_config.input_samples or 0
local player_teleport_x = trace_config.player_teleport_x
local player_teleport_y = trace_config.player_teleport_y
local player_teleport_start_sample = trace_config.player_teleport_start_sample or 1
local player_teleport_persist = trace_config.player_teleport_persist or false
local patch_every_frame = trace_config.patch_every_frame or false
local player_death_bypass = trace_config.player_death_bypass or false
local player_death_bypass_start_sample = trace_config.player_death_bypass_start_sample or 1
local object_patch = trace_config.object_patch
local object_word_patches = trace_config.object_word_patches or {}
local map_cell_word_patches = trace_config.map_cell_word_patches or {}
local select_level = trace_config.select_level or ""
local selector_frames = trace_config.selector_frames or 60
local trace_event_counter = 0
local descriptor_census_done = false
local player_teleport_original = nil
local object_patch_original = nil
local object_word_patch_originals = {}
local global_word_patch_originals = {}
local selector_word_patch_originals = {}
local map_cell_word_patch_originals = {}
local player_death_bypass_original = nil
local watch_skip_segment = nil
local watch_skip_offset = nil
local watch_target_consumed = {}
local frame_mutation_events = {}
local apply_frame_mutations
local current_trace_sequence = 1
local fast_pool_selector = nil
local fast_player_offset = nil
-- Object-focus mode needs the scheduler barrier to walk through callbacks
-- until the selected object is reached.  Player-focus mode already has a
-- stable callback barrier; adding 0F35 there would starve the requested
-- player/transition watch at the end of every scheduler pass.
local watch_barrier_enabled = object_focus_enabled and
                              (#watch_offsets > 0 or #watch_targets > 0)
local watch_barrier_segment = 0x01f7
local watch_barrier_offset = 0x0f35

local collision_offsets = {0x6484, 0x648e, 0x3a8a, 0x3a1f, 0x3df2}

local function factory_active(sequence)
    return factory_focus and sequence >= factory_start_sample
end

local function watch_only_active(sequence)
    return watch_only_start_sample > 0 and sequence >= watch_only_start_sample
end

local function stop_watch_matches(sample)
    if not stop_on_watch or sample.watch_events == nil or
       #sample.watch_events == 0 then
        return false
    end
    if #stop_watch_offsets == 0 then return true end
    for _, event in ipairs(sample.watch_events) do
        local breakpoint = event.breakpoint or {}
        for _, offset in ipairs(stop_watch_offsets) do
            if breakpoint.offset == offset then return true end
        end
    end
    return false
end

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

local function optional_word(s, index)
    if #s < index + 1 then return nil end
    return word(s, index)
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
        raw_size = #raw,
        state_hex = #raw >= 0x78 and hex(raw) or nil,
        position = {
            x_fixed = x_fixed,
            y_fixed = y_fixed,
            x = x_fixed >> 16,
            y = y_fixed >> 16,
        },
        action_word = word(raw, 1),
        velocity_x_fixed = dword(raw, 0x0a + 1),
        velocity_y_fixed = dword(raw, 0x0e + 1),
        object_word_0x2a = word(raw, 0x2a + 1),
        object_word_0x2c = word(raw, 0x2c + 1),
        kind = word(raw, 0x14 + 1),
        phase = string.byte(raw, 0x17 + 1),
        callback = word(raw, 0x18 + 1),
        callback_data = word(raw, 0x1a + 1),
        sprite_slot = word(raw, 0x12 + 1),
        lifetime = word(raw, 0x2c + 1),
        state_field = word(raw, 0x2e + 1),
        object_byte_0x28 = string.byte(raw, 0x28 + 1),
        object_byte_0x29 = string.byte(raw, 0x29 + 1),
        object_byte_0x34 = string.byte(raw, 0x34 + 1),
        object_word_0x38 = word(raw, 0x38 + 1),
        object_word_0x3c = optional_word(raw, 0x3c + 1),
        object_word_0x3e = optional_word(raw, 0x3e + 1),
        object_word_0x40 = optional_word(raw, 0x40 + 1),
        object_word_0x42 = optional_word(raw, 0x42 + 1),
        object_word_0x44 = optional_word(raw, 0x44 + 1),
        update_state = word(raw, 0x32 + 1),
        player_byte_0x36 = string.byte(raw, 0x36 + 1),
        player_byte_0x37 = string.byte(raw, 0x37 + 1),
        player_byte_0x38 = string.byte(raw, 0x38 + 1),
        player_byte_0x39 = string.byte(raw, 0x39 + 1),
        player_byte_0x3a = string.byte(raw, 0x3a + 1),
        player_byte_0x3b = string.byte(raw, 0x3b + 1),
        player_word_0x3e = word(raw, 0x3e + 1),
    }
end

local function object_matches_focus_spec(object, spec)
    if spec.object_offset ~= nil and object.offset ~= spec.object_offset then
        return false
    end
    if spec.callback_offset ~= nil and object.callback ~= spec.callback_offset then
        return false
    end
    local slot = object.sprite_slot or 0xffff
    return slot >= (spec.sprite_slot_min or 900) and
           slot <= (spec.sprite_slot_max or 999)
end

local function object_matches_focus(object)
    if not object_focus_enabled then return false end
    for _, spec in ipairs(focus_specs) do
        if object_matches_focus_spec(object, spec) then return true end
    end
    return false
end

local function exact_focus_offsets()
    local offsets = {}
    for _, spec in ipairs(focus_specs) do
        if spec.object_offset ~= nil then
            offsets[spec.object_offset] = true
        end
    end
    return offsets
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
    local focus_objects = {}
    local kind_0x64 = {}
    if pool_selector == 0 or stride == 0 then
        return {
            selector = pool_selector,
            offset = pool_offset,
            stride = stride,
            objects = objects,
            focus_objects = focus_objects,
            focus_count = #focus_objects,
            kind_0x64 = kind_0x64,
            error = "object pool pointer or stride is zero",
        }
    end
    local exact_offsets = exact_focus_offsets()
    for _, object_offset in ipairs(pool_offsets) do
        exact_offsets[object_offset] = true
    end
    local indices = {}
    if next(exact_offsets) ~= nil then
        for object_offset, _ in pairs(exact_offsets) do
            local delta = object_offset - pool_offset
            if delta < 0 or delta % stride ~= 0 then
                return {
                    selector = pool_selector,
                    offset = pool_offset,
                    stride = stride,
                    objects = objects,
                    focus_objects = focus_objects,
                    focus_count = 0,
                    kind_0x64 = kind_0x64,
                    error = "focused object offset is outside the current pool",
                }
            end
            indices[#indices + 1] = delta // stride
        end
        table.sort(indices)
    else
        for index = 0, 63 do indices[#indices + 1] = index end
    end
    for _, index in ipairs(indices) do
        local offset = pool_offset + index * stride
        local read_size = 0x40
        if exact_offsets[offset] then
            read_size = 0x78
        end
        local ok, raw_or_error = pcall(
            dosbox.mem_read_selector, pool_selector, offset, read_size
        )
        if ok and raw_or_error and #raw_or_error >= 0x40 then
            local object = object_snapshot(raw_or_error, pool_selector, offset, index)
            if object.callback ~= 0 then
                objects[#objects + 1] = object
                if object_matches_focus(object) then
                    focus_objects[#focus_objects + 1] = object
                end
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
        focus_count = #focus_objects,
        kind_0x64_count = #kind_0x64,
        objects = objects,
        focus_objects = focus_objects,
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
    local projectile_slots = {}
    for index = 0, 3 do
        local raw = dosbox.mem_read("ds", 0x87de + index * 4, 4) or ""
        if #raw >= 4 then
            projectile_slots[#projectile_slots + 1] = {
                index = index,
                x = word(raw, 1),
                y = word(raw, 3),
            }
        end
    end
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
        boss_phase_counter = dosbox.mem_read_byte("ds", 0x85da),
        boss_script_state = dosbox.mem_read_word("ds", 0x85d4),
        boss_world_id = dosbox.mem_read_word("ds", 0x85d8),
        boss_runtime_state = dosbox.mem_read_byte("ds", 0x88ae),
        boss_transition_word = dosbox.mem_read_word("ds", 0x89e6),
        boss_completion_word = dosbox.mem_read_word("ds", 0x89ec),
        transition_effect_state = dosbox.mem_read_byte("ds", 0x85db),
        transition_selector_shadow = dosbox.mem_read_word("ds", 0x85d6),
        transition_resource_flag = dosbox.mem_read_byte("ds", 0x5044),
        transition_resource_phase = dosbox.mem_read_word("ds", 0x504c),
        transition_resource_handle = dosbox.mem_read_word("ds", 0x5050),
        transition_resource_offset = dosbox.mem_read_word("ds", 0x5052),
        reentry_gate_word = dosbox.mem_read_word("ds", 0x89e0),
        reentry_pending_word = dosbox.mem_read_word("ds", 0x89ee),
        reentry_mode_word = dosbox.mem_read_word("ds", 0x89f0),
        reentry_aux_word = dosbox.mem_read_word("ds", 0x89f4),
        transition_input_state = dosbox.mem_read_word("ds", 0x88ba),
        transition_timer_word = dosbox.mem_read_word("ds", 0x880a),
        transition_ui_word = dosbox.mem_read_word("ds", 0x880c),
        transition_countdown = dosbox.mem_read_word("ds", 0x8822),
        transition_countdown_limit = dosbox.mem_read_word("ds", 0x8824),
        transition_reset_word = dosbox.mem_read_word("ds", 0x8952),
        projectile_count = dosbox.mem_read_word("ds", 0x8806),
        projectile_capacity = dosbox.mem_read_word("ds", 0x8808),
        projectile_slots = projectile_slots,
    }
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

-- A level-specific constructor can run immediately after the selector's
-- launch dispatch, before the first ordinary object-update barrier.  Arm the
-- requested callback before that dispatch so one-shot initializers such as
-- the W1 boss entry are not lost.
local function arm_startup_callback_targets()
    if not focus_callback or callback_start_sample > 1 then return end
    arm_callback_targets()
end

local function arm_watch_targets()
    if #watch_targets > 0 then
        for _, target in ipairs(watch_targets) do
            local segment = target.segment
            local offset = target.offset
            local start_sample = target.start_sample or 1
            if current_trace_sequence >= start_sample and
               not watch_target_consumed[_] and
               (segment ~= watch_skip_segment or offset ~= watch_skip_offset) then
                dosbox.breakpoint_set(segment, offset, {once = true})
            end
        end
    else
        for _, offset in ipairs(watch_offsets) do
            for _, segment in ipairs(watch_segments) do
                if segment ~= watch_skip_segment or offset ~= watch_skip_offset then
                    dosbox.breakpoint_set(segment, offset, {once = true})
                end
            end
        end
    end
    if watch_barrier_enabled and
       (watch_barrier_segment ~= watch_skip_segment or
        watch_barrier_offset ~= watch_skip_offset) then
        dosbox.breakpoint_set(watch_barrier_segment, watch_barrier_offset,
                              {once = true})
    end
end

local function arm_object_callback_targets()
    if not object_focus_enabled then
        return
    end
    local segments = {0x01d7, 0x01e7, 0x01f7, 0x0207, 0x0227, 0x0237, 0x1997}
    if focus_object.callback_offset ~= nil then
        for _, segment in ipairs(segments) do
            dosbox.breakpoint_set(segment, focus_object.callback_offset, {once = true})
        end
    end
    arm_watch_targets()
end

-- Per-frame mutation experiments already have an exact callback/object focus
-- and do not need to rescan all 64 pool records before every guest frame.
-- Keep the normal pool scan for sample barriers, but use one direct callback
-- breakpoint for the high-frequency safety barrier.
local function arm_frame_patch_target()
    -- Once a transition-watch-only barrier is active, do not re-arm the
    -- ordinary callback target here.  Otherwise the per-frame mutation mode
    -- silently defeats the watch-only handoff and keeps the trace inside the
    -- player callback while SEG01 is waiting on the completion effect.
    if watch_only_active(current_trace_sequence) then
        arm_watch_targets()
    elseif object_focus_enabled and focus_object.callback_offset ~= nil then
        dosbox.breakpoint_set(0x01f7, focus_object.callback_offset, {once = true})
    elseif object_focus_enabled then
        -- Slot/offset-only focus has no exact callback once the main record
        -- changes from B33B to 489C.  The scheduler tail is a cheap,
        -- one-frame barrier that remains valid across that callback swap.
        dosbox.breakpoint_set(0x01f7, 0x0f35, {once = true})
    elseif focus_callback then
        arm_callback_targets()
    end
    if #watch_offsets > 0 or #watch_targets > 0 then
        arm_watch_targets()
    end
end

-- The allocator focus is independent of sprite-slot focus.  A single
-- scheduler window can contain several nested 0E06 calls, so the capture
-- loop keeps both the allocator entry and the post-update 0F35 barrier armed.
local function arm_factory_targets()
    dosbox.breakpoint_set(0x01f7, 0x0e06, {once = true})
    dosbox.breakpoint_set(0x01f7, 0x0f35, {once = true})
end

local function clear_normal_targets()
    for _, offset in ipairs({0x0e96, 0x0f3c, 0x3376, 0x3f27}) do
        dosbox.breakpoint_remove(0x01f7, offset)
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

local function arm_targets(sequence)
    local watch_active = sequence >= (trace_config.watch_start_sample or 1)
    if factory_active(sequence) then
        arm_factory_targets()
        return
    end
    if watch_only_active(sequence) and watch_active then
        arm_watch_targets()
        return
    end
    if object_focus_enabled then
        if focus_object.callback_offset ~= nil then
            arm_object_callback_targets()
        else
            -- 0E96 is the common object-update entry used by the existing
            -- pool probe.  In slot-only mode it gives us a stable snapshot
            -- point without guessing a boss callback in advance.
            dosbox.breakpoint_set(0x01f7, 0x0e96, {once = true})
        end
        return
    end
    if focus_callback and sequence < callback_start_sample then
        -- Allow input phases to run before arming a callback that is created
        -- by the experiment itself, such as SCHUSS.BOB's 45AB update.
        dosbox.breakpoint_set(0x01f7, 0x0e96, {once = true})
        return
    end
    if focus_callback then
        arm_callback_targets()
        if watch_active then
            arm_watch_targets()
        end
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

callback_object_snapshot = function(hit)
    local registers = hit.registers or {}
    local selector = registers.es
    local offset = (registers.edi or 0) & 0xffff
    if selector == nil then return nil end
    local ok, raw_or_error = pcall(
        dosbox.mem_read_selector, selector, offset, 0x78
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

local function is_focused_callback_hit(hit)
    if not object_focus_enabled or focus_object.callback_offset == nil or
       hit.offset ~= focus_object.callback_offset then
        return false
    end
    local object = callback_object_snapshot(hit)
    return object ~= nil and object_matches_focus(object)
end

local function is_watch_target(hit)
    if watch_barrier_enabled and hit.segment == watch_barrier_segment and
       hit.offset == watch_barrier_offset then
        return true
    end
    if #watch_targets > 0 then
        for _, target in ipairs(watch_targets) do
            if hit.segment == target.segment and hit.offset == target.offset then
                return true
            end
        end
    else
        for _, offset in ipairs(watch_offsets) do
            if hit.offset == offset then
                for _, segment in ipairs(watch_segments) do
                    if hit.segment == segment then return true end
                end
            end
        end
    end
    return false
end

local function stop_watch_hit_matches(hit)
    if not stop_on_watch or not is_watch_target(hit) then return false end
    if #stop_watch_offsets == 0 then return true end
    for _, offset in ipairs(stop_watch_offsets) do
        if hit.offset == offset then return true end
    end
    return false
end

local function disarm_watch_hit(hit)
    if is_watch_target(hit) then
        dosbox.breakpoint_remove(hit.segment, hit.offset)
        if #watch_targets > 0 then
            for index, target in ipairs(watch_targets) do
                if target.segment == hit.segment and target.offset == hit.offset then
                    watch_target_consumed[index] = true
                end
            end
        end
        -- A breakpoint stops before executing its instruction.  Keep this
        -- address suppressed until the next breakpoint hit so the following
        -- sample executes the watched instruction instead of re-arming it at
        -- the same EIP.  This is important for code-offset watches in the
        -- SEG01 transition loop, where several watched labels are reached in
        -- one pass.  The next non-watch hit clears the suppression below.
        watch_skip_segment = hit.segment
        watch_skip_offset = hit.offset
    else
        watch_skip_segment = nil
        watch_skip_offset = nil
    end
end

local function append_watch_event(sample, hit, object)
    if not is_watch_target(hit) then return end
    sample.watch_events = sample.watch_events or {}
    local registers = hit.registers or {}
    local event = {
        breakpoint = {segment = hit.segment, offset = hit.offset},
        registers = hit.registers,
        stack_hex = hit.stack_hex or hex(dosbox.mem_read(
            "ss", (registers.esp or 0) & 0xffff, 16) or ""),
        globals = static_globals(),
    }
    if object ~= nil then event.object = object end
    sample.watch_events[#sample.watch_events + 1] = event
end

local function record_watch_event(sample, hit)
    append_watch_event(sample, hit)
end

local function record_ignored_watch_events(sample, ignored)
    for _, item in ipairs(ignored or {}) do
        append_watch_event(sample, item, item.object)
    end
end

local function wait_for_focused_callback(initial_hit)
    if not object_focus_enabled or focus_object.callback_offset == nil then
        return initial_hit, {}
    end
    local hit = initial_hit
    local ignored = {}
    local guard = 0
    while not is_focused_callback_hit(hit) do
        ignored[#ignored + 1] = {
            segment = hit.segment,
            offset = hit.offset,
            registers = hit.registers,
            stack_hex = hex(dosbox.mem_read(
                "ss", (hit.registers.esp or 0) & 0xffff, 16) or ""),
            object = callback_object_snapshot(hit),
        }
        if patch_every_frame and apply_frame_mutations ~= nil and
           is_watch_target(hit) then
            -- A watch breakpoint can be the exact pre-callback boundary
            -- needed by a controlled contact experiment (for example
            -- 489C). Reapply configured mutations before resuming.
            apply_frame_mutations(current_trace_sequence, "watch")
        end
        guard = guard + 1
        if guard > 128 then
            error("object-focus callback filter exceeded 128 callbacks")
        end
        disarm_watch_hit(hit)
        arm_object_callback_targets()
        dosbox.debug_continue()
        watch_skip_segment = nil
        watch_skip_offset = nil
        hit = wait_hit("focused object callback")
    end
    return hit, ignored
end

local function append_ignored_callbacks(destination, source)
    for _, item in ipairs(source or {}) do
        destination[#destination + 1] = item
    end
end

-- Input phases normally run as an unobserved block before the next sample
-- barrier.  When code-offset watches are active, step those windows through
-- the focused callback so a setter reached during input cannot be missed.
local function run_watched_frames(frame_count, label, pending)
    for _ = 1, frame_count do
        if patch_every_frame and apply_frame_mutations ~= nil then
            apply_frame_mutations(current_trace_sequence, "before")
        end
        if patch_every_frame then
            arm_frame_patch_target()
        elseif object_focus_enabled then
            arm_object_callback_targets()
        elseif focus_callback then
            arm_callback_targets()
            arm_watch_targets()
        end
        dosbox.debug_continue()
        -- Preserve a just-hit watch address for this resume.  The arm helper
        -- omits it once, allowing the watched instruction to execute and
        -- exposing the next transition label or normal callback.
        local hit = wait_hit(label)
        if patch_every_frame and apply_frame_mutations ~= nil and
           is_watch_target(hit) then
            apply_frame_mutations(current_trace_sequence, "watch")
        end
        local focused, ignored
        if patch_every_frame and object_focus_enabled and
           focus_object.callback_offset ~= nil then
            -- arm_frame_patch_target uses the exact known callback as the
            -- frame barrier; avoid a full object snapshot on every safety
            -- frame, which would exhaust the guest Lua instruction budget.
            focused, ignored = hit, {}
        else
            focused, ignored = wait_for_focused_callback(hit)
        end
        disarm_watch_hit(focused)
        append_ignored_callbacks(pending, ignored)
        if patch_every_frame and apply_frame_mutations ~= nil then
            apply_frame_mutations(current_trace_sequence, "after")
        end
        if focused == nil then
            error(label .. ": focused callback filter returned no hit")
        end
    end
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
        entry_object = callback_object_snapshot(hit),
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

local function allocator_event_snapshot(hit)
    local registers = hit.registers or {}
    local return_location = far_return_location(hit)
    return {
        event_index = next_trace_event(),
        breakpoint = {segment = hit.segment, offset = hit.offset},
        registers = registers,
        constructor_offset = (registers.eax or 0) & 0xffff,
        object = callback_object_snapshot(hit),
        stack_hex = hex(dosbox.mem_read(
            "ss", (registers.esp or 0) & 0xffff, 12) or ""),
        return_expected = return_location,
    }
end

-- Trace one scheduler window while preserving nested far-call returns.  The
-- allocator is reached by level-specific constructors and may be called
-- several times before the ordinary 0E96 barrier, so a single breakpoint
-- followed by one return is not sufficient for constructor graphs.
local function capture_factory_window(initial_hit)
    local hit = initial_hit
    local events = {}
    local return_stack = {}
    local guard = 0
    local last_event = nil
    while true do
        guard = guard + 1
        if guard > 256 then
            local top = return_stack[#return_stack]
            error(string.format(
                "allocator focus exceeded 256 breakpoints (last=0x%04x:%04x events=%d depth=%d top=%s last_stack=%s)",
                hit.segment, hit.offset, #events, #return_stack,
                top and string.format("0x%04x:%04x", top.location.segment, top.location.offset) or "none",
                last_event and (last_event.stack_hex or "") or "none"))
        end
        if hit.offset == 0x0e06 then
            local event = allocator_event_snapshot(hit)
            last_event = event
            events[#events + 1] = event
            if event.return_expected == nil then
                error("allocator focus could not read far return")
            end
            return_stack[#return_stack + 1] = {
                location = event.return_expected,
                event = event,
            }
        elseif #return_stack > 0 then
            local frame = return_stack[#return_stack]
            local expected = frame.location
            if hit.segment == expected.segment and hit.offset == expected.offset then
                frame.event.return_actual = {
                    segment = hit.segment,
                    offset = hit.offset,
                    registers = hit.registers,
                }
                frame.event.returned_object = callback_object_snapshot(hit)
                return_stack[#return_stack] = nil
            elseif hit.offset == 0x0f35 and #return_stack == 0 then
                return hit, events
            else
                error(string.format(
                    "allocator focus stopped at unexpected 0x%04x:%04x",
                    hit.segment, hit.offset))
            end
        elseif hit.offset == 0x0f35 then
            return hit, events
        else
            error(string.format(
                "allocator focus stopped at unexpected 0x%04x:%04x",
                hit.segment, hit.offset))
        end
        if #return_stack == 0 then
            arm_factory_targets()
        else
            for _, frame in ipairs(return_stack) do
                dosbox.breakpoint_set(frame.location.segment, frame.location.offset,
                                      {once = true})
            end
        end
        dosbox.debug_continue()
        hit = wait_hit("allocator focus window")
    end
end

local function append_factory_events(destination, source)
    for _, event in ipairs(source or {}) do
        destination[#destination + 1] = event
    end
end

local function advance_factory_barrier()
    -- 0F35 is immediately before the scheduler pass's far return.  Step over
    -- the call/return tail so re-arming 0F35 does not retrigger at the current
    -- instruction pointer.
    dosbox.breakpoint_set(0x01f7, 0x0f38, {once = true})
    dosbox.debug_continue()
    local hit = wait_hit("factory barrier advance")
    if hit.offset ~= 0x0f38 then
        error(string.format("factory barrier advance stopped at 0x%04x:%04x",
                            hit.segment, hit.offset))
    end
end

local function run_factory_frames(frame_count, label, pending, first_from_normal_barrier)
    for frame = 1, frame_count do
        -- A late-start factory trace is entered from the ordinary 0E96
        -- scheduler barrier.  The first frame must continue from that
        -- barrier; subsequent frames return through the factory 0F35/0F38
        -- handoff as usual.
        if frame > 1 or not first_from_normal_barrier then
            advance_factory_barrier()
        end
        arm_factory_targets()
        dosbox.debug_continue()
        local hit = wait_hit(label)
        local _, events = capture_factory_window(hit)
        append_factory_events(pending, events)
    end
end

-- 3A1F and 3DF2 are ordinary near-call helpers in segment 3.  Reading a
-- four-byte far return for them would pair the two-byte return IP with the
-- caller's stack data and leave the callback barrier armed at a bogus CS.
local function collision_return_location(hit)
    if hit.offset ~= 0x3a1f and hit.offset ~= 0x3df2 then
        return far_return_location(hit)
    end
    local registers = hit.registers or {}
    local raw = dosbox.mem_read(
        "ss", (registers.esp or 0) & 0xffff, 2
    ) or ""
    if #raw < 2 then return nil end
    return {offset = word(raw, 1), segment = hit.segment}
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
    -- 3DF2 is the leaf that passes the current world probe through the MAP
    -- descriptor path.  Read the live cell here while its AX/BX arguments are
    -- still intact; this avoids needing a second breakpoint mode and keeps
    -- the tile/descriptor evidence attached to the helper event.
    if hit.offset == 0x3df2 then
        if collision_patch_tile ~= nil then
            local lookup = map_lookup_snapshot(hit)
            if lookup.cell_word ~= nil then
                local original = lookup.cell_word
                local patched = (original & 0xfe00) | (collision_patch_tile & 0x1ff)
                dosbox.mem_write_selector(
                    lookup.map_selector, lookup.cell_offset,
                    string.char(patched & 0xff, (patched >> 8) & 0xff)
                )
                collision.patch = {
                    selector = lookup.map_selector,
                    offset = lookup.cell_offset,
                    original = original,
                    patched = patched,
                    readback = selector_word(lookup.map_selector, lookup.cell_offset),
                    tile_id = collision_patch_tile & 0x1ff,
                }
            end
        end
        collision.map_property = map_property_snapshot(hit)
    end
    sample.collisions = sample.collisions or {}
    sample.collisions[#sample.collisions + 1] = collision
    sample.collision = collision
    return collision
end

local function restore_collision_patch(collision)
    if collision == nil or collision.patch == nil then return end
    local patch = collision.patch
    dosbox.mem_write_selector(
        patch.selector, patch.offset,
        string.char(patch.original & 0xff, (patch.original >> 8) & 0xff)
    )
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
        W1IN = 15, W1L4 = 16, W2L4 = 17,
        W3L4 = 18, W4L4 = 19, W5L4 = 20,
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
    arm_startup_callback_targets()
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

local function fixed_dword(value)
    local raw = (value & 0xffff) << 16
    return string.char(raw & 0xff, (raw >> 8) & 0xff,
                       (raw >> 16) & 0xff, (raw >> 24) & 0xff)
end

-- Debugger-only object mutation for controlled collision experiments.  The
-- original object position and, when present, its SCHUSS coordinate-table
-- entry are restored before the trace exits.
local function integer_word(value)
    local raw = value & 0xffff
    return string.char(raw & 0xff, (raw >> 8) & 0xff)
end

-- Debugger-only loaded-MAP mutation for controlled death/collision
-- experiments. Cell coordinates are in the loaded MAP's word grid; the
-- original word is retained and restored before the trace exits.
local function apply_map_cell_word_patches(sequence)
    local applied = {}
    local map_base = dosbox.mem_read_word("ds", 0x657a)
    local map_selector = dosbox.mem_read_word("ds", 0x657c)
    local row_stride = dosbox.mem_read_word("ds", 0x657e)
    if map_base == nil or map_selector == nil or row_stride == nil then
        return nil
    end
    for index, patch in ipairs(map_cell_word_patches) do
        if sequence >= (patch.start_sample or 1) and
           (patch.end_sample == nil or sequence <= patch.end_sample) then
            local offset = map_base + patch.cell_y * row_stride + patch.cell_x * 2
            local original = dosbox.mem_read_selector(map_selector, offset, 2)
            if original and #original >= 2 then
                if map_cell_word_patch_originals[index] == nil then
                    map_cell_word_patch_originals[index] = {
                        selector = map_selector, offset = offset,
                        original = original,
                    }
                end
                dosbox.mem_write_selector(map_selector, offset,
                                          integer_word(patch.value))
                applied[#applied + 1] = {
                    index = index, selector = map_selector,
                    cell_x = patch.cell_x, cell_y = patch.cell_y,
                    offset = offset, value = patch.value,
                    original_hex = hex(original),
                }
            end
        end
    end
    if #applied == 0 then return nil end
    return applied
end

local function restore_map_cell_word_patches()
    for _, original in pairs(map_cell_word_patch_originals) do
        dosbox.mem_write_selector(original.selector, original.offset,
                                  original.original)
    end
end

-- Debugger-only control-flow mutation.  At 01F7:42EB the original JNE
-- enters the normal continuation when the fall probe succeeds and otherwise
-- executes the player reset writes.  Replace that four-byte conditional
-- branch with an equivalent near JMP for one controlled run only.
local function apply_player_death_bypass(sequence)
    if not player_death_bypass or sequence < player_death_bypass_start_sample or
       player_death_bypass_original ~= nil then
        return nil
    end
    local selector = 0x01f7
    local offset = 0x42eb
    local original = dosbox.mem_read_selector(selector, offset, 4)
    if not original or #original < 4 then
        return {error = "player death branch bytes are truncated"}
    end
    player_death_bypass_original = {
        selector = selector, offset = offset, original = original,
    }
    -- 42EB: E9 96 00 lands at 4384; the trailing NOP preserves the
    -- original four-byte instruction footprint.
    dosbox.mem_write_selector(selector, offset,
                              string.char(0xe9, 0x96, 0x00, 0x90))
    return {
        selector = selector, offset = offset,
        original_hex = hex(original), patched_hex = "e9960090",
    }
end

local function restore_player_death_bypass()
    if player_death_bypass_original == nil then return end
    dosbox.mem_write_selector(player_death_bypass_original.selector,
                              player_death_bypass_original.offset,
                              player_death_bypass_original.original)
end

local function apply_object_patch(sequence)
    if type(object_patch) ~= "table" or
       sequence < (object_patch.start_sample or 1) then
        return nil
    end
    local pointer_raw = dosbox.mem_read("ds", 0x755e, 4) or ""
    if #pointer_raw < 4 then return {error = "object pool pointer is truncated"} end
    local pointer = dword(pointer_raw, 1)
    local selector = (pointer >> 16) & 0xffff
    local offset = object_patch.object_offset
    local raw = dosbox.mem_read_selector(selector, offset, 0x78)
    if not raw or #raw < 0x40 then
        return {error = "patched object state is truncated", offset = offset}
    end
    local object = object_snapshot(raw, selector, offset, -1)
    if object_patch.callback_offset ~= nil and
       object.callback ~= object_patch.callback_offset then
        return {
            skipped = true, offset = offset, callback = object.callback,
            expected_callback = object_patch.callback_offset,
        }
    end
    local patch_x = object_patch.x
    local patch_y = object_patch.y
    local reference = nil
    if object_patch.reference_offset ~= nil then
        local reference_raw = dosbox.mem_read_selector(
            selector, object_patch.reference_offset, 0x78
        )
        if not reference_raw or #reference_raw < 0x40 then
            return {
                skipped = true, offset = offset,
                reference_offset = object_patch.reference_offset,
                reason = "patch reference object is not active",
            }
        end
        reference = object_snapshot(reference_raw, selector,
                                    object_patch.reference_offset, -1)
        if reference.callback == 0 then
            return {
                skipped = true, offset = offset,
                reference_offset = object_patch.reference_offset,
                reason = "patch reference object callback is zero",
            }
        end
        patch_x = reference.position.x + (object_patch.dx or 0)
        patch_y = reference.position.y + (object_patch.dy or 0)
    end
    if patch_x == nil or patch_y == nil then
        return {error = "object patch has no fixed or reference position", offset = offset}
    end
    local position_offset = offset + 0x02
    local original_position = dosbox.mem_read_selector(selector, position_offset, 8)
    if not original_position or #original_position < 8 then
        return {error = "patched object position is truncated", offset = offset}
    end
    if object_patch_original == nil then
        object_patch_original = {
            selector = selector,
            offset = offset,
            position_offset = position_offset,
            original_position = original_position,
        }
    end
    dosbox.mem_write_selector(selector, position_offset,
                              fixed_dword(patch_x) .. fixed_dword(patch_y))
    local patch = {
        selector = selector,
        offset = offset,
        x = patch_x,
        y = patch_y,
        callback = object.callback,
        original_position_hex = hex(original_position),
        reference_offset = object_patch.reference_offset,
    }
    -- SCHUSS stores its current position in a shared table.  Keep that table
    -- aligned with the object so the boss-side probe sees the mutation.
    if object.callback == 0x45ab then
        local table_index = object_patch.table_index
        if table_index == nil then table_index = object.object_word_0x2a end
        local table_offset = (0x87de + table_index * 4) & 0xffff
        local original_table = dosbox.mem_read("ds", table_offset, 4)
        if original_table and #original_table >= 4 then
            if object_patch_original.table_original == nil then
                object_patch_original.table_offset = table_offset
                object_patch_original.table_original = original_table
            end
            dosbox.mem_write("ds", table_offset,
                             integer_word(patch_x) .. integer_word(patch_y))
            patch.table_offset = table_offset
            patch.table_index = table_index
            patch.original_table_hex = hex(original_table)
        end
    end
    return patch
end

local function restore_object_patch()
    if object_patch_original == nil then return end
    dosbox.mem_write_selector(object_patch_original.selector,
                              object_patch_original.position_offset,
                              object_patch_original.original_position)
    if object_patch_original.table_offset ~= nil and
       object_patch_original.table_original ~= nil then
        dosbox.mem_write("ds", object_patch_original.table_offset,
                         object_patch_original.table_original)
    end
end

local function apply_object_word_patches(sequence)
    local applied = {}
    for index, patch in ipairs(object_word_patches) do
        if sequence >= (patch.start_sample or 1) and
           (patch.end_sample == nil or sequence <= patch.end_sample) then
            local pointer_raw = dosbox.mem_read("ds", 0x755e, 4) or ""
            if #pointer_raw >= 4 then
                local pointer = dword(pointer_raw, 1)
                local selector = (pointer >> 16) & 0xffff
                local targets = {}
                if patch.callback_offset ~= nil then
                    local pool_offset = pointer & 0xffff
                    local stride = dosbox.mem_read_word("ds", 0x30ce) or 0
                    if stride ~= 0 then
                        for object_index = 0, 63 do
                            local object_offset = pool_offset + object_index * stride
                            local callback_raw = dosbox.mem_read_selector(
                                selector, object_offset + 0x18, 2
                            ) or ""
                            if #callback_raw >= 2 and
                               word(callback_raw, 1) == patch.callback_offset then
                                targets[#targets + 1] = object_offset
                            end
                        end
                    end
                else
                    targets[1] = patch.object_offset
                end
                for _, object_offset in ipairs(targets) do
                    local address = object_offset + patch.field_offset
                    local original = dosbox.mem_read_selector(selector, address, 2)
                    if original and #original >= 2 then
                        local original_key = tostring(index) .. ":" .. tostring(address)
                        if object_word_patch_originals[original_key] == nil then
                            object_word_patch_originals[original_key] = {
                                selector = selector, address = address,
                                original = original,
                            }
                        end
                        dosbox.mem_write_selector(
                            selector, address,
                            integer_word(patch.value)
                        )
                        applied[#applied + 1] = {
                            index = index, selector = selector,
                            object_offset = object_offset,
                            callback_offset = patch.callback_offset,
                            field_offset = patch.field_offset,
                            value = patch.value,
                            original_hex = hex(original),
                        }
                    end
                end
            end
        end
    end
    if #applied == 0 then return nil end
    return applied
end

local function restore_object_word_patches()
    for _, original in pairs(object_word_patch_originals) do
        dosbox.mem_write_selector(original.selector, original.address,
                                  original.original)
    end
end

-- Debugger-only shared-state mutation.  This is deliberately limited to the
-- DS data segment and restored before the trace exits; it is useful for
-- testing control words such as DS:89EA without changing the executable.
local function apply_global_word_patches(sequence)
    local applied = {}
    for index, patch in ipairs(global_word_patches) do
        if sequence >= (patch.start_sample or 1) and
           (patch.end_sample == nil or sequence <= patch.end_sample) then
            local original = dosbox.mem_read_word("ds", patch.address)
            if original ~= nil then
                if global_word_patch_originals[index] == nil then
                    global_word_patch_originals[index] = {
                        address = patch.address,
                        original = original,
                    }
                end
                dosbox.mem_write("ds", patch.address, integer_word(patch.value))
                applied[#applied + 1] = {
                    index = index,
                    address = patch.address,
                    value = patch.value,
                    original = original,
                }
            end
        end
    end
    if #applied == 0 then return nil end
    return applied
end

local function restore_global_word_patches()
    -- Multiple debugger-only intervals may target the same DS word (for
    -- example, forcing state 6 during construction and state 1 afterward).
    -- Restore the earliest captured original once per address; restoring each
    -- interval's snapshot in table order would leave the later interval's
    -- intermediate value in guest memory.
    local indices = {}
    for index in pairs(global_word_patch_originals) do
        indices[#indices + 1] = index
    end
    table.sort(indices)
    local restored = {}
    for _, index in ipairs(indices) do
        local original = global_word_patch_originals[index]
        if restored[original.address] == nil then
            dosbox.mem_write("ds", original.address, integer_word(original.original))
            restored[original.address] = true
        end
    end
end

-- Debugger-only protected-memory mutation.  This is used for transition
-- experiments where the consumer lives in a selector-backed segment (for
-- example FFFF:2FEB), rather than in the executable's DS data segment.  In
-- this DOS extender build FFFF:offset is a conventional real-mode segment
-- address, not a valid protected-mode descriptor selector, so use the numeric
-- segment form for that one address family.
local function experiment_segment_read(selector, address, size)
    if selector == 0xffff then
        return dosbox.mem_read(selector, address, size)
    end
    return dosbox.mem_read_selector(selector, address, size)
end

local function experiment_segment_write(selector, address, raw)
    if selector == 0xffff then
        return dosbox.mem_write(selector, address, raw)
    end
    return dosbox.mem_write_selector(selector, address, raw)
end

local function apply_selector_word_patches(sequence)
    local applied = {}
    for index, patch in ipairs(selector_word_patches) do
        if sequence >= (patch.start_sample or 1) and
           (patch.end_sample == nil or sequence <= patch.end_sample) then
            local ok, original = pcall(
                experiment_segment_read, patch.selector, patch.address, 2
            )
            if ok and original and #original >= 2 then
                if selector_word_patch_originals[index] == nil then
                    selector_word_patch_originals[index] = {
                        selector = patch.selector,
                        address = patch.address,
                        original = original,
                    }
                end
                experiment_segment_write(
                    patch.selector, patch.address, integer_word(patch.value)
                )
                applied[#applied + 1] = {
                    index = index,
                    selector = patch.selector,
                    address = patch.address,
                    value = patch.value,
                    original_hex = hex(original),
                }
            end
        end
    end
    if #applied == 0 then return nil end
    return applied
end

local function restore_selector_word_patches()
    for _, original in pairs(selector_word_patch_originals) do
        experiment_segment_write(original.selector, original.address,
                                 original.original)
    end
end

local function apply_player_teleport(sequence)
    if player_teleport_x == nil or player_teleport_y == nil or
       sequence < player_teleport_start_sample then
        return nil
    end
    local function fixed(value)
        local raw = (value & 0xffff) << 16
        return string.char(raw & 0xff, (raw >> 8) & 0xff,
                           (raw >> 16) & 0xff, (raw >> 24) & 0xff)
    end
    if player_teleport_original ~= nil then
        if not player_teleport_persist and not patch_every_frame then return nil end
        dosbox.mem_write_selector(player_teleport_original.selector,
                                  player_teleport_original.x_offset,
                                  fixed(player_teleport_x) .. fixed(player_teleport_y))
        return {
            selector = player_teleport_original.selector,
            offset = player_teleport_original.offset,
            x = player_teleport_x, y = player_teleport_y,
            persistent = true,
        }
    end
    local pointer_raw = dosbox.mem_read("ds", 0x755e, 4) or ""
    if #pointer_raw < 4 then return {error = "object pool pointer is truncated"} end
    local pointer = dword(pointer_raw, 1)
    local selector = (pointer >> 16) & 0xffff
    local pool_offset = pointer & 0xffff
    local player_offset = dosbox.mem_read_word("ds", 0x881a) or pool_offset
    local x_offset = player_offset + 0x02
    local original = dosbox.mem_read_selector(selector, x_offset, 8)
    if not original or #original < 8 then
        return {error = "player position read is truncated"}
    end
    dosbox.mem_write_selector(selector, x_offset,
                               fixed(player_teleport_x) .. fixed(player_teleport_y))
    player_teleport_original = {
        selector = selector, offset = player_offset, x_offset = x_offset,
        original = original,
    }
    return {
        selector = selector, offset = player_offset,
        x = player_teleport_x, y = player_teleport_y,
        original_hex = hex(original),
    }
end

local function restore_player_teleport()
    if player_teleport_original == nil then return end
    dosbox.mem_write_selector(player_teleport_original.selector,
                              player_teleport_original.x_offset,
                              player_teleport_original.original)
end

-- A normal sample-boundary mutation is not enough for a player-safety
-- experiment: the guest can execute many callbacks between two samples and
-- recreate the death state in the meantime.  In the explicit debugger-only
-- per-frame mode, reapply every configured mutation on both sides of each
-- observed guest frame.  The trace configuration and this compact ledger
-- make the altered run distinguishable from behavioral evidence.
apply_frame_mutations = function(sequence, phase)
    local active = false
    local function record(kind, count)
        if not count or count < 1 then return end
        frame_mutation_events[#frame_mutation_events + 1] = {
            sequence = sequence,
            phase = phase,
            kind = kind,
            count = count,
        }
    end
    local function active_sample(patch)
        return sequence >= (patch.start_sample or 1) and
               (patch.end_sample == nil or sequence <= patch.end_sample)
    end
    local function pool_context()
        if fast_pool_selector ~= nil and fast_player_offset ~= nil then
            return fast_pool_selector, fast_player_offset
        end
        local pointer_raw = dosbox.mem_read("ds", 0x755e, 4) or ""
        if #pointer_raw < 4 then return nil, nil end
        local pointer = dword(pointer_raw, 1)
        fast_pool_selector = (pointer >> 16) & 0xffff
        local pool_offset = pointer & 0xffff
        fast_player_offset = dosbox.mem_read_word("ds", 0x881a) or pool_offset
        return fast_pool_selector, fast_player_offset
    end
    local selector_patch_result = apply_selector_word_patches(sequence)
    if selector_patch_result ~= nil then
        active = true
        record("selector_words", #selector_patch_result)
    end
    local selector, player_offset = pool_context()
    if selector == nil then return end
    if player_teleport_x ~= nil and player_teleport_y ~= nil and
       sequence >= player_teleport_start_sample then
        dosbox.mem_write_selector(selector, player_offset + 0x02,
                                  fixed_dword(player_teleport_x) ..
                                  fixed_dword(player_teleport_y))
        active = true
        record("player_teleport", 1)
    end
    local word_count = 0
    for _, patch in ipairs(object_word_patches) do
        if active_sample(patch) then
            dosbox.mem_write_selector(selector,
                                      patch.object_offset + patch.field_offset,
                                      integer_word(patch.value))
            word_count = word_count + 1
        end
    end
    if word_count > 0 then
        active = true
        record("object_words", word_count)
    end
    local global_count = 0
    for _, patch in ipairs(global_word_patches) do
        if active_sample(patch) then
            dosbox.mem_write("ds", patch.address, integer_word(patch.value))
            global_count = global_count + 1
        end
    end
    if global_count > 0 then
        active = true
        record("global_words", global_count)
    end
    if object_patch ~= nil and object_patch.x ~= nil and object_patch.y ~= nil and
       sequence >= (object_patch.start_sample or 1) then
        dosbox.mem_write_selector(selector, object_patch.object_offset + 0x02,
                                  fixed_dword(object_patch.x) ..
                                  fixed_dword(object_patch.y))
        active = true
        record("object_position", 1)
    end
end

dosbox.output.awaiting_startup_replay = true
dosbox.wait_frames(350)
if select_level ~= "" then
    begin_selected_level()
else
    dosbox.key("KBD_space", true)
    dosbox.wait_frames(4)
    dosbox.key("KBD_space", false)
end

local samples = {}
local experiment_frame = 0
local pending_ignored_focus_callbacks = {}
local pending_factory_events = {}
local stop_requested = false
for sequence = 1, sample_count do
    current_trace_sequence = sequence
    local watched_input_mode = ((object_focus_enabled and
                                 focus_object.callback_offset ~= nil) or
                                focus_callback) and
                               (#watch_offsets > 0 or #watch_targets > 0) and
                               sequence >= (trace_config.watch_start_sample or 1)
    if patch_every_frame and (object_focus_enabled or
                              focus_callback) then
        watched_input_mode = true
    end
    local factory_first_frame = not factory_active(sequence - 1)
    if factory_active(sequence) and not factory_active(sequence - 1) then
        clear_normal_targets()
    end
    local teleport = nil
    if sequence > 1 then
        local phase = input_phases[sequence - 1]
        if phase ~= nil then
            local phase_key = phase.key or ""
            local phase_key_2 = phase.key_2 or ""
            local phase_key_3 = phase.key_3 or ""
            local phase_frames = phase.frames or 0
            if phase_key == "" and phase_frames > 0 then
                if factory_active(sequence) then
                    run_factory_frames(phase_frames, "factory input phase", pending_factory_events,
                                       factory_first_frame)
                    factory_first_frame = false
                elseif watched_input_mode then
                    run_watched_frames(phase_frames, "watched input phase", pending_ignored_focus_callbacks)
                else
                    dosbox.wait_frames(phase_frames)
                end
                experiment_frame = experiment_frame + phase_frames
            elseif phase_key ~= "" and phase_frames > 0 then
                dosbox.key(phase_key, true)
                if phase_key_2 ~= "" then
                    dosbox.key(phase_key_2, true)
                end
                if phase_key_3 ~= "" then
                    dosbox.key(phase_key_3, true)
                end
                if factory_active(sequence) then
                    run_factory_frames(phase_frames, "factory input phase", pending_factory_events,
                                       factory_first_frame)
                    factory_first_frame = false
                elseif watched_input_mode then
                    run_watched_frames(phase_frames, "watched input phase", pending_ignored_focus_callbacks)
                else
                    dosbox.wait_frames(phase_frames)
                end
                if phase_key_3 ~= "" then
                    dosbox.key(phase_key_3, false)
                end
                if phase_key_2 ~= "" then
                    dosbox.key(phase_key_2, false)
                end
                dosbox.key(phase_key, false)
                experiment_frame = experiment_frame + phase_frames
            end
        elseif input_key ~= "" and input_frames > 0 and
               (input_samples == 0 or sequence <= input_samples + 1) then
            dosbox.key(input_key, true)
            if input_key_2 ~= "" then
                dosbox.key(input_key_2, true)
            end
            if factory_active(sequence) then
                run_factory_frames(input_frames, "factory input", pending_factory_events,
                                   factory_first_frame)
                factory_first_frame = false
            elseif watched_input_mode then
                run_watched_frames(input_frames, "watched input", pending_ignored_focus_callbacks)
            else
                dosbox.wait_frames(input_frames)
            end
            if input_key_2 ~= "" then
                dosbox.key(input_key_2, false)
            end
            dosbox.key(input_key, false)
            experiment_frame = experiment_frame + input_frames
        end
        if factory_active(sequence) then
            run_factory_frames(frames_between, "factory inter-sample window", pending_factory_events,
                               factory_first_frame)
            factory_first_frame = false
        elseif watched_input_mode then
            run_watched_frames(frames_between, "watched inter-sample window", pending_ignored_focus_callbacks)
        else
            dosbox.wait_frames(frames_between)
        end
        experiment_frame = experiment_frame + frames_between
    end
    if factory_active(sequence) and factory_active(sequence - 1) then
        advance_factory_barrier()
    end
    local callback_armed = (not focus_callback or sequence >= callback_start_sample) and
                           not watch_only_active(sequence)
    arm_targets(sequence)
    dosbox.debug_continue()
    -- Preserve a just-hit watch address for one resume.  Clearing it here
    -- would immediately re-arm the breakpoint at the current instruction.
    local hit = wait_hit("player/object update breakpoint")
    local factory_events = pending_factory_events
    pending_factory_events = {}
    if factory_active(sequence) then
        local current_factory_events
        hit, current_factory_events = capture_factory_window(hit)
        append_factory_events(factory_events, current_factory_events)
    end
    local ignored_focus_callbacks = pending_ignored_focus_callbacks
    pending_ignored_focus_callbacks = {}
    if object_focus_enabled and focus_object.callback_offset ~= nil then
        local current_ignored
        hit, current_ignored = wait_for_focused_callback(hit)
        append_ignored_callbacks(ignored_focus_callbacks, current_ignored)
    end
    local transition_watch_hit = is_watch_target(hit)
    local stop_target_hit = stop_watch_hit_matches(hit)
    local sample = {
        sequence = sequence,
        frame_index = experiment_frame,
        breakpoint = {segment = hit.segment, offset = hit.offset},
        registers = hit.registers,
        globals = static_globals(),
        pool = transition_watch_hit and {skipped_for_transition_watch = true} or pool_snapshot(),
        scheduler = transition_watch_hit and {skipped_for_transition_watch = true} or scheduler_snapshot(),
        related_breakpoints = {},
    }
    if factory_events ~= nil and #factory_events > 0 then
        sample.factory_events = factory_events
    end
    if #frame_mutation_events > 0 then
        sample.frame_mutations = frame_mutation_events
        frame_mutation_events = {}
    end
    -- Apply the debugger-only position override after the selected barrier so
    -- it is not immediately overwritten by spawn setup.  Delayed application
    -- is useful for reaching a later encounter phase without changing the
    -- earlier movement/collision experiment.
    teleport = apply_player_teleport(sequence)
    if teleport ~= nil then sample.player_teleport = teleport end
    local object_patch_result = apply_object_patch(sequence)
    if object_patch_result ~= nil then
        sample.object_patch = object_patch_result
    end
    local object_word_patch_result = apply_object_word_patches(sequence)
    if object_word_patch_result ~= nil then
        sample.object_word_patches = object_word_patch_result
    end
    local global_word_patch_result = apply_global_word_patches(sequence)
    if global_word_patch_result ~= nil then
        sample.global_word_patches = global_word_patch_result
    end
    local selector_word_patch_result = apply_selector_word_patches(sequence)
    if selector_word_patch_result ~= nil then
        sample.selector_word_patches = selector_word_patch_result
    end
    local map_cell_word_patch_result = apply_map_cell_word_patches(sequence)
    if map_cell_word_patch_result ~= nil then
        sample.map_cell_word_patches = map_cell_word_patch_result
    end
    local player_death_bypass_result = apply_player_death_bypass(sequence)
    if player_death_bypass_result ~= nil then
        sample.player_death_bypass = player_death_bypass_result
    end
    if #ignored_focus_callbacks > 0 then
        sample.ignored_focus_callbacks = ignored_focus_callbacks
        record_ignored_watch_events(sample, ignored_focus_callbacks)
    end
    local initial_hit = hit
    record_watch_event(sample, initial_hit)
    disarm_watch_hit(initial_hit)
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
    if callback_armed and focus_callback and initial_hit.offset ~= focus_callback_offset and
       initial_hit.offset ~= 0x3f27 and not is_watch_target(initial_hit) then
        arm_callback_targets()
        dosbox.debug_continue()
        hit = wait_hit("player callback after related breakpoint")
        sample.related_breakpoints[#sample.related_breakpoints + 1] = {
            segment = hit.segment, offset = hit.offset,
        }
        sample.breakpoint = {segment = hit.segment, offset = hit.offset}
        sample.registers = hit.registers
        sample.globals = static_globals()
        sample.pool = pool_snapshot()
        sample.scheduler = scheduler_snapshot()
    end
    local focused_callback = is_focused_callback_hit(hit)
    if hit.offset == 0x3f27 or (callback_armed and focus_callback and
       hit.offset == focus_callback_offset) or
       focused_callback then
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
        if callback_follow_return and #stack >= 4 and callback_object ~= nil then
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
            local collision_return_event = nil
            local stopped_during_callback_watch = false
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
                    if collision_return_event ~= nil then
                        collision_return_event.return_breakpoint = {
                            segment = candidate.segment,
                            offset = candidate.offset,
                            registers = candidate.registers,
                        }
                        restore_collision_patch(collision_return_event)
                    end
                    collision_return = nil
                    collision_return_event = nil
                elseif candidate.segment == return_segment and candidate.offset == return_offset then
                    returned = candidate
                else
                    sample.related_breakpoints[#sample.related_breakpoints + 1] = {
                        segment = candidate.segment, offset = candidate.offset,
                    }
                    if is_watch_target(candidate) then
                        append_watch_event(sample, candidate)
                        -- A watched transition label can be reached while
                        -- following a callback's return.  If it is the
                        -- requested terminal stop, leave DOSBox paused at
                        -- that exact instruction instead of continuing the
                        -- callback-return loop into an unbounded wait.
                        disarm_watch_hit(candidate)
                        if stop_watch_matches(sample) then
                            sample.stop_reason = "watched code offset"
                            stop_requested = true
                            stopped_during_callback_watch = true
                            break
                        end
                    end
                    if is_property_target(candidate.offset) then
                        record_property(sample, candidate)
                        property_return = far_return_location(candidate)
                    elseif candidate.offset == 0x3376 and map_focus then
                        record_map_lookup(sample, candidate)
                    elseif is_collision_target(candidate.offset) then
                        collision_return_event = record_collision(sample, candidate)
                        collision_return = collision_return_location(candidate)
                    end
                end
            end
            if stopped_during_callback_watch then
                sample.player_callback.stopped_during_watch = true
            else
                sample.player_callback.return_actual = {
                    segment = returned.segment, offset = returned.offset,
                }
                local ok, raw_or_error = pcall(
                    dosbox.mem_read_selector,
                    callback_object.selector, callback_object.offset, 0x78
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
        end
        if focused_callback then
            sample.object_focus = {
                callback_offset = hit.offset,
                object = callback_object,
                post_object = sample.player_callback.post_object,
            }
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
    samples[#samples + 1] = sample
    if stop_watch_matches(sample) then
        sample.stop_reason = "watched code offset"
        stop_requested = true
        break
    end
end

restore_player_teleport()
restore_object_patch()
restore_object_word_patches()
restore_global_word_patches()
restore_selector_word_patches()
restore_map_cell_word_patches()
restore_player_death_bypass()
local capture
if stop_requested then
    -- The watched breakpoint already left DOSBox paused.  Advancing to a
    -- second capture barrier would risk crossing into the transition code
    -- that this mode is explicitly meant to preserve.
    capture = {registers = dosbox.cpu_state()}
else
    capture = stop_for_capture()
end
local result = {
    trace_schema_version = trace_config.schema_version or 1,
    focus_object = focus_object,
    focus_objects = focus_specs,
    samples = samples,
    final_capture_registers = capture.registers,
    final_globals = static_globals(),
    final_pool = pool_snapshot(),
}
for _, sample in ipairs(samples) do
    if sample.descriptor_census ~= nil then
        result.descriptor_census = sample.descriptor_census
        break
    end
end
dosbox.output.player_trace = result
