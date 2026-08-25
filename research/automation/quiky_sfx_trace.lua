-- Quiky SFX runtime tracer. Loaded by research/tools/quikysfxtrace.py.
local config = TRACE_CONFIG or {}
local timeout_ms = config.timeout_ms or 3000
local event_timeout_ms = config.event_timeout_ms or 1200
local max_events = config.max_events or 32
local settle_frames = config.settle_frames or 240
local action_attempts = config.action_attempts or 5
local action_settle_frames = config.action_settle_frames or 30
local inter_event_settle_frames = config.inter_event_settle_frames or 30
local action_profile = config.action_profile or "basic"
local capture_path = config.audio_capture_path or ""
local audio_tail_frames = config.audio_tail_frames or 0
local select_level = config.select_level or ""
local selector_frames = config.selector_frames or 60
local driver_probe_count = config.driver_probe_count or 0
local event_driver_probe_count = config.event_driver_probe_count or 0
local interpreter_probe_count = config.interpreter_probe_count or 0
local macro_probe_count = config.macro_probe_count or 0
local mixer_probe_count = config.mixer_probe_count or 0
local priority_probe_count = config.priority_probe_count or 0
local priority_only = config.priority_only or false
local priority_status_probe_count = config.priority_status_probe_count or 0
local priority_irq_status_probe_count = config.priority_irq_status_probe_count or 0
local mix_probe_count = config.mix_probe_count or 0
local output_probe_count = config.output_probe_count or 0
local output_input_word = config.output_input_word
local irq_probe = config.irq_probe or false
local dma_probe = config.dma_probe or false
local pool_probe = config.pool_probe or false
local callsite_probe = config.callsite_probe or false
local targeted_probe = config.targeted_probe or ""
local teleport_player = config.teleport_player
local teleport_object = config.teleport_object
local camera_x = config.camera_x
local camera_y = config.camera_y
local warmup_key = config.warmup_key or ""
local warmup_frames = config.warmup_frames or 0
local force_all_ids = config.force_all_ids or false
local force_id = config.force_id
local collision_pair = config.collision_pair
local collision_high_bit = config.collision_high_bit or false
local selection_voice = config.selection_voice
local mute_music = config.mute_music or false

local function word(s, index)
    local lo, hi = string.byte(s, index, index + 1)
    return lo | (hi << 8)
end

local function dword(s, index)
    return word(s, index) | (word(s, index + 2) << 16)
end

local function dword_bytes(value)
    local v = value & 0xffffffff
    return string.char(
        v & 0xff, (v >> 8) & 0xff, (v >> 16) & 0xff, (v >> 24) & 0xff)
end

local function word_bytes(value)
    local v = value & 0xffff
    return string.char(v & 0xff, (v >> 8) & 0xff)
end

local function teleport_live_player()
    if teleport_player == nil then return nil end
    local pointer = dosbox.mem_read("ds", 0x755e, 4)
    local pool_offset = word(pointer, 1)
    local pool_selector = word(pointer, 3)
    if pool_selector == 0 then
        return {error = "live player pool selector is zero"}
    end
    dosbox.mem_write_selector(
        pool_selector, pool_offset + 2,
        dword_bytes((teleport_player.x or 0) << 16))
    dosbox.mem_write_selector(
        pool_selector, pool_offset + 6,
        dword_bytes((teleport_player.y or 0) << 16))
    return {
        selector = pool_selector,
        offset = pool_offset,
        x = teleport_player.x,
        y = teleport_player.y,
    }
end

local function write_live_player_position(x, y)
    local pointer = dosbox.mem_read("ds", 0x755e, 4)
    local pool_offset = word(pointer, 1)
    local pool_selector = word(pointer, 3)
    if pool_selector == 0 then
        return {error = "live player pool selector is zero"}
    end
    dosbox.mem_write_selector(
        pool_selector, pool_offset + 2, dword_bytes((x or 0) << 16))
    dosbox.mem_write_selector(
        pool_selector, pool_offset + 6, dword_bytes((y or 0) << 16))
    return {
        selector = pool_selector,
        offset = pool_offset,
        x = x,
        y = y,
    }
end

local function teleport_live_object()
    if teleport_object == nil then return nil end
    local pointer = dosbox.mem_read("ds", 0x755e, 4)
    local pool_offset = word(pointer, 1)
    local pool_selector = word(pointer, 3)
    local stride = dosbox.mem_read_word("ds", 0x30ce)
    if pool_selector == 0 or stride == 0 then
        return {error = "live object pool selector or stride is zero"}
    end
    local object_offset = pool_offset + teleport_object.index * stride
    dosbox.mem_write_selector(
        pool_selector, object_offset + 2,
        dword_bytes((teleport_object.x or 0) << 16))
    dosbox.mem_write_selector(
        pool_selector, object_offset + 6,
        dword_bytes((teleport_object.y or 0) << 16))
    return {
        selector = pool_selector,
        offset = object_offset,
        index = teleport_object.index,
        x = teleport_object.x,
        y = teleport_object.y,
    }
end

local function hex(s)
    return (s:gsub(".", function(c)
        return string.format("%02x", string.byte(c))
    end))
end

local function byte_values(s)
    local values = {}
    for index = 1, #s do
        values[#values + 1] = string.byte(s, index)
    end
    return values
end

local function read_selector(selector, offset, size)
    local ok, data = pcall(function()
        return dosbox.mem_read_selector(selector, offset, size)
    end)
    if ok then return data end
    return nil
end

-- The mixer uses a phase-dependent jump table into its unrolled store body.
-- Keep a breakpoint on every 16-bit store so the first dynamic write is
-- captured even when the entry jumps past the first source-read block.
local function mixer_store_offsets()
    local offsets = {0x1ac2}
    for index = 0, 30 do
        offsets[#offsets + 1] = 0x1ad1 + index * 0x10
    end
    return offsets
end

local function output_store_offsets()
    local offsets = {}
    for index = 0, 31 do
        offsets[#offsets + 1] = 0x1f9c + index * 0x06
    end
    return offsets
end

local function wait_hit(label, timeout)
    local hit, err = dosbox.wait_for_breakpoint(timeout or timeout_ms)
    if not hit then
        return nil, label .. ": " .. (err or "timeout")
    end
    return hit
end

local function global_snapshot()
    local state = dosbox.mem_read("ds", 0x5042, 0x22)
    local flags = dosbox.mem_read("ds", 0x613e, 2)
    local function byte_memory(offset)
        return string.byte(dosbox.mem_read("ds", offset, 1), 1)
    end
    local function byte_at(offset)
        return string.byte(state, offset - 0x5042 + 1)
    end
    local sample_rate = dosbox.mem_read("ds", 0x2b39, 2)
    local render_step = dosbox.mem_read("ds", 0x2b30, 4)
    local output_buffer = dosbox.mem_read("ds", 0x2af6, 4)
    return {
        music_flag = string.byte(flags, 1),
        fx_flag = string.byte(flags, 2),
        audio_ready = byte_at(0x5044),
        sb_base = word(state, 0x5046 - 0x5042 + 1),
        irq = byte_at(0x5048),
        dma = byte_at(0x504a),
        stage = word(state, 0x504c - 0x5042 + 1),
        driver_selector = word(state, 0x504e - 0x5042 + 1),
        current_effect = word(dosbox.mem_read("ds", 0x612e, 2), 1),
        channel_pointer = hex(dosbox.mem_read("ds", 0x2f5c, 8)),
        sample_rate = sample_rate and word(sample_rate, 1) or nil,
        update_frames = word(dosbox.mem_read("ds", 0x2b0d, 2), 1),
        update_frames_copy = word(dosbox.mem_read("ds", 0x2b23, 2), 1),
        frame_remaining = word(dosbox.mem_read("ds", 0x2b0f, 2), 1),
        mix_remaining = word(dosbox.mem_read("ds", 0x2b11, 2), 1),
        source_remaining = word(dosbox.mem_read("ds", 0x2b13, 2), 1),
        render_step = render_step and dword(render_step, 1) or nil,
        output_buffer = output_buffer and dword(output_buffer, 1) or nil,
        driver_state = byte_memory(0x2b38),
        render_mode = byte_memory(0x2b3e),
    }
end

local function player_snapshot()
    local pointer = dosbox.mem_read("ds", 0x755e, 4)
    local pool_selector = word(pointer, 3)
    local player_offset = dosbox.mem_read_word("ds", 0x881a)
    if pool_selector == 0 then
        return {error = "live player pool selector is zero"}
    end
    local ok, raw_or_error = pcall(
        dosbox.mem_read_selector, pool_selector, player_offset, 0x40)
    if not ok or not raw_or_error or #raw_or_error < 0x40 then
        return {
            selector = pool_selector,
            offset = player_offset,
            error = ok and "short player state" or tostring(raw_or_error),
        }
    end
    return {
        selector = pool_selector,
        offset = player_offset,
        x = dword(raw_or_error, 3) >> 16,
        y = dword(raw_or_error, 7) >> 16,
        x_fixed = dword(raw_or_error, 3),
        y_fixed = dword(raw_or_error, 7),
        phase = string.byte(raw_or_error, 0x17 + 1),
        callback = word(raw_or_error, 0x18 + 1),
        action_word = word(raw_or_error, 1),
        mode_0x37 = string.byte(raw_or_error, 0x37 + 1),
        mode_0x38 = string.byte(raw_or_error, 0x38 + 1),
        aux_word_0x3e = word(raw_or_error, 0x3e + 1),
    }
end

local function object_snapshot(raw, selector, offset, index)
    local x_fixed = dword(raw, 3)
    local y_fixed = dword(raw, 7)
    return {
        index = index,
        selector = selector,
        offset = offset,
        x = x_fixed >> 16,
        y = y_fixed >> 16,
        x_fixed = x_fixed,
        y_fixed = y_fixed,
        action_word = word(raw, 1),
        velocity_x_fixed = dword(raw, 0x0a + 1),
        velocity_y_fixed = dword(raw, 0x0e + 1),
        kind = word(raw, 0x14 + 1),
        phase = string.byte(raw, 0x17 + 1),
        callback = word(raw, 0x18 + 1),
        callback_data = word(raw, 0x1a + 1),
        sprite_slot = word(raw, 0x12 + 1),
        lifetime = word(raw, 0x2c + 1),
        state_field = word(raw, 0x2e + 1),
        contact_ring_slot = word(raw, 0x2a + 1),
        update_state = word(raw, 0x32 + 1),
        contact_latch = string.byte(raw, 0x3c + 1),
    }
end

local function pool_snapshot()
    local pointer = dosbox.mem_read("ds", 0x755e, 4)
    local pool_offset = word(pointer, 1)
    local pool_selector = word(pointer, 3)
    local stride = dosbox.mem_read_word("ds", 0x30ce)
    if pool_selector == 0 or stride == 0 then
        return {
            selector = pool_selector,
            offset = pool_offset,
            stride = stride,
            objects = {},
            error = "live object pool selector or stride is zero",
        }
    end
    local objects = {}
    for index = 0, 63 do
        local offset = pool_offset + index * stride
        local raw = read_selector(pool_selector, offset, 0x40)
        if raw and #raw >= 0x40 then
            local object = object_snapshot(raw, pool_selector, offset, index)
            if object.callback ~= 0 then
                objects[#objects + 1] = object
            end
        end
    end
    return {
        selector = pool_selector,
        offset = pool_offset,
        stride = stride,
        objects = objects,
    }
end

local function table_snapshot(selector)
    local raw = read_selector(selector, 0x200, 0x200)
    if not raw then
        return {selector = selector, offset = 0x200, unavailable = true}
    end
    local entries = {}
    for id = 0, 63 do
        local entry = string.sub(raw, id * 8 + 1, id * 8 + 8)
        local active = false
        for index = 1, 8 do
            if string.byte(entry, index) ~= 0xff then
                active = true
                break
            end
        end
        entries[id + 1] = {
            id = id,
            active = active,
            raw_hex = hex(entry),
        }
    end
    return {
        selector = selector,
        offset = 0x200,
        bytes_hex = hex(raw),
        entries = entries,
    }
end

local function channel_snapshot(selector)
    local raw = read_selector(selector, 0x2f5c, 0x400)
    if not raw then
        return {selector = selector, offset = 0x2f5c, unavailable = true}
    end
    return {
        selector = selector,
        offset = 0x2f5c,
        size = 0x400,
        bytes_hex = hex(raw),
    }
end

local function game_channel_snapshot(data_selector)
    local pointer_bytes = read_selector(data_selector, 0x2f5c, 8)
    if not pointer_bytes then
        return {
            selector = data_selector,
            pointer_table_offset = 0x2f5c,
            unavailable = true,
        }
    end
    local voices = {}
    for index = 0, 3 do
        local offset = word(pointer_bytes, index * 2 + 1)
        local voice_bytes = read_selector(data_selector, offset, 0x80)
        voices[index + 1] = {
            index = index,
            offset = offset,
            bytes_hex = voice_bytes and hex(voice_bytes) or nil,
        }
    end
    return {
        selector = data_selector,
        pointer_table_offset = 0x2f5c,
        pointer_table_hex = hex(pointer_bytes),
        voices = voices,
    }
end

local function voice_state_snapshot(selector, offset)
    local raw = read_selector(selector, offset, 0x50)
    if not raw then return nil end
    return {
        selector = selector,
        offset = offset,
        bytes_hex = hex(raw),
        active = string.byte(raw, 1),
        macro_step = word(raw, 0x02 + 1),
        macro_pointer = word(raw, 0x04 + 1),
        period = word(raw, 0x10 + 1),
        period_target = word(raw, 0x12 + 1),
        note = string.byte(raw, 0x14 + 1),
        control = string.byte(raw, 0x2c + 1),
        sample_start = dword(raw, 0x2d + 1),
        sample_length_words = word(raw, 0x31 + 1),
        modulation = dword(raw, 0x33 + 1),
        modulation_ticks = string.byte(raw, 0x38 + 1),
        priority = string.byte(raw, 0x3a + 1),
        status = word(raw, 0x3b + 1),
        table_pointer = word(raw, 0x41 + 1),
    }
end

local function mixer_state_snapshot(selector, offset)
    local raw = read_selector(selector, offset, 0x24)
    if not raw then return nil end
    return {
        selector = selector,
        offset = offset,
        bytes_hex = hex(raw),
        source_start = dword(raw, 0x08 + 1),
        source_end = dword(raw, 0x0c + 1),
        rate = word(raw, 0x10 + 1),
        volume = word(raw, 0x12 + 1),
        flags = word(raw, 0x14 + 1),
        current_source = dword(raw, 0x18 + 1),
        step = dword(raw, 0x1c + 1),
    }
end

local function scheduler_snapshot()
    return {
        frame_length = dosbox.mem_read_word("ds", 0x2b0d),
        frame_remaining = dosbox.mem_read_word("ds", 0x2b0f),
        mix_remaining = dosbox.mem_read_word("ds", 0x2b11),
        source_remaining = dosbox.mem_read_word("ds", 0x2b13),
        output_cursor = dosbox.mem_read("ds", 0x2b17, 4)
            and hex(dosbox.mem_read("ds", 0x2b17, 4)) or nil,
        buffer_base = dosbox.mem_read("ds", 0x2af6, 4)
            and hex(dosbox.mem_read("ds", 0x2af6, 4)) or nil,
    }
end

local function runtime_snapshot()
    local globals = global_snapshot()
    local result = {
        globals = globals,
        data_selector = dosbox.cpu_state().ds,
    }
    result.game_channels = game_channel_snapshot(result.data_selector)
    if globals.driver_selector ~= 0 then
        result.effect_table = table_snapshot(globals.driver_selector)
        result.channels = channel_snapshot(globals.driver_selector)
    end
    return result
end

local function stop_for_capture()
    local current = dosbox.cpu_state()
    dosbox.breakpoint_set(current.cs, current.eip, {once = true})
    dosbox.debug_continue()
    local hit, err = wait_hit("capture barrier")
    if not hit then error(err) end
    return hit
end

local function caller_from_stack(registers, extra)
    local stack = dosbox.mem_read("ss", registers.esp + (extra or 0), 8)
    return {
        offset = word(stack, 1),
        segment = word(stack, 3),
        stack_hex = hex(stack),
    }
end

local function capture_sfx_entry(hit, sequence)
    local event = {
        sequence = sequence,
        entry = {segment = hit.segment, offset = hit.offset},
        registers = hit.registers,
        caller = caller_from_stack(hit.registers, 0),
        globals = global_snapshot(),
        player = player_snapshot(),
    }
    if pool_probe then
        event.pool = pool_snapshot()
    end
    return event
end

local function capture_low_level(hit, event)
    local selector = event.globals.driver_selector
    local caller = caller_from_stack(hit.registers, 36)
    event.low_level = {
        entry = {segment = hit.segment, offset = hit.offset},
        registers = hit.registers,
        caller = caller,
        table = selector ~= 0 and table_snapshot(selector) or nil,
        channels_before = selector ~= 0 and channel_snapshot(selector) or nil,
        game_channels_before = game_channel_snapshot(hit.registers.ds),
    }
    local priority_record = nil
    local priority_pending = false
    if priority_probe_count > 0 then
        local data_selector = hit.registers.ds
        dosbox.breakpoint_set(0x01e7, 0x338f, {once = true})
        dosbox.debug_continue()
        local priority_before, priority_before_err =
            wait_hit("SFX priority comparison", event_timeout_ms)
        if priority_before then
            local selected_voice = priority_before.registers.esi
            local table_offset = priority_before.registers.edi
            local priority = {
                comparison = {
                    entry = {
                        segment = priority_before.segment,
                        offset = priority_before.offset,
                    },
                    registers = priority_before.registers,
                    voice_offset = selected_voice,
                    table_offset = table_offset,
                    incoming_priority = (priority_before.registers.edx or 0) & 0xff,
                    voice_before = hex(read_selector(
                        data_selector, selected_voice, 0x50) or ""),
                    effect_record = hex(read_selector(
                        selector, table_offset, 8) or ""),
                },
            }
            priority_record = priority
            event.low_level.priority_selection = priority
            -- Keep this armed while the existing channel-write probe runs;
            -- the comparison routine's return occurs after the write sites.
            dosbox.breakpoint_set(0x01e7, 0x33d4, {once = true})
            priority_pending = true
        else
            dosbox.breakpoint_remove(0x01e7, 0x338f)
            event.low_level.priority_selection_timeout = priority_before_err
        end
    end
    local function finish_priority_selection()
        if not priority_pending or priority_record == nil then return end
        dosbox.debug_continue()
        local priority_after, priority_after_err =
            wait_hit("SFX priority decision return", event_timeout_ms)
        if priority_after then
            local data_selector = hit.registers.ds
            local selected_voice = priority_record.comparison.voice_offset
            priority_record.after = {
                entry = {
                    segment = priority_after.segment,
                    offset = priority_after.offset,
                },
                registers = priority_after.registers,
                voice_after = hex(read_selector(
                    data_selector, selected_voice, 0x50) or ""),
            }
        else
            dosbox.breakpoint_remove(0x01e7, 0x33d4)
            priority_record.after_timeout = priority_after_err
        end
        priority_pending = false
    end
    if caller.segment ~= 0 and caller.offset ~= 0 then
        dosbox.breakpoint_set(0x01e7, 0x33bd, {once = true})
        dosbox.debug_continue()
        local write, write_err = wait_hit("SFX channel write", event_timeout_ms)
        if write then
            local write_registers = write.registers
            local data_selector = hit.registers.ds
            event.low_level.write_stage = {
                entry = {segment = write.segment, offset = write.offset},
                registers = write_registers,
                selected_voice = write_registers.esi,
                flat_bytes = hex(dosbox.mem_read("ds", write_registers.esi, 0x80)),
                driver_bytes = selector ~= 0 and read_selector(selector, write_registers.esi, 0x80)
                    and hex(read_selector(selector, write_registers.esi, 0x80))
                    or nil,
                data_segment_bytes = hex(
                    read_selector(data_selector, 0x2d44, 0x200) or ""
                ),
                game_voice_before = hex(
                    read_selector(data_selector, write_registers.esi, 0x80) or ""
                ),
            }
            dosbox.breakpoint_set(0x01e7, 0x33c1, {once = true})
            dosbox.debug_continue()
            local after_write, after_write_err =
                wait_hit("SFX first channel write complete", event_timeout_ms)
            if after_write then
                event.low_level.write_stage.after_first_write = {
                    entry = {segment = after_write.segment, offset = after_write.offset},
                    registers = after_write.registers,
                    game_voice_hex = hex(
                        read_selector(data_selector, write_registers.esi, 0x80) or ""
                    ),
                }
            else
                event.low_level.write_stage.after_first_write_timeout = after_write_err
                dosbox.breakpoint_remove(0x01e7, 0x33c1)
            end
            finish_priority_selection()
        else
            event.low_level.write_timeout = write_err
            dosbox.breakpoint_remove(0x01e7, 0x33bd)
            finish_priority_selection()
        end
        -- The scheduler is called from the SFX wrapper. Its far-call return
        -- lands at the wrapper's post-call instruction.
        dosbox.breakpoint_set(0x01e7, 0x0ff7, {once = true})
        dosbox.debug_continue()
        local returned, err = wait_hit("SFX scheduler return", event_timeout_ms)
        if returned then
            event.low_level.return_hit = {
                entry = {segment = returned.segment, offset = returned.offset},
                registers = returned.registers,
            }
            -- The driver update loads ES from DS:0x504e immediately after
            -- its entry. Capture it after that load so the selector and the
            -- game voice records can be compared in the same sample.
            dosbox.breakpoint_set(0x01e7, 0x298d, {once = true})
            dosbox.debug_continue()
            local driver_hit, driver_err =
                wait_hit("audio driver update after SFX", event_timeout_ms)
            if driver_hit then
                local data_selector = event.low_level.game_channels_before.selector
                event.low_level.driver_update = {
                    entry = {segment = driver_hit.segment, offset = driver_hit.offset},
                    registers = driver_hit.registers,
                    globals = global_snapshot(),
                    game_channels = game_channel_snapshot(data_selector),
                    driver_bytes = driver_hit.registers.es ~= 0
                        and read_selector(driver_hit.registers.es, 0, 0x400)
                        and hex(read_selector(driver_hit.registers.es, 0, 0x400))
                        or nil,
                }
                if event_driver_probe_count > 0 and macro_probe_count == 0 then
                    event.low_level.driver_updates = {event.low_level.driver_update}
                    for probe = 2, event_driver_probe_count do
                        dosbox.breakpoint_set(0x01e7, 0x298d, {once = true})
                        dosbox.debug_continue()
                        local next_driver, next_driver_err = wait_hit(
                            "SFX consecutive audio driver update", event_timeout_ms)
                        if not next_driver then
                            dosbox.breakpoint_remove(0x01e7, 0x298d)
                            event.low_level.driver_updates_timeout = next_driver_err
                            break
                        end
                        event.low_level.driver_updates[#event.low_level.driver_updates + 1] = {
                            sequence = probe,
                            entry = {segment = next_driver.segment, offset = next_driver.offset},
                            registers = next_driver.registers,
                            globals = global_snapshot(),
                            game_channels = game_channel_snapshot(data_selector),
                            driver_bytes = next_driver.registers.es ~= 0
                                and read_selector(next_driver.registers.es, 0, 0x400)
                                and hex(read_selector(next_driver.registers.es, 0, 0x400))
                                or nil,
                        }
                    end
                end
            else
                event.low_level.driver_update_timeout = driver_err
                dosbox.breakpoint_remove(0x01e7, 0x298d)
            end
            if mixer_probe_count > 0 then
                event.low_level.mixer_commits = {}
                for sample = 1, mixer_probe_count do
                    local captured = false
                    while not captured do
                        dosbox.breakpoint_set(0x01e7, 0x29bf, {once = true})
                        dosbox.debug_continue()
                        local mixer_hit, mixer_err =
                            wait_hit("SFX mixer voice commit", event_timeout_ms)
                        if not mixer_hit then
                            dosbox.breakpoint_remove(0x01e7, 0x29bf)
                            event.low_level.mixer_commits[#event.low_level.mixer_commits + 1] = {
                                sequence = sample,
                                timeout = mixer_err,
                            }
                            captured = true
                        else
                            local data_selector = mixer_hit.registers.ds
                            local voice_offset = (mixer_hit.registers.esi or 0) & 0xffff
                            local mixer_offset = (mixer_hit.registers.ebp or 0) & 0xffff
                            local before = {
                                entry = {segment = mixer_hit.segment, offset = mixer_hit.offset},
                                registers = mixer_hit.registers,
                                voice_offset = voice_offset,
                                mixer_offset = mixer_offset,
                                voice = voice_state_snapshot(data_selector, voice_offset),
                                mixer = mixer_state_snapshot(mixer_hit.registers.fs, mixer_offset),
                            }
                            dosbox.breakpoint_set(0x01e7, 0x29c2, {once = true})
                            dosbox.debug_continue()
                            local mixer_after, mixer_after_err =
                                wait_hit("SFX mixer voice commit complete", event_timeout_ms)
                            if not mixer_after then
                                dosbox.breakpoint_remove(0x01e7, 0x29c2)
                                before.after_timeout = mixer_after_err
                            else
                                before.after = {
                                    entry = {segment = mixer_after.segment, offset = mixer_after.offset},
                                    registers = mixer_after.registers,
                                    voice = voice_state_snapshot(data_selector, voice_offset),
                                    mixer = mixer_state_snapshot(mixer_after.registers.fs, mixer_offset),
                                }
                            end
                            if voice_offset == 0x2e0d then
                                event.low_level.mixer_commits[#event.low_level.mixer_commits + 1] = {
                                    sequence = sample,
                                    before = before,
                                }
                                captured = true
                            end
                        end
                    end
                end
                dosbox.debug_continue()
            end
            if mix_probe_count > 0 then
                event.low_level.mix_entries = {}
                local at_driver_entry = false
                for probe = 1, mix_probe_count do
                    if at_driver_entry then at_driver_entry = false end
                    dosbox.breakpoint_set(0x01e7, 0x1a89, {once = true})
                    dosbox.debug_continue()
                    local mix_hit, mix_err =
                        wait_hit("SFX raw-sample mixer", event_timeout_ms)
                    if not mix_hit then
                        dosbox.breakpoint_remove(0x01e7, 0x1a89)
                        event.low_level.mix_entries[#event.low_level.mix_entries + 1] = {
                            sequence = probe,
                            timeout = mix_err,
                        }
                        break
                    end
                    -- 8C89 uses ES:ESI for the raw SAM byte and the
                    -- unprefixed DS:EDI store for the mixed output buffer.
                    local source_selector = mix_hit.registers.es
                    local output_selector = mix_hit.registers.ds
                    local source_offset = mix_hit.registers.esi or 0
                    local output_offset = mix_hit.registers.edi or 0
                    event.low_level.mix_entries[#event.low_level.mix_entries + 1] = {
                        sequence = probe,
                        entry = {segment = mix_hit.segment, offset = mix_hit.offset},
                        registers = mix_hit.registers,
                        source_selector = source_selector,
                        source_offset = source_offset,
                        source_bytes_hex = hex(read_selector(source_selector, source_offset, 16) or ""),
                        output_selector = output_selector,
                        output_offset = output_offset,
                        output_bytes_hex = hex(read_selector(output_selector, output_offset, 64) or ""),
                        lookup_base = mix_hit.registers.ebx,
                        lookup_row = (mix_hit.registers.ebx or 0) & 0xffffff00,
                        lookup_row_bytes_hex = hex(read_selector(
                            output_selector,
                            (mix_hit.registers.ebx or 0) & 0xffffff00,
                            0x100) or ""),
                        phase = mix_hit.registers.edx,
                        step = mix_hit.registers.ecx,
                    }
                    local current = event.low_level.mix_entries[#event.low_level.mix_entries]
                    local store_offsets = mixer_store_offsets()
                    for _, store_offset in ipairs(store_offsets) do
                        dosbox.breakpoint_set(0x01e7, store_offset, {once = true})
                    end
                    dosbox.debug_continue()
                    local first_write, first_write_err =
                        wait_hit("SFX first dynamic mixed-word write", event_timeout_ms)
                    if first_write then
                        for _, store_offset in ipairs(store_offsets) do
                            dosbox.breakpoint_remove(0x01e7, store_offset)
                        end
                        current.first_write = {
                            entry = {segment = first_write.segment, offset = first_write.offset},
                            registers = first_write.registers,
                            output_offset = first_write.registers.edi,
                            mixed_word = (first_write.registers.eax or 0) & 0xffff,
                            lookup_address = first_write.registers.ebx,
                            source_offset = first_write.registers.esi,
                            source_byte = string.byte(read_selector(
                                first_write.registers.es,
                                first_write.registers.esi,
                                1) or "", 1),
                            lookup_byte = string.byte(read_selector(
                                first_write.registers.ds,
                                first_write.registers.ebx,
                                1) or "", 1),
                            bytes_hex = hex(read_selector(
                                first_write.registers.ds,
                                first_write.registers.edi,
                                16) or ""),
                        }
                    else
                        for _, store_offset in ipairs(store_offsets) do
                            dosbox.breakpoint_remove(0x01e7, store_offset)
                        end
                        current.first_write_timeout = first_write_err
                    end
                    if output_probe_count > 0 then
                        dosbox.breakpoint_set(0x01e7, 0x1f7f, {once = true})
                        dosbox.debug_continue()
                        local output_entry, output_entry_err =
                            wait_hit("SFX mixed-word output converter", event_timeout_ms)
                        if output_entry then
                            local output = {
                                entry = {
                                    segment = output_entry.segment,
                                    offset = output_entry.offset,
                                },
                                registers = output_entry.registers,
                                input_selector = output_entry.registers.ds,
                                input_offset = output_entry.registers.esi,
                                output_selector = output_entry.registers.es,
                                output_offset = output_entry.registers.edi,
                                lookup_base = output_entry.registers.ebx,
                                lookup_table_hex = hex(read_selector(
                                    output_entry.registers.ds,
                                    output_entry.registers.ebx,
                                    0x400) or ""),
                                count = output_entry.registers.ecx,
                            }
                            if output_input_word ~= nil then
                                dosbox.mem_write_selector(
                                    output_entry.registers.ds,
                                    output_entry.registers.esi,
                                    word_bytes(output_input_word))
                                output.input_override = output_input_word & 0xffff
                            end
                            local converter_store_offsets = output_store_offsets()
                            for _, store_offset in ipairs(converter_store_offsets) do
                                dosbox.breakpoint_set(0x01e7, store_offset, {once = true})
                            end
                            dosbox.debug_continue()
                            local output_store, output_store_err =
                                wait_hit("SFX first DMA-byte output store", event_timeout_ms)
                            if output_store then
                                for _, store_offset in ipairs(converter_store_offsets) do
                                    dosbox.breakpoint_remove(0x01e7, store_offset)
                                end
                                local input_offset = (output_store.registers.esi or 0) - 2
                                local input_bytes = read_selector(
                                    output_store.registers.ds, input_offset, 2) or ""
                                local input_word = string.byte(input_bytes, 1)
                                local input_hi = string.byte(input_bytes, 2)
                                input_word = input_word and input_hi
                                    and (input_word | (input_hi << 8)) or nil
                                output.first_store = {
                                    entry = {
                                        segment = output_store.segment,
                                        offset = output_store.offset,
                                    },
                                    registers = output_store.registers,
                                    input_offset = input_offset,
                                    input_word = input_word,
                                    lookup_address = (output_store.registers.ebx or 0)
                                        + (input_word or 0),
                                    converted_word = (output_store.registers.eax or 0) & 0xffff,
                                    output_byte = (output_store.registers.eax or 0) & 0xff,
                                    output_offset = output_store.registers.edi,
                                    bytes_hex = hex(read_selector(
                                        output_store.registers.es,
                                        output_store.registers.edi,
                                        8) or ""),
                                }
                            else
                                for _, store_offset in ipairs(converter_store_offsets) do
                                    dosbox.breakpoint_remove(0x01e7, store_offset)
                                end
                                output.first_store_timeout = output_store_err
                            end
                            current.output_conversion = output
                        else
                            dosbox.breakpoint_remove(0x01e7, 0x1f7f)
                            current.output_conversion_timeout = output_entry_err
                        end
                    end
                    -- Wait for the next driver entry, then inspect the same
                    -- output location after the mixer body has completed.
                    dosbox.breakpoint_set(0x01e7, 0x298d, {once = true})
                    dosbox.debug_continue()
                    local next_driver, next_driver_err =
                        wait_hit("SFX mix output completion", event_timeout_ms)
                    if next_driver then
                        current.output_after_driver = {
                            entry = {segment = next_driver.segment, offset = next_driver.offset},
                            registers = next_driver.registers,
                            bytes_hex = hex(read_selector(output_selector, output_offset, 64) or ""),
                        }
                        at_driver_entry = true
                    else
                        dosbox.breakpoint_remove(0x01e7, 0x298d)
                        current.output_after_timeout = next_driver_err
                        break
                    end
                end
                dosbox.debug_continue()
            end
            if macro_probe_count > 0 then
                event.low_level.macro_dispatch = {}
                for probe = 1, macro_probe_count do
                    dosbox.breakpoint_set(0x01e7, 0x2b74, {once = true})
                    dosbox.debug_continue()
                    local macro_hit, macro_err =
                        wait_hit("SFX macro dispatch", event_timeout_ms)
                    if not macro_hit then
                        dosbox.breakpoint_remove(0x01e7, 0x2b74)
                        event.low_level.macro_dispatch[#event.low_level.macro_dispatch + 1] = {
                            sequence = probe,
                            timeout = macro_err,
                        }
                        break
                    end
                    local data_selector = macro_hit.registers.ds
                    local voice_offset = macro_hit.registers.esi or 0
                    local voice_raw = read_selector(data_selector, voice_offset, 0x50)
                    local packed_raw = dosbox.mem_read("ds", 0x2fdc, 4)
                    local command_id = (macro_hit.registers.ebx or 0) & 0xff
                    event.low_level.macro_dispatch[#event.low_level.macro_dispatch + 1] = {
                        sequence = probe,
                        entry = {segment = macro_hit.segment, offset = macro_hit.offset},
                        registers = macro_hit.registers,
                        voice_offset = voice_offset,
                        macro_pointer = voice_raw and word(voice_raw, 0x04 + 1) or nil,
                        macro_step = voice_raw and word(voice_raw, 0x02 + 1) or nil,
                        command_id = command_id,
                        command_argument_word = (macro_hit.registers.eax or 0) & 0xffff,
                        command_state_hex = packed_raw and hex(packed_raw) or nil,
                        voice_bytes_hex = voice_raw and hex(voice_raw) or nil,
                        voice_wait_counter = voice_raw and word(voice_raw, 0x26 + 1) or nil,
                        voice_control = voice_raw and string.byte(voice_raw, 0x2c + 1) or nil,
                        scheduler = scheduler_snapshot(),
                    }
                    -- Leave the current EIP before re-arming 0x2B74. The
                    -- one-instruction barrier avoids re-catching the same
                    -- decode before the indirect command dispatch runs.
                    dosbox.breakpoint_set(0x01e7, 0x2b79, {once = true})
                    dosbox.debug_continue()
                    local dispatch, dispatch_err =
                        wait_hit("SFX macro dispatch barrier", event_timeout_ms)
                    if not dispatch then
                        dosbox.breakpoint_remove(0x01e7, 0x2b79)
                        event.low_level.macro_dispatch[#event.low_level.macro_dispatch + 1] = {
                            sequence = probe + 1,
                            timeout = dispatch_err,
                        }
                        break
                    end
                end
                if event_driver_probe_count > 0 then
                    event.low_level.driver_updates_after_macro = {}
                    local data_selector = event.low_level.game_channels_before.selector
                    for probe = 1, event_driver_probe_count do
                        dosbox.breakpoint_set(0x01e7, 0x298d, {once = true})
                        dosbox.debug_continue()
                        local next_driver, next_driver_err = wait_hit(
                            "SFX post-macro audio driver update", event_timeout_ms)
                        if not next_driver then
                            dosbox.breakpoint_remove(0x01e7, 0x298d)
                            event.low_level.driver_updates_after_macro_timeout = next_driver_err
                            break
                        end
                        event.low_level.driver_updates_after_macro[#event.low_level.driver_updates_after_macro + 1] = {
                            sequence = probe,
                            entry = {segment = next_driver.segment, offset = next_driver.offset},
                            registers = next_driver.registers,
                            globals = global_snapshot(),
                            game_channels = game_channel_snapshot(data_selector),
                            driver_bytes = next_driver.registers.es ~= 0
                                and read_selector(next_driver.registers.es, 0, 0x400)
                                and hex(read_selector(next_driver.registers.es, 0, 0x400))
                                or nil,
                        }
                    end
                end
                dosbox.debug_continue()
            end
            if interpreter_probe_count > 0 then
                event.low_level.interpreter = {}
                for probe = 1, interpreter_probe_count do
                    dosbox.breakpoint_set(0x01e7, 0x3237, {once = true})
                    dosbox.debug_continue()
                    local interpreter_hit, interpreter_err =
                        wait_hit("SFX effect interpreter", event_timeout_ms)
                    if not interpreter_hit then
                        dosbox.breakpoint_remove(0x01e7, 0x3237)
                        event.low_level.interpreter[#event.low_level.interpreter + 1] = {
                            sequence = probe,
                            timeout = interpreter_err,
                        }
                        break
                    end
                    local packed = dosbox.mem_read("ds", 0x2fdc, 4)
                    event.low_level.interpreter[#event.low_level.interpreter + 1] = {
                        sequence = probe,
                        entry = {segment = interpreter_hit.segment, offset = interpreter_hit.offset},
                        registers = interpreter_hit.registers,
                        packed_before_hex = hex(packed),
                        packed_from_ecx_hex = hex(dword_bytes(interpreter_hit.registers.ecx or 0)),
                        state_before_hex = hex(dosbox.mem_read("ds", 0x2fdc, 4)),
                        voice_record_before_hex = (interpreter_hit.registers.ds ~= 0
                            and read_selector(interpreter_hit.registers.ds,
                                               interpreter_hit.registers.esi, 0x80)
                            and hex(read_selector(interpreter_hit.registers.ds,
                                                   interpreter_hit.registers.esi, 0x80)))
                            or nil,
                    }
                    -- The interpreter's voice writes complete at 0x3322,
                    -- just before it restores its saved packed state.
                    -- Capture there so the initialized voice record is
                    -- observed after the command has been applied.
                    dosbox.breakpoint_set(0x01e7, 0x3322, {once = true})
                    dosbox.debug_continue()
                    local advance, advance_err =
                        wait_hit("SFX interpreter state handoff", event_timeout_ms)
                    local current = event.low_level.interpreter[#event.low_level.interpreter]
                    if not advance then
                        dosbox.breakpoint_remove(0x01e7, 0x3322)
                        current.advance_timeout = advance_err
                        break
                    end
                    current.after = {
                        entry = {segment = advance.segment, offset = advance.offset},
                        registers = advance.registers,
                        state_after_hex = hex(dosbox.mem_read("ds", 0x2fdc, 4)),
                        voice_record_after_hex = (advance.registers.ds ~= 0
                            and read_selector(advance.registers.ds,
                                               advance.registers.esi, 0x80)
                            and hex(read_selector(advance.registers.ds,
                                                   advance.registers.esi, 0x80)))
                            or nil,
                    }
                end
                dosbox.debug_continue()
            end
            if dma_probe then
                dosbox.breakpoint_set(0x01e7, 0x190e, {once = true})
                dosbox.debug_continue()
                local dma_hit, dma_err = wait_hit("Sound Blaster DMA")
                if dma_hit then
                    event.low_level.dma = {
                        entry = {segment = dma_hit.segment, offset = dma_hit.offset},
                        registers = dma_hit.registers,
                        globals = global_snapshot(),
                    }
                else
                    event.low_level.dma_timeout = dma_err
                    dosbox.breakpoint_remove(0x01e7, 0x190e)
                end
            end
            if irq_probe then
                dosbox.breakpoint_set(0x01e7, 0x17f1, {once = true})
                dosbox.debug_continue()
                local irq_hit, irq_err = wait_hit("Sound Blaster IRQ")
                if irq_hit then
                    event.low_level.irq = {
                        entry = {segment = irq_hit.segment, offset = irq_hit.offset},
                        registers = irq_hit.registers,
                        globals = global_snapshot(),
                    }
                else
                    event.low_level.irq_timeout = irq_err
                    dosbox.breakpoint_remove(0x01e7, 0x17f1)
                end
            end
            if selector ~= 0 then
                event.low_level.channels_after = channel_snapshot(selector)
            end
            local write_stage = event.low_level.write_stage
            if write_stage then
                event.low_level.game_voice_after = hex(
                    read_selector(
                        write_stage.registers.ds,
                        write_stage.selected_voice,
                        0x80
                    ) or ""
                )
            end
            event.low_level.game_channels_after =
                game_channel_snapshot(returned.registers.ds)
        else
            event.low_level.return_timeout = err
            dosbox.breakpoint_remove(0x01e7, 0x0ff7)
        end
    else
        finish_priority_selection()
        dosbox.debug_continue()
    end
end

local function capture_priority_only(low, event)
    local data_selector = low.registers.ds
    dosbox.breakpoint_set(0x01e7, 0x338f, {once = true})
    dosbox.debug_continue()
    local comparison, comparison_err = wait_hit(
        "priority-only comparison", event_timeout_ms)
    if not comparison then
        dosbox.breakpoint_remove(0x01e7, 0x338f)
        event.low_level.priority_only_timeout = comparison_err
        dosbox.debug_continue()
        return
    end
    local selected_voice = comparison.registers.esi or 0
    local table_offset = comparison.registers.edi or 0
    local table_selector = comparison.registers.es
    local effect_bytes = read_selector(table_selector, table_offset, 8) or ""
    local record = {
        comparison = {
            entry = {segment = comparison.segment, offset = comparison.offset},
            registers = comparison.registers,
            voice_offset = selected_voice,
            table_offset = table_offset,
            incoming_priority = (comparison.registers.edx or 0) & 0xff,
            voice_before = hex(read_selector(
                data_selector, selected_voice, 0x50) or ""),
            voice_before_state = voice_state_snapshot(data_selector, selected_voice),
            effect_record = hex(effect_bytes),
            effect_record_bytes = byte_values(effect_bytes),
        },
    }
    dosbox.breakpoint_set(0x01e7, 0x33a2, {once = true})
    dosbox.breakpoint_set(0x01e7, 0x33d4, {once = true})
    dosbox.debug_continue()
    local branch_or_return, branch_or_return_err = wait_hit(
        "priority-only high-bit branch or selection return", event_timeout_ms)
    local returned = branch_or_return
    local returned_err = branch_or_return_err
    if branch_or_return and branch_or_return.offset == 0x33a2 then
        local branch_selector = branch_or_return.registers.es
        local branch_table_offset = (branch_or_return.registers.edi or 0) & 0xffff
        local branch_voice_offset = (branch_or_return.registers.esi or 0) & 0xffff
        local branch_entry = read_selector(branch_selector, branch_table_offset, 8) or ""
        record.branch_check = {
            entry = {segment = branch_or_return.segment, offset = branch_or_return.offset},
            registers = branch_or_return.registers,
            table_priority_byte = string.byte(branch_entry, 6),
            table_entry_bytes = byte_values(branch_entry),
            voice_status = dosbox.mem_read_word(
                "ds", branch_voice_offset + 0x3b),
            voice_table_pointer = dosbox.mem_read_word(
                "ds", branch_voice_offset + 0x41),
        }
        dosbox.breakpoint_remove(0x01e7, 0x33a2)
        dosbox.debug_continue()
        returned, returned_err = wait_hit(
            "priority-only selection return", event_timeout_ms)
    elseif branch_or_return then
        dosbox.breakpoint_remove(0x01e7, 0x33a2)
    end
    if returned then
        record.after = {
            entry = {segment = returned.segment, offset = returned.offset},
            registers = returned.registers,
            voice_after = hex(read_selector(
                data_selector, selected_voice, 0x50) or ""),
            voice_after_state = voice_state_snapshot(data_selector, selected_voice),
        }
    else
        dosbox.breakpoint_remove(0x01e7, 0x33a2)
        dosbox.breakpoint_remove(0x01e7, 0x33d4)
        record.after_timeout = returned_err
    end
    if returned and priority_irq_status_probe_count > 0 then
        record.irq_updates = {}
        for probe = 1, priority_irq_status_probe_count do
            dosbox.breakpoint_set(0x01e7, 0x17f1, {once = true})
            dosbox.debug_continue()
            local irq_hit, irq_err = wait_hit(
                "priority-only audio IRQ probe", event_timeout_ms)
            dosbox.breakpoint_remove(0x01e7, 0x17f1)
            if not irq_hit then
                record.irq_updates[#record.irq_updates + 1] = {
                    sequence = probe,
                    timeout = irq_err,
                }
                break
            end
            local irq_record = {
                sequence = probe,
                entry = {segment = irq_hit.segment, offset = irq_hit.offset},
                registers = irq_hit.registers,
                globals = global_snapshot(),
                voice = voice_state_snapshot(data_selector, selected_voice),
                scheduler = scheduler_snapshot(),
            }
            dosbox.breakpoint_set(0x01e7, 0x2984, {once = true})
            dosbox.debug_continue()
            local driver_hit, driver_err = wait_hit(
                "priority-only driver update after audio IRQ", event_timeout_ms)
            dosbox.breakpoint_remove(0x01e7, 0x2984)
            if driver_hit then
                irq_record.driver_update = {
                    entry = {segment = driver_hit.segment, offset = driver_hit.offset},
                    registers = driver_hit.registers,
                    globals = global_snapshot(),
                    voice = voice_state_snapshot(data_selector, selected_voice),
                    scheduler = scheduler_snapshot(),
                }
            else
                irq_record.driver_update_timeout = driver_err
            end
            record.irq_updates[#record.irq_updates + 1] = irq_record
        end
    elseif returned and priority_status_probe_count > 0 then
        record.driver_updates = {}
        for probe = 1, priority_status_probe_count do
            -- 0x29cc is immediately after all eight per-voice updates;
            -- probing inside the loop would sample one voice iteration,
            -- not one complete driver tick.
            dosbox.breakpoint_set(0x01e7, 0x29cc, {once = true})
            dosbox.debug_continue()
            local driver_hit, driver_err = wait_hit(
                "priority-only driver status probe", event_timeout_ms)
            if not driver_hit then
                dosbox.breakpoint_remove(0x01e7, 0x29cc)
                record.driver_updates[#record.driver_updates + 1] = {
                    sequence = probe,
                    timeout = driver_err,
                }
                break
            end
            record.driver_updates[#record.driver_updates + 1] = {
                sequence = probe,
                entry = {segment = driver_hit.segment, offset = driver_hit.offset},
                registers = driver_hit.registers,
                voice = voice_state_snapshot(data_selector, selected_voice),
                scheduler = scheduler_snapshot(),
            }
        end
    end
    event.low_level.priority_selection = record
    dosbox.debug_continue()
end

local function capture_one_event(sequence)
    dosbox.breakpoint_set(0x01e7, 0x0fcf, {once = true})
    dosbox.debug_continue()
    local hit, err = wait_hit("SFX entry", event_timeout_ms)
    if not hit then
        dosbox.breakpoint_remove(0x01e7, 0x0fcf)
        return nil, err
    end
    local event = capture_sfx_entry(hit, sequence)
    dosbox.breakpoint_set(0x01e7, 0x3360, {once = true})
    dosbox.debug_continue()
    local low, low_err = wait_hit("SFX scheduler", event_timeout_ms)
    if low then
        if priority_only then
            event.low_level = {
                entry = {segment = low.segment, offset = low.offset},
                registers = low.registers,
                table = low.registers.es ~= 0
                    and table_snapshot(low.registers.es) or nil,
            }
            capture_priority_only(low, event)
        else
            capture_low_level(low, event)
        end
    else
        dosbox.breakpoint_remove(0x01e7, 0x3360)
        event.scheduler_timeout = low_err
        dosbox.debug_continue()
    end
    return event
end

local function object_at_hit(hit)
    local registers = hit.registers or {}
    local selector = registers.es
    local offset = (registers.edi or 0) & 0xffff
    if selector == nil then return nil end
    local raw = read_selector(selector, offset, 0x40)
    if not raw or #raw < 0x40 then return nil end
    return object_snapshot(raw, selector, offset, -1)
end

local function capture_callsite(sequence)
    local targets = {
        {id = 8, offset = 0x4582, after = 0x4587},
        {id = 2, offset = 0x4bca, after = 0x4c5a},
    }
    for _, target in ipairs(targets) do
        dosbox.breakpoint_set(0x01f7, target.offset, {once = true})
    end
    dosbox.debug_continue()
    local hit, err = wait_hit("SFX object callsite", event_timeout_ms)
    for _, target in ipairs(targets) do
        dosbox.breakpoint_remove(0x01f7, target.offset)
    end
    if not hit then return nil, err end

    local target_id = nil
    local after_offset = nil
    for _, target in ipairs(targets) do
        if hit.offset == target.offset then
            target_id = target.id
            after_offset = target.after
        end
    end
    local event = {
        sequence = sequence,
        id = target_id,
        callsite = {segment = hit.segment, offset = hit.offset},
        registers = hit.registers,
        caller = caller_from_stack(hit.registers, 0),
        globals_before = global_snapshot(),
        player = player_snapshot(),
        object_before = object_at_hit(hit),
        pool_before = pool_snapshot(),
    }

    dosbox.breakpoint_set(0x01f7, after_offset, {once = true})
    dosbox.debug_continue()
    local after, after_err = wait_hit("SFX object callsite completion", event_timeout_ms)
    if after then
        event.after = {segment = after.segment, offset = after.offset}
        event.globals_after = global_snapshot()
        event.object_after = object_at_hit(after)
        event.pool_after = pool_snapshot()
    else
        dosbox.breakpoint_remove(0x01f7, after_offset)
        event.after_timeout = after_err
        dosbox.debug_continue()
    end
    return event
end

local function player_overlap_table_snapshot()
    local count = dosbox.mem_read_word("ds", 0x8806)
    local cursor = dosbox.mem_read_word("ds", 0x8808)
    local entries = {}
    local limit = math.min(count, 16)
    for index = 0, limit - 1 do
        local raw = dosbox.mem_read("ds", 0x87de + index * 4, 4)
        entries[#entries + 1] = {
            index = index,
            x = word(raw, 1),
            y = word(raw, 3),
            raw_hex = hex(raw),
        }
    end
    return {
        count = count,
        cursor = cursor,
        entries = entries,
    }
end

local function targeted_specs()
    if targeted_probe == "id2" then
        return {
            {label = "crab-update-entry", offset = 0x778c, after = 0x7791},
            {label = "fish-update-entry", offset = 0x7b71, after = 0x7b76},
            {label = "player-overlap-a", offset = 0x7ad3, after = 0x7ad9},
            {label = "player-overlap-b", offset = 0x7e68, after = 0x7e6e},
            {label = "id2-effect-write", offset = 0x4bc4, after = 0x4bcf},
        }
    end
    if targeted_probe == "id2-crab" then
        return {
            {label = "crab-update-entry", offset = 0x778c, after = 0x7791},
            {label = "player-overlap-a", offset = 0x7ad3, after = 0x7ad9},
            {label = "id2-effect-write", offset = 0x4bc4, after = 0x4bcf},
        }
    end
    if targeted_probe == "id11" then
        return {
            {label = "letter-update", offset = 0x8d20, after = 0x8d2d},
            {label = "letter-overlap", offset = 0x8d60, after = 0x8e42},
            {label = "id11-effect-write", offset = 0x8e37, after = 0x8e3d},
        }
    end
    if targeted_probe == "id11-natural" then
        return {
            {label = "letter-update", offset = 0x8d20, after = 0x8d2d},
        }
    end
    if targeted_probe == "contact-producer" then
        return {
            {label = "player-contact-producer", offset = 0x3909, after = 0x3914},
        }
    end
    return {}
end

local function capture_targeted_probe(sequence)
    local specs = targeted_specs()
    for _, spec in ipairs(specs) do
        dosbox.breakpoint_set(0x01f7, spec.offset, {once = true})
    end
    dosbox.debug_continue()
    local hit, err = wait_hit("targeted SFX guard", event_timeout_ms)
    for _, spec in ipairs(specs) do
        dosbox.breakpoint_remove(0x01f7, spec.offset)
    end
    if not hit then return nil, err end

    local selected = nil
    for _, spec in ipairs(specs) do
        if hit.offset == spec.offset then
            selected = spec
            break
        end
    end
    local event = {
        sequence = sequence,
        probe = targeted_probe,
        target = selected and selected.label or "unknown",
        entry = {segment = hit.segment, offset = hit.offset},
        registers = hit.registers,
        caller = caller_from_stack(hit.registers, 0),
        globals = global_snapshot(),
        player = player_snapshot(),
        object = object_at_hit(hit),
        pool_before = pool_snapshot(),
        overlap_table = (targeted_probe == "id2" or targeted_probe == "id2-crab")
            and player_overlap_table_snapshot() or nil,
        letter_state = targeted_probe == "id11" and {
            state = dosbox.mem_read_word("es", (hit.registers.edi or 0) + 0x2c),
            counter_0x2e = dosbox.mem_read_word(
                "es", (hit.registers.edi or 0) + 0x2e),
            mask = dosbox.mem_read_word("es", (hit.registers.edi or 0) + 0x2a),
            collected_mask = dosbox.mem_read_word("ds", 0x60d8),
            level_selector = dosbox.mem_read_word("ds", 0x85d4),
        } or nil,
    }

    if targeted_probe == "contact-producer" and selected ~= nil and
            selected.label == "player-contact-producer" then
        event.contact_producer = {
            player_before = event.player,
            object_before = event.object,
            ring_before = player_overlap_table_snapshot(),
        }

        dosbox.breakpoint_set(0x01f7, 0x0e06, {once = true})
        dosbox.debug_continue()
        local factory, factory_err = wait_hit(
            "contact-effect object factory", event_timeout_ms)
        dosbox.breakpoint_remove(0x01f7, 0x0e06)
        if not factory then
            event.contact_producer.factory_timeout = factory_err
            return event
        end
        event.contact_producer.factory = {
            entry = {segment = factory.segment, offset = factory.offset},
            registers = factory.registers,
            callback_selector = factory.registers.es,
            callback_offset = (factory.registers.eax or 0) & 0xffff,
            ring_after_factory = player_overlap_table_snapshot(),
        }

        dosbox.breakpoint_set(0x01f7, 0x4519, {once = true})
        dosbox.debug_continue()
        local entry, entry_err = wait_hit(
            "contact-effect entry callback", event_timeout_ms)
        if not entry then
            dosbox.breakpoint_remove(0x01f7, 0x4519)
            event.contact_producer.entry_timeout = entry_err
            dosbox.debug_continue()
            return event
        end
        event.contact_producer.entry = {
            entry = {segment = entry.segment, offset = entry.offset},
            registers = entry.registers,
            object = object_at_hit(entry),
            ring_after_entry = player_overlap_table_snapshot(),
            globals = global_snapshot(),
        }
        return event
    end

    if targeted_probe == "id11-natural" and selected ~= nil and
            selected.label == "letter-update" then
        event.natural_probe = true
        dosbox.breakpoint_set(0x01f7, 0x8e37, {once = true})
        dosbox.breakpoint_set(0x01f7, 0x8e42, {once = true})
        dosbox.debug_continue()
        local branch, branch_err = wait_hit(
            "natural ID11 effect branch", event_timeout_ms)
        dosbox.breakpoint_remove(0x01f7, 0x8e37)
        dosbox.breakpoint_remove(0x01f7, 0x8e42)
        if not branch then
            event.natural_branch_timeout = branch_err
            dosbox.debug_continue()
            return event
        end
        event.natural_branch = {
            entry = {segment = branch.segment, offset = branch.offset},
            registers = branch.registers,
            player = player_snapshot(),
            object = object_at_hit(branch),
            globals = global_snapshot(),
            letter_state = {
                state = dosbox.mem_read_word(
                    "es", (branch.registers.edi or 0) + 0x2c),
                mask = dosbox.mem_read_word(
                    "es", (branch.registers.edi or 0) + 0x2a),
                collected_mask = dosbox.mem_read_word("ds", 0x60d8),
            },
        }
        return event
    end

    if (targeted_probe == "id2" or targeted_probe == "id2-crab") and selected ~= nil and
            (selected.label == "fish-update-entry" or
             selected.label == "crab-update-entry") then
        local object = event.object
        if object ~= nil then
            dosbox.mem_write("ds", 0x8806, word_bytes(1))
            dosbox.mem_write("ds", 0x8808, word_bytes(0))
            dosbox.mem_write("ds", 0x87de, word_bytes(object.x))
            dosbox.mem_write("ds", 0x87e0, word_bytes(object.y))
            event.contact_ring_seed = {
                count = 1,
                cursor = 0,
                x = object.x,
                y = object.y,
            }

            local crab = selected.label == "crab-update-entry"
            local scanOffset = crab and 0x7a85 or 0x7e1a
            local comparisonOffset = crab and 0x7ad3 or 0x7e68
            local guards = {
                {label = crab and "crab-contact-scan" or "fish-contact-scan",
                 offset = scanOffset},
                {label = crab and "player-overlap-a" or "player-overlap-b",
                 offset = comparisonOffset},
            }
            for _, guard in ipairs(guards) do
                dosbox.breakpoint_set(0x01f7, guard.offset, {once = true})
            end
            dosbox.debug_continue()
            local contact, contact_err = wait_hit(
                "targeted SFX contact guard", event_timeout_ms)
            for _, guard in ipairs(guards) do
                dosbox.breakpoint_remove(0x01f7, guard.offset)
            end
            if not contact then
                event.contact_timeout = contact_err
                return event
            end

            local scan = nil
            if contact.offset == scanOffset then
                local scan_object = object_at_hit(contact)
                scan = {
                    entry = {segment = contact.segment, offset = contact.offset},
                    registers = contact.registers,
                    object = scan_object,
                    overlap_table = player_overlap_table_snapshot(),
                }
                if scan_object ~= nil then
                    dosbox.mem_write("ds", 0x8806, word_bytes(1))
                    dosbox.mem_write("ds", 0x8808, word_bytes(0))
                    dosbox.mem_write("ds", 0x87de, word_bytes(scan_object.x))
                    dosbox.mem_write("ds", 0x87e0, word_bytes(scan_object.y))
                end
                dosbox.breakpoint_set(0x01f7, comparisonOffset, {once = true})
                dosbox.debug_continue()
                local followup, followup_err = wait_hit(
                    "targeted SFX contact comparison", event_timeout_ms)
                dosbox.breakpoint_remove(0x01f7, comparisonOffset)
                if not followup then
                    event.contact_timeout = followup_err
                    event.contact_scan = scan
                    return event
                end
                contact = followup
            end

            local contact_label = crab and "player-overlap-a" or "player-overlap-b"
            event.contact = {
                target = contact_label,
                entry = {segment = contact.segment, offset = contact.offset},
                registers = contact.registers,
                object = object_at_hit(contact),
                overlap_table = player_overlap_table_snapshot(),
                globals = global_snapshot(),
            }
            event.contact.scan = scan

            local contact_after_offset = contact.offset == 0x7ad3
                and 0x7ad9 or 0x7e6e
            dosbox.breakpoint_set(0x01f7, contact_after_offset, {once = true})
            dosbox.debug_continue()
            local contact_after, contact_after_err = wait_hit(
                "targeted SFX contact completion", event_timeout_ms)
            if not contact_after then
                dosbox.breakpoint_remove(0x01f7, contact_after_offset)
                event.contact_after_timeout = contact_after_err
                dosbox.debug_continue()
                return event
            end
            event.contact.after = {
                segment = contact_after.segment,
                offset = contact_after.offset,
            }
            event.contact.object_after = object_at_hit(contact_after)
            event.contact.globals_after = global_snapshot()
            event.contact.pool_after = pool_snapshot()

            dosbox.breakpoint_set(0x01f7, 0x4bc4, {once = true})
            dosbox.debug_continue()
            local effect_write, effect_err = wait_hit(
                "targeted ID2 effect write", event_timeout_ms)
            if effect_write then
                event.contact.effect_write = {
                    entry = {segment = effect_write.segment,
                             offset = effect_write.offset},
                    registers = effect_write.registers,
                    globals = global_snapshot(),
                }
                dosbox.breakpoint_set(0x01f7, 0x4bcf, {once = true})
                dosbox.debug_continue()
                local effect_after, effect_after_err = wait_hit(
                    "targeted ID2 effect write completion", event_timeout_ms)
                if effect_after then
                    event.contact.effect_write.after = {
                        segment = effect_after.segment,
                        offset = effect_after.offset,
                    }
                    event.contact.effect_write.globals_after = global_snapshot()
                    event.contact.effect_write.pool_after = pool_snapshot()

                    event.contact.effect_write.allocations = {}
                    local allocation_returns = {0x4bda, 0x4c09, 0x4c38}
                    for allocation_index, allocation_return_offset in ipairs(allocation_returns) do
                        dosbox.breakpoint_set(0x01f7, allocation_return_offset, {once = true})
                        dosbox.debug_continue()
                        local allocation, allocation_err = wait_hit(
                            "targeted ID2 pooled allocation return", event_timeout_ms)
                        if not allocation then
                            dosbox.breakpoint_remove(0x01f7, allocation_return_offset)
                            event.contact.effect_write.allocations_timeout = allocation_err
                            dosbox.debug_continue()
                            break
                        end
                        event.contact.effect_write.allocations[#event.contact.effect_write.allocations + 1] = {
                            sequence = allocation_index,
                            entry = {segment = allocation.segment,
                                     offset = allocation.offset},
                            registers = allocation.registers,
                            object = object_at_hit(allocation),
                            pool = pool_snapshot(),
                        }
                    end

                    if event.contact.effect_write.allocations_timeout == nil then
                        dosbox.breakpoint_set(0x01f7, 0x4c5c, {once = true})
                        dosbox.debug_continue()
                        local handler_return, handler_return_err = wait_hit(
                            "targeted ID2 handler completion", event_timeout_ms)
                        if handler_return then
                            event.contact.effect_write.handler_return = {
                                entry = {segment = handler_return.segment,
                                         offset = handler_return.offset},
                                registers = handler_return.registers,
                                object = object_at_hit(handler_return),
                                pool = pool_snapshot(),
                            }

                            event.contact.effect_write.spawn_updates = {}
                            for update_index = 1, 3 do
                                dosbox.breakpoint_set(0x01f7, 0x4ec9, {once = true})
                                dosbox.debug_continue()
                                local update_entry, update_entry_err = wait_hit(
                                    "targeted ID2 spawned-object callback", event_timeout_ms)
                                if not update_entry then
                                    dosbox.breakpoint_remove(0x01f7, 0x4ec9)
                                    event.contact.effect_write.spawn_updates_timeout = update_entry_err
                                    dosbox.debug_continue()
                                    break
                                end
                                local update = {
                                    sequence = update_index,
                                    entry = {segment = update_entry.segment,
                                             offset = update_entry.offset},
                                    registers = update_entry.registers,
                                    object_before = object_at_hit(update_entry),
                                }
                                dosbox.breakpoint_set(0x01f7, 0x4f81, {once = true})
                                dosbox.debug_continue()
                                local update_return, update_return_err = wait_hit(
                                    "targeted ID2 spawned-object callback completion", event_timeout_ms)
                                if not update_return then
                                    dosbox.breakpoint_remove(0x01f7, 0x4f81)
                                    update.return_timeout = update_return_err
                                    event.contact.effect_write.spawn_updates_timeout = update_return_err
                                    event.contact.effect_write.spawn_updates[#event.contact.effect_write.spawn_updates + 1] = update
                                    dosbox.debug_continue()
                                    break
                                end
                                update.return_entry = {
                                    segment = update_return.segment,
                                    offset = update_return.offset,
                                }
                                update.object_after = object_at_hit(update_return)
                                update.pool_after = pool_snapshot()
                                event.contact.effect_write.spawn_updates[#event.contact.effect_write.spawn_updates + 1] = update
                            end
                        else
                            dosbox.breakpoint_remove(0x01f7, 0x4c5c)
                            event.contact.effect_write.handler_return_timeout = handler_return_err
                            dosbox.debug_continue()
                        end
                    end
                else
                    dosbox.breakpoint_remove(0x01f7, 0x4bcf)
                    event.contact.effect_write.after_timeout = effect_after_err
                    dosbox.debug_continue()
                end
            else
                dosbox.breakpoint_remove(0x01f7, 0x4bc4)
                event.contact.effect_write_timeout = effect_err
                dosbox.debug_continue()
            end
        end
        return event
    end

    if targeted_probe == "id11" and selected ~= nil and
            selected.label == "letter-update" then
        local object = event.object
        if object ~= nil then
            event.player_before_control = player_snapshot()
            event.player_control = write_live_player_position(object.x, object.y)
            dosbox.breakpoint_set(0x01f7, 0x8d25, {once = true})
            dosbox.debug_continue()
            local gate, gate_err = wait_hit(
                "targeted ID11 visibility gate", event_timeout_ms)
            dosbox.breakpoint_remove(0x01f7, 0x8d25)
            if not gate then
                event.visibility_gate_timeout = gate_err
                return event
            end
            event.visibility_gate = {
                entry = {segment = gate.segment, offset = gate.offset},
                registers = gate.registers,
                accepted = (gate.registers.flags & 1) == 0,
                object = object_at_hit(gate),
                globals = global_snapshot(),
            }
            if not event.visibility_gate.accepted then
                return event
            end
            dosbox.breakpoint_set(0x01f7, 0x8d2d, {once = true})
            dosbox.debug_continue()
            local state_entry, state_entry_err = wait_hit(
                "targeted ID11 state entry", event_timeout_ms)
            dosbox.breakpoint_remove(0x01f7, 0x8d2d)
            if not state_entry then
                event.state_entry_timeout = state_entry_err
                return event
            end
            event.state_entry = {
                entry = {segment = state_entry.segment, offset = state_entry.offset},
                registers = state_entry.registers,
                object = object_at_hit(state_entry),
                globals = global_snapshot(),
            }
            dosbox.breakpoint_set(0x01f7, 0x8d36, {once = true})
            dosbox.debug_continue()
            local helper, helper_err = wait_hit(
                "targeted ID11 player-bounds helper", event_timeout_ms)
            dosbox.breakpoint_remove(0x01f7, 0x8d36)
            if not helper then
                event.helper_timeout = helper_err
                return event
            end
            event.player_bounds = {
                entry = {segment = helper.segment, offset = helper.offset},
                registers = helper.registers,
                player = player_snapshot(),
                object = object_at_hit(helper),
                globals = global_snapshot(),
            }
            dosbox.breakpoint_set(0x01f7, 0x8d60, {once = true})
            dosbox.debug_continue()
            local overlap, overlap_err = wait_hit(
                "targeted ID11 overlap guard", event_timeout_ms)
            dosbox.breakpoint_remove(0x01f7, 0x8d60)
            if not overlap then
                event.overlap_timeout = overlap_err
                return event
            end
            event.letter_overlap = {
                entry = {segment = overlap.segment, offset = overlap.offset},
                registers = overlap.registers,
                player = player_snapshot(),
                object = object_at_hit(overlap),
                globals = global_snapshot(),
            }
            dosbox.breakpoint_set(0x01f7, 0x8e37, {once = true})
            dosbox.breakpoint_set(0x01f7, 0x8e42, {once = true})
            dosbox.debug_continue()
            local effect_write, effect_err = wait_hit(
                "targeted ID11 effect branch", event_timeout_ms)
            dosbox.breakpoint_remove(0x01f7, 0x8e37)
            dosbox.breakpoint_remove(0x01f7, 0x8e42)
            if not effect_write then
                event.effect_branch_timeout = effect_err
                return event
            end
            event.letter_overlap.effect_branch = {
                entry = {segment = effect_write.segment, offset = effect_write.offset},
                registers = effect_write.registers,
                globals = global_snapshot(),
                letter_state = {
                    state = dosbox.mem_read_word(
                        "es", (effect_write.registers.edi or 0) + 0x2c),
                    counter_0x2e = dosbox.mem_read_word(
                        "es", (effect_write.registers.edi or 0) + 0x2e),
                    mask = dosbox.mem_read_word(
                        "es", (effect_write.registers.edi or 0) + 0x2a),
                    collected_mask = dosbox.mem_read_word("ds", 0x60d8),
                },
            }
            if effect_write.offset == 0x8e37 then
                dosbox.breakpoint_set(0x01f7, 0x8e3d, {once = true})
                dosbox.debug_continue()
                local effect_after, effect_after_err = wait_hit(
                    "targeted ID11 effect branch completion", event_timeout_ms)
                if effect_after then
                    event.letter_overlap.effect_branch.after = {
                        entry = {segment = effect_after.segment,
                                 offset = effect_after.offset},
                        registers = effect_after.registers,
                        globals = global_snapshot(),
                    }
                else
                    dosbox.breakpoint_remove(0x01f7, 0x8e3d)
                    event.letter_overlap.effect_branch.after_timeout = effect_after_err
                end
            end
        end
        return event
    end

    if selected ~= nil then
        dosbox.breakpoint_set(0x01f7, selected.after, {once = true})
        dosbox.debug_continue()
        local after, after_err = wait_hit("targeted SFX guard completion", event_timeout_ms)
        if after then
            event.after = {segment = after.segment, offset = after.offset}
            event.object_after = object_at_hit(after)
            event.globals_after = global_snapshot()
            event.pool_after = pool_snapshot()
        else
            dosbox.breakpoint_remove(0x01f7, selected.after)
            event.after_timeout = after_err
            dosbox.debug_continue()
        end
    end
    return event
end

local function capture_forced_event(effect_id, sequence)
    -- Pause at the same gameplay call boundary used by the ordinary object
    -- path. Replace only the pending table index, then let the real wrapper,
    -- scheduler, driver update, and interrupt path execute unchanged.
    dosbox.breakpoint_set(0x01f7, 0x42f5, {once = true})
    dosbox.key("KBD_space", true)
    dosbox.debug_continue()
    local callsite, callsite_err = wait_hit("forced SFX callsite")
    dosbox.key("KBD_space", false)
    if not callsite then
        dosbox.breakpoint_remove(0x01f7, 0x42f5)
        return nil, callsite_err
    end
    local forced_callsite = {
        entry = {segment = callsite.segment, offset = callsite.offset},
        registers = callsite.registers,
        selector = callsite.registers.es,
        offset = (callsite.registers.edi or 0) & 0xffff,
        bytes_hex = hex(read_selector(
            callsite.registers.es,
            (callsite.registers.edi or 0) & 0xffff,
            0x80) or ""),
    }
    dosbox.mem_write("ds", 0x612e,
        string.char(effect_id & 0xff, (effect_id >> 8) & 0xff))
    dosbox.breakpoint_set(0x01e7, 0x0fcf, {once = true})
    dosbox.debug_continue()
    local entry, entry_err = wait_hit("forced SFX entry")
    if not entry then
        dosbox.breakpoint_remove(0x01e7, 0x0fcf)
        return nil, entry_err
    end
    local event = capture_sfx_entry(entry, sequence)
    event.forced_callsite = forced_callsite
    event.action = "forced-id-" .. tostring(effect_id)
    dosbox.breakpoint_set(0x01e7, 0x3360, {once = true})
    dosbox.debug_continue()
    local low, low_err = wait_hit("forced SFX scheduler", event_timeout_ms)
    if low then
        if priority_only then
            event.low_level = {
                entry = {segment = low.segment, offset = low.offset},
                registers = low.registers,
                table = low.registers.es ~= 0
                    and table_snapshot(low.registers.es) or nil,
            }
            capture_priority_only(low, event)
        else
            capture_low_level(low, event)
        end
    else
        dosbox.breakpoint_remove(0x01e7, 0x3360)
        event.scheduler_timeout = low_err
        dosbox.debug_continue()
    end
    return event
end

local function capture_forced_priority_event(effect_id, sequence, label, restored_callsite)
    dosbox.breakpoint_set(0x01f7, 0x42f5, {once = true})
    if not restored_callsite then
        dosbox.key("KBD_space", true)
    end
    dosbox.debug_continue()
    local callsite, callsite_err = wait_hit(label .. " callsite")
    if not restored_callsite then
        dosbox.key("KBD_space", false)
    end
    if not callsite then
        dosbox.breakpoint_remove(0x01f7, 0x42f5)
        return nil, callsite_err
    end
    local object_raw = read_selector(
        callsite.registers.es,
        (callsite.registers.edi or 0) & 0xffff,
        0x80)
    local forced_callsite = {
        entry = {segment = callsite.segment, offset = callsite.offset},
        registers = callsite.registers,
        selector = callsite.registers.es,
        offset = (callsite.registers.edi or 0) & 0xffff,
        bytes_hex = hex(object_raw or ""),
        restore_fields = object_raw and {
            position = dword(object_raw, 0x0e + 1),
            active_state = string.byte(object_raw, 0x37 + 1),
            state = string.byte(object_raw, 0x3a + 1),
            subtype = string.byte(object_raw, 0x3b + 1),
            timer = word(object_raw, 0x3e + 1),
        } or nil,
    }
    dosbox.mem_write("ds", 0x612e,
        string.char(effect_id & 0xff, (effect_id >> 8) & 0xff))
    dosbox.breakpoint_set(0x01e7, 0x0fcf, {once = true})
    dosbox.debug_continue()
    local entry, entry_err = wait_hit(label .. " wrapper")
    if not entry then
        dosbox.breakpoint_remove(0x01e7, 0x0fcf)
        return nil, entry_err
    end
    local event = capture_sfx_entry(entry, sequence)
    event.forced_callsite = forced_callsite
    event.action = label .. "-id-" .. tostring(effect_id)
    dosbox.breakpoint_set(0x01e7, 0x3360, {once = true})
    dosbox.debug_continue()
    local low, low_err = wait_hit(label .. " priority boundary", event_timeout_ms)
    if not low then
        dosbox.breakpoint_remove(0x01e7, 0x3360)
        event.scheduler_timeout = low_err
        dosbox.debug_continue()
        return event
    end
    event.low_level = {
        entry = {segment = low.segment, offset = low.offset},
        registers = low.registers,
        table = low.registers.es ~= 0
            and table_snapshot(low.registers.es) or nil,
    }
    capture_priority_only(low, event)
    return event
end

local function capture_natural_priority_event(effect_id, sequence, label)
    dosbox.breakpoint_set(0x01e7, 0x0fcf, {once = true})
    local keys = {"KBD_right", "KBD_space", "KBD_leftshift", "KBD_up"}
    for _, key in ipairs(keys) do dosbox.key(key, true) end
    dosbox.debug_continue()
    local entry, entry_err = wait_hit(label .. " wrapper")
    for _, key in ipairs(keys) do dosbox.key(key, false) end
    if not entry then
        dosbox.breakpoint_remove(0x01e7, 0x0fcf)
        return nil, entry_err
    end
    dosbox.mem_write("ds", 0x612e,
        string.char(effect_id & 0xff, (effect_id >> 8) & 0xff))
    local event = capture_sfx_entry(entry, sequence)
    event.action = label .. "-id-" .. tostring(effect_id)
    dosbox.breakpoint_set(0x01e7, 0x3360, {once = true})
    dosbox.debug_continue()
    local low, low_err = wait_hit(label .. " priority boundary", event_timeout_ms)
    if not low then
        dosbox.breakpoint_remove(0x01e7, 0x3360)
        event.scheduler_timeout = low_err
        dosbox.debug_continue()
        return event
    end
    event.low_level = {
        entry = {segment = low.segment, offset = low.offset},
        registers = low.registers,
        table = low.registers.es ~= 0
            and table_snapshot(low.registers.es) or nil,
    }
    capture_priority_only(low, event)
    return event
end

local function capture_collision_pair(first_id, second_id)
    local pair = {first_id = first_id, second_id = second_id}
    local first, first_err = capture_forced_priority_event(
        first_id, 1, "collision-first")
    if not first then
        pair.error = first_err
        return pair
    end
    pair.first = first
    local priority = first.low_level
        and first.low_level.priority_selection
        and first.low_level.priority_selection.comparison
    if not priority then
        pair.error = "first effect did not reach priority comparison"
        return pair
    end

    local selected_voice = priority.voice_offset or 0
    local commit_limit = 16
    for _ = 1, commit_limit do
        dosbox.breakpoint_set(0x01e7, 0x29bf, {once = true})
        dosbox.debug_continue()
        local commit_hit, commit_err = wait_hit(
            "collision first mixer commit", event_timeout_ms)
        if not commit_hit then
            dosbox.breakpoint_remove(0x01e7, 0x29bf)
            pair.commit_timeout = commit_err
            return pair
        end
        local voice_offset = (commit_hit.registers.esi or 0) & 0xffff
        dosbox.breakpoint_set(0x01e7, 0x29c2, {once = true})
        dosbox.debug_continue()
        local after_commit, after_err = wait_hit(
            "collision first mixer commit completion", event_timeout_ms)
        if not after_commit then
            dosbox.breakpoint_remove(0x01e7, 0x29c2)
            pair.commit_timeout = after_err
            return pair
        end
        if voice_offset == (selected_voice & 0xffff) then
            pair.active_commit = {
                entry = {segment = commit_hit.segment, offset = commit_hit.offset},
                registers = commit_hit.registers,
                voice_offset = voice_offset,
                before = voice_state_snapshot(
                    commit_hit.registers.ds, voice_offset),
                after = voice_state_snapshot(
                    after_commit.registers.ds, voice_offset),
                mixer_before = mixer_state_snapshot(
                    commit_hit.registers.fs, commit_hit.registers.ebp or 0),
                mixer_after = mixer_state_snapshot(
                    after_commit.registers.fs, after_commit.registers.ebp or 0),
            }
            break
        end
    end
    if not pair.active_commit then
        pair.error = "selected voice did not reach a mixer commit"
        return pair
    end

    local callsite = first.forced_callsite
    local fields = callsite and callsite.restore_fields
    if callsite and fields and callsite.selector ~= 0 then
        dosbox.mem_write_selector(
            callsite.selector, callsite.offset + 0x0e,
            dword_bytes(fields.position))
        dosbox.mem_write_selector(
            callsite.selector, callsite.offset + 0x37,
            string.char(fields.active_state or 0))
        dosbox.mem_write_selector(
            callsite.selector, callsite.offset + 0x3a,
            string.char(fields.state or 0))
        dosbox.mem_write_selector(
            callsite.selector, callsite.offset + 0x3b,
            string.char(fields.subtype or 0))
        dosbox.mem_write_selector(
            callsite.selector, callsite.offset + 0x3e,
            word_bytes(fields.timer or 0))
        pair.restored_callsite = {
            selector = callsite.selector,
            offset = callsite.offset,
            fields = fields,
        }
    end

    local priority_patch = nil
    if collision_high_bit then
        local first_comparison = first.low_level
            and first.low_level.priority_selection
            and first.low_level.priority_selection.comparison
        local driver_selector = first_comparison
            and first_comparison.registers.es or 0
        local table_offset = 0x200 + second_id * 8
        local saved_entry = driver_selector ~= 0
            and read_selector(driver_selector, table_offset, 8) or nil
        if not saved_entry or #saved_entry < 8 then
            pair.error = "second collision priority table entry unavailable"
            return pair
        end
        local saved_priority = string.byte(saved_entry, 6)
        local patched_priority = saved_priority | 0x80
        local patched_entry = string.sub(saved_entry, 1, 5) ..
            string.char(patched_priority) .. string.sub(saved_entry, 7)
        dosbox.mem_write_selector(driver_selector, table_offset, patched_entry)
        priority_patch = {
            selector = driver_selector,
            offset = table_offset,
            saved_hex = hex(saved_entry),
            patched_hex = hex(patched_entry),
            saved_priority = saved_priority,
            patched_priority = patched_priority,
        }
        pair.priority_high_bit_patch = priority_patch
    end

    -- Let the interrupted gameplay loop return to a frame boundary before
    -- requesting the second effect. The voice remains active while the
    -- input path advances to its next callsite.
    dosbox.debug_continue()
    dosbox.wait_frames(1)
    local second, second_err = capture_natural_priority_event(
        second_id, 2, "collision-second")
    if priority_patch then
        dosbox.mem_write_selector(
            priority_patch.selector, priority_patch.offset,
            (function()
                local bytes = priority_patch.saved_hex
                local restored = ""
                for index = 1, #bytes, 2 do
                    restored = restored .. string.char(tonumber(
                        string.sub(bytes, index, index + 1), 16))
                end
                return restored
            end)())
    end
    if not second then
        pair.error = second_err
        return pair
    end
    pair.second = second
    return pair
end

local function trace_key_action(action, events, callsites, targeted_events)
    local keys = action.keys or {action.key}
    action.player_samples = {}
    for _, key in ipairs(keys) do dosbox.key(key, true) end
    for _ = 1, action.attempts do
        if #events + #callsites + #targeted_events >= max_events then break end
        local sequence = targeted_probe ~= "" and (#targeted_events + 1)
            or (callsite_probe and (#callsites + 1) or (#events + 1))
        local event, err
        if targeted_probe ~= "" then
            event, err = capture_targeted_probe(sequence)
        elseif callsite_probe then
            event, err = capture_callsite(sequence)
        else
            event, err = capture_one_event(sequence)
        end
        if event then
            event.action = action.name or action.key or table.concat(keys, "+")
            if targeted_probe ~= "" then
                targeted_events[#targeted_events + 1] = event
            elseif callsite_probe then
                callsites[#callsites + 1] = event
            else
                events[#events + 1] = event
            end
        else
            action.last_timeout = err
            if not action.continue_on_timeout then
                break
            end
            -- Keep the held key active while re-arming the wrapper. This is
            -- used by the traversal probe, where the interesting event may
            -- occur well after the preceding movement sound.
            action.player_samples[#action.player_samples + 1] = player_snapshot()
            dosbox.wait_frames(1)
        end
    end
    for _, key in ipairs(keys) do dosbox.key(key, false) end
    dosbox.wait_frames(action.settle_frames or action_settle_frames)
end

local function capture_driver_probes(count)
    local probes = {}
    for sequence = 1, count do
        dosbox.breakpoint_set(0x01e7, 0x298d, {once = true})
        dosbox.debug_continue()
        local hit, err = wait_hit("audio driver update")
        if not hit then
            dosbox.breakpoint_remove(0x01e7, 0x298d)
            probes[#probes + 1] = {sequence = sequence, timeout = err}
            break
        end
        local registers = hit.registers
        probes[#probes + 1] = {
            sequence = sequence,
            entry = {segment = hit.segment, offset = hit.offset},
            registers = registers,
            globals = global_snapshot(),
            game_channels = game_channel_snapshot(registers.ds),
            driver_bytes = registers.es ~= 0
                and read_selector(registers.es, 0, 0x400)
                and hex(read_selector(registers.es, 0, 0x400))
                or nil,
        }
    end
    return probes
end

dosbox.output.awaiting_startup_replay = true
dosbox.wait_frames(350)

local selector_indices = {
    W1L1 = 0, W1L2 = 1, W1L3 = 2,
    W2L1 = 3, W2L2 = 4, W2L3 = 5,
    W3L1 = 6, W3L2 = 7, W3L3 = 8,
    W4L1 = 9, W4L2 = 10, W4L3 = 11,
    W5L1 = 12, W5L2 = 13, W5L3 = 14,
    W1L4 = 15, W2L4 = 16, W3L4 = 17,
    W4L4 = 18, W5L4 = 19,
}

if select_level ~= "" then
    local selector_index = selector_indices[select_level]
    assert(selector_index ~= nil, "unsupported level selector target")
    dosbox.key("KBD_space", true)
    dosbox.wait_frames(4)
    dosbox.key("KBD_space", false)
    dosbox.wait_frames(30)
    dosbox.type("QUIKYSUPERHERO")
    dosbox.wait_frames(3)
    dosbox.breakpoint_set(0x01d7, 0x491d, {once = true})
    dosbox.key("KBD_4", true)
    local cheat, cheat_err = wait_hit("level selector branch")
    dosbox.key("KBD_4", false)
    if not cheat then error(cheat_err) end
    dosbox.mem_write("ds", 0x89f2, string.char(1))
    dosbox.mem_write("ds", 0x88ba, string.char(5, 0))
    dosbox.debug_continue()
    dosbox.wait_frames(selector_frames)
    dosbox.breakpoint_set(0x01d7, 0x4ace, {once = true})
    local input_wait, input_err = wait_hit("selector input wait")
    if not input_wait then error(input_err) end
    dosbox.mem_write("ds", 0x85d4,
        string.char(selector_index & 0xff, selector_index >> 8))
    dosbox.breakpoint_set(0x01d7, 0x4b18, {once = true})
    dosbox.mem_write("ds", 0x88bc, string.char(0x20, 0))
    dosbox.debug_continue()
    local launch, launch_err = wait_hit("selector Space dispatch")
    if not launch then error(launch_err) end
end

dosbox.breakpoint_set(0x01e7, 0x0d18, {once = true})
if select_level == "" then
    dosbox.key("KBD_space", true)
end
local load_hit, load_err = wait_hit("gameplay audio load")
if select_level == "" then
    dosbox.key("KBD_space", false)
end
if not load_hit then error(load_err) end

local before = global_snapshot()
dosbox.mem_write("ds", 0x613f, "\x01")
if mute_music then
    dosbox.mem_write("ds", 0x613e, "\x00")
end
local after = global_snapshot()
dosbox.debug_continue()
dosbox.wait_frames(settle_frames)
local settle_hit = stop_for_capture()

local result = {
    schema = "quiky-sfx-trace-v1",
    load = {
        hit = {segment = load_hit.segment, offset = load_hit.offset},
        registers = load_hit.registers,
        globals_before = before,
        globals_after_fx_enable = after,
    },
    settle = {
        hit = {segment = settle_hit.segment, offset = settle_hit.offset},
        runtime = runtime_snapshot(),
        pool = (pool_probe or targeted_probe == "id2" or targeted_probe == "id2-crab")
            and pool_snapshot() or nil,
    },
    events = {},
    actions = {},
}
if callsite_probe then
    result.callsites = {}
end
if targeted_probe ~= "" then
    result.targeted_events = {}
end

local selection_voice_patch = nil
if selection_voice ~= nil then
    if force_id == nil then
        error("selection_voice requires one forced effect ID")
    end
    local driver_selector = global_snapshot().driver_selector
    local table_offset = 0x200 + force_id * 8
    local saved_entry = read_selector(driver_selector, table_offset, 8)
    if driver_selector == 0 or not saved_entry or #saved_entry < 8 then
        error("cannot read effect table entry for selection_voice probe")
    end
    local saved_selector_byte = string.byte(saved_entry, 3)
    local patched_selector_byte = (saved_selector_byte & 0xf0) | selection_voice
    local patched_entry = string.sub(saved_entry, 1, 2) ..
        string.char(patched_selector_byte) .. string.sub(saved_entry, 4)
    dosbox.mem_write_selector(driver_selector, table_offset, patched_entry)
    selection_voice_patch = {
        selector = driver_selector,
        offset = table_offset,
        saved_hex = hex(saved_entry),
        patched_hex = hex(patched_entry),
        voice = selection_voice,
    }
    result.selection_voice_patch = selection_voice_patch
end

if warmup_key ~= "" and warmup_frames > 0 then
    dosbox.key(warmup_key, true)
    dosbox.debug_continue()
    dosbox.wait_frames(warmup_frames)
    dosbox.key(warmup_key, false)
    local warmup_hit = stop_for_capture()
    result.warmup = {
        key = warmup_key,
        frames = warmup_frames,
        hit = {segment = warmup_hit.segment, offset = warmup_hit.offset},
    }
end

if teleport_player ~= nil then
    result.teleport_player = teleport_live_player()
    if targeted_probe == "id2" then
        dosbox.mem_write("ds", 0x8806, word_bytes(1))
        dosbox.mem_write("ds", 0x8808, word_bytes(0))
        dosbox.mem_write("ds", 0x87de, word_bytes(teleport_player.x or 0))
        dosbox.mem_write("ds", 0x87e0, word_bytes(teleport_player.y or 0))
        result.contact_ring_seed = {
            count = 1,
            cursor = 0,
            x = teleport_player.x,
            y = teleport_player.y,
        }
    end
end
if teleport_object ~= nil then
    result.teleport_object = teleport_live_object()
end
if camera_x ~= nil then
    dosbox.mem_write("ds", 0x81c0, word_bytes(camera_x))
end
if camera_y ~= nil then
    dosbox.mem_write("ds", 0x81c4, word_bytes(camera_y))
end
if camera_x ~= nil or camera_y ~= nil then
    result.camera_override = {
        x = camera_x,
        y = camera_y,
    }
end

if capture_path ~= "" then
    dosbox.audio_capture_start(capture_path)
    result.audio_capture = {path = capture_path, started = true}
end

if collision_pair ~= nil then
    local pair = capture_collision_pair(collision_pair[1], collision_pair[2])
    result.collision_pair = pair
    if pair.first then result.events[#result.events + 1] = pair.first end
    if pair.second then result.events[#result.events + 1] = pair.second end
elseif force_all_ids then
    result.forced_ids = {}
    for effect_id = 0, 13 do
        if #result.events >= max_events then break end
        local event, forced_err = capture_forced_event(effect_id, #result.events + 1)
        if event then
            result.events[#result.events + 1] = event
            result.forced_ids[#result.forced_ids + 1] = {
                id = effect_id,
                sequence = event.sequence,
                observed_id = event.globals.current_effect,
            }
        else
            result.forced_ids[#result.forced_ids + 1] = {
                id = effect_id,
                error = forced_err,
            }
        end
        if inter_event_settle_frames > 0 then
            dosbox.wait_frames(inter_event_settle_frames)
        end
    end
elseif force_id ~= nil then
    local event, forced_err = capture_forced_event(force_id, 1)
    result.forced_id = {
        id = force_id,
        observed_id = event and event.globals.current_effect or nil,
        error = event and nil or forced_err,
    }
    if event then result.events[#result.events + 1] = event end
else
local actions = {
    {name = "right", key = "KBD_right", attempts = action_attempts},
    {name = "left", key = "KBD_left", attempts = action_attempts},
    {name = "space", key = "KBD_space", attempts = action_attempts},
    {name = "leftshift", key = "KBD_leftshift", attempts = action_attempts},
    {name = "up", key = "KBD_up", attempts = action_attempts},
    {name = "down", key = "KBD_down", attempts = action_attempts},
}
if action_profile == "pickup" then
    actions = {
        {name = "space", key = "KBD_space", attempts = action_attempts},
        {name = "up", key = "KBD_up", attempts = action_attempts},
        {name = "down", key = "KBD_down", attempts = action_attempts},
        {name = "leftshift", key = "KBD_leftshift", attempts = action_attempts},
    }
elseif action_profile == "explore" then
    actions = {
        {name = "right+space", keys = {"KBD_right", "KBD_space"}, attempts = action_attempts},
        {name = "left+space", keys = {"KBD_left", "KBD_space"}, attempts = action_attempts},
        {name = "right+leftshift", keys = {"KBD_right", "KBD_leftshift"}, attempts = action_attempts},
        {name = "space", key = "KBD_space", attempts = action_attempts},
        {name = "up", key = "KBD_up", attempts = action_attempts},
        {name = "down", key = "KBD_down", attempts = action_attempts},
    }
elseif action_profile == "extended" then
    actions = {
        {name = "right+space", keys = {"KBD_right", "KBD_space"}, attempts = action_attempts},
        {name = "left+space", keys = {"KBD_left", "KBD_space"}, attempts = action_attempts},
        {name = "right+leftctrl", keys = {"KBD_right", "KBD_leftctrl"}, attempts = action_attempts},
        {name = "left+leftctrl", keys = {"KBD_left", "KBD_leftctrl"}, attempts = action_attempts},
        {name = "right+leftalt", keys = {"KBD_right", "KBD_leftalt"}, attempts = action_attempts},
        {name = "left+leftalt", keys = {"KBD_left", "KBD_leftalt"}, attempts = action_attempts},
        {name = "leftctrl", key = "KBD_leftctrl", attempts = action_attempts},
        {name = "leftalt", key = "KBD_leftalt", attempts = action_attempts},
        {name = "leftshift", key = "KBD_leftshift", attempts = action_attempts},
        {name = "z", key = "KBD_z", attempts = action_attempts},
        {name = "x", key = "KBD_x", attempts = action_attempts},
        {name = "c", key = "KBD_c", attempts = action_attempts},
        {name = "space", key = "KBD_space", attempts = action_attempts},
        {name = "up", key = "KBD_up", attempts = action_attempts},
        {name = "down", key = "KBD_down", attempts = action_attempts},
    }
elseif action_profile == "traverse" then
    actions = {
        {name = "right", key = "KBD_right",
         attempts = action_attempts, continue_on_timeout = true},
    }
elseif action_profile == "traverse-jump" then
    actions = {
        {name = "right+space", keys = {"KBD_right", "KBD_space"},
         attempts = action_attempts, continue_on_timeout = true},
    }
elseif action_profile == "traverse-up" then
    actions = {
        {name = "right+up", keys = {"KBD_right", "KBD_up"},
         attempts = action_attempts, continue_on_timeout = true},
    }
elseif action_profile == "traverse-alt" then
    actions = {
        {name = "right+leftalt", keys = {"KBD_right", "KBD_leftalt"},
         attempts = action_attempts, continue_on_timeout = true},
    }
elseif action_profile == "alt-probe" then
    actions = {
        {name = "right+space", keys = {"KBD_right", "KBD_space"}, attempts = action_attempts},
        {name = "left+space", keys = {"KBD_left", "KBD_space"}, attempts = action_attempts},
        {name = "right+leftalt", keys = {"KBD_right", "KBD_leftalt"}, attempts = action_attempts},
        {name = "left+leftalt", keys = {"KBD_left", "KBD_leftalt"}, attempts = action_attempts},
        {name = "leftalt", key = "KBD_leftalt", attempts = action_attempts},
    }
elseif action_profile == "alt-replay" then
    actions = {
        {name = "right+space", keys = {"KBD_right", "KBD_space"}, attempts = action_attempts},
        {name = "z", key = "KBD_z", attempts = action_attempts},
        {name = "x", key = "KBD_x", attempts = action_attempts},
        {name = "c", key = "KBD_c", attempts = action_attempts},
        {name = "space", key = "KBD_space", attempts = action_attempts},
        {name = "up", key = "KBD_up", attempts = action_attempts},
        {name = "down", key = "KBD_down", attempts = action_attempts},
        {name = "left+space", keys = {"KBD_left", "KBD_space"}, attempts = action_attempts},
        {name = "right+leftctrl", keys = {"KBD_right", "KBD_leftctrl"}, attempts = action_attempts},
        {name = "left+leftctrl", keys = {"KBD_left", "KBD_leftctrl"}, attempts = action_attempts},
        {name = "right+leftalt", keys = {"KBD_right", "KBD_leftalt"}, attempts = action_attempts},
        {name = "left+leftalt", keys = {"KBD_left", "KBD_leftalt"}, attempts = action_attempts},
        {name = "leftctrl", key = "KBD_leftctrl", attempts = action_attempts},
        {name = "leftalt", key = "KBD_leftalt", attempts = action_attempts},
        {name = "leftshift", key = "KBD_leftshift", attempts = action_attempts},
    }
end
for _, action in ipairs(actions) do
    if #result.events + #(result.callsites or {}) +
            #(result.targeted_events or {}) >= max_events then break end
    local before_count = #result.events
    local before_callsite_count = #(result.callsites or {})
    local before_targeted_count = #(result.targeted_events or {})
    trace_key_action(action, result.events, result.callsites or {},
                     result.targeted_events or {})
    result.actions[#result.actions + 1] = {
        key = action.name or action.key or table.concat(action.keys, "+"),
        attempts = action.attempts,
        settle_frames = action.settle_frames or action_settle_frames,
        events = #result.events - before_count,
        callsites = #(result.callsites or {}) - before_callsite_count,
        targeted = #(result.targeted_events or {}) - before_targeted_count,
        last_timeout = action.last_timeout,
        continue_on_timeout = action.continue_on_timeout or false,
        player_samples = action.player_samples,
    }
end
end

if selection_voice_patch ~= nil then
    local saved = selection_voice_patch.saved_hex
    local restored = ""
    for index = 1, #saved, 2 do
        restored = restored .. string.char(tonumber(
            string.sub(saved, index, index + 1), 16))
    end
    dosbox.mem_write_selector(
        selection_voice_patch.selector, selection_voice_patch.offset, restored)
    selection_voice_patch.restored = true
end

if driver_probe_count > 0 then
    result.driver_probes = capture_driver_probes(driver_probe_count)
end

if capture_path ~= "" then
    if audio_tail_frames > 0 then
        dosbox.wait_frames(audio_tail_frames)
    end
    dosbox.audio_capture_stop()
    result.audio_capture.stopped = true
end

result.final = {
    runtime = runtime_snapshot(),
    player = player_snapshot(),
    event_count = #result.events,
}
dosbox.output.trace = result
