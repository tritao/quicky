#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <vector>

namespace quiky::research {

constexpr std::size_t kObjectPoolSize = 64;
constexpr std::uint16_t kFreeCallback = 0;
constexpr std::uint16_t kSourceUnset = 0xffff;

constexpr std::int32_t fixed16_16_from_pixels(std::int32_t pixels) {
    return pixels * 0x10000;
}

constexpr std::int32_t pixels_from_fixed16_16(std::int32_t fixed) {
    return fixed >> 16;
}

struct ObjectRecord {
    // Common object fields, retaining the executable's storage terminology.
    std::int32_t world_x_fixed = 0;       // object +0x04
    std::int32_t world_y_fixed = 0;       // object +0x08
    std::uint16_t sprite_or_action = 0xffff; // object +0x12
    std::uint8_t scheduler_phase = 0;     // object +0x17
    std::uint16_t update_callback = kFreeCallback; // object +0x18
    std::uint16_t source_are_offset = kSourceUnset; // object +0x1A
    std::uint16_t update_callback_segment = 0; // object +0x1C

    // Descriptor sequence state used by 01F7:5D38/5D60.
    std::uint16_t descriptor_reload_delay = 0; // object +0x1E
    std::uint16_t descriptor_timer = 0;        // object +0x20
    std::uint16_t descriptor_sequence_base = 0; // object +0x22
    std::uint16_t descriptor_sequence_cursor = 0; // object +0x24
    std::uint8_t descriptor_mode = 0;           // object +0x28

    std::int8_t map_probe_direction = 0; // object +0x29
    std::uint8_t player_contact_state = 0; // object +0x2B, player use
    // Type-0x33 motion fields. These bytes are polymorphic in other object
    // families, so keep the callback-specific names here.
    std::int8_t type33_phase = 0; // object +0x2C
    std::uint16_t type33_phase_timer = 0; // object +0x2D
    std::int8_t type33_transition = 0; // object +0x2F
    std::int32_t type33_velocity_fixed = 0; // object +0x0A
    std::uint8_t type33_state = 0; // object +0x32
    std::uint16_t type33_state_counter = 0; // object +0x33
    std::uint16_t type33_travel_counter = 0; // object +0x2A
    std::uint16_t type33_animation_counter = 0; // object +0x35
    std::uint16_t type33_target_cursor = 0; // object +0x30
    // The same +0x2A word is a target slot byte offset for 45AB emitters;
    // type-0x33 uses the callback-specific travel counter above instead.
    std::uint16_t target_emitter_slot = 0;
    std::uint16_t update_state = 0; // object +0x32
    std::uint8_t player_collision_class = 0; // object +0x37, player use

    [[nodiscard]] bool active() const { return update_callback != kFreeCallback; }
    [[nodiscard]] std::int32_t world_x_pixels() const {
        return pixels_from_fixed16_16(world_x_fixed);
    }
    [[nodiscard]] std::int32_t world_y_pixels() const {
        return pixels_from_fixed16_16(world_y_fixed);
    }
};

class ObjectPool {
public:
    [[nodiscard]] std::optional<std::size_t> first_free() const;
    [[nodiscard]] ObjectRecord& at(std::size_t index);
    [[nodiscard]] const ObjectRecord& at(std::size_t index) const;

    // Matches 01F7:1DEE: only the callback is cleared; the remaining record
    // bytes, including the source pointer and position, survive the cull.
    void deactivate(std::size_t index);

private:
    std::array<ObjectRecord, kObjectPoolSize> records_{};
};

struct SchedulerEntry {
    std::uint16_t callback_offset = 0;
    std::uint16_t callback_segment = 0;
    std::uint16_t object_index = 0;
    std::uint8_t phase = 0;
};

class ObjectScheduler {
public:
    using Bank = std::vector<SchedulerEntry>;

    // Rebuild one bank from active pool records. Pool order is retained inside
    // each phase, matching the append-only scheduler table observed in DOSBox.
    void rebuild(std::uint8_t bank, const ObjectPool& pool);
    [[nodiscard]] const Bank& bank(std::uint8_t bank) const;
    [[nodiscard]] Bank dispatch_order(std::uint8_t bank) const;

private:
    std::array<Bank, 2> banks_{};
};

struct DescriptorMemory {
    std::uint16_t base_address = 0;
    std::vector<std::int16_t> words;

    [[nodiscard]] std::int16_t read(std::uint16_t address) const;
};

// Models 01F7:5D38. initial_word_address is the SI input: the first word is
// the reload delay, and the following word begins the action sequence.
void load_descriptor(ObjectRecord& object,
                     const DescriptorMemory& memory,
                     std::uint16_t initial_word_address);

// Models 01F7:5D60. Returns true when a sequence entry was resolved; returns
// false when only the nonzero timer was decremented.
bool advance_descriptor(ObjectRecord& object, const DescriptorMemory& memory);

struct Type33MotionContext {
    // DS:646C is a byte ring consumed when the state-0 travel counter passes
    // 0x32. The index is DS:6468, shared by type-0x33 objects.
    std::array<std::int8_t, 256> travel_ring{};
    std::uint8_t travel_ring_index = 0;
};

struct Type33StepResult {
    bool map_set_transition = false;
    bool descriptor_loaded = false;
    bool state_changed = false;
};

// Models the recovered motion portion of 01F7:882F. The caller supplies the
// result of the directional 5C27 MAP/descriptor probe: a zero result sets
// object+0x2F before the motion state machine runs. The common DS:8806 target
// tail is intentionally outside this function and is modeled separately by
// step_type33_target_tail.
Type33StepResult step_type33_motion(ObjectRecord& object,
                                    const DescriptorMemory& memory,
                                    Type33MotionContext& context,
                                    bool map_probe_zero);

// View of the shared DS:87DE target list used by the type-0x33 tail and
// several other inline object consumers.
struct Type33TargetList {
    std::uint16_t active_count = 0; // DS:8806
    std::uint16_t capacity = 0; // DS:8808
    std::vector<std::array<std::int16_t, 2>> targets; // DS:87DE, x/y words
};

struct Type33TargetStepResult {
    bool inspected = false;
    bool cursor_wrapped = false;
    bool target_cleared = false;
    std::size_t target_index = 0;
};

// Models the common 01F7:8AE5 tail. It advances the per-object cursor every
// active pass and clears only the matching target X word; target Y survives.
Type33TargetStepResult step_type33_target_tail(
    ObjectRecord& object, Type33TargetList& target_list);

struct Type33TargetRegistrationResult {
    bool admitted = false;
    std::size_t target_index = 0;
};

// Models the static 01F7:4519/45AB/470C target-emitter slot lifecycle.
Type33TargetRegistrationResult register_type33_target_emitter(
    ObjectRecord& object, Type33TargetList& target_list);
void publish_type33_target_emitter_position(
    const ObjectRecord& object, Type33TargetList& target_list);
void release_type33_target_emitter(ObjectRecord& object,
                                   Type33TargetList& target_list);

struct PlayerState {
    std::int32_t world_x_fixed = 0;
    std::int32_t world_y_fixed = 0;
    std::uint8_t collision_class = 0;
    std::uint8_t contact_state = 0;
    std::int32_t displacement_field_fixed = 0;
};

struct Type34Action {
    bool proximity_hit = false;
    std::uint16_t action_word = 0;
    std::uint16_t effect_code = 0;
    std::int32_t player_displacement_fixed = 0;
};

// Models the strict branch at 01F7:9C29 plus the observed 01F7:1B5D /
// 01E7:0FCF output. The returned action/effect values represent DS:612E and
// DS:504C; they are not stored in the pooled object record.
Type34Action test_type34_proximity(const ObjectRecord& object,
                                    PlayerState& player,
                                    std::uint16_t activation_state);

} // namespace quiky::research
