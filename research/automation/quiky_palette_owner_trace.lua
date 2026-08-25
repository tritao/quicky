-- Capture the pre-menu PCC palette staging/ramp/upload chain.
-- DOSBox starts paused before QUIKY.EXE so deferred protected-mode breakpoints
-- can be installed before the title/menu setup runs.
local timeout_ms = TRACE_TIMEOUT_MS or 30000
local event_limit = TRACE_EVENT_LIMIT or 3
local events = {}

local function hex(raw, limit)
    if raw == nil then return "" end
    local count = math.min(#raw, limit or #raw)
    local out = {}
    for index = 1, count do
        out[#out + 1] = string.format("%02x", string.byte(raw, index))
    end
    return table.concat(out)
end

local function word(selector, offset)
    return dosbox.mem_read_word(selector, offset & 0xffff)
end

local function far_snapshot(offset_address, size)
    local offset = word("ds", offset_address)
    local selector = word("ds", offset_address + 2)
    local result = {pointer = {offset = offset, selector = selector}, size = size}
    if offset ~= nil and selector ~= nil and selector ~= 0 then
        local ok, raw_or_error = pcall(
            dosbox.mem_read_selector, selector, offset, size)
        if ok and raw_or_error then
            result.actual_size = #raw_or_error
            result.hex = hex(raw_or_error)
        else
            result.read_error = ok and "empty buffer" or tostring(raw_or_error)
        end
    end
    return result
end

local function snapshot(hit)
    local event = {
        frame = dosbox.frame(),
        segment = hit.segment,
        offset = hit.offset,
        registers = hit.registers,
    }
    if hit.offset == 0x3020 then
        event.kind = "pcc-setup"
        event.palette_stage = far_snapshot(0x610c, 0x400)
        event.selected_descriptors = {
            menu = word("ds", 0x60bc),
            gamebar = word("ds", 0x60be),
            introbar = word("ds", 0x60c0),
        }
    elseif hit.offset == 0x02a5 then
        event.kind = "palette-ramp"
        local registers = hit.registers or dosbox.cpu_state()
        local sp = (registers.esp or registers.sp or 0) & 0xffff
        local fade_step = word("ss", sp + 4)
        local source_offset = word("ss", sp + 6)
        local source_selector = word("ss", sp + 8)
        event.ramp_args = {
            sp = sp,
            fade_step = fade_step,
            source = {offset = source_offset, selector = source_selector},
        }
        if source_offset ~= nil and source_selector ~= nil and source_selector ~= 0 then
            local ok, raw_or_error = pcall(
                dosbox.mem_read_selector, source_selector, source_offset, 0x400)
            if ok and raw_or_error then
                event.source_size = #raw_or_error
                event.source_hex = hex(raw_or_error)
            else
                event.source_read_error = ok and "empty source" or tostring(raw_or_error)
            end
        end
    elseif hit.offset == 0x0536 then
        event.kind = "full-dac-upload"
        local registers = hit.registers or dosbox.cpu_state()
        local offset = (registers.esi or registers.si or 0) & 0xffff
        event.upload_source = {offset = offset, selector = registers.ds}
        local ok, raw_or_error = pcall(
            dosbox.mem_read_selector, registers.ds, offset, 0x300)
        if ok and raw_or_error then
            event.upload_size = #raw_or_error
            event.upload_hex = hex(raw_or_error)
        else
            event.upload_read_error = ok and "empty upload" or tostring(raw_or_error)
        end
    elseif hit.offset == 0x50c3 then
        event.kind = "menu-palette-callsite"
        event.state = {
            selector = word("ds", 0x85d4),
            transition_state = dosbox.mem_read_byte("ds", 0x85da),
            menu_descriptor = word("ds", 0x60bc),
        }
    elseif hit.offset == 0x0880 then
        -- Title/menu setup copies a prepared 0x400-byte palette table into
        -- DS:60E4 before invoking the common 20-step ramp.  Capture both
        -- far buffers so the pre-menu staging owner can be identified even
        -- when the later 50C3/3020 path is skipped.
        event.kind = "title-menu-palette-stage"
        event.palette_stage = far_snapshot(0x6118, 0x400)
        event.ramp_source = far_snapshot(0x60e4, 0x400)
        event.state = {
            selector_state = word("ds", 0x85d4),
            transition_state = dosbox.mem_read_byte("ds", 0x85da),
        }
    elseif hit.offset == 0x1084 or hit.offset == 0x3310
        or hit.offset == 0x51a7 then
        event.kind = "menu-resource-entry"
        event.entry_offset = hit.offset
        event.state = {
            selector_state = word("ds", 0x85d4),
            transition_state = dosbox.mem_read_byte("ds", 0x85da),
            menu_descriptor = word("ds", 0x60bc),
            palette_source = far_snapshot(0x6118, 0x400),
            palette_destination = far_snapshot(0x60e4, 0x400),
        }
    elseif hit.offset == 0x0470 then
        -- One-shot menu setup helper: its body calls 0047 with INFO, SOUND,
        -- and EXIT before the settled loop starts drawing QUICKY.
        event.kind = "menu-label-setup"
        event.state = {
            selector_state = word("ds", 0x85d4),
            transition_state = dosbox.mem_read_byte("ds", 0x85da),
            menu_descriptor = word("ds", 0x60bc),
        }
    elseif hit.offset == 0x0047 then
        event.kind = "menu-variable-font"
        local registers = hit.registers or dosbox.cpu_state()
        local bp = (registers.bp or registers.ebp or 0) & 0xffff
        local text_offset = word("ss", bp + 4)
        local text_selector = word("ss", bp + 6)
        event.text_pointer = {offset = text_offset, selector = text_selector}
        event.text = ""
        if text_offset ~= nil and text_selector ~= nil and text_selector ~= 0 then
            local ok, raw = pcall(dosbox.mem_read_selector, text_selector,
                                  text_offset, 256)
            if ok and raw then
                local length = string.byte(raw, 1) or 0
                event.text = string.sub(raw, 2, math.min(#raw, length + 1))
            end
        end
    elseif hit.offset == 0x0c11 or hit.offset == 0x0f82
        or hit.offset == 0x10d3 or hit.offset == 0x13b6
        or hit.offset == 0x13e0 then
        event.kind = "menu-setup-callsite"
        event.callsite = string.format("%04x", hit.offset)
        event.state = {
            selector_state = word("ds", 0x85d4),
            transition_state = dosbox.mem_read_byte("ds", 0x85da),
            menu_descriptor = word("ds", 0x60bc),
        }
    end
    return event
end

local function clear_targets()
    dosbox.breakpoint_remove(0x01d7, 0x3020)
    dosbox.breakpoint_remove(0x01d7, 0x50c3)
    dosbox.breakpoint_remove(0x01d7, 0x0880)
    dosbox.breakpoint_remove(0x01d7, 0x1084)
    dosbox.breakpoint_remove(0x01d7, 0x3310)
    dosbox.breakpoint_remove(0x01d7, 0x51a7)
    dosbox.breakpoint_remove(0x01d7, 0x0c4c)
    dosbox.breakpoint_remove(0x01d7, 0x10d6)
    dosbox.breakpoint_remove(0x0207, 0x02a5)
    dosbox.breakpoint_remove(0x0207, 0x0536)
    dosbox.breakpoint_remove(0x01d7, 0x0470)
    dosbox.breakpoint_remove(0x01d7, 0x0047)
    dosbox.breakpoint_remove(0x01d7, 0x0c11)
    dosbox.breakpoint_remove(0x01d7, 0x0f82)
    dosbox.breakpoint_remove(0x01d7, 0x10d3)
    dosbox.breakpoint_remove(0x01d7, 0x13b6)
    dosbox.breakpoint_remove(0x01d7, 0x13e0)
end

local function arm_targets(deferred)
    clear_targets()
    dosbox.breakpoint_set(0x01d7, 0x3020,
                          {once = true, deferred = deferred})
    dosbox.breakpoint_set(0x01d7, 0x50c3,
                          {once = true, deferred = deferred})
    dosbox.breakpoint_set(0x01d7, 0x0880,
                          {once = true, deferred = deferred})
    dosbox.breakpoint_set(0x01d7, 0x1084,
                          {once = true, deferred = deferred})
    dosbox.breakpoint_set(0x01d7, 0x3310,
                          {once = true, deferred = deferred})
    dosbox.breakpoint_set(0x01d7, 0x51a7,
                          {once = true, deferred = deferred})
    dosbox.breakpoint_set(0x01d7, 0x0c4c,
                          {once = true, deferred = deferred})
    dosbox.breakpoint_set(0x01d7, 0x10d6,
                          {once = true, deferred = deferred})
    dosbox.breakpoint_set(0x01d7, 0x0470,
                          {once = true, deferred = deferred})
    dosbox.breakpoint_set(0x01d7, 0x0047,
                          {once = true, deferred = deferred})
    for _, offset in ipairs({0x0c11, 0x0f82, 0x10d3, 0x13b6, 0x13e0}) do
        dosbox.breakpoint_set(0x01d7, offset,
                              {once = true, deferred = deferred})
    end
    dosbox.breakpoint_set(0x0207, 0x02a5,
                          {once = true, deferred = deferred})
    dosbox.breakpoint_set(0x0207, 0x0536,
                          {once = true, deferred = deferred})
end

dosbox.output.ready_for_release = true
arm_targets(true)
dosbox.key("KBD_enter", true)
dosbox.wait_frames(1)
dosbox.key("KBD_enter", false)
-- Once the protected-mode image has been entered, replace deferred module
-- breakpoints with ordinary ones as well.  This makes the early-title probe
-- robust against a DOSBox build that does not rebind every deferred NE
-- breakpoint after the first module-generation change.
arm_targets(false)

for index = 1, event_limit do
    dosbox.debug_continue()
    local hit, err = dosbox.wait_for_breakpoint(timeout_ms)
    if not hit then
        events[#events + 1] = {
            kind = "timeout", frame = dosbox.frame(), error = err or "timeout",
        }
        break
    end
    events[#events + 1] = snapshot(hit)
    -- The setup/ramp routines are one-shot for this probe.  Remove every
    -- target after each stop so the debugger cannot retrigger the same entry
    -- while the current call is being acknowledged.
    clear_targets()
    if hit.offset ~= 0x0536 and index < event_limit then
        arm_targets(false)
    end
end

clear_targets()
dosbox.output.palette_owner_events = events
dosbox.output.palette_owner_complete = true
