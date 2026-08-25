-- Shared mechanics for Quiky debugger traces. The host concatenates this
-- chunk before focused trace scripts, avoiding guest filesystem assumptions.
QUIKY_TRACE_COMMON = (function()
    local common = {}

    function common.word(raw, index)
        local lo, hi = string.byte(raw, index, index + 1)
        return lo | (hi << 8)
    end

    function common.dword(raw, index)
        return common.word(raw, index) | (common.word(raw, index + 2) << 16)
    end

    function common.signed16(value)
        return value >= 0x8000 and value - 0x10000 or value
    end

    function common.signed32(value)
        return value >= 0x80000000 and value - 0x100000000 or value
    end

    function common.little_word(value)
        return string.char(value & 0xff, (value >> 8) & 0xff)
    end

    function common.hex(raw)
        return (raw:gsub(".", function(value)
            return string.format("%02x", string.byte(value))
        end))
    end

    function common.hex_differences(before_hex, after_hex)
        if before_hex == nil or after_hex == nil or #before_hex ~= #after_hex then
            return nil
        end
        local changes = {}
        for index = 1, #before_hex, 2 do
            local before = tonumber(before_hex:sub(index, index + 1), 16)
            local after = tonumber(after_hex:sub(index, index + 1), 16)
            if before ~= after then
                changes[#changes + 1] = {
                    offset = (index - 1) >> 1,
                    before = before,
                    after = after,
                }
            end
        end
        return changes
    end

    function common.numeric_differences(before, after)
        if before == nil or after == nil then return nil end
        local changes = {}
        for key, value in pairs(before) do
            if type(value) == "number" and type(after[key]) == "number" and
               value ~= after[key] then
                changes[#changes + 1] = {
                    field = key,
                    before = value,
                    after = after[key],
                }
            end
        end
        table.sort(changes, function(left, right)
            return left.field < right.field
        end)
        return changes
    end

    function common.address_key(address)
        if address == nil then return "<nil>" end
        return string.format("0x%04x:0x%08x", address.segment or 0,
                             address.offset or 0)
    end

    function common.selector_word(dosbox_api, selector, offset)
        local raw = dosbox_api.mem_read_selector(selector, offset, 2)
        if not raw or #raw < 2 then
            error(string.format("short selector word read 0x%04x:0x%x",
                                selector, offset))
        end
        return common.word(raw, 1)
    end

    function common.validate_return_address(dosbox_api, address)
        if type(address) ~= "table" or type(address.segment) ~= "number" or
           type(address.offset) ~= "number" or address.segment < 0 or
           address.segment > 0xffff or address.offset < 0 or
           address.offset > 0xffffffff then
            return false, "return address is not a bounded segment:offset"
        end
        local ok, raw_or_error = pcall(
            dosbox_api.mem_read_selector, address.segment, address.offset, 1
        )
        if not ok then return false, tostring(raw_or_error) end
        if not raw_or_error or #raw_or_error ~= 1 then
            return false, "return address selector read was truncated"
        end
        return true
    end

    function common.new_breakpoint_controller(dosbox_api)
        local controller = {owners = {}}

        function controller:arm(owner, segment, offset, options)
            local key = string.format("%04x:%08x", segment, offset)
            self.owners[key] = self.owners[key] or {}
            self.owners[key][owner] = true
            return dosbox_api.breakpoint_set(segment, offset, options or {once = true})
        end

        function controller:owners_for(segment, offset)
            local key = string.format("%04x:%08x", segment, offset)
            local result = {}
            for owner in pairs(self.owners[key] or {}) do
                result[#result + 1] = owner
            end
            table.sort(result)
            return result
        end

        return controller
    end

    return common
end)()
