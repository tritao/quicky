# W1L1 gameplay-object inventory

This is the level-scoped object inventory for the next faithful-W1L1 slice.
It is derived from the shipped `W1L1.ARE` entry in `game/NESTLE.DAT`, not from
the archive-wide type catalog.

Reproduce the placement counts with:

```sh
python3 research/tools/quikyctl.py are-info <extracted>/W1L1.ARE --json --entities
python3 research/tools/quikyctl.py entity-experiment-plan game/NESTLE.DAT \
  --level W1L1.ARE --json
```

The archive entry has a `68x8` stream layout and 109 declaration references.
Resolving only references present in that layout yields 173 streamed
placements across 21 distinct types. The following counts are the source
inventory; repeated declarations are intentionally retained in the count.

| ARE types | placements | recovered pipeline | player/global effect | W1L1 status |
| --- | ---: | --- | --- | --- |
| `01` | 10 | normal pool via `01F7:0E06`; initializer `6DA3/6DB1`; callback `6DC4` | enemy contact routes to `4AB3/4C5D`; off-camera removal clears the callback and source marker | gameplay family closed for classification |
| `03`, `04` | 7, 3 | normal pool via `0E06`; initializers `689F/68AD`; callback `68C0` | enemy contact routes to `4AB3/4C5D`; fixed-point patrol and MAP blocking affect future contact | state-zero native; states 1–8 require injected startup table |
| `1F`, `20` | 1, 1 | parent state setup `8B3D/8B50`; shared state machine `8E4B`; child effects `16CE -> 10B5` | MAP-driven emissions publish `DS:8828/882A`; transition routine `1AAA` can consume that state and reposition the player | state/effect closure implemented; authored selector still open |
| `28` | 1 | special class-0 object; initializer/callback `9256 -> 9269` | aligned overlap can write `DS:89E6=FFFF` behind the `DS:89EA` and player `+0x37` gates | transition-gate side effect closed; special renderer remains open |
| `29`, `2A`, `2B` | 5, 12, 39 | leaf source through `0E06`; child initializer `474D`; child callback `47E7` | no persistent player/entity collision; MAP blocking only changes pooled leaf motion | presentation-only for player simulation |
| `65`, `67` | 66, 14 | dedicated handlers `178D/17A3 -> 1749`; transient event child `16CE -> 10B5` | no standard player collision or persistent gameplay write observed; event ring and short-lived ICO effects | presentation-only; semantic subtype names remain unresolved |
| `6F`, `71`, `72` | 2, 4, 1 | standard pool; initializers `8BC2/8C08/8C2B`; shared callback `8D20` | aligned player overlap clears the object and writes pickup effects: `DS:612E`, `DS:880C`, `DS:881C` (subtype-specific) | collectible family closed for implementation |
| `79`–`7F` | 1 each | standard pool; initializers `8C71`–`8D07`; shared callback `8D20` | overlap ORs bit `1,2,4,8,0x10,0x20,0x40` into `DS:60D8`, emits `DS:612E=11`, then clears the object; all-seven completion is an outer transition path | collectible family closed; natural completion/reload route remains open |

## Closure classification

The 21 types reduce to eight scheduler families:

1. `wurm2` enemy: `01`.
2. `biene` enemy: `03/04`.
3. MAP-driven animated tile effect: `1F/20`.
4. Cloud/transition gate: `28`.
5. Falling leaves: `29/2A/2B`.
6. Dedicated loop/transient event: `65/67`.
7. Pickup collectible: `6F/71/72`.
8. Puzzle-letter collectible: `79`–`7F`.

No platform-family ARE declaration occurs in W1L1. This is a bounded negative
for the authored ARE inventory only; it does not rule out a pooled or
script-created platform. The platform scheduler/carry boundary therefore
remains a separate runtime investigation rather than being silently modeled
as an ARE object.

## Implementation order

The first W1L1 scheduler addition is now the shared `8D20` collectible
callback. Its interaction contracts are explicit and feed directly into
player-visible globals. The native implementation records the callback
identity, subtype writes, sound-action global, player timer write, and deferred
object release:

1. `8D20` with subtype tables for pickups and puzzle letters — implemented.
2. `6DC4`/`68C0` normal enemy callbacks and their existing `4AB3` contact
   boundary — state-zero WURM2/BIENE patrol and lifecycle are implemented;
   BIENE states 1–8 now accept an explicit `DS:7974` replay table, while its
   DOS-time/software-float generator remains an address-qualified boundary.
3. `8E4B` environmental state machine and `16CE -> 10B5` child effects —
   implemented; `1AAA` remains an explicit transition contract.
4. `9269` cloud gate and `DS:89E6` transition publication.
5. Leaf and dedicated-event families as deterministic presentation objects.

The family evidence is already recorded in
`research/entity-behavior-families.json` and the per-type initializer/resource
contracts in `research/entity-types.json`. This report adds the W1L1 scope and
placement counts; it does not promote unresolved renderer or authored
completion behavior to gameplay semantics.
