-- Capture the initialized ARE entity dispatch entries in one emulator run.
local entity_types = TRACE_ENTITY_TYPES or {}

local function word(s, index)
    local lo, hi = string.byte(s, index, index + 1)
    return lo | (hi << 8)
end

dosbox.output.awaiting_startup_replay = true
dosbox.wait_frames(350)
dosbox.key("KBD_space", true)
dosbox.wait_frames(4)
dosbox.key("KBD_space", false)

dosbox.breakpoint_set(0x01f7, 0x1e04, {once = true})
local hit, err = dosbox.wait_for_breakpoint(TRACE_TIMEOUT_MS or 30000)
if not hit then error("ARE declaration: " .. (err or "timeout")) end

local cpu = dosbox.cpu_state()
local entries = {}
for index, entity_type in ipairs(entity_types) do
    local slot = 0x81d2 + entity_type * 4
    local raw = dosbox.mem_read("ds", slot, 4)
    entries[index] = {
        type = entity_type,
        slot = slot,
        offset = word(raw, 1),
        object_class = string.byte(raw, 3),
        reserved = string.byte(raw, 4),
        raw_bytes = {
            string.byte(raw, 1), string.byte(raw, 2),
            string.byte(raw, 3), string.byte(raw, 4),
        },
    }
end

dosbox.output.dispatch = {
    data_selector = cpu.ds,
    entries = entries,
}
