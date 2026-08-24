-- Capture the live object pool and MAP lookups around the gameplay update loop.
-- Loaded by research/tools/quikytrace.py with a structured TRACE_CONFIG table.
local trace_config = TRACE_CONFIG or {}
assert(type(trace_config) == "table", "TRACE_CONFIG must be a table")
local timeout_ms = trace_config.timeout_ms or 30000
local sample_count = trace_config.samples or 8
local frames_between = trace_config.frames_between or 30
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
local input_frames = trace_config.input_frames or 0
local input_samples = trace_config.input_samples or 0
local select_level = trace_config.select_level or ""
local selector_frames = trace_config.selector_frames or 60
local trace_event_counter = 0
local descriptor_census_done = false

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
for sequence = 1, sample_count do
    if sequence > 1 then
        if input_key ~= "" and input_frames > 0 and
           (input_samples == 0 or sequence <= input_samples + 1) then
            dosbox.key(input_key, true)
            dosbox.wait_frames(input_frames)
            dosbox.key(input_key, false)
            experiment_frame = experiment_frame + input_frames
        end
        dosbox.wait_frames(frames_between)
        experiment_frame = experiment_frame + frames_between
    end
    arm_targets()
    dosbox.debug_continue()
    local hit = wait_hit("player/object update breakpoint")
    local sample = {
        sequence = sequence,
        frame_index = experiment_frame,
        breakpoint = {segment = hit.segment, offset = hit.offset},
        registers = hit.registers,
        globals = static_globals(),
        pool = pool_snapshot(),
        scheduler = scheduler_snapshot(),
        related_breakpoints = {},
    }
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
    samples[#samples + 1] = sample
end

local capture = stop_for_capture()
local result = {
    trace_schema_version = trace_config.schema_version or 1,
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
