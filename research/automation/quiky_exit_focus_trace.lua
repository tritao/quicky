-- Follow the live exit/completion flag writers while the player is driven
-- through a scene.  This is deliberately a narrow breakpoint trace: it does
-- not patch gameplay state, so any hit is a real object/update path.
local timeout_ms = TRACE_TIMEOUT_MS or 30000
local event_limit = TRACE_EVENT_LIMIT or 32
local hold_frames = TRACE_HOLD_FRAMES or 12000
local settle_frames = TRACE_SETTLE_FRAMES or 240
local force_player_x = TRACE_FORCE_PLAYER_X
local force_player_y = TRACE_FORCE_PLAYER_Y
local pretrigger_frames = TRACE_PRETRIGGER_FRAMES or 0
local post_force_player_x = TRACE_POST_FORCE_PLAYER_X
local post_force_player_y = TRACE_POST_FORCE_PLAYER_Y
local capture_frames = TRACE_CAPTURE_FRAMES or false
local capture_names_raw = TRACE_CAPTURE_NAMES or "completion-hud-call,completion-check,completion-branch,transition-setup"
local capture_ack_delay_frames = TRACE_CAPTURE_ACK_DELAY_FRAMES or 4
local post_transition_key = TRACE_POST_TRANSITION_KEY or ""
local post_transition_key_frames = TRACE_POST_TRANSITION_KEY_FRAMES or 8
local palette_loop_trace = TRACE_PALETTE_LOOP ~= false
local score_only = TRACE_SCORE_ONLY or false
local events = {}
local seen_targets = {}
local armed_targets = {}
local forced_player = nil
local capture_names = {}
local post_setup_armed = false
local post_cleanup_armed = false
local pending_dynamic_returns = {}
local post_transition_key_active = false
local input_seed_enabled = false
local palette_loop_enabled = false
local post_cleanup_render_hits = 0
for name in string.gmatch(capture_names_raw, "[^,]+") do
    capture_names[name] = true
end

local function wait_hit(label)
    local hit, err = dosbox.wait_for_breakpoint(timeout_ms)
    if not hit then error(label .. ": " .. (err or "timeout")) end
    return hit
end

local function word(s, index)
    local lo, hi = string.byte(s, index, index + 1)
    return lo | (hi << 8)
end

local function little_dword(value)
    return string.char(value & 0xff, (value >> 8) & 0xff,
                       (value >> 16) & 0xff, (value >> 24) & 0xff)
end

local function hex(s)
    return (s:gsub(".", function(c)
        return string.format("%02x", string.byte(c))
    end))
end

local function object_snapshot(hit)
    local registers = hit.registers or {}
    local selector = registers.es
    local offset = (registers.edi or 0) & 0xffff
    if selector == nil then return nil end
    local ok, raw_or_error = pcall(
        dosbox.mem_read_selector, selector, offset, 0x40
    )
    if not ok or not raw_or_error or #raw_or_error < 0x40 then
        return {selector = selector, offset = offset,
                read_error = ok and "short object state" or tostring(raw_or_error)}
    end
    local raw = raw_or_error
    local x_fixed = word(raw, 3) | (word(raw, 5) << 16)
    local y_fixed = word(raw, 7) | (word(raw, 9) << 16)
    return {
        selector = selector, offset = offset, state_hex = hex(raw),
        x = x_fixed >> 16, y = y_fixed >> 16,
        callback = word(raw, 0x18 + 1), sprite_slot = word(raw, 0x12 + 1),
        kind = word(raw, 0x14 + 1), state_field = word(raw, 0x2e + 1),
        update_state = word(raw, 0x32 + 1),
    }
end

local function far_buffer_snapshot(offset_address, size)
    local offset = dosbox.mem_read_word("ds", offset_address)
    local selector = dosbox.mem_read_word("ds", offset_address + 2)
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

local function globals(hit)
    return {
        goal_mask = dosbox.mem_read_word("ds", 0x60d8),
        exit_flag = dosbox.mem_read_word("ds", 0x89e6),
        pending = dosbox.mem_read_word("ds", 0x89ec),
        done = dosbox.mem_read_word("ds", 0x89e0),
        selector_state = dosbox.mem_read_word("ds", 0x85d4),
        transition_state = dosbox.mem_read_byte("ds", 0x85da),
        level_index = dosbox.mem_read_word("ds", 0x60b2),
        score_low = dosbox.mem_read_word("ds", 0x881c),
        score_high = dosbox.mem_read_word("ds", 0x881e),
        cereal_remaining = dosbox.mem_read_word("ds", 0x880c),
        map_x = dosbox.mem_read_word("ds", 0x81c0),
        map_y = dosbox.mem_read_word("ds", 0x81c4),
    }
end

local targets = {
    {segment = 0x01f7, offset = 0x4996, name = "exit-flag-writer-4996"},
    {segment = 0x01f7, offset = 0x4aac, name = "exit-flag-writer-4aac"},
    {segment = 0x01f7, offset = 0x92a9, name = "exit-flag-writer-92a9"},
    {segment = 0x01d7, offset = 0x4cfc, name = "transition-stage-gate"},
    {segment = 0x01d7, offset = 0x4d03, name = "transition-stage-dispatch"},
    -- 4601 is the hot outer loop.  It was useful as a launch checkpoint,
    -- but re-arming it with the writer set starves the one-shot writer
    -- breakpoints before a long right-input run can reach a hazard.
    {segment = 0x01d7, offset = 0x4ea0, name = "exit-flag-gate"},
    {segment = 0x01d7, offset = 0x4f0d, name = "completion-hud-call"},
    {segment = 0x01d7, offset = 0x5010, name = "resource-reload-gate"},
    {segment = 0x01d7, offset = 0x504f, name = "transition-cleanup"},
    {segment = 0x01d7, offset = 0x1669, name = "completion-check"},
    {segment = 0x01d7, offset = 0x16c6, name = "completion-branch"},
    {segment = 0x01d7, offset = 0x1709, name = "transition-setup"},
    -- The level setup path selects GAMEBAR/INTROBAR and calls the PCC
    -- compositor through 4486/4495 -> 0207:0B6C.  Keep these armed during
    -- ordinary play as well as after a completed-level reload; otherwise a
    -- one-shot setup call can be hidden by the transition checkpoints.
    {segment = 0x01d7, offset = 0x3fb0, name = "gamebar-state-setup"},
    {segment = 0x01d7, offset = 0x4486, name = "gamebar-blit-call-4486"},
    {segment = 0x01d7, offset = 0x4495, name = "introbar-blit-call-4495"},
    {segment = 0x0207, offset = 0x0b6c, name = "pcc-blit-helper-0b6c"},
    {segment = 0x0207, offset = 0x0bef, name = "pcc-blit-return-0bef"},
    {segment = 0x01e7, offset = 0x0d18, name = "transition-effect-call", post_setup = true},
    {segment = 0x01e7, offset = 0x0caa, name = "transition-effect-finalize", post_setup = true},
    {segment = 0x0207, offset = 0x022a, name = "transition-fade-helper", post_setup = true},
    {segment = 0x0207, offset = 0x05ff, name = "transition-dac-write", post_setup = true},
    {segment = 0x0207, offset = 0x0536, name = "transition-full-palette", post_setup = true},
    {segment = 0x0207, offset = 0x0539, name = "transition-full-palette-loop", post_setup = true},
    {segment = 0x0207, offset = 0x18c7, name = "transition-resource-lookup", post_setup = true},
    -- 50C3 is the narrow caller for the PCC palette setup.  Keep the
    -- callsite armed from launch through reload: unlike the per-frame menu
    -- text path, this branch is state-specific and may occur before the
    -- transition cleanup checkpoint arms the ordinary post-cleanup set.
    {segment = 0x01d7, offset = 0x50c3, name = "pcc-palette-callsite"},
    {segment = 0x0207, offset = 0x02a5, name = "pcc-palette-ramp"},
    {segment = 0x01f7, offset = 0x0002, name = "transition-object-reset", post_setup = true},
    {segment = 0x01d7, offset = 0x48e6, name = "post-cleanup-render-input", post_cleanup = true},
    {segment = 0x01d7, offset = 0x491d, name = "post-cleanup-input-wait", post_cleanup = true},
    {segment = 0x01d7, offset = 0x4ace, name = "post-cleanup-selector-input", post_cleanup = true},
    {segment = 0x01d7, offset = 0x4b18, name = "post-cleanup-selector-dispatch", post_cleanup = true},
    {segment = 0x01d7, offset = 0x313d, name = "post-cleanup-ui-dispatch", post_cleanup = true},
    {segment = 0x01d7, offset = 0x3020, name = "pcc-palette-pcc-copy"},
    {segment = 0x01f7, offset = 0x35c7, name = "post-cleanup-render", post_cleanup = true},
    {segment = 0x01d7, offset = 0x01f0, name = "post-transition-input-wait-1", post_setup = true},
    {segment = 0x01d7, offset = 0x01d6, name = "post-transition-input-wait-2", post_setup = true},
    {segment = 0x01d7, offset = 0x01ac, name = "post-transition-input-poll", post_setup = true},
    {segment = 0x01d7, offset = 0x01bd, name = "post-transition-input-seed", post_setup = true},
    {segment = 0x01d7, offset = 0x01d1, name = "post-transition-input-result", post_setup = true},
}

-- The target breakpoints are useful state checkpoints, but the capture is
-- acknowledged at the first instruction after each target so the host sees a
-- stable stopped page and the guest cannot accidentally execute the next
-- stage before the PNG is read.
local capture_barriers = {
    ["transition-stage-gate"] = 0x4d01,
    ["transition-stage-dispatch"] = 0x4d06,
    ["exit-flag-gate"] = 0x4ea5,
    ["completion-hud-call"] = 0x4f10,
    ["completion-check"] = 0x166b,
    ["completion-branch"] = 0x16c8,
    ["transition-setup"] = 0x170c,
    ["pcc-blit-return-0bef"] = 0x0bf2,
}

local function arm_targets()
    for _, target in ipairs(targets) do
        local relevant = not score_only or
            target.name == "exit-flag-writer-92a9" or
            target.name == "completion-check" or
            target.name == "completion-branch" or
            target.name == "transition-setup"
        if relevant then
            local key = string.format("%04x:%04x", target.segment, target.offset)
            local seed_target = target.name == "post-transition-input-seed"
            local palette_loop_target = target.name == "transition-full-palette-loop"
            if not seen_targets[key] and (not target.post_setup or post_setup_armed)
                and (not target.post_cleanup or post_cleanup_armed)
                and (not seed_target or input_seed_enabled)
                and (not palette_loop_target or
                     (palette_loop_trace and palette_loop_enabled)) then
                armed_targets[key] = dosbox.breakpoint_set(
                    target.segment, target.offset, {once = true})
            end
        end
    end
end

local function capture_checkpoint(event, hit)
    if not capture_frames or not capture_names[event.name] then return end
    local barrier_segment = hit.segment
    local barrier = capture_barriers[event.name]
    -- 0BEF is the retf itself, after LEAVE has restored SP to the caller's
    -- far-return frame.  A fixed 0BF2 barrier can consume the next renderer
    -- setup call before the event loop sees it; use the actual caller return.
    if event.name == "pcc-blit-return-0bef" then
        local sp = (hit.registers.esp or hit.registers.sp or 0) & 0xffff
        barrier = dosbox.mem_read_word("ss", sp)
        barrier_segment = dosbox.mem_read_word("ss", (sp + 2) & 0xffff)
    end
    -- The post-cleanup renderer is a near-call entry whose drawing work
    -- completes before its caller resumes.  Its byte+1 is not a stable
    -- acknowledgement point, so use the saved near return address instead.
    if barrier == nil and (event.name == "post-cleanup-render"
        or event.name == "post-cleanup-render-input") then
        local sp = (hit.registers.esp or hit.registers.sp or 0) & 0xffff
        barrier = dosbox.mem_read_word("ss", sp)
    end
    barrier = barrier or ((hit.offset + 1) & 0xffff)
    dosbox.breakpoint_set(barrier_segment, barrier, {once = true})
    event.capture_barrier = {segment = barrier_segment, offset = barrier}
    dosbox.output.exit_focus_checkpoint = event
    local stepped, err = dosbox.wait_for_breakpoint(timeout_ms)
    if not stepped then error("capture acknowledgement: " .. (err or "timeout")) end
    event.capture_frame = dosbox.frame()
    dosbox.wait_frames(capture_ack_delay_frames)
    dosbox.output.exit_focus_checkpoint = nil
end

local function helper_snapshot(event, hit)
    local registers = hit.registers or {}
    if hit.segment == 0x0207 and hit.offset == 0x0b6c then
        -- 0B6C has three Pascal words: destination x/y followed by the PCC
        -- descriptor far pointer.  It temporarily zeros the renderer page
        -- globals before 099C writes nonzero PCC pixels through 0944.
        -- 0B6C is the first byte of the far-call callee, before its
        -- push-bp/mov-bp prologue.  Read the return frame relative to SP.
        local sp = (registers.esp or registers.sp or 0) & 0xffff
        local function stack_word(delta)
            return dosbox.mem_read_word("ss", (sp + delta) & 0xffff)
        end
        local descriptor_id = stack_word(0x08)
        local descriptor_offset = nil
        local descriptor_selector = nil
        local descriptor_hex = ""
        if descriptor_id ~= nil then
            -- 099C indexes the descriptor table at DS:5196 + id*0x16 and
            -- obtains the PCC's far pixel pointer from its first four bytes.
            local descriptor_table = 0x5196 + descriptor_id * 0x16
            local ok, raw = pcall(dosbox.mem_read, "ds", descriptor_table, 0x16)
            if ok and raw and #raw >= 4 then
                descriptor_hex = hex(raw)
                descriptor_offset = string.byte(raw, 1) |
                    (string.byte(raw, 2) << 8)
                descriptor_selector = string.byte(raw, 3) |
                    (string.byte(raw, 4) << 8)
            end
        end
        event.pcc_call = {
            sp = sp,
            x = stack_word(0x04),
            y = stack_word(0x06),
            descriptor_id = descriptor_id,
            descriptor = {selector = descriptor_selector, offset = descriptor_offset,
                          hex = descriptor_hex},
            return_address = {segment = stack_word(0x02), offset = stack_word(0x00)},
            page = {
                current = dosbox.mem_read_word("ds", 0x817a),
                page_x = dosbox.mem_read_word("ds", 0x81a6),
                page_y = dosbox.mem_read_word("ds", 0x81aa),
                camera_x = dosbox.mem_read_word("ds", 0x81a8),
                camera_y = dosbox.mem_read_word("ds", 0x81ac),
            },
            gamebar_pointer = dosbox.mem_read_word("ds", 0x60be),
            introbar_pointer = dosbox.mem_read_word("ds", 0x60c0),
            intro_mode = dosbox.mem_read_byte("ds", 0x85da),
        }
    elseif hit.segment == 0x0207 and hit.offset == 0x0bef then
        event.pcc_return = {
            page = {
                current = dosbox.mem_read_word("ds", 0x817a),
                page_x = dosbox.mem_read_word("ds", 0x81a6),
                page_y = dosbox.mem_read_word("ds", 0x81aa),
                camera_x = dosbox.mem_read_word("ds", 0x81a8),
                camera_y = dosbox.mem_read_word("ds", 0x81ac),
            },
            map_pointer = dosbox.mem_read_word("ds", 0x657a),
            map_stride = dosbox.mem_read_word("ds", 0x657e),
        }
    elseif hit.segment == 0x01d7 and
            (hit.offset == 0x4486 or hit.offset == 0x4495) then
        event.pcc_call = {
            selected_pointer = dosbox.mem_read_word("ds", hit.offset == 0x4486
                                                    and 0x60be or 0x60c0),
            gamebar_pointer = dosbox.mem_read_word("ds", 0x60be),
            introbar_pointer = dosbox.mem_read_word("ds", 0x60c0),
            intro_mode = dosbox.mem_read_byte("ds", 0x85da),
            page = {
                current = dosbox.mem_read_word("ds", 0x817a),
                page_x = dosbox.mem_read_word("ds", 0x81a6),
                page_y = dosbox.mem_read_word("ds", 0x81aa),
                camera_x = dosbox.mem_read_word("ds", 0x81a8),
                camera_y = dosbox.mem_read_word("ds", 0x81ac),
            },
        }
    elseif hit.segment == 0x0207 and hit.offset == 0x05ff then
        local bp = (registers.ebp or registers.bp or 0) & 0xffff
        local function stack_word(delta)
            return dosbox.mem_read_word("ss", (bp + delta) & 0xffff)
        end
        event.call_args = {
            dac_index = stack_word(0x0c) & 0xff,
            rgb = {stack_word(0x0a) & 0xff, stack_word(0x08) & 0xff,
                   stack_word(0x06) & 0xff},
            return_address = {segment = stack_word(0x04), offset = stack_word(0x02)},
        }
    elseif hit.segment == 0x0207 and hit.offset == 0x0536 then
        local si = (registers.esi or registers.si or 0) & 0xffff
        event.palette_pointer = {selector = registers.ds, offset = si}
        local ok, raw = pcall(dosbox.mem_read_selector, registers.ds, si, 768)
        if ok and raw and #raw >= 768 then
            local sample_indices = {0, 1, 2, 16, 31, 63, 127, 255}
            event.palette_samples = {}
            for _, index in ipairs(sample_indices) do
                local base = index * 3 + 1
                event.palette_samples[tostring(index)] = {
                    string.byte(raw, base), string.byte(raw, base + 1),
                    string.byte(raw, base + 2),
                }
            end
        else
            event.palette_read_error = ok and "short palette buffer" or tostring(raw)
        end
    elseif hit.segment == 0x01d7 and hit.offset == 0x50c3 then
        event.palette_callsite = {
            selector_state = dosbox.mem_read_word("ds", 0x85d4),
            transition_state = dosbox.mem_read_byte("ds", 0x85da),
            menu_descriptor = dosbox.mem_read_word("ds", 0x60bc),
            selected_palette = far_buffer_snapshot(0x610c, 0x400),
            source_surface = far_buffer_snapshot(0x60e4, 0x400),
        }
    elseif hit.segment == 0x01d7 and hit.offset == 0x3020 then
        event.palette_setup = {
            selector_state = dosbox.mem_read_word("ds", 0x85d4),
            transition_state = dosbox.mem_read_byte("ds", 0x85da),
            menu_descriptor = dosbox.mem_read_word("ds", 0x60bc),
            selected_palette = far_buffer_snapshot(0x610c, 0x400),
            source_surface = far_buffer_snapshot(0x60e4, 0x400),
            alternate_palette = far_buffer_snapshot(0x6110, 0x400),
        }
    elseif hit.segment == 0x0207 and hit.offset == 0x02a5 then
        local sp = (registers.esp or registers.sp or 0) & 0xffff
        local function stack_word(delta)
            return dosbox.mem_read_word("ss", (sp + delta) & 0xffff)
        end
        event.palette_ramp = {
            sp = sp,
            fade_step = stack_word(0x04),
            source = {offset = stack_word(0x06), selector = stack_word(0x08)},
            staging = far_buffer_snapshot(0x60e4, 0x400),
        }
    elseif hit.segment == 0x01e7 and hit.offset == 0x0d18 then
        local sp = (registers.esp or registers.sp or 0) & 0xffff
        local return_offset = dosbox.mem_read_word("ss", sp)
        local return_segment = dosbox.mem_read_word("ss", (sp + 2) & 0xffff)
        event.call_return = {segment = return_segment, offset = return_offset,
                             stack_pointer = sp}
        if return_segment ~= nil and return_offset ~= nil then
            local key = string.format("%04x:%04x", return_segment, return_offset)
            pending_dynamic_returns[key] = true
            dosbox.breakpoint_set(return_segment, return_offset, {once = true})
        end
    elseif hit.segment == 0x0207 and hit.offset == 0x18c7 then
        -- The breakpoint is at the function entry, before 18C7 executes
        -- `push bp; mov bp,sp`.  EBP is therefore still the caller's frame;
        -- derive the callee BP from the current stack pointer so [bp+6]
        -- names the far Pascal-string argument described by the prologue.
        local sp = (registers.esp or registers.sp or 0) & 0xffff
        local bp = (sp - 2) & 0xffff
        local stack_raw = dosbox.mem_read("ss", sp, 0x10) or ""
        local pointer_offset = dosbox.mem_read_word("ss", (bp + 0x06) & 0xffff)
        local pointer_selector = dosbox.mem_read_word("ss", (bp + 0x08) & 0xffff)
        local raw = ""
        if pointer_selector ~= nil and pointer_offset ~= nil then
            local ok, value = pcall(
                dosbox.mem_read_selector, pointer_selector, pointer_offset, 0x40)
            if ok and value then raw = value end
        end
        local length = (#raw > 0) and string.byte(raw, 1) or 0
        local text_value = ""
        if length > 0 and #raw >= length + 1 then
            text_value = string.sub(raw, 2, length + 1)
        end
        event.lookup_argument = {
            selector = pointer_selector, offset = pointer_offset,
            pascal_length = length, text = text_value, raw_hex = hex(raw),
            stack_hex = hex(stack_raw), stack_pointer = sp,
        }
    end
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
local cheat = wait_hit("cheat selector branch")
dosbox.key("KBD_4", false)
dosbox.output.exit_focus_checkpoints = {cheat = cheat}
dosbox.mem_write("ds", 0x89f2, "\x01")
dosbox.mem_write("ds", 0x88ba, "\x05\x00")
dosbox.debug_continue()
dosbox.wait_frames(60)
dosbox.breakpoint_set(0x01d7, 0x4ace, {once = true})
wait_hit("selector input wait")
dosbox.mem_write("ds", 0x85d4, "\x00\x00")
dosbox.breakpoint_set(0x01d7, 0x4b18, {once = true})
dosbox.mem_write("ds", 0x88bc, "\x20\x00")
dosbox.debug_continue()
wait_hit("selector dispatch")

if force_player_x ~= nil or force_player_y ~= nil then
    dosbox.breakpoint_set(0x01f7, 0x35c7, {once = true})
    dosbox.debug_continue()
    local render = wait_hit("first render for player force")
    local registers = render.registers or dosbox.cpu_state()
    local object_selector = registers.es
    local object_offset = (registers.edi or 0) & 0xffff
    assert(object_selector ~= nil, "player force has no object selector")
    if force_player_x ~= nil then
        dosbox.mem_write_selector(object_selector, object_offset + 0x02,
                                   little_dword(force_player_x << 16))
    end
    if force_player_y ~= nil then
        dosbox.mem_write_selector(object_selector, object_offset + 0x06,
                                   little_dword(force_player_y << 16))
    end
    dosbox.output.exit_focus_player_forced = {
        frame = dosbox.frame(), selector = object_selector,
        offset = object_offset, x = force_player_x, y = force_player_y,
    }
    forced_player = {selector = object_selector, offset = object_offset}
end

-- Let the level renderer and object pool settle before driving the player.
-- A zero settle window is useful for fixtures whose trigger fires during the
-- first object-stream pass; the old fixed delay could miss its one-shot write.
dosbox.wait_frames(settle_frames)
if pretrigger_frames > 0 then
    dosbox.wait_frames(pretrigger_frames)
    if forced_player and (post_force_player_x ~= nil or post_force_player_y ~= nil) then
        if post_force_player_x ~= nil then
            dosbox.mem_write_selector(forced_player.selector,
                                       forced_player.offset + 0x02,
                                       little_dword(post_force_player_x << 16))
        end
        if post_force_player_y ~= nil then
            dosbox.mem_write_selector(forced_player.selector,
                                       forced_player.offset + 0x06,
                                       little_dword(post_force_player_y << 16))
        end
    end
    dosbox.output.exit_focus_pretrigger = {
        frame = dosbox.frame(), frames = pretrigger_frames,
        goal_mask = dosbox.mem_read_word("ds", 0x60d8),
        exit_flag = dosbox.mem_read_word("ds", 0x89e6),
        x = post_force_player_x or force_player_x,
        y = post_force_player_y or force_player_y,
    }
end
dosbox.key("KBD_right", true)
arm_targets()
dosbox.debug_continue()

for index = 1, event_limit do
    local hit, err = dosbox.wait_for_breakpoint(timeout_ms)
    if not hit then
        dosbox.output.exit_focus_timeout = {
            error = err or "timeout", frame = dosbox.frame(),
            held_frames = hold_frames, events = events, globals = globals(),
        }
        break
    end
    local event = {
        sequence = index, frame = dosbox.frame(),
        name = nil, hit = hit, registers = hit.registers,
        object = object_snapshot(hit), globals = globals(hit),
    }
    for _, target in ipairs(targets) do
        if target.segment == hit.segment and target.offset == hit.offset then
            event.name = target.name
            break
        end
    end
    local hit_key = string.format("%04x:%04x", hit.segment, hit.offset)
    if event.name == nil and pending_dynamic_returns[hit_key] then
        event.name = "transition-effect-return"
        pending_dynamic_returns[hit_key] = nil
    end
    helper_snapshot(event, hit)
    if post_transition_key_active and event.name == "post-transition-input-result" then
        dosbox.key(post_transition_key, false)
        post_transition_key_active = false
    end
    events[#events + 1] = event
    dosbox.output.exit_focus_events = events
    capture_checkpoint(event, hit)
    if score_only and event.name == "completion-branch" then
        break
    end
    seen_targets[hit_key] = true
    if event.name == "transition-setup" then
        post_setup_armed = true
    elseif event.name == "transition-cleanup" then
        post_cleanup_armed = true
    elseif event.name == "post-cleanup-render" then
        -- The first post-reload renderer call often draws into the hidden
        -- page.  Capture one more call so the visible-page handoff can be
        -- correlated with the GAMEBAR blit instead of treating the blank
        -- first page as the final result.
        post_cleanup_render_hits = post_cleanup_render_hits + 1
        if post_cleanup_render_hits < 2 then
            seen_targets[hit_key] = false
        end
    end
    -- 01AC refreshes the keyboard state before testing the action bits at
    -- 01BD.  The host seeds the transient confirmation bits there,
    -- immediately before the native OR/AND test consumes them.  Do not
    -- re-arm 01BD while stopped on 01BD itself: that would retrigger the
    -- same instruction before it executes.  It is re-armed at the entry to
    -- the second wait below, after the first confirmation has returned.
    if event.name == "post-transition-input-seed" then
        dosbox.mem_write("ds", 0x88bc, "\x30\x00")
        dosbox.mem_write("ds", 0x8196, "\x30\x00")
        event.input_seed = {
            action_word = dosbox.mem_read_word("ds", 0x8196),
            transient_word = dosbox.mem_read_word("ds", 0x88bc),
        }
    elseif event.name == "post-transition-input-wait-2" then
        local seed_key = "01d7:01bd"
        if post_transition_key ~= "" then
            -- Drive the real keyboard mapper only after the release/debounce
            -- wait has returned.  Holding through 01AC lets the native input
            -- refresh populate its own action words.
            dosbox.key(post_transition_key, true)
            post_transition_key_active = true
            input_seed_enabled = false
        else
            input_seed_enabled = true
        end
        seen_targets[seed_key] = false
    elseif event.name == "transition-full-palette" then
        -- 0536 is the DAC-upload entry.  The loop resumes at 0539; only
        -- after that instruction is it safe to re-arm the next 0536 hit.
        palette_loop_enabled = true
        seen_targets["0207:0539"] = false
    elseif event.name == "transition-full-palette-loop" then
        seen_targets["0207:0536"] = false
    end
    -- Keep the right key down while rearming the one-shot target set.  The
    -- timeout is bounded by the host trace, and the frame guard prevents an
    -- accidental infinite run if a breakpoint fires every frame.
    if index < event_limit then
        arm_targets()
        dosbox.debug_continue()
    end
end

dosbox.key("KBD_right", false)
dosbox.breakpoint_clear()
dosbox.output.exit_focus_events = events
dosbox.output.exit_focus_armed_targets = armed_targets
dosbox.output.exit_focus_complete = true
