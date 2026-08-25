-- Capture repeated object/BOB/ICO renderer entries after a controlled launch.
local level = TRACE_LEVEL or "W4L1"
local focus = TRACE_FOCUS or "bob"
local timeout_ms = TRACE_TIMEOUT_MS or 5000
local event_limit = TRACE_EVENT_LIMIT or 48
local walk_objects = TRACE_WALK_OBJECTS or false

local selector_indices = {
    W1L1 = 0, W1L2 = 1, W1L3 = 2,
    W2L1 = 3, W2L2 = 4, W2L3 = 5,
    W3L1 = 6, W3L2 = 7, W3L3 = 8,
    W4L1 = 9, W4L2 = 10, W4L3 = 11,
    W5L1 = 12, W5L2 = 13, W5L3 = 14,
    W1L4 = 15, W2L4 = 16, W3L4 = 17,
    W4L4 = 18, W5L4 = 19,
}

local targets = {
    [0x3529] = "bob-entry",
    [0x1186] = "ico-entry",
    [0x11b4] = "ico-tile-call",
}
if focus == "ico" then
    targets[0x3529] = nil
end
if walk_objects then
    -- The object-list loop at 3587 calls the common BOB routine at 0013;
    -- 3529 is a separate descriptor-side path and is not the list cursor.
    targets[0x3529] = nil
    targets[0x0016] = "bob-draw"
    targets[0x11b4] = nil
end

local function wait_hit(label)
    local hit, err = dosbox.wait_for_breakpoint(timeout_ms)
    if not hit then error(label .. ": " .. (err or "timeout")) end
    return hit
end

local function word(selector, offset)
    return dosbox.mem_read_word(selector, offset)
end

local function bytes(raw)
    local result = {}
    for index = 1, #(raw or "") do
        result[#result + 1] = string.byte(raw, index)
    end
    return result
end

local function object_snapshot(hit)
    local registers = hit.registers or dosbox.cpu_state()
    local selector = registers.es
    local offset = (registers.edi or registers.di or 0) & 0xffff
    local result = {
        selector = selector,
        offset = offset,
        slot = nil,
        x = nil,
        y = nil,
        raw = {},
    }
    if selector == nil then return result end
    local ok, raw_or_error = pcall(
        dosbox.mem_read_selector, selector, offset, 0x40
    )
    if not ok or not raw_or_error then
        result.read_error = tostring(raw_or_error)
        return result
    end
    local raw = raw_or_error
    result.raw = bytes(raw)
    if #raw >= 0x14 then result.slot = string.byte(raw, 0x13) | (string.byte(raw, 0x14) << 8) end
    if #raw >= 0x0c then
        result.x = (string.byte(raw, 0x05) | (string.byte(raw, 0x06) << 8))
        result.y = (string.byte(raw, 0x09) | (string.byte(raw, 0x0a) << 8))
    end
    return result
end

local function snapshot(hit)
    local registers = hit.registers or dosbox.cpu_state()
    local event = {
        label = targets[hit.offset] or string.format("0x%04x", hit.offset),
        segment = hit.segment,
        offset = hit.offset,
        frame = dosbox.frame(),
        registers = registers,
        camera = {
            x = word("ds", 0x81c0),
            y = word("ds", 0x81c4),
            page_x = word("ds", 0x81a8),
            page_y = word("ds", 0x81ac),
        },
    }
    if hit.offset == 0x3529 or hit.offset == 0x1186 then
        event.object = object_snapshot(hit)
    end
    if hit.offset == 0x0016 then
        local bp = (registers.bp or registers.ebp or 0) & 0xffff
        local function stack_word(delta)
            return dosbox.mem_read_word("ss", (bp + delta) & 0xffff)
        end
        event.bob_call = {
            bp = bp,
            return_address = {
                segment = stack_word(0x04),
                offset = stack_word(0x02),
            },
            slot = stack_word(0x08),
            y = stack_word(0x0a),
            x = stack_word(0x0c),
            mode = stack_word(0x06),
        }
    end
    if hit.offset == 0x1186 then
        event.animation_pointer = {
            selector = registers.fs,
            offset = (registers.ebx or 0) & 0xffff,
        }
        local raw = dosbox.mem_read("ds", 0x8182, 0x20) or ""
        event.ico_objects = bytes(raw)
    end
    return event
end

local function arm_targets()
    for offset, _ in pairs(targets) do
        dosbox.breakpoint_set(0x01f7, offset, {once = true})
    end
end

-- 3587 walks the live object array and invokes the common BOB draw entry at
-- 0016 (after its prologue) once per object.  A breakpoint at a current
-- renderer instruction alone
-- sees the same first object forever because it is re-armed in place.  When
-- requested, decode 0013's far return, stop there, then re-arm 0013; this lets
-- the list loop advance to the next object while preserving draw order.
local function advance_object_call(hit)
    local registers = hit.registers or dosbox.cpu_state()
    local bp = (registers.bp or registers.ebp or 0) & 0xffff
    local return_offset = dosbox.mem_read_word("ss", (bp + 0x02) & 0xffff)
    local return_segment = dosbox.mem_read_word("ss", (bp + 0x04) & 0xffff)
    -- A BOB call from the list returns to 35b8; the descriptor path returns
    -- to 3586.  Decode the live far return instead of assuming one caller.
    if return_segment ~= 0x01f7 then
        return nil, string.format("unexpected BOB return %04x:%04x",
                                  return_segment or 0, return_offset or 0)
    end
    dosbox.breakpoint_set(0x01f7, return_offset, {once = true})
    dosbox.debug_continue()
    local returned, err = dosbox.wait_for_breakpoint(timeout_ms)
    if not returned then
        return nil, err or "object renderer return timeout"
    end
    dosbox.breakpoint_remove(0x01f7, return_offset)
    if walk_objects then
        dosbox.breakpoint_set(0x01f7, 0x0016, {once = true})
    else
        dosbox.breakpoint_set(0x01f7, 0x3529, {once = true})
    end
    return returned, nil
end

local function advance_ico_call()
    -- 1186 emits four calls to 11b4 and returns at 11b0.  Stopping at the
    -- return instruction captures the complete ICO batch after its pixels
    -- have been written, then the next 0016/1186 breakpoint can advance the
    -- following render pass.
    dosbox.breakpoint_set(0x01f7, 0x11b0, {once = true})
    dosbox.debug_continue()
    local returned, err = dosbox.wait_for_breakpoint(timeout_ms)
    if not returned then
        return nil, err or "ICO renderer return timeout"
    end
    dosbox.breakpoint_remove(0x01f7, 0x11b0)
    if walk_objects then
        dosbox.breakpoint_set(0x01f7, 0x0016, {once = true})
    else
        dosbox.breakpoint_set(0x01f7, 0x1186, {once = true})
    end
    return returned, nil
end

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
dosbox.mem_write("ds", 0x89f2, "\x01")
dosbox.mem_write("ds", 0x88ba, "\x05\x00")
dosbox.debug_continue()
dosbox.wait_frames(60)
dosbox.breakpoint_set(0x01d7, 0x4ace, {once = true})
wait_hit("selector input wait")
local selector_index = selector_indices[level]
assert(selector_index ~= nil, "unsupported level selector target")
dosbox.mem_write("ds", 0x85d4,
                 string.char(selector_index & 0xff, selector_index >> 8))
dosbox.breakpoint_set(0x01d7, 0x4b18, {once = true})
dosbox.mem_write("ds", 0x88bc, "\x20\x00")
dosbox.debug_continue()
local launch = wait_hit("selector Space dispatch")
dosbox.output.checkpoints = {cheat = cheat, launch = launch}

arm_targets()
local events = {}
for index = 1, event_limit do
    dosbox.debug_continue()
    local hit, err = dosbox.wait_for_breakpoint(timeout_ms)
    if not hit then
        events[#events + 1] = {label = "timeout", frame = dosbox.frame(), error = err or "timeout"}
        break
    end
    events[#events + 1] = snapshot(hit)
    if walk_objects and hit.offset == 0x0016 then
        local returned, err = advance_object_call(hit)
        if not returned then
            events[#events + 1] = {
                label = "object-walk-timeout",
                frame = dosbox.frame(),
                error = err,
            }
            break
        end
    elseif walk_objects and hit.offset == 0x1186 then
        local returned, err = advance_ico_call()
        if not returned then
            events[#events + 1] = {
                label = "ico-walk-timeout",
                frame = dosbox.frame(),
                error = err,
            }
            break
        end
    else
        -- One-shot breakpoints are removed on hit. Re-arming the address
        -- while stopped captures the same renderer stage on successive guest
        -- frames; walk mode above is the object-list-aware alternative.
        dosbox.breakpoint_set(0x01f7, hit.offset, {once = true})
    end
end
dosbox.breakpoint_clear()
dosbox.output.renderer_census = events
