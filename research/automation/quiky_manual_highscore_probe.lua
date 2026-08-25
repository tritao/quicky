-- Catch the one-time preserved-score finalization from a DOS prompt.
-- This script is intentionally separate from the normal player trace: the
-- 01D7:5089 -> 50B1 path runs before an auto-exec trace can install ordinary
-- selector breakpoints.

local config = TRACE_CONFIG or {}
local timeout_ms = config.timeout_ms or 90000
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

local function read_word(offset)
    local raw = dosbox.mem_read("ds", offset, 2) or ""
    return #raw >= 2 and word(raw, 1) or nil
end

local function read_dword(offset)
    local raw = dosbox.mem_read("ds", offset, 4) or ""
    return #raw >= 4 and dword(raw, 1) or nil
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
        ds = dosbox.cpu_state().ds,
        score = read_dword(0x881c),
        score_aux = read_word(0x880e),
        lives = read_word(0x880a),
        progression = read_word(0x85d4),
        preserved_gate = read_word(0x88af),
        menu_gate = read_word(0x89f2),
        session_words = {
            read_word(0x88b0), read_word(0x88b2), read_word(0x88b4),
            read_word(0x88b6), read_word(0x88b8),
        },
    }
end

local function record(label, hit)
    events[#events + 1] = {
        label = label,
        breakpoint = {segment = hit.segment, offset = hit.offset},
        registers = hit.registers,
        game_state = state_snapshot(),
    }
end

local function wait_hit(label)
    local hit, err = dosbox.wait_for_breakpoint(timeout_ms)
    if not hit then error(label .. ": " .. (err or "timeout")) end
    return hit
end

-- These are deferred because no protected-mode selector exists while the
-- script is waiting at the DOS command prompt.
for _, target in ipairs({
    {0x01d7, 0x5089}, {0x01d7, 0x50b1}, {0x01d7, 0x1084},
    {0x01d7, 0x0703},
}) do
    dosbox.breakpoint_set(target[1], target[2], {once = true, deferred = true})
end

dosbox.output.awaiting_manual_launch = true
local hit = wait_hit("manual-launch preserved-score entry")
dosbox.output.awaiting_manual_launch = nil
if hit.offset == 0x5089 then
    record("program_entry", hit)
    hit = wait_hit("preserved-score finalization after program entry")
end
if hit.offset ~= 0x50b1 then
    record("unexpected_early_entry", hit)
else
    record("preserved_score_finalization_entry", hit)
    -- Controlled state setup at the real one-time entry.  This preserves the
    -- original 50B1 -> 1084 -> 0703 control flow while making insertion
    -- eligibility deterministic and documenting every forced word.
    put_word(0x88af, 1)
    put_word(0x89f2, 0)
    for _, offset in ipairs({0x88b0, 0x88b2, 0x88b4, 0x88b6, 0x88b8}) do
        put_word(offset, 0)
    end
    put_dword(0x881c, seed_score)
    put_word(0x880e, seed_aux)
    put_word(0x85d4, seed_progression)
    dosbox.breakpoint_set(0x01d7, 0x1084, {once = true})
    dosbox.breakpoint_set(0x01d7, 0x0703, {once = true})
    hit = wait_hit("high-score dispatcher/insertion entry")
    if hit.offset == 0x1084 then
        record("high_score_dispatch_entry", hit)
        hit = wait_hit("high-score insertion entry")
    end
    if hit.offset == 0x0703 then
        record("high_score_insert_entry", hit)
        for _, segment in ipairs({0x01e7, 0x0227}) do
            dosbox.breakpoint_set(segment, 0x356f, {once = true})
            dosbox.breakpoint_set(segment, 0x39f0, {once = true})
        end
        dosbox.output.awaiting_name_input = true
        -- Give the host a deterministic window to type the name.  The trace
        -- remains on the emulation thread and therefore preserves ordering.
        dosbox.wait_frames(180)
        dosbox.output.awaiting_name_input = nil
        dosbox.debug_continue()
        local writer_hit, writer_err = dosbox.wait_for_breakpoint(timeout_ms)
        if writer_hit then
            record(writer_hit.offset == 0x356f and
                       "score_file_write_body" or
                       writer_hit.offset == 0x39f0 and
                       "score_file_timer_vector_restore" or
                       "unexpected_writer_entry", writer_hit)
            hit = writer_hit
        else
            events[#events + 1] = {
                label = "writer_wait_timeout",
                error = writer_err or "timeout",
                game_state = state_snapshot(),
            }
        end
    else
        record("high_score_insertion_not_reached", hit)
    end
end

dosbox.output.high_score_trace = {
    trace_schema_version = 1,
    events = events,
    final_capture_registers = hit and hit.registers or dosbox.cpu_state(),
    final_game_state = state_snapshot(),
}
