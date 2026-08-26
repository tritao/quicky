-- Trace the game's all-puzzle-letter completion branch and the level transition
-- that follows it.  The optional mask seed is a debugger-only control: the
-- normal letter callback writes DS:60D8, while this probe can seed the same
-- completed value to isolate the transition renderer.
local timeout_ms = TRACE_TIMEOUT_MS or 10000
local optional_timeout_ms = TRACE_OPTIONAL_TIMEOUT_MS or timeout_ms
local goal_mask = TRACE_GOAL_MASK
local deep_only = TRACE_DEEP_ONLY
local native_goal = TRACE_NATIVE_GOAL or false
local native_cloud_focus = TRACE_NATIVE_CLOUD_FOCUS or false
local native_post_input = TRACE_NATIVE_POST_INPUT or false
local force_player_x = TRACE_FORCE_PLAYER_X
local force_player_y = TRACE_FORCE_PLAYER_Y
local checkpoints = {}

local function wait_hit(label)
    local hit, err = dosbox.wait_for_breakpoint(timeout_ms)
    if not hit then error(label .. ": " .. (err or "timeout")) end
    return hit
end

local function little_dword(value)
    return string.char(value & 0xff, (value >> 8) & 0xff,
                       (value >> 16) & 0xff, (value >> 24) & 0xff)
end

local function hex(raw)
    local output = {}
    for index = 1, #raw do
        output[#output + 1] = string.format("%02x", string.byte(raw, index))
    end
    return table.concat(output)
end

local function player_snapshot()
    local offset = dosbox.mem_read_word("ds", 0x881a)
    local snapshot = {selector = 0x027f, offset = offset}
    if offset == nil then return snapshot end
    local ok, raw = pcall(dosbox.mem_read_selector, snapshot.selector, offset, 0x78)
    if ok and type(raw) == "string" and #raw > 0 then
        snapshot.raw_hex = hex(raw)
    else
        snapshot.read_error = (ok and "empty player record" or tostring(raw))
    end
    return snapshot
end

local function read_dword_ds(address)
    local raw = dosbox.mem_read("ds", address, 4)
    if type(raw) ~= "string" or #raw < 4 then return nil end
    return string.byte(raw, 1) |
        (string.byte(raw, 2) << 8) |
        (string.byte(raw, 3) << 16) |
        (string.byte(raw, 4) << 24)
end

local function gameplay_snapshot()
    return {
        score_low = dosbox.mem_read_word("ds", 0x881c),
        score_high = dosbox.mem_read_word("ds", 0x881e),
        lives = dosbox.mem_read_word("ds", 0x880a),
        ammo = dosbox.mem_read_word("ds", 0x880c),
        current_health = dosbox.mem_read_word("ds", 0x8822),
        maximum_health = dosbox.mem_read_word("ds", 0x8824),
        puzzle_mask = dosbox.mem_read_word("ds", 0x60d8),
        pending_action = dosbox.mem_read_word("ds", 0x612e),
        invulnerability = dosbox.mem_read_word("ds", 0x8810),
    }
end

local function resource_lookup_snapshot(hit)
    if not hit or hit.segment ~= 0x0207 or hit.offset ~= 0x18c7 then
        return nil
    end
    local registers = hit.registers or {}
    local sp = (registers.esp or registers.sp or 0) & 0xffff
    local path_offset = dosbox.mem_read_word("ss", (sp + 0x04) & 0xffff)
    local path_selector = dosbox.mem_read_word("ss", (sp + 0x06) & 0xffff)
    local raw = ""
    if path_selector ~= nil and path_offset ~= nil then
        local ok, value = pcall(dosbox.mem_read_selector, path_selector,
                                path_offset, 0x40)
        if ok and type(value) == "string" then raw = value end
    end
    local length = (#raw > 0) and string.byte(raw, 1) or 0
    local text = ""
    if length > 0 and #raw >= length + 1 then
        text = string.sub(raw, 2, length + 1)
    end
    return {
        stack_pointer = sp,
        path = {selector = path_selector, offset = path_offset,
                pascal_length = length, text = text, raw_hex = hex(raw)},
        published_before = {
            start = read_dword_ds(0x97e8),
            ['end'] = read_dword_ds(0x97e4),
            size = read_dword_ds(0x97ec),
        },
    }
end

local function checkpoint(name, hit, hold_frames)
    local record = {
        name = name,
        frame = dosbox.frame(),
        hit = hit,
        registers = hit and hit.registers or dosbox.cpu_state(),
        goal_mask = dosbox.mem_read_word("ds", 0x60d8),
        exit_flag = dosbox.mem_read_word("ds", 0x89e6),
        selector_state = dosbox.mem_read_word("ds", 0x85d4),
        transition_state = dosbox.mem_read_byte("ds", 0x85da),
        transition_done = dosbox.mem_read_word("ds", 0x89e0),
        transition_pending = dosbox.mem_read_word("ds", 0x89ec),
        audio_ready = dosbox.mem_read_byte("ds", 0x5044),
        reload_state = {
            level_index = dosbox.mem_read_word("ds", 0x60b2),
            object_count = dosbox.mem_read_word("ds", 0x880a),
            object_reset = dosbox.mem_read_word("ds", 0x8810),
            player_offset = dosbox.mem_read_word("ds", 0x881a),
            transition_index = dosbox.mem_read_word("ds", 0x85d2),
            map_pointer = dosbox.mem_read_word("ds", 0x657a),
            map_stride = dosbox.mem_read_word("ds", 0x657e),
            page_current = dosbox.mem_read_word("ds", 0x817a),
            page_pending = dosbox.mem_read_word("ds", 0x817c),
            field_81a6 = dosbox.mem_read_word("ds", 0x81a6),
            field_81aa = dosbox.mem_read_word("ds", 0x81aa),
            field_81be = dosbox.mem_read_word("ds", 0x81be),
            field_81c2 = dosbox.mem_read_word("ds", 0x81c2),
            field_81d1 = dosbox.mem_read_byte("ds", 0x81d1),
            world_selector = dosbox.mem_read_word("ds", 0x5042),
        },
        player = player_snapshot(),
        resource_lookup = resource_lookup_snapshot(hit),
        resource_state = {
            start = read_dword_ds(0x97e8),
            ['end'] = read_dword_ds(0x97e4),
            size = read_dword_ds(0x97ec),
        },
        gameplay_state = gameplay_snapshot(),
    }
    checkpoints[#checkpoints + 1] = record
    dosbox.output.goal_transition_checkpoints = checkpoints
    dosbox.output.goal_transition_checkpoint = record
    -- Leave the guest stopped long enough for the host to capture this exact
    -- rendered page before it acknowledges the checkpoint.
    if hold_frames == nil then hold_frames = 20 end
    if hold_frames > 0 then dosbox.wait_frames(hold_frames) end
    dosbox.output.goal_transition_checkpoint = nil
end

-- A timeout is evidence that this build did not reach the requested path in
-- the observation window, not a reason to abort the whole research run.  Keep
-- those negative observations in the ledger and leave the guest running.
local function optional_checkpoint(name, segment, offset, hold_frames)
    -- The debugger can retain an already-reported one-shot until the next
    -- command round trip. This phase has only one live optional target, so
    -- clear the breakpoint set at each transition rather than allowing a
    -- stale address to masquerade as the next checkpoint.
    dosbox.breakpoint_clear()
    dosbox.breakpoint_set(segment, offset, {once = true})
    dosbox.debug_continue()
    local hit, err = dosbox.wait_for_breakpoint(optional_timeout_ms)
    if hit then
        dosbox.breakpoint_clear()
        checkpoint(name, hit, hold_frames)
        return true
    end
    dosbox.breakpoint_clear()
    local skipped = {
        name = name,
        skipped = true,
        frame = dosbox.frame(),
        error = err or "timeout",
        registers = dosbox.cpu_state(),
        goal_mask = dosbox.mem_read_word("ds", 0x60d8),
        exit_flag = dosbox.mem_read_word("ds", 0x89e6),
        selector_state = dosbox.mem_read_word("ds", 0x85d4),
        transition_state = dosbox.mem_read_byte("ds", 0x85da),
        transition_done = dosbox.mem_read_word("ds", 0x89e0),
        transition_pending = dosbox.mem_read_word("ds", 0x89ec),
        audio_ready = dosbox.mem_read_byte("ds", 0x5044),
    }
    checkpoints[#checkpoints + 1] = skipped
    dosbox.output.goal_transition_checkpoints = checkpoints
    return false
end

dosbox.output.awaiting_startup_replay = true
dosbox.wait_frames(350)

-- Enter the verified cheat selector route and launch W1L1 (index 0).
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
dosbox.wait_frames(60)
dosbox.breakpoint_set(0x01d7, 0x4ace, {once = true})
local selector_wait = wait_hit("selector input wait")
dosbox.mem_write("ds", 0x85d4, "\x00\x00")
dosbox.breakpoint_set(0x01d7, 0x4b18, {once = true})
dosbox.mem_write("ds", 0x88bc, "\x20\x00")
dosbox.debug_continue()
local dispatch = wait_hit("selector dispatch")
checkpoint("selector-dispatch", dispatch)

dosbox.breakpoint_set(0x01f7, 0x35c7, {once = true})
dosbox.debug_continue()
local render = wait_hit("first render")
checkpoint("first-render", render)

if native_goal then
    -- At 35C7 the live ES:EDI pair identifies the player object in the
    -- default W1L1 run (the same pair is recorded by quiky_player_trace).
    -- Move only that object in the fixture; do not seed DS:60D8/89E6.
    local registers = render.registers or dosbox.cpu_state()
    local object_selector = registers.es
    local object_offset = (registers.edi or 0) & 0xffff
    assert(object_selector ~= nil, "native goal has no player object selector")
    if force_player_x ~= nil then
        dosbox.mem_write_selector(object_selector, object_offset + 0x02,
                                   little_dword(force_player_x << 16))
    end
    if force_player_y ~= nil then
        dosbox.mem_write_selector(object_selector, object_offset + 0x06,
                                   little_dword(force_player_y << 16))
    end
    checkpoints[#checkpoints + 1] = {
        name = "native-goal-player-forced",
        frame = dosbox.frame(),
        goal_mask = dosbox.mem_read_word("ds", 0x60d8),
        exit_flag = dosbox.mem_read_word("ds", 0x89e6),
        object = {selector = object_selector, offset = object_offset,
                  x = force_player_x, y = force_player_y},
    }
    dosbox.output.goal_transition_checkpoints = checkpoints
else
    if goal_mask ~= nil then
        dosbox.mem_write("ds", 0x60d8,
                         string.char(goal_mask & 0xff, (goal_mask >> 8) & 0xff))
    end
    -- The normal exit/collision handler writes the sentinel -1, not a
    -- generic boolean.  The main loop explicitly tests DS:89E6 == 0xffff.
    dosbox.mem_write("ds", 0x89e6, "\xff\xff")
    -- Neutralize adjacent state gates for the synthetic transition probe.
    dosbox.mem_write("ds", 0x89ee, "\x00\x00")
    dosbox.mem_write("ds", 0x89ea, "\x00\x00")
    dosbox.mem_write("ds", 0x89f0, "\x00\x00")
    dosbox.mem_write("ds", 0x89f4, "\x00\x00")
    dosbox.mem_write("ds", 0x880a, "\x00\x00")
    dosbox.mem_write("ds", 0x85da, "\x00")
    dosbox.mem_write("ds", 0x89ec, "\x00\x00")
    dosbox.mem_write("ds", 0x89e0, "\x00\x00")
end

if native_cloud_focus and native_goal then
    -- 01F7:92A9 is the accepted cloud/player overlap branch and writes
    -- DS:89E6.  Keep this as an optional one-shot observation so the native
    -- letter fixture can distinguish the real cloud gate from the synthetic
    -- DS:89E6 seed used by older completion probes.
    optional_checkpoint("native-cloud-flag-writer", 0x01f7, 0x92a9, 0)
    optional_checkpoint("native-cloud-outer-gate", 0x01d7, 0x4ea0, 0)
    optional_checkpoint("native-cloud-outer-positive-branch", 0x01d7, 0x4eaa, 0)
    optional_checkpoint("native-cloud-transition-wait", 0x0207, 0x0002, 0)
    local completion_entry = optional_checkpoint("native-cloud-completion-routine", 0x01d7, 0x14e1, 0)
    if native_post_input and completion_entry then
        -- Arm the whole authored post-14E1 path at once. Sequential optional
        -- waits can observe a debugger hit that was queued by the preceding
        -- wait; a single set preserves the guest's actual execution order.
        local post_targets = {
            {name = "input-wait-1", segment = 0x01d7, offset = 0x01f0},
            {name = "input-wait-2", segment = 0x01d7, offset = 0x01d6},
            {name = "input-poll", segment = 0x01d7, offset = 0x01ac},
            {name = "input-seed", segment = 0x01d7, offset = 0x01bd},
            {name = "input-result", segment = 0x01d7, offset = 0x01d1},
            {name = "transition-effect", segment = 0x01e7, offset = 0x0caa},
            {name = "fade-helper", segment = 0x0207, offset = 0x022a},
            {name = "reload-gate", segment = 0x01d7, offset = 0x5010},
            -- These are the callsites and entries of the focused post-5010
            -- closure.  Keep them in the same native run so the ledger
            -- records the actual order instead of treating the reload as one
            -- opaque call.
            {name = "reload-delay-callsite", segment = 0x01d7, offset = 0x5017},
            {name = "reload-delay-callee", segment = 0x01f7, offset = 0x0908},
            {name = "reload-delay-tick", segment = 0x01f7, offset = 0x0931},
            {name = "reload-buffer-copy-callsite", segment = 0x01d7, offset = 0x5038},
            {name = "reload-player-reposition-callsite", segment = 0x01d7, offset = 0x503d},
            {name = "reload-player-reposition-callee", segment = 0x01f7, offset = 0x1aaa},
            {name = "reload-animation-loader", segment = 0x01f7, offset = 0x5d38},
            {name = "reload-camera-rebuild-callsite", segment = 0x01d7, offset = 0x5042},
            {name = "reload-camera-rebuild-callee", segment = 0x01f7, offset = 0x321f},
            {name = "reload-world-dispatch-callsite", segment = 0x01d7, offset = 0x5047},
            {name = "reload-world-dispatch", segment = 0x01d7, offset = 0x313d},
            {name = "reload-resource-lookup", segment = 0x0207, offset = 0x18c7},
            {name = "cleanup", segment = 0x01d7, offset = 0x504f},
            {name = "post-cleanup-render", segment = 0x01f7, offset = 0x35c7},
        }
        for _, target in ipairs(post_targets) do
            dosbox.breakpoint_set(target.segment, target.offset, {once = true})
        end
        dosbox.debug_continue()
        local space_active = false
        for _ = 1, #post_targets + 4 do
            local hit, err = dosbox.wait_for_breakpoint(optional_timeout_ms)
            if not hit then
                checkpoints[#checkpoints + 1] = {
                    name = "native-post-input-timeout",
                    skipped = true,
                    frame = dosbox.frame(),
                    error = err or "timeout",
                }
                break
            end
            local label = string.format("native-post-input-%04x:%04x",
                                        hit.segment, hit.offset)
            checkpoint(label, hit, 0)
            if hit.segment == 0x01d7 and hit.offset == 0x5038 then
                -- 0D5A is also used by the preceding transition-buffer
                -- helper. Arm it only after this reload callsite so the
                -- observed callee is attributed to 5038 rather than to the
                -- earlier fade path.
                dosbox.breakpoint_set(0x0227, 0x0d5a, {once = true})
            end
            if hit.segment == 0x01d7 and hit.offset == 0x01d6 then
                dosbox.key("KBD_space", true)
                space_active = true
            elseif hit.segment == 0x01d7 and hit.offset == 0x01d1 and space_active then
                dosbox.key("KBD_space", false)
                space_active = false
            end
            if hit.segment == 0x01f7 and hit.offset == 0x35c7 then
                break
            end
            dosbox.debug_continue()
        end
        if space_active then dosbox.key("KBD_space", false) end
        dosbox.breakpoint_clear()
    end
    if not native_post_input then
        optional_checkpoint("native-cloud-completion-call", 0x01d7, 0x4f0d, 0)
        optional_checkpoint("native-cloud-completion-check", 0x01d7, 0x1669, 0)
        optional_checkpoint("native-cloud-completion-branch", 0x01d7, 0x16c6, 0)
    end
end

-- These are selector-relative segment-1 offsets.  01D7:1669 is the separate
-- goal-mask state routine; its equal branch at 16C6 performs the completion
-- text/effect setup before returning to the state machine.  01D7:4EA0 is the
-- outer exit-flag gate and 4F0D calls the completion/HUD routine after it.
-- Keep every post-render probe optional: a bounded run can legitimately stay
-- in the ordinary input/update branch.
local second_render = optional_checkpoint("second-render", 0x01f7, 0x35c7)
if second_render then
    if deep_only then
        optional_checkpoint("goal-exit-flag-gate", 0x01d7, 0x4ea0)
        dosbox.breakpoint_clear()
        dosbox.output.goal_transition_checkpoints = checkpoints
        dosbox.output.goal_transition_complete = true
        return
    end
    -- Probe the state gates independently.  The first two are hot loop
    -- entries, so nesting all later breakpoints under a single arm can hide a
    -- deeper branch behind a repeated 48E6/491D hit.
    optional_checkpoint("goal-render-sync-wait", 0x01d7, 0x48bb)
    optional_checkpoint("goal-exit-branch-destination", 0x01d7, 0x4968)
    optional_checkpoint("goal-exit-state-check", 0x01d7, 0x4b8d)
    optional_checkpoint("goal-exit-normalization", 0x01d7, 0x4ba4)
    optional_checkpoint("goal-post-collision-branch", 0x01d7, 0x4c43)
    optional_checkpoint("goal-state-final-check", 0x01d7, 0x4cb1)
    optional_checkpoint("goal-post-render-branch", 0x01d7, 0x48e6)
    optional_checkpoint("goal-input-dispatch", 0x01d7, 0x491d)
    optional_checkpoint("goal-transition-dispatch", 0x01d7, 0x4cfc)
    optional_checkpoint("goal-transition-stage", 0x01d7, 0x4d03)
    optional_checkpoint("goal-exit-flag-gate", 0x01d7, 0x4ea0)
    optional_checkpoint("goal-completion-hud-loop", 0x01d7, 0x4f0d)
    optional_checkpoint("goal-state-reload-stage", 0x01d7, 0x5010)
    optional_checkpoint("goal-state-loop-return", 0x01d7, 0x504f)
    optional_checkpoint("native-exit-writer-4996", 0x01f7, 0x4996)
    optional_checkpoint("native-exit-writer-4aac", 0x01f7, 0x4aac)
    optional_checkpoint("native-exit-writer-92a9", 0x01f7, 0x92a9)
    optional_checkpoint("goal-completion-check", 0x01d7, 0x1669)
    optional_checkpoint("goal-completion-branch", 0x01d7, 0x16c6)
    optional_checkpoint("goal-transition-setup", 0x01d7, 0x1709)
end

dosbox.breakpoint_clear()
dosbox.output.goal_transition_checkpoints = checkpoints
dosbox.output.goal_transition_complete = true
