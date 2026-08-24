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
| `+0x2B` | `player_contact_state` | provisional | Set to `0xFF` by `apply_player_displacement`; broader state meaning is open. |
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
| `0x34` | `01F7:9BEE` | `init_are_type_34` | confirmed | Initializes the bump-family object and descriptor state. |
| `0x34` | `01F7:9C0C` | `update_are_type_34` | confirmed | Proximity-driven steady callback. |
| `0x34` | `01F7:9C29` | `test_type34_proximity` | confirmed | Strict X/Y proximity test gated by player collision class. |

## Type-0x34 action chain

| Address | Research name | Confidence | Evidence |
| --- | --- | --- | --- |
| `01F7:1B5D` | `apply_player_displacement` | probable | Sets player state, subtracts `0x1B000` from the player position dword, and returns action input `0x000A` in the hit trace. |
| `01E7:0FCF` | `emit_effect_action` | probable | Publishes effect code `0x2A` at `DS:504C` and conditionally forwards `DS:612E`. |
| `DS:85DA` | `type34_activation_state` | provisional | Type `0x34` callback checks this state against `0x32` before its proximity path. |
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
