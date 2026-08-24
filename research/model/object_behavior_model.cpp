#include "object_behavior_model.h"

#include <algorithm>
#include <stdexcept>

namespace quiky::research {

std::optional<std::size_t> ObjectPool::first_free() const {
    for (std::size_t index = 0; index < records_.size(); ++index) {
        if (!records_[index].active()) {
            return index;
        }
    }
    return std::nullopt;
}

ObjectRecord& ObjectPool::at(std::size_t index) {
    return records_.at(index);
}

const ObjectRecord& ObjectPool::at(std::size_t index) const {
    return records_.at(index);
}

void ObjectPool::deactivate(std::size_t index) {
    records_.at(index).update_callback = kFreeCallback;
}

void ObjectScheduler::rebuild(std::uint8_t bank_index, const ObjectPool& pool) {
    if (bank_index >= banks_.size()) {
        throw std::out_of_range("scheduler bank");
    }
    Bank& destination = banks_[bank_index];
    destination.clear();
    for (std::size_t index = 0; index < kObjectPoolSize; ++index) {
        const ObjectRecord& object = pool.at(index);
        if (!object.active()) {
            continue;
        }
        destination.push_back({object.update_callback,
                               object.update_callback_segment,
                               static_cast<std::uint16_t>(index),
                               object.scheduler_phase});
    }
}

const ObjectScheduler::Bank& ObjectScheduler::bank(std::uint8_t bank_index) const {
    if (bank_index >= banks_.size()) {
        throw std::out_of_range("scheduler bank");
    }
    return banks_[bank_index];
}

ObjectScheduler::Bank ObjectScheduler::dispatch_order(std::uint8_t bank_index) const {
    Bank result = bank(bank_index);
    std::stable_sort(result.begin(), result.end(),
                     [](const SchedulerEntry& left, const SchedulerEntry& right) {
                         return left.phase < right.phase;
                     });
    return result;
}

std::int16_t DescriptorMemory::read(std::uint16_t address) const {
    const std::uint16_t delta = static_cast<std::uint16_t>(address - base_address);
    if ((delta & 1u) != 0 || (delta / 2u) >= words.size()) {
        throw std::out_of_range("descriptor sequence address");
    }
    return words[delta / 2u];
}

static std::uint16_t descriptor_action(std::int16_t value, std::uint8_t mode) {
    std::int32_t action = value;
    if (mode == 0xff) {
        action += 0x32;
    }
    return static_cast<std::uint16_t>(action);
}

void load_descriptor(ObjectRecord& object,
                     const DescriptorMemory& memory,
                     std::uint16_t initial_word_address) {
    object.descriptor_reload_delay = static_cast<std::uint16_t>(
        memory.read(initial_word_address));
    object.descriptor_timer = object.descriptor_reload_delay;
    object.descriptor_sequence_base = static_cast<std::uint16_t>(
        initial_word_address + 2);
    object.descriptor_sequence_cursor = object.descriptor_sequence_base;
    object.sprite_or_action = descriptor_action(
        memory.read(object.descriptor_sequence_cursor), object.descriptor_mode);
}

bool advance_descriptor(ObjectRecord& object, const DescriptorMemory& memory) {
    if (object.descriptor_timer != 0) {
        --object.descriptor_timer;
        return false;
    }

    object.descriptor_sequence_cursor = static_cast<std::uint16_t>(
        object.descriptor_sequence_cursor + 2);
    std::int16_t value = memory.read(object.descriptor_sequence_cursor);
    while (value < 0) {
        object.descriptor_sequence_cursor = static_cast<std::uint16_t>(
            object.descriptor_sequence_cursor + value * 2);
        value = memory.read(object.descriptor_sequence_cursor);
    }
    object.sprite_or_action = descriptor_action(value, object.descriptor_mode);
    object.descriptor_timer = object.descriptor_reload_delay;
    return true;
}

Type34Action test_type34_proximity(const ObjectRecord& object,
                                    PlayerState& player,
                                    std::uint16_t activation_state) {
    Type34Action result;
    if (activation_state < 0x32 ||
        player.collision_class == 0 || player.collision_class >= 0x80) {
        return result;
    }

    const std::int32_t object_x = object.world_x_pixels();
    const std::int32_t object_y = object.world_y_pixels();
    const std::int32_t player_x = pixels_from_fixed16_16(player.world_x_fixed);
    const std::int32_t player_y = pixels_from_fixed16_16(player.world_y_fixed);
    if (!(object_x - 0x19 < player_x && player_x < object_x + 0x19 &&
          object_y - 8 < player_y && player_y < object_y)) {
        return result;
    }

    result.proximity_hit = true;
    result.action_word = 4;
    result.effect_code = 0x2a;
    result.player_displacement_fixed = -0x1b000;
    player.contact_state = 0xff;
    player.displacement_field_fixed += result.player_displacement_fixed;
    return result;
}

} // namespace quiky::research
