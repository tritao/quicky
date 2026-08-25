-- Trace the transient high-address object callbacks without using the player
-- tracer.  These objects are created by gameplay actions and are not normal
-- ARE declarations, so the probe discovers them from the live object pool.
local trace_config = TRACE_CONFIG or {}
assert(type(trace_config) == "table", "TRACE_CONFIG must be a table")

local timeout_ms = trace_config.timeout_ms or 30000
local frame_count = trace_config.frames or 240
local frame_step = trace_config.frame_step or 30
local select_level = trace_config.select_level or ""
local selector_frames = trace_config.selector_frames or 60
local input_key = trace_config.input_key or ""
local input_frames = trace_config.input_frames or 0
local input_samples = trace_config.input_samples or 0
local target_x_delta = trace_config.target_x_delta or 0
local target_y_delta = trace_config.target_y_delta or -10
local target_cursor_offset = trace_config.target_cursor_offset or 0x2a
local force_object_x = trace_config.force_object_x
local force_object_y = trace_config.force_object_y
local stop_at_cursor = trace_config.stop_at_cursor
local callback_segments = {0x01f7, 0x1997}
local watched_effect_callbacks = {}
local watched_effect_objects = {}
local watch_effect_object

local high_families = {
    {name = "B25D/B266", initializer = 0xb20b, steady = 0xb25d,
     tail = 0xb266, body_offsets = {0xb266, 0xb2b0, 0xb2b8, 0xb2ba,
                                     0xb2bf, 0xb2c4, 0xb303}, action_call = 0xb2c4},
    {name = "BABC/BB0E", initializer = 0xbabc, steady = 0xbb0e,
     tail = 0xbb17, body_offsets = {0xbb17, 0xbb61, 0xbb69, 0xbb6b,
                                     0xbb70, 0xbb75, 0xbbb4}, action_call = 0xbb75},
    {name = "C30D/C328", initializer = 0xc30d, steady = 0xc328,
     tail = 0xc331, body_offsets = {0xc331, 0xc37b, 0xc383, 0xc385,
                                     0xc38a, 0xc38f, 0xc3ce}, action_call = 0xc38f},
    {name = "CD88/CDA3", initializer = 0xcd88, steady = 0xcda3,
     tail = 0xcdac, body_offsets = {0xcdac, 0xcdf6, 0xcdfe, 0xce00,
                                    0xce05, 0xce0a, 0xce49}, action_call = 0xce0a},
    {name = "D53F/D55A", initializer = 0xd53f, steady = 0xd55a,
     tail = 0xd563, body_offsets = {0xd563, 0xd5ad, 0xd5b5, 0xd5b7,
                                    0xd5bc, 0xd5c1, 0xd600}, action_call = 0xd5c1},
}

local function word(s, index)
    local lo, hi = string.byte(s, index, index + 1)
    return (lo or 0) | ((hi or 0) << 8)
end

local function dword(s, index)
    return word(s, index) | (word(s, index + 2) << 16)
end

local function signed_word(value)
    return value >= 0x8000 and value - 0x10000 or value
end

local function little_word(value)
    return string.char(value & 0xff, (value >> 8) & 0xff)
end

local function little_dword(value)
    return little_word(value & 0xffff) .. little_word((value >> 16) & 0xffff)
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

local function family_for_callback(callback)
    for _, family in ipairs(high_families) do
        if callback == family.initializer or callback == family.steady then
            return family
        end
    end
    return nil
end

local function object_snapshot(selector, offset, index)
    local raw = dosbox.mem_read_selector(selector, offset, 0x40)
    local x_fixed = dword(raw, 3)
    local y_fixed = dword(raw, 7)
    return {
        index = index,
        selector = selector,
        offset = offset,
        raw_hex = hex(raw),
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
        source = word(raw, 0x1a + 1),
        target_cursor = word(raw, 0x2a + 1),
        sprite_slot = word(raw, 0x12 + 1),
        lifetime = word(raw, 0x2c + 1),
        state_field = word(raw, 0x2e + 1),
        update_state = word(raw, 0x32 + 1),
    }
end

local function pool_snapshot()
    local pointer_raw = dosbox.mem_read("ds", 0x755e, 4) or ""
    if #pointer_raw < 4 then return {error = "pool pointer truncated"} end
    local pointer = dword(pointer_raw, 1)
    local selector = (pointer >> 16) & 0xffff
    local base = pointer & 0xffff
    local stride = dosbox.mem_read_word("ds", 0x30ce)
    local objects = {}
    local high_objects = {}
    if selector == 0 or stride == 0 then
        return {selector = selector, offset = base, stride = stride,
                objects = objects, high_objects = high_objects,
                error = "pool pointer or stride is zero"}
    end
    for index = 0, 63 do
        local offset = base + index * stride
        local ok, raw = pcall(dosbox.mem_read_selector, selector, offset, 0x40)
        if ok and raw and #raw >= 0x40 then
            local object = object_snapshot(selector, offset, index)
            if object.callback ~= 0 then
                object.family = family_for_callback(object.callback)
                objects[#objects + 1] = object
                if object.family ~= nil then high_objects[#high_objects + 1] = object end
            end
        end
    end
    return {
        selector = selector,
        offset = base,
        stride = stride,
        active_count = #objects,
        high_count = #high_objects,
        objects = objects,
        high_objects = high_objects,
    }
end

local function globals_snapshot()
    return {
        camera_x = dosbox.mem_read_word("ds", 0x81c0),
        camera_y = dosbox.mem_read_word("ds", 0x81c4),
        target_active_count = dosbox.mem_read_word("ds", 0x8806),
        target_capacity = dosbox.mem_read_word("ds", 0x8808),
        target_x = signed_word(dosbox.mem_read_word("ds", 0x87de)),
        target_y = signed_word(dosbox.mem_read_word("ds", 0x87e0)),
        action_word = dosbox.mem_read_word("ds", 0x612e),
        active_scheduler_count = dosbox.mem_read_word("ds", 0x88c8),
    }
end

local function scheduled_high_snapshot()
    local raw = dosbox.mem_read("ds", 0x7566, 0x200) or ""
    local entries = {}
    for index = 0, math.min(64, #raw // 8) - 1 do
        local base = index * 8 + 1
        local callback = word(raw, base)
        local callback_segment = word(raw, base + 2)
        local object_offset = word(raw, base + 4)
        local object_selector = word(raw, base + 6)
        local family = family_for_callback(callback)
        if family ~= nil then
            local entry = {
                index = index,
                callback = callback,
                callback_segment = callback_segment,
                object_offset = object_offset,
                object_selector = object_selector,
                family = family.name,
                tail = family.tail,
            }
            local ok, object = pcall(object_snapshot, object_selector,
                                     object_offset, index)
            if ok then entry.object = object else entry.object_error = tostring(object) end
            entries[#entries + 1] = entry
        end
    end
    return entries
end

local function inject_target(object)
    local target_x = (object.position.x + target_x_delta) & 0xffff
    local target_y = (object.position.y + target_y_delta) & 0xffff
    dosbox.mem_write("ds", 0x8806, little_word(1))
    dosbox.mem_write("ds", 0x8808, little_word(1))
    dosbox.mem_write("ds", 0x87de, little_word(target_x))
    dosbox.mem_write("ds", 0x87e0, little_word(target_y))
    dosbox.mem_write_selector(object.selector,
                               object.offset + target_cursor_offset,
                               little_word(0))
    return {
        target_x = signed_word(target_x),
        target_y = signed_word(target_y),
        x_delta = target_x_delta,
        y_delta = target_y_delta,
        cursor_offset = target_cursor_offset,
        active_count = 1,
        capacity = 1,
    }
end

local function near_return(hit)
    local raw = dosbox.mem_read("ss", (hit.registers.esp or 0) & 0xffff, 2) or ""
    if #raw < 2 then return nil end
    return {segment = hit.segment, offset = word(raw, 1), kind = "near",
            stack_hex = hex(dosbox.mem_read(
                "ss", (hit.registers.esp or 0) & 0xffff, 6) or "")}
end

local function far_return(hit)
    local raw = dosbox.mem_read("ss", (hit.registers.esp or 0) & 0xffff, 4) or ""
    if #raw < 4 then return nil end
    return {segment = word(raw, 3), offset = word(raw, 1), kind = "far",
            stack_hex = hex(raw)}
end

local function arm_callback_targets()
    for _, family in ipairs(high_families) do
        for _, segment in ipairs(callback_segments) do
            dosbox.breakpoint_set(segment, family.initializer, {once = true})
            dosbox.breakpoint_set(segment, family.steady, {once = true})
        end
    end
    for _, segment in ipairs(callback_segments) do
        dosbox.breakpoint_set(segment, 0x4b70, {once = true})
        for callback in pairs(watched_effect_callbacks) do
            dosbox.breakpoint_set(segment, callback, {once = true})
        end
    end
end

local function family_body_offset(family, offset)
    for _, body_offset in ipairs(family.body_offsets) do
        if body_offset == offset then return true end
    end
    return false
end

local function arm_high_body_targets(segment, family, skip)
    for _, offset in ipairs(family.body_offsets) do
        if offset ~= skip then
            dosbox.breakpoint_set(segment, offset, {once = true})
        end
    end
end

local function choose_level(level)
    local selector_indices = {
        W1L1 = 0, W1L2 = 1, W1L3 = 2,
        W2L1 = 3, W2L2 = 4, W2L3 = 5,
        W3L1 = 6, W3L2 = 7, W3L3 = 8,
        W4L1 = 9, W4L2 = 10, W4L3 = 11,
        W5L1 = 12, W5L2 = 13, W5L3 = 14,
        W1L4 = 15, W2L4 = 16, W3L4 = 17,
        W4L4 = 18, W5L4 = 19,
    }
    local selector_index = selector_indices[level]
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

local function trace_high_callback(hit)
    local callback = hit.offset
    local family = family_for_callback(callback)
    if family == nil then return nil end
    local selector = hit.registers.es
    local offset = hit.registers.edi & 0xffff
    local before = object_snapshot(selector, offset, -1)
    local forced_position = nil
    if force_object_x ~= nil or force_object_y ~= nil then
        if force_object_x ~= nil then
            -- ES:DI is the callback's object view; its position words are
            -- two bytes before the logical +0x04/+0x08 names used in notes.
            dosbox.mem_write_selector(selector, offset + 0x02,
                                      little_dword(force_object_x << 16))
        end
        if force_object_y ~= nil then
            dosbox.mem_write_selector(selector, offset + 0x06,
                                      little_dword(force_object_y << 16))
        end
        forced_position = {
            x = force_object_x, y = force_object_y,
        }
        before = object_snapshot(selector, offset, -1)
    end
    local event = {
        callback_entry = {segment = hit.segment, offset = callback,
                          registers = hit.registers},
        family = family.name,
        tail = family.tail,
        object_before = before,
        forced_position = forced_position,
        globals_before = globals_snapshot(),
        target_override = inject_target(before),
        related_hits = {},
    }
    local returned = near_return(hit)
    assert(returned ~= nil, string.format("high callback %04x has no return", callback))
    event.return_expected = returned
    dosbox.breakpoint_set(returned.segment, returned.offset, {once = true})
    if callback == family.steady then
        arm_high_body_targets(returned.segment, family)
    end
    for _, segment in ipairs(callback_segments) do
        dosbox.breakpoint_set(segment, 0x4b70, {once = true})
    end
    local callback_return = nil
    local helper_return = nil
    local import_target_pending = nil
    local import_return = nil
    for attempt = 1, 32 do
        dosbox.debug_continue()
        local candidate = wait_hit("high callback action/return")
        if helper_return ~= nil and candidate.segment == helper_return.segment and
           candidate.offset == helper_return.offset then
            event.related_hits[#event.related_hits + 1] = {
                segment = candidate.segment, offset = candidate.offset,
                kind = "4b70_return", registers = candidate.registers,
            }
            helper_return = nil
        elseif import_return ~= nil and candidate.segment == import_return.segment and
               candidate.offset == import_return.offset then
            event.import_return = {
                segment = candidate.segment, offset = candidate.offset,
                registers = candidate.registers,
            }
            local imported_selector = candidate.registers.es
            local imported_offset = candidate.registers.edi & 0xffff
            local ok, imported_object = pcall(object_snapshot,
                                               imported_selector,
                                               imported_offset, -1)
            if ok then
                event.imported_object_after = imported_object
            else
                event.imported_object_error = tostring(imported_object)
            end
            import_return = nil
        elseif import_target_pending ~= nil and
               candidate.segment == import_target_pending.segment and
               candidate.offset == import_target_pending.offset then
            event.import_entry = {
                segment = candidate.segment, offset = candidate.offset,
                registers = candidate.registers,
            }
            import_target_pending = nil
            local target_return = far_return(candidate)
            if target_return ~= nil then
                import_return = target_return
                dosbox.breakpoint_set(target_return.segment, target_return.offset,
                                      {once = true})
            end
        elseif candidate.segment == returned.segment and
               candidate.offset == returned.offset then
            callback_return = candidate
            break
        elseif candidate.offset == 0x4b70 then
            event.related_hits[#event.related_hits + 1] = {
                segment = candidate.segment, offset = candidate.offset,
                kind = "action_helper", registers = candidate.registers,
            }
            helper_return = far_return(candidate)
            if helper_return ~= nil then
                dosbox.breakpoint_set(helper_return.segment, helper_return.offset,
                                      {once = true})
            end
        elseif family_body_offset(family, candidate.offset) then
            local body_hit = {
                segment = candidate.segment, offset = candidate.offset,
                kind = "high-tail", registers = candidate.registers,
            }
            if candidate.offset == family.action_call then
                local code = dosbox.mem_read("cs", candidate.offset, 8) or ""
                body_hit.code_hex = hex(code)
                if #code >= 5 and string.byte(code, 1) == 0x9a then
                    local target = {
                        segment = word(code, 4), offset = word(code, 2),
                    }
                    body_hit.import_target = target
                    event.import_call = body_hit
                    if target.segment ~= 0xffff and target.offset ~= 0xffff then
                        dosbox.breakpoint_set(target.segment, target.offset,
                                              {once = true})
                        import_target_pending = target
                    end
                end
            end
            event.related_hits[#event.related_hits + 1] = body_hit
            arm_high_body_targets(returned.segment, family, candidate.offset)
        else
            error(string.format("unexpected high callback hit %04x:%04x",
                                candidate.segment, candidate.offset))
        end
    end
    assert(callback_return ~= nil, string.format(
        "high callback %04x did not return", callback))
    event.return_actual = {
        segment = callback_return.segment, offset = callback_return.offset,
        registers = callback_return.registers,
    }
    event.object_after = object_snapshot(selector, offset, -1)
    event.globals_after = globals_snapshot()
    if event.imported_object_after ~= nil then
        local ok, imported_after = pcall(object_snapshot,
                                         event.imported_object_after.selector,
                                         event.imported_object_after.offset, -1)
        if ok then
            event.imported_object_after_callback = imported_after
            watch_effect_object(imported_after)
        end
    end
    return event
end

local function effect_object_key(selector, offset)
    return string.format("%04x:%04x", selector, offset)
end

watch_effect_object = function(object)
    if object == nil or object.callback == 0 then return end
    watched_effect_objects[effect_object_key(object.selector, object.offset)] = true
    if object.callback ~= 0x4b70 then
        watched_effect_callbacks[object.callback] = true
    end
end

local function trace_spawned_effect_callback(hit)
    local selector = hit.registers.es
    local offset = hit.registers.edi & 0xffff
    if hit.offset ~= 0x4b70 and
       not watched_effect_objects[effect_object_key(selector, offset)] then
        return nil
    end
    local event = {
        callback_entry = {segment = hit.segment, offset = hit.offset,
                          registers = hit.registers},
        object_before = object_snapshot(selector, offset, -1),
    }
    local returned = near_return(hit)
    assert(returned ~= nil, "spawned effect callback has no near return")
    event.return_expected = returned
    dosbox.breakpoint_set(returned.segment, returned.offset, {once = true})
    dosbox.debug_continue()
    local callback_return = wait_hit("spawned effect callback return")
    assert(callback_return.segment == returned.segment and
           callback_return.offset == returned.offset,
           string.format("unexpected spawned effect callback hit %04x:%04x",
                         callback_return.segment, callback_return.offset))
    event.return_actual = {
        segment = callback_return.segment,
        offset = callback_return.offset,
        registers = callback_return.registers,
    }
    event.object_after = object_snapshot(selector, offset, -1)
    watch_effect_object(event.object_after)
    return event
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

local frames = {}
local callback_events = {}
local spawned_effect_events = {}
local stopped_at_cursor = nil
for frame = 1, frame_count do
    if frame > 1 and input_key ~= "" and input_frames > 0 and
       (input_samples == 0 or frame <= input_samples + 1) then
        dosbox.key(input_key, true)
        dosbox.wait_frames(input_frames)
        dosbox.key(input_key, false)
    end
    -- Match the established player-trace cadence: let a complete quiet
    -- window run before stopping on one of the transient callbacks.  A
    -- breakpoint on every 0E96 scheduler entry can starve these short-lived
    -- objects before their callback is dispatched.
    dosbox.breakpoint_clear()
    dosbox.wait_frames(frame_step)
    arm_callback_targets()
    dosbox.debug_continue()
    local hit = wait_hit("high-effect callback")
    local event = nil
    if hit.offset == 0x4b70 or watched_effect_callbacks[hit.offset] then
        event = trace_spawned_effect_callback(hit)
        if event ~= nil then
            spawned_effect_events[#spawned_effect_events + 1] = event
            if stop_at_cursor ~= nil and event.object_after ~= nil and
               event.object_after.target_cursor >= stop_at_cursor then
                stopped_at_cursor = event.object_after.target_cursor
            end
        end
    else
        event = trace_high_callback(hit)
        if event ~= nil then callback_events[#callback_events + 1] = event end
    end
    frames[#frames + 1] = {
        sequence = frame,
        breakpoint = {segment = hit.segment, offset = hit.offset,
                      registers = hit.registers},
        globals = globals_snapshot(),
        scheduled_high = scheduled_high_snapshot(),
    }
    if stopped_at_cursor ~= nil then break end
end
if input_key ~= "" then dosbox.key(input_key, false) end
dosbox.output.high_effect_trace = {
    trace_schema_version = 1,
    trace_kind = "high-effect",
    select_level = select_level,
    frames = frames,
    callback_events = callback_events,
    spawned_effect_events = spawned_effect_events,
    target_probe = {
        x_delta = target_x_delta,
        y_delta = target_y_delta,
        cursor_offset = target_cursor_offset,
    },
    stop_at_cursor = stop_at_cursor,
    stopped_at_cursor = stopped_at_cursor,
}
-- Leave the emulator paused at the selected callback so the API client can
-- capture the exact sprite state.  The normal path resumes once the complete
-- trace has been published.
if stopped_at_cursor == nil then dosbox.debug_continue() end
