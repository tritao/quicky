-- Focused construction/MAP-loader trace.
--
-- The host runner supplies TRACE_SELECT_LEVEL and replays the normal startup
-- recording when this script asks for it.  The trace deliberately stops at
-- loader/initializer instructions instead of scanning a running MAP window;
-- this keeps protected-mode selector reads and lifecycle ordering explicit.

local timeout_ms = TRACE_TIMEOUT_MS or 30000
local select_level = TRACE_SELECT_LEVEL or "W1L1"
local selector_frames = TRACE_SELECTOR_FRAMES or 80
local trace_secondary = TRACE_SECONDARY or false
local trace_map_writer = TRACE_MAP_WRITER or false

local selector_indices = {
    W1L1 = 0, W1L2 = 1, W1L3 = 2,
    W2L1 = 3, W2L2 = 4, W2L3 = 5,
    W3L1 = 6, W3L2 = 7, W3L3 = 8,
    W4L1 = 9, W4L2 = 10, W4L3 = 11,
    W5L1 = 12, W5L2 = 13, W5L3 = 14,
    W1L4 = 15, W2L4 = 16, W3L4 = 17,
    W4L4 = 18, W5L4 = 19,
}

local initializer = {
    W1L1 = {entry = 0x1734, ret = 0x19e3, world = 1},
    W1L2 = {entry = 0x1734, ret = 0x19e3, world = 1},
    W1L3 = {entry = 0x1734, ret = 0x19e3, world = 1},
    W1L4 = {entry = 0x1734, ret = 0x19e3, world = 1},
    W2L1 = {entry = 0x19e4, ret = 0x1bf0, world = 2},
    W2L2 = {entry = 0x19e4, ret = 0x1bf0, world = 2},
    W2L3 = {entry = 0x19e4, ret = 0x1bf0, world = 2},
    W2L4 = {entry = 0x19e4, ret = 0x1bf0, world = 2},
    W3L1 = {entry = 0x1bf1, ret = 0x28ec, world = 3},
    W3L2 = {entry = 0x1bf1, ret = 0x28ec, world = 3},
    W3L3 = {entry = 0x1bf1, ret = 0x28ec, world = 3},
    W3L4 = {entry = 0x1bf1, ret = 0x28ec, world = 3},
    W4L1 = {entry = 0x28ed, ret = 0x2d9e, world = 4},
    W4L2 = {entry = 0x28ed, ret = 0x2d9e, world = 4},
    W4L3 = {entry = 0x28ed, ret = 0x2d9e, world = 4},
    W4L4 = {entry = 0x28ed, ret = 0x2d9e, world = 4},
    W5L1 = {entry = 0x2d9f, ret = 0x301f, world = 5},
    W5L2 = {entry = 0x2d9f, ret = 0x301f, world = 5},
    W5L3 = {entry = 0x2d9f, ret = 0x301f, world = 5},
    W5L4 = {entry = 0x2d9f, ret = 0x301f, world = 5},
}

local function word(s, index)
    local lo, hi = string.byte(s, index, index + 1)
    return lo | (hi << 8)
end

local function hex(s)
    return (s:gsub(".", function(c) return string.format("%02x", string.byte(c)) end))
end

local function wait_hit(label)
    local hit, err = dosbox.wait_for_breakpoint(timeout_ms)
    if not hit then error(label .. ": " .. (err or "breakpoint wait failed")) end
    return hit
end

local function ds_word(offset)
    return dosbox.mem_read_word("ds", offset)
end

local function map_state()
    return {
        base = ds_word(0x657a),
        selector = ds_word(0x657c),
        row_stride = ds_word(0x657e),
        height = ds_word(0x6580),
    }
end

local function map_word(map, offset)
    local raw = dosbox.mem_read_selector(map.selector, map.base + offset, 2)
    return word(raw, 1)
end

local function map_row(map, count)
    local result = {}
    for index = 0, count - 1 do
        result[index + 1] = map_word(map, index * 2)
    end
    return result
end

local function descriptor_state()
    local base = ds_word(0x6582)
    local selector = ds_word(0x6584)
    local stride = ds_word(0x30d4)
    local entries = {}
    for tile_id = 0, 0x1ff do
        local raw = dosbox.mem_read_selector(selector, base + tile_id * stride, 4)
        entries[tile_id + 1] = {
            tile_id = tile_id,
            offset = base + tile_id * stride,
            tile_index = word(raw, 1),
            flags = word(raw, 3),
        }
    end
    return {
        base = base,
        selector = selector,
        stride = stride,
        count = #entries,
        entries = entries,
    }
end

assert(selector_indices[select_level] ~= nil, "unsupported level selector target")
assert(not (trace_secondary and trace_map_writer),
       "secondary and MAP-writer probes are mutually exclusive")
local init = initializer[select_level]
assert(init ~= nil, "unsupported initializer target")
-- Level selection enters the primary loader for the normal MAP payload.  The
-- secondary routine is a separate reload/stream path (documented statically)
-- and is not assumed merely from the L3 index.
local secondary_loader = trace_secondary
local loader_entry_offset = secondary_loader and 0x3861 or 0x365b
local loader_alt_entry_offset = secondary_loader and 0x3862 or 0x365c
local loader_call_offset = secondary_loader and 0x4bf1 or 0x4009
local mutation_read_offset = secondary_loader and 0x394c or 0x37ad
local mutation_or_offset = secondary_loader and 0x394f or 0x37b2
local mutation_write_after_offset = secondary_loader and 0x3963 or 0x37c3
local mutation_done_offset = secondary_loader and 0x396d or 0x37cb

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
dosbox.wait_frames(selector_frames)
dosbox.breakpoint_set(0x01d7, 0x4ace, {once = true})
wait_hit("selector input wait")
dosbox.mem_write("ds", 0x85d4,
                 string.char(selector_indices[select_level] & 0xff,
                             selector_indices[select_level] >> 8))
dosbox.breakpoint_set(0x01d7, 0x4b18, {once = true})
-- Arm the loader call and entry before releasing the selector dispatch.  The
-- call can begin before the 4b18 breakpoint is observed on some recordings.
dosbox.breakpoint_set(0x01d7, loader_call_offset, {once = true})
dosbox.breakpoint_set(0x01d7, loader_entry_offset, {once = true})
dosbox.breakpoint_set(0x01d7, loader_alt_entry_offset, {once = true})
dosbox.mem_write("ds", 0x88bc, "\x20\x00")
dosbox.debug_continue()
local launch = wait_hit("selector Space dispatch")

-- Pass resource lookups through until the selected MAP loader is reached.
-- W1L3/W2L3/etc. enter the secondary loader only after several resources;
-- stopping each lookup keeps that lifecycle deterministic.
local loader = nil
local current = launch
for attempt = 1, 128 do
    if current.segment == 0x01d7 and
       (current.offset == loader_entry_offset or current.offset == loader_alt_entry_offset) then
        loader = current
        break
    elseif current.segment == 0x01d7 and current.offset == loader_call_offset then
        dosbox.breakpoint_set(0x01d7, loader_entry_offset, {once = true})
        dosbox.debug_continue()
        current = wait_hit("MAP loader after call site")
    elseif current.segment == 0x0207 and current.offset == 0x18c7 then
        local stack = dosbox.mem_read("ss", current.registers.esp, 4)
        local return_offset = word(stack, 1)
        local return_segment = word(stack, 3)
        dosbox.breakpoint_set(return_segment, return_offset, {once = true})
        dosbox.debug_continue()
        wait_hit("resource lookup return")
        dosbox.breakpoint_set(0x0207, 0x18c7, {once = true})
        dosbox.debug_continue()
        current = wait_hit("next resource or MAP loader")
    elseif current.segment == 0x01d7 and current.offset == 0x4b18 then
        dosbox.breakpoint_set(0x0207, 0x18c7, {once = true})
        dosbox.debug_continue()
        current = wait_hit("first resource or MAP loader")
    else
        error("unexpected selector dispatch breakpoint")
    end
end
if loader == nil then error("MAP loader was not reached") end
assert(loader.segment == 0x01d7 and
       (loader.offset == loader_entry_offset or loader.offset == loader_alt_entry_offset),
       "unexpected MAP loader breakpoint")
-- Clear the alternate entry/call-site breakpoints before stepping into the
-- mutation loop; otherwise the 365c instruction can be mistaken for 37ad.
dosbox.breakpoint_remove(0x01d7, loader_call_offset)
dosbox.breakpoint_remove(0x01d7, loader_entry_offset)
dosbox.breakpoint_remove(0x01d7, loader_alt_entry_offset)
dosbox.breakpoint_remove(0x0207, 0x18c7)

-- Catch the first post-copy OR instruction.  The loader publishes the new
-- far MAP pointer after entry, so refresh it at the mutation breakpoint.
local map = nil
dosbox.breakpoint_set(0x01d7, mutation_read_offset, {once = true})
dosbox.debug_continue()
local mutation_before_hit = wait_hit("primary MAP OR before")
map = map_state()
dosbox.output.debug_map = map
local mutation_before_regs = mutation_before_hit.registers
dosbox.output.debug_mutation_before_regs = mutation_before_regs
local mutation_before = map_word(map, mutation_before_regs.edi - 1)
dosbox.breakpoint_set(0x01d7, mutation_or_offset, {once = true})
dosbox.debug_continue()
local mutation_or_hit = wait_hit("primary MAP OR after")
dosbox.breakpoint_set(0x01d7, mutation_write_after_offset, {once = true})
dosbox.debug_continue()
local mutation_after_hit = wait_hit("primary MAP OR write after")
local mutation_after_regs = mutation_after_hit.registers
local mutation_after = map_word(map, mutation_after_regs.edi - 1)
dosbox.breakpoint_set(0x01d7, mutation_done_offset, {once = true})
dosbox.debug_continue()
local mutation_done = wait_hit("primary MAP first-row mutation end")
local first_row = map_row(map, map.row_stride // 2)

-- Descriptor table publication happens before the world dispatch.  Capture
-- the table at dispatch, at initializer entry, and immediately before RET.
dosbox.breakpoint_set(0x01d7, 0x3808, {once = true})
dosbox.debug_continue()
local dispatch = wait_hit("descriptor world dispatch")
local published = {
    base = ds_word(0x6582), selector = ds_word(0x6584), stride = ds_word(0x30d4),
}
dosbox.breakpoint_set(0x01d7, init.entry, {once = true})
dosbox.debug_continue()
local initializer_entry = wait_hit("descriptor initializer entry")
local before_table = descriptor_state()
dosbox.breakpoint_set(0x01d7, init.ret, {once = true})
dosbox.debug_continue()
local initializer_return = wait_hit("descriptor initializer return")
local after_table = descriptor_state()

local secondary_trace = nil
local map_writer_trace = nil
if trace_map_writer then
    -- Inject a debugger-only call to 01f7:5c9d from the stopped initializer
    -- return. The helper is a far-return routine, so the trampoline performs
    -- a far call and then returns to the original same-segment frame. Inputs
    -- are deliberately nontrivial coordinates/value so the address formula
    -- is independently checkable in the artifact.
    local trampoline = 0x51d0
    local return_patch = 0x19e3
    local y = 0x0123
    local x = 0x0045
    local value = 0xa55a
    local trampoline_call_disp = 0x5c9d - (trampoline + 3 + 9)
    local return_call_disp = trampoline - (return_patch + 3)
    local controlled_patch = {
        coordinates = {x = x, y = y, value = value},
        trampoline = {segment = 0x01d7, offset = trampoline},
        original_trampoline = hex(dosbox.mem_read("cs", trampoline, 16)),
        original_return_bytes = hex(dosbox.mem_read("cs", return_patch, 3)),
    }
    -- B8/BB/B9 load AX/BX/CX, then 9A performs the far helper call.
    dosbox.mem_write("cs", trampoline, string.char(
        0xb8, y & 0xff, (y >> 8) & 0xff,
        0xbb, x & 0xff, (x >> 8) & 0xff,
        0xb9, value & 0xff, (value >> 8) & 0xff,
        0x9a, 0x9d, 0x5c, 0xf7, 0x01,
        0xc3))
    dosbox.mem_write("cs", return_patch, string.char(
        0xe8, return_call_disp & 0xff, (return_call_disp >> 8) & 0xff))
    controlled_patch.injected_trampoline = hex(
        dosbox.mem_read("cs", trampoline, 16))
    controlled_patch.injected_return_call = hex(
        dosbox.mem_read("cs", return_patch, 3))
    dosbox.output.map_writer_controlled_patch = controlled_patch
    dosbox.breakpoint_set(0x01f7, 0x5c9d, {once = true})
    dosbox.debug_continue()
    local helper_entry = wait_hit("MAP writer helper entry")
    local entry_regs = helper_entry.registers
    local map_writer_map = map_state()
    local expected_offset = (y // 16) * map_writer_map.row_stride
        + ((x // 8) & 0xfffe)
    dosbox.breakpoint_set(0x01f7, 0x5cbe, {once = true})
    dosbox.debug_continue()
    local write_hit = wait_hit("MAP writer helper write")
    local write_regs = write_hit.registers
    local write_selector = write_regs.fs
    local write_offset = write_regs.ebx & 0xffff
    local before_write = word(
        dosbox.mem_read_selector(write_selector, write_offset, 2), 1)
    dosbox.breakpoint_set(0x01f7, 0x5cc1, {once = true})
    dosbox.debug_continue()
    local after_write_hit = wait_hit("MAP writer helper return")
    local after_write = word(
        dosbox.mem_read_selector(write_selector, write_offset, 2), 1)
    map_writer_trace = {
        entry = helper_entry,
        write = write_hit,
        return_hit = after_write_hit,
        map = map_writer_map,
        expected_offset = map_writer_map.base + expected_offset,
        actual_offset = write_offset,
        selector = write_selector,
        input_registers = entry_regs,
        write_registers = write_regs,
        before_word = before_write,
        after_word = after_write,
        value = value,
        offset_match = write_offset == map_writer_map.base + expected_offset,
    }
end

if trace_secondary then
    -- Normal continuation remains in the transition wait state. To isolate
    -- the loader itself, inject a debugger-only near-call trampoline at the
    -- initializer RET. The loader uses the already-published MAP globals;
    -- the trampoline calls it with the same-segment calling convention and
    -- returns to the stopped initializer frame. This is a controlled call,
    -- not evidence that normal gameplay reaches the routine.
    local trampoline = 0x51e0
    local trampoline_call_disp = 0x3861 - (trampoline + 3)
    local return_patch = 0x19e3
    local return_call_disp = trampoline - (return_patch + 3)
    local controlled_patch = {
        level_index = ds_word(0x85d4),
        trampoline = {segment = 0x01d7, offset = trampoline},
        original_trampoline = hex(dosbox.mem_read("cs", trampoline, 4)),
        original_return_bytes = hex(dosbox.mem_read("cs", return_patch, 3)),
    }
    dosbox.mem_write("cs", trampoline,
                     string.char(0xe8, trampoline_call_disp & 0xff,
                                 (trampoline_call_disp >> 8) & 0xff, 0xc3))
    dosbox.mem_write("cs", return_patch,
                     string.char(0xe8, return_call_disp & 0xff,
                                 (return_call_disp >> 8) & 0xff))
    controlled_patch.injected_trampoline = hex(
        dosbox.mem_read("cs", trampoline, 4))
    controlled_patch.injected_return_call = hex(
        dosbox.mem_read("cs", return_patch, 3))
    dosbox.output.secondary_controlled_patch = controlled_patch

    dosbox.breakpoint_set(0x01d7, 0x3861, {once = true})
    dosbox.breakpoint_set(0x01d7, 0x3862, {once = true})
    dosbox.debug_continue()
    local secondary_first = wait_hit("secondary MAP loader call or entry")
    local secondary_call = secondary_first
    if secondary_first.offset == 0x3861 or secondary_first.offset == 0x3862 then
        secondary_call = nil
    else
        dosbox.breakpoint_set(0x01d7, 0x3861, {once = true})
        dosbox.breakpoint_set(0x01d7, 0x3862, {once = true})
        dosbox.debug_continue()
        secondary_first = wait_hit("secondary MAP loader entry")
    end
    assert(secondary_first.segment == 0x01d7 and
           (secondary_first.offset == 0x3861 or secondary_first.offset == 0x3862),
           "unexpected secondary MAP breakpoint")
    dosbox.breakpoint_remove(0x01d7, 0x3861)
    dosbox.breakpoint_remove(0x01d7, 0x3862)

    local secondary_map = nil
    dosbox.breakpoint_set(0x01d7, 0x394c, {once = true})
    dosbox.debug_continue()
    local secondary_before_hit = wait_hit("secondary MAP OR before")
    secondary_map = map_state()
    local secondary_before_regs = secondary_before_hit.registers
    local secondary_before = map_word(secondary_map, secondary_before_regs.edi - 1)
    dosbox.breakpoint_set(0x01d7, 0x394f, {once = true})
    dosbox.debug_continue()
    local secondary_or_hit = wait_hit("secondary MAP OR after")
    dosbox.breakpoint_set(0x01d7, 0x3963, {once = true})
    dosbox.debug_continue()
    local secondary_after_hit = wait_hit("secondary MAP OR write after")
    local secondary_after_regs = secondary_after_hit.registers
    local secondary_after = map_word(secondary_map, secondary_after_regs.edi - 1)
    dosbox.breakpoint_set(0x01d7, 0x396d, {once = true})
    dosbox.debug_continue()
    local secondary_done = wait_hit("secondary MAP first-row mutation end")
    secondary_trace = {
        call = secondary_call,
        entry = secondary_first,
        map = secondary_map,
        mutation = {
            offset = secondary_before_regs.edi - 1,
            before_word = secondary_before,
            after_word = secondary_after,
            delta = secondary_after - secondary_before,
            or_registers = secondary_or_hit.registers,
            before_registers = secondary_before_regs,
            after_registers = secondary_after_regs,
        },
        done = secondary_done,
        controlled_patch = controlled_patch,
    }
end

dosbox.output.result = {
    schema = "quiky-runtime-descriptor-construction-trace-v1",
    level = select_level,
    world = init.world,
    checkpoints = {
        cheat = cheat,
        launch = launch,
        loader = loader,
        mutation_before = mutation_before_hit,
        mutation_or = mutation_or_hit,
        mutation_after = mutation_after_hit,
        mutation_done = mutation_done,
        dispatch = dispatch,
        initializer_entry = initializer_entry,
        initializer_return = initializer_return,
    },
    map = map,
    mutation = {
        offset = mutation_before_regs.edi - 1,
        before_word = mutation_before,
        after_word = mutation_after,
        delta = mutation_after - mutation_before,
        or_registers = mutation_or_hit.registers,
        before_registers = mutation_before_regs,
        after_registers = mutation_after_regs,
        first_row_after = first_row,
    },
    published = published,
    descriptor_before = before_table,
    descriptor_after = after_table,
    secondary = secondary_trace,
    map_writer = map_writer_trace,
}
