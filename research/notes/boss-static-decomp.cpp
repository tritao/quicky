/*
 * Focused Ghidra closure: pooled world-boss/end-stage callbacks.
 *
 * The authoritative contract rows are in
 * research/ghidra/boss-static-closure.json. The generated C-like export and
 * instruction listing are produced by run_boss_static_closure.py with
 * Ghidra's x86:LE:16:Protected Mode language. No objdump output is used.
 *
 * ES:DI is a 0x78-byte pooled object record. Constructors and callbacks are
 * far procedures. The scheduler does not consume their return flags; flags
 * returned by nested opaque helpers remain address-qualified and are not
 * converted into guessed C booleans.
 */

struct BossObject {
    s32 x_02_05;                 // fixed-point X
    s32 y_06_09;                 // fixed-point Y
    s32 velocity_x_0A_0D;        // fixed-point X velocity
    s32 velocity_y_0E_11;        // fixed-point Y velocity
    u16 sprite_slot_12;
    u8  phase_17;
    u16 callback_18;
    u16 phase_or_timer_2A;
    u16 damage_count_2C;
    u8  hit_latch_2E;
    u8  hit_cooldown_2F;
    u16 effect_cursor_30;
    u8  state_34;
    u16 timer_38;
    u8  direction_3E;
    u8  direction_40;
    u16 timer_42;
    u16 effect_burst_44;
    u16 child_46;
    u16 child_48;
};

/* Shared static contracts. */

// 01F7:0E06 -> 01F7:1036
// Allocate and publish a pooled record. Exact allocator failure flags are
// retained at the assembly boundary; constructors do not expose them to the
// scheduler as a callback return value.
extern BossObject *boss_pool_allocate_0E06();
extern void boss_scheduler_insert_1036(BossObject *object);

// 01F7:0E96 then 01F7:0FA2
// Main-loop ordering is confirmed by relocation/call-site analysis: phase
// callbacks are dispatched before the selected nonzero-state list, where the
// player callback is reached. This order is simulation-relevant.
extern void object_update_pass_by_phase_0E96();
extern void object_update_pass_nonzero_state_0FA2();

// 01F7:44FF/4519/45AB/470C
// DS:87DE is a four-byte-row coordinate/effect table. 4519 is capacity-guarded
// by DS:8806 < DS:8808 and accepts either DS:88AE or DS:880C as its producer
// gate. 45AB updates live rows and 470C removes them.
extern void reset_effect_rows_44FF();
extern bool spawn_effect_row_4519(/* original register contract */);
extern void update_effect_rows_45AB();
extern void remove_effect_row_470C();

/* Shared boss damage contract. */

// Every world-specific damage callback selects one row using object+0x2A and
// compares strict bands:
//   boss_x - 15 < row_x < boss_x + 15
//   boss_y - 25 < row_y < boss_y + 5
// A match clears row.x, advances the cursor/counter, writes DS:612E=0x000D,
// and can allocate a world-specific child/effect. There is no direct player
// record write. Thresholds are W1=5, W2=6, W3=5, W4=6, W5=4 consumed hits.
extern void consume_w1_boss_damage_B25D(BossObject *o);
extern void consume_w2_boss_damage_BB0E(BossObject *o);
extern void consume_w3_boss_damage_C328(BossObject *o);
extern void consume_w4_boss_damage_CDA3(BossObject *o);
extern void consume_w5_boss_damage_D55A(BossObject *o);

/* World-specific callback skeletons. */

// 01F7:B142 -> B33B, W1: slot 0x3B7, vx=-0x9000, children +0x2A/+0x36.
// B25D promotes DS:88AE 1->2 after five rows. B33B writes stages 3, 4, 5
// at its recovered effect/exit boundaries and may clear/replace children.
extern void initialize_w1_boss_B142(BossObject *o);
extern void update_w1_boss_B33B(BossObject *o);

// 01F7:B9F3 -> BBEC, W2: slot 0x10E, vx=-0x7000, children +0x2A/+0x36.
extern void initialize_w2_boss_B9F3(BossObject *o);
extern void update_w2_boss_BBEC(BossObject *o);

// 01F7:C28A -> C40B, W3: slot 0x3B7, vx=-0x13000, child +0x36.
// C328 and C40B use an increment for the stage-3 -> stage-4 transition.
extern void initialize_w3_boss_C28A(BossObject *o);
extern void update_w3_boss_C40B(BossObject *o);

// 01F7:CC68 -> CE81, W4: slot 0x3B7, vx=vy=-0x9000.
extern void initialize_w4_boss_CC68(BossObject *o);
extern void update_w4_boss_CE81(BossObject *o);

// 01F7:D2F6 -> D63D, W5: slot 0x3B6, vx=-0x13000, vy=-0x370000,
// children +0x2A/+0x36/+0x48.
extern void initialize_w5_boss_D2F6(BossObject *o);
extern void update_w5_boss_D63D(BossObject *o);

/* Completion boundary. */

// 01D7:4EA0 consumes the completion gate and reaches the 4F10/5010 selector
// and reload continuation. The completion producer and authored resource
// values are still external data boundaries; no C++ progression behavior is
// inferred from them yet.
extern void boss_transition_consumer_4EA0();
extern void boss_completion_selector_4F10_5010();

/* Static conclusions. */

// 1. The boss callbacks do not directly mutate the player record.
// 2. Their simulation feedback is through pooled object state, DS:87DE row
//    consumption, DS:88AE stage writes, and callback insertion/removal.
// 3. Effect/sound/render leaves remain address-named until a parity mismatch
//    demonstrates feedback into the next player callback.
// 4. Repeated authored damage timing and completion production need targeted
//    runtime traces; they are not guessed from the callback state machines.
