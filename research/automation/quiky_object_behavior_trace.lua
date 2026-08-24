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
        target_declaration = {entry = entry, record = record}
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
local object_selector = factory_return.registers.es
local object_offset = factory_return.registers.edi & 0xffff
local initial_object = object_snapshot(object_selector, object_offset)
if camera_x >= 0 then dosbox.mem_write("ds", 0x81c0, little_word(camera_x)) end
if camera_y >= 0 then dosbox.mem_write("ds", 0x81c4, little_word(camera_y)) end
-- DS:81D2 contains the normal object update callback. The object word at
-- +0x18 is a separate per-object callback slot used by some sprite paths.
local class_entry_offsets = {0x0ec7, 0x0eee, 0x0f14}
local class_return_offsets = {[0] = 0x0ed3, [1] = 0x0efd, [2] = 0x0f26}
local object_class = initial_object.object_class
assert(class_return_offsets[object_class] ~= nil,
       string.format("unsupported object scheduler class %s", tostring(object_class)))

local samples = {}
local attempts = 0
while #samples < sample_count and attempts < sample_count * 128 do
    attempts = attempts + 1
    for _, offset in ipairs(class_entry_offsets) do
        dosbox.breakpoint_set(0x01f7, offset, {once = true})
    end
    dosbox.debug_continue()
    local scheduler_hit = wait_hit("object scheduler entry")
    local scheduler_entry = class_entry_offsets[object_class + 1]
    local scheduler_match = scheduler_hit.offset == scheduler_entry and
        scheduler_hit.registers.es == object_selector and
        (scheduler_hit.registers.edi & 0xffff) == object_offset
    if scheduler_match then
        local before = object_snapshot(object_selector, object_offset)
        local expected_return = class_return_offsets[object_class]
        local callback_offset = scheduler_hit.registers.eax & 0xffff
        assert(callback_offset ~= 0, "scheduler supplied no object callback")
        dosbox.breakpoint_set(0x01f7, callback_offset, {once = true})
        dosbox.breakpoint_set(0x01f7, expected_return, {once = true})
        dosbox.debug_continue()
        local callback_entry = wait_hit("object callback entry")
        assert(callback_entry.registers.es == object_selector and
               (callback_entry.registers.edi & 0xffff) == object_offset,
               "object callback entry did not carry the target object")
        local returned = stack_return(callback_entry)
        assert(returned ~= nil, "object callback has no near return address")
        dosbox.debug_continue()
        local callback_return = wait_hit("object callback return")
        local after = object_snapshot(object_selector, object_offset)
        samples[#samples + 1] = {
            sequence = #samples + 1,
            scheduler = {
                segment = scheduler_hit.segment,
                offset = scheduler_hit.offset,
                registers = scheduler_hit.registers,
                expected_return = expected_return,
                callback_offset = callback_offset,
            },
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
            globals_before = static_globals(),
            object_before = before,
            object_after = after,
            changed_bytes = changed_bytes(before.raw_hex, after.raw_hex),
        }
    end
end

assert(#samples == sample_count,
       string.format("captured %d/%d object callbacks", #samples, sample_count))
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
    camera_override = {x = camera_x, y = camera_y},
    samples = samples,
}
dosbox.debug_continue()
