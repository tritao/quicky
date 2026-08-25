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

static std::int32_t clamp_type33_velocity(std::int32_t value,
                                          std::int32_t limit) {
    return std::clamp(value, -limit, limit);
}

static std::int8_t negate_type33_byte(std::int8_t value) {
    return static_cast<std::int8_t>(-static_cast<std::int16_t>(value));
}

static bool type33_timer_expired(std::uint16_t& timer) {
    // 882F uses DEC word followed by JGE, so zero decrements to -1 and
    // expires; all positive values simply count down.
    timer = static_cast<std::uint16_t>(timer - 1);
    return static_cast<std::int16_t>(timer) < 0;
}

Type33StepResult step_type33_motion(ObjectRecord& object,
                                    const DescriptorMemory& memory,
                                    Type33MotionContext& context,
                                    bool map_probe_zero) {
    Type33StepResult result;
    const auto old_state = object.type33_state;
    if (map_probe_zero) {
        object.type33_transition = 1;
        result.map_set_transition = true;
    }

    auto mark_state_change = [&] {
        result.state_changed = result.state_changed ||
            object.type33_state != old_state;
    };

    if (object.type33_state < 1) {
        if (object.type33_transition <= 0) {
            object.world_x_fixed += object.type33_velocity_fixed;
            ++object.type33_animation_counter;
            if (object.type33_animation_counter <= 0x50) {
                ++object.type33_travel_counter;
                if (object.type33_travel_counter > 0x32) {
                    const std::uint8_t ring_index = context.travel_ring_index++;
                    // The original sign-extends the byte, then performs a
                    // logical AX shift by three.
                    const auto signed_value = static_cast<std::int16_t>(
                        context.travel_ring[ring_index]);
                    object.type33_travel_counter =
                        static_cast<std::uint16_t>(signed_value) >> 3;
                    object.type33_state = 1;
                }
            } else {
                object.type33_animation_counter = 0;
                object.type33_transition = 1;
            }
        } else if (object.type33_phase < 0) {
            auto velocity = object.type33_velocity_fixed -
                static_cast<std::int32_t>(object.map_probe_direction) * 0x400;
            velocity = clamp_type33_velocity(velocity, 0x6000);
            object.type33_velocity_fixed = velocity;
            object.world_x_fixed += velocity;
            if (type33_timer_expired(object.type33_phase_timer)) {
                object.map_probe_direction = negate_type33_byte(
                    object.map_probe_direction);
                object.descriptor_mode = static_cast<std::uint8_t>(
                    negate_type33_byte(static_cast<std::int8_t>(
                        object.descriptor_mode)));
                object.type33_phase = negate_type33_byte(object.type33_phase);
                object.type33_velocity_fixed =
                    static_cast<std::int8_t>(object.map_probe_direction) << 5;
                object.type33_phase_timer = 0x14;
            }
        } else {
            auto velocity = object.type33_velocity_fixed +
                static_cast<std::int32_t>(object.map_probe_direction) * 0x400;
            velocity = clamp_type33_velocity(velocity, 0x6000);
            object.type33_velocity_fixed = velocity;
            object.world_x_fixed += velocity;
            if (type33_timer_expired(object.type33_phase_timer)) {
                object.type33_phase = negate_type33_byte(object.type33_phase);
                object.type33_transition = -1;
                object.type33_phase_timer = 0x14;
            }
        }
        mark_state_change();
        return result;
    }

    if (object.type33_transition > 0) {
        object.type33_state = 0;
        object.type33_travel_counter = 0x23;
        mark_state_change();
        return result;
    }

    if (object.type33_state == 2) {
        ++object.type33_state_counter;
        if (object.type33_state_counter <= 0x2d) {
            mark_state_change();
            return result;
        }
        object.type33_state_counter = 0;
        object.type33_state = 3;
        load_descriptor(object, memory, 0x3504);
        result.descriptor_loaded = true;
    } else if (object.type33_state != 3) {
        auto velocity = object.type33_velocity_fixed -
            static_cast<std::int32_t>(object.map_probe_direction) * 0x100;
        velocity = clamp_type33_velocity(velocity, 0x5000);
        object.type33_velocity_fixed = velocity;
        object.world_x_fixed += velocity;
        const bool reached_zero = object.map_probe_direction <= 0
            ? velocity < 0
            : velocity > 0;
        if (!reached_zero) {
            object.type33_velocity_fixed = 0;
            object.type33_state = 2;
            load_descriptor(object, memory, 0x3510);
            result.descriptor_loaded = true;
            mark_state_change();
            return result;
        }
        mark_state_change();
        return result;
    }

    auto velocity = object.type33_velocity_fixed +
        static_cast<std::int32_t>(object.map_probe_direction) * 0x200;
    velocity = clamp_type33_velocity(velocity, 0x5000);
    object.type33_velocity_fixed = velocity;
    object.world_x_fixed += velocity;
    const bool at_limit = object.map_probe_direction <= 0
        ? velocity <= -0x5000
        : velocity >= 0x5000;
    if (at_limit) {
        object.type33_state = 0;
    }
    mark_state_change();
    return result;
}

Type33TargetStepResult step_type33_target_tail(
    ObjectRecord& object, Type33TargetList& target_list) {
    Type33TargetStepResult result;
    if (target_list.active_count == 0 || target_list.targets.empty()) {
        return result;
    }
    if (object.type33_target_cursor >= target_list.capacity) {
        object.type33_target_cursor = 0;
        result.cursor_wrapped = true;
    }
    const std::size_t index = object.type33_target_cursor;
    if (index >= target_list.targets.size()) {
        return result;
    }
    ++object.type33_target_cursor;
    result.inspected = true;
    result.target_index = index;
    auto& target = target_list.targets[index];
    const auto object_x = object.world_x_pixels();
    const auto object_y = object.world_y_pixels();
    if (object_x - 10 < target[0] && target[0] < object_x + 10 &&
        object_y - 0x23 < target[1] && target[1] < object_y + 5) {
        target[0] = 0;
        result.target_cleared = true;
    }
    return result;
}

Type33TargetRegistrationResult register_type33_target_emitter(
    ObjectRecord& object, Type33TargetList& target_list) {
    Type33TargetRegistrationResult result;
    if (target_list.active_count >= target_list.capacity) {
        return result;
    }
    const std::size_t limit = std::min<std::size_t>(
        target_list.capacity, target_list.targets.size());
    for (std::size_t index = 0; index < limit; ++index) {
        auto& target = target_list.targets[index];
        if (static_cast<std::int32_t>(target[0]) + target[1] != 0) {
            continue;
        }
        ++target_list.active_count;
        object.target_emitter_slot = static_cast<std::uint16_t>(index * 4);
        target[0] = 1;
        result.admitted = true;
        result.target_index = index;
        return result;
    }
    return result;
}

void publish_type33_target_emitter_position(
    const ObjectRecord& object, Type33TargetList& target_list) {
    const std::size_t index = object.target_emitter_slot / 4;
    if (index >= target_list.targets.size()) {
        return;
    }
    target_list.targets[index][0] = static_cast<std::int16_t>(
        object.world_x_pixels());
    target_list.targets[index][1] = static_cast<std::int16_t>(
        object.world_y_pixels());
}

void release_type33_target_emitter(ObjectRecord& object,
                                   Type33TargetList& target_list) {
    if (target_list.active_count != 0) {
        --target_list.active_count;
    }
    const std::size_t index = object.target_emitter_slot / 4;
    if (index < target_list.targets.size()) {
        target_list.targets[index] = {0, 0};
    }
    object.update_callback = kFreeCallback;
}

Type34Action test_type34_proximity(const ObjectRecord& object,
                                    PlayerState& player,
                                    std::uint16_t activation_state) {
    Type34Action result;
    // 01F7:9C0C uses CMP DS:85DA,0x32 / JGE to skip the proximity helper.
    // The active range is therefore strictly below 0x32.
    if (activation_state >= 0x32 ||
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
