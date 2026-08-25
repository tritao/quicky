-- Capture the palette upload helpers during a controlled level launch.
-- The Python wrapper supplies TRACE_LEVEL and replays startup-to-input.json.
local timeout_ms = TRACE_TIMEOUT_MS or 30000
local level = TRACE_LEVEL or "W4L1"
local sample_count = TRACE_SAMPLE_COUNT or 8
local startup_sample_count = TRACE_STARTUP_SAMPLES or 0
local callsite_mode = TRACE_PALETTE_CALLSITE or false
local post_input_key = TRACE_POST_INPUT_KEY or ""
local post_input_frames = TRACE_POST_INPUT_FRAMES or 0
local post_sample_count = TRACE_POST_SAMPLES or 8

local selector_indices = {
    W1L1 = 0, W1L2 = 1, W1L3 = 2,
    W2L1 = 3, W2L2 = 4, W2L3 = 5,
    W3L1 = 6, W3L2 = 7, W3L3 = 8,
    W4L1 = 9, W4L2 = 10, W4L3 = 11,
    W5L1 = 12, W5L2 = 13, W5L3 = 14,
    W1L4 = 15, W2L4 = 16, W3L4 = 17,
    W4L4 = 18, W5L4 = 19,
}

local function wait_hit(label, timeout)
    local hit, err = dosbox.wait_for_breakpoint(timeout or timeout_ms)
    if not hit then error(label .. ": " .. (err or "timeout")) end
    return hit
end

local function arm_palette_breakpoints()
    dosbox.breakpoint_remove(0x0207, 0x0536)
    dosbox.breakpoint_remove(0x0207, 0x05ff)
    dosbox.breakpoint_remove(0x0207, 0x0924)
    dosbox.breakpoint_set(0x0207, 0x0536, {once = true})
    dosbox.breakpoint_set(0x0207, callsite_mode and 0x0924 or 0x05ff,
                          {once = true})
end

local function disarm_palette_breakpoints()
    for _, offset in ipairs({0x0536, 0x0539, 0x05ff, 0x0602,
                             0x0924, 0x0929}) do
        dosbox.breakpoint_remove(0x0207, offset)
    end
end

local function palette_event(hit)
    local registers = hit.registers or dosbox.cpu_state()
    local si = (registers.si or registers.esi or 0) & 0xffff
    local bp = (registers.bp or registers.ebp or 0) & 0xffff
    local event = {
        offset = hit.offset,
        segment = hit.segment,
        frame = dosbox.frame(),
        registers = registers,
    }
    if hit.offset == 0x0536 then
        event.kind = "full-upload"
        event.dac_index = 0
        event.palette_pointer = {
            selector = registers.ds,
            offset = si,
        }
    elseif hit.offset == 0x05ff then
        event.kind = "single-entry"
        -- 0207:05ff has already established a BP frame.  The four far-call
        -- arguments are words at BP+6..BP+C; keep both the decoded values and
        -- a numeric byte dump so JSON never contains binary replacement chars.
        local raw = dosbox.mem_read("ss", bp, 0x10) or ""
        local bytes = {}
        for index = 1, #raw do
            bytes[#bytes + 1] = string.byte(raw, index)
        end
        local function stack_word(delta)
            return dosbox.mem_read_word("ss", (bp + delta) & 0xffff)
        end
        local index_word = stack_word(0x0c)
        local red_word = stack_word(0x0a)
        local green_word = stack_word(0x08)
        local blue_word = stack_word(0x06)
        local return_offset = stack_word(0x02)
        local return_segment = stack_word(0x04)
        local caller_bp = stack_word(0x00)
        event.stack_bytes = bytes
        event.return_address = {
            segment = return_segment,
            offset = return_offset,
        }
        event.caller_frame = {
            bp = caller_bp,
            local_minus_2 = caller_bp and dosbox.mem_read_word(
                "ss", (caller_bp - 2) & 0xffff) or nil,
        }
        event.call_args = {
            index_word = index_word,
            red_word = red_word,
            green_word = green_word,
            blue_word = blue_word,
            dac_index = (index_word or 0) & 0xff,
            rgb = {
                (red_word or 0) & 0xff,
                (green_word or 0) & 0xff,
                (blue_word or 0) & 0xff,
            },
        }
    elseif hit.offset == 0x0924 then
        event.kind = "single-entry-callsite"
        local caller_bp = (registers.ebp or 0) & 0xffff
        local index_word = dosbox.mem_read_word(
            "ss", (caller_bp - 2) & 0xffff)
        event.caller_frame = {
            bp = caller_bp,
            local_minus_2 = index_word,
        }
        event.call_args = {
            index_word = index_word,
            dac_index = (index_word or 0) & 0xff,
            rgb = {0, 0, 0},
        }
    end
    return event
end

-- A breakpoint leaves DOSBox stopped at the instruction itself, and
-- DEBUG_ApiContinue deliberately keeps that address disabled for the first
-- resumed instruction.  Stop at the next instruction, then re-arm the target
-- while the guest is stopped there; this lets a tight palette loop reach its
-- next call instead of repeatedly reporting the same instruction.
local function advance_after_palette_event(hit)
    local next_offset = nil
    if hit.offset == 0x0536 then
        next_offset = 0x0539 -- after MOV DX,3C8
    elseif hit.offset == 0x05ff then
        next_offset = 0x0602 -- after MOV DX,3C8
    elseif hit.offset == 0x0924 then
        next_offset = 0x0929 -- after the far call
    end
    if not next_offset then
        dosbox.debug_continue()
        return false
    end
    dosbox.breakpoint_set(0x0207, next_offset, {once = true})
    dosbox.debug_continue()
    local stepped, err = dosbox.wait_for_breakpoint(timeout_ms)
    if not stepped then
        error("palette helper step: " .. (err or "timeout"))
    end
    dosbox.breakpoint_remove(0x0207, next_offset)
    return true -- stopped at the next instruction
end

dosbox.output.awaiting_startup_replay = true

-- The title/menu replay can perform a full 256-entry upload before the level
-- selector is reached.  Capture a small, optional prefix while the guest is
-- still running; after a hit DOSBox is stopped, so the loop explicitly
-- resumes it before waiting for the next one.
local startup_events = {}
local startup_stopped = false
for index = 1, startup_sample_count do
    arm_palette_breakpoints()
    if startup_stopped then
        dosbox.debug_continue()
        startup_stopped = false
    end
    local hit, err = dosbox.wait_for_breakpoint(timeout_ms)
    if not hit then
        startup_events[#startup_events + 1] = {
            kind = "timeout",
            error = err or "timeout",
            frame = dosbox.frame(),
        }
        break
    end
    startup_events[#startup_events + 1] = palette_event(hit)
    startup_stopped = advance_after_palette_event(hit)
end
if startup_stopped then dosbox.debug_continue() end
disarm_palette_breakpoints()
dosbox.output.startup_palette_events = startup_events
dosbox.wait_frames(350)

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
dosbox.wait_frames(60)
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


-- Palette activity occurs during the level launch. Arm while stopped at the
-- selector dispatch so the first DAC update cannot be missed.
arm_palette_breakpoints()
local events = {}
local guest_stopped = true -- selector dispatch breakpoint is still active
for index = 1, sample_count do
    arm_palette_breakpoints()
    if guest_stopped and index > 1 then
        dosbox.debug_continue()
        guest_stopped = false
    end
    if guest_stopped then
        dosbox.debug_continue()
        guest_stopped = false
    end
    local hit, err = dosbox.wait_for_breakpoint(timeout_ms)
    if not hit then
        events[#events + 1] = {kind = "timeout", error = err or "timeout"}
        break
    end
    events[#events + 1] = palette_event(hit)
    guest_stopped = advance_after_palette_event(hit)
end
if guest_stopped then dosbox.debug_continue() end
disarm_palette_breakpoints()

dosbox.output.palette_events = events

-- Optional authentic gameplay phase.  Holding Right long enough reaches the
-- W1L1 hazard/reset path while the same DAC helpers remain armed; this probe
-- does not write player memory.
local post_events = {}
if post_input_key ~= "" and post_input_frames > 0 then
    dosbox.wait_frames(60)
    dosbox.key(post_input_key, true)
    local post_stopped = false
    local post_deadline = dosbox.frame() + post_input_frames
    for index = 1, post_sample_count do
        if dosbox.frame() >= post_deadline then break end
        arm_palette_breakpoints()
        if post_stopped then
            dosbox.debug_continue()
            post_stopped = false
        end
        local remaining = post_deadline - dosbox.frame()
        local wait_ms = math.max(1000, math.min(
            timeout_ms, math.floor((remaining * 1000) / 70)))
        local hit, err = dosbox.wait_for_breakpoint(wait_ms)
        if not hit then
            post_events[#post_events + 1] = {
                kind = "timeout", error = err or "timeout",
                frame = dosbox.frame(),
            }
            break
        end
        post_events[#post_events + 1] = palette_event(hit)
        post_stopped = advance_after_palette_event(hit)
    end
    dosbox.key(post_input_key, false)
    if post_stopped then dosbox.debug_continue() end
end
dosbox.output.post_palette_events = post_events
