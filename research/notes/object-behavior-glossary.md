# Object-behavior semantic glossary

This glossary gives the reverse-engineering work stable, behavior-oriented
names. These are research labels, not claims about the original Pascal/C
source names. Every label keeps its segment-relative address so it can be
checked against Ghidra and DOSBox traces.

Confidence uses three levels:

- **confirmed** — directly established by disassembly and/or controlled
  runtime traces;
- **probable** — the call contract is clear, but one higher-level gameplay
  interpretation remains open;
- **provisional** — useful for implementation and searching, but deliberately
  easy to rename when a stronger observation appears.

## Shared object lifecycle

| Address | Research name | Confidence | Evidence / scope |
| --- | --- | --- | --- |
| `01F7:0E06` | `are_object_factory` | confirmed | Scans the 64-record pool and initializes the first free record. |
| `01F7:0E96` | `object_update_pass_by_phase` | confirmed | Flips scheduler banks and dispatches phases selected by `object+0x17`. |
| `01F7:1036` | `register_object_scheduler_entry` | confirmed | Copies the live callback and object offset into the active scheduler bank. |
| `01F7:1CDA` | `stream_are_regions` | confirmed | Re-visits declarations in newly visible 64-pixel regions. |
| `01F7:1E04` | `instantiate_are_declaration` | confirmed | Marks an ARE record processed and creates its pooled object. |
| `01F7:1DCA` | `object_camera_visibility_gate` | confirmed | Returns the camera-window acceptance result for `ES:DI`. |
| `01F7:1DEE` | `deactivate_object_outside_camera` | confirmed | Clears the callback and the source declaration's processed marker. |

### Pool and scheduler globals

| Address | Research name | Confidence | Meaning |
| --- | --- | --- | --- |
| `DS:755E` | `object_pool_base` | confirmed | Base of 64 pooled records. |
| `DS:30CE` | `object_stride_bytes` | confirmed | Record stride, `0x78`. |
| `DS:7566` | `scheduler_bank_a` | confirmed | First eight-byte scheduler-entry bank. |
| `DS:7766` | `scheduler_bank_b` | confirmed | Second eight-byte scheduler-entry bank. |
| `DS:7966` | `scheduler_insert_cursor` | confirmed | Bank selector plus insertion cursor; advances by eight per entry. |
| `DS:7968` | `stream_reference_grid_stride` | confirmed | Reference-grid stride used by directional ARE streaming. |
| `DS:796E` | `are_source_segment_selector` | probable | Segment value used with `object+0x1A` to reach the source ARE record. |
| `DS:88C8` | `scheduler_active_count` | probable | Incremented while the update pass dispatches entries. |
| `DS:81D2` | `are_entity_dispatch_table` | confirmed | Four-byte dispatch entries indexed by ARE type. |

## Pooled-object record

The record is polymorphic: the same word can be a sprite selector for one
family and an animation/state value for another. The names below describe the
stable use seen so far, while the address remains the authoritative identity.

| Offset | Research name | Confidence | Observed use |
| --- | --- | --- | --- |
| `+0x04` | `world_x_fixed` | confirmed | World X position in the common 16.16 object transform. |
| `+0x08` | `world_y_fixed` | confirmed | World Y position in the common 16.16 object transform. |
| `+0x12` | `sprite_slot_or_descriptor` | confirmed | Logical BOB/sprite selector for normal drawable objects; `0xFFFF` for some state objects. |
| `+0x17` | `scheduler_phase` | confirmed | Scheduler phase byte, observed as `0`, `1`, or `2`. |
| `+0x18` | `update_callback_offset` | confirmed | Near callback offset; zero means the record is free/inactive. |
| `+0x1A` | `source_are_pointer` | confirmed | Object offset of the originating ARE declaration; its high marker byte is cleared on cull. |
| `+0x1C` | `update_callback_segment` | confirmed | Far callback selector paired with `+0x18`. |
| `+0x1E` | `descriptor_reload_delay` | confirmed | Reload value copied from the sequence's first word by `load_animation_descriptor`. |
| `+0x20` | `descriptor_timer` | confirmed | Countdown decremented by `advance_animation_descriptor`; reloads from `+0x1E`. |
| `+0x22` | `descriptor_sequence_base` | confirmed | Fixed sequence start, set to `DS:SI+2` by `load_animation_descriptor`. |
| `+0x24` | `descriptor_sequence_cursor` | confirmed | Byte cursor advanced by two and redirected by signed jump words. |
| `+0x28` | `descriptor_mode` | provisional | Selects an alternate descriptor adjustment when equal to `0xFFFF`. |
| `+0x29` | `map_probe_direction` | confirmed | Direction byte used when forming the type-`0x33` MAP probe. |
| `+0x2A` | `target_slot_or_type33_travel_counter` | confirmed | Shared-emitter slot byte offset for `45AB`; type `0x33` uses the same word as its travel counter. |
| `+0x2B` | `player_contact_state` | provisional | Set to `0xFF` by `apply_player_displacement`; broader state meaning is open. |
| `+0x2C` | `type33_phase` | confirmed | Signed acceleration phase used by the type-`0x33` transition branch. |
| `+0x2D` | `type33_phase_timer` | confirmed | Signed-countdown reversal timer, reloaded to `0x14` on expiry. |
| `+0x2F` | `type33_transition` | confirmed | MAP/state transition flag; positive selects the acceleration/state-transition path. |
| `+0x30` | `shared_target_cursor` | confirmed | Cursor into the shared `DS:87DE` target list; the `0x8AE5` type-`0x33` tail uses this word. |
| `+0x32` | `type33_state` | confirmed | Type-`0x33` motion substate: `0`, `1`, `2`, or `3`. |
| `+0x33` | `type33_state_counter` | confirmed | State-`2` hold counter; transition occurs after it passes `0x2D`. |
| `+0x35` | `type33_animation_counter` | confirmed | State-`0` animation counter; resets and sets the transition flag after `0x50`. |
| `+0x37` | `player_collision_class` | confirmed | Returned in `CL` by `query_player_collision_state`; gates type-`0x34` proximity. |
| `+0x44/+0x48` | `player_position_snapshots` | confirmed | Persistent-player position snapshots used by the player callback. |

The collision-bound fields at `+0x2C/+0x2E/+0x30/+0x32` are intentionally not
given one global interpretation. On the persistent player record they form
the bounds offsets consumed by `compute_player_collision_bounds`; on other
families, nearby fields participate in animation or state machines.

## MAP and descriptor helpers

| Address | Research name | Confidence | Contract |
| --- | --- | --- | --- |
| `01F7:3376` | `map_tile_id_lookup_16px` | confirmed | Converts 16-pixel coordinates to a MAP cell and returns its low 9-bit tile ID. |
| `01F7:1C6E` | `map_word_probe_16px` | confirmed | Computes a 16-pixel MAP address, returns the raw word, and tests bit `0x4000`. |
| `01F7:1C4D` | `check_object_map_contact` | probable | Forms a directional object probe and forwards it to `map_word_probe_16px`. |
| `01F7:5C27` | `map_descriptor_quadrant_probe` | confirmed | Masks the raw MAP word, indexes the descriptor table, and tests a coordinate-selected low-nibble flag. |
| `01F7:5CC3` | `map_descriptor_read` | confirmed | Returns the descriptor word for a masked MAP tile. |
| `01F7:393C` | `compute_player_collision_bounds` | confirmed | Returns four bounds from the persistent player record at `DS:881A`, or zeroes when `DS:89EA` is nonzero. |
| `01F7:39FE` | `query_player_collision_state` | confirmed | Returns player/bounds X, Y, and collision-class byte for type `0x34`. |
| `01F7:69FF -> 6D01` | `player_target_collision_handoff` | confirmed | Player-position gate followed by a shared-target consumer; clears a matching X and switches the object to callback `0x4AB3`. |
| `01F7:5D38` | `load_animation_descriptor` | probable | Loads a descriptor/table entry into object animation state and selects the current slot/action. |
| `01F7:5D60` | `advance_animation_descriptor` | probable | Decrements the active timer or advances the descriptor cursor when it expires. |

Relevant data names:

| Address | Research name | Confidence |
| --- | --- | --- |
| `DS:657A` | `map_buffer_offset` | confirmed |
| `DS:657C` | `map_buffer_segment` | confirmed |
| `DS:657E` | `map_row_stride_bytes` | confirmed |
| `DS:6582` | `tile_descriptor_table_offset` | confirmed |
| `DS:6584` | `tile_descriptor_table_segment` | confirmed |
| `DS:30D4` | `tile_descriptor_stride` | confirmed |
| `DS:87DE` | `shared_target_list` | confirmed | Shared target-list entries, stored as signed X/Y word pairs. |
| `DS:8806` | `shared_target_list_active_count` | confirmed | Number of active producer objects; nonzero enables the inline target scans. |
| `DS:8808` | `shared_target_list_capacity` | confirmed | Capacity and cursor wrap bound for the shared target list. |
| `DS:880C` | `target_emitter_pending_budget` | confirmed | Alternate admission budget consumed by `4519` when `DS:88AE` is not positive. |
| `DS:88AE` | `target_emitter_spawn_gate` | confirmed | Positive byte admits the `4519` producer path and avoids consuming `DS:880C`. |

The names `floor`, `ceiling`, and `wall` are intentionally not used for
descriptor bits yet. The player-side traces establish the consumers of bits
`0x20` and `0x40`, but the gameplay orientation still needs both boundary
directions.

## Type-specific callbacks

| Type | Address | Research name | Confidence | Behavior summary |
| ---: | --- | --- | --- | --- |
| `0x2C` | `01F7:8C4E` | `init_are_type_2c` | confirmed | Installs the steady callback and initializes the paper-family object. |
| `0x2C` | `01F7:8D20` | `update_are_type_2c` | confirmed | Persistent steady callback; its action helper consumes player bounds. |
| `0x2C` | `01F7:8D31` | `update_are_type_2c_action_helper` | probable | Uses `compute_player_collision_bounds`; terminal clear path is at `0x8E42`. |
| `0x33` | `01F7:87D1` | `init_are_type_33` | confirmed | Initializes the snow-family object and descriptor state. |
| `0x33` | `01F7:882F` | `update_are_type_33` | confirmed | Autonomous motion plus MAP/descriptor branch. |

### Normal-family target-tail associations

These are static callback-body associations, not guesses from the German
asset labels. Each update callback reaches the listed inline shared-target
tail before returning:

| ARE types | Update callback | Shared target tail | Family |
| --- | --- | --- | --- |
| `0x01/0x02` | `01F7:6DC4` | `01F7:707B` | WURM2 |
| `0x03/0x04` | `01F7:68C0` | `01F7:6D01` | BIENE |
| `0x05/0x06` | `01F7:7B71` | `01F7:7E1A` | FISCH |
| `0x07/0x08` | `01F7:778C` | `01F7:7A85` | KRABBE |
| `0x09/0x0A` | `01F7:715E` | `01F7:76BF` | PENGO |
| `0x0B/0x0C` | `01F7:66E1` | `01F7:67E0` | SCHNEE |
| `0x15/0x16` | `01F7:7EF8` | `01F7:83AF` | FLIEGE |
| `0x17/0x18` | `01F7:8472` | `01F7:8773` | SPINNE |
| `0x19/0x1A` | `01F7:5071` | `01F7:5399` | BUGGY |
| `0x1B/0x1C` | `01F7:5F28` | `01F7:62AE` | UFO |
| `0x33` | `01F7:882F` | `01F7:8AE5` | snow-family object |
| `0x35/0x36` | `01F7:546D` | `01F7:58A7` | later normal family |

The generic object tracer can inject one target relative to the selected
object (`--probe-target-x-delta`/`--probe-target-y-delta`). Its default cursor
field is `+0x30`; the high-address family can be probed with
`--probe-target-cursor-offset 0x2A`. Camera-centered dynamic checks confirm
target-X clearing and cursor advancement for representatives of `707B`,
`6D01`, `76BF`, `67E0`, `5399`, `62AE`, and `58A7`. The `707B`, `6D01`,
`76BF`, and `5399` families switch to callback `4AB3`; `67E0` is clear-only;
`62AE` and `58A7` run their `+0x42` action paths, with `58A7` subsequently
reaching `4AB3`.

The five high tails are statically grouped as dedicated steady callbacks
`B25D -> B266`, `BB0E -> BB17`, `C328 -> C331`, `CDA3 -> CDAC`, and
`D55A -> D563`. Their match path clears target X, increments the separate
`+0x2C` hit counter, calls `4B70`, sets scheduler phase `+0x17=2`, and then
continues into the effect-specific state update. Their gameplay identities
remain intentionally unnamed pending a live camera-centered spawn trace.

| shared | `01F7:44FF` | `reset_target_list` | confirmed | Sets target capacity to four, clears the active count, and zeroes target slots. |
| shared | `01F7:4519` | `init_target_emitter` | confirmed | Reserves a shared target-list slot and installs callback `45AB`. |
| shared | `01F7:45AB` | `update_target_emitter` | confirmed | Publishes emitter position and releases when its slot/camera gate invalidates. |
| shared | `01F7:470C` | `release_target_emitter` | confirmed | Decrements the active count, clears the target pair, and clears the callback. |
| `0x34` | `01F7:9BEE` | `init_are_type_34` | confirmed | Initializes the bump-family object and descriptor state. |
| `0x34` | `01F7:9C0C` | `update_are_type_34` | confirmed | Proximity-driven steady callback. |
| `0x34` | `01F7:9C29` | `test_type34_proximity` | confirmed | Strict X/Y proximity test gated by player collision class. |

## Type-0x34 action chain

| Address | Research name | Confidence | Evidence |
| --- | --- | --- | --- |
| `01F7:1B5D` | `apply_player_displacement` | probable | Sets player state, subtracts `0x1B000` from the player position dword, and returns action input `0x000A` in the hit trace. |
| `01E7:0FCF` | `emit_effect_action` | probable | Publishes effect code `0x2A` at `DS:504C` and conditionally forwards `DS:612E`. |
| `DS:85DA` | `type34_activation_state` | confirmed | Type `0x34` callback enters the visibility/descriptor/proximity path only when this byte is strictly below `0x32`; `0x32` and above return immediately. |
| `DS:612E` | `pending_action_word` | confirmed | Changes from `0` to `4` on the controlled type-`0x34` proximity hit. |
| `DS:504C` | `pending_effect_code` | confirmed | Receives `0x2A` in the action/effect trace. |
| `DS:881A` | `player_object_offset` | confirmed | Persistent player object offset; W1L1 uses pool record offset zero. |
| `DS:89EA` | `player_collision_mode` | provisional | Zero/nonzero selects the bounds-helper mode; exact broader gameplay role remains open. |

## Naming rules for the C++ model

1. Use the research names above in code and keep the numeric address in a
   comment or trace metadata.
2. Use `fixed16_16` for `+0x04/+0x08`; do not silently convert these fields to
   integer pixels at the storage boundary.
3. Represent polymorphic fields with names scoped to the callback or behavior
   (`type33_descriptor_cursor`, `type34_action_state`) when their global
   meaning is not established.
4. Keep `unknown_0xNN` for fields without a tested consumer; do not invent
   gameplay names such as `floor` or `damage` from one trace.
5. When a new trace changes a provisional label, update this glossary and the
   Ghidra annotation script together.
