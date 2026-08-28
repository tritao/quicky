-- Capture the live object pool and MAP lookups around the gameplay update loop.
-- Loaded by research/tools/quikytrace.py with a structured TRACE_CONFIG table.
local trace_config = TRACE_CONFIG or {}
assert(type(trace_config) == "table", "TRACE_CONFIG must be a table")
local common = assert(QUIKY_TRACE_COMMON, "QUIKY_TRACE_COMMON was not loaded")
local patch_watch = assert(QUIKY_PATCH_WATCH, "QUIKY_PATCH_WATCH was not loaded")
local breakpoint_controller = common.new_breakpoint_controller(dosbox)
local patch_engine = patch_watch.new(dosbox, trace_config.patches or {})
local execute_watches = trace_config.execute_watches or {}
local pending_sound_watch_enabled = false
for _, watch in ipairs(execute_watches) do
    if watch.segment == 0x01e7 and watch.offset == 0x0fcf then
        pending_sound_watch_enabled = true
        break
    end
end
local timeout_ms = trace_config.timeout_ms or 30000
local sample_count = trace_config.samples or 8
local frames_between = trace_config.frames_between or 30
local frames_between_after_sample = trace_config.frames_between_after_sample or 0
local frames_between_after = trace_config.frames_between_after or frames_between
local object_focus = trace_config.object_focus
local factory_focus = trace_config.factory_focus or false
local focus_callback = trace_config.focus_callback or object_focus ~= nil
local focus_callback_offset = object_focus and object_focus.callback_offset or
                              trace_config.focus_callback_offset or 0x3ff8
local map_focus = trace_config.map_focus or false
local collision_focus = trace_config.collision_focus or false
local property_focus = trace_config.property_focus or false
local property_helper_offset = trace_config.property_helper_offset or 0
local branch_focus = trace_config.branch_focus or false
local branch_patch_tile = trace_config.branch_patch_tile
local collision_patch_tile = trace_config.collision_patch_tile
local collision_patch_side = trace_config.collision_patch_side or "left"
local branch_patch_flags = trace_config.branch_patch_flags
local descriptor_census = trace_config.descriptor_census or false
local descriptor_count = trace_config.descriptor_count or 512
local map_width = trace_config.map_width or 270
local map_height = trace_config.map_height or 30
local probe_spawn_emitter = trace_config.probe_spawn_emitter or false
local probe_release_emitter = trace_config.probe_release_emitter or false
local spawn_probe_done = false
local input_key = trace_config.input_key or ""
local input_key_switch = trace_config.input_key_switch or ""
local input_switch_sample = trace_config.input_switch_sample or 0
local input_key_secondary = trace_config.input_key_secondary or ""
local secondary_pulse_frames = trace_config.secondary_pulse_frames or 0
local secondary_start_sample = trace_config.secondary_start_sample or 1
local secondary_end_sample = trace_config.secondary_end_sample or 0
local input_frames = trace_config.input_frames or 0
local input_samples = trace_config.input_samples or 0
local input_warmup_frames = trace_config.input_warmup_frames or 0
local record_input_stream = trace_config.record_input_stream or false
local input_hold_key = trace_config.input_hold_key or ""
local input_hold_frames = trace_config.input_hold_frames or 0
local input_hold_keys = {}
for key in string.gmatch(input_hold_key, "[^+]+") do
    input_hold_keys[#input_hold_keys + 1] = key
end
local map_patch_cell = trace_config.map_patch_cell
local map_patch_descriptor = trace_config.map_patch_descriptor
local map_patch_word = trace_config.map_patch_word
local persistent_map_patch = trace_config.persistent_map_patch or false
local persistent_map_patch_state = nil
local input_phases = trace_config.input_phases or {}
local input_phase_through_callback = trace_config.input_phase_through_callback or false
local input_phase_hold_callbacks = trace_config.input_phase_hold_callbacks or 1
local input_phase_hold_after_wait = trace_config.input_phase_hold_after_wait or false
local capture_player_record = trace_config.capture_player_record or false
local minimal_callback_capture = trace_config.minimal_callback_capture or false
local parity_callback_capture = trace_config.parity_callback_capture or false
local scheduler_only = trace_config.scheduler_only or false
-- Full pool snapshots are useful for discovery, but they are expensive: each
-- sample walks 64 records and decodes every field.  Once the callback has
-- been focused and the 0x78-byte player record is requested, the callback
-- itself is the authoritative target and the repeated pool walk only burns
-- the automation Lua instruction budget.
-- Execute-watch-only runs do not need the 64-record pool walk either.  When
-- the caller explicitly requests minimal capture, retain just the initial
-- pool snapshot and the watched instruction/object state; this keeps a long
-- approach trajectory below the Lua instruction budget while preserving the
-- exact watched callback address and registers.
local lean_player_capture = scheduler_only or
                            (focus_callback and capture_player_record) or
                            (minimal_callback_capture and
                             (focus_callback or #execute_watches > 0))
local transition_focus = trace_config.transition_focus or false
local transition_steps = trace_config.transition_steps or 48
local transition_hold_events = trace_config.transition_hold_events or 48
local transition_force_player_fall = trace_config.transition_force_player_fall or false
local transition_probe_frames = trace_config.transition_probe_frames or 0
local transition_probe_tail_frames = trace_config.transition_probe_tail_frames or 0
local transition_probe_tail_camera_x = trace_config.transition_probe_tail_camera_x or 0
local transition_warmup_frames = trace_config.transition_warmup_frames or 0
local select_level = trace_config.select_level or ""
local selector_frames = trace_config.selector_frames or 60
local capture_startup_stream = trace_config.capture_startup_stream or false
local startup_stream_max_attempts = trace_config.startup_stream_max_attempts or 640
local collision_event_limit = trace_config.collision_event_limit or 96
local collision_repeat_limit = trace_config.collision_repeat_limit or 3
local trace_event_counter = 0
local descriptor_census_done = false
local startup_stream_events = {}
local startup_stream_pending_hit = nil
local startup_previous_leaf_index = nil

local function arm_breakpoint(owner, segment, offset, options)
    return breakpoint_controller:arm(owner, segment, offset,
                                     options or {once = true})
end

local player_record_size = 0x78

local collision_offsets = {0x6484, 0x648e, 0x3a8a, 0x3986, 0x3a1f, 0x3df2}

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

local word = common.word
local dword = common.dword
local signed16 = common.signed16
local signed32 = common.signed32
local signed_word = common.signed16
local little_word = common.little_word
local hex = common.hex
local hex_differences = common.hex_differences
local numeric_differences = common.numeric_differences
local function selector_word(selector, offset)
    return common.selector_word(dosbox, selector, offset)
end

local function apply_player_map_patch()
    if map_patch_cell == nil then return nil end
    local x = map_patch_cell[1]
    local y = map_patch_cell[2]
    local tile = map_patch_cell[3] & 0x1ff
    local map_base = dosbox.mem_read_word("ds", 0x657a)
    local map_selector = dosbox.mem_read_word("ds", 0x657c)
    local row_stride = dosbox.mem_read_word("ds", 0x657e)
    local offset = map_base + y * row_stride + x * 2
    local original = selector_word(map_selector, offset)
    local patched = map_patch_word ~= nil and (map_patch_word & 0xffff) or
                    ((original & 0xfe00) | tile)
    tile = patched & 0x1ff
    dosbox.mem_write_selector(map_selector, offset,
                              string.char(patched & 0xff,
                                          (patched >> 8) & 0xff))
    local descriptor_patch = nil
    if map_patch_descriptor ~= nil then
        local descriptor_base = dosbox.mem_read_word("ds", 0x6582)
        local descriptor_selector = dosbox.mem_read_word("ds", 0x6584)
        local descriptor_stride = dosbox.mem_read_word("ds", 0x30d4)
        local descriptor_offset = descriptor_base + tile * descriptor_stride + 2
        local descriptor_original = selector_word(descriptor_selector,
                                                   descriptor_offset)
        dosbox.mem_write_selector(descriptor_selector, descriptor_offset,
                                  string.char(map_patch_descriptor & 0xff,
                                              (map_patch_descriptor >> 8) & 0xff))
        descriptor_patch = {
            selector = descriptor_selector,
            offset = descriptor_offset,
            original = descriptor_original,
            patched = map_patch_descriptor & 0xffff,
            readback = selector_word(descriptor_selector, descriptor_offset),
        }
    end
    return {
        x = x,
        y = y,
        tile_id = tile,
        selector = map_selector,
        offset = offset,
        original = original,
        patched = patched,
        readback = selector_word(map_selector, offset),
        descriptor = descriptor_patch,
    }
end

local function restore_player_map_patch(patch)
    if patch == nil then return end
    if patch.descriptor ~= nil then
        dosbox.mem_write_selector(patch.descriptor.selector,
                                  patch.descriptor.offset,
                                  string.char(patch.descriptor.original & 0xff,
                                              (patch.descriptor.original >> 8) & 0xff))
    end
    dosbox.mem_write_selector(patch.selector, patch.offset,
                              string.char(patch.original & 0xff,
                                          (patch.original >> 8) & 0xff))
end

local function wait_hit(label)
    local hit, err = dosbox.wait_for_breakpoint(timeout_ms)
    if not hit then error(label .. ": " .. (err or "timeout")) end
    hit.breakpoint_owners = breakpoint_controller:consume(hit.segment, hit.offset)
    return hit
end

local address_key = common.address_key

-- A return address is data read from the guest stack, not trusted tracer
-- input.  Reading one byte through the selector both checks that the
-- selector exists and that the offset is inside its mapped range before a
-- breakpoint is armed.  This is especially important after a helper entry:
-- a malformed far/near interpretation otherwise leaves the script waiting
-- on an address that can never execute.
local function validate_return_address(address)
    return common.validate_return_address(dosbox, address)
end

local function remember_unresolved_return(sample, helper_offset, address, reason)
    sample.unresolved_returns = sample.unresolved_returns or {}
    sample.unresolved_returns[#sample.unresolved_returns + 1] = {
        event_index = next_trace_event(),
        helper_offset = helper_offset,
        address = address,
        reason = reason,
    }
end

local function arm_validated_return(sample, address, helper_offset, label)
    local valid, reason = validate_return_address(address)
    if not valid then
        remember_unresolved_return(sample, helper_offset, address,
                                   label .. ": " .. reason)
        return false
    end
    local ok, added_or_error = pcall(
        breakpoint_controller.arm, breakpoint_controller, label,
        address.segment, address.offset, {once = true}
    )
    -- false is the debugger's "already armed" result, which is still safe:
    -- the validated address remains covered by that existing breakpoint.
    if not ok then
        remember_unresolved_return(
            sample, helper_offset, address,
            label .. ": breakpoint was not armed: " .. tostring(added_or_error)
        )
        return false
    end
    return true
end

local function object_snapshot(raw, selector, offset, index)
    local x_fixed = dword(raw, 3)
    local y_fixed = dword(raw, 7)
    local vertical_step = #raw >= 0x74 and word(raw, 0x72 + 1) or nil
    return {
        index = index,
        selector = selector,
        offset = offset,
        state_hex = (not minimal_callback_capture or capture_player_record or
                     parity_callback_capture) and hex(raw) or nil,
        position = {
            x_fixed = x_fixed,
            y_fixed = y_fixed,
            x_fixed_signed = signed32(x_fixed),
            y_fixed_signed = signed32(y_fixed),
            x = x_fixed >> 16,
            y = y_fixed >> 16,
        },
        action_word = word(raw, 1),
        velocity_x_fixed = dword(raw, 0x0a + 1),
        velocity_y_fixed = dword(raw, 0x0e + 1),
        velocity_x_fixed_signed = signed32(dword(raw, 0x0a + 1)),
        velocity_y_fixed_signed = signed32(dword(raw, 0x0e + 1)),
        kind = word(raw, 0x14 + 1),
        phase = string.byte(raw, 0x17 + 1),
        callback = word(raw, 0x18 + 1),
        callback_data = word(raw, 0x1a + 1),
        target_emitter_slot = word(raw, 0x2a + 1),
        sprite_slot = word(raw, 0x12 + 1),
        lifetime = word(raw, 0x2c + 1),
        state_field = word(raw, 0x2e + 1),
        state_field_signed = signed32(dword(raw, 0x2e + 1)),
        animation_reload = word(raw, 0x1e + 1),
        animation_delay = word(raw, 0x20 + 1),
        animation_sequence = word(raw, 0x22 + 1),
        animation_cursor = word(raw, 0x24 + 1),
        animation_mode = string.byte(raw, 0x28 + 1),
        animation_direction = string.byte(raw, 0x29 + 1),
        vertical_step = vertical_step,
        vertical_step_signed = vertical_step and signed16(vertical_step) or nil,
        target_cursor = word(raw, 0x30 + 1),
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

local function map_lookup_snapshot(hit, coordinate_x, coordinate_y)
    local registers = hit.registers or {}
    local y = coordinate_y ~= nil and coordinate_y or
              ((registers.eax or 0) & 0xffff)
    local x = coordinate_x ~= nil and coordinate_x or
              ((registers.ebx or 0) & 0xffff)
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
    -- 01F7:3986 receives its probe coordinates from the player record and
    -- then calls 01F7:1C92.  At a breakpoint on 3986, EAX/EBX are scratch
    -- values from the helper's preceding setup, not the original probe
    -- coordinates.  Reconstruct the exact static contract (Y - +0x72, X)
    -- from ES:DI so a negative-mode trace can prove whether the raw 0x1000
    -- contact bit was set at the actual probe cell.
    local coordinate_x = nil
    local coordinate_y = nil
    if hit.offset == 0x3986 and registers.es ~= nil and registers.edi ~= nil then
        local ok, raw = pcall(
            dosbox.mem_read_selector, registers.es,
            (registers.edi or 0) & 0xffff, player_record_size
        )
        if ok and raw ~= nil and #raw >= player_record_size then
            local x_fixed = signed32(dword(raw, 0x02 + 1))
            local y_fixed = signed32(dword(raw, 0x06 + 1))
            local vertical_step = word(raw, 0x72 + 1)
            coordinate_x = math.floor(x_fixed / 65536) & 0xffff
            coordinate_y = (math.floor(y_fixed / 65536) - vertical_step) & 0xffff
        end
    end
    local lookup = map_lookup_snapshot(hit, coordinate_x, coordinate_y)
    local tile_id = lookup.tile_id or 0
    local x = lookup.x
    local y = lookup.y
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
    local raw_map_mask = nil
    if hit.offset == 0x1c6e then
        raw_map_mask = 0x4000
    elseif hit.offset == 0x1c92 or hit.offset == 0x3986 then
        raw_map_mask = 0x1000
    end
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
        coordinate_source = hit.offset == 0x3986 and
            (coordinate_x ~= nil and "player_record_plus_0x72" or
             "registers_unavailable") or "helper_registers",
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
        raw_map_mask = raw_map_mask,
        raw_map_bit_set = raw_map_mask ~= nil and lookup.cell_word ~= nil and
            ((lookup.cell_word & raw_map_mask) ~= 0),
        descriptor_read_error = descriptor_read_error,
    }
end

local function branch_descriptor_snapshot(hit)
    local lookup = map_lookup_snapshot(hit)
    local tile_id = lookup.tile_id
    if tile_id == nil then return {map_lookup = lookup} end
    local base = dosbox.mem_read_word("ds", 0x6582)
    local selector = dosbox.mem_read_word("ds", 0x6584)
    local stride = dosbox.mem_read_word("ds", 0x30d4)
    local record_offset = base + tile_id * stride
    local ok, raw_or_error = pcall(
        dosbox.mem_read_selector, selector, record_offset, 4
    )
    if not ok or not raw_or_error or #raw_or_error < 4 then
        return {
            map_lookup = lookup,
            descriptor_selector = selector,
            descriptor_offset = record_offset,
            descriptor_read_error = ok and "short descriptor record" or tostring(raw_or_error),
        }
    end
    return {
        map_lookup = lookup,
        descriptor_selector = selector,
        descriptor_base = base,
        descriptor_stride = stride,
        descriptor_offset = record_offset,
        tile_id = tile_id,
        tile_index = word(raw_or_error, 1),
        descriptor_word = word(raw_or_error, 3),
    }
end

local function static_globals()
    local target_capacity = dosbox.mem_read_word("ds", 0x8808)
    local raw_targets = dosbox.mem_read("ds", 0x87de, 40) or ""
    local target_entries = {}
    for index = 0, 9 do
        local base = index * 4 + 1
        if #raw_targets >= base + 3 then
            target_entries[#target_entries + 1] = {
                index = index,
                x = signed_word(word(raw_targets, base)),
                y = signed_word(word(raw_targets, base + 2)),
            }
        end
    end
    return {
        -- 01F7:5937 dispatch/auxiliary aliases.  Keep these in the ordinary
        -- callback snapshot so a callback-scoped DS:60D8 experiment can
        -- distinguish the dispatcher's own writes from later movement.
        dispatch_gate_85da = dosbox.mem_read_byte("ds", 0x85da),
        dispatch_word_60d8 = dosbox.mem_read_word("ds", 0x60d8),
        dispatch_previous_word_60da = dosbox.mem_read_word("ds", 0x60da),
        dispatch_score_low_881c = dosbox.mem_read_word("ds", 0x881c),
        dispatch_score_high_881e = dosbox.mem_read_word("ds", 0x881e),
        dispatch_lives_880a = dosbox.mem_read_word("ds", 0x880a),
        dispatch_ammo_880c = dosbox.mem_read_word("ds", 0x880c),
        dispatch_health_8822 = dosbox.mem_read_word("ds", 0x8822),
        -- 5937 compares the dword at 4FF2 against the score dword at
        -- 881C. Preserve both words here instead of truncating the replay
        -- input to the low word.
        dispatch_aux_4ff2 = dword(dosbox.mem_read("ds", 0x4ff2, 4) or
                                  "\0\0\0\0", 1),
        dispatch_aux_4ff6 = dosbox.mem_read_word("ds", 0x4ff6),
        dispatch_aux_4ff8 = dosbox.mem_read_word("ds", 0x4ff8),
        dispatch_aux_4ffa = dosbox.mem_read_word("ds", 0x4ffa),
        dispatch_view_page_817c = dosbox.mem_read_word("ds", 0x817c),
        dispatch_view_state_81a6 = dosbox.mem_read_word("ds", 0x81a6),
        dispatch_view_state_81aa = dosbox.mem_read_word("ds", 0x81aa),
        dispatch_view_state_81be = dosbox.mem_read_word("ds", 0x81be),
        dispatch_view_state_81c2 = dosbox.mem_read_word("ds", 0x81c2),
        input_action_flags = dosbox.mem_read_word("ds", 0x8196),
        keyboard_action_flags = dosbox.mem_read_word("ds", 0x88bc),
        last_scan_code = dosbox.mem_read_word("ds", 0x88ba),
        camera_x = dosbox.mem_read_word("ds", 0x81c0),
        camera_y = dosbox.mem_read_word("ds", 0x81c4),
        -- 01F7:1CDA's 64-pixel stream cursor and 1E04's current ARE
        -- declaration origin. These are read-only startup diagnostics: the
        -- cursor explains which newly-visible strip caused a pool entry,
        -- while the origins identify the region used for its coordinates.
        are_stream_cursor_x_3710 = dosbox.mem_read_word("ds", 0x3710),
        are_stream_cursor_y_3712 = dosbox.mem_read_word("ds", 0x3712),
        are_region_origin_x_3714 = dosbox.mem_read_word("ds", 0x3714),
        are_region_origin_y_3716 = dosbox.mem_read_word("ds", 0x3716),
        map_row_stride = dosbox.mem_read_word("ds", 0x657e),
        -- 01D7:34C7 publishes this interleaved recovery table and
        -- 01F7:1AAA consumes the row selected by DS:85D2.  Keep the raw
        -- words at lifecycle breakpoints so a recovery trace can distinguish
        -- the resource-selected spawn from the ordinary level-start spawn.
        recovery_selector_85d4 = dosbox.mem_read_word("ds", 0x85d4),
        recovery_row_85d2 = dosbox.mem_read_word("ds", 0x85d2),
        recovery_resource_97e2 = dosbox.mem_read_word("ds", 0x97e2),
        recovery_spawn_row0_x = dosbox.mem_read_word("ds", 0x8828),
        recovery_spawn_row0_y = dosbox.mem_read_word("ds", 0x882a),
        recovery_spawn_selected_x = dosbox.mem_read_word("ds", 0x8828 +
                                                         4 * dosbox.mem_read_word("ds", 0x85d2)),
        recovery_spawn_selected_y = dosbox.mem_read_word("ds", 0x882a +
                                                         4 * dosbox.mem_read_word("ds", 0x85d2)),
        object_list_cursor = dosbox.mem_read_word("ds", 0x36e0),
        player_object_offset = dosbox.mem_read_word("ds", 0x881a),
        player_control_word = dosbox.mem_read_word("ds", 0x89ea),
        player_transition_word = dosbox.mem_read_word("ds", 0x89e6),
        player_vertical_adjust = dosbox.mem_read_word("ds", 0x8812),
        -- Complete callback-global inputs used by the replay manifest.  Keep
        -- the fixed-point carry as a signed dword and the signed mode words
        -- in their two's-complement form; the C++ replay boundary otherwise
        -- has to guess values that are present in the guest at every sample.
        external_x_delta = signed32(dword(dosbox.mem_read("ds", 0x8816, 4) or "\0\0\0\0", 1)),
        timer_clear = dosbox.mem_read_word("ds", 0x8810),
        view_state_a = signed_word(dosbox.mem_read_word("ds", 0x4fe4)),
        pending_event = dosbox.mem_read_word("ds", 0x612e),
        camera_y_limit = dosbox.mem_read_word("ds", 0x81cc),
        action_source = dosbox.mem_read_word("ds", 0x656c),
        activation_state = signed_word(dosbox.mem_read_word("ds", 0x85da)),
        speed_cap_mode = dosbox.mem_read_word("ds", 0x88b6),
        action_suppressor = signed_word(dosbox.mem_read_word("ds", 0x89e6)),
        transition_mode = signed_word(dosbox.mem_read_word("ds", 0x89ea)),
        player_reset_state = dosbox.mem_read_word("ds", 0x8810),
        horizontal_limit = dosbox.mem_read_word("ds", 0x4fe2),
        horizontal_aux = dosbox.mem_read_word("ds", 0x4fe6),
        horizontal_accumulator = dosbox.mem_read_word("ds", 0x4fe8),
        horizontal_branch_counter = dosbox.mem_read_word("ds", 0x4fec),
        horizontal_timer = dosbox.mem_read_word("ds", 0x4fee),
        horizontal_result_byte = string.byte(
            dosbox.mem_read("ds", 0x4ff0, 1) or "\0", 1
        ),
        transition_flags = dosbox.mem_read_word("ds", 0x8822),
        transition_state = dosbox.mem_read_word("ds", 0x8810),
        object_count = dosbox.mem_read_word("ds", 0x880a),
        transition_event = dosbox.mem_read_word("ds", 0x89e6),
        -- 01F7:4727/47E7 consume the byte ring at DS:646C through the
        -- wrapping index DS:6468. Keep both values in the trace so leaf
        -- velocity/animation initialization can be replayed without
        -- guessing a random seed.
        leaf_prng_index = dosbox.mem_read_word("ds", 0x6468),
        leaf_prng_ring_hex = hex(dosbox.mem_read("ds", 0x646c, 0x100) or ""),
        transition_error = dosbox.mem_read_word("ds", 0x89ec),
        target_active_count = dosbox.mem_read_word("ds", 0x8806),
        target_capacity = target_capacity,
        target_entries = target_entries,
    }
end

-- This is deliberately narrower than static_globals().  It contains every
-- global named by player_replay_manifest.py, including the 5937 dispatch
-- inputs, but omits the PRNG ring, target ring, and startup-only diagnostics.
-- The explicit mode keeps long natural routes within the Lua instruction
-- budget without silently dropping a value that can affect callback replay.
local function parity_globals_snapshot()
    return {
        dispatch_gate_85da = dosbox.mem_read_byte("ds", 0x85da),
        dispatch_word_60d8 = dosbox.mem_read_word("ds", 0x60d8),
        dispatch_previous_word_60da = dosbox.mem_read_word("ds", 0x60da),
        dispatch_score_low_881c = dosbox.mem_read_word("ds", 0x881c),
        dispatch_score_high_881e = dosbox.mem_read_word("ds", 0x881e),
        dispatch_lives_880a = dosbox.mem_read_word("ds", 0x880a),
        dispatch_ammo_880c = dosbox.mem_read_word("ds", 0x880c),
        dispatch_health_8822 = dosbox.mem_read_word("ds", 0x8822),
        dispatch_aux_4ff2 = dword(dosbox.mem_read("ds", 0x4ff2, 4) or
                                  "\0\0\0\0", 1),
        dispatch_aux_4ff6 = dosbox.mem_read_word("ds", 0x4ff6),
        dispatch_aux_4ff8 = dosbox.mem_read_word("ds", 0x4ff8),
        dispatch_aux_4ffa = dosbox.mem_read_word("ds", 0x4ffa),
        input_action_flags = dosbox.mem_read_word("ds", 0x8196),
        keyboard_action_flags = dosbox.mem_read_word("ds", 0x88bc),
        external_x_delta = signed32(dword(
            dosbox.mem_read("ds", 0x8816, 4) or "\0\0\0\0", 1)),
        timer_clear = dosbox.mem_read_word("ds", 0x8810),
        horizontal_limit = dosbox.mem_read_word("ds", 0x4fe2),
        horizontal_aux = dosbox.mem_read_word("ds", 0x4fe6),
        horizontal_accumulator = dosbox.mem_read_word("ds", 0x4fe8),
        horizontal_branch_counter = dosbox.mem_read_word("ds", 0x4fec),
        horizontal_timer = dosbox.mem_read_word("ds", 0x4fee),
        horizontal_result_byte = string.byte(
            dosbox.mem_read("ds", 0x4ff0, 1) or "\0", 1),
        view_state_a = signed_word(dosbox.mem_read_word("ds", 0x4fe4)),
        pending_event = dosbox.mem_read_word("ds", 0x612e),
        camera_x = dosbox.mem_read_word("ds", 0x81c0),
        camera_y = dosbox.mem_read_word("ds", 0x81c4),
        camera_y_limit = dosbox.mem_read_word("ds", 0x81cc),
        player_vertical_adjust = dosbox.mem_read_word("ds", 0x8812),
        action_source = dosbox.mem_read_word("ds", 0x656c),
        activation_state = signed_word(dosbox.mem_read_word("ds", 0x85da)),
        speed_cap_mode = dosbox.mem_read_word("ds", 0x88b6),
        action_suppressor = signed_word(dosbox.mem_read_word("ds", 0x89e6)),
        transition_mode = signed_word(dosbox.mem_read_word("ds", 0x89ea)),
    }
end

local function trace_globals()
    if parity_callback_capture then return parity_globals_snapshot() end
    if minimal_callback_capture then return nil end
    return static_globals()
end

local function callback_globals()
    if parity_callback_capture then return parity_globals_snapshot() end
    if minimal_callback_capture then return {} end
    return static_globals()
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

local function startup_stream_snapshot()
    -- Keep this intentionally smaller than static_globals(): the startup
    -- scan can execute hundreds of 1CDA calls before the first player
    -- callback, and only these words can explain its strip selection.
    return {
        camera_x = dosbox.mem_read_word("ds", 0x81c0),
        camera_y = dosbox.mem_read_word("ds", 0x81c4),
        stream_cursor_x = dosbox.mem_read_word("ds", 0x3710),
        stream_cursor_y = dosbox.mem_read_word("ds", 0x3712),
        region_origin_x = dosbox.mem_read_word("ds", 0x3714),
        region_origin_y = dosbox.mem_read_word("ds", 0x3716),
        leaf_prng_index = dosbox.mem_read_word("ds", 0x6468),
    }
end

local function arm_callback_targets()
    for _, segment in ipairs({0x01d7, 0x01e7, 0x01f7, 0x0207, 0x0227, 0x0237,
                              0x1997}) do
        arm_breakpoint("callback", segment, focus_callback_offset)
    end
    if probe_release_emitter then
        arm_breakpoint("release-emitter", 0x01f7, 0x470c)
    end
end

local function release_callback_targets(callback_offset)
    for _, segment in ipairs({0x01d7, 0x01e7, 0x01f7, 0x0207, 0x0227, 0x0237,
                              0x1997}) do
        breakpoint_controller:release("callback", segment, callback_offset)
    end
end

local function arm_property_targets(blocked)
    if property_helper_offset == 0x1c6e or property_helper_offset == 0x1c92 or
       property_helper_offset == 0x5c27 or property_helper_offset == 0x5cc3 then
        local key = address_key({segment = 0x01f7,
                                 offset = property_helper_offset})
        if not blocked or not blocked[key] then
            arm_breakpoint("property", 0x01f7, property_helper_offset)
        end
    else
        for _, offset in ipairs({0x1c6e, 0x1c92, 0x5c27, 0x5cc3}) do
            local key = address_key({segment = 0x01f7, offset = offset})
            if not blocked or not blocked[key] then
                arm_breakpoint("property", 0x01f7, offset)
            end
        end
    end
end

-- A helper breakpoint can fire before the requested player callback when
-- another object is being updated.  Once that happens, leave the helper
-- breakpoints suspended until the callback barrier is reached.  The inner
-- callback loop re-arms them and records only helper calls that occur in the
-- player closure.  Without this separation a repeated property/collision
-- hit can produce a property-only sample and prevent the complete 0x78
-- callback record from being captured.
local function suspend_side_probe_targets()
    if property_focus then
        for _, offset in ipairs({0x1c6e, 0x1c92, 0x5c27, 0x5cc3}) do
            breakpoint_controller:release("property", 0x01f7, offset)
        end
    end
    if collision_focus then
        for _, offset in ipairs(collision_offsets) do
            breakpoint_controller:release("collision", 0x01f7, offset)
        end
    end
    if map_focus then
        breakpoint_controller:release("map", 0x01f7, 0x3376)
    end
end

local branch_entry_offset = 0x3d02
local branch_offsets = {0x3d1e, 0x3d36, 0x3d40, 0x3d45, 0x3dd0,
                        0x3de4, 0x3d44, 0x3df1}

local function arm_branch_targets(exclude_offset)
    for _, offset in ipairs(branch_offsets) do
        if offset ~= exclude_offset then
            arm_breakpoint("branch", 0x01f7, offset)
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
    breakpoint_controller:release("branch", 0x01f7, branch_entry_offset)
    for _, offset in ipairs(branch_offsets) do
        breakpoint_controller:release("branch", 0x01f7, offset)
    end
end

local callback_object_snapshot

local function patch_branch_probe_cell(sample, hit)
    if not branch_patch_tile and branch_patch_flags == nil then return nil end
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
    local patched = original
    if branch_patch_tile then
        patched = (patched & 0xfe00) | (branch_patch_tile & 0x1ff)
        dosbox.mem_write_selector(map_selector, offset,
                                  string.char(patched & 0xff,
                                              (patched >> 8) & 0xff))
    end
    local readback = selector_word(map_selector, offset)
    local patch = {
        selector = map_selector,
        offset = offset,
        x = x,
        y = y,
        original = original,
        tile_id = readback & 0x1ff,
        patched = patched,
        readback = readback,
    }
    if branch_patch_flags ~= nil then
        local descriptor_base = dosbox.mem_read_word("ds", 0x6582)
        local descriptor_selector = dosbox.mem_read_word("ds", 0x6584)
        local descriptor_stride = dosbox.mem_read_word("ds", 0x30d4)
        local descriptor_offset = descriptor_base + (readback & 0x1ff) *
                                  descriptor_stride + 2
        local descriptor_original = selector_word(descriptor_selector,
                                                   descriptor_offset)
        local descriptor_patched = branch_patch_flags & 0xffff
        dosbox.mem_write_selector(descriptor_selector, descriptor_offset,
                                  string.char(descriptor_patched & 0xff,
                                              (descriptor_patched >> 8) & 0xff))
        patch.descriptor = {
            base = descriptor_base,
            selector = descriptor_selector,
            stride = descriptor_stride,
            tile_id = readback & 0x1ff,
            offset = descriptor_offset,
            original = descriptor_original,
            patched = descriptor_patched,
            readback = selector_word(descriptor_selector, descriptor_offset),
        }
    end
    sample.branch_patch = patch
    return patch
end

local function restore_branch_probe_cell(patch)
    if patch == nil then return end
    if patch.descriptor ~= nil then
        local descriptor = patch.descriptor
        dosbox.mem_write_selector(descriptor.selector, descriptor.offset,
                                  string.char(descriptor.original & 0xff,
                                              (descriptor.original >> 8) & 0xff))
    end
    dosbox.mem_write_selector(patch.selector, patch.offset,
                              string.char(patch.original & 0xff,
                                          (patch.original >> 8) & 0xff))
end

local function is_property_target(offset)
    return property_focus and (offset == 0x1c6e or offset == 0x1c92 or
                               offset == 0x5c27 or offset == 0x5cc3) and
           (property_helper_offset == 0 or offset == property_helper_offset)
end

local function arm_targets()
    if factory_focus then
        arm_breakpoint("factory-entry", 0x01f7, 0x0e06)
        return
    end
    if focus_callback then
        arm_callback_targets()
    end
    if map_focus then
        arm_breakpoint("map", 0x01f7, 0x3376)
    end
    if collision_focus then
        for _, offset in ipairs(collision_offsets) do
            arm_breakpoint("collision", 0x01f7, offset)
        end
    end
    if property_focus then
        arm_property_targets()
    end
    if descriptor_census and not descriptor_census_done then
        arm_breakpoint("descriptor-census", 0x01f7, 0x5cc3)
    end
    if branch_focus then
        arm_breakpoint("branch", 0x01f7, branch_entry_offset)
        arm_branch_targets()
    end
    if probe_spawn_emitter and not spawn_probe_done then
        -- 38EC is the post-update producer call site. The player callback
        -- rewrites object+0 before reaching it, so 3FF8 is too early.
        arm_breakpoint("spawn-emitter", 0x01f7, 0x38ec)
    end
    -- Execute watches are global code addresses shared by many object
    -- callbacks.  When the player callback is focused, arm them only after
    -- the player entry barrier so a different object's 5D60 does not get
    -- attributed to the player sample.
    if not focus_callback then
        patch_watch.arm_execute_watches(breakpoint_controller, execute_watches)
    end
    if focus_callback or map_focus or collision_focus or property_focus or branch_focus or
       (descriptor_census and not descriptor_census_done) then
        return
    end
    -- A global execute-watch may be hit only once (for example 4BA4 at the
    -- start of the death/recovery branch).  Keep 3FF8 as the ordinary sample
    -- barrier in watch-only mode so the trace can advance after that hit and
    -- cannot wait forever for another lifecycle address.
    if #execute_watches > 0 then
        arm_breakpoint("execute-watch-sample", 0x01f7, 0x3ff8)
        return
    end
    arm_breakpoint("default-pool", 0x01f7, 0x0e96)
    arm_breakpoint("default-kind-scan", 0x01f7, 0x0f3c)
    arm_breakpoint("default-map", 0x01f7, 0x3376)
    arm_breakpoint("default-callback", 0x01f7, 0x3f27)
end

callback_object_snapshot = function(hit)
    local registers = hit.registers or {}
    local selector = registers.es
    local offset = (registers.edi or 0) & 0xffff
    if selector == nil then return nil end
    local read_size = capture_player_record and player_record_size or 0x40
    local ok, raw_or_error = pcall(
        dosbox.mem_read_selector, selector, offset, read_size
    )
    if not ok or not raw_or_error or #raw_or_error < read_size then
        return {
            selector = selector,
            offset = offset,
            state_size = read_size,
            read_error = ok and "short object state" or tostring(raw_or_error),
        }
    end
    local object = object_snapshot(raw_or_error, selector, offset, -1)
    object.state_size = read_size
    return object
end

local function apply_spawn_probe(hit)
    if not probe_spawn_emitter or spawn_probe_done or hit.offset ~= 0x38ec then return nil end
    spawn_probe_done = true
    local object = callback_object_snapshot(hit)
    if object == nil then return nil end
    dosbox.mem_write_selector(object.selector, object.offset,
                               little_word(object.action_word | 0x10))
    dosbox.mem_write_selector(object.selector, object.offset + 0x3c, "\x00")
    -- 4519 admits the emitter only while this gate is positive (or DS:880C
    -- is positive). This is debugger-only setup of the observed call path.
    dosbox.mem_write("ds", 0x88ae, little_word(1))
    return {
        player_action_word = object.action_word | 0x10,
        player_byte_3c = 0,
        spawn_gate_88ae = 1,
    }
end

local function apply_release_probe(hit)
    if not probe_release_emitter or hit.offset ~= 0x45ab then return nil end
    local object = callback_object_snapshot(hit)
    if object == nil then return nil end
    local slot = object.target_emitter_slot
    dosbox.mem_write("ds", 0x87de + slot, little_word(0))
    arm_breakpoint("release-emitter", 0x01f7, 0x470c)
    return {
        target_slot = slot,
        target_x = 0,
    }
end

local function capture_release_callback(sample)
    dosbox.debug_continue()
    local hit = wait_hit("target-emitter release callback")
    sample.related_breakpoints[#sample.related_breakpoints + 1] = {
        segment = hit.segment, offset = hit.offset, owners = hit.breakpoint_owners,
    }
    local object = callback_object_snapshot(hit)
    local release = {
        breakpoint = {segment = hit.segment, offset = hit.offset},
        callback_offset = hit.offset,
        registers = hit.registers,
        object = object,
    }
    local stack = dosbox.mem_read(
        "ss", (hit.registers.esp or 0) & 0xffff, 4
    ) or ""
    if #stack >= 4 and object ~= nil then
        local return_offset = word(stack, 1)
        release.return_expected = {segment = hit.segment, offset = return_offset}
        arm_breakpoint("release-return", hit.segment, return_offset)
        dosbox.debug_continue()
        local returned = wait_hit("target-emitter release return")
        release.return_actual = {segment = returned.segment, offset = returned.offset}
        local ok, raw_or_error = pcall(
            dosbox.mem_read_selector, object.selector, object.offset, 0x40
        )
        if ok and raw_or_error and #raw_or_error >= 0x40 then
            release.post_object = object_snapshot(
                raw_or_error, object.selector, object.offset, -1
            )
            release.post_globals = callback_globals()
        else
            release.post_object_read_error =
                ok and "short object state" or tostring(raw_or_error)
        end
    end
    sample.release_callback = release
end

local function record_map_lookup(sample, hit)
    local lookup = map_lookup_snapshot(hit)
    lookup.breakpoint = {segment = hit.segment, offset = hit.offset}
    lookup.registers = hit.registers
    if collision_patch_tile ~= nil then
        local player_x = nil
        local globals = sample.globals or {}
        local pool = sample.pool or {}
        local target_offset = globals.player_object_offset
        for _, candidate in ipairs(pool.objects or {}) do
            if candidate.offset == target_offset and candidate.position then
                player_x = candidate.position.x
                break
            end
        end
        if player_x == nil then
            local object = callback_object_snapshot(hit)
            player_x = object and object.position and object.position.x
        end
        local side = nil
        if type(player_x) == "number" then
            if lookup.x < player_x then side = "left"
            elseif lookup.x > player_x then side = "right"
            else side = "center" end
        end
        local side_allowed = collision_patch_side == "both" or side == collision_patch_side
        if side_allowed and side ~= "center" and lookup.cell_word ~= nil then
            local original = lookup.cell_word
            local patched = (original & 0xfe00) | (collision_patch_tile & 0x1ff)
            dosbox.mem_write_selector(
                lookup.map_selector, lookup.cell_offset,
                string.char(patched & 0xff, (patched >> 8) & 0xff)
            )
            local descriptor_base = dosbox.mem_read_word("ds", 0x6582)
            local descriptor_selector = dosbox.mem_read_word("ds", 0x6584)
            local descriptor_stride = dosbox.mem_read_word("ds", 0x30d4)
            local descriptor_offset = descriptor_base +
                (collision_patch_tile & 0x1ff) * descriptor_stride + 2
            local d_ok, descriptor_word = pcall(
                selector_word, descriptor_selector, descriptor_offset
            )
            lookup.patch = {
                side = side,
                x = lookup.x,
                y = lookup.y,
                selector = lookup.map_selector,
                offset = lookup.cell_offset,
                original = original,
                patched = patched,
                readback = selector_word(lookup.map_selector, lookup.cell_offset),
                tile_id = collision_patch_tile & 0x1ff,
                descriptor_word = d_ok and descriptor_word or nil,
            }
        end
    end
    sample.map_lookups = sample.map_lookups or {}
    sample.map_lookups[#sample.map_lookups + 1] = lookup
    sample.map_lookup = lookup
    return lookup
end

local function restore_map_patch(lookup)
    if lookup == nil or lookup.patch == nil then return end
    local patch = lookup.patch
    dosbox.mem_write_selector(
        patch.selector, patch.offset,
        string.char(patch.original & 0xff, (patch.original >> 8) & 0xff)
    )
end

local function arm_map_target(blocked)
    if map_focus then
        local key = address_key({segment = 0x01f7, offset = 0x3376})
        if not blocked or not blocked[key] then
            arm_breakpoint("map", 0x01f7, 0x3376)
        end
    end
end

local function record_property(sample, hit)
    local property = map_property_snapshot(hit)
    -- Property focus is armed alongside the callback barrier and may observe
    -- another object's helper before 3FF8 is reached. Keep that diagnostic
    -- event in the trace, but distinguish it from property calls made by the
    -- player callback so parity can compare the callback-owned stream only.
    property.scope = sample.player_callback ~= nil and
                     "player_callback" or "outside_player_callback"
    if collision_patch_tile ~= nil and hit.offset == 0x5c27 and
       (not collision_focus or collision_patch_side == "left") and
       sample.player_callback ~= nil then
        local player_x = nil
        local globals = sample.globals or {}
        local pool = sample.pool or {}
        local target_offset = globals.player_object_offset
        for _, candidate in ipairs(pool.objects or {}) do
            if candidate.offset == target_offset and candidate.position then
                player_x = candidate.position.x
                break
            end
        end
        local side = nil
        if type(player_x) == "number" then
            if property.coordinates.x < player_x then side = "left"
            elseif property.coordinates.x > player_x then side = "right"
            else side = "center" end
        end
        local side_allowed = collision_patch_side == "both" or side == collision_patch_side
        if side_allowed and side ~= "center" and property.raw_cell_word ~= nil then
            local original = property.raw_cell_word
            local patched = (original & 0xfe00) | (collision_patch_tile & 0x1ff)
            dosbox.mem_write_selector(
                property.map_lookup.map_selector, property.map_lookup.cell_offset,
                string.char(patched & 0xff, (patched >> 8) & 0xff)
            )
            local descriptor_base = dosbox.mem_read_word("ds", 0x6582)
            local descriptor_selector = dosbox.mem_read_word("ds", 0x6584)
            local descriptor_stride = dosbox.mem_read_word("ds", 0x30d4)
            local descriptor_offset = descriptor_base +
                (collision_patch_tile & 0x1ff) * descriptor_stride + 2
            local d_ok, descriptor_word = pcall(
                selector_word, descriptor_selector, descriptor_offset
            )
            local patch = {
                side = side,
                x = property.coordinates.x,
                y = property.coordinates.y,
                selector = property.map_lookup.map_selector,
                offset = property.map_lookup.cell_offset,
                original = original,
                patched = patched,
                readback = selector_word(property.map_lookup.map_selector,
                                         property.map_lookup.cell_offset),
                tile_id = collision_patch_tile & 0x1ff,
                descriptor_word = d_ok and descriptor_word or nil,
            }
            property = map_property_snapshot(hit)
            property.patch = patch
        end
    end
    sample.map_properties = sample.map_properties or {}
    sample.map_properties[#sample.map_properties + 1] = property
    if sample.map_property == nil then sample.map_property = property end
    return property
end

local function restore_property_patch(property)
    if property == nil or property.patch == nil then return end
    local patch = property.patch
    dosbox.mem_write_selector(
        patch.selector, patch.offset,
        string.char(patch.original & 0xff, (patch.original >> 8) & 0xff)
    )
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
        globals = trace_globals(),
    }
    sample.branch_events = sample.branch_events or {}
    if hit.offset == branch_entry_offset then
        event.descriptor_lookup = branch_descriptor_snapshot(hit)
    end
    sample.branch_events[#sample.branch_events + 1] = event
    sample.branch_event = event
end

local function capture_branch_sequence(sample, initial_hit)
    local patch = patch_branch_probe_cell(sample, initial_hit)
    local hit = initial_hit
    local event_count = 0
    local last_breakpoint = nil
    local repeated_breakpoint_count = 0
    while true do
        if not is_branch_target(hit.offset) then
            error(string.format("unexpected collision branch breakpoint 0x%04x", hit.offset))
        end
        sample.related_breakpoints[#sample.related_breakpoints + 1] = {
            segment = hit.segment, offset = hit.offset, owners = hit.breakpoint_owners,
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
        event_count = event_count + 1
        local key = address_key({segment = hit.segment, offset = hit.offset})
        if key == last_breakpoint then
            repeated_breakpoint_count = repeated_breakpoint_count + 1
        else
            last_breakpoint = key
            repeated_breakpoint_count = 1
        end
        if repeated_breakpoint_count > collision_repeat_limit or
           event_count >= collision_event_limit then
            sample.branch_trace_guard = {
                event_count = event_count,
                repeated_breakpoint = {
                    segment = hit.segment, offset = hit.offset,
                },
                repeat_count = repeated_breakpoint_count,
                reason = repeated_breakpoint_count > collision_repeat_limit and
                    "repeated breakpoint" or "event-count limit",
            }
            clear_branch_targets()
            restore_branch_probe_cell(patch)
            return hit
        end
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
    if #raw < 4 then return nil, "far return stack read was truncated" end
    return {offset = word(raw, 1), segment = word(raw, 3)}
end

-- 3A1F and 3DF2 are ordinary near-call helpers in segment 3.  Reading a
-- four-byte far return for them would pair the two-byte return IP with the
-- caller's stack data and leave the callback barrier armed at a bogus CS.
local function collision_return_location(hit)
    if hit.offset ~= 0x3986 and hit.offset ~= 0x3a1f and
       hit.offset ~= 0x3df2 then
        return far_return_location(hit)
    end
    local registers = hit.registers or {}
    local raw = dosbox.mem_read(
        "ss", (registers.esp or 0) & 0xffff, 2
    ) or ""
    if #raw < 2 then return nil, "near return stack read was truncated" end
    return {offset = word(raw, 1), segment = hit.segment}
end

local function validated_collision_return(sample, hit)
    local address, reason = collision_return_location(hit)
    if address == nil then
        remember_unresolved_return(sample, hit.offset, address,
                                   reason or "collision return was not readable")
        return nil
    end
    local valid, validation_reason = validate_return_address(address)
    if not valid then
        remember_unresolved_return(sample, hit.offset, address,
                                   validation_reason or
                                   "collision return failed validation")
        return nil
    end
    return address
end

local function record_collision(sample, hit)
    local collision = {
        event_index = next_trace_event(),
        frame_index = sample.frame_index,
        helper_offset = hit.offset,
        breakpoint = {segment = hit.segment, offset = hit.offset},
        registers = hit.registers,
        object = callback_object_snapshot(hit),
        globals = trace_globals(),
    }
    if hit.offset == 0x3986 then
        local code = dosbox.mem_read_selector(hit.segment, hit.offset, 0x18)
        if code ~= nil then collision.code_hex = hex(code) end
    end
    -- 3DF2 is the leaf that passes the current world probe through the MAP
    -- descriptor path.  Read the live cell here while its AX/BX arguments are
    -- still intact; this avoids needing a second breakpoint mode and keeps
    -- the tile/descriptor evidence attached to the helper event.
    if hit.offset == 0x3df2 and not map_focus and
       (not property_focus or collision_patch_side ~= "left") then
        if collision_patch_tile ~= nil then
            local lookup = map_lookup_snapshot(hit)
            local sides = {}
            if collision_patch_side == "left" or collision_patch_side == "both" then
                sides[#sides + 1] = {name = "left", x = lookup.x}
            end
            if collision_patch_side == "right" or collision_patch_side == "both" then
                sides[#sides + 1] = {name = "right", x = lookup.x + 10}
            end
            local patches = {}
            for _, side in ipairs(sides) do
                local offset = lookup.map_base + (lookup.y >> 4) * lookup.row_stride +
                               ((side.x >> 3) & 0xfffe)
                local ok, original = pcall(selector_word, lookup.map_selector, offset)
                if ok then
                    local patched = (original & 0xfe00) | (collision_patch_tile & 0x1ff)
                    dosbox.mem_write_selector(
                        lookup.map_selector, offset,
                        string.char(patched & 0xff, (patched >> 8) & 0xff)
                    )
                    local descriptor_base = dosbox.mem_read_word("ds", 0x6582)
                    local descriptor_selector = dosbox.mem_read_word("ds", 0x6584)
                    local descriptor_stride = dosbox.mem_read_word("ds", 0x30d4)
                    local descriptor_offset = descriptor_base +
                        (collision_patch_tile & 0x1ff) * descriptor_stride + 2
                    local d_ok, descriptor_word = pcall(
                        selector_word, descriptor_selector, descriptor_offset
                    )
                    patches[#patches + 1] = {
                        side = side.name,
                        x = side.x,
                        y = lookup.y,
                        selector = lookup.map_selector,
                        offset = offset,
                        original = original,
                        patched = patched,
                        readback = selector_word(lookup.map_selector, offset),
                        tile_id = collision_patch_tile & 0x1ff,
                        descriptor_word = d_ok and descriptor_word or nil,
                    }
                end
            end
            if #patches > 0 then
                collision.patches = patches
                if #patches == 1 then collision.patch = patches[1] end
            end
        end
        collision.map_property = map_property_snapshot(hit)
    end
    sample.collisions = sample.collisions or {}
    sample.collisions[#sample.collisions + 1] = collision
    sample.collision = collision
    return collision
end

local function external_dispatch_snapshot(hit)
    if hit.offset ~= 0x0442 then return nil end
    local registers = hit.registers or {}
    -- 386F pushes AX, BX, CX before CALLF 0442.  At the 0442 entry
    -- breakpoint the callee prologue has not run yet, so the three words are
    -- at SS:(SP+4), SS:(SP+6), and SS:(SP+8) after the far return address.
    -- Once 0442 establishes BP, these are [BP+0x06], [BP+0x08], and
    -- [BP+0x0A].  The first word is therefore the actual selector consumed
    -- by 0442; EBX is merely the middle caller argument.
    local stack_args = nil
    if registers.ss ~= nil and registers.esp ~= nil then
        local stack_base = (registers.esp & 0xffff) + 4
        local ok_stack, stack_raw = pcall(
            dosbox.mem_read_selector, registers.ss, stack_base, 6
        )
        if ok_stack and stack_raw ~= nil and #stack_raw >= 6 then
            stack_args = {
                bp_06 = word(stack_raw, 1),
                bp_08 = word(stack_raw, 3),
                bp_0a = word(stack_raw, 5),
                stack_pointer = registers.esp & 0xffff,
                stack_selector = registers.ss,
            }
        end
    end
    local selector_raw = dosbox.mem_read("ds", 0x6d8a, 4) or ""
    local table_offset = #selector_raw >= 4 and word(selector_raw, 1) or nil
    local table_selector = #selector_raw >= 4 and word(selector_raw, 3) or nil
    local selector_index = stack_args and stack_args.bp_06 or
                           ((registers.ebx or 0) & 0xffff)
    local index_offset = 0x6d8e + selector_index * 2
    local ok_index, table_index = pcall(dosbox.mem_read_word, "ds", index_offset)
    if not ok_index then table_index = nil end
    local ok_stride, stride = pcall(dosbox.mem_read_word, "ds", 0x30d2)
    if not ok_stride then stride = nil end
    local record_offset = nil
    local record_raw = nil
    if table_offset ~= nil and table_selector ~= nil and table_index ~= nil and stride ~= nil then
        record_offset = table_offset + table_index * stride
        local ok_record, raw = pcall(
            dosbox.mem_read_selector, table_selector, record_offset, 0x2c
        )
        if ok_record then record_raw = raw end
    end
    local callback_offset = nil
    local callback_selector = nil
    local callback_data = nil
    if record_raw ~= nil and #record_raw >= 0x1c then
        callback_offset = word(record_raw, 0x18 + 1)
        callback_selector = word(record_raw, 0x1a + 1)
        callback_data = dword(record_raw, 0x18 + 1)
    end
    return {
        selector_index = selector_index,
        caller_registers = {
            ax = (registers.eax or 0) & 0xffff,
            bx = (registers.ebx or 0) & 0xffff,
            cx = (registers.ecx or 0) & 0xffff,
        },
        stack_args = stack_args,
        index_address = {segment = registers.ds,
                         offset = index_offset},
        table_index = table_index,
        table_pointer = {selector = table_selector, offset = table_offset},
        table_stride = stride,
        record = {selector = table_selector, offset = record_offset},
        callback = {word_0x18 = callback_offset, word_0x1a = callback_selector,
                    dword_0x18 = callback_data},
        record_hex = record_raw ~= nil and hex(record_raw) or nil,
    }
end

-- 0442 can far-call a callback loaded into a resource segment.  That
-- callback is allowed to change its own ES:DI object, but it may also reach
-- the original player/data segment through aliases.  Keep the small set of
-- simulation-facing words used by the recovered 5937 contract address-named
-- and read them through the saved original DS selector.  Reading the live
-- `ds` here would be wrong: the loaded callback changes DS before it runs.
local dispatch_global_offsets = {
    0x60d8, 0x60da, 0x85da, 0x881c, 0x881e, 0x880a, 0x880c, 0x8822,
    0x4ff2, 0x4ff6, 0x4ff8, 0x4ffa, 0x89ea, 0x89e6, 0x8812, 0x8816,
    0x881a, 0x8810, 0x89ec,
}

local function dispatch_object_state_snapshot(selector, offset)
    if selector == nil or offset == nil then return nil end
    local ok, raw = pcall(
        dosbox.mem_read_selector, selector, offset, player_record_size
    )
    if not ok or raw == nil or #raw < player_record_size then return nil end
    return {
        selector = selector,
        offset = offset,
        record_hex = hex(raw),
    }
end

local function dispatch_state_snapshot(context)
    if context == nil or context.data_selector == nil or
       context.player_selector == nil or context.player_offset == nil then
        return nil
    end
    local globals = {}
    for _, offset in ipairs(dispatch_global_offsets) do
        local ok, raw = pcall(
            dosbox.mem_read_selector, context.data_selector, offset, 2
        )
        if ok and raw ~= nil and #raw >= 2 then
            globals[string.format("0x%04x", offset)] = word(raw, 1)
        end
    end
    local ok_player, raw_player = pcall(
        dosbox.mem_read_selector, context.player_selector,
        context.player_offset, player_record_size
    )
    return {
        data_selector = context.data_selector,
        player = {
            selector = context.player_selector,
            offset = context.player_offset,
            record_hex = ok_player and raw_player ~= nil and
                         #raw_player >= player_record_size and hex(raw_player) or nil,
        },
        globals = globals,
    }
end

local function dispatch_object_differences(before, after)
    if before == nil or after == nil then return nil end
    return hex_differences(before.record_hex, after.record_hex)
end

local function dispatch_state_differences(before, after)
    if before == nil or after == nil then return nil end
    local result = {
        player = nil,
        globals = {},
    }
    if before.player ~= nil and after.player ~= nil then
        result.player = hex_differences(before.player.record_hex,
                                         after.player.record_hex)
    end
    for address, value in pairs(before.globals or {}) do
        if after.globals ~= nil and after.globals[address] ~= nil and
           after.globals[address] ~= value then
            result.globals[#result.globals + 1] = {
                address = address,
                before = value,
                after = after.globals[address],
            }
        end
    end
    table.sort(result.globals, function(left, right)
        return left.address < right.address
    end)
    return result
end

-- 01F7:16CE is the one contact-child callee that can feed back into later
-- simulation.  Its register convention is different from 01F7:3376:
-- AX=cell X pixels, BX=cell Y pixels, and DX is the replacement tile/effect
-- word.  Capture the selected MAP word at entry and defer the second read
-- until the focused callback has returned.  This keeps the observation
-- scoped to the writer call without introducing a guessed breakpoint at an
-- internal return address.
local function map_writer_snapshot(hit)
    if hit.offset ~= 0x16ce then return nil end
    local registers = hit.registers or {}
    local coordinate_x = (registers.eax or 0) & 0xffff
    local coordinate_y = (registers.ebx or 0) & 0xffff
    local lookup = map_lookup_snapshot(hit, coordinate_x, coordinate_y)
    return {
        coordinates = {x = coordinate_x, y = coordinate_y},
        effect_selector = (registers.ecx or 0) & 0xffff,
        effect_word = (registers.edx or 0) & 0xffff,
        map_selector = lookup.map_selector,
        map_base = lookup.map_base,
        row_stride = lookup.row_stride,
        cell_offset = lookup.cell_offset,
        before_word = lookup.cell_word,
        before_tile_id = lookup.tile_id,
    }
end

local function finalize_map_writer_events(sample)
    for _, event in ipairs(sample.execute_watches or {}) do
        local writer = event.map_writer
        if writer == nil then goto continue end
        local ok, after_word = pcall(
            selector_word, writer.map_selector, writer.cell_offset
        )
        if ok then
            writer.after_word = after_word
            writer.after_tile_id = after_word & 0x1ff
            writer.map_write_applied = writer.before_word ~= after_word
        else
            writer.after_read_error = tostring(after_word)
        end
        ::continue::
    end
end

local function dispatch_target_return_location(hit)
    local address, reason = far_return_location(hit)
    if address == nil then return nil, reason end
    local valid, validation_reason = validate_return_address(address)
    if not valid then return nil, validation_reason end
    return address
end

local function record_execute_watch(sample, hit)
    local index = patch_watch.is_execute_watch(
        execute_watches, hit.segment, hit.offset)
    local context = sample.indirect_dispatch_context
    local indirect_target = context ~= nil and
        hit.segment == context.target_segment and
        hit.offset == context.target_offset
    if index == nil and not indirect_target then return false end
    if indirect_target and index == nil then
        index = "indirect-target"
    end
    sample.execute_watches = sample.execute_watches or {}
    local event = {
        event_index = next_trace_event(),
        frame_index = sample.frame_index,
        index = index,
        segment = hit.segment,
        offset = hit.offset,
        breakpoint = {segment = hit.segment, offset = hit.offset},
        registers = hit.registers,
        object = callback_object_snapshot(hit),
        globals = trace_globals(),
    }
    if indirect_target then
        event.indirect_target = true
        -- Preserve the loaded callback's instruction bytes at the target
        -- entry.  These bytes are runtime resource code rather than part of
        -- QUIKY.EXE, so they cannot be recovered by the EXE-only Ghidra
        -- import.  The dump is evidence for a separate protected-mode
        -- Ghidra import; it is never interpreted here as a semantic guess.
        local ok_target_code, target_code = pcall(
            dosbox.mem_read_selector, hit.segment, hit.offset, 0x0400
        )
        if ok_target_code and target_code ~= nil then
            event.target_code_hex = hex(target_code)
            event.target_code_size = #target_code
        else
            event.target_code_read_error = tostring(target_code)
        end
        event.stack_hex = hex(dosbox.mem_read(
            "ss", (hit.registers.esp or 0) & 0xffff, 8
        ) or "")
        event.simulation_before = dispatch_state_snapshot(context)
        event.target_object_before = dispatch_object_state_snapshot(
            hit.registers and hit.registers.es,
            hit.registers and ((hit.registers.edi or 0) & 0xffff)
        )
        local target_return, target_return_reason =
            dispatch_target_return_location(hit)
        event.return_expected = target_return
        event.return_read_error = target_return == nil and target_return_reason or nil
        if target_return ~= nil then
            local armed = arm_validated_return(
                sample, target_return, hit.offset, "indirect target return"
            )
            if armed then
                sample.pending_indirect_target_return = {
                    address = target_return,
                    event = event,
                }
            end
        end
    end
    event.external_dispatch = external_dispatch_snapshot(hit)
    if hit.segment == 0x01e7 and hit.offset == 0x0fcf and
       sample.player_callback ~= nil and
       sample.player_callback.effect_dispatches ~= nil then
        local registers = hit.registers or {}
        sample.player_callback.effect_dispatches[
            #sample.player_callback.effect_dispatches + 1
        ] = {
            address = 0x01e70fcf,
            code = (registers.edx or 0) & 0xffff,
        }
    end
    if event.external_dispatch ~= nil and
       event.external_dispatch.callback ~= nil and
       event.external_dispatch.callback.word_0x18 ~= nil and
       event.external_dispatch.callback.word_0x1a ~= nil then
        sample.indirect_dispatch_context = sample.indirect_dispatch_context or {}
        sample.indirect_dispatch_context.target_offset =
            event.external_dispatch.callback.word_0x18
        sample.indirect_dispatch_context.target_segment =
            event.external_dispatch.callback.word_0x1a
        -- The selected callback lives in a loaded resource segment and is
        -- not knowable from the executable's static watch list.  Arm its
        -- entry immediately after 0442 resolves the table record so the
        -- target-inclusive trace remains useful for held-out levels without
        -- guessing an address in the command line.
        local target = {
            segment = sample.indirect_dispatch_context.target_segment,
            offset = sample.indirect_dispatch_context.target_offset,
        }
        local target_valid, target_reason = validate_return_address(target)
        if target_valid then
            arm_breakpoint("indirect-target", target.segment, target.offset,
                           {once = true})
            event.indirect_target_breakpoint_armed = true
        else
            event.indirect_target_breakpoint_error = target_reason
        end
    end
    -- 3986 is the player negative-mode vertical probe, but its flag result
    -- comes from the 1C92 MAP helper.  Capture the live cell and descriptor
    -- at the 3986 entry so a one-event execute watch carries the complete
    -- probe input without rearming the repeated 1C92 breakpoint.
    if hit.offset == 0x3986 or hit.offset == 0x1c92 then
        event.map_property = map_property_snapshot(hit)
    end
    event.map_writer = map_writer_snapshot(hit)
    -- 3D02 receives the live player coordinates through the callback's
    -- object record rather than stable AX/BX values at its entry.  Preserve
    -- the descriptor lookup at that exact helper event when a full record is
    -- requested, so the response polarity is auditable without a second
    -- breakpoint pass.
    if hit.offset == 0x3d02 and event.object ~= nil and
       event.object.selector ~= nil and event.object.offset ~= nil then
        local ok, raw = pcall(
            dosbox.mem_read_selector, event.object.selector,
            event.object.offset, player_record_size
        )
        if ok and raw ~= nil and #raw >= 0x0c then
            local x_fixed = signed32(dword(raw, 0x02 + 1))
            local y_fixed = signed32(dword(raw, 0x06 + 1))
            local x = math.floor(x_fixed / 65536)
            local y = math.floor(y_fixed / 65536)
            local synthetic = {
                offset = 0x5cc3,
                segment = hit.segment,
                registers = {
                    eax = y & 0xffff,
                    ebx = x & 0xffff,
                    esp = hit.registers and hit.registers.esp or 0,
                },
            }
            event.player_descriptor = {
                player_position = {x = x, y = y,
                                   x_fixed_signed = x_fixed,
                                   y_fixed_signed = y_fixed},
                helper = map_property_snapshot(synthetic),
            }
        end
    end
    sample.execute_watches[#sample.execute_watches + 1] = event
    sample.execute_watch = event
    return true
end

local function restore_collision_patch(collision)
    if collision == nil then return end
    local patches = collision.patches
    if patches == nil and collision.patch ~= nil then patches = {collision.patch} end
    if patches == nil then return end
    for _, patch in ipairs(patches) do
        dosbox.mem_write_selector(
            patch.selector, patch.offset,
            string.char(patch.original & 0xff, (patch.original >> 8) & 0xff)
        )
    end
end

local function stop_for_capture()
    local current = dosbox.cpu_state()
    arm_breakpoint("capture-barrier", current.cs, current.eip)
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
    arm_breakpoint("level-selector-cheat", 0x01d7, 0x491d)
    dosbox.key("KBD_4", true)
    local cheat = wait_hit("level selector branch")
    dosbox.key("KBD_4", false)
    dosbox.output.checkpoints = {cheat = cheat}
    dosbox.mem_write("ds", 0x89f2, "\x01")
    dosbox.mem_write("ds", 0x88ba, "\x05\x00")
    dosbox.debug_continue()
    dosbox.wait_frames(selector_frames)
    arm_breakpoint("level-selector-input", 0x01d7, 0x4ace)
    local input_wait = wait_hit("selector input wait")
    dosbox.output.checkpoints.input_wait = input_wait
    dosbox.mem_write("ds", 0x85d4,
                     string.char(selector_index & 0xff, selector_index >> 8))
    arm_breakpoint("level-selector-launch", 0x01d7, 0x4b18)
    dosbox.mem_write("ds", 0x88bc, "\x20\x00")
    dosbox.debug_continue()
    dosbox.output.checkpoints.launch = wait_hit("selector Space dispatch")
end

local function scheduler_bank_snapshot(base)
    local raw = dosbox.mem_read("ds", base, 0x200) or ""
    local entries = {}
    for index = 0, 63 do
        local base = index * 8 + 1
        if base + 7 > #raw then break end
        local phase_callback_offset = word(raw, base)
        local secondary_callback_offset = word(raw, base + 2)
        local object_offset = word(raw, base + 4)
        local entry_word_06 = word(raw, base + 6)
        if phase_callback_offset == 0xffff then
            break
        end
        entries[#entries + 1] = {
            index = index,
            -- The scheduler words are two independent near offsets, not a
            -- far callback pointer. Keep the raw roles explicit so a trace
            -- consumer cannot mistake +0x02 for a segment.
            phase_callback_offset = phase_callback_offset,
            secondary_callback_offset = secondary_callback_offset,
            object_offset = object_offset,
            entry_word_06 = entry_word_06,
            -- Compatibility aliases for older report consumers. They carry
            -- offsets only; no segment meaning is assigned here.
            callback = {offset = phase_callback_offset},
            secondary_callback = {offset = secondary_callback_offset},
            object = {offset = object_offset},
        }
    end
    return {base = base, stride = 8, entries = entries}
end

local function scheduler_snapshot()
    -- 0E96 flips DS:7966 before dispatching the old bank; 0FA2 then
    -- selects the bank named by the post-flip cursor.  Capture both banks
    -- so a callback entry cannot be mistaken for an inactive terminator in
    -- the other bank.  The old `entries` field remains the bank-A view for
    -- consumers that only need the historical shape.
    local insert_cursor = dosbox.mem_read_word("ds", 0x7966) or 0
    local banks = {
        scheduler_bank_snapshot(0x7566),
        scheduler_bank_snapshot(0x7766),
    }
    return {
        base = 0x7566,
        stride = 8,
        insert_cursor = insert_cursor,
        nonzero_state_bank = insert_cursor & 0x0200,
        entries = banks[1].entries,
        banks = banks,
    }
end

dosbox.output.awaiting_startup_replay = true
dosbox.wait_frames(350)
if select_level ~= "" then
    begin_selected_level()
    if capture_startup_stream then
        -- begin_selected_level() leaves the selector at the launch barrier
        -- (01D7:4B18). Re-arm the one-shot entry until the first non-stream
        -- target is reached. This captures the complete startup strip order
        -- and leaves that first ordinary callback pending for the normal
        -- sample loop instead of consuming it.
        -- Keep a player barrier armed in parallel.  A stream call returns
        -- through 01F7:1DC9; stepping to that return address before
        -- re-arming 1CDA avoids re-reporting the current instruction on
        -- debugger builds whose one-shot breakpoint is not removed until
        -- after execution resumes.
        arm_breakpoint("startup-stream-player", 0x01f7, 0x3ff8)
        for stream_attempt = 1, startup_stream_max_attempts do
            arm_breakpoint("startup-stream", 0x01f7, 0x1cda)
            dosbox.debug_continue()
            local stream_hit = wait_hit("startup ARE stream entry")
            dosbox.breakpoint_remove(0x01f7, 0x1cda)
            if stream_hit.offset ~= 0x1cda then
                startup_stream_pending_hit = stream_hit
                break
            end
            startup_stream_events[#startup_stream_events + 1] = {
                attempt = stream_attempt,
                segment = stream_hit.segment,
                offset = stream_hit.offset,
                registers = stream_hit.registers,
                globals = startup_stream_snapshot(),
            }
            local leaf_index = startup_stream_events[#startup_stream_events]
                .globals.leaf_prng_index
            if startup_previous_leaf_index ~= nil and
                    leaf_index ~= startup_previous_leaf_index then
                local leaf_pool = {}
                local snapshot = pool_snapshot()
                for _, object in ipairs(snapshot.objects or {}) do
                    if object.callback == 0x47e7 then
                        leaf_pool[#leaf_pool + 1] = object
                    end
                end
                startup_stream_events[#startup_stream_events].leaf_pool = leaf_pool
            end
            startup_previous_leaf_index = leaf_index
            arm_breakpoint("startup-stream-return", 0x01f7, 0x1dc9)
            dosbox.debug_continue()
            local stream_return = wait_hit("startup ARE stream return")
            dosbox.breakpoint_remove(0x01f7, 0x1dc9)
            assert(stream_return.segment == 0x01f7 and
                       stream_return.offset == 0x1dc9,
                   "startup ARE stream returned through an unexpected address")
        end
    end
else
    dosbox.key("KBD_space", true)
    dosbox.wait_frames(4)
    dosbox.key("KBD_space", false)
end

-- Object-focus lifetime experiments need the triggering MAP cell to remain
-- patched before the first focused child callback.  The ordinary player
-- trace applies and restores this mutation around each player callback;
-- persistent mode is deliberately opt-in and is restored before capture.
if persistent_map_patch then
    assert(map_patch_cell ~= nil,
           "persistent MAP patch requires map_patch_cell")
    if object_focus ~= nil then
        -- The selected-level launcher returns before the first live player
        -- callback and its MAP pointer can still refer to the selector's
        -- previous level.  Reach the live 3FF8 entry first, install the
        -- mutation before its contact probes, then retarget the requested
        -- pooled callback.  This also guarantees that exactly one triggering
        -- child is created before the lifetime experiment begins.
        local child_callback_offset = focus_callback_offset
        focus_callback_offset = 0x3ff8
        arm_callback_targets()
        dosbox.debug_continue()
        local warmup_hit = wait_hit("player MAP-patch warmup callback")
        while warmup_hit.offset ~= 0x3ff8 do
            arm_callback_targets()
            dosbox.debug_continue()
            warmup_hit = wait_hit("player MAP-patch warmup callback")
        end
        persistent_map_patch_state = apply_player_map_patch()
        assert(persistent_map_patch_state ~= nil,
               "persistent MAP patch could not resolve the live MAP cell")
        -- Restore immediately after this triggering player callback returns;
        -- leaving the cell patched would allocate one new 6328 child per
        -- callback and make an exact pool-offset lifetime trace ambiguous.
        local stack = dosbox.mem_read(
            "ss", (warmup_hit.registers.esp or 0) & 0xffff, 2) or ""
        assert(#stack >= 2, "player MAP-patch warmup return was not readable")
        local warmup_return = word(stack, 1)
        release_callback_targets(0x3ff8)
        arm_breakpoint("persistent-map-patch-return", warmup_hit.segment,
                       warmup_return)
        dosbox.debug_continue()
        local return_hit = wait_hit("player MAP-patch warmup return")
        assert(return_hit.segment == warmup_hit.segment and
                   return_hit.offset == warmup_return,
               "player MAP-patch warmup returned through an unexpected address")
        restore_player_map_patch(persistent_map_patch_state)
        persistent_map_patch_state = nil
        focus_callback_offset = child_callback_offset
    else
        persistent_map_patch_state = apply_player_map_patch()
        assert(persistent_map_patch_state ~= nil,
               "persistent MAP patch could not resolve the MAP cell")
    end
end

if transition_focus then
    local watched = {
        {offset = 0x1ae6, name = "write_89ea_clear"},
        {offset = 0x19e6, name = "state_update_entry", repeat_target = true},
        {offset = 0x19a3, name = "write_89ea_start", repeat_target = true},
        {offset = 0x1a3d, name = "write_89ea_state", repeat_target = true},
        {offset = 0x199d, name = "write_89ea_callback", repeat_target = true},
        {offset = 0x1bc4, name = "overlap_to_19e6", repeat_target = true},
        {offset = 0x3ab3, name = "motion_to_19e6", repeat_target = true},
        {offset = 0x3ff8, name = "player_callback", repeat_target = true},
        {offset = 0x43d0, name = "player_boundary_check", repeat_target = true},
        {offset = 0x44dc, name = "decrement_89ea", repeat_target = true},
        {offset = 0x339a, name = "map_low_id_writer", repeat_target = true},
        {offset = 0x340a, name = "map_property_writer", repeat_target = true},
        {offset = 0x5c9d, name = "map_cell_store_helper", repeat_target = true},
        {offset = 0x16ce, name = "map_effect_tile_rewrite", repeat_target = true},
        {offset = 0x4ba4, name = "scheduler_gate_test", segment = 0x01d7},
        {offset = 0x4bd8, name = "secondary_gate_test", segment = 0x01d7},
        {offset = 0x3861, name = "secondary_loader_entry", segment = 0x01d7},
        {offset = 0x1aaa, name = "player_recovery_entry", repeat_target = false},
    }
    local seen = {}
    local events = {}
    local input_active = false
    local input_event_count = 0
    local blocked_repeat_name = nil
    local forced_player_fall = false
    local function arm_transition_targets()
        for _, item in ipairs(watched) do
            if (item.repeat_target and item.name ~= blocked_repeat_name) or
                    (not item.repeat_target and not seen[item.name]) then
                arm_breakpoint("transition", item.segment or 0x01f7, item.offset)
            end
        end
    end
    if input_key ~= "" then
        dosbox.key(input_key, true)
        input_active = true
    end
    if transition_warmup_frames > 0 then
        dosbox.wait_frames(transition_warmup_frames)
    end
    for sequence = 1, transition_steps do
        if sequence > 1 and transition_probe_frames > 0 then
            -- Run ordinary gameplay between breakpoint passes. Remove the
            -- one-shot probes first so wait_frames can advance freely.
            for _, item in ipairs(watched) do
                breakpoint_controller:release(
                    "transition", item.segment or 0x01f7, item.offset)
            end
            blocked_repeat_name = nil
            local probe_frames = transition_probe_frames
            if transition_probe_tail_frames > 0 and
                    transition_probe_tail_camera_x > 0 and
                    dosbox.mem_read_word("ds", 0x81c0) >= transition_probe_tail_camera_x then
                probe_frames = transition_probe_tail_frames
            end
            dosbox.wait_frames(probe_frames)
        end
        arm_transition_targets()
        dosbox.debug_continue()
        local hit, err = dosbox.wait_for_breakpoint(5000)
        if not hit then
            events[#events + 1] = {sequence = sequence, timeout = err or "timeout"}
            break
        end
        local name = "unknown"
        for _, item in ipairs(watched) do
            if (item.segment or 0x01f7) == hit.segment and item.offset == hit.offset then
                name = item.name
                break
            end
        end
        local registers = hit.registers or {}
        local stack = dosbox.mem_read("ss", (registers.esp or 0) & 0xffff, 8) or ""
        local event = {
            sequence = sequence,
            name = name,
            breakpoint = {segment = hit.segment, offset = hit.offset},
            registers = registers,
            caller = {stack_hex = hex(stack)},
            globals = trace_globals(),
        }
        if name == "scheduler_gate_test" or
           name == "secondary_gate_test" or
           name == "player_recovery_entry" or
           name == "write_89ea_clear" or
           name == "player_callback" then
            -- The recovery branch is an outer scheduler boundary. Capture
            -- both rotating banks only at those events so the trace can
            -- distinguish player identity from append-index movement without
            -- paying the full pool-walk cost on every countdown callback.
            event.scheduler = scheduler_snapshot()
        end
        if #stack >= 4 then
            event.caller.return_address = {offset = word(stack, 1), segment = word(stack, 3)}
        end
        events[#events + 1] = event
        if transition_force_player_fall and not forced_player_fall and
                hit.segment == 0x01f7 and hit.offset == 0x3ff8 then
            local registers = hit.registers or {}
            local selector = registers.es or 0
            local object_offset = (registers.edi or 0) & 0xffff
            dosbox.mem_write_selector(selector, object_offset + 8, "\xff\x7f")
            forced_player_fall = true
            event.action = "force_player_y=0x7fff"
        end
        local hit_item = nil
        for _, item in ipairs(watched) do
            if (item.segment or 0x01f7) == hit.segment and item.offset == hit.offset then
                hit_item = item
                break
            end
        end
        if hit_item == nil or not hit_item.repeat_target then
            seen[name] = true
        end
        blocked_repeat_name = hit_item and hit_item.repeat_target and name or nil
        input_event_count = input_event_count + 1
        if input_active and transition_hold_events > 0 and
                input_event_count >= transition_hold_events then
            dosbox.key(input_key, false)
            input_active = false
            event.action = "release_" .. input_key
        end
    end
    if input_active then dosbox.key(input_key, false) end
    dosbox.output.player_trace = {
        trace_schema_version = trace_config.schema_version or 1,
        transition_trace = {
            steps = transition_steps,
            hold_events = transition_hold_events,
            input_key = input_key,
            force_player_fall = transition_force_player_fall,
            probe_frames = transition_probe_frames,
            probe_tail_frames = transition_probe_tail_frames,
            probe_tail_camera_x = transition_probe_tail_camera_x,
            warmup_frames = transition_warmup_frames,
            events = events,
        },
        final_globals = trace_globals(),
    }
    return
end

local samples = {}
local input_stream = {}
local experiment_frame = 0
local continuous_input_active = false
local continuous_input_released = false
local continuous_input_callbacks = 0
local active_phase_keys = nil
local active_phase_callbacks_remaining = 0

local function capture_input_sample()
    if not record_input_stream then return end
    local keyboard = dosbox.mem_read_word("ds", 0x88bc)
    local auxiliary = dosbox.mem_read_word("ds", 0x8196)
    local camera_x = dosbox.mem_read_word("ds", 0x81c0)
    local camera_y = dosbox.mem_read_word("ds", 0x81c4)
    -- The startup recording briefly leaves DS pointed at a non-game data
    -- block. Do not publish those words as replay input; the first sane
    -- camera sample is the level's actual callback boundary.
    if camera_x > 0x4000 or camera_y > 0x4000 then return end
    input_stream[#input_stream + 1] = {
        sequence = #input_stream + 1,
        guest_frame = experiment_frame,
        input_flags = (keyboard | auxiliary) & 0xffff,
        camera = {
            x = camera_x,
            y = camera_y,
        },
    }
end

local function wait_recorded_frames(frames)
    for _ = 1, frames do
        dosbox.wait_frames(1)
        experiment_frame = experiment_frame + 1
        capture_input_sample()
    end
end

-- Run the primary input before the first post-baseline sample.  This keeps
-- long approach trajectories out of the per-sample input wait, so execute
-- watches are armed on the natural contact window instead of after it.
if input_warmup_frames > 0 and input_key ~= "" then
    dosbox.key(input_key, true)
    wait_recorded_frames(input_warmup_frames)
    dosbox.key(input_key, false)
end

local function key_is_continuously_held(key)
    if input_hold_key == "" then return false end
    for _, held_key in ipairs(input_hold_keys) do
        if held_key == key then return true end
    end
    return false
end

local function release_phase_through_callback()
    if active_phase_keys == nil then return end
    for index = #active_phase_keys, 1, -1 do
        if not key_is_continuously_held(active_phase_keys[index]) then
            dosbox.key(active_phase_keys[index], false)
        end
    end
    active_phase_keys = nil
    active_phase_callbacks_remaining = 0
end

for sequence = 1, sample_count do
    local phase = nil
    local phase_through_callback_keys = nil
    if sequence > 1 then
        phase = input_phases[sequence - 1]
        local held_input = phase == nil and input_key ~= "" and input_frames > 0 and
            (input_samples == 0 or sequence <= input_samples + 1)
        local active_input_key = input_key
        if input_key_switch ~= "" and input_switch_sample > 0 and
           sequence >= input_switch_sample then
            active_input_key = input_key_switch
        end
        local held_secondary = held_input and input_key_secondary ~= "" and
            sequence >= secondary_start_sample and
            (secondary_end_sample == 0 or sequence <= secondary_end_sample)
        local secondary_pressed = false
        if input_hold_key ~= "" then
            if not continuous_input_active and
                    continuous_input_callbacks < input_hold_frames then
                for _, key in ipairs(input_hold_keys) do dosbox.key(key, true) end
                continuous_input_active = true
            end
            if continuous_input_callbacks < input_hold_frames then
                -- The callback barrier itself is the clock in this mode.
                -- Waiting a DOSBox display frame here would let several
                -- player callbacks run before the breakpoint is re-armed.
            elseif continuous_input_active and not continuous_input_released then
                for index = #input_hold_keys, 1, -1 do
                    dosbox.key(input_hold_keys[index], false)
                end
                continuous_input_active = false
                continuous_input_released = true
            end
            continuous_input_callbacks = continuous_input_callbacks + 1
            experiment_frame = experiment_frame + 1
            capture_input_sample()
            if phase ~= nil then
                local keys = phase.keys or {}
                local phase_frames = phase.frames or 0
                -- The historical trace route needs both forms: zero phases
                -- change keys at a callback barrier, while the explicit
                -- hold-after-wait mode keeps keys down during a guest wait
                -- and through the callback sampled at its end.
                if input_phase_through_callback and
                   (phase_frames == 0 or input_phase_hold_after_wait) then
                    for _, key in ipairs(keys) do dosbox.key(key, true) end
                    if phase_frames > 0 then wait_recorded_frames(phase_frames) end
                    phase_through_callback_keys = keys
                else
                    for _, key in ipairs(keys) do dosbox.key(key, true) end
                    if phase_frames > 0 then wait_recorded_frames(phase_frames) end
                    for index = #keys, 1, -1 do dosbox.key(keys[index], false) end
                end
            end
        elseif phase ~= nil then
            local keys = phase.keys or {}
            local phase_frames = phase.frames or 0
            -- See the corresponding branch above: :0 means one callback
            -- step; hold-after-wait additionally preserves nonzero phases
            -- until the callback barrier after their guest-frame wait.
            if input_phase_through_callback and
               (phase_frames == 0 or input_phase_hold_after_wait) then
                for _, key in ipairs(keys) do dosbox.key(key, true) end
                if phase_frames > 0 then wait_recorded_frames(phase_frames) end
                phase_through_callback_keys = keys
            else
                for _, key in ipairs(keys) do dosbox.key(key, true) end
                if phase_frames > 0 then wait_recorded_frames(phase_frames) end
                for index = #keys, 1, -1 do dosbox.key(keys[index], false) end
            end
        elseif held_input then
            dosbox.key(active_input_key, true)
            if held_secondary then
                dosbox.key(input_key_secondary, true)
                secondary_pressed = true
            end
            if held_secondary and secondary_pulse_frames > 0 then
                local remaining = input_frames
                local pressed = true
                while remaining > 0 do
                    local chunk = math.min(secondary_pulse_frames, remaining)
                    wait_recorded_frames(chunk)
                    remaining = remaining - chunk
                    pressed = not pressed
                    dosbox.key(input_key_secondary, pressed)
                    secondary_pressed = pressed
                end
            else
                wait_recorded_frames(input_frames)
            end
            if held_secondary and secondary_pressed then
                dosbox.key(input_key_secondary, false)
            end
            dosbox.key(active_input_key, false)
        end
        local sample_frames_between = frames_between
        if frames_between_after_sample > 0 and
                sequence >= frames_between_after_sample then
            sample_frames_between = frames_between_after
        end
        wait_recorded_frames(sample_frames_between)
        end
        if phase_through_callback_keys ~= nil then
            if active_phase_keys ~= nil then
                release_phase_through_callback()
            end
            for _, key in ipairs(phase_through_callback_keys) do
                dosbox.key(key, true)
            end
            active_phase_keys = phase_through_callback_keys
            active_phase_callbacks_remaining = input_phase_hold_callbacks
        end
        local map_patch = nil
        if not persistent_map_patch then
            map_patch = apply_player_map_patch()
        end
        arm_targets()
        local pending_hit = sequence == 1 and startup_stream_pending_hit or nil
        if pending_hit ~= nil then
            startup_stream_pending_hit = nil
        else
            dosbox.debug_continue()
        end
    local hit = pending_hit or wait_hit("player/object update breakpoint")
    local ignored_object_callbacks = 0
    if object_focus ~= nil and object_focus.object_offset ~= nil then
        while hit.offset == focus_callback_offset do
            local candidate = callback_object_snapshot(hit)
            if candidate ~= nil and candidate.offset == object_focus.object_offset then
                break
            end
            ignored_object_callbacks = ignored_object_callbacks + 1
            if ignored_object_callbacks >= 256 then
                error("object focus exceeded 256 non-matching callbacks")
            end
            arm_callback_targets()
            dosbox.debug_continue()
            hit = wait_hit("focused object callback")
        end
    end
    local sample = {
        sequence = sequence,
        frame_index = experiment_frame,
        breakpoint = {segment = hit.segment, offset = hit.offset},
        breakpoint_owners = hit.breakpoint_owners,
        registers = hit.registers,
        globals = trace_globals(),
        related_breakpoints = {},
    }
    if object_focus ~= nil then
        sample.object_focus = {
            callback_offset = object_focus.callback_offset,
            object_offset = object_focus.object_offset,
            ignored_callbacks = ignored_object_callbacks,
        }
    end
    -- Preserve the original callback DS and player ES:DI before 5937 can
    -- dispatch into a loaded resource segment.  The target may switch DS and
    -- ES, so its entry snapshot alone cannot establish player side effects.
    if focus_callback and hit.offset == focus_callback_offset then
        local player = callback_object_snapshot(hit)
        if player ~= nil and hit.registers ~= nil then
            sample.indirect_dispatch_context = {
                data_selector = hit.registers.ds,
                player_selector = player.selector,
                player_offset = player.offset,
            }
        end
    end
    record_execute_watch(sample, hit)
    if scheduler_only then
        sample.scheduler = scheduler_snapshot()
    elseif not lean_player_capture or sequence == 1 then
        sample.pool = pool_snapshot()
        sample.scheduler = scheduler_snapshot()
    end
    sample.spawn_probe = apply_spawn_probe(hit)
    sample.release_probe = apply_release_probe(hit)
    if factory_focus and hit.offset == 0x0e06 then
        local before = sample.pool or pool_snapshot()
        arm_breakpoint("factory-tail", 0x01f7, 0x0f35)
        dosbox.debug_continue()
        local tail = wait_hit("factory allocation window tail")
        local after = pool_snapshot()
        local before_offsets = {}
        for _, object in ipairs(before.objects or {}) do
            before_offsets[object.offset] = true
        end
        local created = {}
        for _, object in ipairs(after.objects or {}) do
            if not before_offsets[object.offset] then created[#created + 1] = object end
        end
        sample.factory_event = {
            entry = {segment = hit.segment, offset = hit.offset,
                     registers = hit.registers, owners = hit.breakpoint_owners},
            tail = {segment = tail.segment, offset = tail.offset,
                    registers = tail.registers, owners = tail.breakpoint_owners},
            before_pool = before,
            after_pool = after,
            created_objects = created,
        }
    end
    local initial_hit = hit
    local descriptor_census_result = nil
    if descriptor_census and initial_hit.offset == 0x5cc3 then
        descriptor_census_result = descriptor_census_snapshot()
        descriptor_census_done = true
        sample.descriptor_census = descriptor_census_result
        local census_return, census_reason = far_return_location(initial_hit)
        if census_return ~= nil and arm_validated_return(
            sample, census_return, initial_hit.offset, "descriptor census return") then
            dosbox.debug_continue()
            hit = wait_hit("descriptor census helper return")
        else
            remember_unresolved_return(
                sample, initial_hit.offset, census_return,
                census_reason or "descriptor census return failed validation")
        end
    elseif property_focus and (initial_hit.offset == 0x1c6e or
                           initial_hit.offset == 0x1c92 or
                           initial_hit.offset == 0x5c27 or
                           initial_hit.offset == 0x5cc3) then
        sample.related_breakpoints[#sample.related_breakpoints + 1] = {
            segment = initial_hit.segment, offset = initial_hit.offset,
            owners = initial_hit.breakpoint_owners,
        }
        record_property(sample, initial_hit)
    elseif initial_hit.offset == 0x3376 and map_focus then
        sample.related_breakpoints[#sample.related_breakpoints + 1] = {
            segment = initial_hit.segment, offset = initial_hit.offset,
            owners = initial_hit.breakpoint_owners,
        }
        record_map_lookup(sample, initial_hit)
    elseif is_collision_target(initial_hit.offset) then
        sample.related_breakpoints[#sample.related_breakpoints + 1] = {
            segment = initial_hit.segment, offset = initial_hit.offset,
            owners = initial_hit.breakpoint_owners,
        }
        record_collision(sample, initial_hit)
    end
    if branch_focus and is_branch_target(initial_hit.offset) then
        hit = capture_branch_sequence(sample, initial_hit)
    end
    if focus_callback and initial_hit.offset ~= focus_callback_offset and
       initial_hit.offset ~= 0x3f27 and
       not (probe_release_emitter and initial_hit.offset == 0x470c) then
        suspend_side_probe_targets()
        arm_callback_targets()
        dosbox.debug_continue()
        hit = wait_hit("player callback after related breakpoint")
        sample.related_breakpoints[#sample.related_breakpoints + 1] = {
            segment = hit.segment, offset = hit.offset, owners = hit.breakpoint_owners,
        }
        sample.breakpoint = {segment = hit.segment, offset = hit.offset}
        sample.registers = hit.registers
        if not minimal_callback_capture then
            sample.globals = trace_globals()
        end
        if scheduler_only then
            sample.scheduler = scheduler_snapshot()
        elseif not lean_player_capture then
            sample.pool = pool_snapshot()
            sample.scheduler = scheduler_snapshot()
        end
        sample.release_probe = apply_release_probe(hit)
    end
    if hit.offset == 0x3f27 or
       (focus_callback and (hit.offset == focus_callback_offset or
                            (probe_release_emitter and hit.offset == 0x470c))) then
        local callback_object = callback_object_snapshot(hit)
        local callback_globals_before = callback_globals()
        sample.player_callback = {
            breakpoint = {segment = hit.segment, offset = hit.offset},
            callback_offset = hit.offset,
            registers = hit.registers,
            stack_hex = hex(dosbox.mem_read(
                "ss", (hit.registers.esp or 0) & 0xffff, 12) or ""),
            object = callback_object,
            pre_object = callback_object,
            pre_globals = callback_globals_before,
            record_size = callback_object and callback_object.state_size or 0x40,
        }
        if pending_sound_watch_enabled then
            -- This array is present only when the explicit 01E7:0FCF watch
            -- instruments the callback. An empty array is therefore real
            -- negative evidence for that boundary, not an inferred absence.
            sample.player_callback.effect_dispatches = {}
        end
        local stack = dosbox.mem_read(
            "ss", (hit.registers.esp or 0) & 0xffff, 4) or ""
        if #stack >= 4 and callback_object ~= nil then
            local callback_ok, callback_error = xpcall(function()
                if #(trace_config.patches or {}) > 0 then
                    patch_engine:apply(sample, {player = callback_object})
                end
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
            local property_return_event = nil
            local collision_return = nil
            local collision_return_event = nil
            local map_return = nil
            local map_return_event = nil
            local unresolved_targets = {}
            local event_count = 0
            local last_breakpoint = nil
            local repeated_breakpoint_count = 0
            local helper_tracing_aborted = false

            patch_watch.arm_execute_watches(breakpoint_controller, execute_watches)

            local function mark_unresolved_target(candidate, reason)
                local key = address_key(candidate)
                unresolved_targets[key] = true
                remember_unresolved_return(sample, candidate.offset, candidate,
                                           reason)
            end

            -- The callback return is a near return in the callback's current
            -- code segment.  It is validated too, so the guard below can
            -- always fall back to a known-safe callback barrier.
            local callback_return = {
                segment = return_segment, offset = return_offset,
            }
            local callback_return_valid, callback_return_reason =
                validate_return_address(callback_return)
            if not callback_return_valid then
                remember_unresolved_return(sample, 0x3f27, callback_return,
                                           "callback return: " ..
                                           callback_return_reason)
                sample.player_callback.return_unresolved = true
                helper_tracing_aborted = true
            end

            while returned == nil and callback_return_valid and
                  event_count < collision_event_limit do
                if callback_return_valid then
                    arm_validated_return(sample, callback_return, 0x3f27,
                                         "callback return")
                end
                if not helper_tracing_aborted then
                    if property_return ~= nil then
                        arm_validated_return(sample, property_return,
                                             0x5c27, "property return")
                    end
                    if collision_return ~= nil then
                        arm_validated_return(sample, collision_return,
                                             0x3df2, "collision return")
                    end
                    if map_return ~= nil then
                        arm_validated_return(sample, map_return,
                                             0x3376, "MAP return")
                    end
                    if property_return == nil and property_focus then
                        arm_property_targets(unresolved_targets)
                    end
                    if collision_return == nil and collision_focus then
                        for _, offset in ipairs(collision_offsets) do
                            if not unresolved_targets[address_key({
                                segment = 0x01f7, offset = offset
                            })] then
                                arm_breakpoint("collision", 0x01f7, offset)
                            end
                        end
                    end
                    if map_return == nil then arm_map_target(unresolved_targets) end
                end

                dosbox.debug_continue()
                local candidate = wait_hit("player callback return")
                record_execute_watch(sample, candidate)
                local candidate_key = address_key(candidate)
                local pending_target = sample.pending_indirect_target_return
                local indirect_target_returned = pending_target ~= nil and
                    candidate.segment == pending_target.address.segment and
                    candidate.offset == pending_target.address.offset
                local candidate_is_callback_return =
                    candidate.segment == return_segment and
                    candidate.offset == return_offset
                local candidate_is_property_return = property_return ~= nil and
                    candidate.segment == property_return.segment and
                    candidate.offset == property_return.offset
                local candidate_is_collision_return = collision_return ~= nil and
                    candidate.segment == collision_return.segment and
                    candidate.offset == collision_return.offset
                local candidate_is_map_return = map_return ~= nil and
                    candidate.segment == map_return.segment and
                    candidate.offset == map_return.offset
                -- The guard is a limit on nested helper entries, not on the
                -- return addresses used to unwind those helpers.  Counting
                -- both halves made four helper calls consume an eight-event
                -- budget and truncated the callback before its next probe.
                if not indirect_target_returned and
                   not candidate_is_callback_return and
                   not candidate_is_property_return and
                   not candidate_is_collision_return and
                   not candidate_is_map_return then
                    event_count = event_count + 1
                end
                if indirect_target_returned then
                    pending_target.event.return_actual = {
                        segment = candidate.segment,
                        offset = candidate.offset,
                        registers = candidate.registers,
                    }
                    pending_target.event.simulation_after =
                        dispatch_state_snapshot(sample.indirect_dispatch_context)
                    pending_target.event.target_object_after =
                        dispatch_object_state_snapshot(
                            pending_target.event.target_object_before and
                            pending_target.event.target_object_before.selector,
                            pending_target.event.target_object_before and
                            pending_target.event.target_object_before.offset
                        )
                    pending_target.event.differences = dispatch_state_differences(
                        pending_target.event.simulation_before,
                        pending_target.event.simulation_after
                    )
                    pending_target.event.target_object_differences =
                        dispatch_object_differences(
                            pending_target.event.target_object_before,
                            pending_target.event.target_object_after
                        )
                    sample.indirect_dispatch_return = {
                        segment = candidate.segment,
                        offset = candidate.offset,
                    }
                    sample.pending_indirect_target_return = nil
                end
                if candidate_key == last_breakpoint then
                    repeated_breakpoint_count = repeated_breakpoint_count + 1
                else
                    last_breakpoint = candidate_key
                    repeated_breakpoint_count = 1
                end

                if indirect_target_returned then
                    -- The loaded callback has returned to its dispatcher.
                    -- Its before/after player and original-DS snapshots are
                    -- already attached to the indirect-target event above.
                elseif candidate_is_callback_return then
                    returned = candidate
                elseif candidate_is_property_return then
                    if property_return_event ~= nil then
                        property_return_event.return_breakpoint = {
                            segment = candidate.segment,
                            offset = candidate.offset,
                            registers = candidate.registers,
                        }
                        restore_property_patch(property_return_event)
                    end
                    property_return = nil
                    property_return_event = nil
                elseif candidate_is_collision_return then
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
                elseif candidate_is_map_return then
                    if map_return_event ~= nil then
                        map_return_event.return_breakpoint = {
                            segment = candidate.segment,
                            offset = candidate.offset,
                            registers = candidate.registers,
                        }
                        restore_map_patch(map_return_event)
                    end
                    map_return = nil
                    map_return_event = nil
                elseif helper_tracing_aborted then
                    sample.related_breakpoints[#sample.related_breakpoints + 1] = {
                        segment = candidate.segment, offset = candidate.offset,
                        owners = candidate.breakpoint_owners,
                    }
                elseif repeated_breakpoint_count > collision_repeat_limit then
                    -- A breakpoint that repeats without reaching any pending
                    -- return is the failure mode that used to consume Lua's
                    -- instruction budget.  Stop tracing helpers for this
                    -- callback, preserve the evidence, and let the ordinary
                    -- callback return finish the sample.
                    sample.collision_trace_guard = {
                        event_count = event_count,
                        repeated_breakpoint = {
                            segment = candidate.segment,
                            offset = candidate.offset,
                        },
                        repeat_count = repeated_breakpoint_count,
                        reason = "repeated breakpoint",
                    }
                    mark_unresolved_target(candidate,
                                           "repeated breakpoint guard")
                    helper_tracing_aborted = true
                elseif event_count >= collision_event_limit then
                    sample.collision_trace_guard = {
                        event_count = event_count,
                        reason = "event-count limit",
                    }
                    mark_unresolved_target(candidate,
                                           "event-count guard")
                    helper_tracing_aborted = true
                else
                    sample.related_breakpoints[#sample.related_breakpoints + 1] = {
                        segment = candidate.segment, offset = candidate.offset,
                        owners = candidate.breakpoint_owners,
                    }
                    if is_property_target(candidate.offset) then
                        property_return_event = record_property(sample, candidate)
                        property_return = far_return_location(candidate)
                        if property_return == nil then
                            mark_unresolved_target(candidate,
                                                   "property return was not readable")
                            property_return_event = nil
                        elseif not arm_validated_return(
                            sample, property_return, candidate.offset,
                            "property return") then
                            mark_unresolved_target(candidate,
                                                   "property return failed validation")
                            property_return = nil
                            property_return_event = nil
                        end
                    elseif candidate.offset == 0x3376 and map_focus then
                        map_return_event = record_map_lookup(sample, candidate)
                        map_return = far_return_location(candidate)
                        if map_return == nil then
                            mark_unresolved_target(candidate,
                                                   "MAP return was not readable")
                            map_return_event = nil
                        elseif not arm_validated_return(
                            sample, map_return, candidate.offset, "MAP return") then
                            mark_unresolved_target(candidate,
                                                   "MAP return failed validation")
                            map_return = nil
                            map_return_event = nil
                        end
                    elseif is_collision_target(candidate.offset) then
                        collision_return_event = record_collision(sample, candidate)
                        collision_return = validated_collision_return(sample,
                                                                      candidate)
                        if collision_return == nil then
                            collision_return_event = nil
                            unresolved_targets[candidate_key] = true
                        end
                    end
                end
            end

            if returned == nil then
                -- The event cap is deliberately fail-soft.  Do not leave
                -- the script in the helper loop; return the sample with an
                -- explicit guard record and continue to the next barrier.
                if sample.collision_trace_guard == nil then
                    sample.collision_trace_guard = {
                        event_count = event_count,
                        reason = "event-count limit",
                    }
                end
                helper_tracing_aborted = true
                if callback_return_valid then
                    arm_validated_return(sample, callback_return, 0x3f27,
                                         "callback return after guard")
                    dosbox.debug_continue()
                    local guard_return = wait_hit("callback return after guard")
                    if guard_return.segment == return_segment and
                       guard_return.offset == return_offset then
                        returned = guard_return
                    end
                end
            end
            sample.player_callback.return_actual = {
                segment = returned and returned.segment or nil,
                offset = returned and returned.offset or nil,
            }
            local read_size = capture_player_record and player_record_size or 0x40
            local ok, raw_or_error = pcall(
                dosbox.mem_read_selector,
                callback_object.selector, callback_object.offset, read_size
            )
            if ok and raw_or_error and #raw_or_error >= read_size then
                sample.player_callback.post_object = object_snapshot(
                    raw_or_error, callback_object.selector, callback_object.offset, -1
                )
                sample.player_callback.post_object.state_size = read_size
                if not minimal_callback_capture then
                    sample.player_callback.writes = hex_differences(
                        callback_object.state_hex,
                        sample.player_callback.post_object.state_hex
                    )
                end
                    sample.player_callback.post_globals = callback_globals()
            else
                sample.player_callback.post_object_read_error =
                    ok and "short object state" or tostring(raw_or_error)
            end
            sample.player_callback.post_globals = callback_globals()
            sample.player_callback.global_writes = numeric_differences(
                callback_globals_before, sample.player_callback.post_globals
            )
            end, function(problem) return tostring(problem) end)
            local restore_ok, restore_error = pcall(
                patch_engine.restore, patch_engine
            )
            if not restore_ok then
                error("player patch restoration failed: " .. tostring(restore_error))
            end
            if not callback_ok then error(callback_error) end
        end
        if sample.release_probe ~= nil then
            capture_release_callback(sample)
        end
        if map_focus and sample.map_lookup == nil then
            arm_breakpoint("map", 0x01f7, 0x3376)
            dosbox.debug_continue()
            local related = wait_hit("MAP lookup after player callback")
            sample.related_breakpoints[#sample.related_breakpoints + 1] = {
                segment = related.segment, offset = related.offset,
                owners = related.breakpoint_owners,
            }
            record_map_lookup(sample, related)
        elseif collision_focus and sample.collision == nil then
            for _, offset in ipairs(collision_offsets) do
                arm_breakpoint("collision", 0x01f7, offset)
            end
            dosbox.debug_continue()
            local related = wait_hit("collision helper after player callback")
            sample.related_breakpoints[#sample.related_breakpoints + 1] = {
                segment = related.segment, offset = related.offset,
                owners = related.breakpoint_owners,
            }
            record_collision(sample, related)
        end
        if map_patch ~= nil then
            sample.map_patch = map_patch
            restore_player_map_patch(map_patch)
        end
    elseif hit.offset == 0x0f3c then
        sample.kind_scan = {
            cursor = dosbox.mem_read_word("ds", 0x36e0),
            target_kind = 0x64,
        }
    end
    if phase_through_callback_keys ~= nil then
        sample.input_phase_through_callback = {
            keys = phase_through_callback_keys,
            hold_callbacks = input_phase_hold_callbacks,
        }
    end
    if active_phase_keys ~= nil then
        active_phase_callbacks_remaining = active_phase_callbacks_remaining - 1
        if active_phase_callbacks_remaining <= 0 then
            release_phase_through_callback()
        end
    end
    finalize_map_writer_events(sample)
    samples[#samples + 1] = sample
end
release_phase_through_callback()
if continuous_input_active then
    for index = #input_hold_keys, 1, -1 do
        dosbox.key(input_hold_keys[index], false)
    end
end

if persistent_map_patch_state ~= nil then
    restore_player_map_patch(persistent_map_patch_state)
    persistent_map_patch_state = nil
end

local capture = stop_for_capture()
local result = {
    trace_schema_version = trace_config.schema_version or 1,
    samples = samples,
    final_capture_registers = capture.registers,
    final_globals = trace_globals(),
    final_pool = minimal_callback_capture and nil or pool_snapshot(),
    startup_stream_events = startup_stream_events,
    input_stream = record_input_stream and input_stream or nil,
}
for _, sample in ipairs(samples) do
    if sample.descriptor_census ~= nil then
        result.descriptor_census = sample.descriptor_census
        break
    end
end
dosbox.output.player_trace = result
