-- Capture a rendered frame at the completed BOB and ICO draw boundaries.
-- The host observes renderer_pixel_checkpoint, saves the stopped frame, then
-- POSTs /api/v1/debug/continue.  This keeps the screenshot synchronized with
-- the guest-side draw return rather than with a later timer tick.
local level = TRACE_LEVEL or "W4L1"
local timeout_ms = TRACE_TIMEOUT_MS or 10000
local event_limit = TRACE_EVENT_LIMIT or 16
local acknowledgement_delay_frames = TRACE_ACK_DELAY_FRAMES or 20
local focus = TRACE_FOCUS or "both"
local patch_map_run = TRACE_PATCH_MAP_RUN or false
local patch_map_x = TRACE_PATCH_MAP_X or 784
local patch_map_y = TRACE_PATCH_MAP_Y or 192
local patch_camera_x = TRACE_PATCH_CAMERA_X
local patch_camera_y = TRACE_PATCH_CAMERA_Y
local bob_draw_slot = TRACE_BOB_DRAW_SLOT
local bob_draw_x = TRACE_BOB_DRAW_X
local bob_draw_y = TRACE_BOB_DRAW_Y
local force_bob_mode = TRACE_FORCE_BOB_MODE
local force_bob_mode_slot = TRACE_FORCE_BOB_MODE_SLOT

local selector_indices = {
    W1L1 = 0, W1L2 = 1, W1L3 = 2,
    W2L1 = 3, W2L2 = 4, W2L3 = 5,
    W3L1 = 6, W3L2 = 7, W3L3 = 8,
    W4L1 = 9, W4L2 = 10, W4L3 = 11,
    W5L1 = 12, W5L2 = 13, W5L3 = 14,
    W1L4 = 15, W2L4 = 16, W3L4 = 17,
    W4L4 = 18, W5L4 = 19,
}

local function wait_hit(label)
    local hit, err = dosbox.wait_for_breakpoint(timeout_ms)
    if not hit then error(label .. ": " .. (err or "timeout")) end
    return hit
end

local function word(selector, offset)
    return dosbox.mem_read_word(selector, offset)
end

local function little_word(value)
    return string.char(value & 0xff, (value >> 8) & 0xff)
end

local function dword(raw, index)
    local b1 = string.byte(raw or "", index) or 0
    local b2 = string.byte(raw or "", index + 1) or 0
    local b3 = string.byte(raw or "", index + 2) or 0
    local b4 = string.byte(raw or "", index + 3) or 0
    return b1 | (b2 << 8) | (b3 << 16) | (b4 << 24)
end

local function raw_word(raw, index)
    local b1 = string.byte(raw or "", index) or 0
    local b2 = string.byte(raw or "", index + 1) or 0
    return b1 | (b2 << 8)
end

local function effect_run_base()
    local raw = dosbox.mem_read("ds", 0x6986, 0x400) or ""
    local run = 0
    for tile = 0, 0x1ff do
        if raw_word(raw, tile * 2 + 1) ~= 0 then
            run = run + 1
            if run == 5 then return tile - 4 end
        else
            run = 0
        end
    end
    return nil
end

local function patch_effect_map()
    if not patch_map_run then return nil end
    local pointer = dword(dosbox.mem_read("ds", 0x657a, 4), 1)
    local map_base = pointer & 0xffff
    local map_selector = (pointer >> 16) & 0xffff
    local row_stride = word("ds", 0x657e)
    local base_tile = effect_run_base()
    if map_selector == 0 or base_tile == nil or row_stride == 0 then
        return {map_selector = map_selector, row_stride = row_stride,
                base_tile = base_tile, error = "MAP/effect table not ready"}
    end
    local base_offset = map_base + (patch_map_y >> 4) * row_stride +
        (patch_map_x >> 4) * 2
    local cells = {}
    for index = 0, 4 do
        local offset = base_offset + index * 2
        local before = dosbox.mem_read_word(map_selector, offset)
        local after = base_tile + index
        dosbox.mem_write_selector(map_selector, offset, little_word(after))
        cells[#cells + 1] = {selector = map_selector, offset = offset,
                             before = before, after = after}
    end
    if patch_camera_x ~= nil then
        dosbox.mem_write("ds", 0x81c0, little_word(patch_camera_x))
    end
    if patch_camera_y ~= nil then
        dosbox.mem_write("ds", 0x81c4, little_word(patch_camera_y))
    end
    return {selector = map_selector, base_offset = base_offset,
            row_stride = row_stride, x = patch_map_x, y = patch_map_y,
            base_tile = base_tile, cells = cells,
            camera_x = patch_camera_x, camera_y = patch_camera_y}
end

local function bytes(raw)
    local result = {}
    for index = 1, #(raw or "") do
        result[#result + 1] = string.byte(raw, index)
    end
    return result
end

local function hex(raw)
    local result = {}
    for index = 1, #(raw or "") do
        result[#result + 1] = string.format("%02x", string.byte(raw, index))
    end
    return table.concat(result)
end

local function bob_call_snapshot(hit)
    local registers = hit.registers or dosbox.cpu_state()
    local bp = (registers.bp or registers.ebp or 0) & 0xffff
    local function stack_word(delta)
        return dosbox.mem_read_word("ss", (bp + delta) & 0xffff)
    end
    local original_x = stack_word(0x0c)
    local original_y = stack_word(0x0a)
    local original_mode = stack_word(0x06)
    local slot = stack_word(0x08)
    if force_bob_mode ~= nil and
            (force_bob_mode_slot == nil or slot == force_bob_mode_slot) then
        dosbox.mem_write("ss", (bp + 0x06) & 0xffff,
                         little_word(force_bob_mode))
    end
    local override = nil
    if bob_draw_slot ~= nil and slot == bob_draw_slot and
            bob_draw_x ~= nil and bob_draw_y ~= nil then
        dosbox.mem_write("ss", (bp + 0x0c) & 0xffff,
                         little_word(bob_draw_x))
        dosbox.mem_write("ss", (bp + 0x0a) & 0xffff,
                         little_word(bob_draw_y))
        override = {slot = bob_draw_slot, x = bob_draw_x, y = bob_draw_y,
                    original_x = original_x, original_y = original_y}
    end
    local descriptor = nil
    local descriptor_index = dosbox.mem_read_word("ds", 0x6d8e + slot * 2)
    local table_raw = dosbox.mem_read("ds", 0x6d8a, 4) or ""
    if descriptor_index ~= nil and #table_raw >= 4 then
        local table_offset = raw_word(table_raw, 1)
        local table_selector = raw_word(table_raw, 3)
        local descriptor_offset = table_offset + descriptor_index * 0x2c
        local ok, raw = pcall(dosbox.mem_read_selector, table_selector,
                              descriptor_offset, 0x2c)
        if ok and raw and #raw >= 0x0c then
            descriptor = {
                index = descriptor_index,
                selector = table_selector,
                offset = descriptor_offset,
                width = raw_word(raw, 1),
                height = raw_word(raw, 3),
                origin_x = raw_word(raw, 9),
                origin_y = raw_word(raw, 11),
                raw_hex = hex(raw),
            }
        else
            descriptor = {index = descriptor_index, selector = table_selector,
                          offset = descriptor_offset,
                          read_error = ok and "short descriptor" or tostring(raw)}
        end
    end
    return {
        bp = bp,
        return_address = {
            segment = stack_word(0x04),
            offset = stack_word(0x02),
        },
        slot = slot,
        y = stack_word(0x0a),
        x = stack_word(0x0c),
        mode = stack_word(0x06),
        original_mode = original_mode,
        forced_mode = force_bob_mode,
        forced_mode_slot = force_bob_mode_slot,
        descriptor = descriptor,
        draw_override = override,
    }
end

local function ico_snapshot(hit)
    local registers = hit.registers or dosbox.cpu_state()
    local object_offset = (registers.edi or registers.di or 0) & 0xffff
    local object_selector = registers.es
    local object_raw = dosbox.mem_read_selector(object_selector, object_offset, 64) or ""
    return {
        animation_pointer = {
            selector = registers.fs,
            offset = (registers.ebx or registers.bx or 0) & 0xffff,
        },
        object = {
            selector = object_selector,
            offset = object_offset,
            state_hex = hex(object_raw),
            x = raw_word(object_raw, 0x04 + 1),
            y = raw_word(object_raw, 0x08 + 1),
            sprite_slot = raw_word(object_raw, 0x12 + 1),
            callback = raw_word(object_raw, 0x18 + 1),
            state = raw_word(object_raw, 0x2e + 1),
        },
        objects = bytes(dosbox.mem_read("ds", 0x8182, 0x20) or ""),
    }
end

local function common_event(stage, hit, extra)
    local registers = hit.registers or dosbox.cpu_state()
    local event = {
        stage = stage,
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
    for key, value in pairs(extra or {}) do event[key] = value end
    return event
end

local function vga_snapshot()
    local selector = word("ds", 0x6572)
    local page_offset = word("ds", 0x817c)
    local stride = word("ds", 0x5c9a)
    local x = word("ds", 0x8182)
    local y = word("ds", 0x8184)
    local plane_offset = word("ds", 0x81a0)
    local target_offset = page_offset + plane_offset + y * stride + (x >> 2)
    local ok, raw = pcall(dosbox.mem_read_selector, selector, target_offset, 0x100)
    if not ok or not raw then
        return {selector = selector, offset = target_offset, stride = stride,
                page_offset = page_offset, x = x, y = y,
                plane_offset = plane_offset,
                read_error = ok and "empty VGA sample" or tostring(raw)}
    end
    return {selector = selector, offset = target_offset, stride = stride,
            page_offset = page_offset, x = x, y = y,
            plane_offset = plane_offset,
            page_x = word("ds", 0x81a8), page_y = word("ds", 0x81ac),
            camera_x = word("ds", 0x81c0), camera_y = word("ds", 0x81c4),
            raw_hex = hex(raw)}
end

local events = {}
local sequence = 0
local map_patch = nil

local function checkpoint(event, barrier_segment, barrier_offset)
    sequence = sequence + 1
    event.sequence = sequence
    events[#events + 1] = event
    dosbox.output.renderer_pixel_checkpoints = events
    dosbox.output.renderer_pixel_checkpoint = event
    -- Hold the guest at the exact draw-return instruction until the host has
    -- captured the frame and acknowledged it.  This avoids a timer-based
    -- polling race and preserves the natural frame cadence between draws.
    local current = dosbox.cpu_state()
    -- The BOB return breakpoint is the first instruction after a far call;
    -- the ICO breakpoint is the RET itself.  Use the next instruction/outer
    -- caller address as the acknowledgement barrier so continue cannot fall
    -- straight back into the same stopped instruction.
    dosbox.breakpoint_set(barrier_segment or current.cs,
                          barrier_offset or ((current.eip + 1) & 0xffff),
                          {once = true})
    wait_hit("capture acknowledgement")
    -- Keep the acknowledged record visible for a few guest frames while the
    -- host's POST returns and the next status poll starts.  The saved frame
    -- was already taken at the exact draw boundary above.
    dosbox.wait_frames(acknowledgement_delay_frames)
    dosbox.output.renderer_pixel_checkpoint = nil
end

local function arm_bob()
    dosbox.breakpoint_set(0x01f7, 0x0016, {once = true})
end

local function arm_ico()
    dosbox.breakpoint_set(0x01f7, 0x1186, {once = true})
end

local function advance_bob(hit, call)
    local return_offset = call.return_address.offset
    local return_segment = call.return_address.segment
    assert(return_segment == 0x01f7,
           string.format("unexpected BOB return %04x:%04x",
                         return_segment or 0, return_offset or 0))
    dosbox.breakpoint_set(0x01f7, return_offset, {once = true})
    dosbox.debug_continue()
    local returned, err = dosbox.wait_for_breakpoint(timeout_ms)
    if not returned then error("BOB draw return: " .. (err or "timeout")) end
    dosbox.breakpoint_remove(0x01f7, return_offset)
    return returned, returned.segment, (returned.offset + 1) & 0xffff
end

local function advance_ico(ico)
    -- 1186 performs four 11b4 tile calls.  Break at each near-call return so
    -- every helper's pixels gets its own synchronized frame.  The barrier is
    -- the next call site, which prevents the host acknowledgement from
    -- running the following tile before the screenshot is taken.
    local helper_returns = {0x1192, 0x119b, 0x11a7, 0x11b0}
    local next_calls = {0x1198, 0x11a4, 0x11ad, nil}
    local returned = nil
    for tile = 1, 4 do
        dosbox.breakpoint_set(0x01f7, helper_returns[tile], {once = true})
        dosbox.debug_continue()
        local hit, err = dosbox.wait_for_breakpoint(timeout_ms)
        if not hit then error("ICO tile return: " .. (err or "timeout")) end
        dosbox.breakpoint_remove(0x01f7, helper_returns[tile])
        returned = hit
        local barrier_segment = hit.segment
        local barrier_offset = next_calls[tile]
        if tile == 4 then
            local registers = hit.registers or dosbox.cpu_state()
            local sp = (registers.sp or registers.esp or 0) & 0xffff
            barrier_offset = dosbox.mem_read_word("ss", sp)
        end
        checkpoint(common_event("after-ico-tile", hit, {
            ico = ico,
            ico_tile = tile,
            map_patch = map_patch,
            vga = vga_snapshot(),
        }), barrier_segment, barrier_offset)
    end
    return returned
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

assert(focus == "both" or focus == "bob" or focus == "ico",
       "TRACE_FOCUS must be bob, ico, or both")
-- An ICO-only run has no BOB breakpoint at which to install the controlled
-- effect MAP.  Stop at the state-machine update entry instead; by this point
-- the level MAP pointer and effect table are live.
if focus == "ico" and patch_map_run then
    dosbox.breakpoint_set(0x01f7, 0x8e4b, {once = true})
    dosbox.debug_continue()
    wait_hit("state-machine map patch entry")
    map_patch = patch_effect_map()
    dosbox.breakpoint_set(0x01f7, 0x8e4b, {once = true})
end
if focus == "ico" then
    arm_ico()
else
    arm_bob()
    if focus == "both" then arm_ico() end
end
local attempts = 0
while sequence < event_limit do
    attempts = attempts + 1
    if attempts > event_limit * 100 then
        events[#events + 1] = {
            stage = "timeout", sequence = sequence + 1,
            frame = dosbox.frame(), error = "internal breakpoint limit",
        }
        break
    end
    if patch_map_run then
        local candidate = patch_effect_map()
        if candidate ~= nil and candidate.error == nil then
            map_patch = candidate
        end
    end
    dosbox.debug_continue()
    local hit, err = dosbox.wait_for_breakpoint(timeout_ms)
    if not hit then
        events[#events + 1] = {
            stage = "timeout", sequence = sequence + 1,
            frame = dosbox.frame(), error = err or "timeout",
        }
        break
    end
    if hit.offset == 0x8e4b and focus == "ico" and patch_map_run then
        local candidate = patch_effect_map()
        if candidate ~= nil and candidate.error == nil then map_patch = candidate end
        dosbox.breakpoint_set(0x01f7, 0x8e4b, {once = true})
        arm_ico()
        -- Continue collecting until the ICO helper itself is reached; the
        -- state-machine entry is a patching synchronization point, not a
        -- rendered event.
    elseif hit.offset == 0x0016 then
        -- Do not leave the other one-shot breakpoint armed while the host is
        -- holding this draw-return checkpoint; otherwise an ICO update can
        -- consume it before the next loop iteration.
        if focus == "both" then dosbox.breakpoint_remove(0x01f7, 0x1186) end
        if patch_map_run then
            local candidate = patch_effect_map()
            if candidate ~= nil and candidate.error == nil then map_patch = candidate end
        end
        local call = bob_call_snapshot(hit)
        local returned, barrier_segment, barrier_offset = advance_bob(hit, call)
        checkpoint(common_event("after-bob", returned,
                               {bob = call, map_patch = map_patch,
                                vga = vga_snapshot()}),
                   barrier_segment, barrier_offset)
        arm_bob()
        if focus == "both" then arm_ico() end
    elseif hit.offset == 0x1186 then
        if focus == "both" then dosbox.breakpoint_remove(0x01f7, 0x0016) end
        local ico = ico_snapshot(hit)
        advance_ico(ico)
        if focus == "both" or focus == "ico" then arm_ico() end
        if focus == "both" then arm_bob() end
    else
        events[#events + 1] = {
            stage = "unexpected", sequence = sequence + 1,
            frame = dosbox.frame(), offset = hit.offset,
        }
        break
    end
end
dosbox.breakpoint_clear()
dosbox.output.renderer_pixel_checkpoints = events
dosbox.output.renderer_pixel_complete = true
