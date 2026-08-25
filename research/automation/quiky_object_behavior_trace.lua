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
local trace_contact = trace_config.trace_contact or false
local trace_stream_lifecycle = trace_config.trace_stream_lifecycle or false
local lifecycle_return_camera_x = trace_config.lifecycle_return_camera_x or 700
local lifecycle_return_camera_y = trace_config.lifecycle_return_camera_y or 350
local force_active_player_bounds = trace_config.force_active_player_bounds or false
local force_bump_player_state = trace_config.force_bump_player_state or false
local force_cloud_player_state = trace_config.force_cloud_player_state or false
local trace_cloud_consumers = trace_config.trace_cloud_consumers or false
local cloud_probe_frames = trace_config.cloud_probe_frames or 8
local cloud_consumer_offset = trace_config.cloud_consumer_offset or 0
local trace_cloud_outer_renderer = trace_config.trace_cloud_outer_renderer or false
local trace_cloud_hardware_renderer = trace_config.trace_cloud_hardware_renderer or false
local cloud_hardware_frames = trace_config.cloud_hardware_frames or 8
local force_contact_gate = trace_config.force_contact_gate or false
local align_x_offset = trace_config.align_x_offset or 0
local align_y_offset = trace_config.align_y_offset or 0
local force_velocity_x = trace_config.force_velocity_x
local force_velocity_y = trace_config.force_velocity_y
local force_platform_ready = trace_config.force_platform_ready or false
local reload_after_collect = trace_config.reload_after_collect or false
local reload_level = trace_config.reload_level
if reload_level == nil or reload_level == "" then reload_level = select_level end
local reload_wait_frames = trace_config.reload_wait_frames or 30
local force_tile_mask = trace_config.force_tile_mask
local trace_puzzle_completion = trace_config.trace_puzzle_completion or false
local force_completion_outer_state = trace_config.force_completion_outer_state or false
local puzzle_probe_frames = trace_config.puzzle_probe_frames or 120
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

local function action_descriptor(action_selector, action)
    if not action_selector or action_selector == 0 then return nil end
    local raw = dosbox.mem_read_selector(action_selector, 0x200 + action * 8, 8)
    local pool_byte = string.byte(raw, 3) or 0
    local pool_index = pool_byte & 0x0f
    local target_offset = dosbox.mem_read_word("ds", 0x2f5c + pool_index * 2)
    local target_raw = dosbox.mem_read("ds", target_offset, 0x80) or ""
    return {
        selector = action_selector,
        action = action,
        raw_hex = hex(raw),
        pool_byte = pool_byte,
        pool_index = pool_index,
        target_offset = target_offset,
        target_raw_hex = hex(target_raw),
    }
end

local function static_globals(object_selector)
    local carry_y_raw = dosbox.mem_read("ds", 0x8812, 4) or ""
    local carry_x_raw = dosbox.mem_read("ds", 0x8816, 4) or ""
    local action_selector = dosbox.mem_read_word("ds", 0x504e)
    local globals = {
        camera_x = dosbox.mem_read_word("ds", 0x81c0),
        camera_y = dosbox.mem_read_word("ds", 0x81c4),
        action_flags = dosbox.mem_read_word("ds", 0x8196),
        keyboard_flags = dosbox.mem_read_word("ds", 0x88bc),
        bounds_object_offset = dosbox.mem_read_word("ds", 0x881a),
        bounds_object_flag = dosbox.mem_read_word("ds", 0x89ea),
        tile_flag_word = dosbox.mem_read_word("ds", 0x60d8),
        action_word = dosbox.mem_read_word("ds", 0x612e),
        action_table_selector = action_selector,
        action_descriptors = {
            ["2"] = action_descriptor(action_selector, 2),
            ["4"] = action_descriptor(action_selector, 4),
            ["13"] = action_descriptor(action_selector, 13),
        },
        platform_overlap_latch = dosbox.mem_read_word("ds", 0x5006),
        player_carry_y_fixed = dword(carry_y_raw, 1),
        player_carry_x_fixed = dword(carry_x_raw, 1),
        object_global_880c = dosbox.mem_read_word("ds", 0x880c),
        object_global_8806 = dosbox.mem_read_word("ds", 0x8806),
        object_global_8808 = dosbox.mem_read_word("ds", 0x8808),
        object_global_880a = dosbox.mem_read_word("ds", 0x880a),
        effect_table_87de_hex = hex(dosbox.mem_read("ds", 0x87de, 16) or ""),
        contact_player_x = dosbox.mem_read_word("ds", 0x87de),
        contact_player_y = dosbox.mem_read_word("ds", 0x87e0),
        object_global_881c = dosbox.mem_read_word("ds", 0x881c),
        object_global_8822 = dosbox.mem_read_word("ds", 0x8822),
        object_global_8824 = dosbox.mem_read_word("ds", 0x8824),
        cloud_global_89e6 = dosbox.mem_read_word("ds", 0x89e6),
    }
    if object_selector then
        local bounds_offset = globals.bounds_object_offset
        local state = dosbox.mem_read_selector(object_selector, bounds_offset + 0x37, 1)
        globals.player_state_byte = string.byte(state, 1) or 0
    end
    return globals
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
    if force_bump_player_state then
        -- BUMP's shared player helper returns a zero-length gate when the
        -- persistent-player flag is set or player+0x37 is zero.  Keep this
        -- explicit debugger-only state control separate from the normal
        -- active bounds override.
        dosbox.mem_write("ds", 0x89ea, little_word(0))
        dosbox.mem_write_selector(object_selector, bounds_offset + 0x37,
                                  string.char(0x01))
    end
    local aligned_x_fixed = player.position.x_fixed + align_x_offset * 0x10000
    dosbox.mem_write_selector(object_selector, object_offset + 0x02,
                              little_dword(aligned_x_fixed))
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
        x_offset = align_x_offset,
        y_offset = align_y_offset,
    }
end
local callback_offset = initialized_object.update_callback
assert(callback_offset ~= 0,
       "initialized object has no per-object callback")
local function apply_cloud_player_state()
    -- Debugger-only WOLKE probe: make the cloud object its own synthetic
    -- player-bounds object.  This preserves the callback's native overlap
    -- arithmetic while satisfying the player-state gate at 92A2.
    dosbox.mem_write("ds", 0x89ea, little_word(0))
    dosbox.mem_write("ds", 0x881a, little_word(object_offset))
    dosbox.mem_write_selector(object_selector, object_offset + 0x2c,
                              little_word(0x0000))
    dosbox.mem_write_selector(object_selector, object_offset + 0x2e,
                              little_word(0x0000))
    dosbox.mem_write_selector(object_selector, object_offset + 0x30,
                              little_word(0x0010))
    dosbox.mem_write_selector(object_selector, object_offset + 0x32,
                              little_word(0x0010))
    dosbox.mem_write_selector(object_selector, object_offset + 0x37,
                              string.char(0x00))
end

local function record_state(selector, offset)
    local raw = dosbox.mem_read_selector(selector, offset, 6) or ""
    return {
        selector = selector,
        offset = offset,
        raw_hex = hex(raw),
        type = (#raw >= 2 and (word(raw, 1) & 0xff) or nil),
        claimed = (#raw >= 2 and string.byte(raw, 2) or nil),
    }
end

local function pool_object_for_record(record_offset)
    local pointer = dosbox.mem_read("ds", 0x755e, 4) or ""
    if #pointer < 4 then return nil end
    local pool_offset = word(pointer, 1)
    local pool_selector = word(pointer, 3)
    local stride = dosbox.mem_read_word("ds", 0x30ce)
    if pool_selector == 0 or stride == 0 then return nil end
    for index = 0, 63 do
        local candidate_offset = pool_offset + index * stride
        local raw = dosbox.mem_read_selector(pool_selector, candidate_offset, 0x40) or ""
        if #raw >= 0x1c and word(raw, 0x18 + 1) ~= 0 and
                word(raw, 0x1a + 1) == record_offset then
            return {
                selector = pool_selector,
                offset = candidate_offset,
                index = index,
                snapshot = object_snapshot(pool_selector, candidate_offset),
            }
        end
    end
    return nil
end

if trace_stream_lifecycle then
    -- Lifecycle probe: force the selected object outside the native camera
    -- window, capture 1DEE's record-claim reset, then move the camera back
    -- across a 64-pixel region boundary and capture 1E04 revisiting the same
    -- runtime ARE record. This is deliberately separate from ordinary callback
    -- sampling so the stream scan and object-pool reuse remain observable.
    local runtime_selector = entry.registers.fs
    local runtime_record_offset = entry.registers.ebx & 0xffff
    local initial_camera = {
        x = dosbox.mem_read_word("ds", 0x81c0),
        y = dosbox.mem_read_word("ds", 0x81c4),
        stream_x = dosbox.mem_read_word("ds", 0x3710),
        stream_y = dosbox.mem_read_word("ds", 0x3712),
    }
    local initial_record_state = record_state(runtime_selector, runtime_record_offset)

    dosbox.mem_write("ds", 0x81c0, little_word(0))
    dosbox.mem_write("ds", 0x81c4, little_word(0))
    dosbox.breakpoint_set(0x01f7, callback_offset, {once = true})
    dosbox.debug_continue()
    local callback_hit = nil
    for attempt = 1, 512 do
        local hit = wait_hit("lifecycle off-camera callback")
        if hit.segment == 0x01f7 and hit.offset == callback_offset and
                hit.registers.es == object_selector and
                (hit.registers.edi & 0xffff) == object_offset then
            callback_hit = hit
            break
        end
        dosbox.breakpoint_set(0x01f7, callback_offset, {once = true})
        dosbox.debug_continue()
    end
    assert(callback_hit ~= nil, "lifecycle probe did not find the selected callback")

    dosbox.breakpoint_set(0x01f7, 0x1dee, {once = true})
    dosbox.debug_continue()
    local removal_hit = nil
    for attempt = 1, 512 do
        local hit = wait_hit("lifecycle removal helper")
        if hit.segment == 0x01f7 and hit.offset == 0x1dee and
                hit.registers.es == object_selector and
                (hit.registers.edi & 0xffff) == object_offset then
            removal_hit = hit
            break
        end
        dosbox.breakpoint_set(0x01f7, 0x1dee, {once = true})
        dosbox.debug_continue()
    end
    assert(removal_hit ~= nil, "lifecycle probe did not reach 1DEE for the selected object")

    local callback_return = stack_return(callback_hit)
    assert(callback_return ~= nil, "lifecycle callback has no near return address")
    dosbox.breakpoint_set(callback_return.segment, callback_return.offset, {once = true})
    dosbox.debug_continue()
    local removal_return = wait_hit("lifecycle callback return")
    local removed_object = object_snapshot(object_selector, object_offset)
    local removed_record = record_state(runtime_selector, runtime_record_offset)

    -- Reset the stream-region cache as well as the camera so the next main-loop
    -- pass must revisit the target's 64-pixel cell rather than treating it as
    -- already seen.
    dosbox.mem_write("ds", 0x3710, little_word(0))
    dosbox.mem_write("ds", 0x3712, little_word(0))
    dosbox.mem_write("ds", 0x81c0, little_word(lifecycle_return_camera_x))
    dosbox.mem_write("ds", 0x81c4, little_word(lifecycle_return_camera_y))

    local declaration_hit = nil
    local declaration_record_before = nil
    local stream_hits = {}
    for attempt = 1, 4096 do
        dosbox.breakpoint_set(0x01f7, 0x1cda, {once = true})
        dosbox.breakpoint_set(0x01f7, 0x1e04, {once = true})
        dosbox.debug_continue()
        local hit = wait_hit("lifecycle re-stream declaration")
        if hit.segment == 0x01f7 and hit.offset == 0x1cda then
            if #stream_hits < 32 then
                stream_hits[#stream_hits + 1] = {
                    segment = hit.segment,
                    offset = hit.offset,
                    registers = hit.registers,
                    camera = {
                        x = dosbox.mem_read_word("ds", 0x81c0),
                        y = dosbox.mem_read_word("ds", 0x81c4),
                    },
                }
            end
            dosbox.mem_write("ds", 0x81c0, little_word(lifecycle_return_camera_x))
            dosbox.mem_write("ds", 0x81c4, little_word(lifecycle_return_camera_y))
        end
        if hit.segment == 0x01f7 and hit.offset == 0x1e04 then
            local candidate_offset = hit.registers.ebx & 0xffff
            local candidate_record = record_state(runtime_selector, candidate_offset)
            if candidate_offset == runtime_record_offset then
                declaration_hit = hit
                declaration_record_before = candidate_record
                break
            end
        end
        dosbox.mem_write("ds", 0x81c0, little_word(lifecycle_return_camera_x))
        dosbox.mem_write("ds", 0x81c4, little_word(lifecycle_return_camera_y))
    end

    local declaration_return = nil
    local declaration_return_hit = nil
    local reconstructed_object = nil
    local declaration_record_after = nil
    if declaration_hit ~= nil then
        declaration_return = stack_return(declaration_hit)
        assert(declaration_return ~= nil, "1E04 has no near return address")
        dosbox.breakpoint_set(declaration_return.segment, declaration_return.offset,
                              {once = true})
        dosbox.debug_continue()
        declaration_return_hit = wait_hit("lifecycle re-stream declaration return")
        declaration_record_after = record_state(runtime_selector, runtime_record_offset)
        reconstructed_object = pool_object_for_record(runtime_record_offset)
    end

    dosbox.output.behavior_trace = {
        trace_schema_version = 1,
        trace_kind = "object-stream-lifecycle",
        type = expected_type,
        record_offset = record_offset,
        runtime_record = {selector = runtime_selector, offset = runtime_record_offset},
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
        lifecycle = {
            initial_camera = initial_camera,
            initial_record = initial_record_state,
            off_camera_callback = {
                segment = callback_hit.segment,
                offset = callback_hit.offset,
                registers = callback_hit.registers,
            },
            removal_helper = {
                segment = removal_hit.segment,
                offset = removal_hit.offset,
                registers = removal_hit.registers,
            },
            removal_return = {
                segment = removal_return.segment,
                offset = removal_return.offset,
                registers = removal_return.registers,
            },
            removed_object = removed_object,
            removed_record = removed_record,
            return_camera = {x = lifecycle_return_camera_x,
                             y = lifecycle_return_camera_y},
            stream_hits = stream_hits,
            declaration = declaration_hit and {
                segment = declaration_hit.segment,
                offset = declaration_hit.offset,
                registers = declaration_hit.registers,
            } or nil,
            declaration_record_before = declaration_record_before,
            declaration_return = declaration_return_hit and {
                segment = declaration_return_hit.segment,
                offset = declaration_return_hit.offset,
                registers = declaration_return_hit.registers,
            } or nil,
            declaration_record_after = declaration_record_after,
            reconstructed_object = reconstructed_object,
            re_streamed = declaration_hit ~= nil and declaration_return_hit ~= nil,
        },
        samples = {},
    }
    dosbox.debug_continue()
    return
end

if force_contact_gate then
    -- Controlled branch probe: the native callback gates this path on
    -- DS:8806 and compares the object integer coordinates against the
    -- selected pair in DS:87DE/87E0.  Keep this explicit debugger-only
    -- control separate from the unmodified overlap alignment.
    dosbox.mem_write("ds", 0x8806, little_word(1))
    dosbox.mem_write("ds", 0x87de, little_word(initialized_object.position.x))
    dosbox.mem_write("ds", 0x87e0, little_word(initialized_object.position.y))
end
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
        -- Let the 9256 initializer run with the native player pointer first;
        -- apply the synthetic bounds only once the steady-state 9269 callback
        -- has been installed.
        if force_cloud_player_state and callback_offset ~= 0x9256 then
            apply_cloud_player_state()
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
        if force_bump_player_state then
            local bounds_offset = dosbox.mem_read_word("ds", 0x881a)
            dosbox.mem_write("ds", 0x89ea, little_word(0))
            dosbox.mem_write_selector(object_selector, bounds_offset + 0x37,
                                      string.char(0x01))
        end
        if force_tile_mask ~= nil then
            dosbox.mem_write("ds", 0x60d8, little_word(force_tile_mask))
        end
        local before = object_snapshot(object_selector, object_offset)
        if force_contact_gate then
            dosbox.mem_write("ds", 0x8806, little_word(1))
            dosbox.mem_write("ds", 0x87de, little_word(before.position.x))
            -- WURM2's contact window is evaluated after its movement step;
            -- place the controlled player sample just above the object's
            -- post-update Y interval while leaving the branch mechanics
            -- explicit.
            dosbox.mem_write("ds", 0x87e0, little_word(before.position.y - 12))
        end
        local globals_before = static_globals(object_selector)
        local returned = stack_return(callback_entry)
        assert(returned ~= nil, "object callback has no near return address")
        local overlap_probe = nil
        local collision_probe = nil
        local bump_probe = nil
        local contact_probe = nil
        local puzzle_probe = nil
        local callback_return = nil
        if trace_overlap or trace_collision or trace_platform or trace_bump or trace_contact or trace_puzzle_completion then
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
            if trace_contact then
                contact_probe = {hits = {}}
                dosbox.breakpoint_set(0x01f7, 0x707b, {once = true})
                dosbox.breakpoint_set(0x01f7, 0x70c9, {once = true})
                dosbox.breakpoint_set(0x01f7, 0x4ab3, {once = true})
                dosbox.breakpoint_set(0x01f7, 0x4ba0, {once = true})
                dosbox.breakpoint_set(0x01f7, 0x4c5d, {once = true})
                dosbox.breakpoint_set(0x01f7, 0x4c74, {once = true})
            end
            if trace_puzzle_completion then
                puzzle_probe = {hits = {}}
                dosbox.breakpoint_set(0x01f7, 0x5936, {once = true})
                dosbox.breakpoint_set(0x01f7, 0x5940, {once = true})
                dosbox.breakpoint_set(0x01f7, 0x5a03, {once = true})
                dosbox.breakpoint_set(0x01f7, 0x5af5, {once = true})
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
                if trace_contact and (probe_hit.offset == 0x707b or
                        probe_hit.offset == 0x70c9 or probe_hit.offset == 0x4ab3 or
                        probe_hit.offset == 0x4ba0 or probe_hit.offset == 0x4c5d or
                        probe_hit.offset == 0x4c74) then
                    contact_probe.hits[#contact_probe.hits + 1] = hit_record
                end
                if trace_puzzle_completion and (probe_hit.offset == 0x5936 or
                        probe_hit.offset == 0x5940 or probe_hit.offset == 0x5a03 or
                        probe_hit.offset == 0x5af5) then
                    puzzle_probe.hits[#puzzle_probe.hits + 1] = hit_record
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
            globals_after = static_globals(object_selector),
            object_before = before,
            object_after = after,
            changed_bytes = changed_bytes(before.raw_hex, after.raw_hex),
            overlap_probe = overlap_probe,
            collision_probe = collision_probe,
            bump_probe = bump_probe,
            contact_probe = contact_probe,
            puzzle_probe = puzzle_probe,
        }
        callback_offset = after.update_callback
        if callback_offset == 0 then break end
    end
end

assert(#samples > 0,
       "captured no callbacks for the initialized object")
local puzzle_completion_probe = nil
if trace_puzzle_completion and force_tile_mask ~= nil and callback_offset == 0 then
    -- The final-letter callback has returned and cleared the live object. Let
    -- the native game loop run for a bounded interval, then sample the puzzle
    -- mask, loader globals, and current execution state. A transition would
    -- change these fields even when no direct DS:60D8 comparator is present.
    local presentation_points = {
        {segment = 0x01d7, offset = 0x14e1},
        {segment = 0x01d7, offset = 0x4eaa},
        {segment = 0x01d7, offset = 0x4f0d},
        {segment = 0x0207, offset = 0x10a9},
        {segment = 0x0207, offset = 0x10cb},
        {segment = 0x0207, offset = 0x1113},
        {segment = 0x01d7, offset = 0x1670},
        {segment = 0x01d7, offset = 0x16c6},
        {segment = 0x01d7, offset = 0x16de},
        {segment = 0x01d7, offset = 0x16f0},
        {segment = 0x01d7, offset = 0x1704},
        {segment = 0x01d7, offset = 0x4f10},
        {segment = 0x01d7, offset = 0x4faf},
        {segment = 0x01d7, offset = 0x5017},
        {segment = 0x01d7, offset = 0x5038},
        {segment = 0x01d7, offset = 0x5047},
    }
    local outer_state_point = {segment = 0x01d7, offset = 0x4ea0}
    local presentation_hits = {}
    local presentation_timeout_ms = force_completion_outer_state and
        math.min(timeout_ms, 30000) or math.min(timeout_ms, 5000)
    local function arm_presentation_points()
        for _, point in ipairs(presentation_points) do
            dosbox.breakpoint_set(point.segment, point.offset, {once = true})
        end
    end
    if force_completion_outer_state then
        -- The authored comparator is reached from the outer cloud-state
        -- consumer (01D7:4EA0), not from the ordinary object callback.  A
        -- real nearby cloud writes DS:89E6=-1; seed that gate here only for
        -- the diagnostic handoff probe.
        dosbox.mem_write("ds", 0x89e6, little_word(0xffff))
        dosbox.breakpoint_set(outer_state_point.segment, outer_state_point.offset,
                              {once = true})
    end
    arm_presentation_points()
    dosbox.debug_continue()
    for sequence = 1, 32 do
        local hit = dosbox.wait_for_breakpoint(presentation_timeout_ms)
        if not hit then break end
        local is_presentation = false
        for _, point in ipairs(presentation_points) do
            if hit.segment == point.segment and hit.offset == point.offset then
                is_presentation = true
                break
            end
        end
        if hit.segment == outer_state_point.segment and
                hit.offset == outer_state_point.offset then
            is_presentation = true
        end
        if is_presentation then
            local stack = dosbox.mem_read("ss", hit.registers.esp & 0xffff, 12) or ""
            presentation_hits[#presentation_hits + 1] = {
                sequence = #presentation_hits + 1,
                hit = {segment = hit.segment, offset = hit.offset,
                       registers = hit.registers},
                stack_hex = hex(stack),
                tile_mask = dosbox.mem_read_word("ds", 0x60d8),
                completion_flag = dosbox.mem_read_word("ds", 0x85db),
                selector_index = dosbox.mem_read_word("ds", 0x85d4),
                selector_state = dosbox.mem_read_word("ds", 0x85d6),
                action_word = dosbox.mem_read_word("ds", 0x612e),
                timer_tick = dosbox.mem_read_word("ds", 0x97f4),
            }
        end
        if #presentation_hits >= 32 then break end
        dosbox.debug_continue()
    end
    dosbox.debug_continue()
    dosbox.wait_frames(puzzle_probe_frames)
    local resource_state = dosbox.mem_read("ds", 0x97e4, 12) or ""
    puzzle_completion_probe = {
        forced_mask = force_tile_mask,
        frames = puzzle_probe_frames,
        globals = static_globals(nil),
        resource = {
            end_offset = dword(resource_state, 1),
            start_offset = dword(resource_state, 5),
            size = dword(resource_state, 9),
        },
        selector_index = dosbox.mem_read_word("ds", 0x85d4),
        completion_flag = dosbox.mem_read_word("ds", 0x85db),
        selector_state = dosbox.mem_read_word("ds", 0x85d6),
        score_lo = dosbox.mem_read_word("ds", 0x881c),
        score_hi = dosbox.mem_read_word("ds", 0x881e),
        level_loop_state = dosbox.mem_read_word("ds", 0x819e),
        presentation_hits = presentation_hits,
        outer_state_forced = force_completion_outer_state,
        cpu = dosbox.cpu_state(),
    }
end
local reload_probe = nil
if reload_after_collect then
    assert(callback_offset == 0,
           "--reload-after-collect requires the first callback to clear after collection")
    assert(reload_level ~= "", "reload selector level is empty")

    -- The callback return breakpoint leaves the machine paused in gameplay.
    -- Let the level loop settle before invoking the same native selector path
    -- used for the initial load.  This is deliberately a stateful probe: the
    -- second ARE declaration and factory allocation are captured independently
    -- of the first object's cleared callback.
    dosbox.debug_continue()
    dosbox.wait_frames(reload_wait_frames)
    local second_declaration = choose_level(reload_level)
    local second_target = nil
    for attempt = 1, 4096 do
        if second_declaration then
            second_declaration = false
        else
            dosbox.breakpoint_set(0x01f7, 0x1e04, {once = true})
        end
        local declaration = wait_hit("reload ARE declaration")
        local declaration_record = dosbox.mem_read("fs", declaration.registers.ebx & 0xffff, 6)
        local declaration_type = word(declaration_record, 1) & 0xff
        if (declaration.registers.ebx & 0xffff) == runtime_offset then
            second_target = {entry = declaration, record = declaration_record,
                             entity_type = declaration_type}
            break
        end
        dosbox.debug_continue()
        dosbox.wait_frames(1)
    end
    reload_probe = {
        selected_level = reload_level,
        target_declaration = nil,
        reconstructed = false,
    }
    if second_target ~= nil then
        reload_probe.target_declaration = {
            selector = second_target.entry.registers.fs,
            offset = second_target.entry.registers.ebx & 0xffff,
            type = second_target.entity_type,
            record_hex = hex(second_target.record),
            entry = {segment = second_target.entry.segment,
                     offset = second_target.entry.offset,
                     registers = second_target.entry.registers},
        }
        local reload_dispatch_offset = 0x81d2 + expected_type * 4
        local reload_dispatch = dosbox.mem_read("ds", reload_dispatch_offset, 4)
        local reload_callback_offset = word(reload_dispatch, 1)
        if second_target.entity_type == expected_type and reload_callback_offset ~= 0 then
            dosbox.breakpoint_set(0x01f7, 0x1e8e, {once = true})
            dosbox.debug_continue()
            local reload_factory = wait_hit("reload object factory return")
            local reload_object_selector = reload_factory.registers.es
            local reload_object_offset = reload_factory.registers.edi & 0xffff
            -- The factory return still contains the dispatch/initializer
            -- callback (for 0x6f this is 8bc2).  Run that callback once and
            -- sample the object only after its near return, when object+0x18
            -- has been replaced by the shared update callback (8d20).
            local reload_initializer = nil
            local reload_initializer_return = nil
            for initializer_attempt = 1, 16 do
                dosbox.breakpoint_set(0x01f7, reload_callback_offset,
                                      {once = true})
                dosbox.debug_continue()
                local initializer_hit = wait_hit("reload object initializer")
                local matches_reload_object = initializer_hit.segment == 0x01f7 and
                    initializer_hit.offset == reload_callback_offset and
                    initializer_hit.registers.es == reload_object_selector and
                    (initializer_hit.registers.edi & 0xffff) == reload_object_offset
                if matches_reload_object then
                    reload_initializer = initializer_hit
                    local returned = stack_return(initializer_hit)
                    assert(returned ~= nil,
                           "reload object initializer has no near return address")
                    dosbox.breakpoint_set(returned.segment, returned.offset,
                                          {once = true})
                    dosbox.debug_continue()
                    reload_initializer_return = wait_hit(
                        "reload object initializer return")
                    break
                end
            end
            assert(reload_initializer ~= nil,
                   "reload object initializer did not match allocated object")
            local reload_object = object_snapshot(reload_object_selector,
                                                  reload_object_offset)
            reload_probe.reconstructed = reload_object.update_callback ~= 0 and
                reload_object.update_callback ~= reload_callback_offset
            reload_probe.dispatch = {
                segment = 0x01f7,
                offset = reload_callback_offset,
                raw_hex = hex(reload_dispatch),
            }
            reload_probe.factory = {
                segment = reload_factory.segment,
                offset = reload_factory.offset,
                registers = reload_factory.registers,
            }
            reload_probe.initializer = {
                segment = reload_initializer.segment,
                offset = reload_initializer.offset,
                registers = reload_initializer.registers,
            }
            reload_probe.initializer_return = {
                segment = reload_initializer_return.segment,
                offset = reload_initializer_return.offset,
                registers = reload_initializer_return.registers,
            }
            reload_probe.object = reload_object
            reload_probe.callback_installed = reload_object.update_callback
        end
    end
end
local cloud_consumer_probe = nil
local cloud_outer_renderer_probe = nil
local cloud_hardware_renderer_probe = nil
if trace_cloud_outer_renderer and expected_type == 0x28 then
    -- WOLKE leaves object+0x12 at FFFF, so observe the main-loop state branch
    -- after the callback returns. This documents the outer DS:89E6 consumer
    -- even when the standard object queue deliberately skips FFFF.  Keep the
    -- queue append/draw boundary armed at the same time: the special path may
    -- enqueue an explicit descriptor rather than passing the cloud object to
    -- the normal 1024 renderer.
    cloud_outer_renderer_probe = {
        frames = 64,
        breakpoints = {"01D7:4EA0", "01D7:4EAA", "01D7:4F03", "01D7:4F08",
                       "01F7:34BC", "01F7:3587", "01F7:0013"},
        samples = {},
    }
    local offsets = {
        {segment = 0x01d7, offset = 0x4ea0},
        {segment = 0x01d7, offset = 0x4eaa},
        {segment = 0x01d7, offset = 0x4f03},
        {segment = 0x01d7, offset = 0x4f08},
        {segment = 0x01f7, offset = 0x34bc},
        {segment = 0x01f7, offset = 0x3587},
        {segment = 0x01f7, offset = 0x0013},
    }
    local function arm_all()
        for _, point in ipairs(offsets) do
            dosbox.breakpoint_set(point.segment, point.offset, {once = true})
        end
    end
    local function queue_snapshot()
        local queue_ptr = dosbox.mem_read("ds", 0x6d86, 4) or ""
        local descriptor_ptr = dosbox.mem_read("ds", 0x6d8a, 4) or ""
        local queue_offset = (#queue_ptr >= 4 and word(queue_ptr, 1) or 0)
        local queue_selector = (#queue_ptr >= 4 and word(queue_ptr, 3) or 0)
        local descriptor_offset = (#descriptor_ptr >= 4 and word(descriptor_ptr, 1) or 0)
        local descriptor_selector = (#descriptor_ptr >= 4 and word(descriptor_ptr, 3) or 0)
        local count = dosbox.mem_read_word("ds", 0x8174)
        local entries = {}
        if queue_selector ~= 0 and count > 0 and count < 0x100 then
            local raw = dosbox.mem_read_selector(queue_selector, queue_offset, count * 8) or ""
            for index = 0, count - 1 do
                local start = index * 8 + 1
                local entry = raw:sub(start, start + 7)
                entries[#entries + 1] = {
                    index = index,
                    raw_hex = hex(entry),
                    x = word(entry, 1),
                    y = word(entry, 3),
                    logical_slot = word(entry, 5),
                    flags = string.byte(entry, 7) or 0,
                    mode = string.byte(entry, 8) or 0,
                }
            end
        end
        return {
            count = count,
            queue_offset = queue_offset,
            queue_selector = queue_selector,
            descriptor_offset = descriptor_offset,
            descriptor_selector = descriptor_selector,
            entries = entries,
        }
    end
    arm_all()
    dosbox.debug_continue()
    for sequence = 1, cloud_outer_renderer_probe.frames do
        local hit = dosbox.wait_for_breakpoint(timeout_ms)
        if not hit then break end
        local stack_offset = hit.registers.esp & 0xffff
        local stack = dosbox.mem_read("ss", stack_offset, 16) or ""
        local sample = {
            sequence = sequence,
            hit = {segment = hit.segment, offset = hit.offset,
                   registers = hit.registers},
            stack_hex = hex(stack),
            return_offset = (#stack >= 2 and word(stack, 1) or nil),
            return_segment = (#stack >= 4 and word(stack, 3) or nil),
            cloud_global_89e6 = dosbox.mem_read_word("ds", 0x89e6),
            queue = queue_snapshot(),
        }
        if hit.segment == 0x01f7 and hit.offset == 0x34bc then
            sample.incoming = {
                x = hit.registers.eax & 0xffff,
                y = hit.registers.ebx & 0xffff,
                logical_slot = hit.registers.edx & 0xffff,
                flags = hit.registers.ecx & 0xff,
            }
        end
        cloud_outer_renderer_probe.samples[#cloud_outer_renderer_probe.samples + 1] = sample
        if #cloud_outer_renderer_probe.samples >= cloud_outer_renderer_probe.frames then break end
        arm_all()
        dosbox.debug_continue()
    end
end
if trace_cloud_hardware_renderer and expected_type == 0x28 then
    -- The cloud has no logical object slot, but its special-state path can
    -- still enter the generic VGA/BOB blitter with an explicit slot argument.
    -- Capture the entry stack and the resolved descriptor to identify that
    -- special call without tracing every normal sprite draw.
    cloud_hardware_renderer_probe = {
        frames = cloud_hardware_frames,
        breakpoint = "01F7:0013",
        samples = {},
    }
    dosbox.breakpoint_set(0x01f7, 0x0013, {once = true})
    dosbox.debug_continue()
    for sequence = 1, cloud_hardware_frames do
        local hit = wait_hit("cloud hardware renderer entry")
        local stack_offset = hit.registers.esp & 0xffff
        local stack = dosbox.mem_read("ss", stack_offset, 32) or ""
        local param1 = (#stack >= 6 and word(stack, 5) or nil)
        local param2 = (#stack >= 8 and word(stack, 7) or nil)
        local param3 = (#stack >= 10 and word(stack, 9) or nil)
        local param4 = (#stack >= 12 and word(stack, 11) or nil)
        local descriptor = nil
        local map_index = nil
        if param2 ~= nil and param2 < 0x800 then
            map_index = dosbox.mem_read_word("ds", 0x6d8e + param2 * 2)
            local descriptor_offset = dosbox.mem_read_word("ds", 0x6d8a)
            local descriptor_selector = dosbox.mem_read_word("ds", 0x6d8c)
            local stride = dosbox.mem_read_word("ds", 0x30d2)
            if descriptor_offset ~= 0 and descriptor_selector ~= 0 and
               map_index ~= 0xffff and stride ~= 0 then
                local descriptor_raw = dosbox.mem_read_selector(
                    descriptor_selector, descriptor_offset + map_index * stride, 0x2c)
                descriptor = {
                    selector = descriptor_selector,
                    offset = descriptor_offset + map_index * stride,
                    map_index = map_index,
                    stride = stride,
                    raw_hex = hex(descriptor_raw),
                    width = word(descriptor_raw, 1),
                    height = word(descriptor_raw, 3),
                    origin_x = word(descriptor_raw, 9),
                    origin_y = word(descriptor_raw, 11),
                    blitter_offset = word(descriptor_raw, 0x10 + 1),
                    blitter_selector = word(descriptor_raw, 0x12 + 1),
                }
            end
        end
        cloud_hardware_renderer_probe.samples[#cloud_hardware_renderer_probe.samples + 1] = {
            sequence = sequence,
            hit = {segment = hit.segment, offset = hit.offset,
                   registers = hit.registers},
            stack_offset = stack_offset,
            stack_hex = hex(stack),
            return_offset = (#stack >= 2 and word(stack, 1) or nil),
            return_segment = (#stack >= 4 and word(stack, 3) or nil),
            params = {flags = param1, logical_slot = param2,
                      y = param3, x = param4},
            object_slot = word(dosbox.mem_read_selector(
                object_selector, object_offset + 0x12, 2), 1),
            map_index = map_index,
            descriptor = descriptor,
            cloud_global_89e6 = dosbox.mem_read_word("ds", 0x89e6),
        }
        if #cloud_hardware_renderer_probe.samples >= cloud_hardware_frames then break end
        dosbox.breakpoint_set(hit.segment, hit.offset, {once = true})
        dosbox.debug_continue()
    end
end
if trace_cloud_consumers and expected_type == 0x28 then
    -- 4087 and 4406 are the player-side readers of DS:89E6.  Capture one
    -- reader from the paused post-callback state.  A one-shot capture avoids
    -- perturbing the scheduler with nested player-callback breakpoints.
    assert(cloud_consumer_offset == 0x4087 or cloud_consumer_offset == 0x4406,
           "cloud consumer offset must be 0x4087 or 0x4406")
    cloud_consumer_probe = {
        frames = 1,
        reader_offsets = {string.format("01F7:%04X", cloud_consumer_offset)},
        samples = {},
    }
    dosbox.breakpoint_set(0x01f7, cloud_consumer_offset, {once = true})
    dosbox.debug_continue()
    local hit = wait_hit("cloud consumer reader")
    cloud_consumer_probe.samples[1] = {
        sequence = 1,
        hit = {segment = hit.segment, offset = hit.offset,
               registers = hit.registers},
        cloud_global_89e6 = dosbox.mem_read_word("ds", 0x89e6),
    }
end
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
    puzzle_completion_probe = puzzle_completion_probe,
    cloud_consumer_probe = cloud_consumer_probe,
    cloud_outer_renderer_probe = cloud_outer_renderer_probe,
    cloud_hardware_renderer_probe = cloud_hardware_renderer_probe,
    reload_probe = reload_probe,
    samples = samples,
}
dosbox.debug_continue()
