-- Quiky resource lookup tracer. Loaded by research/tools/quikytrace.py.
local count = TRACE_COUNT or 2
local timeout_ms = TRACE_TIMEOUT_MS or 30000
local prepare_w1l3 = TRACE_PREPARE_W1L3 or false
local navigate_w1l3 = TRACE_NAVIGATE_W1L3 or false
local selector_frames = TRACE_SELECTOR_FRAMES or 60

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

if navigate_w1l3 then
    -- Replay the opening skips and launch the default level. Resource paths
    -- are redirected to W1L3 atomically at the lookup breakpoint below.
    dosbox.output.awaiting_startup_replay = true
    dosbox.wait_frames(350)
    local menu = dosbox.cpu_state()
    assert(menu.ds == 0x0237, "Quiky menu data segment is not initialized")
    dosbox.output.checkpoints = {
        menu = menu,
    }
    dosbox.key("KBD_space", true)
    dosbox.wait_frames(4)
    dosbox.key("KBD_space", false)
    dosbox.wait_frames(2)
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
dosbox.breakpoint_set(0x0207, 0x18c7, {once = true})
dosbox.output.events = {}
for sequence = 1, count do
    local entry = wait_hit("resource lookup")
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
    if navigate_w1l3 then
        local level_at = string.find(path, "W1L1", 1, true)
        if level_at then
            dosbox.mem_write(path_register, path_offset + level_at + 3, "3")
            path = string.gsub(path, "W1L1", "W1L3", 1)
        end
    end

    local state = dosbox.mem_read("ds", 0x97e4, 12)

    dosbox.output.events[sequence] = {
        sequence = sequence,
        path = path,
        original_path = original_path,
        entry = {segment = entry.segment, offset = entry.offset},
        return_location = {segment = return_segment, offset = return_offset},
        path_pointer = {segment = path_segment, offset = path_offset,
                        register = path_register},
        resource = {start = dword(state, 5), ['end'] = dword(state, 1), size = dword(state, 9)},
        entry_registers = r,
        stack_hex = hex(stack),
    }

    if sequence < count then
        dosbox.debug_continue()
        dosbox.wait_frames(1)
        dosbox.breakpoint_set(0x0207, 0x18c7, {once = true})
    end
end
