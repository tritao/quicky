-- Reversible, declarative memory mutations for focused gameplay experiments.
QUIKY_PATCH_WATCH = (function()
    local module = {}

    local function bytes_for(value, width)
        local bytes = {}
        for index = 1, width do
            bytes[index] = string.char(value & 0xff)
            value = value >> 8
        end
        return table.concat(bytes)
    end

    local function byte_array(raw)
        local result = {}
        for index = 1, #raw do result[index] = string.byte(raw, index) end
        return result
    end

    local function resolve(spec, context)
        if spec.space == "map" then
            local map_base = context.dosbox.mem_read_word("ds", 0x657a)
            local map_selector = context.dosbox.mem_read_word("ds", 0x657c)
            local row_stride = context.dosbox.mem_read_word("ds", 0x657e)
            return {
                kind = "selector",
                selector = map_selector,
                offset = map_base + spec.map_y * row_stride + spec.map_x * 2,
                map_x = spec.map_x,
                map_y = spec.map_y,
            }
        end
        if spec.space == "player" then
            if context.player == nil then return nil, "player object unavailable" end
            return {
                kind = "selector",
                selector = context.player.selector,
                offset = context.player.offset + spec.offset,
            }
        end
        if spec.space == "selector" then
            return {kind = "selector", selector = spec.selector, offset = spec.offset}
        end
        if spec.space == "ds" then
            return {kind = "segment", segment = "ds", offset = spec.offset}
        end
        return nil, "unsupported memory space " .. tostring(spec.space)
    end

    function module.new(dosbox_api, specs)
        local engine = {dosbox = dosbox_api, specs = specs or {}, active = {}}

        function engine:apply(sample, context)
            sample.mutation_ledger = sample.mutation_ledger or {}
            for index, spec in ipairs(self.specs) do
                context = context or {}
                context.dosbox = self.dosbox
                local target, reason = resolve(spec, context)
                if target == nil then error("patch " .. index .. ": " .. reason) end
                local original
                if target.kind == "selector" then
                    original = self.dosbox.mem_read_selector(
                        target.selector, target.offset, spec.width)
                else
                    original = self.dosbox.mem_read(
                        target.segment, target.offset, spec.width)
                end
                if original == nil or #original ~= spec.width then
                    error("patch " .. index .. ": target was not readable")
                end
                local replacement = bytes_for(spec.value, spec.width)
                if target.kind == "selector" then
                    self.dosbox.mem_write_selector(
                        target.selector, target.offset, replacement)
                else
                    self.dosbox.mem_write(target.segment, target.offset, replacement)
                end
                local entry = {
                    index = index,
                    space = spec.space,
                    selector = target.selector,
                    segment = target.segment,
                    offset = target.offset,
                    width = spec.width,
                    value = spec.value,
                    map_x = target.map_x,
                    map_y = target.map_y,
                    original_bytes = byte_array(original),
                    replacement_bytes = byte_array(replacement),
                    restored = false,
                }
                sample.mutation_ledger[#sample.mutation_ledger + 1] = entry
                self.active[#self.active + 1] = {
                    target = target, original = original, ledger = entry,
                }
            end
        end

        function engine:restore()
            for index = #self.active, 1, -1 do
                local mutation = self.active[index]
                local target = mutation.target
                if target.kind == "selector" then
                    self.dosbox.mem_write_selector(
                        target.selector, target.offset, mutation.original)
                else
                    self.dosbox.mem_write(
                        target.segment, target.offset, mutation.original)
                end
                mutation.ledger.restored = true
                self.active[index] = nil
            end
        end

        return engine
    end

    function module.arm_execute_watches(controller, watches)
        for index, watch in ipairs(watches or {}) do
            controller:arm("execute-watch-" .. index, watch.segment,
                           watch.offset, {once = true})
        end
    end

    function module.is_execute_watch(watches, segment, offset)
        for index, watch in ipairs(watches or {}) do
            if watch.segment == segment and watch.offset == offset then
                return index
            end
        end
        return nil
    end

    return module
end)()
