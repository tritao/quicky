-- Trace one ARE record from declaration dispatch through object-factory return.
local trace_config = TRACE_CONFIG or {}
assert(type(trace_config) == "table", "TRACE_CONFIG must be a table")
local state_machine_config = trace_config.state_machine or {}
local timeout_ms = trace_config.timeout_ms or 30000
local record_offset = trace_config.record_offset or 0x1792
local expected_type = trace_config.entity_type or 0x2b
local capture_delay_frames = trace_config.capture_delay_frames or 0
local lifetime_sample_count = trace_config.lifetime_samples or 0
local state_machine_sample_count = state_machine_config.samples or 0
local state_machine_camera_x = state_machine_config.camera_x or -1
local state_machine_camera_y = state_machine_config.camera_y or -1
local state_machine_keep_camera = state_machine_config.keep_camera or false
local state_machine_position_x = state_machine_config.position_x or -1
local state_machine_position_y = state_machine_config.position_y or -1
local state_machine_force_emission = state_machine_config.force_emission or false
local state_machine_patch_map_run = state_machine_config.patch_map_run or false
local sprite_init_offset = trace_config.sprite_init_offset or 0
local capture_frame_count = trace_config.capture_frames or 1
local capture_frame_step = trace_config.frame_step or 30
local select_level = trace_config.select_level or ""
local selector_frames = trace_config.selector_frames or 60
local source_scan_enabled = trace_config.source_scan or false
local movement_key = trace_config.movement_key or ""
local movement_frames = trace_config.movement_frames or 0
local return_key = trace_config.return_key or ""
local return_frames = trace_config.return_frames or 0
local movement_camera_x = trace_config.movement_camera_x or -1
local movement_camera_y = trace_config.movement_camera_y or -1
local runtime_offset = record_offset - 0x160
local startup_camera_x = nil
local startup_camera_y = nil

local function word(s, index)
    local lo, hi = string.byte(s, index, index + 1)
    return lo | (hi << 8)
end

local function dword(s, index)
    return word(s, index) | (word(s, index + 2) << 16)
end

local function little_word(value)
    return string.char(value & 0xff, (value >> 8) & 0xff)
end

local function hex(s)
    return (s:gsub(".", function(c)
        return string.format("%02x", string.byte(c))
    end))
end

local function byte_prefix(s, limit)
    local bytes = {}
    for index = 1, math.min(#s, limit) do
        bytes[#bytes + 1] = string.byte(s, index)
    end
    return bytes
end

local function sprite_map_probe(slot)
    if slot == nil or slot == 0xffff then return nil end
    local base = 0x6d8e + slot * 2
    local raw = dosbox.mem_read("ds", base, 0x40) or ""
    local words = {}
    for index = 0, math.min(#raw // 2, 32) - 1 do
        words[#words + 1] = word(raw, index * 2 + 1)
    end
    return {slot = slot, base = base, raw_hex = hex(raw), words = words}
end

local function sprite_descriptor_probe(slot)
    if slot == nil or slot == 0xffff then return nil end
    local map_address = 0x6d8e + slot * 2
    local map_index = dosbox.mem_read_word("ds", map_address)
    local stride = dosbox.mem_read_word("ds", 0x30d2)
    local base_offset = dosbox.mem_read_word("ds", 0x6d8a)
    local selector = dosbox.mem_read_word("ds", 0x6d8c)
    local descriptor_offset = (base_offset + map_index * stride) & 0xffff
    local ok, raw_or_error = pcall(
        dosbox.mem_read_selector, selector, descriptor_offset, 0x30
    )
    if not ok then
        return {
            map_address = map_address,
            map_index = map_index,
            stride = stride,
            base_offset = base_offset,
            selector = selector,
            offset = descriptor_offset,
            read_error = tostring(raw_or_error),
        }
    end
    local raw = raw_or_error or ""
    return {
        map_address = map_address,
        map_index = map_index,
        stride = stride,
        base_offset = base_offset,
        selector = selector,
        offset = descriptor_offset,
        raw_hex = hex(raw),
        width = (#raw >= 2 and word(raw, 1) or nil),
        height = (#raw >= 4 and word(raw, 3) or nil),
        origin_x = (#raw >= 10 and word(raw, 9) or nil),
        origin_y = (#raw >= 12 and word(raw, 11) or nil),
    }
end

local function dedicated_event_queue()
    local count = dosbox.mem_read_word("ds", 0x895e)
    local ring = dosbox.mem_read("ds", 0x8960, 0x80) or ""
    local events = {}
    local limit = count
    if limit > 0x80 then limit = 0x80 end
    for index = 0, limit - 1 do
        local ring_slot = string.byte(ring, index + 1)
        if not ring_slot then break end
        local offset = 0x6586 + ring_slot * 8
        local raw = dosbox.mem_read("ds", offset, 8) or ""
        local raw_bytes = {}
        for byte_index = 1, #raw do
            raw_bytes[#raw_bytes + 1] = string.byte(raw, byte_index)
        end
        events[#events + 1] = {
            queue_index = index,
            ring_slot = ring_slot,
            table_offset = offset,
            raw_hex = hex(raw),
            raw_bytes = raw_bytes,
            raw_length = #raw,
            position_dword = (#raw >= 4 and dword(raw, 1) or nil),
            record_word = (#raw >= 6 and word(raw, 5) or nil),
            animation_state = string.byte(raw, 7),
            subtype = string.byte(raw, 8),
        }
    end
    return {
        pending_count = count,
        ring_hex = hex(ring),
        subtype = string.byte(dosbox.mem_read("ds", 0x36ee, 1)),
        events = events,
    }
end

local function wait_hit(label)
    local hit, err = dosbox.wait_for_breakpoint(timeout_ms)
    if not hit then error(label .. ": " .. (err or "timeout")) end
    return hit
end

local function stop_for_capture()
    local current = dosbox.cpu_state()
    dosbox.breakpoint_set(current.cs, current.eip, {once = true})
    return wait_hit("capture barrier")
end

local function write_movement_camera_lock()
    if movement_camera_x >= 0 then
        dosbox.mem_write("ds", 0x81c0, little_word(movement_camera_x))
    end
    if movement_camera_y >= 0 then
        dosbox.mem_write("ds", 0x81c4, little_word(movement_camera_y))
    end
end

-- Advance from one scheduler frame boundary to the next while servicing the
-- camera update sites.  A plain wait_frames() cannot do this: a debugger stop
-- during that wait freezes the Lua coroutine, so the camera breakpoint would
-- never be observed.  0E96 is the normal frame/scheduler boundary, 1ED7 is
-- the camera-scroll routine, and 1DCA is the object visibility gate used by
-- the object callback tracer.
local function advance_locked_frames(count)
    local boundary = nil
    for _ = 1, count do
        dosbox.breakpoint_set(0x01f7, 0x0e96, {once = true})
        dosbox.breakpoint_set(0x01f7, 0x1ed7, {once = true})
        dosbox.breakpoint_set(0x01f7, 0x1dca, {once = true})
        dosbox.debug_continue()
        while true do
            local hit = wait_hit("camera-locked frame advance")
            if hit.segment == 0x01f7 and hit.offset == 0x1ed7 then
                local stack = dosbox.mem_read(
                    "ss", hit.registers.esp & 0xffff, 4) or ""
                assert(#stack >= 2, "camera-scroll call has no near return")
                local return_offset = word(stack, 1)
                dosbox.breakpoint_set(hit.registers.cs, return_offset,
                                      {once = true})
                dosbox.debug_continue()
                local returned = wait_hit("camera-scroll return")
                assert(returned.offset == return_offset,
                       "unexpected camera-scroll return breakpoint")
                write_movement_camera_lock()
                dosbox.breakpoint_set(0x01f7, 0x1ed7, {once = true})
                dosbox.breakpoint_set(0x01f7, 0x1dca, {once = true})
                dosbox.debug_continue()
            elseif hit.segment == 0x01f7 and hit.offset == 0x1dca then
                write_movement_camera_lock()
                dosbox.breakpoint_set(0x01f7, 0x1dca, {once = true})
                dosbox.debug_continue()
            else
                assert(hit.segment == 0x01f7 and hit.offset == 0x0e96,
                       "unexpected camera-locked frame breakpoint")
                -- 0E96 is the hand-off into the next object pass.  Reapply
                -- here as well as after 1ED7/1DCA so the next pass observes
                -- the requested camera even on frames where the normal
                -- camera path is skipped or clamped by the level bounds.
                write_movement_camera_lock()
                boundary = hit
                break
            end
        end
    end
    return boundary
end

-- Optional source-aware lifecycle ledger for movement experiments. Keep this
-- behind a config flag: the normal entity tracer should retain its small,
-- single-record output and timing characteristics.
local function lifecycle_source_snapshot(selector, offset)
    local snapshot = {selector = selector, offset = offset}
    if selector == nil or offset == nil or offset == 0xffff then
        snapshot.absent = true
        return snapshot
    end
    local ok, raw_or_error = pcall(dosbox.mem_read_selector, selector, offset, 6)
    if not ok then
        snapshot.read_error = tostring(raw_or_error)
        return snapshot
    end
    local raw = raw_or_error or ""
    snapshot.raw_hex = "hex:" .. hex(raw)
    if #raw >= 2 then
        snapshot.marker_word = word(raw, 1)
        snapshot.marker_low = string.byte(raw, 1)
        snapshot.marker_high = string.byte(raw, 2)
        snapshot.processed_marker = string.byte(raw, 2)
    end
    return snapshot
end

local function lifecycle_record_snapshot(raw, selector, offset, index,
                                         target_source_offset)
    local callback = word(raw, 0x18 + 1)
    local record = {
        index = index,
        selector = selector,
        offset = offset,
        active = callback ~= 0,
        update_callback = callback,
        object_class = string.byte(raw, 0x17 + 1),
        source_offset = word(raw, 0x1a + 1),
        lifetime = word(raw, 0x2c + 1),
        sprite_slot = word(raw, 0x12 + 1),
        state_field = word(raw, 0x2e + 1),
        update_state = word(raw, 0x32 + 1),
        position = {
            x = dword(raw, 3) >> 16,
            y = dword(raw, 7) >> 16,
        },
    }
    record.source_match = target_source_offset ~= nil and
        record.source_offset == target_source_offset
    return record
end

local function lifecycle_scan(source_selector, target_source_offset)
    if not source_scan_enabled then return nil end
    local pool_offset = dosbox.mem_read_word("ds", 0x755e)
    local pool_selector = dosbox.mem_read_word("ds", 0x7560)
    local stride = dosbox.mem_read_word("ds", 0x30ce)
    local records = {}
    local source_matches = {}
    local matching_offsets = {}
    local pool_raw_ok, pool_raw_or_error = pcall(
        dosbox.mem_read_selector, pool_selector, pool_offset, stride * 64)
    if not pool_raw_ok then
        return {
            target_source_offset = target_source_offset,
            source = lifecycle_source_snapshot(source_selector,
                                                target_source_offset),
            pool = {
                selector = pool_selector,
                offset = pool_offset,
                stride = stride,
                count = 64,
                read_error = tostring(pool_raw_or_error),
                records = records,
                source_matches = source_matches,
            },
        }
    end
    local pool_raw = pool_raw_or_error or ""
    for index = 0, 63 do
        local offset = pool_offset + index * stride
        local start = index * stride + 1
        local raw = pool_raw:sub(start, start + 0x40 - 1)
        local record = lifecycle_record_snapshot(
            raw, pool_selector, offset, index, target_source_offset)
        if record.source_match then
            source_matches[#source_matches + 1] = {
                index = index,
                offset = offset,
                active = record.active,
                update_callback = record.update_callback,
                source_offset = record.source_offset,
                object_class = record.object_class,
            }
            matching_offsets[offset] = true
        end
        records[#records + 1] = record
    end

    local banks = {}
    for bank = 0, 1 do
        local base = 0x7566 + bank * 0x200
        local raw = dosbox.mem_read("ds", base, 0x200) or ""
        local entries = {}
        local entry_count = 0
        local sentinel_index = nil
        for index = 0, math.min(#raw // 8, 64) - 1 do
            local callback = word(raw, index * 8 + 1)
            if callback == 0xffff then
                sentinel_index = index
                break
            end
            if callback ~= 0 then
                entry_count = entry_count + 1
                local object_offset = word(raw, index * 8 + 5)
                if matching_offsets[object_offset] then
                    entries[#entries + 1] = {
                        bank = bank,
                        index = index,
                        table_offset = base + index * 8,
                        callback = callback,
                        callback_selector = word(raw, index * 8 + 3),
                        object_offset = object_offset,
                        object_selector = word(raw, index * 8 + 7),
                        source_match = true,
                    }
                end
            end
        end
        banks[#banks + 1] = {
            bank = bank,
            base = base,
            entry_count = entry_count,
            sentinel_index = sentinel_index,
            entries = entries,
        }
    end
    return {
        target_source_offset = target_source_offset,
        globals = {
            camera_x = dosbox.mem_read_word("ds", 0x81c0),
            camera_y = dosbox.mem_read_word("ds", 0x81c4),
            stream_camera_x = dosbox.mem_read_word("ds", 0x3710),
            stream_camera_y = dosbox.mem_read_word("ds", 0x3712),
            stream_region_x = dosbox.mem_read_word("ds", 0x3714),
            stream_region_y = dosbox.mem_read_word("ds", 0x3716),
            bounds_object_offset = dosbox.mem_read_word("ds", 0x881a),
        },
        source = lifecycle_source_snapshot(source_selector,
                                            target_source_offset),
        pool = {
            selector = pool_selector,
            offset = pool_offset,
            stride = stride,
            count = 64,
            records = records,
            source_matches = source_matches,
        },
        scheduler = {
            bank_cursor = dosbox.mem_read_word("ds", 0x7966),
            banks = banks,
        },
    }
end

local state_machine_targets = {0x16ce, 0x3376, 0x171c, 0x393c, 0x8eb5}
local state_machine_exits = {0x8e78, 0x8e85, 0x9254, 0x9255}

local function arm_state_machine_breakpoints(skip_offset)
    local armed = {}
    for _, offset in ipairs(state_machine_targets) do
        if offset ~= skip_offset then
            armed[string.format("%04x", offset)] = dosbox.breakpoint_set(
                0x01f7, offset, {once = true})
        end
    end
    for _, offset in ipairs(state_machine_exits) do
        if offset ~= skip_offset then
            armed[string.format("%04x", offset)] = dosbox.breakpoint_set(
                0x01f7, offset, {once = true})
        end
    end
    return armed
end

local function disarm_state_machine_breakpoints()
    for _, offset in ipairs(state_machine_targets) do
        dosbox.breakpoint_remove(0x01f7, offset)
    end
    for _, offset in ipairs(state_machine_exits) do
        dosbox.breakpoint_remove(0x01f7, offset)
    end
end

local function state_machine_tile_effects()
    local raw = dosbox.mem_read("ds", 0x6986, 0x400) or ""
    local nonzero = {}
    for tile = 0, 0x1ff do
        local value = word(raw, tile * 2 + 1)
        if value ~= 0 then
            nonzero[#nonzero + 1] = {tile = tile, value = value}
        end
    end
    return {raw_hex = hex(raw), nonzero = nonzero}
end

local function state_machine_effect_run_base()
    local raw = dosbox.mem_read("ds", 0x6986, 0x400) or ""
    local run = 0
    for tile = 0, 0x1ff do
        if word(raw, tile * 2 + 1) ~= 0 then
            run = run + 1
            if run == 5 then return tile - 4 end
        else
            run = 0
        end
    end
    return nil
end

local function patch_state_machine_map_run(x, y, base_tile)
    if not state_machine_patch_map_run or base_tile == nil then return nil end
    local pointer = dword(dosbox.mem_read("ds", 0x657a, 4), 1)
    local map_base = pointer & 0xffff
    local map_selector = (pointer >> 16) & 0xffff
    local row_stride = dosbox.mem_read_word("ds", 0x657e)
    local base_offset = map_base + (y >> 4) * row_stride + (x >> 4) * 2
    local cells = {}
    for index = 0, 4 do
        local offset = base_offset + index * 2
        local before = dosbox.mem_read_word(map_selector, offset)
        local after = base_tile + index
        dosbox.mem_write_selector(map_selector, offset, little_word(after))
        cells[#cells + 1] = {
            selector = map_selector,
            offset = offset,
            before = before,
            after = after,
        }
    end
    return {
        selector = map_selector,
        base_offset = base_offset,
        row_stride = row_stride,
        x = x,
        y = y,
        base_tile = base_tile,
        cells = cells,
    }
end

-- Globals selected by the static input/camera slice.  Keep this probe
-- read-only: the state-machine experiment may override camera/position
-- fields, but these observations must describe what the original code saw at
-- each breakpoint.
local function static_slice_globals()
    return {
        input_action_flags = dosbox.mem_read_word("ds", 0x8196),
        keyboard_action_flags = dosbox.mem_read_word("ds", 0x88bc),
        last_keyboard_scan_code = dosbox.mem_read_word("ds", 0x88ba),
        camera_x = dosbox.mem_read_word("ds", 0x81c0),
        camera_y = dosbox.mem_read_word("ds", 0x81c4),
        camera_subtile_x = dosbox.mem_read_word("ds", 0x81ce),
        camera_subtile_phase = dosbox.mem_read_word("ds", 0x81d0),
        camera_scroll_x_fixed = dword(dosbox.mem_read("ds", 0x81a6, 4), 1),
        camera_scroll_y_fixed = dword(dosbox.mem_read("ds", 0x81aa, 4), 1),
        camera_target_x_fixed = dword(dosbox.mem_read("ds", 0x81be, 4), 1),
        camera_target_y_fixed = dword(dosbox.mem_read("ds", 0x81c2, 4), 1),
        camera_target_left = dosbox.mem_read_word("ds", 0x36fc),
        camera_target_top = dosbox.mem_read_word("ds", 0x36fe),
        camera_target_right = dosbox.mem_read_word("ds", 0x3700),
        camera_target_bottom = dosbox.mem_read_word("ds", 0x3702),
        bounds_object_offset = dosbox.mem_read_word("ds", 0x881a),
        bounds_object_flag = dosbox.mem_read_word("ds", 0x89ea),
    }
end

-- Capture the helper's indirect object lookup at the call site.  The
-- pre-update sample is useful for experiment setup, but 0x393C is the first
-- point where the guest actually consumes DS:881A and ES together.
local function bounds_object_probe(selector, offset)
    local probe = {selector = selector, offset = offset}
    local ok, raw_or_error = pcall(
        dosbox.mem_read_selector, selector, offset, 128)
    if not ok then
        probe.read_error = tostring(raw_or_error)
        return probe
    end
    local raw = raw_or_error or ""
    probe.state_hex = hex(raw)
    if #raw >= 0x34 then
        probe.fields = {
            base_x = word(raw, 0x04 + 1),
            base_y = word(raw, 0x08 + 1),
            x_left = word(raw, 0x2c + 1),
            y_bottom = word(raw, 0x2e + 1),
            x_right = word(raw, 0x30 + 1),
            y_top = word(raw, 0x32 + 1),
        }
    end
    return probe
end

local function matches_object(hit, selector, offset)
    return hit.registers and hit.registers.es == selector and
           (hit.registers.edi & 0xffff) == offset
end

local function capture_timeline(entity, object_selector, object_offset, first_capture,
                                source_selector, source_offset)
    if capture_frame_count <= 1 then return end
    entity.frames = {}
    local camera_lock_enabled = movement_camera_x >= 0 and movement_camera_y >= 0
    if camera_lock_enabled then write_movement_camera_lock() end
    local input_elapsed = 0
    local movement_pressed = false
    local return_pressed = false
    local return_start = movement_frames + 1
    if movement_key ~= "" and movement_frames > 0 then
        dosbox.key(movement_key, true)
        movement_pressed = true
    end
    for index = 0, capture_frame_count - 1 do
        local capture = nil
        if index == 0 then
            capture = first_capture
        else
            if camera_lock_enabled then
                capture = advance_locked_frames(capture_frame_step)
            else
                dosbox.wait_frames(capture_frame_step)
                capture = stop_for_capture()
            end
        end
        -- A stopped Lua breakpoint can precede the renderer's present call on
        -- the inert branch. Advance one frame and stop again so the REST video
        -- endpoint sees a fully rendered surface for both variants.
        if capture_frame_count > 1 then
            if camera_lock_enabled then
                capture = advance_locked_frames(1)
            else
                dosbox.wait_frames(1)
            end
            input_elapsed = input_elapsed + capture_frame_step + 1
            if movement_pressed and input_elapsed >= movement_frames then
                dosbox.key(movement_key, false)
                movement_pressed = false
                if return_key ~= "" and return_frames > 0 then
                    dosbox.key(return_key, true)
                    return_pressed = true
                end
            end
            if return_pressed and input_elapsed >= return_start + return_frames then
                dosbox.key(return_key, false)
                return_pressed = false
            end
            if not camera_lock_enabled then capture = stop_for_capture() end
        end
        local frame = {
            index = index,
            capture_registers = capture.registers,
        }
        if object_selector and object_offset then
            local state = dosbox.mem_read_selector(object_selector, object_offset, 64)
            frame.object_state_hex = hex(state)
            frame.object = {selector = object_selector, offset = object_offset}
            frame.position = {x = dword(state, 3) >> 16,
                              y = dword(state, 7) >> 16}
            frame.sprite_slot = word(state, 0x12 + 1)
        end
        frame.lifecycle = lifecycle_scan(source_selector, source_offset)
        entity.frames[index + 1] = frame
        entity.capture_index = index
        dosbox.output.entity = entity
        -- Hold the guest at this exact frame until Python captures the PNG
        -- and acknowledges it through /api/v1/debug/continue.
        local current = dosbox.cpu_state()
        dosbox.breakpoint_set(current.cs, current.eip, {once = true})
        wait_hit("capture acknowledgement")
    end
    if movement_pressed then dosbox.key(movement_key, false) end
    if return_pressed then dosbox.key(return_key, false) end
end

dosbox.output.awaiting_startup_replay = true
dosbox.wait_frames(350)
if select_level ~= "" then
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

    -- Enter the selector through the same cheat path as the resource
    -- tracer, then use its input wait to write the desired level index.
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
    if state_machine_sample_count > 0 and state_machine_camera_x >= 0 then
        startup_camera_x = dosbox.mem_read_word("ds", 0x81c0)
        dosbox.mem_write("ds", 0x81c0, little_word(state_machine_camera_x))
    end
    if state_machine_sample_count > 0 and state_machine_camera_y >= 0 then
        startup_camera_y = dosbox.mem_read_word("ds", 0x81c4)
        dosbox.mem_write("ds", 0x81c4, little_word(state_machine_camera_y))
    end
    dosbox.debug_continue()
    local launch = wait_hit("selector Space dispatch")
    dosbox.output.checkpoints.launch = launch
    -- The first ARE declaration can execute in the same resumed slice as the
    -- selector dispatch, so arm it before leaving the stopped breakpoint.
    dosbox.breakpoint_set(0x01f7, 0x1e04, {once = true})
    dosbox.debug_continue()
else
    dosbox.key("KBD_space", true)
    dosbox.wait_frames(4)
    dosbox.key("KBD_space", false)
end

dosbox.output.declaration_samples = {}
for attempt = 1, 4096 do
    dosbox.breakpoint_set(0x01f7, 0x1e04, {once = true})
    local entry = wait_hit("ARE declaration")
    local r = entry.registers
    local record = dosbox.mem_read("fs", r.ebx & 0xffff, 6)
    local entity_type = word(record, 1) & 0xff
    if #dosbox.output.declaration_samples < 128 then
        dosbox.output.declaration_samples[#dosbox.output.declaration_samples + 1] = {
            attempt = attempt,
            runtime_offset = r.ebx & 0xffff,
            entity_type = entity_type,
            record_hex = hex(record),
            registers = r,
        }
    end
    if (r.ebx & 0xffff) == runtime_offset then
        assert(entity_type == expected_type,
               string.format("record type %02x, expected %02x",
                             entity_type, expected_type))
        local local_x = word(record, 3)
        local local_y = word(record, 5)
        local origin_x = dosbox.mem_read_word("ds", 0x3714)
        local origin_y = dosbox.mem_read_word("ds", 0x3716)
        local dispatch = dosbox.mem_read("ds", 0x81d2 + entity_type * 4, 4)
        local sprite_animation_tables = nil
        if entity_type >= 0x29 and entity_type <= 0x2b then
            sprite_animation_tables = {
                positive = {offset = 0x3312,
                            raw_hex = hex(dosbox.mem_read("ds", 0x3312, 20))},
                nonpositive = {offset = 0x3326,
                               raw_hex = hex(dosbox.mem_read("ds", 0x3326, 20))},
            }
        end

        local dedicated_handlers = {
            [0x65] = 0x178d, [0x66] = 0x1798, [0x67] = 0x17a3,
        }
        local dedicated_handler = dedicated_handlers[entity_type]
        if dedicated_handler then
            dosbox.breakpoint_set(0x01f7, dedicated_handler, {once = true})
            dosbox.debug_continue()
            local handler_hit = wait_hit("dedicated ARE handler")
            assert(handler_hit.segment == 0x01f7 and
                   handler_hit.offset == dedicated_handler,
                   "unexpected dedicated handler breakpoint")
            local dedicated_returns = {
                [0x65] = 0x1797, [0x66] = 0x17a2, [0x67] = 0x17ad,
            }
            local dedicated_return = dedicated_returns[entity_type]
            dosbox.breakpoint_set(0x01f7, 0x1749, {once = true})
            dosbox.breakpoint_set(0x01f7, dedicated_return, {once = true})
            dosbox.debug_continue()
            local creator_hit = wait_hit("dedicated common creator")
            assert(creator_hit.segment == 0x01f7 and
                   creator_hit.offset == 0x1749,
                   "unexpected dedicated common creator breakpoint")
            local creator_queue_before = dedicated_event_queue()
            dosbox.debug_continue()
            local return_hit = wait_hit("dedicated handler return")
            assert(return_hit.segment == 0x01f7 and
                   return_hit.offset == dedicated_return,
                   "unexpected dedicated handler return breakpoint")
            local creator_queue_after = dedicated_event_queue()
            dosbox.breakpoint_set(0x01f7, 0x1892, {once = true})
            dosbox.debug_continue()
            local render_call = wait_hit("dedicated event renderer")
            assert(render_call.segment == 0x01f7 and render_call.offset == 0x1892,
                   "unexpected dedicated event renderer breakpoint")
            dosbox.breakpoint_set(0x01f7, 0x171c, {once = true})
            dosbox.breakpoint_set(0x01f7, 0x1897, {once = true})
            dosbox.debug_continue()
            local render_object_hit = wait_hit("dedicated event object allocation")
            assert(render_object_hit.segment == 0x01f7 and
                   render_object_hit.offset == 0x171c,
                   "unexpected dedicated event object allocation breakpoint")
            local event_object_selector = render_object_hit.registers.es
            local event_object_offset = render_object_hit.registers.edi & 0xffff
            local event_animation_lookup = nil
            local event_animation_candidates = {}
            dosbox.breakpoint_set(0x01f7, 0x1186, {once = true})
            dosbox.debug_continue()
            local render_return = nil
            for attempt = 1, 64 do
                local candidate = dosbox.wait_for_breakpoint(1000)
                if not candidate then break end
                if #event_animation_candidates < 16 then
                    event_animation_candidates[#event_animation_candidates + 1] = {
                        segment = candidate.segment,
                        offset = candidate.offset,
                        registers = candidate.registers,
                    }
                end
                if candidate.segment == 0x01f7 and candidate.offset == 0x1897 then
                    render_return = candidate
                    break
                end
                if candidate.segment == 0x01f7 and candidate.offset == 0x1186 and
                        candidate.registers.es == event_object_selector and
                        (candidate.registers.edi & 0xffff) == event_object_offset then
                    local animation_offset = candidate.registers.ebx & 0xffff
                    local animation_bytes = dosbox.mem_read(
                        "fs", animation_offset, 0x100) or ""
                    event_animation_lookup = {
                        object = {selector = event_object_selector,
                                  offset = event_object_offset},
                        selector = candidate.registers.fs,
                        offset = animation_offset,
                        raw_length = #animation_bytes,
                        raw_prefix = byte_prefix(animation_bytes, 32),
                        raw_bytes = byte_prefix(animation_bytes, #animation_bytes),
                        breakpoint = {segment = candidate.segment,
                                      offset = candidate.offset},
                    }
                end
                if candidate.segment == 0x01f7 and candidate.offset == 0x1186 then
                    dosbox.breakpoint_set(0x01f7, 0x1897, {once = true})
                else
                    dosbox.breakpoint_set(0x01f7, 0x1186, {once = true})
                end
                dosbox.debug_continue()
            end
            assert(render_return ~= nil,
                   "dedicated event renderer return did not execute")
            local event_object_state_before_update = dosbox.mem_read_selector(
                event_object_selector, event_object_offset, 64)
            local event_object_x = word(event_object_state_before_update, 5)
            local event_object_y = word(event_object_state_before_update, 9)
            local saved_event_camera_x = dosbox.mem_read_word("ds", 0x81c0)
            local saved_event_camera_y = dosbox.mem_read_word("ds", 0x81c4)
            dosbox.mem_write("ds", 0x81c0, little_word(event_object_x))
            dosbox.mem_write("ds", 0x81c4, little_word(event_object_y))
            local event_update_candidates = {}
            local event_update_lookup = nil
            local event_effect_call = nil
            dosbox.breakpoint_set(0x01f7, 0x10b5, {once = true})
            dosbox.breakpoint_set(0x01f7, 0x1186, {once = true})
            dosbox.breakpoint_set(0x01f7, 0x1693, {once = true})
            dosbox.debug_continue()
            for attempt = 1, 32 do
                local candidate = dosbox.wait_for_breakpoint(500)
                if not candidate then break end
                if #event_update_candidates < 32 then
                    event_update_candidates[#event_update_candidates + 1] = {
                        segment = candidate.segment,
                        offset = candidate.offset,
                        registers = candidate.registers,
                    }
                end
                local candidate_offset = candidate.registers.edi & 0xffff
                local candidate_base = candidate.registers.ebp & 0xffff
                local matches_event_object = candidate.registers.es == event_object_selector and
                    (candidate_offset == event_object_offset or
                     candidate_base == event_object_offset)
                if candidate.segment == 0x01f7 and candidate.offset == 0x10b5 and
                        matches_event_object and event_update_lookup == nil then
                    event_update_lookup = {
                        selector = candidate.registers.es,
                        offset = event_object_offset,
                        breakpoint = {segment = candidate.segment,
                                      offset = candidate.offset},
                        registers = candidate.registers,
                    }
                elseif candidate.segment == 0x01f7 and candidate.offset == 0x1186 and
                        matches_event_object and event_animation_lookup == nil then
                    local animation_offset = candidate.registers.ebx & 0xffff
                    local animation_bytes = dosbox.mem_read(
                        "fs", animation_offset, 0x100) or ""
                    event_animation_lookup = {
                        object = {selector = event_object_selector,
                                  offset = event_object_offset},
                        selector = candidate.registers.fs,
                        offset = animation_offset,
                        raw_length = #animation_bytes,
                        raw_prefix = byte_prefix(animation_bytes, 32),
                        raw_bytes = byte_prefix(animation_bytes, #animation_bytes),
                        breakpoint = {segment = candidate.segment,
                                      offset = candidate.offset},
                    }
                elseif candidate.segment == 0x01f7 and candidate.offset == 0x1693 and
                        matches_event_object and event_effect_call == nil then
                    event_effect_call = {
                        breakpoint = {segment = candidate.segment,
                                      offset = candidate.offset},
                        registers = candidate.registers,
                    }
                end
                if candidate.segment == 0x01f7 and candidate.offset == 0x10b5 then
                    dosbox.breakpoint_set(0x01f7, 0x1186, {once = true})
                    dosbox.breakpoint_set(0x01f7, 0x1693, {once = true})
                elseif candidate.segment == 0x01f7 and candidate.offset == 0x1186 then
                    dosbox.breakpoint_set(0x01f7, 0x10b5, {once = true})
                    dosbox.breakpoint_set(0x01f7, 0x1693, {once = true})
                else
                    dosbox.breakpoint_set(0x01f7, 0x10b5, {once = true})
                    dosbox.breakpoint_set(0x01f7, 0x1186, {once = true})
                end
                dosbox.debug_continue()
            end
            dosbox.mem_write("ds", 0x81c0, little_word(saved_event_camera_x))
            dosbox.mem_write("ds", 0x81c4, little_word(saved_event_camera_y))
            local creator_queue_after_render = dedicated_event_queue()
            local event_object_state = dosbox.mem_read_selector(
                event_object_selector, event_object_offset, 64)
            dosbox.output.entity = {
                type = entity_type,
                record_offset = record_offset,
                runtime_record = {selector = r.fs, offset = r.ebx & 0xffff},
                local_position = {x = local_x, y = local_y},
                region_origin = {x = origin_x, y = origin_y},
                world_position = {x = origin_x + local_x, y = origin_y + local_y},
                dedicated_handler = {
                    segment = handler_hit.segment, offset = handler_hit.offset,
                },
                common_creator = {segment = 0x01f7, offset = 0x1749},
                entry_registers = r,
                handler_registers = handler_hit.registers,
                common_creator_registers = creator_hit.registers,
                handler_return = {
                    segment = return_hit.segment, offset = return_hit.offset,
                },
                handler_return_registers = return_hit.registers,
                event_queue_before_creator = creator_queue_before,
                event_queue_after_creator = creator_queue_after,
                event_renderer_call = {
                    segment = render_call.segment, offset = render_call.offset,
                    registers = render_call.registers,
                },
                event_object_allocation = {
                    segment = render_object_hit.segment,
                    offset = render_object_hit.offset,
                    registers = render_object_hit.registers,
                },
                event_renderer_return = {
                    segment = render_return.segment, offset = render_return.offset,
                    registers = render_return.registers,
                },
                event_queue_after_render = creator_queue_after_render,
                event_object = {
                    selector = event_object_selector,
                    offset = event_object_offset,
                    initial_state_hex = hex(event_object_state_before_update),
                    initial_x = event_object_x,
                    initial_y = event_object_y,
                    state_hex = hex(event_object_state),
                    sprite_slot = word(event_object_state, 0x12 + 1),
                    update_callback = word(event_object_state, 0x18 + 1),
                    state_field = word(event_object_state, 0x2e + 1),
                },
                animation_lookup = event_animation_lookup,
                animation_candidates = event_animation_candidates,
                update_lookup = event_update_lookup,
                update_candidates = event_update_candidates,
                effect_call = event_effect_call,
                camera_override = {
                    before = {x = saved_event_camera_x, y = saved_event_camera_y},
                    applied = {x = event_object_x, y = event_object_y},
                },
                record_hex = hex(record),
            }
            return
        end

        if entity_type == 0 then
            dosbox.debug_continue()
            dosbox.wait_frames(1)
            dosbox.wait_frames(capture_delay_frames)
            local capture = stop_for_capture()
            local removed_entity = {
                type = entity_type,
                record_offset = record_offset,
                runtime_record = {selector = r.fs, offset = r.ebx & 0xffff},
                local_position = {x = local_x, y = local_y},
                region_origin = {x = origin_x, y = origin_y},
                world_position = {x = origin_x + local_x, y = origin_y + local_y},
                removed = true,
                entry_registers = r,
                capture_registers = capture.registers,
                record_hex = hex(record),
            }
            capture_timeline(removed_entity, nil, nil, capture)
            if capture_frame_count <= 1 then dosbox.wait_frames(1) end
            dosbox.output.entity = removed_entity
            return
        end

        local sprite_initialization = nil
        local lifetime_samples = {}
        if entity_type >= 0x29 and entity_type <= 0x2b then
            -- The leaf update initializes the visible sprite through a far
            -- helper call. At 474D ES:DI still identifies the ARE object.
            dosbox.breakpoint_set(0x01f7, 0x474d, {once = true})
        elseif sprite_init_offset ~= 0 then
            dosbox.breakpoint_set(0x01f7, sprite_init_offset, {once = true})
        end
        dosbox.breakpoint_set(0x01f7, 0x1e8e, {once = true})
        dosbox.debug_continue()
        local returned = wait_hit("object factory return")
        assert(returned.segment == 0x01f7 and returned.offset == 0x1e8e,
               "unexpected factory return breakpoint")
        local after = returned.registers
        local object_selector = after.es
        local object_offset = after.edi & 0xffff
        dosbox.debug_continue()
        if entity_type >= 0x29 and entity_type <= 0x2b then
            local initialized = wait_hit("leaf sprite initialization")
            assert(initialized.segment == 0x01f7 and initialized.offset == 0x474d,
                   "unexpected sprite-initialization breakpoint")
            local initialized_selector = initialized.registers.es
            local initialized_offset = initialized.registers.edi & 0xffff
            local initialized_state = dosbox.mem_read_selector(
                initialized_selector, initialized_offset, 64)
                sprite_initialization = {
                selector = initialized_selector,
                offset = initialized_offset,
                same_as_entity_object = initialized_selector == object_selector and
                                        initialized_offset == object_offset,
                sprite_slot = word(initialized_state, 0x12 + 1),
                state_hex = hex(initialized_state),
                breakpoint = {segment = 0x01f7, offset = 0x474d},
            }
            local attempts = 0
            local emulator_running = false
            while #lifetime_samples < lifetime_sample_count and attempts < 8192 do
                attempts = attempts + 1
                dosbox.breakpoint_set(0x01f7, 0x47e7, {once = true})
                if not emulator_running then
                    dosbox.debug_continue()
                    emulator_running = true
                end
                local updated = wait_hit("sprite lifetime update")
                emulator_running = false
                local rr = updated.registers
                local update_selector = rr.es
                local update_offset = rr.edi & 0xffff
                local update_state = dosbox.mem_read_selector(
                    update_selector, update_offset, 52)
                lifetime_samples[#lifetime_samples + 1] = {
                    sequence = #lifetime_samples + 1,
                    object = {selector = update_selector,
                              offset = update_offset},
                    matches_entity_object = update_selector == object_selector and
                                            update_offset == object_offset,
                    slot = word(update_state, 0x12 + 1),
                    animation_delay = word(update_state, 0x20 + 1),
                    animation_cursor = word(update_state, 0x24 + 1),
                    variant_flag = string.byte(update_state, 0x28 + 1),
                }
                -- Step past the current callback entry before re-arming it.
                dosbox.breakpoint_set(0x01f7, 0x47ec, {once = true})
                dosbox.debug_continue()
                local advanced = wait_hit("sprite update step-over")
                assert(advanced.segment == 0x01f7 and advanced.offset == 0x47ec,
                       "unexpected sprite update step-over breakpoint")
                emulator_running = false
            end
            assert(#lifetime_samples == lifetime_sample_count,
                   "lifetime sample limit exceeded")
            if not emulator_running then dosbox.debug_continue() end
        elseif sprite_init_offset ~= 0 then
            local initialized = wait_hit("sprite initializer")
            assert(initialized.segment == 0x01f7 and
                   initialized.offset == sprite_init_offset,
                   "unexpected sprite initializer breakpoint")
            local initialized_selector = initialized.registers.es
            local initialized_offset = initialized.registers.edi & 0xffff
            local initialized_state = dosbox.mem_read_selector(
                initialized_selector, initialized_offset, 64)
            sprite_initialization = {
                selector = initialized_selector,
                offset = initialized_offset,
                same_as_entity_object = initialized_selector == object_selector and
                                        initialized_offset == object_offset,
                sprite_slot = word(initialized_state, 0x12 + 1),
                state_hex = hex(initialized_state),
                    breakpoint = {segment = 0x01f7, offset = sprite_init_offset},
                    sprite_map = sprite_map_probe(
                        word(initialized_state, 0x12 + 1)
                    ),
                    sprite_descriptor = sprite_descriptor_probe(
                        word(initialized_state, 0x12 + 1)
                    ),
                }
            dosbox.debug_continue()
        end
        local state_machine_samples = {}
        local state_machine_object_updates = {}
        local state_machine_callback_candidates = {}
        local state_machine_emitted_objects = {}
        if entity_type >= 0x1f and entity_type <= 0x21 and
                state_machine_sample_count > 0 then
            local saved_camera_x = nil
            local saved_camera_y = nil
            local saved_bounds_bytes = nil
            local saved_position_bytes = nil
            local bounds_object_offset = dosbox.mem_read_word("ds", 0x881a)
            local initial_state = dosbox.mem_read_selector(
                object_selector, object_offset, 64)
            local probe_position_x = dword(initial_state, 3) >> 16
            local probe_position_y = dword(initial_state, 7) >> 16
            if state_machine_position_x >= 0 then
                saved_position_bytes = dosbox.mem_read_selector(
                    object_selector, object_offset + 0x02, 8)
                dosbox.mem_write_selector(
                    object_selector, object_offset + 0x02,
                    little_word(0) .. little_word(state_machine_position_x))
                dosbox.mem_write_selector(
                    object_selector, object_offset + 0x06,
                    little_word(0) .. little_word(state_machine_position_y))
                probe_position_x = state_machine_position_x
                probe_position_y = state_machine_position_y
            end
            -- Apply the camera override before waiting for the first
            -- state-machine entry.  The ordinary object callback performs
            -- its visibility gate before 0x8E4B, so a far-away controlled
            -- position would otherwise never reach the breakpoint that is
            -- supposed to install this override.
            if state_machine_camera_x >= 0 then
                saved_camera_x = startup_camera_x or dosbox.mem_read_word("ds", 0x81c0)
                dosbox.mem_write("ds", 0x81c0,
                                 little_word(state_machine_camera_x))
            end
            if state_machine_camera_y >= 0 then
                saved_camera_y = startup_camera_y or dosbox.mem_read_word("ds", 0x81c4)
                dosbox.mem_write("ds", 0x81c4,
                                 little_word(state_machine_camera_y))
            end
            for sequence = 1, state_machine_sample_count do
                dosbox.breakpoint_set(0x01f7, 0x8e4b, {once = true})
                dosbox.debug_continue()
                local update_entry = nil
                while true do
                    local candidate = wait_hit("state-machine update entry")
                    if candidate.segment == 0x01f7 and
                            candidate.offset == 0x8e4b and
                            matches_object(candidate, object_selector, object_offset) then
                        update_entry = candidate
                        break
                    end
                    dosbox.breakpoint_set(0x01f7, 0x8e4b, {once = true})
                    dosbox.debug_continue()
                end
                local state = dosbox.mem_read_selector(
                    object_selector, object_offset, 64)
                local bounds_object_state = dosbox.mem_read_selector(
                    object_selector, bounds_object_offset, 128)
                local tile_effect_table = state_machine_tile_effects()
                local map_run_base = state_machine_patch_map_run and
                    state_machine_effect_run_base() or nil
                local map_run_patch = patch_state_machine_map_run(
                    probe_position_x, probe_position_y, map_run_base)
                local sample = {
                    sequence = sequence,
                    breakpoint = {segment = update_entry.segment,
                                  offset = update_entry.offset},
                    update_entry_registers = update_entry.registers,
                    object_state_hex = hex(state),
                    state_field = word(state, 0x2e + 1),
                    update_state = word(state, 0x32 + 1),
                    sprite_slot = word(state, 0x12 + 1),
                    position = {x = dword(state, 3) >> 16,
                                y = dword(state, 7) >> 16},
                    globals = {
                        action_word = dosbox.mem_read_word("ds", 0x612e),
                        tile_flag_word = dosbox.mem_read_word("ds", 0x60d8),
                        camera_x = dosbox.mem_read_word("ds", 0x81c0),
                        camera_y = dosbox.mem_read_word("ds", 0x81c4),
                        scratch_x = dosbox.mem_read_word("ds", 0x8828),
                        scratch_y = dosbox.mem_read_word("ds", 0x882a),
                        bounds_object_offset = bounds_object_offset,
                        bounds_object_flag = dosbox.mem_read_word("ds", 0x89ea),
                        bounds_object_state_hex = hex(bounds_object_state),
                        animation_table_offset = dosbox.mem_read_word("ds", 0x6574),
                        animation_segment_stride = dosbox.mem_read_word("ds", 0x6570),
                        animation_selector_base = dosbox.mem_read_word("ds", 0x6576),
                        position_override = (state_machine_position_x >= 0 and {
                            x = state_machine_position_x,
                            y = state_machine_position_y,
                        } or nil),
                    },
                    static_slice_globals = static_slice_globals(),
                    tile_effect_table = tile_effect_table,
                    map_run_base = map_run_base,
                    map_run_patch = map_run_patch,
                    nested_calls = {},
                }
                if state_machine_camera_x >= 0 then
                    if saved_camera_x == nil then
                        saved_camera_x = dosbox.mem_read_word("ds", 0x81c0)
                    end
                    sample.globals.camera_x_override = state_machine_camera_x
                    dosbox.mem_write("ds", 0x81c0,
                                     little_word(state_machine_camera_x))
                end
                if state_machine_camera_y >= 0 then
                    if saved_camera_y == nil then
                        saved_camera_y = dosbox.mem_read_word("ds", 0x81c4)
                    end
                    sample.globals.camera_y_override = state_machine_camera_y
                    dosbox.mem_write("ds", 0x81c4,
                                     little_word(state_machine_camera_y))
                end
                if state_machine_force_emission then
                    if saved_bounds_bytes == nil then
                        saved_bounds_bytes = dosbox.mem_read_selector(
                            object_selector, bounds_object_offset + 0x2c, 8)
                    end
                    local bounds_x = dword(bounds_object_state, 3) >> 16
                    local bounds_y = dword(bounds_object_state, 7) >> 16
                    local left = probe_position_x - bounds_x - 200
                    local bottom = probe_position_y - bounds_y - 100
                    local right = probe_position_x - bounds_x + 400
                    local top = probe_position_y - bounds_y + 100
                    -- Widen the bounds helper's synthetic rectangle so the
                    -- callback's grid-emission path can be sampled even when
                    -- the opening replay has not moved the player there yet.
                    dosbox.mem_write_selector(
                        object_selector, bounds_object_offset + 0x2c,
                        little_word(left))
                    dosbox.mem_write_selector(
                        object_selector, bounds_object_offset + 0x2e,
                        little_word(bottom))
                    dosbox.mem_write_selector(
                        object_selector, bounds_object_offset + 0x30,
                        little_word(right))
                    dosbox.mem_write_selector(
                        object_selector, bounds_object_offset + 0x32,
                        little_word(top))
                    sample.globals.bounds_override = {
                        left = left,
                        bottom = bottom,
                        right = right,
                        top = top,
                    }
                end
                sample.armed_breakpoints = arm_state_machine_breakpoints()
                dosbox.debug_continue()
                local update_exit = nil
                while true do
                    local nested = wait_hit("state-machine update event")
                    if nested.offset == 0x8e78 or nested.offset == 0x8e85 or
                            nested.offset == 0x9254 or nested.offset == 0x9255 then
                        if not matches_object(nested, object_selector, object_offset) then
                            dosbox.breakpoint_set(0x01f7, nested.offset, {once = true})
                            dosbox.debug_continue()
                        else
                            update_exit = nested
                            -- 0x9254 is the final `pop bx` on the state-4/6/8/10
                            -- paths and 0x9255 is the common return.  Keep both
                            -- as exits because either boundary can be reported
                            -- depending on the debugger's instruction timing.
                            break
                        end
                    elseif nested.offset == 0x16ce or nested.offset == 0x3376 or
                            nested.offset == 0x171c or nested.offset == 0x393c or
                            nested.offset == 0x8eb5 then
                        if nested.offset ~= 0x171c and
                                not matches_object(nested, object_selector, object_offset) then
                            dosbox.breakpoint_set(0x01f7, nested.offset, {once = true})
                            dosbox.debug_continue()
                        elseif nested.offset == 0x8eb5 then
                            sample.state_write = {
                                segment = nested.segment,
                                offset = nested.offset,
                                registers = nested.registers,
                                object_state_hex = hex(dosbox.mem_read_selector(
                                    object_selector, object_offset, 64)),
                            }
                            -- The write instruction jumps directly to the
                            -- common return path; leave the other breakpoints
                            -- armed and let the exit handler observe it.
                            dosbox.debug_continue()
                        else
                            local nested_stack = dosbox.mem_read(
                                "ss", nested.registers.esp & 0xffff, 16) or ""
                            local call = {
                                target = {segment = nested.segment, offset = nested.offset},
                                registers = nested.registers,
                                stack_hex = hex(nested_stack),
                                return_offset = (#nested_stack >= 2 and word(nested_stack, 1) or nil),
                                return_segment = (#nested_stack >= 4 and word(nested_stack, 3) or nil),
                                arguments = {
                                    ax = nested.registers.eax & 0xffff,
                                    bx = nested.registers.ebx & 0xffff,
                                    cx = nested.registers.ecx & 0xffff,
                                    dx = nested.registers.edx & 0xffff,
                                },
                            }
                            if nested.offset == 0x3376 then
                                local pointer = dword(
                                    dosbox.mem_read("ds", 0x657a, 4), 1)
                                local map_base = pointer & 0xffff
                                local map_selector = (pointer >> 16) & 0xffff
                                local row_stride = dosbox.mem_read_word("ds", 0x657e)
                                local map_offset = map_base +
                                    ((nested.registers.eax & 0xffff) >> 4) * row_stride +
                                    ((nested.registers.ebx & 0xffff) >> 4) * 2
                                call.map_pointer = {
                                    selector = map_selector,
                                    offset = map_offset,
                                    register_offset = nested.registers.esi & 0xffff,
                                    word = dosbox.mem_read_word(
                                        map_selector, map_offset),
                                }
                            end
                            if nested.offset == 0x393c then
                                local bounds_offset = dosbox.mem_read_word(
                                    "ds", 0x881a)
                                call.bounds_lookup = bounds_object_probe(
                                    nested.registers.es, bounds_offset)
                                call.bounds_lookup.ds = nested.registers.ds
                                call.bounds_lookup.global_flag = dosbox.mem_read_word(
                                    "ds", 0x89ea)
                                call.bounds_lookup.entry_di = nested.registers.edi & 0xffff
                                call.bounds_lookup.entry_di_probe = bounds_object_probe(
                                    nested.registers.es,
                                    nested.registers.edi & 0xffff)
                            end
                            if nested.offset == 0x171c then
                                local nested_selector = nested.registers.es
                                local nested_offset = nested.registers.edi & 0xffff
                                local read_ok, nested_state = pcall(
                                    dosbox.mem_read_selector,
                                    nested_selector, nested_offset, 64)
                                if read_ok then
                                    call.object = {
                                        selector = nested_selector,
                                        offset = nested_offset,
                                        state_hex = hex(nested_state),
                                        sprite_slot = word(nested_state, 0x12 + 1),
                                        update_callback = word(nested_state, 0x18 + 1),
                                        state_field = word(nested_state, 0x2e + 1),
                                    }
                                else
                                    call.object_read_error = tostring(nested_state)
                                end
                            end
                            sample.nested_calls[#sample.nested_calls + 1] = call
                            assert(call.return_offset and call.return_segment,
                                   "state-machine nested call has no far return address")
                            dosbox.breakpoint_set(call.return_segment, call.return_offset,
                                                  {once = true})
                            dosbox.debug_continue()
                            local nested_return = nil
                            local internal_creator_call = nil
                            while true do
                                local candidate_return = wait_hit(
                                    "state-machine nested return")
                                if candidate_return.segment == call.return_segment and
                                        candidate_return.offset == call.return_offset then
                                    nested_return = candidate_return
                                    break
                                end
                                if nested.offset == 0x16ce and
                                        candidate_return.offset == 0x171c then
                                    -- 0x16CE calls the common creator at 0x10B5;
                                    -- 0x171C is the creator's internal continuation.
                                    -- Capture the object there, then continue
                                    -- waiting for the outer 0x16CE return. It is
                                    -- not itself a far-call entry, so its stack
                                    -- words are not a return address to decode.
                                    local creator_call = {
                                        target = {segment = candidate_return.segment,
                                                  offset = candidate_return.offset},
                                        registers = candidate_return.registers,
                                        arguments = {
                                            ax = candidate_return.registers.eax & 0xffff,
                                            bx = candidate_return.registers.ebx & 0xffff,
                                            cx = candidate_return.registers.ecx & 0xffff,
                                            dx = candidate_return.registers.edx & 0xffff,
                                        },
                                    }
                                    local creator_selector = candidate_return.registers.es
                                    local creator_offset = candidate_return.registers.edi & 0xffff
                                    local creator_state = dosbox.mem_read_selector(
                                        creator_selector, creator_offset, 64)
                                    creator_call.object = {
                                        selector = creator_selector,
                                        offset = creator_offset,
                                        state_hex = hex(creator_state),
                                        sprite_slot = word(creator_state, 0x12 + 1),
                                        update_callback = word(creator_state, 0x18 + 1),
                                        state_field = word(creator_state, 0x2e + 1),
                                        position = {
                                            x = dword(creator_state, 3) >> 16,
                                            y = dword(creator_state, 7) >> 16,
                                        },
                                        internal_continuation = true,
                                    }
                                    if state_machine_force_emission then
                                        -- Native effect objects live for only
                                        -- three update ticks. Keep synthetic
                                        -- objects alive long enough to observe
                                        -- their ordinary update/visibility/
                                        -- renderer chain after the state probe;
                                        -- this is debugger-only object memory.
                                        creator_call.object.probe_lifetime = word(
                                            creator_state, 0x2c + 1)
                                        dosbox.mem_write_selector(
                                            creator_selector, creator_offset + 0x2c,
                                            little_word(0x40))
                                        creator_call.object.probe_lifetime_override = 0x40
                                    end
                                    sample.nested_calls[#sample.nested_calls + 1] = creator_call
                                    internal_creator_call = creator_call
                                    local already_recorded = false
                                    for _, emitted in ipairs(state_machine_emitted_objects) do
                                        if emitted.selector == creator_selector and
                                                emitted.offset == creator_offset then
                                            already_recorded = true
                                            break
                                        end
                                    end
                                    if not already_recorded then
                                        state_machine_emitted_objects[#state_machine_emitted_objects + 1] = {
                                            selector = creator_selector,
                                            offset = creator_offset,
                                        }
                                    end
                                    arm_state_machine_breakpoints(0x171c)
                                    dosbox.debug_continue()
                                else
                                    error("unexpected state-machine nested return")
                                end
                            end
                            call.return_registers = nested_return.registers
                            if internal_creator_call ~= nil then
                                local final_creator_state = dosbox.mem_read_selector(
                                    internal_creator_call.object.selector,
                                    internal_creator_call.object.offset, 64)
                                internal_creator_call.object.post_state_hex = hex(
                                    final_creator_state)
                                internal_creator_call.object.post_sprite_slot = word(
                                    final_creator_state, 0x12 + 1)
                                internal_creator_call.object.post_update_callback = word(
                                    final_creator_state, 0x18 + 1)
                                internal_creator_call.object.post_state_field = word(
                                    final_creator_state, 0x2e + 1)
                                internal_creator_call.object.post_lifetime = word(
                                    final_creator_state, 0x2c + 1)
                                internal_creator_call.object.post_position = {
                                    x = dword(final_creator_state, 3) >> 16,
                                    y = dword(final_creator_state, 7) >> 16,
                                }
                            end
                            arm_state_machine_breakpoints()
                            dosbox.debug_continue()
                        end
                    else
                        error("unexpected state-machine nested call")
                    end
                end
                disarm_state_machine_breakpoints()
                local post_state = dosbox.mem_read_selector(
                    object_selector, object_offset, 64)
                sample.update_exit = {
                    segment = update_exit.segment, offset = update_exit.offset,
                }
                sample.update_exit_registers = update_exit.registers
                sample.post_object_state_hex = hex(post_state)
                sample.post_state_field = word(post_state, 0x2e + 1)
                sample.post_update_state = word(post_state, 0x32 + 1)
                sample.post_sprite_slot = word(post_state, 0x12 + 1)
                sample.post_globals = {
                    action_word = dosbox.mem_read_word("ds", 0x612e),
                    tile_flag_word = dosbox.mem_read_word("ds", 0x60d8),
                    camera_x = dosbox.mem_read_word("ds", 0x81c0),
                    camera_y = dosbox.mem_read_word("ds", 0x81c4),
                    scratch_x = dosbox.mem_read_word("ds", 0x8828),
                    scratch_y = dosbox.mem_read_word("ds", 0x882a),
                }
                sample.post_static_slice_globals = static_slice_globals()
                if update_exit.offset == 0x9254 or update_exit.offset == 0x9255 then
                    dosbox.debug_continue()
                end
                state_machine_samples[#state_machine_samples + 1] = sample
            end
            if not state_machine_keep_camera then
                if saved_camera_x ~= nil then
                    dosbox.mem_write("ds", 0x81c0, little_word(saved_camera_x))
                end
                if saved_camera_y ~= nil then
                    dosbox.mem_write("ds", 0x81c4, little_word(saved_camera_y))
                end
            end
            if saved_bounds_bytes ~= nil then
                dosbox.mem_write_selector(
                    object_selector, bounds_object_offset + 0x2c,
                    saved_bounds_bytes)
            end
            if saved_position_bytes ~= nil then
                dosbox.mem_write_selector(
                    object_selector, object_offset + 0x02,
                    saved_position_bytes)
            end
            local captured_objects = {}
            local captured_count = 0
            local attempts = 0
            local callback_attempts = 0
            if #state_machine_emitted_objects > 0 then
                dosbox.breakpoint_set(0x01f7, 0x10b5, {once = true})
                dosbox.breakpoint_set(0x01f7, 0x1186, {once = true})
                dosbox.breakpoint_set(0x01f7, 0x1693, {once = true})
                dosbox.debug_continue()
            end
            while #state_machine_emitted_objects > 0 and
                    captured_count < #state_machine_emitted_objects and
                    attempts < 128 and callback_attempts < 128 do
                attempts = attempts + 1
                callback_attempts = callback_attempts + 1
                local candidate = dosbox.wait_for_breakpoint(100)
                if not candidate then break end
                local candidate_offset = candidate.registers.edi & 0xffff
                local candidate_base = candidate.registers.ebp & 0xffff
                local match = nil
                for _, emitted in ipairs(state_machine_emitted_objects) do
                    if emitted.selector == candidate.registers.es and
                            (candidate_offset == emitted.offset or
                             candidate_base == emitted.offset) then
                        match = emitted
                        break
                    end
                end
                if #state_machine_callback_candidates < 128 then
                    local callback = {
                        breakpoint = {segment = candidate.segment,
                                      offset = candidate.offset},
                        registers = candidate.registers,
                        stack_hex = hex(dosbox.mem_read(
                            "ss", candidate.registers.esp & 0xffff, 12) or ""),
                        camera = {
                            x = dosbox.mem_read_word("ds", 0x81c0),
                            y = dosbox.mem_read_word("ds", 0x81c4),
                            target_left = dosbox.mem_read_word("ds", 0x36fc),
                            target_top = dosbox.mem_read_word("ds", 0x36fe),
                            target_right = dosbox.mem_read_word("ds", 0x3700),
                            target_bottom = dosbox.mem_read_word("ds", 0x3702),
                        },
                        matches_emitted_object = (match ~= nil),
                    }
                    if match ~= nil and candidate.segment == 0x01f7 and
                            candidate.offset == 0x10b5 then
                        if state_machine_camera_x >= 0 then
                            dosbox.mem_write("ds", 0x81c0,
                                             little_word(state_machine_camera_x))
                        end
                        if state_machine_camera_y >= 0 then
                            dosbox.mem_write("ds", 0x81c4,
                                             little_word(state_machine_camera_y))
                        end
                        callback.camera_override = {
                            x = state_machine_camera_x,
                            y = state_machine_camera_y,
                        }
                    end
                    if match ~= nil then
                        local callback_state = dosbox.mem_read_selector(
                            match.selector, match.offset, 64)
                        callback.object = {
                            selector = match.selector,
                            offset = match.offset,
                            state_field = word(callback_state, 0x2e + 1),
                            lifetime = word(callback_state, 0x2c + 1),
                            position = {
                                x = dword(callback_state, 3) >> 16,
                                y = dword(callback_state, 7) >> 16,
                            },
                        }
                    end
                    if match ~= nil and candidate.segment == 0x01f7 and
                            candidate.offset == 0x1693 then
                        local stack = dosbox.mem_read(
                            "ss", candidate.registers.esp & 0xffff, 8) or ""
                        if #stack >= 4 then
                            local return_offset = word(stack, 1)
                            local return_segment = word(stack, 3)
                            callback.visibility_return = {
                                expected = {segment = return_segment,
                                            offset = return_offset},
                            }
                            dosbox.breakpoint_set(return_segment, return_offset,
                                                  {once = true})
                            dosbox.debug_continue()
                            local visibility_return = wait_hit(
                                "state-machine visibility return")
                            callback.visibility_return.actual = {
                                segment = visibility_return.segment,
                                offset = visibility_return.offset,
                            }
                            callback.visibility_return.registers =
                                visibility_return.registers
                        end
                    end
                    state_machine_callback_candidates[#state_machine_callback_candidates + 1] = callback
                end
                if match ~= nil then
                    local animation_state = dosbox.mem_read_selector(
                        match.selector, match.offset, 64)
                    local object_key = match.selector .. ":" .. match.offset
                    if candidate.segment == 0x01f7 and candidate.offset == 0x1186 and
                            not captured_objects[object_key] then
                        local animation_selector = candidate.registers.fs
                        local animation_offset = candidate.registers.ebx & 0xffff
                        local animation_bytes = dosbox.mem_read(
                            "fs", animation_offset, 0x100) or ""
                        state_machine_object_updates[#state_machine_object_updates + 1] = {
                            object = {selector = match.selector, offset = match.offset},
                            state_field = word(animation_state, 0x2e + 1),
                            lifetime = word(animation_state, 0x2c + 1),
                            position = {
                                x = dword(animation_state, 3) >> 16,
                                y = dword(animation_state, 7) >> 16,
                            },
                            update_callback = word(animation_state, 0x18 + 1),
                            sprite_slot = word(animation_state, 0x12 + 1),
                            animation_lookup = {
                                selector = animation_selector,
                                offset = animation_offset,
                                raw_length = #animation_bytes,
                                raw_prefix = byte_prefix(animation_bytes, 32),
                                raw_bytes = byte_prefix(animation_bytes, #animation_bytes),
                            },
                        }
                        captured_objects[object_key] = true
                        captured_count = captured_count + 1
                    end
                end
                if candidate.segment == 0x01f7 and candidate.offset == 0x10b5 then
                    dosbox.breakpoint_set(0x01f7, 0x1186, {once = true})
                    dosbox.breakpoint_set(0x01f7, 0x1693, {once = true})
                elseif candidate.segment == 0x01f7 and candidate.offset == 0x1186 then
                    dosbox.breakpoint_set(0x01f7, 0x10b5, {once = true})
                    dosbox.breakpoint_set(0x01f7, 0x1693, {once = true})
                else
                    dosbox.breakpoint_set(0x01f7, 0x10b5, {once = true})
                    dosbox.breakpoint_set(0x01f7, 0x1186, {once = true})
                end
                dosbox.debug_continue()
            end
        end
        dosbox.wait_frames(1 + capture_delay_frames)
        local capture = stop_for_capture()
        local object_state = dosbox.mem_read_selector(
            object_selector, object_offset, 64)
        local sprite_slot = word(object_state, 0x12 + 1)
        local normal_entity = {
            trace_schema_version = trace_config.schema_version or 1,
            type = entity_type,
            record_offset = record_offset,
            runtime_record = {selector = r.fs, offset = r.ebx & 0xffff},
            local_position = {x = local_x, y = local_y},
            region_origin = {x = origin_x, y = origin_y},
            world_position = {x = origin_x + local_x, y = origin_y + local_y},
            dispatch_slot = 0x81d2 + entity_type * 4,
            dispatch_hex = hex(dispatch),
            dispatch_word = word(dispatch, 1),
            object_class = string.byte(dispatch, 3),
            dispatch_reserved = string.byte(dispatch, 4),
            update_callback = {
                runtime_segment = 0x01f7,
                offset = word(dispatch, 1),
            },
            factory = {segment = 0x01f7, offset = 0x0e06},
            factory_return = {segment = 0x01f7, offset = 0x1e8e},
            object = {selector = object_selector, offset = object_offset},
            sprite_initialization = sprite_initialization,
            sprite_animation_tables = sprite_animation_tables,
            lifetime_samples = lifetime_samples,
            state_machine_samples = state_machine_samples,
            state_machine_object_updates = state_machine_object_updates,
            state_machine_callback_candidates = state_machine_callback_candidates,
            sprite_slot = sprite_slot,
            sprite_slot_field_offset = 0x12,
            initialized_position = {
                x_fixed = dword(object_state, 3),
                y_fixed = dword(object_state, 7),
                x = dword(object_state, 3) >> 16,
                y = dword(object_state, 7) >> 16,
            },
            object_state_hex = hex(object_state),
            entry_registers = r,
            return_registers = after,
            capture_registers = capture.registers,
            record_hex = hex(record),
        }
        local tracked_source_offset = word(object_state, 0x1a + 1)
        normal_entity.source = {
            selector = r.fs,
            offset = tracked_source_offset,
        }
        capture_timeline(normal_entity, object_selector, object_offset, capture,
                         r.fs, tracked_source_offset)
        if capture_frame_count <= 1 then dosbox.wait_frames(1) end
        dosbox.output.entity = normal_entity
        return
    end
    dosbox.debug_continue()
    dosbox.wait_frames(1)
end

error(string.format("record 0x%x was not instantiated", record_offset))
