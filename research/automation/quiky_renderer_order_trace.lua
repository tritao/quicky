-- Capture the first renderer-stage entries after a controlled level launch.
local level = TRACE_LEVEL or "W4L1"
local timeout_ms = TRACE_TIMEOUT_MS or 10000
local event_limit = TRACE_EVENT_LIMIT or 24
local patch_map_flags = TRACE_PATCH_MAP_FLAGS or false
local map_flag_mask = TRACE_MAP_FLAG_MASK or 0xfe00
local rearm_special_bob = TRACE_REARM_SPECIAL_BOB or false

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
    [0x0013] = "bob-blit",
    [0x20c8] = "map-column",
    [0x2cb2] = "map-strip",
    [0x2d56] = "map-cell-read",
    [0x2d69] = "map-tile-select",
    [0x2f71] = "page-copy-horizontal",
    [0x2fe9] = "page-copy-vertical",
    [0x3529] = "bob-entry",
    [0x3587] = "render-object-list",
    [0x35c7] = "render-frame",
    [0x36d5] = "bob-copy-clipped",
    [0x1186] = "ico-entry",
    [0x11b4] = "ico-tile-call",
    [0x10aa] = "pit-timer-wait",
    [0x1024] = "special-bob-list-builder",
}

local function wait_hit(label)
    local hit, err = dosbox.wait_for_breakpoint(timeout_ms)
    if not hit then error(label .. ": " .. (err or "timeout")) end
    return hit
end

local function word(selector, offset)
    return dosbox.mem_read_word(selector, offset)
end

local function dword(raw, index)
    local b1 = string.byte(raw or "", index) or 0
    local b2 = string.byte(raw or "", index + 1) or 0
    local b3 = string.byte(raw or "", index + 2) or 0
    local b4 = string.byte(raw or "", index + 3) or 0
    return b1 | (b2 << 8) | (b3 << 16) | (b4 << 24)
end

local function hex(raw)
    return (raw or ""):gsub(".", function(c)
        return string.format("%02x", string.byte(c))
    end)
end

local map_patch = nil

local function map_source_snapshot()
    local pointer = dword(dosbox.mem_read("ds", 0x657a, 4), 1)
    local map_offset = pointer & 0xffff
    local map_selector = (pointer >> 16) & 0xffff
    local stride = word("ds", 0x657e)
    local x = (word("ds", 0x81a8) + word("ds", 0x370c)) & 0xffff
    local y = word("ds", 0x81c4)
    local cell_offset = map_offset + (y >> 4) * stride + (x >> 4) * 2
    local raw = nil
    if map_selector ~= 0 and stride ~= 0 then
        raw = dosbox.mem_read_selector(map_selector, cell_offset, 2)
    end
    return {
        selector = map_selector,
        offset = cell_offset,
        stride = stride,
        x = x,
        y = y,
        cell = raw and (string.byte(raw, 1) or 0) |
            ((string.byte(raw, 2) or 0) << 8) or nil,
    }
end

local function patch_map_source_flags()
    local sample = map_source_snapshot()
    if sample.selector == 0 or sample.cell == nil then
        sample.error = "MAP source not ready"
        return sample
    end
    sample.before = sample.cell
    sample.after = (sample.cell & 0x01ff) | (map_flag_mask & 0xfe00)
    dosbox.mem_write_selector(sample.selector, sample.offset,
                              string.char(sample.after & 0xff,
                                          (sample.after >> 8) & 0xff))
    sample.cell = sample.after
    return sample
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
    if hit.offset == 0x2cb2 or hit.offset == 0x2d56 or
            hit.offset == 0x2d69 or hit.offset == 0x2f71 or
            hit.offset == 0x2fe9 then
        event.map_source = map_source_snapshot()
        event.map_patch = map_patch
    end
    if hit.offset == 0x10aa then
        event.timer = {
            reload_value = word("ds", 0x97f4),
            last_counter = word("ds", 0x60ae),
            timer_pending = word("ds", 0x819e),
        }
    end
    if hit.offset == 0x2d56 then
        event.map_cell_registers = {
            fs = registers.fs,
            bx = (registers.ebx or registers.bx or 0) & 0xffff,
            raw_hex = hex(dosbox.mem_read("fs",
                (registers.ebx or registers.bx or 0) & 0xffff, 2) or ""),
        }
    elseif hit.offset == 0x2d69 then
        event.map_tile_registers = {
            gs = registers.gs,
            bx = (registers.ebx or registers.bx or 0) & 0xffff,
            raw_hex = hex(dosbox.mem_read_selector(registers.gs,
                (registers.ebx or registers.bx or 0) & 0xffff, 16) or ""),
        }
    end
    if hit.offset == 0x3529 or hit.offset == 0x0013 then
        local di = (registers.edi or 0) & 0xffff
        event.es_di = di
        event.object_prefix = {}
        local raw = dosbox.mem_read("es", di, 0x20) or ""
        for index = 1, #raw do
            event.object_prefix[#event.object_prefix + 1] = string.byte(raw, index)
        end
    end
    if hit.offset == 0x1024 then
        -- 1024 copies the special-object byte +0x16 into CL before calling
        -- 34BC. That byte becomes the BOB entry mode/flag word at +6/+7.
        local di = (registers.edi or 0) & 0xffff
        local raw = dosbox.mem_read("es", di, 0x20) or ""
        local function byte_at(delta)
            return string.byte(raw, delta + 1) or 0
        end
        local function word_at(delta)
            return byte_at(delta) | (byte_at(delta + 1) << 8)
        end
        event.special_object = {
            es_di = di,
            kind = word_at(0x14),
            sprite_or_flags = word_at(0x12),
            mode_source_byte = byte_at(0x16),
            mode_source_word = word_at(0x16),
            x = word_at(0x04),
            y = word_at(0x08),
            state = hex(raw),
        }
        event.list_mode_registers = {
            cl = (registers.ecx or registers.cx or 0) & 0xff,
            dx = (registers.edx or registers.dx or 0) & 0xffff,
        }
    end
    if hit.offset == 0x3587 then
        -- 3587 is entered before the object-list count is copied into CX;
        -- the live counter and far pointer are still published in DS.
        event.object_list = {
            count = word("ds", 0x8174),
            pointer = {
                offset = word("ds", 0x6d86),
                selector = word("ds", 0x6d88),
            },
        }
        local count = event.object_list.count or 0
        if count > 0 and count < 64 then
            local raw = dosbox.mem_read_selector(
                event.object_list.pointer.selector,
                event.object_list.pointer.offset, count * 8)
            event.object_list.entries = {}
            for index = 0, count - 1 do
                local base = index * 8
                local function entry_word(delta)
                    local lo = string.byte(raw, base + delta + 1) or 0
                    local hi = string.byte(raw, base + delta + 2) or 0
                    return lo | (hi << 8)
                end
                event.object_list.entries[#event.object_list.entries + 1] = {
                    x = entry_word(0x04),
                    y = entry_word(0x06),
                    flags = entry_word(0x00),
                    aux = entry_word(0x02),
                }
            end
        end
    end
    return event
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
local input_wait = wait_hit("selector input wait")
dosbox.mem_write("ds", 0x85d4,
                 string.char(selector_indices[level] & 0xff,
                             selector_indices[level] >> 8))
dosbox.breakpoint_set(0x01d7, 0x4b18, {once = true})
dosbox.mem_write("ds", 0x88bc, "\x20\x00")
dosbox.debug_continue()
local launch = wait_hit("selector Space dispatch")
dosbox.output.checkpoints = {
    cheat = cheat,
    input_wait = input_wait,
    launch = launch,
}

for offset, _ in pairs(targets) do
    local segment = offset == 0x10aa and 0x0207 or 0x01f7
    dosbox.breakpoint_set(segment, offset, {once = true})
end

local events = {}
for index = 1, event_limit do
    dosbox.debug_continue()
    local hit, err = dosbox.wait_for_breakpoint(timeout_ms)
    if not hit then
        events[#events + 1] = {
            label = "timeout",
            error = err or "timeout",
            frame = dosbox.frame(),
        }
        break
    end
    if patch_map_flags and hit.offset == 0x2cb2 and map_patch == nil then
        map_patch = patch_map_source_flags()
    end
    events[#events + 1] = snapshot(hit)
    if rearm_special_bob and hit.offset == 0x1024 then
        dosbox.breakpoint_set(0x01f7, 0x1024, {once = true})
    end
end
dosbox.breakpoint_clear()
dosbox.output.renderer_events = events
