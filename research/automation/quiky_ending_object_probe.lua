-- Exercise the real late-scene callback with one controlled pooled object.
-- The ordinary exits never instantiate 01F7:487F in the bounded routes. This
-- probe reuses one existing pooled object/scheduler entry and replaces only
-- its callback with 489C; at callback entry it enters the handler's terminal
-- state so 4968/4973 can be observed using the original scoring code.

local config = TRACE_CONFIG or {}
local timeout_ms = config.timeout_ms or 15000
local select_level = config.select_level or "W5L4"
local route = config.route or 1
local seed_score = config.seed_score or 0
local events = {}

local function word(raw, index)
    local lo, hi = string.byte(raw, index, index + 1)
    return lo | (hi << 8)
end

local function dword(raw, index)
    return word(raw, index) | (word(raw, index + 2) << 16)
end

local function put_word(selector, offset, value)
    value = value & 0xffff
    dosbox.mem_write_selector(selector, offset,
        string.char(value & 0xff, (value >> 8) & 0xff))
end

local function put_dword(selector, offset, value)
    value = value & 0xffffffff
    dosbox.mem_write_selector(selector, offset,
        string.char(value & 0xff, (value >> 8) & 0xff,
                    (value >> 16) & 0xff, (value >> 24) & 0xff))
end

local function read_word_ds(offset)
    local raw = dosbox.mem_read("ds", offset, 2) or ""
    return #raw >= 2 and word(raw, 1) or nil
end

local function read_dword_ds(offset)
    local raw = dosbox.mem_read("ds", offset, 4) or ""
    return #raw >= 4 and dword(raw, 1) or nil
end

local function snapshot(label, hit)
    local pointer = read_dword_ds(0x755e) or 0
    local cpu = dosbox.cpu_state()
    return {
        label = label,
        breakpoint = hit and {segment = hit.segment, offset = hit.offset} or nil,
        registers = hit and hit.registers or cpu,
        state = {
            data_selector = cpu.ds,
            score = read_dword_ds(0x881c),
            progression = read_word_ds(0x85d4),
            route = read_word_ds(0x85d8),
            ending_stage = read_word_ds(0x88ae),
            transition_signal = read_word_ds(0x89e6),
            lives = read_word_ds(0x880a),
            health = read_word_ds(0x8822),
            pool_selector = (pointer >> 16) & 0xffff,
            pool_offset = pointer & 0xffff,
        },
    }
end

local function wait_hit(label)
    local hit, err = dosbox.wait_for_breakpoint(timeout_ms)
    if not hit then error(label .. ": " .. (err or "timeout")) end
    return hit
end

local selector_indices = {
    W1L1 = 0, W1L2 = 1, W1L3 = 2,
    W2L1 = 3, W2L2 = 4, W2L3 = 5,
    W3L1 = 6, W3L2 = 7, W3L3 = 8,
    W4L1 = 9, W4L2 = 10, W4L3 = 11,
    W5L1 = 12, W5L2 = 13, W5L3 = 14,
    W1L4 = 15, W2L4 = 16, W3L4 = 17,
    W4L4 = 18, W5L4 = 19,
}

local function launch_selected_level()
    local index = selector_indices[select_level]
    assert(index ~= nil, "unsupported selector level")
    dosbox.key("KBD_space", true)
    dosbox.wait_frames(4)
    dosbox.key("KBD_space", false)
    dosbox.wait_frames(30)
    dosbox.type("QUIKYSUPERHERO")
    dosbox.wait_frames(3)
    dosbox.breakpoint_set(0x01d7, 0x491d, {once = true})
    dosbox.key("KBD_4", true)
    local cheat = wait_hit("level selector cheat branch")
    dosbox.key("KBD_4", false)
    events[#events + 1] = snapshot("selector_cheat_branch", cheat)
    dosbox.mem_write("ds", 0x89f2, "\x01")
    dosbox.mem_write("ds", 0x88ba, "\x05\x00")
    dosbox.debug_continue()
    dosbox.wait_frames(60)
    dosbox.breakpoint_set(0x01d7, 0x4ace, {once = true})
    local input_wait = wait_hit("level selector input wait")
    events[#events + 1] = snapshot("selector_input_wait", input_wait)
    dosbox.mem_write("ds", 0x85d4,
        string.char(index & 0xff, (index >> 8) & 0xff))
    dosbox.breakpoint_set(0x01d7, 0x4b18, {once = true})
    dosbox.mem_write("ds", 0x88bc, "\x20\x00")
    dosbox.debug_continue()
    local launch = wait_hit("level selector launch")
    events[#events + 1] = snapshot("selector_launch", launch)
end

local function find_target_object(pool_selector, pool_offset, stride, player_offset)
    for index = 0, 63 do
        local offset = pool_offset + index * stride
        if offset ~= player_offset then
            local raw = dosbox.mem_read_selector(pool_selector, offset, 0x40) or ""
            if #raw >= 0x40 and word(raw, 0x18 + 1) ~= 0 then
                return index, offset, raw
            end
        end
    end
    return nil
end

local function patch_scheduler_entry(object_offset)
    local raw = dosbox.mem_read("ds", 0x7566, 0x200) or ""
    for index = 0, 63 do
        local base = index * 8 + 1
        if base + 7 > #raw then break end
        local callback_offset = word(raw, base)
        local callback_segment = word(raw, base + 2)
        local entry_object_offset = word(raw, base + 4)
        if callback_offset == 0xffff and callback_segment == 0xffff then break end
        if entry_object_offset == object_offset then
            put_word(0x0237, 0x7566 + index * 8, 0x489c)
            return index, callback_segment
        end
    end
    return nil
end

local function patch_object(pool_selector, object_offset, player_offset)
    local raw = dosbox.mem_read_selector(pool_selector, object_offset, 0x40) or ""
    local player = dosbox.mem_read_selector(pool_selector, player_offset, 0x10) or ""
    if #raw < 0x40 or #player < 0x0a then error("short pooled object state") end
    put_word(pool_selector, object_offset + 0x18, 0x489c)
    put_dword(pool_selector, object_offset + 0x0e, 0x11000)
    put_dword(pool_selector, object_offset + 0x02, dword(player, 3))
    put_dword(pool_selector, object_offset + 0x06, dword(player, 7))
    local scheduler_index, scheduler_segment = patch_scheduler_entry(object_offset)
    return {
        object_offset = object_offset,
        scheduler_index = scheduler_index,
        scheduler_segment = scheduler_segment,
        before_hex = raw:gsub(".", function(c)
            return string.format("%02x", string.byte(c))
        end),
    }
end

dosbox.output.awaiting_startup_replay = true
dosbox.wait_frames(350)
dosbox.output.awaiting_startup_replay = nil
launch_selected_level()

dosbox.breakpoint_clear()
dosbox.breakpoint_set(0x01f7, 0x0e96, {once = true})
dosbox.debug_continue()
local ready = wait_hit("post-level object-update barrier")
events[#events + 1] = snapshot("post_level_barrier", ready)

local pointer = read_dword_ds(0x755e) or 0
local pool_selector = (pointer >> 16) & 0xffff
local pool_offset = pointer & 0xffff
local stride = read_word_ds(0x30ce) or 0
local player_offset = read_word_ds(0x881a) or 0
if pool_selector == 0 or stride == 0 then error("invalid object pool pointer") end
local target_index, target_offset = find_target_object(
    pool_selector, pool_offset, stride, player_offset)
if not target_offset then error("no reusable pooled object") end
local patch = patch_object(pool_selector, target_offset, player_offset)
patch.target_index = target_index
events[#events + 1] = {
    label = "controlled_late_object_patch",
    object = patch,
    state = snapshot("controlled_late_object_patch").state,
}

dosbox.breakpoint_set(0x01f7, 0x489c, {once = true})
dosbox.breakpoint_set(0x01f7, 0x4968, {once = true})
dosbox.breakpoint_set(0x01f7, 0x4973, {once = true})
dosbox.mem_write("ds", 0x85d8,
    string.char(route & 0xff, (route >> 8) & 0xff))
dosbox.mem_write("ds", 0x881c,
    string.char(seed_score & 0xff, (seed_score >> 8) & 0xff,
                (seed_score >> 16) & 0xff, (seed_score >> 24) & 0xff))
dosbox.debug_continue()
local hit = wait_hit("late object callback/tally")
if hit.offset == 0x489c then
    events[#events + 1] = snapshot("alternate_completion_callback_entry", hit)
    local object_selector = hit.registers and hit.registers.es or pool_selector
    local object_offset = hit.registers and ((hit.registers.edi or 0) & 0xffff) or target_offset
    local player = dosbox.mem_read_selector(pool_selector, player_offset, 0x10) or ""
    if #player >= 0x0a then
        put_dword(object_selector, object_offset + 0x02, dword(player, 3))
        put_dword(object_selector, object_offset + 0x06, dword(player, 7))
    end
    dosbox.mem_write_selector(object_selector, object_offset + 0x2a, "\x01")
    dosbox.mem_write_selector(object_selector, object_offset + 0x0e, "\x00\x00\x00\x00")
    dosbox.breakpoint_set(0x01f7, 0x4968, {once = true})
    dosbox.debug_continue()
    hit = wait_hit("late score tally entry")
    if hit.offset == 0x4968 then
        events[#events + 1] = snapshot("alternate_score_tally_entry", hit)
        dosbox.breakpoint_set(0x01f7, 0x4973, {once = true})
        dosbox.debug_continue()
        local score = wait_hit("late score write")
        events[#events + 1] = snapshot("alternate_score_write_entry", score)
        -- 4973 is the ADD instruction itself.  Stop at the following route
        -- comparison so the snapshot proves the dword was actually updated.
        dosbox.breakpoint_set(0x01f7, 0x497c, {once = true})
        dosbox.debug_continue()
        local after_score = wait_hit("late score write completion")
        events[#events + 1] = snapshot("alternate_score_applied", after_score)
        if route ~= 1 and route ~= 3 and route ~= 5 then
            -- Non-route continuations set the object terminal state and latch
            -- the ordinary level-transition signal in this tail block.
            dosbox.breakpoint_set(0x01f7, 0x4996, {once = true})
            dosbox.debug_continue()
            local transition = wait_hit("late route transition")
            events[#events + 1] = snapshot("alternate_transition_signal", transition)
            dosbox.breakpoint_set(0x01f7, 0x499c, {once = true})
            dosbox.debug_continue()
            local signal_after = wait_hit("late transition signal write")
            events[#events + 1] = snapshot("alternate_transition_signal_applied", signal_after)
        else
            -- Route 1/3/5 take the continuation-object branch at 499E.
            dosbox.breakpoint_set(0x01f7, 0x499e, {once = true})
            dosbox.debug_continue()
            local continuation = wait_hit("late route continuation")
            events[#events + 1] = snapshot("alternate_route_continuation", continuation)
        end
    else
        events[#events + 1] = snapshot("unexpected_after_callback", hit)
    end
else
    events[#events + 1] = snapshot("unexpected_late_object_entry", hit)
end

dosbox.output.ending_trace = {
    trace_schema_version = 1,
    events = events,
    final_state = snapshot("final_state").state,
    final_registers = dosbox.cpu_state(),
}
