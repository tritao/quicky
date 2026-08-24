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
local sprite_init_offset = trace_config.sprite_init_offset or 0
local align_object_to_player = trace_config.align_object_to_player or false
local trace_overlap = trace_config.trace_overlap or false
local trace_collision = trace_config.trace_collision or false
local trace_platform = trace_config.trace_platform or false
local trace_bump = trace_config.trace_bump or false
local force_active_player_bounds = trace_config.force_active_player_bounds or false
local align_y_offset = trace_config.align_y_offset or 0
local force_velocity_x = trace_config.force_velocity_x
local force_velocity_y = trace_config.force_velocity_y
local force_platform_ready = trace_config.force_platform_ready or false
local runtime_offset = record_offset - 0x160

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

local function little_dword(value)
    return string.char(value & 0xff, (value >> 8) & 0xff,
                       (value >> 16) & 0xff, (value >> 24) & 0xff)
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
    local raw = dosbox.mem_read_selector(selector, offset, 128)
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
        state_field = word(raw, 0x2e + 1),
        update_state = word(raw, 0x32 + 1),
        object_class = string.byte(raw, 0x17 + 1),
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
        tile_flag_word = dosbox.mem_read_word("ds", 0x60d8),
        action_word = dosbox.mem_read_word("ds", 0x612e),
        object_global_880c = dosbox.mem_read_word("ds", 0x880c),
        object_global_881c = dosbox.mem_read_word("ds", 0x881c),
        object_global_8822 = dosbox.mem_read_word("ds", 0x8822),
        object_global_8824 = dosbox.mem_read_word("ds", 0x8824),
        cloud_global_89e6 = dosbox.mem_read_word("ds", 0x89e6),
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
    -- The first ARE declaration can execute in the same resumed slice as the
    -- selector dispatch, so arm it before leaving the selector breakpoint.
    dosbox.breakpoint_set(0x01f7, 0x1e04, {once = true})
    dosbox.debug_continue()
    dosbox.output.checkpoints.launch = wait_hit("selector Space dispatch")
    -- Leave the launch breakpoint and let the polling loop consume the first
    -- declaration hit, mirroring the resource tracer lifecycle.
    dosbox.breakpoint_set(0x01f7, 0x1e04, {once = true})
    dosbox.debug_continue()
    return true
end

dosbox.output.awaiting_startup_replay = true
dosbox.wait_frames(350)
local first_declaration = false
if select_level ~= "" then
    first_declaration = choose_level(select_level)
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
    if first_declaration then
        first_declaration = false
    else
        dosbox.breakpoint_set(0x01f7, 0x1e04, {once = true})
    end
    local entry = wait_hit("ARE declaration")
    local record = dosbox.mem_read("fs", entry.registers.ebx & 0xffff, 6)
    local entity_type = word(record, 1) & 0xff
    if (entry.registers.ebx & 0xffff) == runtime_offset then
        assert(entity_type == expected_type,
               string.format("record type %02x, expected %02x",
                             entity_type, expected_type))
        target_declaration = {entry = entry, record = record}
        break
    end
    dosbox.debug_continue()
    dosbox.wait_frames(1)
end
assert(target_declaration ~= nil, "target ARE declaration was not found")

local entry = target_declaration.entry
local record = target_declaration.record
local dispatch_offset = 0x81d2 + expected_type * 4
local dispatch = dosbox.mem_read("ds", dispatch_offset, 4)
local dispatch_callback_offset = word(dispatch, 1)
if expected_type == 0 then
    -- Inert records have no dispatch callback or object allocation. Stop at a
    -- one-shot execution barrier after the declaration so paired experiments
    -- can retain a machine-readable removal observation.
    local current = dosbox.cpu_state()
    dosbox.breakpoint_set(current.cs, current.eip, {once = true})
    dosbox.debug_continue()
    local capture = wait_hit("inert capture barrier")
    dosbox.output.behavior_trace = {
        trace_schema_version = 1,
        trace_kind = "object-behavior",
        type = expected_type,
        record_offset = record_offset,
        runtime_record = {selector = entry.registers.fs,
                          offset = entry.registers.ebx & 0xffff},
        record_hex = hex(record),
        dispatch = {
            segment = 0x01f7,
            offset = 0,
            raw_hex = hex(dispatch),
            object_class = string.byte(dispatch, 3),
            reserved = string.byte(dispatch, 4),
        },
        dispatch_callback = {segment = 0x01f7, offset = 0},
        object = nil,
        initial_object = nil,
        initialized_object = nil,
        initializer_breakpoint = nil,
        removed = true,
        capture = {segment = capture.segment, offset = capture.offset,
                   registers = capture.registers},
        samples = {},
    }
    dosbox.debug_continue()
    return
end
assert(dispatch_callback_offset ~= 0, "target object has no update callback")

dosbox.breakpoint_set(0x01f7, 0x1e8e, {once = true})
dosbox.debug_continue()
local factory_return = wait_hit("object factory return")
local object_selector = factory_return.registers.es
local object_offset = factory_return.registers.edi & 0xffff
local initial_object = object_snapshot(object_selector, object_offset)
if camera_x >= 0 then dosbox.mem_write("ds", 0x81c0, little_word(camera_x)) end
if camera_y >= 0 then dosbox.mem_write("ds", 0x81c4, little_word(camera_y)) end
-- The dispatch callback in DS:81D2 is the initializer. The post-initializer
-- installs the callback stored in object+0x18; break directly on that live
-- callback instead of relying on scheduler entry offsets, which vary between
-- runtime builds and can miss one-shot normal objects.
local initialized_object = initial_object
local initializer_breakpoint = nil
if sprite_init_offset ~= 0 then
    dosbox.breakpoint_set(0x01f7, sprite_init_offset, {once = true})
    dosbox.debug_continue()
    local initialized = wait_hit("object post-initializer")
    assert(initialized.segment == 0x01f7 and initialized.offset == sprite_init_offset,
           "unexpected object post-initializer breakpoint")
    initialized_object = object_snapshot(object_selector, object_offset)
    initializer_breakpoint = {
        segment = initialized.segment,
        offset = initialized.offset,
        registers = initialized.registers,
    }
end
local interaction_alignment = nil
if align_object_to_player then
    local bounds_offset = dosbox.mem_read_word("ds", 0x881a)
    local player = object_snapshot(object_selector, bounds_offset)
    local player_bounds_before = dosbox.mem_read_selector(
        object_selector, bounds_offset + 0x2c, 8)
    if force_active_player_bounds then
        dosbox.mem_write_selector(object_selector, bounds_offset + 0x2c,
                                  little_word(0xfff6))
        dosbox.mem_write_selector(object_selector, bounds_offset + 0x2e,
                                  little_word(0xffd8))
        dosbox.mem_write_selector(object_selector, bounds_offset + 0x30,
                                  little_word(0x000a))
        dosbox.mem_write_selector(object_selector, bounds_offset + 0x32,
                                  little_word(0x0000))
    end
    dosbox.mem_write_selector(object_selector, object_offset + 0x02,
                              little_dword(player.position.x_fixed))
    local aligned_y_fixed = player.position.y_fixed + align_y_offset * 0x10000
    dosbox.mem_write_selector(object_selector, object_offset + 0x06,
                              little_dword(aligned_y_fixed))
    initialized_object = object_snapshot(object_selector, object_offset)
    interaction_alignment = {
        bounds_object = {selector = object_selector, offset = bounds_offset,
                         position = player.position},
        player_bounds_before_hex = hex(player_bounds_before),
        player_bounds_after_hex = hex(dosbox.mem_read_selector(
            object_selector, bounds_offset + 0x2c, 8)),
        object_position = initialized_object.position,
        y_offset = align_y_offset,
    }
end
local callback_offset = initialized_object.update_callback
assert(callback_offset ~= 0,
       "initialized object has no per-object callback")
local samples = {}
local attempts = 0
while #samples < sample_count and attempts < sample_count * 128 do
    attempts = attempts + 1
    dosbox.breakpoint_set(0x01f7, callback_offset, {once = true})
    dosbox.debug_continue()
    local callback_entry = wait_hit("object callback entry")
    local matches_object = callback_entry.segment == 0x01f7 and
        callback_entry.offset == callback_offset and
        callback_entry.registers.es == object_selector and
        (callback_entry.registers.edi & 0xffff) == object_offset
    if not matches_object then
        dosbox.breakpoint_set(0x01f7, callback_offset, {once = true})
        dosbox.debug_continue()
    else
        if camera_x >= 0 then
            dosbox.mem_write("ds", 0x81c0, little_word(camera_x))
            dosbox.mem_write("ds", 0x81c4, little_word(camera_y))
        end
        if #samples == 0 then
            if force_platform_ready then
                -- Debugger-only control: clear the carry latch that the
                -- normal player branch sets, allowing the platform's own
                -- horizontal/vertical motion branch to execute.
                dosbox.mem_write_selector(object_selector, object_offset + 0x58,
                                          string.char(0))
                dosbox.mem_write_selector(object_selector, object_offset + 0x59,
                                          string.char(0))
                dosbox.mem_write_selector(object_selector, object_offset + 0x5a,
                                          string.char(0))
                dosbox.mem_write_selector(object_selector, object_offset + 0x54,
                                          little_word(0))
            end
            if force_velocity_x ~= nil then
                dosbox.mem_write_selector(object_selector, object_offset + 0x0a,
                                          little_dword(force_velocity_x))
            end
            if force_velocity_y ~= nil then
                dosbox.mem_write_selector(object_selector, object_offset + 0x0e,
                                          little_dword(force_velocity_y))
            end
        end
        if force_active_player_bounds then
            local bounds_offset = dosbox.mem_read_word("ds", 0x881a)
            dosbox.mem_write_selector(object_selector, bounds_offset + 0x2c,
                                      little_word(0xfff6))
            dosbox.mem_write_selector(object_selector, bounds_offset + 0x2e,
                                      little_word(0xffd8))
            dosbox.mem_write_selector(object_selector, bounds_offset + 0x30,
                                      little_word(0x000a))
            dosbox.mem_write_selector(object_selector, bounds_offset + 0x32,
                                      little_word(0x0000))
        end
        local globals_before = static_globals()
        local before = object_snapshot(object_selector, object_offset)
        local returned = stack_return(callback_entry)
        assert(returned ~= nil, "object callback has no near return address")
        local overlap_probe = nil
        local collision_probe = nil
        local bump_probe = nil
        local callback_return = nil
        if trace_overlap or trace_collision or trace_platform or trace_bump then
            overlap_probe = {hits = {}}
            if trace_overlap then
                dosbox.breakpoint_set(0x01f7, 0x8d36, {once = true})
                dosbox.breakpoint_set(0x01f7, 0x8d4a, {once = true})
                dosbox.breakpoint_set(0x01f7, 0x8d60, {once = true})
                dosbox.breakpoint_set(0x01f7, 0x8e08, {once = true})
                dosbox.breakpoint_set(0x01f7, 0x8e29, {once = true})
                dosbox.breakpoint_set(0x01f7, 0x8e42, {once = true})
                dosbox.breakpoint_set(0x01f7, 0x8e48, {once = true})
                dosbox.breakpoint_set(0x01f7, 0x8e49, {once = true})
            end
            if trace_collision then
                collision_probe = {hits = {}}
                dosbox.breakpoint_set(0x01f7, 0x1b77, {once = true})
                dosbox.breakpoint_set(0x01f7, 0x1c4d, {once = true})
                dosbox.breakpoint_set(0x01f7, 0x1c6e, {once = true})
                dosbox.breakpoint_set(0x01f7, 0x1dca, {once = true})
                dosbox.breakpoint_set(0x01f7, 0x1dee, {once = true})
            end
            if trace_platform then
                collision_probe = {hits = {}}
                dosbox.breakpoint_set(0x01f7, 0x9e75, {once = true})
                dosbox.breakpoint_set(0x01f7, 0x9fb2, {once = true})
                dosbox.breakpoint_set(0x01f7, 0xa075, {once = true})
                dosbox.breakpoint_set(0x01f7, 0xa0b2, {once = true})
                dosbox.breakpoint_set(0x01f7, 0x1dee, {once = true})
            end
            if trace_bump then
                bump_probe = {hits = {}}
                dosbox.breakpoint_set(0x01f7, 0x9c13, {once = true})
                dosbox.breakpoint_set(0x01f7, 0x9c20, {once = true})
                dosbox.breakpoint_set(0x01f7, 0x9c29, {once = true})
                dosbox.breakpoint_set(0x01f7, 0x9c2e, {once = true})
                dosbox.breakpoint_set(0x01f7, 0x9c45, {once = true})
                dosbox.breakpoint_set(0x01f7, 0x9c57, {once = true})
                dosbox.breakpoint_set(0x01f7, 0x9c5f, {once = true})
                dosbox.breakpoint_set(0x01f7, 0x9c64, {once = true})
                dosbox.breakpoint_set(0x01f7, 0x9c6a, {once = true})
            end
            dosbox.breakpoint_set(returned.segment, returned.offset, {once = true})
            dosbox.debug_continue()
            for probe_attempt = 1, 16 do
                local probe_hit = dosbox.wait_for_breakpoint(timeout_ms)
                if not probe_hit then break end
                local hit_record = {
                    segment = probe_hit.segment,
                    offset = probe_hit.offset,
                    registers = probe_hit.registers,
                }
                overlap_probe.hits[#overlap_probe.hits + 1] = hit_record
                if trace_collision and (probe_hit.offset == 0x1b77 or
                        probe_hit.offset == 0x1c4d or probe_hit.offset == 0x1c6e or
                        probe_hit.offset == 0x1dca or probe_hit.offset == 0x1dee) then
                    collision_probe.hits[#collision_probe.hits + 1] = hit_record
                end
                if trace_platform and (probe_hit.offset == 0x9e75 or
                        probe_hit.offset == 0x9fb2 or probe_hit.offset == 0xa075 or
                        probe_hit.offset == 0xa0b2 or probe_hit.offset == 0x1dee) then
                    collision_probe.hits[#collision_probe.hits + 1] = hit_record
                end
                if trace_bump and (probe_hit.offset == 0x9c13 or
                        probe_hit.offset == 0x9c20 or probe_hit.offset == 0x9c29 or
                        probe_hit.offset == 0x9c2e or probe_hit.offset == 0x9c45 or
                        probe_hit.offset == 0x9c57 or probe_hit.offset == 0x9c5f or
                        probe_hit.offset == 0x9c64 or probe_hit.offset == 0x9c6a) then
                    bump_probe.hits[#bump_probe.hits + 1] = hit_record
                end
                if probe_hit.segment == returned.segment and
                        probe_hit.offset == returned.offset then
                    callback_return = probe_hit
                    break
                end
                if probe_hit.segment == 0x01f7 and probe_hit.offset == 0x8d36 then
                    dosbox.breakpoint_set(0x01f7, 0x8e42, {once = true})
                elseif probe_hit.segment == 0x01f7 and probe_hit.offset == 0x8e42 then
                    dosbox.breakpoint_set(returned.segment, returned.offset,
                                          {once = true})
                end
                dosbox.debug_continue()
            end
        else
            dosbox.breakpoint_set(returned.segment, returned.offset, {once = true})
            dosbox.debug_continue()
            callback_return = wait_hit("object callback return")
        end
        assert(callback_return ~= nil, "overlap probe did not reach callback return")
        local after = object_snapshot(object_selector, object_offset)
        samples[#samples + 1] = {
            sequence = #samples + 1,
            callback = {
                segment = callback_entry.segment,
                offset = callback_entry.offset,
                registers = callback_entry.registers,
                return_address = returned,
            },
            return_hit = {
                segment = callback_return.segment,
                offset = callback_return.offset,
                registers = callback_return.registers,
            },
            globals_before = globals_before,
            globals_after = static_globals(),
            object_before = before,
            object_after = after,
            changed_bytes = changed_bytes(before.raw_hex, after.raw_hex),
            overlap_probe = overlap_probe,
            collision_probe = collision_probe,
            bump_probe = bump_probe,
        }
        callback_offset = after.update_callback
        if callback_offset == 0 then break end
    end
end

assert(#samples > 0,
       "captured no callbacks for the initialized object")
dosbox.output.behavior_trace = {
    trace_schema_version = 1,
    trace_kind = "object-behavior",
    type = expected_type,
    record_offset = record_offset,
    runtime_record = {selector = entry.registers.fs, offset = entry.registers.ebx & 0xffff},
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
    initial_object = initial_object,
    initialized_object = initialized_object,
    interaction_alignment = interaction_alignment,
    initializer_breakpoint = initializer_breakpoint,
    camera_override = {x = camera_x, y = camera_y},
    samples = samples,
}
dosbox.debug_continue()
