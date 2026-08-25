-- Quiky resource lookup tracer. Loaded by research/tools/quikytrace.py.
local count = TRACE_COUNT or 2
local timeout_ms = TRACE_TIMEOUT_MS or 30000
local prepare_w1l3 = TRACE_PREPARE_W1L3 or false
local navigate_w1l3 = TRACE_NAVIGATE_W1L3 or false
local navigate_level = TRACE_NAVIGATE_LEVEL or ""
local select_level = TRACE_SELECT_LEVEL or ""
local selector_frames = TRACE_SELECTOR_FRAMES or 60
local tail_count = TRACE_TAIL_COUNT or 0

local function word(s, index)
    local lo, hi = string.byte(s, index, index + 1)
    return lo | (hi << 8)
end

local function dword(s, index)
    return word(s, index) | (word(s, index + 2) << 16)
end

local function hex(s)
    return (s:gsub(".", function(c) return string.format("%02x", string.byte(c)) end))
end

local function register_for_selector(r, selector)
    for _, name in ipairs({"ds", "es", "ss", "fs", "gs", "cs"}) do
        if r[name] == selector then return name end
    end
    return nil
end

local function wait_hit(label)
    local hit, err = dosbox.wait_for_breakpoint(timeout_ms)
    if not hit then
        error(label .. ": " .. (err or "breakpoint wait failed"))
    end
    return hit
end

local first_entry = nil
if select_level ~= "" then
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
    assert(selector_index, "unsupported level selector target")
    dosbox.output.awaiting_startup_replay = true
    dosbox.wait_frames(350)
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
    local launch = wait_hit("selector Space dispatch")
    dosbox.output.checkpoints.launch = launch
    dosbox.breakpoint_set(0x0207, 0x18c7, {once = true})
    dosbox.debug_continue()
    first_entry = wait_hit("initial resource lookup")
elseif navigate_level ~= "" then
    -- Replay the opening skips and launch the default level. Resource paths
    -- are redirected to the requested level atomically at the lookup
    -- breakpoint below.
    dosbox.output.awaiting_startup_replay = true
    dosbox.wait_frames(350)
    local menu = dosbox.cpu_state()
    assert(menu.ds == 0x0237, "Quiky menu data segment is not initialized")
    dosbox.output.checkpoints = {
        menu = menu,
    }
    -- Arm immediately before the launch key. The first level-load lookup can
    -- happen during the key-down frame, before a later wait_frames() resumes.
    dosbox.breakpoint_set(0x0207, 0x18c7, {once = true})
    dosbox.key("KBD_space", true)
    first_entry = wait_hit("initial resource lookup")
    dosbox.key("KBD_space", false)
elseif prepare_w1l3 then
    local r = dosbox.cpu_state()
    assert(r.cs == 0x01d7 and r.eip == 0x491d,
           "prepare_w1l3 requires a stop at 01D7:491D")
    dosbox.mem_write("ds", 0x89f2, "\x01")
    dosbox.mem_write("ds", 0x88ba, "\x05\x00")
    dosbox.debug_continue()
    dosbox.wait_frames(selector_frames)
    dosbox.mem_write("ds", 0x85d4, "\x02\x00")
    dosbox.key("KBD_space", true)
    dosbox.key("KBD_space", false)
end

-- Startup and selector activity must not be mistaken for trace results.
if navigate_level == "" and select_level == "" then
    dosbox.breakpoint_set(0x0207, 0x18c7, {once = true})
end
dosbox.output.events = {}
local function capture_event(entry, sequence)
    assert(entry.segment == 0x0207 and entry.offset == 0x18c7,
           "unexpected resource lookup breakpoint")
    local r = entry.registers
    local stack = dosbox.mem_read("ss", r.esp, 16)
    local return_offset = word(stack, 1)
    local return_segment = word(stack, 3)
    local path_offset = word(stack, 5)
    local path_segment = word(stack, 7)
    local path_register = register_for_selector(r, path_segment)
    assert(path_register, "Pascal path selector is not in a segment register")
    local path_length = dosbox.mem_read_byte(path_register, path_offset)
    assert(path_length <= 127, "implausible Pascal path length")
    local path = dosbox.mem_read(path_register, path_offset + 1, path_length)
    local original_path = path
    if navigate_level ~= "" then
        local level_at = string.find(path, "W1L1", 1, true)
        if level_at then
            dosbox.mem_write(path_register, path_offset + level_at, navigate_level)
            path = string.gsub(path, "W1L1", navigate_level, 1)
        end
    end

    -- The lookup routine writes DS:97E4 only after it has resolved the
    -- Pascal path. Stop at the caller's far-return address before sampling
    -- that state, otherwise the trace reports the previous resource.
    dosbox.breakpoint_set(return_segment, return_offset, {once = true})
    dosbox.debug_continue()
    local returned = wait_hit("resource lookup return")
    assert(returned.segment == return_segment and returned.offset == return_offset,
           "unexpected resource lookup return breakpoint")
    local state = dosbox.mem_read("ds", 0x97e4, 12)

    return {
        sequence = sequence,
        path = path,
        original_path = original_path,
        entry = {segment = entry.segment, offset = entry.offset},
        return_location = {segment = return_segment, offset = return_offset},
        path_pointer = {segment = path_segment, offset = path_offset,
                        register = path_register},
        resource = {start = dword(state, 5), ['end'] = dword(state, 1), size = dword(state, 9)},
        entry_registers = r,
        return_registers = returned.registers,
        stack_hex = hex(stack),
    }
end

for sequence = 1, count do
    local entry = first_entry or wait_hit("resource lookup")
    first_entry = nil
    dosbox.output.events[sequence] = capture_event(entry, sequence)

    if sequence < count then
        dosbox.breakpoint_set(0x0207, 0x18c7, {once = true})
        dosbox.debug_continue()
    end
end

-- Some world BOBs are requested lazily when the first matching ARE object is
-- initialized. Continue from the final post-return stop and collect those
-- optional lookups without making an absent lazy request an error.
for tail = 1, tail_count do
    dosbox.breakpoint_set(0x0207, 0x18c7, {once = true})
    dosbox.debug_continue()
    local entry, err = dosbox.wait_for_breakpoint(timeout_ms)
    if not entry then
        dosbox.output.tail_timeout = err or "timeout"
        break
    end
    dosbox.output.events[count + tail] = capture_event(entry, count + tail)
end
