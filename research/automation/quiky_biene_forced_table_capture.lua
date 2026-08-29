-- Capture the retail 01F7:0A43 startup table from a live DOSBox runtime.
--
-- The normal level-selector route does not execute the one-time reset caller
-- that owns 0A43.  After the deterministic opening replay reaches a safe
-- instruction, call the retail builder through a temporary far-call patch.
-- The patch is restored as soon as the builder entry is reached; the table
-- itself is read at 0B54, immediately before the retail far return.

local config = TRACE_CONFIG or {}
local timeout_ms = config.timeout_ms or 30000

local function hex(raw)
    return (raw or ""):gsub(".", function(c)
        return string.format("%02x", string.byte(c))
    end)
end

local function wait_hit(label)
    local hit, err = dosbox.wait_for_breakpoint(timeout_ms)
    if not hit then error(label .. ": " .. (err or "timeout")) end
    return hit
end

dosbox.output.awaiting_startup_replay = true
dosbox.wait_frames(350)
dosbox.output.awaiting_startup_replay = nil

local current = dosbox.cpu_state()
dosbox.breakpoint_clear()
dosbox.breakpoint_set(current.cs, current.eip, {once = true})
dosbox.debug_continue()
local barrier = wait_hit("post-start table-call barrier")

local call_cs = barrier.registers.cs
local call_ip = barrier.registers.eip & 0xffff
local original = dosbox.mem_read_selector(call_cs, call_ip, 5) or ""
if #original ~= 5 then error("could not read five-byte barrier instruction") end

dosbox.breakpoint_clear()
dosbox.breakpoint_set(0x01f7, 0x0a43, {once = true, deferred = true})
dosbox.breakpoint_set(0x01f7, 0x0b54, {once = true, deferred = true})
dosbox.mem_write_selector(call_cs, call_ip,
    string.char(0x9a, 0x43, 0x0a, 0xf7, 0x01))
dosbox.debug_continue()

local entry = wait_hit("retail 0A43 entry")
dosbox.mem_write_selector(call_cs, call_ip, original)
dosbox.debug_continue()

local exit = wait_hit("retail 0A43 table completion")
local table_bytes = dosbox.mem_read("ds", 0x7974, 0x800)
dosbox.output.biene_startup_table = {
    schema_version = 1,
    barrier = {
        segment = barrier.segment,
        offset = barrier.offset,
        registers = barrier.registers,
    },
    entry = {
        segment = entry.segment,
        offset = entry.offset,
        registers = entry.registers,
    },
    exit = {
        segment = exit.segment,
        offset = exit.offset,
        registers = exit.registers,
    },
    data_selector = dosbox.cpu_state().ds,
    table_address = "DS:7974",
    table_size = #table_bytes,
    table_hex = hex(table_bytes),
}
