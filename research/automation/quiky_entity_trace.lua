-- Trace one ARE record from declaration dispatch through object-factory return.
local timeout_ms = TRACE_TIMEOUT_MS or 30000
local record_offset = TRACE_RECORD_OFFSET or 0x1792
local expected_type = TRACE_ENTITY_TYPE or 0x2b
local capture_delay_frames = TRACE_CAPTURE_DELAY_FRAMES or 0
local lifetime_sample_count = TRACE_LIFETIME_SAMPLES or 0
local sprite_init_offset = TRACE_SPRITE_INIT_OFFSET or 0
local capture_frame_count = TRACE_CAPTURE_FRAMES or 1
local capture_frame_step = TRACE_FRAME_STEP or 30
local runtime_offset = record_offset - 0x160

local function word(s, index)
    local lo, hi = string.byte(s, index, index + 1)
    return lo | (hi << 8)
end

local function dword(s, index)
    return word(s, index) | (word(s, index + 2) << 16)
end

local function hex(s)
    return (s:gsub(".", function(c)
        return string.format("%02x", string.byte(c))
    end))
end

local function dedicated_event_queue()
    local count = dosbox.mem_read_word("ds", 0x895e)
    local ring = dosbox.mem_read("ds", 0x8960, 0x80) or ""
    local events = {}
    local limit = count
    if limit > 0x80 then limit = 0x80 end
    for index = 0, limit - 1 do
        local ring_slot = string.byte(ring, index + 1)
        if not ring_slot then break end
        local offset = 0x6586 + ring_slot * 8
        local raw = dosbox.mem_read("ds", offset, 8) or ""
        local raw_bytes = {}
        for byte_index = 1, #raw do
            raw_bytes[#raw_bytes + 1] = string.byte(raw, byte_index)
        end
        events[#events + 1] = {
            queue_index = index,
            ring_slot = ring_slot,
            table_offset = offset,
            raw_hex = hex(raw),
            raw_bytes = raw_bytes,
            raw_length = #raw,
            position_dword = (#raw >= 4 and dword(raw, 1) or nil),
            record_word = (#raw >= 6 and word(raw, 5) or nil),
            animation_state = string.byte(raw, 7),
            subtype = string.byte(raw, 8),
        }
    end
    return {
        pending_count = count,
        ring_hex = hex(ring),
        subtype = string.byte(dosbox.mem_read("ds", 0x36ee, 1)),
        events = events,
    }
end

local function wait_hit(label)
    local hit, err = dosbox.wait_for_breakpoint(timeout_ms)
    if not hit then error(label .. ": " .. (err or "timeout")) end
    return hit
end

local function stop_for_capture()
    local current = dosbox.cpu_state()
    dosbox.breakpoint_set(current.cs, current.eip, {once = true})
    return wait_hit("capture barrier")
end

local function capture_timeline(entity, object_selector, object_offset, first_capture)
    if capture_frame_count <= 1 then return end
    entity.frames = {}
    for index = 0, capture_frame_count - 1 do
        if index > 0 then dosbox.wait_frames(capture_frame_step) end
        local capture = (index == 0 and first_capture) or stop_for_capture()
        -- A stopped Lua breakpoint can precede the renderer's present call on
        -- the inert branch. Advance one frame and stop again so the REST video
        -- endpoint sees a fully rendered surface for both variants.
        if capture_frame_count > 1 then
            dosbox.wait_frames(1)
            capture = stop_for_capture()
        end
        local frame = {
            index = index,
            capture_registers = capture.registers,
        }
        if object_selector and object_offset then
            local state = dosbox.mem_read_selector(object_selector, object_offset, 64)
            frame.object_state_hex = hex(state)
            frame.object = {selector = object_selector, offset = object_offset}
            frame.position = {x = dword(state, 3) >> 16,
                              y = dword(state, 7) >> 16}
            frame.sprite_slot = word(state, 0x12 + 1)
        end
        entity.frames[index + 1] = frame
        entity.capture_index = index
        dosbox.output.entity = entity
        -- Hold the guest at this exact frame until Python captures the PNG
        -- and acknowledges it through /api/v1/debug/continue.
        local current = dosbox.cpu_state()
        dosbox.breakpoint_set(current.cs, current.eip, {once = true})
        wait_hit("capture acknowledgement")
    end
end

dosbox.output.awaiting_startup_replay = true
dosbox.wait_frames(350)
dosbox.key("KBD_space", true)
dosbox.wait_frames(4)
dosbox.key("KBD_space", false)

for attempt = 1, 4096 do
    dosbox.breakpoint_set(0x01f7, 0x1e04, {once = true})
    local entry = wait_hit("ARE declaration")
    local r = entry.registers
    local record = dosbox.mem_read("fs", r.ebx & 0xffff, 6)
    local entity_type = word(record, 1) & 0xff
    if (r.ebx & 0xffff) == runtime_offset then
        assert(entity_type == expected_type,
               string.format("record type %02x, expected %02x",
                             entity_type, expected_type))
        local local_x = word(record, 3)
        local local_y = word(record, 5)
        local origin_x = dosbox.mem_read_word("ds", 0x3714)
        local origin_y = dosbox.mem_read_word("ds", 0x3716)
        local dispatch = dosbox.mem_read("ds", 0x81d2 + entity_type * 4, 4)
        local sprite_animation_tables = nil
        if entity_type >= 0x29 and entity_type <= 0x2b then
            sprite_animation_tables = {
                positive = {offset = 0x3312,
                            raw_hex = hex(dosbox.mem_read("ds", 0x3312, 20))},
                nonpositive = {offset = 0x3326,
                               raw_hex = hex(dosbox.mem_read("ds", 0x3326, 20))},
            }
        end

        local dedicated_handlers = {
            [0x65] = 0x178d, [0x66] = 0x1798, [0x67] = 0x17a3,
        }
        local dedicated_handler = dedicated_handlers[entity_type]
        if dedicated_handler then
            dosbox.breakpoint_set(0x01f7, dedicated_handler, {once = true})
            dosbox.debug_continue()
            local handler_hit = wait_hit("dedicated ARE handler")
            assert(handler_hit.segment == 0x01f7 and
                   handler_hit.offset == dedicated_handler,
                   "unexpected dedicated handler breakpoint")
            local dedicated_returns = {
                [0x65] = 0x1797, [0x66] = 0x17a2, [0x67] = 0x17ad,
            }
            local dedicated_return = dedicated_returns[entity_type]
            dosbox.breakpoint_set(0x01f7, 0x1749, {once = true})
            dosbox.breakpoint_set(0x01f7, dedicated_return, {once = true})
            dosbox.debug_continue()
            local creator_hit = wait_hit("dedicated common creator")
            assert(creator_hit.segment == 0x01f7 and
                   creator_hit.offset == 0x1749,
                   "unexpected dedicated common creator breakpoint")
            local creator_queue_before = dedicated_event_queue()
            dosbox.debug_continue()
            local return_hit = wait_hit("dedicated handler return")
            assert(return_hit.segment == 0x01f7 and
                   return_hit.offset == dedicated_return,
                   "unexpected dedicated handler return breakpoint")
            local creator_queue_after = dedicated_event_queue()
            dosbox.breakpoint_set(0x01f7, 0x1892, {once = true})
            dosbox.debug_continue()
            local render_call = wait_hit("dedicated event renderer")
            assert(render_call.segment == 0x01f7 and render_call.offset == 0x1892,
                   "unexpected dedicated event renderer breakpoint")
            dosbox.breakpoint_set(0x01f7, 0x171c, {once = true})
            dosbox.breakpoint_set(0x01f7, 0x1897, {once = true})
            dosbox.debug_continue()
            local render_object_hit = wait_hit("dedicated event object allocation")
            assert(render_object_hit.segment == 0x01f7 and
                   render_object_hit.offset == 0x171c,
                   "unexpected dedicated event object allocation breakpoint")
            local event_object_selector = render_object_hit.registers.es
            local event_object_offset = render_object_hit.registers.edi & 0xffff
            dosbox.debug_continue()
            local render_return = wait_hit("dedicated event renderer return")
            assert(render_return.segment == 0x01f7 and render_return.offset == 0x1897,
                   "unexpected dedicated event renderer return breakpoint")
            local creator_queue_after_render = dedicated_event_queue()
            local event_object_state = dosbox.mem_read_selector(
                event_object_selector, event_object_offset, 64)
            dosbox.output.entity = {
                type = entity_type,
                record_offset = record_offset,
                runtime_record = {selector = r.fs, offset = r.ebx & 0xffff},
                local_position = {x = local_x, y = local_y},
                region_origin = {x = origin_x, y = origin_y},
                world_position = {x = origin_x + local_x, y = origin_y + local_y},
                dedicated_handler = {
                    segment = handler_hit.segment, offset = handler_hit.offset,
                },
                common_creator = {segment = 0x01f7, offset = 0x1749},
                entry_registers = r,
                handler_registers = handler_hit.registers,
                common_creator_registers = creator_hit.registers,
                handler_return = {
                    segment = return_hit.segment, offset = return_hit.offset,
                },
                handler_return_registers = return_hit.registers,
                event_queue_before_creator = creator_queue_before,
                event_queue_after_creator = creator_queue_after,
                event_renderer_call = {
                    segment = render_call.segment, offset = render_call.offset,
                    registers = render_call.registers,
                },
                event_object_allocation = {
                    segment = render_object_hit.segment,
                    offset = render_object_hit.offset,
                    registers = render_object_hit.registers,
                },
                event_renderer_return = {
                    segment = render_return.segment, offset = render_return.offset,
                    registers = render_return.registers,
                },
                event_queue_after_render = creator_queue_after_render,
                event_object = {
                    selector = event_object_selector,
                    offset = event_object_offset,
                    state_hex = hex(event_object_state),
                    sprite_slot = word(event_object_state, 0x12 + 1),
                    update_callback = word(event_object_state, 0x18 + 1),
                    state_field = word(event_object_state, 0x2e + 1),
                },
                record_hex = hex(record),
            }
            return
        end

        if entity_type == 0 then
            dosbox.debug_continue()
            dosbox.wait_frames(1)
            dosbox.wait_frames(capture_delay_frames)
            local capture = stop_for_capture()
            local removed_entity = {
                type = entity_type,
                record_offset = record_offset,
                runtime_record = {selector = r.fs, offset = r.ebx & 0xffff},
                local_position = {x = local_x, y = local_y},
                region_origin = {x = origin_x, y = origin_y},
                world_position = {x = origin_x + local_x, y = origin_y + local_y},
                removed = true,
                entry_registers = r,
                capture_registers = capture.registers,
                record_hex = hex(record),
            }
            capture_timeline(removed_entity, nil, nil, capture)
            if capture_frame_count <= 1 then dosbox.wait_frames(1) end
            dosbox.output.entity = removed_entity
            return
        end

        local sprite_initialization = nil
        local lifetime_samples = {}
        if entity_type >= 0x29 and entity_type <= 0x2b then
            -- The leaf update initializes the visible sprite through a far
            -- helper call. At 474D ES:DI still identifies the ARE object.
            dosbox.breakpoint_set(0x01f7, 0x474d, {once = true})
        elseif sprite_init_offset ~= 0 then
            dosbox.breakpoint_set(0x01f7, sprite_init_offset, {once = true})
        end
        dosbox.breakpoint_set(0x01f7, 0x1e8e, {once = true})
        dosbox.debug_continue()
        local returned = wait_hit("object factory return")
        assert(returned.segment == 0x01f7 and returned.offset == 0x1e8e,
               "unexpected factory return breakpoint")
        local after = returned.registers
        local object_selector = after.es
        local object_offset = after.edi & 0xffff
        dosbox.debug_continue()
        if entity_type >= 0x29 and entity_type <= 0x2b then
            local initialized = wait_hit("leaf sprite initialization")
            assert(initialized.segment == 0x01f7 and initialized.offset == 0x474d,
                   "unexpected sprite-initialization breakpoint")
            local initialized_selector = initialized.registers.es
            local initialized_offset = initialized.registers.edi & 0xffff
            local initialized_state = dosbox.mem_read_selector(
                initialized_selector, initialized_offset, 64)
            sprite_initialization = {
                selector = initialized_selector,
                offset = initialized_offset,
                same_as_entity_object = initialized_selector == object_selector and
                                        initialized_offset == object_offset,
                sprite_slot = word(initialized_state, 0x12 + 1),
                state_hex = hex(initialized_state),
                breakpoint = {segment = 0x01f7, offset = 0x474d},
            }
            local attempts = 0
            local emulator_running = false
            while #lifetime_samples < lifetime_sample_count and attempts < 8192 do
                attempts = attempts + 1
                dosbox.breakpoint_set(0x01f7, 0x47e7, {once = true})
                if not emulator_running then
                    dosbox.debug_continue()
                    emulator_running = true
                end
                local updated = wait_hit("sprite lifetime update")
                emulator_running = false
                local rr = updated.registers
                local update_selector = rr.es
                local update_offset = rr.edi & 0xffff
                local update_state = dosbox.mem_read_selector(
                    update_selector, update_offset, 52)
                lifetime_samples[#lifetime_samples + 1] = {
                    sequence = #lifetime_samples + 1,
                    object = {selector = update_selector,
                              offset = update_offset},
                    matches_entity_object = update_selector == object_selector and
                                            update_offset == object_offset,
                    slot = word(update_state, 0x12 + 1),
                    animation_delay = word(update_state, 0x20 + 1),
                    animation_cursor = word(update_state, 0x24 + 1),
                    variant_flag = string.byte(update_state, 0x28 + 1),
                }
                -- Step past the current callback entry before re-arming it.
                dosbox.breakpoint_set(0x01f7, 0x47ec, {once = true})
                dosbox.debug_continue()
                local advanced = wait_hit("sprite update step-over")
                assert(advanced.segment == 0x01f7 and advanced.offset == 0x47ec,
                       "unexpected sprite update step-over breakpoint")
                emulator_running = false
            end
            assert(#lifetime_samples == lifetime_sample_count,
                   "lifetime sample limit exceeded")
            if not emulator_running then dosbox.debug_continue() end
        elseif sprite_init_offset ~= 0 then
            local initialized = wait_hit("sprite initializer")
            assert(initialized.segment == 0x01f7 and
                   initialized.offset == sprite_init_offset,
                   "unexpected sprite initializer breakpoint")
            local initialized_selector = initialized.registers.es
            local initialized_offset = initialized.registers.edi & 0xffff
            local initialized_state = dosbox.mem_read_selector(
                initialized_selector, initialized_offset, 64)
            sprite_initialization = {
                selector = initialized_selector,
                offset = initialized_offset,
                same_as_entity_object = initialized_selector == object_selector and
                                        initialized_offset == object_offset,
                sprite_slot = word(initialized_state, 0x12 + 1),
                state_hex = hex(initialized_state),
                breakpoint = {segment = 0x01f7, offset = sprite_init_offset},
            }
            dosbox.debug_continue()
        end
        dosbox.wait_frames(1 + capture_delay_frames)
        local capture = stop_for_capture()
        local object_state = dosbox.mem_read_selector(
            object_selector, object_offset, 64)
        local sprite_slot = word(object_state, 0x12 + 1)
        local normal_entity = {
            type = entity_type,
            record_offset = record_offset,
            runtime_record = {selector = r.fs, offset = r.ebx & 0xffff},
            local_position = {x = local_x, y = local_y},
            region_origin = {x = origin_x, y = origin_y},
            world_position = {x = origin_x + local_x, y = origin_y + local_y},
            dispatch_slot = 0x81d2 + entity_type * 4,
            dispatch_hex = hex(dispatch),
            dispatch_word = word(dispatch, 1),
            object_class = string.byte(dispatch, 3),
            dispatch_reserved = string.byte(dispatch, 4),
            update_callback = {
                runtime_segment = 0x01f7,
                offset = word(dispatch, 1),
            },
            factory = {segment = 0x01f7, offset = 0x0e06},
            factory_return = {segment = 0x01f7, offset = 0x1e8e},
            object = {selector = object_selector, offset = object_offset},
            sprite_initialization = sprite_initialization,
            sprite_animation_tables = sprite_animation_tables,
            lifetime_samples = lifetime_samples,
            sprite_slot = sprite_slot,
            sprite_slot_field_offset = 0x12,
            initialized_position = {
                x_fixed = dword(object_state, 3),
                y_fixed = dword(object_state, 7),
                x = dword(object_state, 3) >> 16,
                y = dword(object_state, 7) >> 16,
            },
            object_state_hex = hex(object_state),
            entry_registers = r,
            return_registers = after,
            capture_registers = capture.registers,
            record_hex = hex(record),
        }
        capture_timeline(normal_entity, object_selector, object_offset, capture)
        if capture_frame_count <= 1 then dosbox.wait_frames(1) end
        dosbox.output.entity = normal_entity
        return
    end
    dosbox.debug_continue()
    dosbox.wait_frames(1)
end

error(string.format("record 0x%x was not instantiated", record_offset))
