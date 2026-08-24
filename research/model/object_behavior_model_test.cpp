#include "object_behavior_model.h"

#include <cassert>
#include <cstdint>

using namespace quiky::research;

static void test_type33_descriptor_sequence() {
    const DescriptorMemory memory{
        0x3504,
        {10, 0x00d6, 0x00d7, 0x00d8, 0x00d9, -4, 6, 0x00d7},
    };
    ObjectRecord object;
    object.descriptor_mode = 1;
    load_descriptor(object, memory, 0x3504);
    assert(object.descriptor_reload_delay == 10);
    assert(object.descriptor_timer == 10);
    assert(object.descriptor_sequence_base == 0x3506);
    assert(object.descriptor_sequence_cursor == 0x3506);
    assert(object.sprite_or_action == 0x00d6);

    for (int count = 0; count < 10; ++count) {
        assert(!advance_descriptor(object, memory));
    }
    assert(object.descriptor_timer == 0);
    assert(advance_descriptor(object, memory));
    assert(object.descriptor_sequence_cursor == 0x3508);
    assert(object.sprite_or_action == 0x00d7);
    object.descriptor_timer = 0;
    assert(advance_descriptor(object, memory));
    assert(object.descriptor_sequence_cursor == 0x350a);
    assert(object.sprite_or_action == 0x00d8);
    object.descriptor_timer = 0;
    assert(advance_descriptor(object, memory));
    assert(object.descriptor_sequence_cursor == 0x350c);
    assert(object.sprite_or_action == 0x00d9);
    object.descriptor_timer = 0;
    assert(advance_descriptor(object, memory));
    assert(object.descriptor_sequence_cursor == 0x3506);
    assert(object.sprite_or_action == 0x00d6);
}

static void test_descriptor_mode_adjustment() {
    const DescriptorMemory memory{0x3568, {6, 0x0190, 0x0192, 0x0193, 0x0191, -1}};
    ObjectRecord object;
    object.descriptor_mode = 0xff;
    load_descriptor(object, memory, 0x3568);
    assert(object.sprite_or_action == 0x01c2);
    object.descriptor_timer = 0;
    assert(advance_descriptor(object, memory));
    assert(object.sprite_or_action == 0x01c4);
    object.descriptor_timer = 0;
    assert(advance_descriptor(object, memory));
    assert(object.sprite_or_action == 0x01c5);
    object.descriptor_timer = 0;
    assert(advance_descriptor(object, memory));
    assert(object.sprite_or_action == 0x01c3);
    object.descriptor_timer = 0;
    assert(advance_descriptor(object, memory));
    assert(object.descriptor_sequence_cursor == 0x3570);
    assert(object.sprite_or_action == 0x01c3);
}

static void test_pool_and_scheduler() {
    ObjectPool pool;
    pool.at(3).update_callback = 0x1234;
    pool.at(3).source_are_offset = 0x1632;
    pool.at(3).scheduler_phase = 2;
    pool.at(7).update_callback = 0x5678;
    pool.at(7).scheduler_phase = 0;
    assert(pool.first_free() == 0);
    pool.deactivate(3);
    assert(pool.first_free() == 0);
    assert(pool.at(3).source_are_offset == 0x1632);

    pool.at(0).update_callback = 0x1111;
    pool.at(0).scheduler_phase = 1;
    ObjectScheduler scheduler;
    scheduler.rebuild(0, pool);
    const auto order = scheduler.dispatch_order(0);
    assert(order.size() == 2);
    assert(order[0].object_index == 7);
    assert(order[1].object_index == 0);
}

static void test_type34_proximity() {
    ObjectRecord object;
    object.world_x_fixed = fixed16_16_from_pixels(128);
    object.world_y_fixed = fixed16_16_from_pixels(404);
    PlayerState player;
    player.world_x_fixed = fixed16_16_from_pixels(128);
    player.world_y_fixed = fixed16_16_from_pixels(400);
    player.collision_class = 1;

    const auto miss = test_type34_proximity(object, player, 0x31);
    assert(!miss.proximity_hit);
    const auto hit = test_type34_proximity(object, player, 0x32);
    assert(hit.proximity_hit);
    assert(hit.action_word == 4);
    assert(hit.effect_code == 0x2a);
    assert(hit.player_displacement_fixed == -0x1b000);
    assert(player.contact_state == 0xff);
    assert(player.displacement_field_fixed == -0x1b000);

    object.world_x_fixed = fixed16_16_from_pixels(103);
    assert(!test_type34_proximity(object, player, 0x32).proximity_hit);
}

int main() {
    test_type33_descriptor_sequence();
    test_descriptor_mode_adjustment();
    test_pool_and_scheduler();
    test_type34_proximity();
}
