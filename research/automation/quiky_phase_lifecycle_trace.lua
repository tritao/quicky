-- Lightweight lifecycle sampler for the B33B linked-object phases.
-- It uses one debugger stop to force the phase boundary, then samples the
-- live pool between ordinary frame waits.  This intentionally avoids a
-- breakpoint on every callback: the callback-heavy tracer is too expensive
-- for the several-hundred-frame phase-3 cycle.
local trace_config = TRACE_CONFIG or {}
assert(type(trace_config) == "table", "TRACE_CONFIG must be a table")

local timeout_ms = trace_config.timeout_ms or 30000
local select_level = trace_config.select_level or ""
local selector_frames = trace_config.selector_frames or 60
local sample_count = trace_config.sample_count or 120
local sample_interval = trace_config.sample_interval or 1
local warmup_frames = trace_config.warmup_frames or 0
local scan_limit = trace_config.scan_limit or 16
local force_phase = trace_config.force_phase
local force_transition = trace_config.force_transition
local force_x = trace_config.force_x
local force_y = trace_config.force_y

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

local function little_dword(value)
    local function little_word(word_value)
        return string.char(word_value & 0xff, (word_value >> 8) & 0xff)
    end
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

local function object_snapshot(selector, offset, index)
    local raw = dosbox.mem_read_selector(selector, offset, 0x48)
    local x_fixed = dword(raw, 3)
    local y_fixed = dword(raw, 7)
    return {
        index = index,
        selector = selector,
        offset = offset,
        raw_hex = hex(raw),
        x_fixed = x_fixed,
        y_fixed = y_fixed,
        x = signed_word((x_fixed >> 16) & 0xffff),
        y = signed_word((y_fixed >> 16) & 0xffff),
        callback = word(raw, 0x18 + 1),
        callback_segment = word(raw, 0x1c + 1),
        source = word(raw, 0x1a + 1),
        scheduler_phase = string.byte(raw, 0x17 + 1),
        sprite_slot = word(raw, 0x12 + 1),
        descriptor_delay = word(raw, 0x1e + 1),
        descriptor_timer = word(raw, 0x20 + 1),
        linked_offset = word(raw, 0x2a + 1),
        lifetime = word(raw, 0x2c + 1),
        state_field = word(raw, 0x2e + 1),
        transition = string.byte(raw, 0x34 + 1),
        linked_child = word(raw, 0x36 + 1),
        phase_timer = word(raw, 0x38 + 1),
        emission_counter = word(raw, 0x44 + 1),
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
    local limit = math.min(64, math.max(1, scan_limit))
    for index = 0, limit - 1 do
        local offset = base + index * stride
        local ok, object = pcall(object_snapshot, selector, offset, index)
        if ok and object.callback ~= 0 then objects[#objects + 1] = object end
    end
    return {
        selector = selector,
        base = base,
        stride = stride,
        scanned = limit,
        active_count = #objects,
        objects = objects,
    }
end

local function globals_snapshot()
    return {
        game_phase = dosbox.mem_read_byte("ds", 0x88ae),
        camera_x = dosbox.mem_read_word("ds", 0x81c0),
        camera_y = dosbox.mem_read_word("ds", 0x81c4),
        scheduler_cursor = dosbox.mem_read_word("ds", 0x7966),
        action_word = dosbox.mem_read_word("ds", 0x612e),
    }
end

local function force_owner_phase(hit)
    local selector = hit.registers.es or 0
    local offset = (hit.registers.edi or 0) & 0xffff
    if force_phase ~= nil then
        dosbox.mem_write("ds", 0x88ae, string.char(force_phase & 0xff))
    end
    if force_transition ~= nil then
        dosbox.mem_write_selector(selector, offset + 0x34,
                                   string.char(force_transition & 0xff))
    end
    if force_x ~= nil then
        dosbox.mem_write_selector(selector, offset + 0x02,
                                   little_dword(force_x << 16))
    end
    if force_y ~= nil then
        dosbox.mem_write_selector(selector, offset + 0x06,
                                   little_dword(force_y << 16))
    end
    return object_snapshot(selector, offset, -1)
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

dosbox.breakpoint_clear()
dosbox.breakpoint_set(0x01f7, 0xb33b, {once = true})
dosbox.breakpoint_set(0x1997, 0xb33b, {once = true})
dosbox.debug_continue()
local owner_hit = wait_hit("B33B phase probe")
local owner_before = object_snapshot(owner_hit.registers.es,
                                     owner_hit.registers.edi & 0xffff, -1)
local owner_forced = force_owner_phase(owner_hit)
dosbox.breakpoint_clear()
dosbox.debug_continue()
if warmup_frames > 0 then dosbox.wait_frames(warmup_frames) end

local samples = {}
for sequence = 1, sample_count do
    dosbox.wait_frames(sample_interval)
    samples[#samples + 1] = {
        sequence = sequence,
        globals = globals_snapshot(),
        pool = pool_snapshot(),
    }
end

dosbox.output.phase_lifecycle_trace = {
    trace_schema_version = 1,
    trace_kind = "phase-lifecycle",
    select_level = select_level,
    owner_hit = {
        segment = owner_hit.segment,
        offset = owner_hit.offset,
        registers = owner_hit.registers,
    },
    owner_before = owner_before,
    owner_forced = owner_forced,
    samples = samples,
}
dosbox.debug_continue()
