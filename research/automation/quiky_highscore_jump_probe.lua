-- Controlled debugger entry into the one-time 01D7:50B1 finalizer.
--
-- The natural caller is reached only after a completed-session branch that
-- the bounded gameplay routes do not yet reproduce. The host therefore
-- stops at a normal post-start barrier, writes CS:EIP to the real 50B1 entry,
-- and lets the original 50B1 -> 1084 -> 0703 -> SCORE.DAT path execute.

local config = TRACE_CONFIG or {}
local timeout_ms = config.timeout_ms or 15000
local post_timeout_ms = config.post_timeout_ms or math.min(timeout_ms, 10000)
local seed_score = config.seed_score or 0x7ffff000
local seed_aux = config.seed_aux or 13
local seed_progression = config.seed_progression or 14
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

local function put_word(offset, value)
    value = value & 0xffff
    dosbox.mem_write("ds", offset,
        string.char(value & 0xff, (value >> 8) & 0xff))
end

local function put_dword(offset, value)
    value = value & 0xffffffff
    dosbox.mem_write("ds", offset,
        string.char(value & 0xff, (value >> 8) & 0xff,
                    (value >> 16) & 0xff, (value >> 24) & 0xff))
end

local function state_snapshot()
    return {
        registers = dosbox.cpu_state(),
        score = dword(dosbox.mem_read("ds", 0x881c, 4), 1),
        score_aux = word(dosbox.mem_read("ds", 0x880e, 2), 1),
        lives = word(dosbox.mem_read("ds", 0x880a, 2), 1),
        progression = word(dosbox.mem_read("ds", 0x85d4, 2), 1),
        preserved_gate = word(dosbox.mem_read("ds", 0x88af, 2), 1),
        menu_gate = word(dosbox.mem_read("ds", 0x89f2, 2), 1),
        session_words = {
            word(dosbox.mem_read("ds", 0x88b0, 2), 1),
            word(dosbox.mem_read("ds", 0x88b2, 2), 1),
            word(dosbox.mem_read("ds", 0x88b4, 2), 1),
            word(dosbox.mem_read("ds", 0x88b6, 2), 1),
            word(dosbox.mem_read("ds", 0x88b8, 2), 1),
        },
    }
end

local function record(label, hit)
    events[#events + 1] = {
        label = label,
        breakpoint = hit and {segment = hit.segment, offset = hit.offset} or nil,
        registers = hit and hit.registers or dosbox.cpu_state(),
        game_state = state_snapshot(),
    }
end

local function wait_hit(label)
    local hit, err = dosbox.wait_for_breakpoint(timeout_ms)
    if not hit then error(label .. ": " .. (err or "timeout")) end
    return hit
end

local function wait_post_hit(label)
    local hit, err = dosbox.wait_for_breakpoint(post_timeout_ms)
    if not hit then error(label .. ": " .. (err or "timeout")) end
    return hit
end

-- Let the host replay the deterministic opening skips. Once this wait ends,
-- a current-CS/EIP breakpoint supplies a safe stopped point for the jump.
dosbox.output.awaiting_startup_replay = true
dosbox.wait_frames(350)
dosbox.output.awaiting_startup_replay = nil
local current = dosbox.cpu_state()
dosbox.breakpoint_set(current.cs, current.eip, {once = true})
dosbox.debug_continue()
local ready = wait_hit("post-start jump barrier")
record("post_start_barrier", ready)

dosbox.breakpoint_clear()
dosbox.breakpoint_set(0x01d7, 0x50b1, {once = true})
-- Install a temporary protected-mode far CALL at the stopped instruction.
-- This uses only the debugger's selector memory write and preserves/restores
-- the original bytes after the target breakpoint fires.
local jump_cs = ready.registers.cs
local jump_eip = ready.registers.eip & 0xffff
local original_jump_bytes = dosbox.mem_read_selector(jump_cs, jump_eip, 5) or ""
dosbox.breakpoint_set(0x01d7, 0x50b1, {once = true})
dosbox.mem_write_selector(jump_cs, jump_eip,
    string.char(0x9a, 0xb1, 0x50, 0xd7, 0x01))
dosbox.debug_continue()
local entry = wait_hit("forced preserved-score finalization entry")
if #original_jump_bytes == 5 then
    dosbox.mem_write_selector(jump_cs, jump_eip, original_jump_bytes)
end
record("preserved_score_finalization_entry", entry)
events[#events].code_bytes = hex(dosbox.mem_read_selector(0x01d7, 0x50b1, 256))

-- The runtime helper calls before 50C6 can normalize the gate, so stop at the
-- actual branch instruction and seed the preserved-session fields there.
dosbox.breakpoint_set(0x01d7, 0x50c6, {once = true})
dosbox.debug_continue()
local gate_branch = wait_hit("preserved-score gate branch")
record("preserved_score_gate_branch", gate_branch)

-- Preserve the original branch and make the insertion threshold deterministic.
put_word(0x88af, 1)
put_word(0x89f2, 0)
for _, offset in ipairs({0x88b0, 0x88b2, 0x88b4, 0x88b6, 0x88b8}) do
    put_word(offset, 0)
end
put_dword(0x881c, seed_score)
put_word(0x880e, seed_aux)
put_word(0x85d4, seed_progression)

local armed_targets = {}
local post_insert_targets = {
    0x08c7, 0x08cc, 0x08d1, 0x08d6, 0x08e0, 0x08e5,
    0x10bd, 0x10d0, 0x1135, 0x347a,
}
for _, target in ipairs({0x518e, 0x5190, 0x51a7, 0x1084, 0x0703, 0x10a1, 0x10ba, 0x10bd, 0x347a}) do
    armed_targets[string.format("%04x", target)] =
        dosbox.breakpoint_set(0x01d7, target, {once = true})
end
for _, target in ipairs(post_insert_targets) do
    armed_targets[string.format("post_%04x", target)] =
        dosbox.breakpoint_set(0x01d7, target, {once = true})
end
armed_targets["01e7:39f0"] = dosbox.breakpoint_set(0x01e7, 0x39f0,
    {once = true, deferred = true})
for _, segment in ipairs({0x01d7, 0x01e7, 0x01f7, 0x0207, 0x0227, 0x0237}) do
    armed_targets[string.format("%04x:356f", segment)] =
        dosbox.breakpoint_set(segment, 0x356f,
            {once = true, deferred = true})
    armed_targets[string.format("%04x:39f0", segment)] =
        dosbox.breakpoint_set(segment, 0x39f0,
            {once = true, deferred = true})
end
dosbox.output.high_score_armed_targets = armed_targets

dosbox.debug_continue()
local hit = wait_hit("high-score dispatcher or insertion")
events[#events + 1] = {
    label = "high_score_armed_targets",
    armed = armed_targets,
    code_bytes = {
        ["1084"] = hex(dosbox.mem_read_selector(0x01d7, 0x1084, 32)),
        ["0703"] = hex(dosbox.mem_read_selector(0x01d7, 0x0703, 32)),
    },
    state = state_snapshot(),
}
while hit.offset == 0x518e or hit.offset == 0x5190 or hit.offset == 0x51a7 or
      hit.offset == 0x347a or (hit.segment == 0x01e7 and hit.offset == 0x39f0) do
    record("preserved_score_internal_" .. string.format("%04x", hit.offset), hit)
    dosbox.debug_continue()
    hit = wait_hit("high-score dispatcher after finalizer internal")
end
if hit.offset == 0x1084 then
    record("high_score_dispatch_entry", hit)
    dosbox.debug_continue()
    hit = wait_hit("high-score insertion entry")
    while hit.offset == 0x10a1 or hit.offset == 0x10ba do
        record("high_score_dispatch_internal_" .. string.format("%04x", hit.offset), hit)
        dosbox.debug_continue()
        hit = wait_hit("high-score insertion after dispatch branch")
    end
end
if hit.offset == 0x0703 then
    record("high_score_insert_entry", hit)
    -- Let the insertion screen finish rendering before the host injects the
    -- name/confirm keystrokes.
    dosbox.wait_frames(60)
    -- 0703 resumes with a short sequence of local calls that copies the
    -- accepted name into the record and hands the table to the file layer.
    -- Keep these return sites armed so a writer address that is reached via
    -- an indirect/far call can be identified without guessing its segment.
    -- Wait on those continuation sites while the host supplies the name.  A
    -- prior version waited a fixed number of frames here, which let 0703
    -- return before its post-insert breakpoints were armed.
    dosbox.output.awaiting_name_input = true
    dosbox.debug_continue()
    local writer = wait_post_hit("score writer or post-insert continuation")
    dosbox.output.awaiting_name_input = nil
    local post_steps = 0
    while writer and post_steps < 20 do
        post_steps = post_steps + 1
        if writer.offset == 0x356f then
            record("score_file_write_body", writer)
            dosbox.breakpoint_set(writer.segment, 0x39f0, {once = true})
            dosbox.debug_continue()
            local cleanup = wait_post_hit("score writer cleanup")
            record("score_writer_followup", cleanup)
            break
        elseif writer.offset == 0x39f0 then
            record("score_file_timer_vector_restore", writer)
            break
        else
            record("post_insert_target_" .. string.format("%04x", writer.offset), writer)
            if writer.offset == 0x10d0 or writer.offset == 0x1135 then
                -- The finalization menu may continue after the record write;
                -- no further persistence breakpoint is needed in that path.
                break
            end
            dosbox.debug_continue()
            writer = wait_post_hit("score writer or post-insert continuation")
        end
    end
else
    record("high_score_insertion_not_reached", hit)
end

dosbox.output.high_score_jump_trace = {
    trace_schema_version = 1,
    events = events,
    final_capture_registers = dosbox.cpu_state(),
    final_game_state = state_snapshot(),
}
