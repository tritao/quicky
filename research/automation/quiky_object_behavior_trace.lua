-- Trace one normal ARE object's update callback without entering the player
-- callback or the generic entity/resource tracer.
local trace_config = TRACE_CONFIG or {}
assert(type(trace_config) == "table", "TRACE_CONFIG must be a table")

local timeout_ms = trace_config.timeout_ms or 30000
local record_offset = trace_config.record_offset or 0x1792
local expected_type = trace_config.entity_type or 0x2b
local sample_count = trace_config.samples or 32
local select_level = trace_config.select_level or ""
local selector_frames = trace_config.selector_frames or 60
local camera_x = trace_config.camera_x or -1
local camera_y = trace_config.camera_y or -1
local followup_passes = trace_config.followup_passes or 0
local reactivate_camera_x = trace_config.reactivate_camera_x or -1
local reactivate_camera_y = trace_config.reactivate_camera_y or -1
local runtime_offset = record_offset - 0x160
local class_entry_offsets = {0x0ec7, 0x0eee, 0x0f14}
local class_loop_offsets = {[0] = 0x0eba, [1] = 0x0ee4, [2] = 0x0f0d}
local class_return_offsets = {[0] = 0x0ed3, [1] = 0x0efd, [2] = 0x0f26}
local class_post_offsets = {[0] = 0x0edb, [1] = 0x0f05, [2] = 0x0f2e}
local callback_related_offsets = {
    0x1dca, 0x1dee, 0x106a, 0x1036, 0x0fa2,
    0x8e4b, 0x8e78, 0x8e85, 0x9254, 0x9255,
}
local callback_step_offsets = {
    [0x47e7] = 0x47ec,
    [0x9269] = 0x926e,
    [0x8d20] = 0x8d25,
    [0x882f] = 0x8834,
    [0x9c0c] = 0x9c11,
}

local function word(s, index)
    local lo, hi = string.byte(s, index, index + 1)
    return (lo or 0) | ((hi or 0) << 8)
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

local function wait_hit(label)
    local hit, err = dosbox.wait_for_breakpoint(timeout_ms)
    if not hit then error(label .. ": " .. (err or "timeout")) end
    return hit
end

local function object_snapshot(selector, offset)
    local raw = dosbox.mem_read_selector(selector, offset, 64)
    return {
        selector = selector,
        offset = offset,
        raw_hex = hex(raw),
        position = {
            x_fixed = dword(raw, 3),
            y_fixed = dword(raw, 7),
            x = dword(raw, 3) >> 16,
            y = dword(raw, 7) >> 16,
        },
        lifetime = word(raw, 0x2c + 1),
        sprite_slot = word(raw, 0x12 + 1),
        update_callback = word(raw, 0x18 + 1),
        source_offset = word(raw, 0x1a + 1),
        state_field = word(raw, 0x2e + 1),
        update_state = word(raw, 0x32 + 1),
        object_class = string.byte(raw, 0x17 + 1),
    }
end

local function source_snapshot(selector, offset)
    if offset == 0xffff then
        return {selector = selector, offset = offset, absent = true}
    end
    local ok, raw_or_error = pcall(dosbox.mem_read_selector, selector, offset, 6)
    if not ok then
        return {selector = selector, offset = offset, error = tostring(raw_or_error)}
    end
    return {
        selector = selector,
        offset = offset,
        raw_hex = "hex:" .. hex(raw_or_error),
        marker_word = word(raw_or_error, 1),
        processed_marker = string.byte(raw_or_error, 2),
    }
end

local function lifecycle_globals()
    return {
        camera_x = dosbox.mem_read_word("ds", 0x81c0),
        camera_y = dosbox.mem_read_word("ds", 0x81c4),
        object_pool_selector = dosbox.mem_read_word("ds", 0x7560),
        object_array_offset = dosbox.mem_read_word("ds", 0x755e),
        scheduler_bank = dosbox.mem_read_word("ds", 0x7966),
        scheduler_active_count = dosbox.mem_read_word("ds", 0x88c8),
        bounds_object_offset = dosbox.mem_read_word("ds", 0x881a),
        bounds_object_flag = dosbox.mem_read_word("ds", 0x89ea),
        stream_camera_x = dosbox.mem_read_word("ds", 0x3710),
        stream_camera_y = dosbox.mem_read_word("ds", 0x3712),
        stream_region_x = dosbox.mem_read_word("ds", 0x3714),
        stream_region_y = dosbox.mem_read_word("ds", 0x3716),
        stream_grid_selector = dosbox.mem_read_word("ds", 0x796e),
        stream_grid_offset = dosbox.mem_read_word("ds", 0x796c),
        stream_grid_stride = dosbox.mem_read_word("ds", 0x7968),
        stream_grid_limit = dosbox.mem_read_word("ds", 0x7972),
    }
end

local function pool_snapshot()
    local pool_selector = dosbox.mem_read_word("ds", 0x7560)
    local banks = {}
    for bank = 0, 1 do
        local base = 0x7566 + bank * 0x200
        local raw = dosbox.mem_read("ds", base, 0x200) or ""
        local entries = {}
        for index = 0, math.min(#raw // 8, 64) - 1 do
            local callback = word(raw, index * 8 + 1)
            local callback_segment = word(raw, index * 8 + 3)
            local object_offset = word(raw, index * 8 + 5)
            local object_selector = word(raw, index * 8 + 7)
            if callback == 0xffff then
                break
            end
            if callback ~= 0 then
                local entry = {
                    bank = bank,
                    index = index,
                    table_offset = base + index * 8,
                    callback = callback,
                    callback_selector = callback_segment,
                    object_offset = object_offset,
                    object_selector = object_selector,
                }
                if object_offset ~= 0xffff then
                    local ok, state = pcall(
                        dosbox.mem_read_selector, pool_selector, object_offset, 64)
                    if ok then
                        entry.object_class = string.byte(state, 0x17 + 1)
                        entry.callback_field = word(state, 0x18 + 1)
                        entry.source_offset = word(state, 0x1a + 1)
                        entry.lifetime = word(state, 0x2c + 1)
                        entry.position = {
                            x = dword(state, 3) >> 16,
                            y = dword(state, 7) >> 16,
                        }
                        entry.state_field = word(state, 0x2e + 1)
                        entry.update_state = word(state, 0x32 + 1)
                    else
                        entry.object_read_error = tostring(state)
                    end
                end
                entries[#entries + 1] = entry
            end
        end
        banks[#banks + 1] = {
            bank = bank,
            base = base,
            entry_count = #entries,
            entries = entries,
        }
    end
    return {
        selector = pool_selector,
        banks = banks,
    }
end

local function changed_bytes(before_hex, after_hex)
    local changed = {}
    for offset = 0, math.min(#before_hex, #after_hex) // 2 - 1 do
        local before = tonumber(before_hex:sub(offset * 2 + 1, offset * 2 + 2), 16)
        local after = tonumber(after_hex:sub(offset * 2 + 1, offset * 2 + 2), 16)
        if before ~= after then
            changed[#changed + 1] = {offset = offset, before = before, after = after}
        end
    end
    return changed
end

local function related_hit(related_hits, offset)
    for _, hit in ipairs(related_hits) do
        if hit.offset == offset then return true end
    end
    return false
end

local function source_marker_processed(snapshot)
    return snapshot ~= nil and snapshot.marker_word ~= nil and
        ((snapshot.marker_word >> 8) & 0xff) ~= 0
end

local function scheduler_hit_snapshot(hit)
    local object_selector = hit.registers.es
    local object_offset = hit.registers.edi & 0xffff
    local ok, object = pcall(object_snapshot, object_selector, object_offset)
    return {
        segment = hit.segment,
        offset = hit.offset,
        registers = hit.registers,
        object = ok and object or {error = tostring(object)},
    }
end

local function follow_scheduler_passes(count)
    local passes = {}
    for pass = 1, count do
        dosbox.breakpoint_clear()
        local start = nil
        while start == nil do
            dosbox.breakpoint_set(0x01f7, 0x0e96, {once = true})
            dosbox.debug_continue()
            local candidate = wait_hit("follow-up scheduler start")
            if candidate.offset == 0x0e96 then
                start = candidate
            end
        end
        local current = {
            start = scheduler_hit_snapshot(start),
            globals = lifecycle_globals(),
            pool = pool_snapshot(),
            entries = {},
        }
        local complete = false
        for step = 1, 512 do
            for _, offset in ipairs(class_entry_offsets) do
                dosbox.breakpoint_set(0x01f7, offset, {once = true})
            end
            dosbox.breakpoint_set(0x01f7, 0x0e96, {once = true})
            dosbox.debug_continue()
            local hit = wait_hit("follow-up scheduler entry")
            if hit.offset == 0x0e96 then
                current.end_start = scheduler_hit_snapshot(hit)
                current.end_globals = lifecycle_globals()
                current.end_pool = pool_snapshot()
                complete = true
                break
            end
            current.entries[#current.entries + 1] = scheduler_hit_snapshot(hit)
        end
        assert(complete, "follow-up scheduler pass did not reach the next frame")
        passes[#passes + 1] = current
    end
    return passes
end

local function stack_return(hit)
    -- Normal ARE dispatch entries store a near offset in DS:81D2 and the
    -- callback returns with a near RET. The callback still has a full CS in
    -- the breakpoint state, so do not interpret the next stack word as a
    -- far-return segment.
    local raw = dosbox.mem_read("ss", hit.registers.esp & 0xffff, 4) or ""
    if #raw < 2 then return nil end
    return {
        offset = word(raw, 1),
        segment = hit.registers.cs,
        stack_hex = hex(raw),
        kind = "near",
    }
end

local function static_globals()
    return {
        camera_x = dosbox.mem_read_word("ds", 0x81c0),
        camera_y = dosbox.mem_read_word("ds", 0x81c4),
        action_flags = dosbox.mem_read_word("ds", 0x8196),
        keyboard_flags = dosbox.mem_read_word("ds", 0x88bc),
        bounds_object_offset = dosbox.mem_read_word("ds", 0x881a),
        bounds_object_flag = dosbox.mem_read_word("ds", 0x89ea),
    }
end

local function choose_level(level)
    local indices = {
        W1L1 = 0, W1L2 = 1, W1L3 = 2,
        W2L1 = 3, W2L2 = 4, W2L3 = 5,
        W3L1 = 6, W3L2 = 7, W3L3 = 8,
        W4L1 = 9, W4L2 = 10, W4L3 = 11,
        W5L1 = 12, W5L2 = 13, W5L3 = 14,
        W1L4 = 15, W2L4 = 16, W3L4 = 17,
        W4L4 = 18, W5L4 = 19,
    }
    local selector_index = indices[level]
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

dosbox.output.awaiting_startup_replay = true
dosbox.wait_frames(350)
if select_level ~= "" then
    choose_level(select_level)
else
    dosbox.key("KBD_space", true)
    dosbox.wait_frames(4)
    dosbox.key("KBD_space", false)
end

-- Find the requested ARE declaration and let its normal initializer create the
-- object. The player record is never selected by this path unless explicitly
-- requested by its type/record, which is outside this tracer's intended use.
local target_declaration = nil
for attempt = 1, 4096 do
    dosbox.breakpoint_set(0x01f7, 0x1e04, {once = true})
    local entry = wait_hit("ARE declaration")
    local record = dosbox.mem_read("fs", entry.registers.ebx & 0xffff, 6)
    local entity_type = word(record, 1) & 0xff
    if (entry.registers.ebx & 0xffff) == runtime_offset then
        assert(entity_type == expected_type,
               string.format("record type %02x, expected %02x",
                             entity_type, expected_type))
        local stream_origin_x = dosbox.mem_read_word("ds", 0x3714)
        local stream_origin_y = dosbox.mem_read_word("ds", 0x3716)
        local grid_selector = dosbox.mem_read_word("ds", 0x796e)
        local grid_offset = dosbox.mem_read_word("ds", 0x796c)
        local grid_stride = dosbox.mem_read_word("ds", 0x7968)
        local grid_cell_offset = grid_offset +
            (stream_origin_x // 0x40) * 2 +
            (stream_origin_y // 0x40) * grid_stride
        local grid_cell = dosbox.mem_read_selector(grid_selector,
                                                    grid_cell_offset, 2)
        target_declaration = {
            entry = entry,
            record = record,
            stream_region_origin = {
                x = stream_origin_x,
                y = stream_origin_y,
            },
            stream_grid_cell = {
                selector = grid_selector,
                offset = grid_cell_offset,
                value = word(grid_cell, 1),
            },
        }
        break
    end
    dosbox.debug_continue()
end
assert(target_declaration ~= nil, "target ARE declaration was not found")

local entry = target_declaration.entry
local record = target_declaration.record
local dispatch_offset = 0x81d2 + expected_type * 4
local dispatch = dosbox.mem_read("ds", dispatch_offset, 4)
local dispatch_callback_offset = word(dispatch, 1)
assert(dispatch_callback_offset ~= 0, "target object has no update callback")

dosbox.breakpoint_set(0x01f7, 0x1e8e, {once = true})
dosbox.debug_continue()
local factory_return = wait_hit("object factory return")
assert(factory_return.segment == 0x01f7 and factory_return.offset == 0x1e8e,
       "unexpected object factory breakpoint")
local object_selector = factory_return.registers.es
local object_offset = factory_return.registers.edi & 0xffff
local initial_object = object_snapshot(object_selector, object_offset)
local source_selector = entry.registers.fs
if camera_x >= 0 then dosbox.mem_write("ds", 0x81c0, little_word(camera_x)) end
if camera_y >= 0 then dosbox.mem_write("ds", 0x81c4, little_word(camera_y)) end
-- DS:81D2 contains the normal object update callback. The object word at
-- +0x18 is a separate per-object callback slot used by some sprite paths.
local object_class = string.byte(dispatch, 3)
assert(class_return_offsets[object_class] ~= nil,
       string.format("unsupported object scheduler class %s", tostring(object_class)))

local samples = {}
local attempts = 0
local scheduler_entry_offset = class_entry_offsets[object_class + 1]
while #samples < sample_count and attempts < sample_count * 128 do
    attempts = attempts + 1
    local direct_scheduler_hit = nil
    local direct_callback_entry = nil
    if #samples > 0 then
        -- The previous sample stopped at the class post-callback site, so its
        -- scheduler entry has already been consumed. Hook the live callback
        -- directly, while also watching the next frame boundary. Shared
        -- callbacks are stepped over until ES:DI identifies the target.
        local steady_object = object_snapshot(object_selector, object_offset)
        local steady_callback = steady_object.update_callback
        assert(steady_callback ~= 0,
               string.format("object callback cleared before steady sample %d", #samples + 1))
        dosbox.breakpoint_clear()
        dosbox.breakpoint_set(0x01f7, 0x0e96, {once = true})
        local steady_hit = nil
        local steady_attempts = 0
        local steady_candidates = {}
        while steady_hit == nil do
            steady_attempts = steady_attempts + 1
            if steady_attempts > 128 then
                local candidates = {}
                for _, candidate in ipairs(steady_candidates) do
                    candidates[#candidates + 1] = string.format(
                        "%04x:%04x:%04x", candidate.offset, candidate.es,
                        candidate.di)
                end
                error(string.format(
                    "steady callback did not reach target %04x:%04x candidates=%s",
                    object_selector, object_offset, table.concat(candidates, ",")))
            end
            dosbox.breakpoint_set(0x01f7, steady_callback, {once = true})
            dosbox.debug_continue()
            local candidate = wait_hit("steady callback entry")
            if #steady_candidates < 16 then
                steady_candidates[#steady_candidates + 1] = {
                    offset = candidate.offset,
                    es = candidate.registers.es,
                    di = candidate.registers.edi & 0xffff,
                }
            end
            if candidate.offset == steady_callback and
               candidate.registers.es == object_selector and
               (candidate.registers.edi & 0xffff) == object_offset then
                steady_hit = candidate
            elseif candidate.offset == steady_callback then
                local step_offset = callback_step_offsets[steady_callback]
                assert(step_offset ~= nil,
                       string.format("no steady callback step-over for %04x",
                                     steady_callback))
                dosbox.breakpoint_set(0x01f7, step_offset, {once = true})
                dosbox.debug_continue()
                local stepped = wait_hit("steady callback step-over")
                assert(stepped.offset == step_offset,
                       string.format("unexpected steady callback step-over %04x",
                                     stepped.offset))
            end
        end
        direct_scheduler_hit = {
            segment = steady_hit.segment,
            offset = scheduler_entry_offset,
            generation = steady_hit.generation,
            registers = steady_hit.registers,
        }
        direct_callback_entry = steady_hit
    end
    local scheduler_hit = direct_scheduler_hit
    if scheduler_hit == nil then
        dosbox.breakpoint_set(0x01f7, scheduler_entry_offset, {once = true})
        dosbox.debug_continue()
        scheduler_hit = wait_hit("object scheduler entry")
    end
    local scheduler_match = direct_scheduler_hit ~= nil or
        (scheduler_hit.offset == scheduler_entry_offset and
        scheduler_hit.registers.es == object_selector and
        (scheduler_hit.registers.edi & 0xffff) == object_offset)
    if scheduler_match then
        local before = object_snapshot(object_selector, object_offset)
        if initial_object.observed_scheduler_class == nil then
            initial_object = before
            initial_object.observed_scheduler_class = object_class
        end
        local source_before = source_snapshot(source_selector, before.source_offset)
        local globals_before = lifecycle_globals()
        local pool_before = pool_snapshot()
        local expected_return = class_return_offsets[object_class]
        local post_offset = class_post_offsets[object_class]
        local callback_offset = scheduler_hit.registers.eax & 0xffff
        assert(callback_offset ~= 0, "scheduler supplied no object callback")
        local callback_entry = direct_callback_entry
        local direct_sample = callback_entry ~= nil
        local other_callback_hits = {}
        local callback_attempts = 0
        while callback_entry == nil do
            dosbox.breakpoint_set(0x01f7, callback_offset, {once = true})
            callback_attempts = callback_attempts + 1
            assert(callback_attempts <= 128,
                   string.format("shared callback did not reach target %04x:%04x",
                                 object_selector, object_offset))
            dosbox.debug_continue()
            local candidate = wait_hit("object callback entry")
            if candidate.offset == callback_offset and
               candidate.registers.es == object_selector and
               (candidate.registers.edi & 0xffff) == object_offset then
                callback_entry = candidate
            else
                other_callback_hits[#other_callback_hits + 1] = {
                    segment = candidate.segment,
                    offset = candidate.offset,
                    registers = candidate.registers,
                }
                dosbox.breakpoint_set(0x01f7, callback_offset, {once = true})
            end
        end
        local returned = stack_return(callback_entry)
        assert(returned ~= nil, "object callback has no near return address")
        local callback_return_offset = returned.offset
        assert(callback_return_offset ~= callback_entry.offset,
               string.format("steady callback stack points back to callback %04x (stack=%s esp=%08x)",
                             callback_entry.offset, returned.stack_hex or "",
                             callback_entry.registers.esp or 0))
        dosbox.breakpoint_set(0x01f7, callback_return_offset, {once = true})
        dosbox.breakpoint_set(0x01f7, post_offset, {once = true})
        for _, offset in ipairs(callback_related_offsets) do
            dosbox.breakpoint_set(0x01f7, offset, {once = true})
        end
        local callback_return = nil
        local related_hits = {}
        local callback_return_attempts = 0
        while callback_return == nil do
            callback_return_attempts = callback_return_attempts + 1
            if callback_return_attempts > 32 then
                local offsets = {}
                for _, hit in ipairs(related_hits) do
                    offsets[#offsets + 1] = string.format("%04x", hit.offset)
                end
                error(string.format("callback return not reached: entry=%04x return=%04x stack=%s hits=%s",
                                    callback_entry.offset, callback_return_offset,
                                    returned.stack_hex or "", table.concat(offsets, ",")))
            end
            dosbox.debug_continue()
            local candidate = wait_hit("object callback return")
            if candidate.offset == callback_return_offset then
                callback_return = candidate
            else
                local related = {
                    segment = candidate.segment,
                    offset = candidate.offset,
                    registers = candidate.registers,
                }
                if candidate.offset == 0x1dca then
                    related.camera_before = {
                        x = dosbox.mem_read_word("ds", 0x81c0),
                        y = dosbox.mem_read_word("ds", 0x81c4),
                    }
                    if camera_x >= 0 then
                        dosbox.mem_write("ds", 0x81c0, little_word(camera_x))
                    end
                    if camera_y >= 0 then
                        dosbox.mem_write("ds", 0x81c4, little_word(camera_y))
                    end
                    related.camera_after_write = {
                        x = dosbox.mem_read_word("ds", 0x81c0),
                        y = dosbox.mem_read_word("ds", 0x81c4),
                    }
                end
                related_hits[#related_hits + 1] = related
                if not direct_sample then
                    for _, offset in ipairs(callback_related_offsets) do
                        dosbox.breakpoint_set(0x01f7, offset, {once = true})
                    end
                end
            end
        end
        for _, offset in ipairs(callback_related_offsets) do
            dosbox.breakpoint_remove(0x01f7, offset)
        end
        dosbox.debug_continue()
        local scheduler_post = wait_hit("object scheduler post-callback")
        local after = object_snapshot(object_selector, object_offset)
        local source_after = source_snapshot(source_selector, after.source_offset)
        local globals_after = lifecycle_globals()
        local pool_after = pool_snapshot()
        local callback_cleared = before.update_callback ~= 0 and
            after.update_callback == 0
        local visibility_gate_hit = related_hit(related_hits, 0x1dee)
        local state_machine_hit = related_hit(related_hits, 0x8e4b) or
            related_hit(related_hits, 0x8e78) or
            related_hit(related_hits, 0x8e85) or
            related_hit(related_hits, 0x9254) or
            related_hit(related_hits, 0x9255)
        local termination_reason = "callback_survived"
        if callback_cleared and visibility_gate_hit then
            termination_reason = "visibility_gate"
        elseif callback_cleared and state_machine_hit then
            termination_reason = "state_machine_exit"
        elseif callback_cleared then
            termination_reason = "callback_specific"
        end
        samples[#samples + 1] = {
            sequence = #samples + 1,
            scheduler = {
                segment = scheduler_hit.segment,
                offset = scheduler_hit.offset,
                registers = scheduler_hit.registers,
                expected_return = expected_return,
                post_offset = post_offset,
                callback_offset = callback_offset,
                mode = direct_sample and "direct_callback" or "scheduler_entry",
            },
            callback = {
                segment = callback_entry.segment,
                offset = callback_entry.offset,
                registers = callback_entry.registers,
                mode = direct_sample and "steady_direct" or "scheduler_callback",
                return_address = returned,
                other_callback_hits = other_callback_hits,
                related_hits = related_hits,
            },
            return_hit = {
                segment = callback_return.segment,
                offset = callback_return.offset,
                registers = callback_return.registers,
            },
            scheduler_post = {
                segment = scheduler_post.segment,
                offset = scheduler_post.offset,
                registers = scheduler_post.registers,
            },
            globals_before = globals_before,
            globals_after = globals_after,
            pool_before = pool_before,
            pool_after = pool_after,
            object_before = before,
            object_after = after,
            source_before = source_before,
            source_after = source_after,
            termination = {
                callback_before = before.update_callback,
                callback_after = after.update_callback,
                callback_cleared = callback_cleared,
                source_processed_before = source_marker_processed(source_before),
                source_processed_after = source_marker_processed(source_after),
                visibility_gate_hit = visibility_gate_hit,
                state_machine_hit = state_machine_hit,
                reason = termination_reason,
            },
            changed_bytes = changed_bytes(before.raw_hex, after.raw_hex),
        }
    else
        -- The debugger leaves a breakpoint at the current IP inactive when
        -- continuing. A non-target scheduler entry branches back to the
        -- class loop head rather than the callback-return path; stop there
        -- so the next scheduler-entry breakpoint is armed from a different
        -- instruction and can catch the next object in the same pass.
        local loop_offset = class_loop_offsets[object_class]
        dosbox.breakpoint_set(0x01f7, loop_offset, {once = true})
        dosbox.debug_continue()
        local skipped_return = wait_hit("non-target scheduler loop")
        assert(skipped_return.segment == 0x01f7 and
               skipped_return.offset == loop_offset,
               "unexpected non-target scheduler loop")
    end
end

assert(#samples == sample_count,
       string.format("captured %d/%d object callbacks", #samples, sample_count))
local reactivation = {}
if reactivate_camera_x >= 0 and reactivate_camera_y >= 0 then
    local tracked_source_offset = samples[1].object_after.source_offset
    reactivation.before = {
        camera = lifecycle_globals(),
        object = object_snapshot(object_selector, object_offset),
        source = source_snapshot(source_selector, tracked_source_offset),
        pool = pool_snapshot(),
    }
    reactivation.camera_write = {
        x = reactivate_camera_x,
        y = reactivate_camera_y,
    }
    dosbox.breakpoint_clear()
    local stream_entries = {}
    local function wait_stream(label)
        dosbox.breakpoint_set(0x01f7, 0x1cda, {once = true})
        dosbox.debug_continue()
        local stream_entry = wait_hit(label)
        stream_entries[#stream_entries + 1] = {
            segment = stream_entry.segment,
            offset = stream_entry.offset,
            registers = stream_entry.registers,
            globals = lifecycle_globals(),
        }
    end
    local aligned_reactivate_y = reactivate_camera_y & 0xffc0
    local target_stream_y = target_declaration.stream_region_origin.y
    -- The X scan in 1CDA uses the previous Y stream tracker.  Set that
    -- tracker to the target declaration's recorded row, then cross X in one
    -- controlled stream pass.  This is a debugger-only setup of the same
    -- state the normal camera path would establish while traversing regions.
    dosbox.mem_write("ds", 0x3712, little_word(target_stream_y))
    dosbox.mem_write("ds", 0x81c0, little_word(reactivate_camera_x))
    dosbox.mem_write("ds", 0x81c4, little_word(aligned_reactivate_y))
    wait_stream("ARE region stream pass")
    reactivation.stream_entries = stream_entries
    reactivation.camera_write.y_aligned = aligned_reactivate_y
    reactivation.camera_write.previous_stream_y = target_stream_y
    local declaration_visit = nil
    local declaration_error = nil
    local declaration_call_sites = {}
    for attempt = 1, 256 do
        dosbox.breakpoint_set(0x01f7, 0x1e04, {once = true})
        dosbox.breakpoint_set(0x01f7, 0x1d35, {once = true})
        dosbox.debug_continue()
        local ok, candidate = pcall(wait_hit, "ARE reactivation declaration")
        if not ok then
            declaration_error = tostring(candidate)
            break
        end
        if candidate.offset == 0x1d35 then
            declaration_call_sites[#declaration_call_sites + 1] = {
                segment = candidate.segment,
                offset = candidate.offset,
                registers = candidate.registers,
            }
        end
        if candidate.registers.fs == source_selector and
           candidate.offset == 0x1e04 and
           (candidate.registers.ebx & 0xffff) == tracked_source_offset then
            declaration_visit = candidate
            break
        end
    end
    if declaration_visit == nil then
        reactivation.declaration_call_sites = declaration_call_sites
        reactivation.declaration_error = declaration_error or
            string.format("source declaration %04x:%04x was not revisited",
                          source_selector, tracked_source_offset)
        reactivation.after = {
            camera = lifecycle_globals(),
            object = object_snapshot(object_selector, object_offset),
            source = source_snapshot(source_selector, tracked_source_offset),
            pool = pool_snapshot(),
        }
    else
        reactivation.declaration_visit = {
            segment = declaration_visit.segment,
            offset = declaration_visit.offset,
            registers = declaration_visit.registers,
            source_before = source_snapshot(source_selector, tracked_source_offset),
        }
        dosbox.breakpoint_set(0x01f7, 0x1e8e, {once = true})
        dosbox.debug_continue()
        local recreated_factory = wait_hit("reactivated object factory return")
        local recreated_selector = recreated_factory.registers.es
        local recreated_offset = recreated_factory.registers.edi & 0xffff
        local recreated_object = object_snapshot(recreated_selector, recreated_offset)
        reactivation.factory_return = {
            segment = recreated_factory.segment,
            offset = recreated_factory.offset,
            registers = recreated_factory.registers,
        }
        dosbox.breakpoint_set(0x01f7, 0x1ebd, {once = true})
        dosbox.debug_continue()
        local initialized_return = wait_hit("reactivated object initialization")
        local initialized_object = object_snapshot(recreated_selector, recreated_offset)
        reactivation.recreated_object_at_factory_return = recreated_object
        reactivation.recreated_object = initialized_object
        reactivation.initialization_return = {
            segment = initialized_return.segment,
            offset = initialized_return.offset,
            registers = initialized_return.registers,
        }
        reactivation.declaration_visit.source_after_factory =
            source_snapshot(source_selector, tracked_source_offset)
        -- The registration helper appends to the current bank before the
        -- next scheduler pass.  Snapshot that bank immediately; waiting for
        -- a later callback would allow ordinary game frames to mutate or
        -- recycle this same pool slot again.
        reactivation.initialized_pool = pool_snapshot()
        reactivation.after = {
            camera = lifecycle_globals(),
            object = initialized_object,
            source = source_snapshot(source_selector, tracked_source_offset),
            pool = reactivation.initialized_pool,
        }
    end
end
local followup = {}
if followup_passes > 0 then
    followup = follow_scheduler_passes(followup_passes)
end
dosbox.output.behavior_trace = {
    trace_schema_version = 1,
    trace_kind = "object-behavior",
    type = expected_type,
    record_offset = record_offset,
    runtime_record = {
        selector = entry.registers.fs,
        offset = entry.registers.ebx & 0xffff,
        stream_region_origin = target_declaration.stream_region_origin,
        stream_grid_cell = target_declaration.stream_grid_cell,
    },
    record_hex = hex(record),
    dispatch = {
        segment = 0x01f7,
        offset = word(dispatch, 1),
        raw_hex = hex(dispatch),
        object_class = string.byte(dispatch, 3),
        reserved = string.byte(dispatch, 4),
    },
    dispatch_callback = {segment = 0x01f7, offset = dispatch_callback_offset},
    object = {selector = object_selector, offset = object_offset},
    source = {selector = source_selector},
    initial_object = initial_object,
    camera_override = {x = camera_x, y = camera_y},
    samples = samples,
    reactivation = reactivation,
    followup_passes = followup,
}
dosbox.debug_continue()
