-- Controlled entry into the original SCORE.DAT writer.
--
-- The natural high-score caller reaches 01E7:356F only after the name/menu
-- branch.  This probe uses the debugger to issue a protected-mode far CALL
-- from a stopped post-start barrier, then records the real writer entry and
-- vector-cleanup return.  It does not reimplement the codec.

local config = TRACE_CONFIG or {}
local timeout_ms = config.timeout_ms or 15000
local events = {}

local function word(raw, index)
    local lo, hi = string.byte(raw, index, index + 1)
    return lo | (hi << 8)
end

local function dword(raw, index)
    return word(raw, index) | (word(raw, index + 2) << 16)
end

local function hex(raw)
    return (raw or ""):gsub(".", function(c)
        return string.format("%02x", string.byte(c))
    end)
end

local function state(label, hit)
    local table_bytes = dosbox.mem_read("ds", 0x88ca, 0x82) or ""
    return {
        label = label,
        breakpoint = hit and {segment = hit.segment, offset = hit.offset} or nil,
        registers = hit and hit.registers or dosbox.cpu_state(),
        score = dword(dosbox.mem_read("ds", 0x881c, 4), 1),
        progression = word(dosbox.mem_read("ds", 0x85d4, 2), 1),
        table_hex = hex(table_bytes),
    }
end

local function wait_hit(label)
    local hit, err = dosbox.wait_for_breakpoint(timeout_ms)
    if not hit then error(label .. ": " .. (err or "timeout")) end
    return hit
end

dosbox.output.awaiting_startup_replay = true
dosbox.wait_frames(350)
dosbox.output.awaiting_startup_replay = nil

local current = dosbox.cpu_state()
dosbox.breakpoint_set(current.cs, current.eip, {once = true})
dosbox.debug_continue()
local ready = wait_hit("post-start writer barrier")
events[#events + 1] = state("post_start_barrier", ready)

local callsite_bytes = dosbox.mem_read_selector(ready.registers.cs,
                                                 ready.registers.eip, 5) or ""
local targets = {}
for _, segment in ipairs({0x01e7, 0x01d7, 0x01f7, 0x0207, 0x0227, 0x0237}) do
    targets[string.format("%04x:356f", segment)] =
        dosbox.breakpoint_set(segment, 0x356f,
            {once = true, deferred = true})
    targets[string.format("%04x:39f0", segment)] =
        dosbox.breakpoint_set(segment, 0x39f0,
            {once = true, deferred = true})
end
events[#events + 1] = {
    label = "writer_targets_armed",
    targets = targets,
    callsite = {segment = ready.registers.cs, offset = ready.registers.eip},
    callsite_bytes = hex(callsite_bytes),
}

-- 9A ptr16:16 is a protected-mode far CALL.  The writer body is at
-- 01E7:356F in the extracted executable.
dosbox.mem_write_selector(ready.registers.cs, ready.registers.eip,
    string.char(0x9a, 0x6f, 0x35, 0xe7, 0x01))
dosbox.debug_continue()
local writer = wait_hit("SCORE.DAT writer entry")
if #callsite_bytes == 5 then
    dosbox.mem_write_selector(ready.registers.cs, ready.registers.eip,
        callsite_bytes)
end
events[#events + 1] = state("score_file_write_body", writer)
dosbox.output.score_writer_trace = {
    trace_schema_version = 1,
    events = events,
    final_game_state = state("final", nil),
}
