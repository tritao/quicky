# Rendering behavior research

This note is the rendering slice of the reverse-engineering work. It separates
behavior observed in `QUIKY.EXE` from the current SDL frontend policy. The
latter is useful for exercising assets, but must not be promoted to an engine
fact without a runtime trace or a controlled pixel experiment.

## Evidence already established

### Resource resolution

- MAP, ARE, BOB, and ICO loaders all construct `GAMEDATA\\` Pascal paths and
  enter the common archive lookup at `0207:18C7`. The lookup publishes the
  resource start/end/size in `DS:97E8`, `DS:97E4`, and `DS:97EC`.
- World rendering loads `Wn.PCC` and `Wn.ICO`. ICO records are 256-byte,
  16x16 indexed tiles stored in VGA column-interleaved order; the inverse
  permutation is implemented by `Tileset::parseIco`.
- The animated world-ICO state machine (`ARE` types `0x1F`-`0x21`) uses the
  live MAP tile ID, masks it to nine bits, maps it through `DS:6986`, and
  resolves the resulting tile in the current `Wn.ICO`. Runtime probes match
  W1 states 120-124, W2 states 126-130, W3 states 400-404, W4 states 240-244,
  and W5 states 61-64 byte-for-byte against the corresponding ICO records.
- Dedicated event types `0x65`-`0x67` use `LOOP_Wn.ICO`, not a standard BOB
  slot. The live `01F7:1186 -> 01F7:11B4` path draws four short-lived ICO
  objects. Cross-world probes match W2 record 8, W3 record 6, W4 record 22,
  and W5 record 4 for the selected event variants.
- Standard entity slots resolve through the renderer-facing object field
  `object+0x12` and the slot map at `DS:6D8E`. Native-context probes show the
  live descriptor geometry and origin equal the selected BOB record for the
  ordinary enemy, effect, platform, pickup, and puzzle-letter families.
- The native ICO tile helper at `01F7:11B4` is not a transparent-zero blitter:
  it selects VGA planes `0x11`, `0x22`, `0x44`, and `0x88` and copies every
  four-byte source group from the 256-byte `FS:BX` tile block to video memory,
  with no zero test. An indexed-zero ICO pixel therefore clears the earlier
  BOB/MAP pixel. The PCC helper is different: `0207:099C` explicitly skips
  zero pixels before calling `0207:0944`.

### Early-screen, menu, and HUD assets

Static archive extraction resolves the screen-sized PCC family without
assuming a draw order:

| asset family | PCX canvas | observed role/content |
| --- | ---: | --- |
| `MENU.PCC` | 320x200 | purple menu background with a centered framed panel |
| `NES.PCC` | 320x200 | Argon/“Concept & Development” title card |
| `NES2.PCC` | 320x200 | Quiky Games logo title card |
| `INTROBAR.PCC` | 320x24 | repeating orange/brown intro strip |
| `GAMEBAR.PCC` | 320x24 | textured in-game status-bar strip |
| `BIGFONT.PCC` | 320x48 | red large font atlas |
| `SMFONT.PCC` / `INFONT.PCC` | 320x8 | small yellow/brown font atlases |

All are 8-bit, single-plane PCX images with a trailing 768-byte DAC palette;
the title cards use 230 and 144 distinct pixel indices respectively, while
the bars and fonts are sparse atlases. This establishes the source geometry
and palette ownership for title/menu/HUD rendering, but not whether the game
uploads each asset's palette wholesale or composites over a prior palette.
The extracted source files and the visual montage used for this inventory are
in the main-worktree build at `/home/joao/dev/quicky/research/build/early-assets`.

Static VGA I/O inspection also resolves the palette-upload primitives in
`QUIKY_SEG04.bin`: the helper at offset `0x0536` selects DAC index 0 (`3C8h`)
and loops exactly `0x100` times, writing three bytes per entry to `3C9h` from
a 768-byte buffer. The helper at `0x05FF` updates one indexed entry. The code
immediately before the full upload transforms all 256 RGB triples before
uploading them, so fades or palette adjustments are executable capabilities,
not merely frontend speculation. The callers that select title/menu/gameplay
states and the exact transition curve still need runtime checkpoints.

A debugger-side DAC trace closes part of that gap. Using the debugger build
from the main worktree, the controlled W4L1 selector launch was run with
`research/tools/quikypalette.py --samples 8 --startup-samples 4`. The optional
startup window reached frame 530 without a hit on either palette helper; the
post-selector window then produced this ordered prefix:

| frame | helper | caller return | DAC index | RGB |
| ---: | --- | --- | ---: | --- |
| 1283 | `0207:05FF` single-entry | `0207:0929` | 255 | 0,0,0 |
| 1287 | `0207:05FF` single-entry | `0207:0929` | 254 | 0,0,0 |
| 1289 | `0207:05FF` single-entry | `0207:0929` | 253 | 0,0,0 |
| 1291 | `0207:05FF` single-entry | `0207:0929` | 252 | 0,0,0 |
| 1293 | `0207:05FF` single-entry | `0207:0929` | 251 | 0,0,0 |
| 1296 | `0207:05FF` single-entry | `0207:0929` | 250 | 0,0,0 |
| 1298 | `0207:05FF` single-entry | `0207:0929` | 249 | 0,0,0 |
| 1300 | `0207:05FF` single-entry | `0207:0929` | 248 | 0,0,0 |

The caller disassembly initializes its local index to `0xFF`, calls the
single-entry helper with three zero color components, decrements, and loops;
the live stack values match that code. This is direct evidence of a
level-entry fade-to-black sequence, not a generic assumption based on the
frontend capture. It does not prove that no full 256-entry upload occurred
before the script attached, nor does it resolve the palette curve for title,
pause, death, or level-exit transitions. The reproducible ledger is kept at
`research/build/traces/render-w4l1-palette.json` when generated locally.

An earlier attach-gated run covers the start of the executable rather than a
level launch. `research/tools/quikyearlypalette.py` starts DOSBox with a
protected-mode debugger build paused before `QUIKY.EXE`, arms both palette
helpers, and releases the guest after the descriptors are available. The first
observed event was the single-entry helper at frame 97 (`0207:05FF`), with DAC
index `255` and RGB `0,0,0`; the helper's stack arguments decode to the same
values. No `0207:0536` full-upload hit occurred before that first event. This
narrows the earliest observed transition to the same descending black-entry
operation, but it cannot rule out a full upload that ran before debugger
attachment or identify which title/menu state owns the write.

A longer W1L1 run with sixteen optional startup samples reached frame 1882
without a helper hit, then captured sixteen post-selector writes from frame
2636 through 2671: indices `255` down through `240`, all RGB `(0,0,0)`. This
repeats the W4L1 pattern across a second world and still leaves the title/menu
full-upload boundary outside the protected-mode observation window.

A manual Xvfb capture of the unmodified game (`dosbox`, 16,000 cycles,
45 seconds) shows the attract sequence as: DOSBox splash, Argon card, a black
interval, Quiky logo card, another black interval, then a 320x200 gameplay/demo
scene with the speech-bubble overlay. The Argon and Quiky cards match the
`NES.PCC`/`NES2.PCC` pixels above. The dark-to-bright card changes are
consistent with palette fades, but this capture does not expose DAC writes or
the input-driven menu path, so it is screen-sequence evidence only.

A second Xvfb capture injected `Escape` at roughly 1-4 second intervals. It
reached the menu and showed a centered 320x200 logical frame (presented as a
640x400 viewport): the `MENU.PCC` purple/four-corner background and white
frame are stable, while red menu text from the large-font family is drawn over
the black panel. The `QUIKY` line blinked on and off in consecutive samples;
the surrounding `INFO`, `SOUND`, and `EXIT` labels stayed fixed. This confirms
a PCC background plus a separately drawn font layer and a time-varying menu
element. It still does not identify the exact palette upload boundary or the
selector's draw call order.

The live menu font probe catches the text blitter at `01D7:0047` before the
selector route. Across 64 calls (frames 399-473), every call carries the same
Pascal string `QUICKY`, logical origin `(100,160)`, and alignment word
`0xfef4`; there are no skipped calls in that window. This makes the selected
line a per-frame draw rather than a one-shot asset copy. The probe then arms
both DAC helpers for eight waits and sees no `0207:0536` full upload or
`0207:05ff` single-entry write through frame 606. Thus the visible blink is
not caused by a palette-helper write in this live-menu interval; the fixed
labels use a different text path or the blink is produced by page/bitmap
composition. The reproducible ledger is
`research/build/traces/render-transitions-menu-font-palette-v1.json`.

Static segment-1 callers correct the earlier fixed-label interpretation:
`01D7:03F4` is the small numeric/character copier used by score and status
strings, not the main-menu label path. The menu setup helper at `01D7:0470`
passes literal `INFO`, `SOUND`, and `EXIT` strings through the same 16x16
blitter at `01D7:0047`, while the settled loop passes the variable `QUICKY`
line through `0047` every frame. The live probe
`research/build/traces/render-transitions-menu-fixed-font-startup-v1.json`
sees no `03F4` entry, which is expected for this menu route. A direct selector
action-mask injection now reaches the `0470` call and captures `INFO`; the
remaining dynamic menu task is page handoff and blink timing.

That launch-time checkpoint was then armed with a DOSBox pause and deferred
breakpoint (`research/build/traces/render-transitions-menu-setup-paused-v1.json`).
It still skipped `0470` while the replay reached the normal menu and continued
to observe the per-frame `0047`/`QUICKY` calls. This was a one-shot route issue,
not evidence that the labels are PCC-owned; the asset/pixel comparison below
rules that out, and the direct selector injection now proves the runtime
caller.

A direct stopped-menu injection removes that ambiguity. At the post-input
return (`01D7:13A5`), writing action mask `1` to the normalized/raw action
words reaches the selector branch at `01D7:13E0`, then the static `call 0470`
and a live `0047` invocation. The first captured fixed-label call is Pascal
text `INFO` at logical `(160,60)`; the static body continues with `SOUND`, the
runtime `DS:60C6` label, and `EXIT`. This is archived in
`research/build/traces/render-transitions-menu-nav-mask1-only-v4.json`.
The exact page handoff and blink cadence remain separate questions, but the
fixed-label caller is now dynamically reached rather than merely inferred.

A direct asset/pixel check rules out baking those labels into `MENU.PCC`: the
extracted `MENU.PCC` palette does contain the red DAC colors, but its 320x200
pixel surface contains no red label glyphs, while the stopped `menu-ready`
frame contains about 10k red label pixels (`INFO`, `SOUND`, and `EXIT`). The
labels are therefore a runtime overlay (or a separate font surface) using the
shared menu palette. `MENU.PCC`, `BIGFONT.PCC`, `SMFONT.PCC`, and `INFONT.PCC`
have byte-identical 768-byte palettes except for index 0, which narrows the
palette-owner ambiguity to a shared UI palette rather than four independent
color systems. The exact page handoff and blink timing still need native
capture.

The menu-confirmation probe closes the selector's transition callsite. Pressing
Enter at the settled menu stops in `0207:022A` with `transition_flag=1` and
`fade_step=20`; its far return address is `01D7:1473`, the selector function's
confirmation branch. In this branch `022A` zero-fills `DS:5C9C` and calls
`02A5` directly, so the observed menu-entry ramp is a blacking transition,
not the `01D7:3020` PCC-copy path. The earlier live-color stage is captured
below; the remaining menu work is upload timing/page handoff and the selector
blink, not the palette source itself.

That earlier palette stage is now captured at the settled menu boundary. The
`menu-ready` checkpoint reports descriptor `DS:60BC = 7` (`MENU.PCC`) and a
`DS:6118` 0x400-byte table whose 256 records are **exactly**
`(1, MENU_R >> 2, MENU_G >> 2, MENU_B >> 2)` for every extracted `MENU.PCC`
DAC entry; `DS:60E4` is byte-identical. This closes the menu palette source
and its 6-bit conversion contract even though the protected-mode `0536` upload
itself occurs outside the stopped probe window. The reproducible ledger is
`research/build/traces/render-transitions-menu-ready-palette-buffer-v1.json`.

Static I/O scanning found one additional DAC write site outside the common
palette helpers: `01F7:0E21` writes index `0` as RGB `(0x3f,0,0)` while the
object allocator is handling a full pool. The menu transition probe armed this
site alongside `0207:0536`/`05ff` and saw no hit through the live menu window,
so it is not the observed selector blink owner. The live palette source is the
settled `MENU.PCC` staging table described above; the remaining question is
when that table reaches the DAC and which VGA page becomes visible.

Targeted decompilation resolves the earlier setup boundary. `01D7:3020` loads a
0x400-byte palette staging buffer, draws the selected PCC through `0207:00E3`,
then calls `0207:02A5` with fade step `0x14`. `02A5` reads 256 four-byte
records, computes a 16-bit ramp for each RGB component, packs the high bytes
into a 768-byte upload buffer, and enters `0207:0536` for the full DAC write.
The menu-confirmation probe now reaches the selector's direct callsite at
`01D7:1473 -> 0207:022A` with `transition_flag=1` and `fade_step=20`; the
wrapper takes its flag-one branch, clears `DS:5C9C` with the `0227:1B7E`
byte-fill helper, and ramps that zero source through `02A5`. Thus the direct
menu-entry fade is measured as a blacking transition. The settled checkpoint
identifies the live source table, so the remaining menu work is upload
timing/page handoff and selector blink, not palette ownership. This also
explains why the GAMEBAR descriptor can retain its original pixel indices:
palette selection is a separate setup pass.

The attach-before-launch probe
`research/tools/quiky_palette_owner_trace.py` (ledger
`research/build/traces/render-palette-owner-v3.json`) arms `01D7:50C3`,
`01D7:3020`, `0207:02A5`, and `0207:0536` with deferred protected-mode
breakpoints. The scripted attract/menu replay reaches frame 1279 without any
of those targets. This is useful negative evidence: the `3020` ramp is not a
per-frame menu path and must be triggered by a narrower title/selector state or
reload branch. The static chain remains authoritative for the owner; the next
dynamic experiment should enter the exact state that calls `50C3`, rather than
repeating a generic startup replay.

The follow-up exit/reload probe
`research/build/traces/render-w1l1-goal-all-exit-x200/exit-focus-palette-owner-v2.json`
keeps `50C3`, `3020`, and `02A5` armed through the authentic completion and
reload path. It hits `02A5` first at frame 1340 with a non-menu transition
source and `fade_step = 35`, then reaches the normal 20-upload fade-to-zero,
resource reload, GAMEBAR setup, and post-cleanup renderer without hitting
`50C3`/`3020` through frame 2256. This separates the generic transition ramp
from the PCC palette setup and rules out the forced gameplay reload as the
missing `3020` trigger. A no-input title/intro run
(`research/build/traces/render-palette-owner-title-v2.json`) also reaches frame
1168 without those targets. These negative traces should not be read as
evidence that the chain is dead code; the remaining dynamic question is the
exact settled-menu upload frame/cadence and page handoff.

The checkpointed transition trace records the input-driven path with the
debugger stopped at each boundary. The host saved the corresponding rendered
frames under `research/build/traces/render-transitions-frames/`:

| checkpoint | frame | observed screen |
| --- | ---: | --- |
| `menu-ready` | 352 | `MENU.PCC` purple background, framed black panel, red menu labels |
| `cheat-selector` | 632 | selector/gameplay-like screen after the verified cheat route |
| `selector-dispatch` | 815 | black transition interval |
| `first-resource-lookup` | 867 | still black while the first archive lookup runs |
| `first-render-frame` | 963 | first dark gameplay frame after renderer entry |

The attempted Escape return did not reach the selector input wait before the
trace deadline (the real `KBD_esc` path also timed out at frame 3044). Thus
title/menu composition and the selector's blanking interval are observed; a
successful exit and Escape-to-menu return remain unverified. The separate long
right-input run below now verifies the gameplay death/recovery path.

Static segment-1 disassembly adds two UI boundaries to the asset inventory:
`01D7:3310` initializes the SMFONT/INFONT/BIGFONT/MENU/GAMEBAR/INTROBAR/NES
pointer table, `01D7:3020` copies the selected 0x400-byte PCC surface and
associated 0x14-row strip, and `01D7:313D` dispatches the selector/menu state
from `DS:85D4`. A live transition run also armed `01D7:313D` and `01D7:3020`
around the cheat-selector path; both optional checkpoints timed out, while the
verified selector dispatch (`01D7:4B18`) and first resource/render boundaries
hit normally. These helpers are therefore static/menu entry points not reached
on that observed selector-to-level path. The follow-up ledger also armed the
scoreboard/HUD compositor at `01D7:14E1`; it timed out at frame 1488 after the
first gameplay render. Static callers show that routine is reached from the
outer completion/exit path (`01D7:4F0D`), not from the ordinary per-frame
renderer, so the normal gameplay HUD strip still needs a call-site-specific
probe rather than being inferred from the menu copy helper.
The static HUD call site is now narrowed further. During the resource/setup
routine at `01D7:3FB0`, the branch at `01D7:4486` (or `4495` in intro mode)
passes the loaded `GAMEBAR.PCC` (or `INTROBAR.PCC`) pointer to `0207:0B6C` at
`(0,0)`. That helper selects the PCC descriptor and its `0207:099C` loop writes
only nonzero indexed pixels through the planar `0944` writer. This explains
why the bar can be prepared before the world list, but a runtime stop at
`4486`/`0B6C` plus a sprite straddling row 176 is still needed to settle the
final page and viewport precedence.

The native completion/reload trace now reaches that call site without a
coordinate patch. In
`research/build/traces/render-w1l1-goal-all-exit-x200/exit-focus-gamebar-v6.json`,
the sequence is `3FB0` (frame 1634), `4486` (1713), `0207:0B6C` (1714), and
the helper return `0BEF` (1718). The call has `DS:60BE=5`, `DS:60C0=6`,
`DS:85DA=0`, and passes descriptor id 5 with `(x,y)=(0,0)`; the descriptor
table entry contains the 320x24 `GAMEBAR.PCC` geometry and a live far pixel
pointer. The helper renders into the hidden page (`DS:817A=0x82A0`) and
restores the page globals at `0BEF`. The first post-cleanup renderer page is
still black, while the second renderer stop shows the vivid gameplay page
with an opaque black separator at row 176 and GAMEBAR pixels on rows 177-199.
This closes the native HUD call-site and page-handoff timing. The independent
BOB/ICO/MAP zero-write rule is established by the segment-3 helper decompile;
HUD-versus-entity clipping is tested separately below.

The follow-up live-buffer probe
`research/build/traces/render-transitions-gamebar-buffer-v3.json` snapshots the
descriptor at the ordinary `01D7:4486` GAMEBAR call. `DS:60BE` is descriptor id
5; its `DS:5196 + 5*0x16` entry reports a far pixel buffer at offset `0`,
selector `0x337`, width `320`, height `24`, and size `7680` bytes. The first
`1024` live bytes match the first `1024` bytes of the extracted
`GAMEBAR.PCC` PCX decode exactly. The native loader/compositor therefore does
not remap GAMEBAR pixel indices before `0207:099C`; resource-owned indices
`99..123` remain intact. The remaining palette task is specifically to locate
the setup path that makes the GAMEBAR DAC entries active (or composes them with
the world palette), not a pixel-buffer conversion problem.

A controlled native BOB boundary run settles that remaining HUD-specific
question. The W1L1 trace
`research/build/traces/render-w1l1-hud-straddle-y444-slot700.json` targets live
slot 700 (`BLATT.BOB`, descriptor 14x12, origin 7,12) and rewrites one native
draw call from `(272,268)` to `(272,444)`, which puts its top at screen y=170
with the live camera at y=262. The stopped raw frame
`.../006-after-bob.png` shows the leaf's opaque green pixels at rows 173-175,
an entirely black row 176, and no leaf pixels in rows 177-199 despite the
GAMEBAR being present there. This is direct native evidence that the world BOB
blitter clips at the 320x176 viewport before the opaque GAMEBAR rows; there is
no BOB/HUD same-pixel precedence to emulate.
The candidate setup ledger `render-transitions-ui-hud-v8.json` also armed the
three screen-copy helpers (`01D7:0880`, `0C4C`, `10D6`), the PCC copy
(`01D7:3020`), the GAMEBAR-state candidate (`01D7:3FB0`), and `01D7:14E1`
before the first level lookup. None of those candidates fired; the only hit in
that arm set was the expected `0207:18C7` resource lookup. This narrows the
ordinary HUD path to code outside those one-shot/status candidates or to a
direct page copy not yet identified by a symbolic routine boundary.
The same candidate set was re-armed between the first resource lookup and the
first world render; it produced only the expected `01F7:35C7` render hit. This
rules out those named helpers as both the pre-render and post-lookup HUD copy
boundary in the verified launch path.

The player-side transition probe separates a normal wall stop from that open
case: four 180-frame right-input holds reached `(1167,336)` and set the live
vertical/reset-related byte `+0x3A` to `1`, but did not enter the level-reset
sequence. A debugger-only attempt to seed `(2131,368)` and `+0x3E=1000` held
that state without advancing it, so those fields are checkpoints rather than a
valid synthetic death trigger.

A long, real right-input run does reach the game's hazard/reset path without
writing player memory. W1L1 moves the live player from `(128,400)` to
`(2132,368)` at the hazard boundary; the player remains in the animated death
state for the next 60-frame sample (`action_word=4`, sprite slots `28` then
`26`, and `+0x3E=1000`). By the following sample the game has restored normal
control at checkpoint `(1673,368)` (`action_word=0`, sprite slot `0`). This is
direct evidence of an authentic death/recovery transition and a checkpoint
respawn, not the earlier synthetic field-seeding attempt. The run is archived
as `player-w1l1-death-recovery-next.json`.

The focused writer arm was corrected to remove the hot `01D7:4601` outer-loop
breakpoint, which previously starved the three writer targets. The resulting
55-second W1L1 right-input run (`render-w1l1-exit-focus-v4.json`) advances the
camera to `map_x=2213` while retaining `DS:89E6=0` and records no hit at
`4996`, `4AAC`, or `92A9`. This is a bounded negative for the ordinary hazard
route: death/recovery is not itself a level-exit writer. It does not rule out
an exit declaration elsewhere in the level.

The palette helper was also armed during an authentic W1L1 right-input run.
After the launch fade, the trace held `KBD_right` through frame 2709; no
additional `0207:0536` or `0207:05FF` event occurred during the death/recovery
window (through frame 2276). This is negative evidence that the observed death animation does not
reuse the launch's descending black-entry helper in that sample (it does not
rule out a non-DAC fade or an earlier palette operation). The ledger is
`render-w1l1-palette-death-v2.json`.

A controlled overlap fixture was built from W1L1. The first declaration is the
real animated-tile state-machine type `0x1F`; the debugger patches its five
live MAP cells to effect tiles `200..204`, which makes it create the transient
ICO object through the normal `01F7:16CE -> 01F7:1186` path. The companion
declaration is ordinary BOB type `0x04`. In
`render-w1l1-overlap-ico-bob/pixel-same-draw-ack8.json`, the BOB return at
sequence 8 is a debugger-only draw-coordinate override to `(768,240)` while
the entity remains at its original world position; the ICO object reports the
same `(768,240)` at sequence 9, followed by tile helper returns 1-4. Thus both
families are instantiated in one live sequence and their cross-family order is
measured: BOB return first, then ICO tile writes. The later ICO write is the
framebuffer winner for every covered pixel, including indexed zero, because the
decompiled `11B4` path writes all four planes unconditionally. Paired
remove-first/remove-second archives remain negative controls; the screenshot
harness can still show an intermediate backbuffer at some stopped BOB
boundaries, so the static helper contract is the authoritative
transparent/opaque result. `quiky_overlap.py` also supports independent record
targets for future sprite-origin experiments.

The same test was then rebuilt without a debugger draw-coordinate override.
`quiky_overlap.py` independently relocates the ARE declarations so the live
renderer receives BOB slot `226` at `(768,256)` and the transient ICO at
`(768,240)`; `pixel-natural-v2.json` records those native arguments, followed
by the four `11B4` tile returns. This closes the robustness check for a real
archive-instantiated overlap. The final-color rule remains the static one
above: BOB/MAP writes happen first, then ICO writes all four planes, including
zero.

The synchronized renderer harness now stops at each completed draw boundary,
saves the rendered PNG, and stores the guest arguments beside it. In
`render-w4l1-pixel-trace-tiles24.json`, every event was captured (33 events and
33 PNGs): BOB returns at `01F7:35B8` are followed by four ICO helper returns at
`01F7:1192`, `119B`, `11A7`, and `11B0`. The first ICO tile batch used
`FS:0x035F`, offset `0xDC00`; later batches selected `0xD900` and `0xD600`.
The incremental image differences are localized to the ICO sprite region (for
example, the first tile transitions occupy the rendered crop around
`(341,122)-(417,393)`), confirming that screenshots are taken after each
helper's pixel writes rather than at a later present tick. The W1L1 overlap
trace resolves the same-coordinate cross-family order; the remaining visual
caveat is page presentation at some stopped BOB boundaries, not an unknown
transparent-mask policy.

A goal fixture relocates the native W1L1 puzzle-letter family into a streamed
anchor, and a companion fixture places all seven types (`0x79`-`0x7F`) at one
coordinate. The latter loads slots `600`-`606` with update callback
`01F7:8D20`; their object `+0x2A` values are the expected bit masks
`1,2,4,8,0x10,0x20,0x40`. In the bounded player trace those seven objects stay
resident, while `DS:60D8` remains `0`, `DS:89E6` remains `0`, and no transition
state changes. This is a useful negative control, not proof that normal
gameplay cannot collect the letters.

Static code establishes the intended completion mechanism: callback
`01F7:8D20` ORs the object mask into `DS:60D8` and deactivates the object; the
segment-1 completion routine compares `DS:60D8` with `0x7F` at protected-mode
address `01D7:1669`, branches at `01D7:16C6` to draw `BONUS-LEVEL!`, and enters
transition setup at `01D7:1709`. Three segment-3 sites write the level-exit
flag `DS:89E6` (`01F7:4996`, `4AAC`, and `92A9`). A debugger-only run seeded
`DS:60D8=0x7F` and `DS:89E6=1`, reached a second gameplay render, and then
timed out at the outer exit gate `01D7:4EA0`; it did not reach the completion
branch or a menu return. The fixture and ledgers are under
`research/build/traces/render-w1l1-goal-all-start/`, while the synthetic
transition ledger is `render-w1l1-goal-transition-mask7f.json`.

The player tracer now supports a slot-filtered object-callback stop and an
arbitrary callback-body offset. The original all-letter fixture at `(128,400)`
was a deliberate negative control: the callback body (`01F7:8D36`) shows the
letter exactly on the strict vertical rejection edge, and the slot-600 object
returns resident with `DS:60D8=0`. A regenerated copy at `(128,390)` reaches
the mask-setting branch (`01F7:8E29`) for slot 600; the branch trace records
`DS:60D8=1` and the callback cleared (`+0x18=0`) after the first letter. The
same valid fixture, traced at the post-setup body entry over subsequent
samples, reaches the remaining letters and finishes with `DS:60D8=0x7F`.
This resolves the live collision gate: the object x interval must strictly
overlap the x bounds, and the y interval must strictly overlap the dynamic
player bounds (the exact edge at y=400 is rejected). It also confirms the
native `+0x2A` bit masks are ORed in collection order; the outer completion
transition remains a separate open call-site problem.

### Targeted state/render decompile

The NE relocation tables are now applied by
[`ne_target_decompile.py`](../tools/ne_target_decompile.py), so the far calls
in the raw segment bytes are reported with their runtime selectors. The
reproducible output used for this pass is
`research/build/traces/ne-target-decompile.txt` (generated, ignored output).
The segment-1 state function at `01D7:45B1` has this live frame structure:

```text
45B1 setup/resource copy -> 45FE state/menu dispatch
47F0 update calls -> 4829 01F7:3062 -> 482E 01F7:35C7
4872 second update pass -> 48C2 01F7:3062 -> 48C7 01F7:35C7
48E6 post-render state checks -> 491D input/state dispatch
4CFC transition-stage gate -> 4D03 staged effects -> 4EA0 exit-flag gate
4EA0 -> 4F0D completion/HUD helper -> 5010 reload -> 504F loop/cleanup
```

The exit flag is a sentinel, not a boolean: the live writers at
`01F7:4996`, `01F7:4AAC`, and `01F7:92A9` store `0xFFFF`, and the main loop
tests that exact value at `01D7:48E6`. Once the sentinel path is selected,
`4968` clears `89EC`, `4B8D` rejects the path when `89EE == -1`, and the
`4BA4`/`4C43`/`4CB1` gates inspect `89EA`, `880A`, and `89F0` before reaching
`4CFC`. `4CFC` dispatches `85DA <= 1` directly to `4EA0`; larger stage values
run the explicit effect/title cases at `4D06`-`4E8E` and increment `85DA`.
The completion/HUD routine at `4F0D` then maps selector states through
`85D4/85D6`, while `5010` either reloads the level resources or exits through
`504F` depending on `89E0`/`89EC`.

This corrected the debugger fixture: its former seed `DS:89E6=1` was invalid
and intentionally bypassed the sentinel branch. With `0xFFFF` and neutral
adjacent gates (`89EE=0`, `89EA=0`, `89F0=0`, `89F4=0`, `880A=0`, `85DA=0`),
the values persist, but a 30-second deep arm remains in the timer/VGA helper
`0207:10AA` (`EIP 10BA`-`10CB`) after the stopped `35C7` frame. That is a
probe-timing/synchronization limitation caused by seeding state at a renderer
boundary; it does not supersede the static branch proof. A future dynamic
run should trigger one of the native `01F7:4996/4AAC/92A9` writers and arm
the state gates before the transition frame rather than patching them after
`35C7`.

The normal per-frame renderer therefore has two page-copy/world-render pairs;
the scoreboard helper at `01D7:14E1` is not in this chain. `01D7:1669` is a
separate state routine containing the `DS:60D8 == 0x7F` test and the
`BONUS-LEVEL!` branch at `16C6`; it is not a direct near-call from the main
loop and should be treated as a state-dispatch target rather than as a child
of `4F0D`.

The segment-3 targets resolve as follows. `01F7:2CB2` prepares MAP strips;
`2F36`, `2F71`, `2FE9`, and `3062` copy prepared strips/pages through VGA
state; `35C7` walks the active object list and invokes the clipped BOB path at
`36D5`; `3749` applies the 320x176 viewport bounds (`DS:81B0=320`,
`DS:81B4=176`). No GAMEBAR pointer or font resource is read by `3062` or
`35C7`, which rules them out as the gameplay HUD compositor. The static BOB
entry also tests caller mode bit `0x0100` and temporarily enables all VGA
planes for the opaque/all-plane variant; the live W4L1 list uses mode zero.

The three native exit-flag writers have now been bounded to their triggering
families. `01F7:4996` is in the generic collision/update callback: after a
strict object/player overlap it awards `0x1388`, checks the level/state word
`DS:85D8` against `1`, `3`, and `5`, and otherwise marks the object terminal
and stores the `DS:89E6=0xffff` sentinel. The `01F7:4AAC` callback belongs to
the `DS:85D4==0x0e` staged event. Its object timer advances through stage
`0x32`, can set `DS:89EC=0xffff` at the intermediate threshold, then resets
the stage and finally stores `DS:89E6=0xffff` after `0x258` ticks. The
`01F7:92A9` callback is a separate collision trigger: it computes a strict
overlap against the player bounds, checks the player descriptor byte
`+0x37`, and only then writes the same sentinel. These are static trigger
classifications; a native run that reaches each writer is still required to
map them to concrete ARE declarations and screen effects.

A surgical dynamic arm after the first world frame is archived as
`render-transitions-state-path-v11.json`. It hit `01D7:48E6` and `01D7:491D`
on consecutive frames with `DS:89E6=0`, `DS:89E0=0`, and `DS:89EC=0`. The
deeper `4CFC`, `4D03`, `4EA0`, `4F0D`, `5010`, and `504F` boundaries all
timed out in this ordinary selector launch. This is a bounded negative result:
the normal run remains in the input/update branch, so the completion/exit
chain needs a fixture that reaches the state-dispatch conditions, not more
one-shot HUD guesses. The hot-loop entries were probed independently to avoid
starving deeper targets.

The debugger-seeded completion arm repeats that result under the stronger
condition `DS:60D8=0x7F` and `DS:89E6=1`: the fixture ledger
`render-w1l1-goal-transition-state-path-v2.json` reaches `48E6` at frame 932
and `491D` at frame 953, but still skips `4CFC`, `4D03`, `4EA0`, `4F0D`,
`5010`, `504F`, and the separate `1669/16C6/1709` state routine through the
bounded arm. This rules out the simple hypothesis that the mask or exit flag
alone enters the completion chain; the preceding state words/dispatch event
must be driven as well.

A native all-letter fixture now separates collection from transition dispatch.
The player is moved to the measured valid overlap `(128,390)` only after the
first renderer boundary; the seven real callbacks then drive `DS:60D8` to
`0x7f` by frame 937. The resulting ledger is
`render-w1l1-goal-all-valid/native-goal-transition-v1.json`. Despite the
native mask reaching `0x7f`, no `89e6` writer, `1669`, `16c6`, `1709`, or
`4ea0` checkpoint fires through frame 2028. This is stronger than the earlier
synthetic-mask negative: the collection callbacks are live and complete, but
the state-machine dispatch that consumes the completed mask is still gated by
another level-state word/event. It is not valid to wire “all seven letters ->
immediate exit” into the engine yet.

The type-`0x28` fixture closes the missing native exit-writer bridge. The W1L1
ARE record at offset `0x180a` is streamed into the player overlap and dispatches
`01F7:9256 -> 9269 -> 92A9`; the live `92A9` breakpoint at frame 893 records
the strict collision and the next sample has `DS:89E6=0xffff`. The same run
then reaches the real segment-1 state path: `01D7:4cfc` (frame 895),
`4d03` (896), and `4ea0` (897). Around the transition call, `4f0d` invokes its
completion/HUD helper at frame 1317, enters the `14e1` state routine at
`1669` (1342), and reaches transition setup at `1709` (1394). The
`DS:60d8` value is `0` in this exit fixture, so the `1669` test does not take
the `BONUS-LEVEL!` branch; this is an exit-trigger path, not proof of the
all-letter completion screen. The archive is
`research/build/traces/render-w1l1-exit-type28/exit-focus-v3.json`.

The probe also armed `5010` (resource-reload gate) and `504f` (cleanup). The
`504f` one-shot was consumed by the ordinary pre-trigger loop at frame 855,
while no `5010` hit occurred after `1709` through the bounded hold. Thus the
native exit trigger and transition staging are now measured, but level reload,
menu return, and the exact transition effect/blanking interval remain open.

To separate the completion branch from the exit trigger, the all-letter fixture
was combined with a second type-`0x28` fixture at world `(200,390)`. The player
was held at `(128,390)` until the seven native callbacks produced
`DS:60D8=0x7f` (frame 1032), then moved to `(200,390)`. The resulting native
sequence reaches `92a9` (1033), `4cfc` (1035), `4d03` (1036), `4ea0` (1037),
`4f0d` (1457), `1669` (1482), **`16c6` (1534)**, and `1709` (1595), with the
mask still `0x7f`. This is the first live proof that `BONUS-LEVEL!` is selected
on the real exit path; the ledger is
`research/build/traces/render-w1l1-goal-all-exit-x200/exit-focus-v1.json`.
The post-setup reload/menu return is still not observed, so its screen timing
and resource ownership remain open.

### Native completion-screen framebuffer

The same composite fixture was rerun with raw video capture at the transition
checkpoints. `research/build/traces/render-w1l1-goal-all-exit-x200/exit-focus-raw-v1.json`
contains exact 320x200 PNGs rather than the scaled presentation surface. At
`16c6` (frame 1539) the page is the level score card: a purple patterned
background, a white rectangular frame from `(61,39)` through `(259,168)`, a
black panel occupying `(68,45)` through `(251,161)`, red `LEVEL 1`, and yellow
`SCORE`, `CEREALS`, and `NESQUIK` rows (the fixture displays `100`, `0`, and
`2000`, respectively). At `1709` (frame 1601) the same card adds the yellow
`BONUS-LEVEL!` row. Both images use the same 16-color indexed
palette; the dominant background is RGB `(49,0,101)`, panel black is `(0,0,0)`,
red title is `(202,0,0)`, and yellow text is `(251,186,0)`.

The targeted `01D7:14E1` decompile now identifies the live score sources and
the bonus update contract. The routine copies the selected level text, stages
the score dword from `DS:881C:881E` through `0207:11AC`, and formats it at
`(0x5a,0xb3)`. It formats the cereal counter from a local incrementing word at
`(0x73,0xcb)` while decrementing `DS:880C` eight times; each pass adds `0x14`
to the score dword and redraws the score. When `DS:60D8 == 0x7f`, the
`16C6` branch draws `BONUS-LEVEL!`, adds `0x7d0` (2000) to the score, and
continues to `1709`. This closes the data-flow question for completion HUD
recreation; a live score/cereal capture is still needed to verify when the
frontend updates those words relative to each raw frame.

The post-`1709` code statically explains why the earlier probe stopped before
reload: `01D7:1f0` and `01D7:1d6` spin on the input mask `(DS:8196 | DS:88bc)
& 0x30` before the `01E7:0caa` and `0207:022a` calls. The no-input ledger
`research/build/traces/render-w1l1-goal-all-exit-x200/exit-focus-input-v2.json`
shows the native sequence stopping at `01D7:1d6` after the first wait. The
confirmation-timed ledger
`research/build/traces/render-w1l1-goal-all-exit-x200/exit-focus-space-v8.json`
then crosses the release/debounce wait (`01D7:1f0`), the confirmation wait
(`01D7:1d6`), and reaches `01E7:0caa`, `0207:022a`, the full-palette helper,
object reset (`01F7:0002`), `01D7:5010`, the `0207:18c7` resource lookup, and
`01D7:504f` cleanup. The confirmation seed is injected only at `01D7:01bd`,
after the native input-refresh call, so this does not bypass the state
machine. At cleanup, `DS:89e6` and `DS:60d8` are cleared and the map reload
state is reset. The remaining transition work is visual timing and the
post-cleanup menu/level screen, not an unknown completion renderer.

The native-key follow-up
`research/build/traces/render-w1l1-goal-all-exit-x200/exit-focus-space-native-v2.json`
presses Space only after `01D7:1d6` is entered and holds it through the real
`01AC` input refresh. It reaches the same `0caa`/`022a`/palette/reset/reload
sequence without writing the input words. After `01D7:504f`, the next live
boundaries are a new renderer entry (`01F7:35c7`) and the normal post-render
input branch (`01D7:48e6`/`491d`); no selector/menu dispatch (`4ace`/`4b18`)
occurs before the trace timeout. For this W1L1 completion path, the native
state machine therefore returns directly to gameplay/next-level setup rather
than immediately returning to the title/menu selector. The next open visual
check is a stopped raw page at that first post-cleanup renderer boundary.

That visual check is now captured in
`research/build/traces/render-w1l1-goal-all-exit-x200/exit-focus-space-native-postcapture-v3.json`.
The first post-cleanup renderer return (frame 1731, PNG
`022-post-cleanup-render.png`) is a near-black page while the transition
palette is still blanked. By the normal post-render/input boundary (frame
1773/1774, `023-post-cleanup-render-input.png`) the page is a vivid blue
gameplay scene with clouds, player/world objects, and the 24-row GAMEBAR. This
confirms a direct gameplay/next-level handoff and puts the visible blank-to-live
interval at roughly 42 native frames for this run; it is not a menu card.

Targeted segment-3 decompilation now closes the ownership of the first reload
steps. `01D7:5010` calls `01F7:0908`, which iterates the 1000-entry object
slot table; `01F7:0931` frees each live resource pointer at object offsets
`+0x10` and `+0x20`, clears those far pointers, and restores the slot map
entry to `0xffff`. The next call, `01F7:1aaa`, invokes `01F7:0b56` to reset
the renderer/object tables, loads the next level's spawn coordinates from the
`DS:8828` table selected by `DS:85d2`, resets the player descriptor, and
installs the initial animation table through `01F7:5d38`. Finally,
`01F7:321f` recomputes the camera from the player fixed-point position and
rebuilds the prepared map pages through `01F7:20af`, `01F7:1ed7`,
`01F7:1cda`, and `01F7:3062`, restoring the saved camera after the page-copy
loop. A faithful reload must therefore tear down object-owned resources
before constructing the next world; simply swapping the MAP name is
insufficient. The dynamic post-cleanup trace already confirms the call order
and W1L2/W1L4 lookup identity, so the remaining reload work is to expose the
first visible rebuilt page and verify optional resource failures, not to infer
another hidden renderer stage.

The repeated-palette ledger
`research/build/traces/render-w1l1-goal-all-exit-x200/exit-focus-palette-loop-v3.json`
re-arms the `0207:0536` DAC-upload entry at its `0539` loop instruction and
records the source palette buffer on every pass. The exit fade performs 20 full
768-byte uploads from frame 1611 through 1685; sampled entries reach exact
zero on the twentieth upload. The palette remains black until the next-level
upload phase begins around frame 1790 (about 105 native frames after the final
fade-out upload), then ramps upward through 20 full uploads. The caller at
`01D7:1724` passes `0x14` (20) into `0207:02a5`; its loop counter runs from 1
through that value, so the static duration agrees with the dynamic count. The
v3 ledger's twentieth next-level upload is frame 1862 (sampled palette entries
127=`(35,0,0)`, 255=`(35,35,35)`, and 16=`(13,0,0)`). The static body of
`0207:02a5` confirms that `0536` writes all 256 RGB triples via VGA `3C8/3C9`
I/O, so this is a palette-buffer fade rather than a framebuffer black fill.
Exit fade-out and next-level fade-in cadence are now measured; only the later
settled-menu upload timing and page handoff remain open.

The attach-before-launch sequence
`research/build/traces/render-title-early-palette-sequence-v5.json` catches all
256 protected-mode operations: `0207:05ff` writes DAC entries continuously
from 255 down to 0, every RGB triple exactly `(0,0,0)`, over native frames
96–133. This confirms that title/menu entry starts with a complete indexed
single-entry black fade rather than a hidden PCC-wide palette swap. The
remaining title question is the later upload/page boundary that installs the
live menu palette; the fade itself is fully measured.

An extended pre-launch run (`render-title-early-palette-sequence-v12.json`)
kept the full-upload entry armed after the 256-entry loop. It records three
successive 255-to-0 single-entry clears (frames 96–210), then four complete
20-call full uploads at frames 213–238, 642–667, 673–699, and 1022–1048,
followed by a new source buffer at frame 1156. The first full-upload source is
the zeroed 0x400-byte ramp buffer; later calls show the same buffer changing
monotonically between zero and live indexed values. This closes the startup
DAC cadence and proves that the live-color transition is a repeated full
upload, not a per-frame selector blink write. Exact VGA page visibility and
which of these startup ramps corresponds to the settled menu still need the
page-handoff checkpoint.

A companion ramp-entry run (`render-title-early-palette-ramp-args-v3.json`)
captures the caller rather than only the DAC loop: at frame 211,
`0207:02A5` receives `fade_step=20` and source pointer `selector:679,
offset:0`, the same far pointer that the settled `menu-ready` checkpoint
reports as `DS:60E4`. The first full uploads follow at frame 213. The source
table is still being staged at that earlier instant, so its bytes are not yet
the final settled asset table; the pointer identity and call order are
nevertheless direct evidence of the menu palette-ramp owner.

The `1709` call argument is the Pascal string `ONGAME2`, not a hidden image
name. The archive contains matching `ONGAME2.TFX` and `ONGAME2.SAM` entries,
and the loader is the already identified SAM/TFX music-resource path. Thus the
post-score-card stage also starts the `ONGAME2` transition track; it should not
be modeled as a PCC/world-image reload. The audio handoff is covered separately
by the music resource implementation; the remaining visual question is the
first post-cleanup gameplay/next-level page and its palette ownership.

The corrected transition lookup probe closes the level-routing identity. The
`18C7` breakpoint is at the entry before its `push bp; mov bp,sp` prologue, so
the far Pascal argument must be read from `(SP-2)+6`/`(SP-2)+8`, not the
caller's EBP frame. With that correction, an authentic W1L1 type-`0x28` exit
with `DS:60D8=0` looks up `GAMEDATA\\W1L2.map`; the all-seven-letter fixture
with `DS:60D8=0x7f` looks up `GAMEDATA\\W1L4.map`. The frontend now tracks
goal-letter bits for ARE types `0x79..0x7f`, routes a full mask to `WnL4`, and
retains ordinary numeric progression otherwise. The reproducible ledgers are
`exit-focus-resource-lookup-v1.json` and
`render-w1l1-goal-all-exit-x200/exit-focus-resource-lookup-v3.json`.

Static decompilation now closes the completion text contract as well. The
`01D7:14E1` routine's inline Pascal strings are `LEVEL`, `BONUS-LEVEL!`,
`SCORE:`, `CEREALS:`, and `NESQUIK:`; its callers pass the live score/cereal
values through the small numeric glyph copier at `01D7:03F4` and draw the
labels through the 16x16 font blitter at `01D7:0047`. The routine selects the
normal card at `16A9` and the bonus branch at `16C6`, then performs the
measured 20-step `0207:02A5` ramp before the input-locked transition. This
leaves the score/cereal data flow identified (`DS:881C:881E` and `DS:880C`),
with only the exact per-frame value timing and frontend wiring as
implementation work, rather than an unknown screen layout.

The renderer pixel probe now also saves raw 320x200 gameplay pages. The W4L1
frame `research/build/traces/render-w4l1-pixel-raw-v1-frames/003-after-bob.png`
shows the native GAMEBAR occupying exactly rows `176..199`; row 176 is an
opaque black separator and the textured/status pixels begin on row 177. The
world renderer therefore owns rows `0..175` and the HUD is a distinct final
surface, consistent with the static `DS:81b4=176` clip bound. This proves the
logical HUD geometry and a world-to-HUD hard boundary, but not yet the exact
ordinary call-site order for a sprite whose source rectangle straddles that
boundary.

### BOB geometry and clipping

The BOB loader reads a repeated `0x2C`-stride runtime descriptor. The relevant
fields are origin (`+04/+06` in the file record), width/height, blitter data,
and offset tables. Segment 3's draw path subtracts the origin from the object
world coordinate, then uses width and height to reject pixels outside the
destination/camera bounds. The safe decoder in `bob.cpp` reconstructs only the
immediate planar writes; it does not execute archive-provided code.

The engine-side contract now covered by tests is:

```text
left = world_x - origin_x       top = world_y - origin_y
for each decoded pixel:
    skip transparent pixels
    skip destination x/y outside the surface
    otherwise write the palette index
```

The confirmed palette side effect is also modeled by
`Palette::blackoutEntry`/`Palette::blackoutDescending`: each selected indexed
entry is replaced with RGB `(0,0,0)`, with the observed fade represented as a
high-to-low range. The operation is covered by the format tests but is not
automatically attached to the SDL frontend's transitions; the latter still
needs title/menu/death/exit timing evidence.

A static census of all 951 BOB records in the extracted archive narrows the
draw-mode question. Every record contains the four-plane `EE D0 C0` phase
marker and passes the safe immediate-write decoder. 911 records use both
immediate byte (`C6 84`) and word (`C7 84`) writes, 16 use byte writes only,
20 use word writes only, and four `EMPTY0.BOB` records contain no immediate
pixel writes. No record requires a second pixel codec or an out-of-canvas
write in the safe model. This confirms the common planar blitter contract and
isolates the remaining runtime question to branch/ordering semantics inside
the executable's segment-3 entry, rather than another on-disk BOB format.

### Current frontend composition (provisional)

`engine/apps/quiky-play.cpp` currently composes a frame in this order:

```text
MAP tiles -> optional ARE/debug markers -> active BOB entities + player BOB
           -> transient ICO effects -> camera crop to rows 0..175
           -> opaque GAMEBAR.PCC rows 176..199 -> present
```

The BOB/ICO boundary now follows the measured stepped list: the player is
drawn in the BOB pass and the later ICO batch overwrites every covered pixel,
including indexed zero. Foreground MAP tiles and HUD layers still need their
own native stage evidence.

The engine now has an indexed PCX/PCC decoder (`PcxImage`) and a clipped
`blitIndexedSurface` primitive with explicit opaque versus transparent-zero
modes. `quiky-play` loads `GAMEBAR.PCC`, crops the world to the measured
320x176 viewport, and composites the opaque 320x24 bar into a 320x200 logical
frame before presentation. Palette application remains separate, matching the
native byte-identical GAMEBAR buffer evidence; the remaining palette
uncertainty is upload timing/page handoff, not the menu/title source.

The gameplay `LevelRuntime` now applies the measured VGA palette contract via
`Palette::parsePcxDac`: each PCX component is truncated to six bits (`>> 2`)
and expanded with the DOSBox/VGA `(c * 259 + 33) >> 6` lookup. Raw
`Palette::parsePcx` remains available for asset inspection, while the live
world path uses the DAC form. The synthetic format test covers both paths.

The frontend no longer derives player slots from a global frame modulo. The
engine-side `PlayerAnimation` cursor mirrors the native table interpreter:
idle/walk/rising/falling delays are `14/4/8/20`, direction switches keep the
same cursor while adding the paired `+50` slot offset, and an explicit death
state exposes the measured `20..28,-3` table as `20..28,26,27,28`. Native
`action_word=4` is not itself a death indicator: the running table also uses
that word, while the hazard path changes the animation table and state bytes.
Regression tests cover delay boundaries, mirror slots, and the negative-loop
expansion. Natural hazard/recovery ownership is intentionally still separate;
the current `LevelSession` reset path does not claim to reproduce the native
death hold yet.

The page-handoff boundary is now statically bounded even though its live menu
call is not yet captured. `0207:00B3` programs the VGA CRTC start address
through ports `3D4/3D5`; `0207:00E3` copies a selected PCC descriptor into the
current VGA page and does not itself flip the display. The `01D7:3020` setup
sequence calls the timer/VGA setup helpers, copies the selected PCC, and then
enters the palette ramp. An optional CRTC trace at startup
(`render-title-early-palette-page-start-v2.json`) observed three native
`00B3` calls at frames `133`, `169`, and `210`, all writing start address
`0x0000`; the following `02A5` ramp begins at frame `211` and the first full
`0536` upload follows at frame `213`. This proves that the early blacking
setup does not select a hidden nonzero page, but it does not yet correlate the
settled-menu frame with its later page write. The remaining page experiment
should arm the CRTC writer around the settled menu ramp and record the
start-address value alongside the stopped frame.

Static segment-3 disassembly narrows the MAP part of that question. The
`render_map_column` routine (`01F7:20C8`) and `render_map_strip`
(`01F7:2CB2`) are incremental VGA writers: the camera-update code calls one
for a vertical or horizontal scroll boundary, then copies the prepared strip
into the visible page. Both routines mask the MAP cell to the low nine-bit
tile ID before indexing the tile/descriptor data; the upper MAP bits are not a
second foreground image selector. The current evidence therefore supports a
single background tile plane plus separate entity/overlay passes. The exact
entity-vs-entity and entity-vs-HUD overwrite order remains a runtime pixel
experiment.

The same targeted decompile also bounds the renderer stage that follows MAP
preparation. `01F7:35C7` sets the VGA graphics controller read/write mode and
iterates the pending 16-byte draw descriptors. Standard descriptors call
`3450` for edge clipping and `36D5` for the planar copy; `36D5` computes the
source row stride as `(width + 3) >> 2` and copies only positive-width,
positive-height rectangles. A descriptor with slot `-2` takes the separate
`165B` path, while slot `-1` is rejected by the standard BOB path. The frame
then swaps the pending descriptor buffers and restores the graphics-controller
state. This explains why the observed BOB/ICO list is a post-MAP overlay stage,
but no separate ARE foreground bitmap writer is present in this loop. A native
ARE/debug marker capture is still needed before treating the frontend's
optional ARE overlay as faithful game rendering.

The complete normal ARE dispatch census (`render-dispatch-table-v1.json`) adds
negative evidence: all 47 registered types are ordinary object callbacks
except type `0x28`, whose class-0 entry points directly at the native exit
writer (`01F7:9256`). There is no registered foreground/second-tile-plane
class in the table. The remaining ARE experiment is therefore about proving
which entity callback is visible at a chosen coordinate, not discovering an
unlisted MAP foreground bitmap path.

A first live renderer-stage trace now supplies a provisional call sequence for
the W4L1 launch. Breakpoints were placed on the map writers, page-copy paths,
object-list stages, BOB entry/blit helpers, and ICO entry/call helpers. The
first hit of each target (therefore not one complete frame) was:

| frame | stage | address | camera x/y |
| ---: | --- | --- | ---: |
| 837 | MAP strip | `01F7:2CB2` | 863 / 262 |
| 838 | horizontal page copy | `01F7:2F71` | 863 / 262 |
| 853 | render object list | `01F7:3587` | 352 / 262 |
| 854 | BOB blit entry | `01F7:0013` | 352 / 262 |
| 855 | BOB object entry | `01F7:3529` | 352 / 262 |
| 857 | render frame | `01F7:35C7` | 352 / 262 |
| 894 | clipped-copy helper | `01F7:36D5` | 352 / 262 |
| 907 | ICO entry | `01F7:1186` | 352 / 262 |
| 908 | ICO tile call | `01F7:11B4` | 352 / 262 |

The trace also captured the object pointer prefixes at the BOB entry points.
Static disassembly ties `01F7:3529` to the `object+0x12` slot lookup and the
`01F7:0013` draw path, while `01F7:1186` dispatches the four short-lived ICO
objects. This confirms that map/page preparation precedes the first
object-list/BOB stages in this launch and that ICO work is reached later, but
it is not yet a pixel-accurate per-frame overwrite order. The reproducible
ledger is `research/build/traces/render-w4l1-renderer-order.json`.

The order trace now includes the native PIT wait entry at `0207:10AA`. In a
W1L1 launch it lands between the first BOB entry and `01F7:35C7` frame return
(frames 862, 863, and 865 respectively), while the preceding MAP strip and
page-copy stages remain ordered at frames 842-846. Targeted segment-4
disassembly closes the helper's control flow: it latches PIT channel 0 by
writing `0` to port `0x43`, reads the low/high counter bytes from `0x40`,
compares the value against `DS:97F4`, and, only when two consecutive reads
match, programs mode `0x34` with divisor `0x03e8`; it then stores the latest
counter and loops until it changes. The debugger boundary confirms the wait is
on the normal render path but cannot expose the unperturbed hardware counter
cadence itself. The reproducible ledger is
`render-w1l1-order-timer-v1.json`.

The MAP flag probe removes the remaining ambiguity about the cell's upper
bits. In paired W1L1 runs, the first horizontal strip reads the same source
cell at the loaded MAP selector `0377`, offset `0x21fe`, with value `0x0121`.
The patched run changes only bits 9-15 to produce `0xff21`, while the low
nine-bit tile remains `0x121`. The native `2d56` cell read sees the patched
word, and the following `2d69` tile-selection registers remain
`GS=0277`, `BX=0x0484`, matching the unpatched run. The first page-copy stage
still follows the strip in both runs, and the object-list/BOB/frame stages
retain their order. This is direct runtime evidence that those upper cell
bits do not select a second foreground bitmap in the MAP writer; they are
metadata consumed outside this tile image path. The paired ledgers are
`research/build/traces/render-w1l1-map-baseline-v1.json` and
`research/build/traces/render-w1l1-map-flags-v1.json`.

The repeated stage census separates stable object identity from the first-hit
sequence. W1L1 repeatedly enters BOB slot `703` at `(768,224)` and W4L1
repeatedly enters slot `208` at `(1200,144)`; the W4 ICO-focused run repeatedly
enters an object with slot `0xFFFF` at `(512,304)` and the same animation-data
pointer. These are useful family-specific draw probes; the original one-shot
census still reports the same visible object on successive frames rather than
walking every object in one list pass. The stepped ledger below supplies that
list walk. The earlier ledgers are `render-w1l1-renderer-census.json`,
`render-w4l1-renderer-census.json`, and `render-w4l1-ico-census.json`.

A stepped list pass closes that limitation for the common BOB draw call. The
trace breaks at `01F7:0016`, after the BOB routine's stack frame is established,
decodes its far return, and resumes at the caller's next instruction before
arming the next call. In the first W4L1 pass, all four active list entries are
observed in one render-list traversal, with the following argument order:

```text
frame 850 (run-relative): slot 208, world (1200,144)
frame 853: slot 607, world (1153,366)
frame 855: slot 214, world (704,400)
frame 858: slot   0, world (512,400)
```

The BOB routine returns to `01F7:35B8` for each entry, confirming that this is
the `3587` object-list loop rather than repeated hits on one object. Later
passes retain the same list order (for example `214` then `0`), with inactive or
off-screen entries skipped by the loop's flag/position tests. The same trace
reaches the separate ICO batch only after the BOB calls; `01F7:1186` emits its
four `11B4` helper calls and returns at `11B0`. The synchronized pixel ledger
now captures each helper return as a separate frame, so the call ordering and
per-tile write boundaries are directly observable. The controlled W1L1 overlap
fixture plus the `11B4` decompile closes the cross-family final-color winner.
The reproducible ledger is `render-w4l1-walk-call42.json`.

For BOB mode coverage, the existing native initializer captures add the
descriptor-side family split: W1 type `0x01` selects slot `281`, type `0x03`
selects slot `276`, and the type `0x3F` platform path selects slot `301`.
All three use the common segment-3 entry and declared origin/size fields; the
   live traces do not yet expose a distinct opaque/mirrored branch at the pixel
   write level. The compact source ledgers remain in the main-tree build under
   `entity-01-evidence`, `entity-03-evidence`, and `entity-3f-evidence`.

The segment-3 BOB callee itself now resolves the mode bit that matters for
the VGA write policy. Its caller-supplied mode word is at `BP+6`; `mode=0` is
the normal transparent/planar path seen in the live W4L1 list (`slots 208,
607, 214, 0`). When bit `0x0100` is set, the callee writes `0xFF` to both VGA
map-mask registers before its immediate pixel loop and restores zero masks on
return. The runtime call census records the list's mode as zero, while this
static branch identifies the executable's opaque/all-plane capability. The
remaining experiment was made reproducible with
`quiky_renderer_pixel_trace.py --force-bob-mode 0x100`. In the W4L1 run
`render-w4l1-bob-mode100-v1.json`, the first two stopped BOB frames (slots 208
and 214) were pixel-identical to the unforced trace, despite the guest stack
mode changing from `0` to `0x100`. Later frames diverged because the debugger
patch changes the renderer's timing/list progression, not because a stable
same-frame pixel difference was established. Therefore the indexed engine
should retain one decoded transparent pixel contract while keeping the native
all-plane branch as an execution-mode detail until a same-object paired
capture is available.

A same-slot paired probe now reaches that boundary. Slot `700` (`BLATT.BOB`,
14x12, origin 7,12) was forced to the same draw coordinates `(272,340)` in
both runs. The normal `mode=0` stopped frame retains the indexed gameplay
page, while the forced `mode=0x100` frame is entirely index zero in the raw
320x200 capture. This is a real pixel divergence for the same descriptor and
arguments, consistent with the all-plane branch writing indexed zero as well
as nonzero source pixels. Because the mode was debugger-injected and no
natural call has yet supplied `0x0100`, keep the branch marked experimental
until an in-game caller or a VGA-register-level replay confirms whether the
whole-page blank is an emulator-side artifact or the intended opaque mode.
The paired ledgers are `render-w1l1-bob-slot700-y340-mode0-v3.json` and
`render-w1l1-bob-slot700-y340-mode100-v3.json`.

Static caller analysis narrows the mode branch further. `01F7:35B3` is the
only relocated call to `01F7:0013`. The BOB-list entries consumed there live at
`DS:6D86` and are built by `01F7:34BC`, which writes entry `+6/+7` as
`0`/`CL`. `01F7:1024` is the only direct caller of this list builder and copies
`CL` from the special-object record at `+0x16`. A rearmed W4L1 trace captures
76 consecutive special-list builds across the live render loop; every record
has source byte `+0x16 = 0` and `CL = 0`, matching the ordinary W1/W4 mode-zero
snapshots. No natural caller of the known list path supplies `0x0100`; retain
the branch for compatibility, but do not make it the default indexed-engine
policy.

The player direction probe closes the native mirror ownership rule. With the
same W1L1 startup and six-frame left-input holds, the player callback reports
slots `52, 54, 56, 51, 53, 55, 50` as x moves from 128 down to the wall at 72;
the action word remains zero. `QUIKYW1.BOB` record pairs `(0,1)`, `(2,3)`, …,
`(14,15)` have identical nonzero masks under horizontal reflection, and the
runtime slot map resolves those records as `(0,50)`, `(1,51)`, …, `(7,57)`.
The right-input sample retains slot 0 until its later step to slot 2, matching
the even-record family. This proves mirroring is represented by distinct BOB
records/slots, not a renderer-time horizontal flip. The left ledger is
`research/build/traces/player-w1l1-cadence-left-v1.json`; the pairwise asset
check is in `research/build/traces/player-pairs/`.

The live player cadence probe adds one normal-control sample: with six-frame
right-input holds and twelve-frame callback samples, W1L1 advances the player
from x=128 to x=233 at y=400; the renderer-facing player slot remains `0`
through x=196, then becomes slot `2` at x=208 and stays there through x=233.
The callback's action word remains `0` throughout. This is useful timing and
slot evidence, but it is not a complete direction/action table: no left,
jump, mirror, or animation-transition branch was exercised. The ledger is
`research/build/traces/player-w1l1-cadence-v1.json`.

The follow-up table probe resolves the normal walking cadence rather than just
sampling rendered slots. Runtime DS table `0x3142` contains delay `4`, frame
sequence `0,1,2,3,4,5,6,7`, and a negative loop marker; `01F7:5D38` installs
that table's current pointer and adds `0x32` to every frame when the player
direction byte is `0xff`. Consequently the measured families are
`0..7` (right) and `50..57` (left), with a four-update delay. The W1L1
left run emits `51,53,54,55,57,50` after the initial idle frame, while the
right run emits the corresponding even-family records; paired asset masks
confirm these are pre-baked horizontal mirrors. The idle table at `0x316a`
has delay `14` and sequence `0,16,17,18,18,19,19,19,18,17,16` before its
negative loop marker. The raw table snapshots and object animation pointers
are in `player-w1l1-tables-left-v2.json`, `player-w1l1-tables-right-v1.json`,
and `player-w1l1-idle-tables-v2.json`. The jump probe now identifies both
normal airborne tables: runtime `0x3160` has delay `8` and frames `10,11,12`
before its negative loop marker, while `0x3186` has delay `20` and frames
`13,14,15`. A W1L1 up-input run observes slots `11,12,13`, then returns to
the idle family on landing; `+0x3e` and byte `+0x37` distinguish the airborne
phases. The ledger is `player-w1l1-jump-tables-up-v1.json`.

The long natural hazard run also captures the death table directly. The hazard
path switches to runtime table `0x31a4`, delay `14`, frames `20..28`, with
negative marker `-3` looping terminal frames `26,27,28`; the object still
reports action word `4`, which is also the ordinary running action before the
hazard. Native W1L1 slots `28` and `26` at the hazard boundary are therefore
table-driven death animation, followed by checkpoint reset to the normal idle
table/action state; this extends the earlier death/recovery evidence without
debugger-seeding player state. The table ledger is
`player-w1l1-death-tables-v1.json` and the recovery checkpoint remains
`player-w1l1-death-recovery-next.json`.

A lightweight high-rate callback trace fills in the cadence that the full pool
probe could not sample. `player-w1l1-death-timeline-light-v3.json` records the
native callback every three guest frames: after the table switch, the death
object remains on action `4` from the first observed death sample at frame
`1583` through frame `1810`, then returns to action `0` at frame `1813` (about
230 guest frames in that natural run). A denser entry/recovery run,
`player-w1l1-death-entry-recovery-light-v2.json`, captures the table switch
itself: slots `20,21,22,23,24,25` begin at a fixed `(2131,368)` before the
terminal loop, and the checkpoint returns to idle at frame `1888` before the
held-right input re-enters the running table. The differing absolute frame
numbers reflect debugger overhead and input sampling, so the stable contract
is the state/table boundary rather than a hard-coded wall-clock frame. No DAC
helper appears in the corresponding held-death palette probe, so the visual
death state is an object-animation transition rather than a palette fade.
Exact death-during-another-transition behavior remains open.

Targeted segment-3 disassembly now explains how those table bytes become the
rendered slot. `01F7:5D38` installs a table's delay and frame cursor, selects
the first slot, and adds `0x32` for the pre-baked mirrored records when the
direction byte is `0xff`. `01F7:5D60` decrements the per-object delay, advances
the cursor by one word when it expires, and interprets a negative table word as
a backwards jump (`-3` therefore loops the final `26,27,28` trio). The player
callback invokes this updater in its common render path at `01F7:438D`; the
hazard branch enters the death table at `01F7:4376`/`41DD` and the recovery
branch returns through the normal table setup. This closes the animation-table
interpreter and leaves only the exact hazard-to-recovery trigger timing and
transition-overlap case for further runtime work. The reproducible static
outputs are `ne-target-player-animation-5c80-5e80.txt` and
`ne-target-player-callback-4240-4610.txt`.

### Animated ICO timing and draw selection

The live state-machine traces now resolve the timing and lookup boundary for
the two animated-ICO families:

- `0x1F`-`0x21` share the `01F7:8E4B` callback. Its state byte advances once
  per update and emits at states `4`, `6`, `8`, and `10`; each emission queries
  five MAP cells in the ordered strip described in the static notes. A W1L1
  camera-centered run returned MAP tiles `201, 200, 202, 203, 204`, mapped to
  ICO states `121, 120, 122, 123, 124`, at positions `(32,272)`,
  `(16,272)`, `(48,272)`, `(64,272)`, and `(80,272)`. The resulting objects
  have update callback `01F7:10B5`, no BOB slot (`object+0x12 = 0xFFFF`), and
  lifetime `3`. The `01F7:1186` lookup for state `121` selects a 256-byte
  block at offset `0x7900` (`121 * 0x100`) that matches raw tile 121 in
  `W1.ICO`; equivalent block matches cover W2 states `126`-`130`, W3
  `400`-`404`, W4 `240`-`244`, and W5 `61`-`64`.
- Dedicated event types `0x65`-`0x67` enter the event ring through
  `01F7:1749`. The event-loop animation byte is incremented modulo eight,
  combined with the subtype and `0x1D6`, and passed to `01F7:16CE` as the
  object's animation state. The resulting object again uses `01F7:10B5` and
  no standard BOB slot. Visible W1L1 traces reach the four-call ICO path
  through `01F7:1186`/`01F7:11B4` and match `LOOP_W1.ICO`; independent first-
  entity probes match `LOOP_W2.ICO` record 8, `LOOP_W3.ICO` record 6,
  `LOOP_W4.ICO` record 22, and `LOOP_W5.ICO` record 4. This establishes the
   world-relative resource family and the modulo-eight event cursor. The
   synchronized per-helper frames resolve the four-call pixel order; the
   archive-relocated native overlap plus the `11B4` decompile close the
   remaining cross-family zero-write question.

The removal edge is now measured for all three dedicated variants. With the
native `10B5` update callback sampled before its decrement, types `0x65`,
`0x66`, and `0x67` each report lifetimes `3 -> 2 -> 1`; a stop at `10C1`
immediately after the final decrement reads `object+0x18 = 0` and lifetime
`0`. The compact ledgers are
`research/build/traces/entity-65-lifetime-worktree-v1.json`,
`entity-66-lifetime-worktree-v1.json`, and
`entity-67-lifetime-worktree-v2.json`. Later event objects reuse the same
pool offsets only after this inactive state, so the dedicated ICO lifetime
contract is no longer an open question.

The frontend now matches that removal edge: `LevelSession::spawnTransientEffect`
uses lifetime `3`, so one streamed event is drawable at its initial state and
for two subsequent updates before removal. The existing format/session test
asserts animation frames `1`, `2`, then empty. The remaining implementation gap
is the per-world LOOP cursor mapping, not lifetime ownership.

The compact source ledgers for these observations are in the main-worktree
build requested for this branch (`/home/joao/dev/quicky/research/build`),
not regenerated in the branch: `entity-20-state-emission-v6.json`,
`entity-1f-animation-v11.json`, `entity-65-dedicated-animation-v9.json`,
`entity-67-dedicated-animation-v8.json`, and the cross-world
`traces/dedicated-animation-w{3,4,5}-v1.json` plus
`traces/entity-66-dedicated-animation-w2l1-first-v1.json`.

## Open rendering questions and experiments

1. **Layer ordering and overlap.** The MAP routines ignore upper cell bits for
   image selection; the paired live flag probe confirms that changing bits
   9-15 leaves the selected tile pointer unchanged. The stepped W4L1 trace plus synchronized ledger
   establish the BOB list order and its precedence over the later ICO batch.
   The W1L1 state-machine fixture now instantiates a BOB and ICO at the same
   renderer coordinates and measures BOB-return then ICO-tile order. The
   archive-relocated native overlap run also exercises this without a debugger
   coordinate rewrite.
   HUD-vs-entity clipping is closed: the forced slot-700 BOB straddle reaches
   screen rows 173-175, then stops at the opaque row-176 separator, with no
   BOB pixels in GAMEBAR rows 177-199. Keep this as a viewport-clip contract,
   independent of the cross-family ICO zero-write test. A separate ARE/debug
   or true foreground-overlay writer has not yet been observed, so that
   terminology should not be implemented as a second MAP image plane without
   a new native hit.
2. **ICO pixel ordering and lifetime edge cases.** The four helper boundaries,
   localized incremental PNG differences, and the natural `3 -> 2 -> 1 -> 0`
   lifetime/removal edge for dedicated events are resolved. The controlled
   BOB/ICO fixture instantiates both records, and the `11B4` decompile closes
   the remaining pixel question: ICO writes indexed zero, so it clears any
   earlier BOB/MAP pixel at a covered coordinate.
3. **BOB draw modes.** Initializer and census traces now cover ordinary W1
   enemy slots `281`/`276`, platform slot `301`, and live slots `703`/`208`.
   A same-slot forced `0x0100` probe produces an all-zero raw page versus the
   normal indexed page, proving the branch is pixel-visible but leaving natural
   caller ownership and the apparent whole-page blank to settle. Static caller
   analysis shows normal list construction supplies mode zero, and the player
   direction probe proves mirror slots are separate BOB records rather than a
   renderer flip. Runtime table probes now cover idle, walking, jump, and
   natural death action families; remaining player work is wiring those tables
   and checking left-side airborne/death variants. Any nonstandard caller that
   can reach the opaque branch remains open. The safe decoder still proves
   only the common immediate-write path, not every possible blitter mode.
4. **Palette writes.** The W4L1 and W1L1 launch traces both resolve a
   descending single-entry black fade (`255` downwards) through `0207:05FF`;
   an attach-before-launch probe catches entry 255 at frame 96. The exit
   ledger now re-arms `0207:0536` and measures 20 complete DAC uploads fading
   to exact zero, followed by a black interval and a next-level ramp. The
   live-menu probe sees no `0536` or `05ff` helper through frame 606. A
   menu-Enter probe measures `01D7:1473 -> 0207:022A` (`flag=1`,
   `fade_step=20`), the `DS:5C9C` zero-fill, and the internal `02A5` ramp.
The settled menu checkpoint now supplies the missing source: descriptor 7
(`MENU.PCC`) and an exact 6-bit-converted `DS:6118`/`DS:60E4` table. Only
the exact settled-menu upload/frame correlation and page handoff remain;
the source owner is no longer open.

The stable-menu negative check is now explicit as well. In
`render-transitions-menu-nav-mask1-page-v1.json`, the `menu-ready` checkpoint
is frame 352; an eight-iteration palette/CRTC window runs through frame 539
without a `0536`, `05FF`, `0E21`, or `00B3` hit. The selector mask experiment
then reaches its normal `0470`/`0047` label path at frame 540. Thus the menu
does not continuously upload its palette or rewrite the CRTC while idle; the
remaining question is the one-time upload/page handoff during the earlier
setup path, not a per-frame menu blink palette owner.
   The current
   frontend only converts a
   copied PCC palette to RGB and has no transition fade/cycle implementation.
5. **HUD and menus.** The menu background/font composition, 320x200 logical
   geometry, selector input wait (`01D7:4ACE`), selector dispatch
   (`01D7:4B18`), PCC copy helper (`01D7:3020`), and first level lookup are now
   checkpointed or statically bounded. The live probe explicitly armed
   `01D7:313D` and `01D7:3020` around the selector path; both were skipped by
   timeout. The dedicated `01D7:14E1` gameplay-HUD probe also skipped, and
   static callers place it on completion/exit rather than the ordinary frame
   loop. The native reload trace now captures `3FB0 -> 4486 -> 0B6C -> 0BEF`
   and the second post-cleanup renderer page, confirming the GAMEBAR handoff
   and rows 176-199 geometry. The forced slot-700 straddle closes the
   HUD-specific experiment; remaining work is the foreground/background MAP
   stage contract, UI palette upload timing, and wiring the measured HUD
   surface into the frontend.
6. **Transitions.** Selector launch now has a frame/resource checkpoint
   sequence and a blacking interval, and W1L1 death/recovery is confirmed at
   the gameplay-object level. Static code identifies the seven-bit goal mask,
   the `BONUS-LEVEL!` branch, and three exit-flag writers. The native
   type-`0x28` fixture reaches the live `92a9` writer and the
   `4cfc`/`4d03`/`4ea0`/`4f0d`/`1709` chain. A real Space key then crosses both
   post-`1709` input waits and reaches `0caa`/`022a`, full-palette uploads,
   reset/reload (`5010`/`504f`), and a new gameplay renderer; no selector/menu
   dispatch occurs immediately afterward. The corrected `18C7` lookup decode
   now closes the resource-routing identity (`W1L2` for goal mask `0`, `W1L4`
   for `0x7f`). Remaining transition work is correlating the measured DAC ramp
   to exact visible frames and VGA page handoff; the exit gate, input-lock
   path, 20-step ramp count, and next-level choice are closed.


## Reproducibility

Static evidence and existing debugger commands are documented in
[`ghidra-analysis.md`](ghidra-analysis.md), [`cheat-trace.md`](cheat-trace.md),
and [`research/README.md`](../README.md). When the debugger-enabled
dosbox-automation build is available, start with:

```sh
python3 research/tools/quikytrace.py --launch --headless \
  --select-level W4L1 --tail-count 8 \
  --output research/build/traces/render-w4l1-resources.json
```

The checked-in C++ tests exercise the asset-level invariants that can be
verified without a live debugger: ICO deinterleave, explicit masked-layer
drawing, native opaque ICO zero writes, surface clipping, BOB origin
subtraction, and world-relative LOOP slot selection. Runtime claims above
remain labeled as runtime/static evidence and are not inferred from those unit
tests.

## First live resource capture

Using the existing debugger build from the main worktree, with
`DOSBOX_AUTOMATION_BUILD_DIR=/home/joao/dev/quicky/research/build/dosbox-automation-debug`
and `DOSBOX_AUTOMATION_DEPS_PREFIX=/home/joao/dev/quicky/research/build/sdl-prefix`,
the selector trace completed successfully in headless mode:

```sh
timeout 90s env \
  DOSBOX_AUTOMATION_BUILD_DIR=/home/joao/dev/quicky/research/build/dosbox-automation-debug \
  DOSBOX_AUTOMATION_DEPS_PREFIX=/home/joao/dev/quicky/research/build/sdl-prefix \
  SDL_VIDEODRIVER=dummy SDL_AUDIODRIVER=dummy \
  python3 research/tools/quikytrace.py --launch --headless \
    --select-level W4L1 --tail-count 8 \
    --output research/build/traces/render-w4l1-resources.json
```

The run produced ten ordered lookup events and confirmed this W4L1 batch:

| sequence | resource | size |
| ---: | --- | ---: |
| 1 | `W4L1.MAP` | 25,490 |
| 2 | `W4L1.ARE` | 6,552 |
| 3 | `W4.ICO` | 122,880 |
| 4 | `W4ANI.ICO` | 3,840 |
| 5 | `LOOP_W4.ICO` | 6,912 |
| 6 | `QUIKYW1.BOB` | 196,056 |
| 7 | `PUZZLE.BOB` | 6,545 |
| 8 | `PUFF.BOB` | 6,962 |
| 9 | `SPATZ.BOB` | 2,238 |
| 10 | `PLATFW4.BOB` | 3,878 |

This is direct runtime evidence that animated ICO resources are loaded as a
separate `W4ANI.ICO` member and that `LOOP_W4.ICO` is loaded in the same level
context. It also supplies the next target for a draw-call trace: stop after
the `W4ANI.ICO` lookup and correlate its first selected 256-byte block with
the renderer's animation state and visible frame; the stage trace above now
provides the complementary renderer-entry sequence.

## Rendering follow-up trace commands

All live traces in this worktree reuse the debugger-enabled build and SDL
dependency prefix from the main tree:

```sh
export DOSBOX_AUTOMATION_BUILD_DIR=/home/joao/dev/quicky/research/build/dosbox-automation-debug
export DOSBOX_AUTOMATION_DEPS_PREFIX=/home/joao/dev/quicky/research/build/sdl-prefix
export SDL_VIDEODRIVER=dummy SDL_AUDIODRIVER=dummy
```

The early attach, renderer-stage, and transition ledgers can be regenerated
with:

```sh
python3 research/tools/quikyearlypalette.py --timeout 30 --events 256 \
  --output research/build/traces/render-early-palette.json
python3 research/tools/quikyearlypalette.py --timeout 20 --events 1000 \
  --capture-page-start \
  --output research/build/traces/render-title-early-palette-page-start-v2.json
python3 research/tools/quiky_renderer_trace.py --level W4L1 --events 24 \
  --timeout 4 --output research/build/traces/render-w4l1-renderer-order.json
python3 research/tools/quiky_renderer_census.py --level W4L1 --focus ico \
  --events 24 --timeout 10 \
  --output research/build/traces/render-w4l1-ico-census.json
python3 research/tools/quiky_renderer_census.py --level W4L1 --focus bob \
  --walk-objects --events 42 --timeout 8 \
  --output research/build/traces/render-w4l1-walk-call42.json
python3 research/tools/quikytrace.py --launch --headless --select-level W1L1 \
  --player-trace --player-focus-callback --player-callback-offset 0x3ff8 \
  --player-samples 8 --player-frames-between 60 \
  --player-input-key KBD_right --player-input-frames 1600 \
  --player-input-samples 1 --timeout 150 \
  --output research/build/traces/player-w1l1-death-recovery-next.json
python3 research/tools/quikytrace.py --launch --headless --select-level W1L1 \
  --player-trace --player-focus-callback --player-callback-offset 0x3ff8 \
  --player-samples 80 --player-frames-between 3 --player-lightweight \
  --player-input-key KBD_right --player-input-frames 1600 \
  --player-input-samples 1 --timeout 150 \
  --output research/build/traces/player-w1l1-death-timeline-light-v3.json
python3 research/tools/quiky_transition_trace.py --timeout 15 \
  --output research/build/traces/render-transitions.json
python3 research/tools/quiky_transition_trace.py --timeout 30 \
  --output research/build/traces/render-transitions-escape-real.json
python3 research/tools/quiky_transition_trace.py --timeout 8 \
  --output research/build/traces/render-transitions-ui-hud-v8.json
python3 research/tools/quiky_transition_trace.py --timeout 3 \
  --menu-nav-mask 1 --menu-nav-only --menu-font-limit 0 \
  --output research/build/traces/render-transitions-menu-nav-mask1-page-v1.json
python3 research/tools/quiky_transition_trace.py --timeout 8 \
  --output research/build/traces/render-transitions-state-path-v11.json
python3 research/tools/ne_target_decompile.py game/QUIKY.EXE \
  --segment 1 --start 0x45b1 --end 0x5130 \
  > research/build/traces/ne-target-decompile.txt
python3 research/tools/ne_target_decompile.py game/QUIKY.EXE \
  --segment 3 --start 0x2cb2 --end 0x37f0 \
  >> research/build/traces/ne-target-decompile.txt
python3 research/tools/quikypalette.py --level W1L1 --samples 8 \
  --post-input-key KBD_right --post-input-frames 1600 --post-samples 8 \
  --timeout 30 --output research/build/traces/render-w1l1-palette-death-v2.json
python3 research/tools/quiky_renderer_pixel_trace.py --level W4L1 \
  --focus both --events 24 --timeout 8 \
  --output research/build/traces/render-w4l1-pixel-trace-tiles24.json
python3 research/tools/quiky_renderer_pixel_trace.py --level W1L1 \
  --runtime-dir research/build/traces/render-w1l1-overlap-bob-ico/overlap/game \
  --focus both --events 20 --timeout 8 \
  --output research/build/traces/render-w1l1-overlap-pixel-trace-both20.json
python3 research/tools/quiky_renderer_pixel_trace.py --level W1L1 \
  --runtime-dir research/build/traces/render-w1l1-overlap-ico-bob/overlap/game \
  --focus both --events 24 --timeout 8 --ack-delay-frames 8 \
  --patch-map-run --patch-map-x 784 --patch-map-y 192 \
  --patch-camera-x 768 --patch-camera-y 192 \
  --bob-draw-slot 226 --bob-draw-x 768 --bob-draw-y 240 \
  --output research/build/traces/render-w1l1-overlap-ico-bob/pixel-same-draw-ack8.json
python3 research/tools/quiky_goal_fixture.py game/NESTLE.DAT \
  research/build/traces/render-w1l1-goal-letter-ground \
  --level W1L1.ARE --record-offset 0x1782 \
  --target-x 768 --target-y 400 --stream-cell 12,3 --overwrite
python3 research/tools/quikytrace.py --launch --headless \
  --runtime-dir research/build/traces/render-w1l1-goal-letter-ground/goal/game \
  --player-trace --player-focus-callback --player-collision-focus \
  --select-level W1L1 --player-samples 8 --player-frames-between 30 \
  --player-force-position-x 768 --player-force-position-y 400 \
  --output research/build/traces/render-w1l1-goal-letter-ground/player.json
python3 research/tools/quiky_goal_all_fixture.py game/NESTLE.DAT \
  research/build/traces/render-w1l1-goal-all-start \
  --target-x 128 --target-y 400 --stream-cell 2,6 --overwrite
python3 research/tools/quikytrace.py --launch --headless \
  --runtime-dir research/build/traces/render-w1l1-goal-all-start/goal/game \
  --select-level W1L1 --player-trace \
  --player-object-callback-offset 0x8d20 \
  --player-object-callback-slot 600 \
  --player-samples 12 --player-frames-between 5 --timeout 6 \
  --output research/build/traces/render-w1l1-goal-all-start/goal-object-slot600.json
python3 research/tools/quiky_goal_all_fixture.py game/NESTLE.DAT \
  research/build/traces/render-w1l1-goal-all-valid \
  --target-x 128 --target-y 390 --stream-cell 2,6 --overwrite
python3 research/tools/quikytrace.py --launch --headless \
  --runtime-dir research/build/traces/render-w1l1-goal-all-valid/goal/game \
  --select-level W1L1 --player-trace \
  --player-object-callback-offset 0x8e29 \
  --player-samples 1 --player-frames-between 5 --timeout 6 \
  --output research/build/traces/render-w1l1-goal-all-valid/goal-branch-8e29-any.json
python3 research/tools/quikytrace.py --launch --headless \
  --runtime-dir research/build/traces/render-w1l1-goal-all-valid/goal/game \
  --select-level W1L1 --player-trace \
  --player-object-callback-offset 0x8d36 \
  --player-object-callback-slot 600 \
  --player-samples 4 --player-frames-between 5 --timeout 6 \
  --output research/build/traces/render-w1l1-goal-all-valid/goal-branch-8d36.json
python3 research/tools/quiky_goal_transition_trace.py --timeout 5 \
  --output research/build/traces/render-w1l1-goal-transition-mask7f.json
python3 research/tools/quiky_goal_transition_trace.py --timeout 5 \
  --output research/build/traces/render-w1l1-goal-transition-state-path-v2.json
python3 research/tools/quiky_goal_transition_trace.py --deep --timeout 5 \
  --output research/build/traces/render-w1l1-goal-transition-deep-v13.json
python3 research/tools/quiky_goal_transition_trace.py --native-goal \
  --runtime-dir research/build/traces/render-w1l1-goal-all-valid/goal/game \
  --force-player-x 128 --force-player-y 390 --timeout 10 \
  --output research/build/traces/render-w1l1-goal-all-valid/native-goal-transition-v1.json
python3 research/tools/quiky_exit_focus_trace.py \
  --runtime-dir research/build/traces/render-w1l1-goal-all-exit-x200/goal/game \
  --timeout 20 --events 32 --hold-frames 2000 --settle-frames 0 \
  --force-player-x 128 --force-player-y 390 --pretrigger-frames 180 \
  --post-force-player-x 200 --post-force-player-y 390 \
  --capture-frames --capture-mode raw \
  --capture-names pcc-blit-return-0bef,post-cleanup-render \
  --post-transition-key KBD_space --skip-palette-loop \
  --output research/build/traces/render-w1l1-goal-all-exit-x200/exit-focus-gamebar-v6.json
python3 research/tools/quiky_renderer_pixel_trace.py --level W1L1 \
  --focus bob --events 6 --timeout 8 --bob-draw-slot 700 \
  --bob-draw-x 272 --bob-draw-y 444 --screenshot-mode raw \
  --output research/build/traces/render-w1l1-hud-straddle-y444-slot700.json
```

The overlap fixture is generated from the shared W1L1 reference with:

```sh
python3 research/tools/quiky_overlap.py game/NESTLE.DAT \
  research/build/traces/render-w1l1-overlap-bob-ico \
  --level W1L1.ARE --first-offset 0x163e --second-offset 0x1644 \
  --stream-cell 12,3 --overwrite
python3 research/tools/quiky_overlap.py game/NESTLE.DAT \
  research/build/traces/render-w1l1-overlap-bob-bob4 \
  --level W1L1.ARE --first-offset 0x163e --second-offset 0x1644 \
  --stream-cell 12,3 --second-type 4 --overwrite
python3 research/tools/quiky_overlap.py game/NESTLE.DAT \
  research/build/traces/render-w1l1-overlap-ico-bob \
  --level W1L1.ARE --first-offset 0x1894 --second-offset 0x189a \
  --target-x 784 --target-y 192 --stream-cell 12,3 \
  --second-type 4 --overwrite
python3 research/tools/quiky_overlap.py game/NESTLE.DAT \
  research/build/traces/render-w1l1-overlap-natural-bob-ico \
  --level W1L1.ARE --first-offset 0x1894 --second-offset 0x189a \
  --first-target-x 768 --first-target-y 240 \
  --second-target-x 768 --second-target-y 224 \
  --stream-cell 12,3 --second-type 4 --overwrite
python3 research/tools/quiky_renderer_pixel_trace.py --level W1L1 \
  --runtime-dir research/build/traces/render-w1l1-overlap-natural-bob-ico/overlap/game \
  --focus both --events 16 --timeout 8 --patch-map-run \
  --patch-map-x 784 --patch-map-y 192 --patch-camera-x 768 \
  --patch-camera-y 192 --ack-delay-frames 8 --screenshot-mode raw \
  --output research/build/traces/render-w1l1-overlap-natural-bob-ico/pixel-natural-v2.json
```

The trace wrappers record executable/archive hashes and debugger metadata; the
PNG checkpoint frames and JSON ledgers under `research/build/` are generated
artifacts and remain ignored by Git.

The focused native post-input closure trace
`research/build/traces/player-w1l1-goal-cloud-current/native-cloud-post-input-v23.json`
observes the reload order directly: `5010 -> 5017 -> 0908 -> 0931
-> 0207:18C7 -> 5038 -> 0227:0D5A -> 503D -> 1AAA -> 5D38 -> 5042
-> 321F -> 5047 -> 313D -> 504F -> 35C7`. The lookup argument is the
Pascal path `GAMEDATA\\W1L4.map`; the trace records the published
`DS:97E8=952944`, `DS:97E4=957333`, and `DS:97EC=4389` values. `0D5A` is
armed only after `5038`, because the same helper also occurs in the preceding
fade path. The player snapshot is valid before `5010`, becomes unavailable
while `1AAA` tears down/reinitializes the pool identity, is populated by
`5D38`, and is present again at the first post-cleanup `35C7` render. This
closes the gameplay-affecting reload order for the focused fixture; the
remaining unclassified calls in the export are presentation/resource helpers
with no demonstrated player-state feedback.
