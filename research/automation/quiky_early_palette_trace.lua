-- Capture the first Quiky VGA DAC helper call before the executable starts.
-- The launcher places a DOSBox `pause` command before QUIKY.EXE; deferred
-- breakpoints become valid when the protected-mode segments are loaded.
local timeout_ms = TRACE_TIMEOUT_MS or 30000
local event_limit = TRACE_EVENT_LIMIT or 1
local capture_page_start = TRACE_CAPTURE_PAGE_START or false

local function arm(deferred)
    dosbox.breakpoint_remove(0x0207, 0x0536)
    dosbox.breakpoint_remove(0x0207, 0x05ff)
    dosbox.breakpoint_remove(0x0207, 0x02a5)
    dosbox.breakpoint_remove(0x0207, 0x00b3)
    dosbox.breakpoint_set(0x0207, 0x0536,
                          {once = true, deferred = deferred})
    dosbox.breakpoint_set(0x0207, 0x05ff,
                          {once = true, deferred = deferred})
    dosbox.breakpoint_set(0x0207, 0x02a5,
                          {once = true, deferred = deferred})
    if capture_page_start then
        dosbox.breakpoint_set(0x0207, 0x00b3,
                              {once = true, deferred = deferred})
    end
end

local function wait_hit(timeout)
    return dosbox.wait_for_breakpoint(timeout or timeout_ms)
end

local function bytes_at(selector, offset, count)
    local raw = dosbox.mem_read(selector, offset, count) or ""
    local bytes = {}
    for index = 1, #raw do
        bytes[#bytes + 1] = string.byte(raw, index)
    end
    return bytes
end

local function bytes_hex(raw, limit)
    local bytes = {}
    for index = 1, math.min(#raw, limit or #raw) do
        bytes[#bytes + 1] = string.format("%02x", string.byte(raw, index))
    end
    return table.concat(bytes)
end

local function bytes_list(raw, limit)
    local bytes = {}
    for index = 1, math.min(#raw, limit or #raw) do
        bytes[#bytes + 1] = string.byte(raw, index)
    end
    return bytes
end

local function event_for(hit)
    local registers = hit.registers or dosbox.cpu_state()
    local event = {
        segment = hit.segment,
        offset = hit.offset,
        frame = dosbox.frame(),
        registers = registers,
    }
    if hit.offset == 0x02a5 then
        event.kind = "ramp-entry"
        local sp = (registers.sp or registers.esp or 0) & 0xffff
        local function word(delta)
            return dosbox.mem_read_word("ss", (sp + delta) & 0xffff)
        end
        local source_offset = word(0x06)
        local source_selector = word(0x08)
        event.ramp_args = {
            fade_step = word(0x04),
            source = {offset = source_offset, selector = source_selector},
        }
        if source_selector ~= nil and source_selector ~= 0 then
            local ok, raw = pcall(dosbox.mem_read_selector,
                                  source_selector, source_offset, 0x400)
            if ok and raw then
                event.source_size = #raw
                event.source_bytes = bytes_list(raw, 32)
            else
                event.source_read_error = ok and "empty source" or tostring(raw)
            end
        end
    elseif hit.offset == 0x0536 then
        event.kind = "full-upload"
        event.dac_index = 0
        event.palette_pointer = {
            selector = registers.ds,
            offset = (registers.si or registers.esi or 0) & 0xffff,
        }
        local ok, raw = pcall(dosbox.mem_read_selector,
                              event.palette_pointer.selector,
                              event.palette_pointer.offset, 0x400)
        if ok and raw then
            event.palette_source_size = #raw
            event.palette_source_hex = bytes_hex(raw, 64)
            event.palette_source_bytes = bytes_list(raw, 32)
        else
            event.palette_source_read_error = ok and "empty source" or tostring(raw)
        end
    elseif hit.offset == 0x05ff then
        event.kind = "single-entry"
        local bp = (registers.bp or registers.ebp or 0) & 0xffff
        local function word(delta)
            return dosbox.mem_read_word("ss", (bp + delta) & 0xffff)
        end
        local index_word = word(0x0c)
        local red_word = word(0x0a)
        local green_word = word(0x08)
        local blue_word = word(0x06)
        event.stack_bytes = bytes_at("ss", bp, 0x10)
        event.call_args = {
            index_word = index_word,
            dac_index = (index_word or 0) & 0xff,
            rgb = {
                (red_word or 0) & 0xff,
                (green_word or 0) & 0xff,
                (blue_word or 0) & 0xff,
            },
        }
    elseif hit.offset == 0x00b3 then
        event.kind = "page-start"
        local sp = (registers.sp or registers.esp or 0) & 0xffff
        local start_address = dosbox.mem_read_word("ss", (sp + 0x04) & 0xffff)
        event.page_start = {
            address = start_address,
            low = (start_address or 0) & 0xff,
            high = ((start_address or 0) >> 8) & 0xff,
        }
        event.crtc = {index_port = 0x3d4, data_port = 0x3d5}
        local function ds_word(offset)
            local ok, value = pcall(dosbox.mem_read_word, "ds", offset)
            return ok and value or nil
        end
        event.state = {
            page_a = ds_word(0x817a),
            page_b = ds_word(0x817c),
            page_c = ds_word(0x81a6),
            page_d = ds_word(0x81aa),
        }
    end
    return event
end

dosbox.output.ready_for_release = true
arm(true)
-- Release the DOSBox pause.  The target executable starts only after this
-- key reaches the DOS shell, while the deferred breakpoints remain armed.
dosbox.key("KBD_enter", true)
dosbox.wait_frames(1)
dosbox.key("KBD_enter", false)

local events = {}
local hit, err
-- Deferred API breakpoints are resolved by the debugger's protected-mode
-- descriptor pass. Re-arm without `deferred` while the newly started EXE is
-- running; after each DAC helper entry, stop at its next instruction so the
-- one-shot target can be safely installed for the next palette operation.
local function advance_after_event(event_hit)
    local next_offset = nil
    if event_hit.offset == 0x02a5 then
        next_offset = 0x02a8
    elseif event_hit.offset == 0x0536 then
        next_offset = 0x0539
    elseif event_hit.offset == 0x05ff then
        next_offset = 0x0605
    elseif event_hit.offset == 0x00b3 then
        next_offset = 0x00b6
    end
    if next_offset then
        dosbox.breakpoint_set(0x0207, next_offset, {once = true})
        dosbox.debug_continue()
        local stepped = dosbox.wait_for_breakpoint(1000)
        dosbox.breakpoint_remove(0x0207, next_offset)
        if stepped then return true end
    end
    -- If a protected-mode build does not expose the acknowledgement boundary,
    -- still advance one frame so the next target cannot retrigger the current
    -- instruction indefinitely.
    dosbox.debug_continue()
    dosbox.wait_frames(1)
    return true
end

local function wait_palette_hit()
    local last_err = "timeout"
    for attempt = 1, 120 do
        arm(false)
        dosbox.debug_continue()
        local candidate, candidate_err = wait_hit(250)
        if candidate then return candidate end
        last_err = candidate_err or last_err
    end
    return nil, last_err
end

for index = 1, event_limit do
    hit, err = wait_palette_hit()
    if not hit then
        events[#events + 1] = {
            kind = "timeout", error = err or "timeout",
            frame = dosbox.frame(),
        }
        break
    end
    local event = event_for(hit)
    events[#events + 1] = event
    if not advance_after_event(hit) then break end
end
dosbox.output.palette_events = events
dosbox.output.first_palette_event = events[1] or {
    kind = "timeout", error = "no palette event", frame = dosbox.frame(),
}
dosbox.breakpoint_remove(0x0207, 0x0536)
dosbox.breakpoint_remove(0x0207, 0x05ff)
dosbox.breakpoint_remove(0x0207, 0x02a5)
dosbox.breakpoint_remove(0x0207, 0x00b3)
dosbox.debug_continue()
dosbox.wait_frames(2)
