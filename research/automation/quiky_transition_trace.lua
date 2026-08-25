-- Capture transition checkpoints while the guest is stopped at known loops.
-- The host captures a video frame and POSTs debug/continue for each request.
local timeout_ms = TRACE_TIMEOUT_MS or 15000
local ui_timeout_ms = TRACE_UI_TIMEOUT_MS or 3000
local menu_font_limit = TRACE_MENU_FONT_LIMIT or 32
local startup_status_font_limit = TRACE_STARTUP_STATUS_FONT_LIMIT or 0
local startup_palette_limit = TRACE_STARTUP_PALETTE_LIMIT or 0
local pause_before_launch = TRACE_PAUSE_BEFORE_LAUNCH or false
local capture_menu_setup = TRACE_CAPTURE_MENU_SETUP or false
local menu_key = TRACE_MENU_KEY or ""
local menu_nav_key = TRACE_MENU_NAV_KEY or ""
local menu_nav_mask = TRACE_MENU_NAV_MASK and tonumber(TRACE_MENU_NAV_MASK) or nil
local menu_nav_only = TRACE_MENU_NAV_ONLY or false
local checkpoints = {}
local pcc_probe_done = false
local palette_ramp_probe_done = false
local far_buffer_snapshot

local function wait_hit(label)
    local hit, err = dosbox.wait_for_breakpoint(timeout_ms)
    if not hit then error(label .. ": " .. (err or "timeout")) end
    return hit
end

local function checkpoint(name, hit)
    local record = {
        name = name,
        frame = dosbox.frame(),
        hit = hit,
        registers = hit and hit.registers or dosbox.cpu_state(),
    }
    checkpoints[#checkpoints + 1] = record
    if name == "menu-ready" and far_buffer_snapshot ~= nil then
        record.menu_palette_stage = far_buffer_snapshot(0x6118, 0x400)
        record.menu_palette_destination = far_buffer_snapshot(0x60e4, 0x400)
        record.menu_pcc_descriptor = dosbox.mem_read_word("ds", 0x60bc)
    end
    dosbox.output.transition_checkpoints = checkpoints
    dosbox.output.transition_checkpoint = record
    -- The host owns the debug continue.  Holding for several frames gives the
    -- polling client time to read the stopped frame before the request clears.
    dosbox.wait_frames(30)
    dosbox.output.transition_checkpoint = nil
end

local function optional_ui_checkpoint(name, segment, offset)
    dosbox.breakpoint_set(segment, offset, {once = true})
    local hit, err = dosbox.wait_for_breakpoint(ui_timeout_ms)
    if hit then
        checkpoint(name, hit)
        return hit
    end
    dosbox.breakpoint_remove(segment, offset)
    checkpoints[#checkpoints + 1] = {
        name = name,
        frame = dosbox.frame(),
        skipped = true,
        error = err or "timeout",
    }
    dosbox.output.transition_checkpoints = checkpoints
    return nil
end

local function optional_ui_checkpoint_after_stop(name, segment, offset)
    dosbox.breakpoint_set(segment, offset, {once = true})
    dosbox.debug_continue()
    local hit, err = dosbox.wait_for_breakpoint(ui_timeout_ms)
    if hit then
        checkpoint(name, hit)
        if name == "pcc-copy" then
            checkpoints[#checkpoints].palette_stage = far_buffer_snapshot(0x610c, 0x400)
            dosbox.output.transition_checkpoints = checkpoints
        end
        return hit
    end
    dosbox.breakpoint_remove(segment, offset)
    checkpoints[#checkpoints + 1] = {
        name = name,
        frame = dosbox.frame(),
        skipped = true,
        error = err or "timeout",
    }
    dosbox.output.transition_checkpoints = checkpoints
    return nil
end

local function capture_menu_setup_call()
    dosbox.breakpoint_set(0x01d7, 0x0470,
                          {once = true, deferred = pause_before_launch})
    dosbox.debug_continue()
    local hit, err = dosbox.wait_for_breakpoint(ui_timeout_ms)
    if hit then
        checkpoint("menu-label-setup", hit)
        return hit
    end
    dosbox.breakpoint_remove(0x01d7, 0x0470)
    checkpoints[#checkpoints + 1] = {
        name = "menu-label-setup", frame = dosbox.frame(), skipped = true,
        error = err or "timeout",
    }
    dosbox.output.transition_checkpoints = checkpoints
    return nil
end

local function pascal_text(selector, offset)
    if selector == nil or selector == 0 then
        return {length = 0, text = "", error = "null selector",
                selector = selector, offset = offset}
    end
    local ok, raw = pcall(dosbox.mem_read_selector, selector, offset, 256)
    if not ok or not raw then
        return {length = 0, text = "", error = ok and "empty string" or tostring(raw),
                selector = selector, offset = offset}
    end
    local length = string.byte(raw, 1) or 0
    local chars = {}
    for index = 1, math.min(length, #raw - 1) do
        chars[#chars + 1] = string.char(string.byte(raw, index + 1))
    end
    return {length = length, text = table.concat(chars)}
end

local function bytes_hex(raw, limit)
    if raw == nil then return "" end
    local count = math.min(#raw, limit or #raw)
    local out = {}
    for index = 1, count do
        out[#out + 1] = string.format("%02x", string.byte(raw, index))
    end
    return table.concat(out)
end

local function pcc_buffer_snapshot(descriptor_id)
    if descriptor_id == nil then return {error = "null descriptor id"} end
    local descriptor_table = 0x5196 + descriptor_id * 0x16
    local ok, raw_or_error = pcall(dosbox.mem_read, "ds", descriptor_table, 0x16)
    if not ok or not raw_or_error or #raw_or_error < 0x16 then
        return {descriptor_id = descriptor_id,
                error = ok and "short descriptor" or tostring(raw_or_error)}
    end
    local raw = raw_or_error
    local function word_at(delta)
        local lo = string.byte(raw, delta + 1) or 0
        local hi = string.byte(raw, delta + 2) or 0
        return lo | (hi << 8)
    end
    local pixel_offset = word_at(0)
    local pixel_selector = word_at(2)
    local width = word_at(4)
    local height = word_at(6)
    local size = width * height
    local result = {
        descriptor_id = descriptor_id,
        descriptor_offset = descriptor_table,
        descriptor_hex = bytes_hex(raw),
        pixel_pointer = {offset = pixel_offset, selector = pixel_selector},
        width = width, height = height, pixel_size = size,
    }
    if size > 0 and size <= 0x20000 and pixel_selector ~= 0 then
        local sample_size = math.min(size, 0x400)
        local pixel_ok, pixels_or_error = pcall(
            dosbox.mem_read_selector, pixel_selector, pixel_offset, sample_size)
        if pixel_ok and pixels_or_error then
            local pixels = pixels_or_error
            result.pixel_sample_size = #pixels
            result.pixel_hex = bytes_hex(pixels)
        else
            result.pixel_read_error = pixel_ok and "empty pixel buffer"
                or tostring(pixels_or_error)
        end
    end
    return result
end

local function palette_ramp_snapshot(hit)
    local registers = hit and hit.registers or dosbox.cpu_state()
    local sp = (registers.esp or registers.sp or 0) & 0xffff
    local fade_step = dosbox.mem_read_word("ss", (sp + 4) & 0xffff)
    local source_offset = dosbox.mem_read_word("ss", (sp + 6) & 0xffff)
    local source_selector = dosbox.mem_read_word("ss", (sp + 8) & 0xffff)
    local result = {
        sp = sp, fade_step = fade_step,
        source = {offset = source_offset, selector = source_selector},
    }
    if source_selector ~= nil and source_selector ~= 0 then
        local ok, raw_or_error = pcall(
            dosbox.mem_read_selector, source_selector, source_offset, 0x400)
        if ok and raw_or_error then
            result.source_hex = bytes_hex(raw_or_error)
            result.source_size = #raw_or_error
        else
            result.source_read_error = ok and "empty source" or tostring(raw_or_error)
        end
    end
    return result
end

far_buffer_snapshot = function(offset_address, size)
    local offset = dosbox.mem_read_word("ds", offset_address)
    local selector = dosbox.mem_read_word("ds", offset_address + 2)
    local result = {pointer = {offset = offset, selector = selector}, size = size}
    if selector ~= nil and selector ~= 0 and offset ~= nil then
        local ok, raw_or_error = pcall(
            dosbox.mem_read_selector, selector, offset, size)
        if ok and raw_or_error then
            result.hex = bytes_hex(raw_or_error)
            result.actual_size = #raw_or_error
        else
            result.read_error = ok and "empty buffer" or tostring(raw_or_error)
        end
    end
    return result
end

local function collect_menu_font_calls(limit)
    local events = {}
    for index = 1, limit do
        dosbox.breakpoint_set(0x01d7, 0x0047, {once = true})
        dosbox.debug_continue()
        local hit, err = dosbox.wait_for_breakpoint(ui_timeout_ms)
        if not hit then
            events[#events + 1] = {skipped = true, frame = dosbox.frame(),
                                   error = err or "timeout"}
            dosbox.breakpoint_remove(0x01d7, 0x0047)
            break
        end
        local registers = hit.registers or dosbox.cpu_state()
        local bp = (registers.bp or registers.ebp or 0) & 0xffff
        local sp = (registers.sp or registers.esp or 0) & 0xffff
        -- The breakpoint is at the callee entry before its prologue, so BP
        -- still names the caller's frame.  The near-call return IP is BP+2;
        -- the Pascal far string pointer begins at BP+4.
        local text_offset = dosbox.mem_read_word("ss", (bp + 4) & 0xffff)
        local text_selector = dosbox.mem_read_word("ss", (bp + 6) & 0xffff)
        local text = pascal_text(text_selector, text_offset)
        events[#events + 1] = {
            frame = dosbox.frame(),
            hit = hit,
            registers = registers,
            stack = {
                pointer = {selector = text_selector, offset = text_offset},
                x = dosbox.mem_read_word("ss", (bp + 8) & 0xffff),
                y = dosbox.mem_read_word("ss", (bp + 10) & 0xffff),
                align = dosbox.mem_read_word("ss", (bp + 12) & 0xffff),
            },
            return_address = {
                offset = dosbox.mem_read_word("ss", sp),
                segment = dosbox.mem_read_word("ss", (sp + 2) & 0xffff),
                stack_pointer = sp,
            },
            pascal = text,
            selector_state = dosbox.mem_read_word("ds", 0x85d4),
            action_flags = dosbox.mem_read_word("ds", 0x88bc),
            normalized_action = dosbox.mem_read_word("ds", 0x8196),
            raw_stack = {bp = bp, words = {}},
        }
        for delta = -8, 32, 2 do
            events[#events].raw_stack.words[tostring(delta)] =
                dosbox.mem_read_word("ss", (bp + delta) & 0xffff)
        end
        dosbox.breakpoint_remove(0x01d7, 0x0047)
    end
    dosbox.output.menu_font_events = events
    return events
end

-- Fixed menu labels use the character-string helper at 03F4, while the
-- blinking selector line uses 0047.  Capture both call families in the same
-- stopped menu window so page/composition behavior is not inferred from the
-- screenshot alone.
local function collect_status_font_calls(limit)
    local events = {}
    for index = 1, limit do
        dosbox.breakpoint_set(0x01d7, 0x03f4, {once = true})
        dosbox.debug_continue()
        local hit, err = dosbox.wait_for_breakpoint(ui_timeout_ms)
        if not hit then
            events[#events + 1] = {skipped = true, frame = dosbox.frame(),
                                   error = err or "timeout"}
            dosbox.breakpoint_remove(0x01d7, 0x03f4)
            break
        end
        local registers = hit.registers or dosbox.cpu_state()
        local bp = (registers.bp or registers.ebp or 0) & 0xffff
        local text_offset = dosbox.mem_read_word("ss", (bp + 4) & 0xffff)
        local text_selector = dosbox.mem_read_word("ss", (bp + 6) & 0xffff)
        events[#events + 1] = {
            frame = dosbox.frame(), hit = hit, registers = registers,
            stack = {
                pointer = {selector = text_selector, offset = text_offset},
                x = dosbox.mem_read_word("ss", (bp + 8) & 0xffff),
                y = dosbox.mem_read_word("ss", (bp + 10) & 0xffff),
            },
            pascal = pascal_text(text_selector, text_offset),
            selector_state = dosbox.mem_read_word("ds", 0x85d4),
            action_flags = dosbox.mem_read_word("ds", 0x88bc),
            normalized_action = dosbox.mem_read_word("ds", 0x8196),
        }
        dosbox.breakpoint_remove(0x01d7, 0x03f4)
    end
    dosbox.output.status_font_events = events
    return events
end

local function collect_menu_palette_calls(limit)
    local events = {}
    for _ = 1, limit do
        dosbox.breakpoint_set(0x0207, 0x0536, {once = true})
        dosbox.breakpoint_set(0x0207, 0x05ff, {once = true})
        -- Stable-menu page ownership is separate from the DAC helpers.  Keep
        -- the CRTC writer armed in the same window so a page handoff cannot
        -- be mistaken for a palette event.
        dosbox.breakpoint_set(0x0207, 0x00b3, {once = true})
        -- 01F7:0E21 is the only non-helper code site that loads DAC index
        -- 0 directly (RGB 0x3f,0,0). It is a candidate for transient red
        -- feedback, so include it when checking whether menu blink owns a
        -- palette write outside the common helpers.
        dosbox.breakpoint_set(0x01f7, 0x0e21, {once = true})
        dosbox.debug_continue()
        local hit, err = dosbox.wait_for_breakpoint(ui_timeout_ms)
        if not hit then
            events[#events + 1] = {skipped = true, frame = dosbox.frame(),
                                   error = err or "timeout"}
            dosbox.breakpoint_remove(0x0207, 0x0536)
            dosbox.breakpoint_remove(0x0207, 0x05ff)
            dosbox.breakpoint_remove(0x0207, 0x00b3)
            dosbox.breakpoint_remove(0x01f7, 0x0e21)
            break
        end
        local registers = hit.registers or dosbox.cpu_state()
        local event = {frame = dosbox.frame(), hit = hit,
                       registers = registers, offset = hit.offset}
        if hit.offset == 0x00b3 then
            event.kind = "page-start"
            local sp = (registers.sp or registers.esp or 0) & 0xffff
            local start_address = dosbox.mem_read_word("ss", (sp + 0x04) & 0xffff)
            event.page_start = {
                address = start_address,
                low = (start_address or 0) & 0xff,
                high = ((start_address or 0) >> 8) & 0xff,
            }
        elseif hit.offset == 0x0e21 then
            event.kind = "direct-index-zero"
            event.call_args = {dac_index = 0, rgb = {0x3f, 0, 0}}
        elseif hit.offset == 0x05ff then
            local bp = (registers.bp or registers.ebp or 0) & 0xffff
            local function stack_word(delta)
                return dosbox.mem_read_word("ss", (bp + delta) & 0xffff)
            end
            event.kind = "single-entry"
            event.call_args = {
                dac_index = stack_word(0x0c) & 0xff,
                rgb = {stack_word(0x0a) & 0xff, stack_word(0x08) & 0xff,
                       stack_word(0x06) & 0xff},
            }
        else
            event.kind = "full-upload"
            event.palette_pointer = {
                selector = registers.ds,
                offset = (registers.si or registers.esi or 0) & 0xffff,
            }
        end
        events[#events + 1] = event
        dosbox.breakpoint_remove(0x0207, 0x0536)
        dosbox.breakpoint_remove(0x0207, 0x05ff)
        dosbox.breakpoint_remove(0x0207, 0x00b3)
        dosbox.breakpoint_remove(0x01f7, 0x0e21)
    end
    dosbox.output.menu_palette_events = events
    return events
end

-- Capture the one-time title/menu setup writes before the normal menu loop
-- takes over.  The settled-menu probe intentionally starts later and therefore
-- cannot see the upload/page handoff that installs MENU.PCC's palette.
local function advance_startup_palette_event(hit)
    local next_offset = nil
    if hit.offset == 0x0536 then
        next_offset = 0x0539
    elseif hit.offset == 0x05ff then
        next_offset = 0x0605
    elseif hit.offset == 0x00b3 then
        next_offset = 0x00b6
    elseif hit.offset == 0x0e21 then
        next_offset = 0x0e24
    end
    if next_offset ~= nil then
        dosbox.breakpoint_set(0x0207, next_offset, {once = true})
        -- 0E21 is in segment 01F7, but its acknowledgement is not needed
        -- for the normal startup path; the other three helpers share 0207.
        if hit.offset == 0x0e21 then
            dosbox.breakpoint_remove(0x0207, next_offset)
            next_offset = nil
        end
    end
    if next_offset ~= nil then
        dosbox.debug_continue()
        local stepped = dosbox.wait_for_breakpoint(ui_timeout_ms)
        dosbox.breakpoint_remove(0x0207, next_offset)
        if stepped then return true end
    end
    dosbox.debug_continue()
    dosbox.wait_frames(1)
    return true
end

local function collect_startup_palette_calls(limit)
    local events = {}
    local idle_misses = 0
    local first_arm = true
    local attempts = 0
    while #events < limit and attempts < limit + 3 do
        attempts = attempts + 1
        -- Segment 4 may not have a protected-mode descriptor on the first
        -- pass after script load; deferred breakpoints survive that loader
        -- boundary and become active when QUIKY.EXE enters protected mode.
        local deferred = first_arm
        first_arm = false
        dosbox.breakpoint_set(0x0207, 0x0536, {once = true, deferred = deferred})
        dosbox.breakpoint_set(0x0207, 0x05ff, {once = true, deferred = deferred})
        dosbox.breakpoint_set(0x0207, 0x00b3, {once = true, deferred = deferred})
        dosbox.breakpoint_set(0x01f7, 0x0e21, {once = true, deferred = deferred})
        dosbox.debug_continue()
        local hit, err = dosbox.wait_for_breakpoint(ui_timeout_ms)
        if not hit then
            dosbox.breakpoint_remove(0x0207, 0x0536)
            dosbox.breakpoint_remove(0x0207, 0x05ff)
            dosbox.breakpoint_remove(0x0207, 0x00b3)
            dosbox.breakpoint_remove(0x01f7, 0x0e21)
            idle_misses = idle_misses + 1
            if idle_misses >= 3 then
                events[#events + 1] = {skipped = true, frame = dosbox.frame(),
                                       error = err or "timeout"}
                break
            end
        else
            idle_misses = 0
            local registers = hit.registers or dosbox.cpu_state()
            local event = {frame = dosbox.frame(), hit = hit,
                       registers = registers, offset = hit.offset}
            if hit.offset == 0x00b3 then
            event.kind = "page-start"
            local sp = (registers.sp or registers.esp or 0) & 0xffff
            local start_address = dosbox.mem_read_word("ss", (sp + 0x04) & 0xffff)
            event.page_start = {
                address = start_address,
                low = (start_address or 0) & 0xff,
                high = ((start_address or 0) >> 8) & 0xff,
            }
            elseif hit.offset == 0x0e21 then
            event.kind = "direct-index-zero"
            event.call_args = {dac_index = 0, rgb = {0x3f, 0, 0}}
            elseif hit.offset == 0x05ff then
            local bp = (registers.bp or registers.ebp or 0) & 0xffff
            local function stack_word(delta)
                return dosbox.mem_read_word("ss", (bp + delta) & 0xffff)
            end
            event.kind = "single-entry"
            event.call_args = {
                dac_index = stack_word(0x0c) & 0xff,
                rgb = {stack_word(0x0a) & 0xff, stack_word(0x08) & 0xff,
                       stack_word(0x06) & 0xff},
            }
            else
            event.kind = "full-upload"
            event.palette_pointer = {
                selector = registers.ds,
                offset = (registers.si or registers.esi or 0) & 0xffff,
            }
            end
            events[#events + 1] = event
            dosbox.breakpoint_remove(0x0207, 0x0536)
            dosbox.breakpoint_remove(0x0207, 0x05ff)
            dosbox.breakpoint_remove(0x0207, 0x00b3)
            dosbox.breakpoint_remove(0x01f7, 0x0e21)
            advance_startup_palette_event(hit)
        end
    end
    dosbox.output.startup_palette_events = events
    return events
end

local function probe_menu_confirmation_key(key)
    local targets = {
        {segment = 0x01d7, offset = 0x50c3, name = "pcc-palette-callsite"},
        {segment = 0x01d7, offset = 0x3020, name = "pcc-palette-pcc-copy"},
        {segment = 0x0207, offset = 0x022a, name = "transition-helper"},
        {segment = 0x0207, offset = 0x02a5, name = "pcc-palette-ramp"},
        {segment = 0x0207, offset = 0x0536, name = "pcc-palette-upload"},
    }
    for _, target in ipairs(targets) do
        dosbox.breakpoint_set(target.segment, target.offset, {once = true})
    end
    dosbox.key(key, true)
    dosbox.wait_frames(1)
    dosbox.key(key, false)
    dosbox.debug_continue()
    local hit, err = dosbox.wait_for_breakpoint(ui_timeout_ms * 2)
    local event = {key = key, frame = dosbox.frame()}
    if hit then
        event.hit = hit
        event.name = "unknown"
        for _, target in ipairs(targets) do
            if target.segment == hit.segment and target.offset == hit.offset then
                event.name = target.name
                break
            end
        end
        event.selector_state = dosbox.mem_read_word("ds", 0x85d4)
        event.transition_state = dosbox.mem_read_byte("ds", 0x85da)
        if hit.segment == 0x0207 and hit.offset == 0x022a then
            local registers = hit.registers or dosbox.cpu_state()
            local sp = (registers.esp or registers.sp or 0) & 0xffff
            local function stack_word(delta)
                return dosbox.mem_read_word("ss", (sp + delta) & 0xffff)
            end
            event.transition_args = {
                sp = sp,
                return_address = {offset = stack_word(0x00),
                                  segment = stack_word(0x02)},
                transition_flag = stack_word(0x04),
                fade_step = stack_word(0x06),
            }
        end
        if hit.segment == 0x0207 and hit.offset == 0x02a5 then
            local registers = hit.registers or dosbox.cpu_state()
            local sp = (registers.esp or registers.sp or 0) & 0xffff
            local function stack_word(delta)
                return dosbox.mem_read_word("ss", (sp + delta) & 0xffff)
            end
            local source_offset = stack_word(0x06)
            local source_selector = stack_word(0x08)
            event.ramp_args = {
                sp = sp,
                fade_step = stack_word(0x04),
                source = {offset = source_offset, selector = source_selector},
                return_address = {offset = stack_word(0x00),
                                   segment = stack_word(0x02)},
            }
            if source_offset ~= nil and source_selector ~= nil and source_selector ~= 0 then
                local ok, raw = pcall(dosbox.mem_read_selector,
                                      source_selector, source_offset, 0x400)
                if ok and raw then
                    event.source_size = #raw
                    -- Keep the ledger JSON small and avoid coercion of a
                    -- 0x400-byte hex string by the debugger bridge.  The
                    -- transition wrapper's flag=1 branch is expected to
                    -- memset this staging source to zero.
                    event.source_sample_hex = bytes_hex(raw, 32)
                    local zero_count = 0
                    for index = 1, #raw do
                        if string.byte(raw, index) == 0 then
                            zero_count = zero_count + 1
                        end
                    end
                    event.source_zero_count = zero_count
                else
                    event.source_read_error = ok and "empty source" or tostring(raw)
                end
            end
        end
    else
        event.skipped = true
        event.error = err or "timeout"
    end
    for _, target in ipairs(targets) do
        dosbox.breakpoint_remove(target.segment, target.offset)
    end
    dosbox.output.menu_key_event = event
    return event
end

local function probe_menu_navigation_key(key)
    -- The fixed INFO/SOUND/EXIT labels are redrawn by the selector's
    -- navigation branch.  Arm the helper and its two arrow callsites without
    -- arming the hot per-frame 0047 blitter, so a one-shot hit cannot be
    -- starved by the blinking QUICKY line.
    local targets = {
        {segment = 0x01d7, offset = 0x0470, name = "menu-label-setup"},
        {segment = 0x01d7, offset = 0x13b6, name = "menu-down-callsite"},
        {segment = 0x01d7, offset = 0x13e0, name = "menu-up-callsite"},
    }
    for _, target in ipairs(targets) do
        dosbox.breakpoint_set(target.segment, target.offset, {once = true})
    end
    dosbox.key(key, true)
    dosbox.wait_frames(1)
    dosbox.key(key, false)
    dosbox.debug_continue()
    local hit, err = dosbox.wait_for_breakpoint(ui_timeout_ms * 2)
    local event = {key = key, frame = dosbox.frame()}
    if hit then
        event.hit = hit
        event.name = "unknown"
        for _, target in ipairs(targets) do
            if target.segment == hit.segment and target.offset == hit.offset then
                event.name = target.name
                break
            end
        end
        event.selector_state = dosbox.mem_read_word("ds", 0x85d4)
        event.transition_state = dosbox.mem_read_byte("ds", 0x85da)
    else
        event.skipped = true
        event.error = err or "timeout"
    end
    for _, target in ipairs(targets) do
        dosbox.breakpoint_remove(target.segment, target.offset)
    end
    dosbox.output.menu_nav_event = event
    return event
end

local function probe_menu_navigation_mask(mask)
    -- Drive the selector branch at the post-input test directly.  This keeps
    -- the experiment independent of whether the DOSBox key name maps to the
    -- executable's legacy action bit for the current keyboard layout.
    local targets = {
        {segment = 0x01d7, offset = 0x13a5, name = "menu-input-return"},
        {segment = 0x01d7, offset = 0x0470, name = "menu-label-setup"},
        {segment = 0x01d7, offset = 0x13b6, name = "menu-down-callsite"},
        {segment = 0x01d7, offset = 0x13e0, name = "menu-up-callsite"},
    }
    for _, target in ipairs(targets) do
        dosbox.breakpoint_set(target.segment, target.offset, {once = true})
    end
    dosbox.debug_continue()
    local hit, err = dosbox.wait_for_breakpoint(ui_timeout_ms * 2)
    local event = {mask = mask, frame = dosbox.frame()}
    if hit then
        event.first_hit = hit
        event.first_name = "unknown"
        for _, target in ipairs(targets) do
            if target.segment == hit.segment and target.offset == hit.offset then
                event.first_name = target.name
                break
            end
        end
        if hit.segment == 0x01d7 and hit.offset == 0x13a5 then
            local value = mask & 0xffff
            dosbox.mem_write("ds", 0x8196,
                string.char(value & 0xff, (value >> 8) & 0xff))
            dosbox.mem_write("ds", 0x88bc,
                string.char(value & 0xff, (value >> 8) & 0xff))
            dosbox.debug_continue()
            local branch_hit, branch_err =
                dosbox.wait_for_breakpoint(ui_timeout_ms * 2)
            if branch_hit then
                event.branch_hit = branch_hit
                event.branch_name = "unknown"
                for _, target in ipairs(targets) do
                    if target.segment == branch_hit.segment and
                            target.offset == branch_hit.offset then
                        event.branch_name = target.name
                        break
                    end
                end
                if branch_hit.segment == 0x01d7 and
                        (branch_hit.offset == 0x13b6 or
                         branch_hit.offset == 0x13e0) then
                    dosbox.breakpoint_set(0x01d7, 0x0470, {once = true})
                    dosbox.debug_continue()
                    local label_hit, label_err =
                        dosbox.wait_for_breakpoint(ui_timeout_ms * 2)
                    if label_hit then
                        event.label_hit = label_hit
                        event.label_callsite = string.format("%04x",
                            branch_hit.offset)
                        event.label_font_events = {}
                        for index = 1, 4 do
                            dosbox.breakpoint_set(0x01d7, 0x0047, {once = true})
                            dosbox.debug_continue()
                            local font_hit, font_err =
                                dosbox.wait_for_breakpoint(ui_timeout_ms * 2)
                            if not font_hit then
                                event.label_font_events[#event.label_font_events + 1] =
                                    {skipped = true, error = font_err or "timeout"}
                                dosbox.breakpoint_remove(0x01d7, 0x0047)
                                break
                            end
                            local font_registers = font_hit.registers or dosbox.cpu_state()
                            local font_bp = (font_registers.bp or
                                font_registers.ebp or 0) & 0xffff
                            local font_sp = (font_registers.sp or
                                font_registers.esp or 0) & 0xffff
                            -- This call is reached synchronously from 0470,
                            -- whose BP is its own frame.  Read the near-call
                            -- argument layout from SP rather than assuming
                            -- the outer menu loop's BP-relative layout.
                            local font_offset = dosbox.mem_read_word(
                                "ss", (font_sp + 2) & 0xffff)
                            local font_selector = dosbox.mem_read_word(
                                "ss", (font_sp + 4) & 0xffff)
                            event.label_font_events[#event.label_font_events + 1] = {
                                frame = dosbox.frame(), hit = font_hit,
                                pascal = pascal_text(font_selector, font_offset),
                                stack = {
                                    pointer = {offset = font_offset,
                                               selector = font_selector},
                                    y = dosbox.mem_read_word("ss", (font_sp + 6) & 0xffff),
                                    x = dosbox.mem_read_word("ss", (font_sp + 8) & 0xffff),
                                },
                            }
                            dosbox.breakpoint_remove(0x01d7, 0x0047)
                        end
                    else
                        event.label_error = label_err or "timeout"
                    end
                end
                hit = branch_hit
            else
                event.branch_error = branch_err or "timeout"
            end
        end
        event.selector_state = dosbox.mem_read_word("ds", 0x85d4)
        event.transition_state = dosbox.mem_read_byte("ds", 0x85da)
    else
        event.skipped = true
        event.error = err or "timeout"
    end
    for _, target in ipairs(targets) do
        dosbox.breakpoint_remove(target.segment, target.offset)
    end
    if hit then dosbox.debug_continue() end
    dosbox.output.menu_nav_mask_event = event
    return event
end

local function collect_hud_candidates()
    local targets = {
        {name = "status-copy-0880", segment = 0x01d7, offset = 0x0880},
        {name = "status-copy-0c4c", segment = 0x01d7, offset = 0x0c4c},
        {name = "status-copy-10d6", segment = 0x01d7, offset = 0x10d6},
        {name = "scoreboard-14e1", segment = 0x01d7, offset = 0x14e1},
        {name = "pcc-copy-3020", segment = 0x01d7, offset = 0x3020},
        {name = "gamebar-state-3fb0", segment = 0x01d7, offset = 0x3fb0},
        {name = "gamebar-blit-call-4486", segment = 0x01d7, offset = 0x4486},
        {name = "introbar-blit-call-4495", segment = 0x01d7, offset = 0x4495},
        {name = "pcc-blit-helper-0b6c", segment = 0x0207, offset = 0x0b6c},
        {name = "palette-ramp-02a5", segment = 0x0207, offset = 0x02a5},
        {name = "first-resource-lookup", segment = 0x0207, offset = 0x18c7},
    }
    local hits = {}
    local lookup_hit = nil
    for _ = 1, 8 do
        for _, target in ipairs(targets) do
            dosbox.breakpoint_set(target.segment, target.offset, {once = true})
        end
        dosbox.debug_continue()
        local hit, err = dosbox.wait_for_breakpoint(ui_timeout_ms)
        if not hit then
            hits[#hits + 1] = {skipped = true, error = err or "timeout",
                               frame = dosbox.frame()}
            break
        end
        local label = string.format("0x%04x", hit.offset)
        for _, target in ipairs(targets) do
            if target.offset == hit.offset and target.segment == hit.segment then
                label = target.name
                break
            end
        end
        hits[#hits + 1] = {name = label, frame = dosbox.frame(), hit = hit,
                           registers = hit.registers}
        if not pcc_probe_done and (label == "gamebar-blit-call-4486" or
                label == "introbar-blit-call-4495") then
            local descriptor_id = dosbox.mem_read_word("ds",
                label == "gamebar-blit-call-4486" and 0x60be or 0x60c0)
            hits[#hits].pcc_buffer = pcc_buffer_snapshot(descriptor_id)
            hits[#hits].pcc_buffer_probe = true
            pcc_probe_done = true
        end
        if not palette_ramp_probe_done and label == "palette-ramp-02a5" then
            hits[#hits].palette_ramp = palette_ramp_snapshot(hit)
            palette_ramp_probe_done = true
        end
        if label == "pcc-copy-3020" then
            hits[#hits].palette_stage = far_buffer_snapshot(0x610c, 0x400)
        end
        if label == "first-resource-lookup" then
            lookup_hit = hit
            break
        end
    end
    for _, target in ipairs(targets) do
        dosbox.breakpoint_remove(target.segment, target.offset)
    end
    dosbox.output.hud_candidates = hits
    return lookup_hit
end

local function collect_hud_candidates_until_render(existing)
    local targets = {
        {name = "status-copy-0880", segment = 0x01d7, offset = 0x0880},
        {name = "status-copy-0c4c", segment = 0x01d7, offset = 0x0c4c},
        {name = "status-copy-10d6", segment = 0x01d7, offset = 0x10d6},
        {name = "scoreboard-14e1", segment = 0x01d7, offset = 0x14e1},
        {name = "pcc-copy-3020", segment = 0x01d7, offset = 0x3020},
        {name = "gamebar-state-3fb0", segment = 0x01d7, offset = 0x3fb0},
        {name = "gamebar-blit-call-4486", segment = 0x01d7, offset = 0x4486},
        {name = "introbar-blit-call-4495", segment = 0x01d7, offset = 0x4495},
        {name = "pcc-blit-helper-0b6c", segment = 0x0207, offset = 0x0b6c},
        {name = "palette-ramp-02a5", segment = 0x0207, offset = 0x02a5},
        {name = "first-render-frame", segment = 0x01f7, offset = 0x35c7},
    }
    local hits = existing or {}
    local render_hit = nil
    for _ = 1, 8 do
        for _, target in ipairs(targets) do
            dosbox.breakpoint_set(target.segment, target.offset, {once = true})
        end
        dosbox.debug_continue()
        local hit, err = dosbox.wait_for_breakpoint(timeout_ms)
        if not hit then
            hits[#hits + 1] = {skipped = true, error = err or "timeout",
                               frame = dosbox.frame()}
            break
        end
        local label = string.format("0x%04x", hit.offset)
        for _, target in ipairs(targets) do
            if target.offset == hit.offset and target.segment == hit.segment then
                label = target.name
                break
            end
        end
        hits[#hits + 1] = {name = label, frame = dosbox.frame(), hit = hit,
                           registers = hit.registers}
        if not pcc_probe_done and (label == "gamebar-blit-call-4486" or
                label == "introbar-blit-call-4495") then
            local descriptor_id = dosbox.mem_read_word("ds",
                label == "gamebar-blit-call-4486" and 0x60be or 0x60c0)
            hits[#hits].pcc_buffer = pcc_buffer_snapshot(descriptor_id)
            hits[#hits].pcc_buffer_probe = true
            pcc_probe_done = true
        end
        if not palette_ramp_probe_done and label == "palette-ramp-02a5" then
            hits[#hits].palette_ramp = palette_ramp_snapshot(hit)
            palette_ramp_probe_done = true
        end
        if label == "pcc-copy-3020" then
            hits[#hits].palette_stage = far_buffer_snapshot(0x610c, 0x400)
        end
        if label == "first-render-frame" then
            render_hit = hit
            break
        end
    end
    for _, target in ipairs(targets) do
        dosbox.breakpoint_remove(target.segment, target.offset)
    end
    dosbox.output.hud_candidates = hits
    return render_hit
end

local function collect_state_path_after_render()
    -- 47F0 is the normal per-frame update/render-loop entry.  Capture it once
    -- to prove that the static state-loop disassembly is the live caller, then
    -- remove it so the deeper conditional path can be observed without being
    -- starved by a breakpoint that fires every frame.
    local loop_hit, loop_err
    dosbox.breakpoint_set(0x01d7, 0x47f0, {once = true})
    dosbox.debug_continue()
    loop_hit, loop_err = dosbox.wait_for_breakpoint(ui_timeout_ms)
    local events = {}
    if loop_hit then
        events[#events + 1] = {
            name = "state-loop-entry", frame = dosbox.frame(), hit = loop_hit,
            registers = loop_hit.registers,
            goal_mask = dosbox.mem_read_word("ds", 0x60d8),
            exit_flag = dosbox.mem_read_word("ds", 0x89e6),
            transition_done = dosbox.mem_read_word("ds", 0x89e0),
            transition_pending = dosbox.mem_read_word("ds", 0x89ec),
        }
    else
        events[#events + 1] = {name = "state-loop-entry", skipped = true,
                               frame = dosbox.frame(), error = loop_err or "timeout"}
    end

    local branch_target = {
        name = "state-post-render-branch", segment = 0x01d7, offset = 0x48e6,
    }
    dosbox.breakpoint_set(branch_target.segment, branch_target.offset, {once = true})
    dosbox.debug_continue()
    local branch_hit, branch_err = dosbox.wait_for_breakpoint(ui_timeout_ms)
    if branch_hit then
        events[#events + 1] = {
            name = branch_target.name, frame = dosbox.frame(), hit = branch_hit,
            registers = branch_hit.registers,
            goal_mask = dosbox.mem_read_word("ds", 0x60d8),
            exit_flag = dosbox.mem_read_word("ds", 0x89e6),
            transition_done = dosbox.mem_read_word("ds", 0x89e0),
            transition_pending = dosbox.mem_read_word("ds", 0x89ec),
        }
    else
        events[#events + 1] = {name = branch_target.name, skipped = true,
                               frame = dosbox.frame(), error = branch_err or "timeout"}
    end

    local targets = {
        {name = "state-input-dispatch", segment = 0x01d7, offset = 0x491d},
        {name = "state-transition-dispatch", segment = 0x01d7, offset = 0x4cfc},
        {name = "state-transition-stage", segment = 0x01d7, offset = 0x4d03},
        {name = "exit-flag-gate", segment = 0x01d7, offset = 0x4ea0},
        {name = "completion-hud-call", segment = 0x01d7, offset = 0x4f0d},
        {name = "state-reload-stage", segment = 0x01d7, offset = 0x5010},
        {name = "state-loop-return", segment = 0x01d7, offset = 0x504f},
    }
    -- Probe each deeper boundary independently.  Several of these are loop
    -- entries (notably 491D), so arming the whole set at once would repeatedly
    -- stop at the first hot address and starve the later branches.
    for _, target in ipairs(targets) do
        dosbox.breakpoint_set(target.segment, target.offset, {once = true})
        dosbox.debug_continue()
        local hit, err = dosbox.wait_for_breakpoint(ui_timeout_ms)
        if not hit then
            events[#events + 1] = {name = target.name, skipped = true,
                                   frame = dosbox.frame(),
                                   error = err or "timeout"}
        else
            events[#events + 1] = {
                name = target.name, frame = dosbox.frame(), hit = hit,
                registers = hit.registers,
                goal_mask = dosbox.mem_read_word("ds", 0x60d8),
                exit_flag = dosbox.mem_read_word("ds", 0x89e6),
                transition_done = dosbox.mem_read_word("ds", 0x89e0),
                transition_pending = dosbox.mem_read_word("ds", 0x89ec),
            }
        end
        dosbox.breakpoint_remove(target.segment, target.offset)
    end
    dosbox.output.state_path_events = events
    return events
end

dosbox.output.awaiting_startup_replay = true
if pause_before_launch then
    dosbox.key("KBD_enter", true)
    dosbox.wait_frames(1)
    dosbox.key("KBD_enter", false)
end
if capture_menu_setup then
    capture_menu_setup_call()
end
if startup_status_font_limit > 0 then
    -- Arm before the scripted Esc sequence begins; score/status text can be
    -- copied during title/menu setup and is no longer called in the stable
    -- menu loop at frame 350.
    collect_status_font_calls(startup_status_font_limit)
end
if startup_palette_limit > 0 then
    collect_startup_palette_calls(startup_palette_limit)
end
dosbox.wait_frames(350)

-- The normal startup replay lands in the menu input loop.
local menu_cpu = dosbox.cpu_state()
checkpoint("menu-ready", {
    segment = menu_cpu.cs,
    offset = menu_cpu.eip,
    registers = menu_cpu,
})

-- Capture the actual menu text blitter calls before entering the selector.
-- The call arguments include the Pascal string pointer and logical x/y, which
-- lets us distinguish fixed labels from the blinking selected item without
-- inferring the font layer from screenshots alone.
collect_menu_font_calls(menu_font_limit)
collect_menu_palette_calls(8)
if menu_nav_key ~= "" then
    probe_menu_navigation_key(menu_nav_key)
end
if menu_nav_mask ~= nil then
    probe_menu_navigation_mask(menu_nav_mask)
end
if menu_nav_only then
    dosbox.output.transition_trace_complete = true
    return
end
if menu_key ~= "" then
    probe_menu_confirmation_key(menu_key)
end

-- Use the verified cheat route from the other traces to enter the selector;
-- this avoids depending on the menu's blinking selection edge while the
-- checkpoint is held.
dosbox.key("KBD_space", true)
dosbox.wait_frames(4)
dosbox.key("KBD_space", false)
dosbox.wait_frames(30)
dosbox.type("QUIKYSUPERHERO")
dosbox.wait_frames(3)
dosbox.breakpoint_set(0x01d7, 0x491d, {once = true})
dosbox.key("KBD_4", true)
local cheat = wait_hit("cheat selector branch")
dosbox.key("KBD_4", false)
checkpoint("cheat-selector", cheat)
dosbox.mem_write("ds", 0x89f2, "\x01")
dosbox.mem_write("ds", 0x88ba, "\x05\x00")
dosbox.debug_continue()
-- The selector/menu dispatcher is a live boundary shared by the HUD and
-- menu state machine.  Capture it when this route reaches it, but keep the
-- trace useful on builds that skip the branch.
optional_ui_checkpoint("ui-state-dispatch", 0x01d7, 0x313d)
dosbox.wait_frames(60)
dosbox.breakpoint_set(0x01d7, 0x4ace, {once = true})
local selector_wait = wait_hit("selector input wait")
-- This helper copies the selected PCC surface and its 0x14-row strip.  It is
-- the concrete menu/HUD blit boundary, so capture its registers and frame.
optional_ui_checkpoint_after_stop("pcc-copy", 0x01d7, 0x3020)
dosbox.mem_write("ds", 0x85d4, "\x09\x00")
dosbox.breakpoint_set(0x01d7, 0x4b18, {once = true})
dosbox.mem_write("ds", 0x88bc, "\x20\x00")
dosbox.debug_continue()
local dispatch = wait_hit("selector Space dispatch")
checkpoint("selector-dispatch", dispatch)

-- Candidate HUD/status copies are event-driven and may run before the first
-- world frame. Capture their one-shot call order without assuming any one
-- helper is the normal per-frame path.
local early_lookup = collect_hud_candidates()

-- Capture the first archive lookup and the first renderer frame after it.
local lookup = early_lookup
if lookup == nil then
    dosbox.breakpoint_set(0x0207, 0x18c7, {once = true})
    dosbox.debug_continue()
    lookup = wait_hit("first level resource lookup")
end
checkpoint("first-resource-lookup", lookup)

local render = collect_hud_candidates_until_render()
if render == nil then
    dosbox.breakpoint_set(0x01f7, 0x35c7, {once = true})
    dosbox.debug_continue()
    render = wait_hit("first renderer frame")
end
checkpoint("first-render-frame", render)

-- The resource/state initializer that names GAMEBAR.PCC sits at 01D7:3FB0.
-- Probe it after the first gameplay render as a candidate normal-HUD setup
-- boundary; this is intentionally separate from the completion compositor
-- below because static callers place that routine on the exit path.
optional_ui_checkpoint_after_stop("gamebar-state-setup", 0x01d7, 0x3fb0)

-- Segment 1's relative 0x14E1 routine is the executable's scoreboard/HUD
-- compositor (LEVEL, SCORE, CEREALS, and completion strings).  Probe it after
-- the first gameplay render so a hit gives a stopped frame at the actual HUD
-- call boundary; a timeout is retained as negative evidence below.
local gameplay_hud = optional_ui_checkpoint_after_stop(
    "gameplay-hud-draw", 0x01d7, 0x14e1)
-- Keep the local binding for interactive debugger sessions that inspect the
-- result while stopped; optional_ui_checkpoint_after_stop already records a
-- skipped checkpoint when the call is not reached.
if gameplay_hud ~= nil then dosbox.output.gameplay_hud_hit = true end

-- Resolve the live state-loop boundary and record whether the statically
-- identified exit/completion branches are traversed by this launch.
collect_state_path_after_render()

-- Escape is a safe exit from the running scene.  Capture the return to the
-- menu/input loop, if this build routes it through the same selector loop.
dosbox.wait_frames(60)
dosbox.breakpoint_set(0x01d7, 0x4ace, {once = true})
-- Use the real Escape path; the earlier flag-only injection could leave the
-- gameplay loop running without traversing the menu-return dispatcher.
dosbox.key("KBD_esc", true)
dosbox.wait_frames(2)
dosbox.key("KBD_esc", false)
dosbox.debug_continue()
local returned_menu, return_err = dosbox.wait_for_breakpoint(timeout_ms)
if returned_menu then
    checkpoint("escape-menu-return", returned_menu)
else
    dosbox.output.transition_timeout = {
        name = "escape-menu-return",
        error = return_err or "timeout",
        frame = dosbox.frame(),
    }
end

dosbox.breakpoint_clear()
dosbox.output.transition_checkpoints = checkpoints
dosbox.output.transition_complete = true
