# Research consolidation checkpoint

This checkpoint reunifies the parallel descriptor, object lifecycle, entity
family, game-state, boss, rendering, player, and audio work on one branch.
The integration base is main commit `191a14d`; the source worktrees are merged
as explicit parents so no source commit is orphaned.

## Preserved durable work

- Runtime descriptor construction notes, evidence JSON, static reports, and
  selector-safe trace modes.
- Object scheduler, linked-child teardown, render-owner, frame-comparison, and
  high-effect evidence and tools.
- Entity family, moving-platform, pickup reconstruction, WOLKE, puzzle,
  effect-table, and authored boss-route evidence.
- Actual game-state, ending, checkpoint, score-file, and high-score probes.
- Cross-world boss constructors, callbacks, damage/phase matrices, lifecycle
  probes, and targeted decompilation tools.
- Rendering order, pixel, palette, transition, HUD, PCX, player-animation, and
  fidelity-audit work.
- Main's full 0x78-byte player callback capture, ordered frame reporting,
  signed fixed-point audit, audio mixer, and SFX implementation.

Every tracked path present on a source branch is present on the integration
branch. All known Ghidra symbols from the branch-local `AnnotateQuiky.java`
variants are represented by the integrated annotation script; specialized
boss, disassembly, reference, and call-form tools are retained separately.

## Shared tracer reconciliation

`quiky_player_trace.lua`, `quikytrace.py`, and `test_quikytrace.py` use main's
newer full-record player tracer as the base, with descriptor-construction and
object-emitter options added. Branch-local experiments remain reproducible
from their dedicated scripts and merged history.

The following older branch-only CLI option families were deliberately not
copied wholesale into the canonical player CLI because doing so would restore
four divergent copies of player state, breakpoint ownership, and input
scheduling:

- entity branch: boss-stage, effect-table, player-alignment, and secondary
  pulse controls;
- actual-state branch: checkpoint, goal, menu, high-score, state-event, and
  seed controls;
- boss branch: generic object patch/watch, factory, death-bypass, teleport,
  callback, and MAP patch controls;
- rendering branch: lightweight callback and forced position/camera controls.

The conclusions and dedicated probe scripts that used those options are
retained. Before another broad player-tracer extension, migrate only the
still-needed controls into focused subsystem scripts or a versioned common
trace schema. The merged source commits remain available for exact option
recovery.

The first Lua consolidation pass now keeps that migration small:

- `quiky_trace_common.lua` owns byte decoding, signed conversion, state
  differences, selector-safe reads, return validation, and breakpoint-owner
  bookkeeping;
- `quiky_patch_watch.lua` owns callback-scoped reversible memory mutations and
  records original/replacement bytes plus restoration status;
- `quikytrace.py` composes those two modules with the player probe and parses
  repeatable `--player-patch` declarations such as
  `player:0x3e:u16=0` or `selector:0x27f:0x3a:u8=0xff`.
- Repeatable `--player-input-phase` declarations now cover the reusable
  multi-key sequencing recovered in the boss/entity worktrees without
  reintroducing their tracer forks. For example,
  `KBD_right+KBD_up:12`, `WAIT:3`, and `KBD_left:8` describe three successive
  post-baseline sample windows.

The player trace schema remains version 1. Existing focused options and output
fields are unchanged; mutation ledgers are additive and appear only when a
declarative patch is requested.

The hardening follow-up routes every canonical player breakpoint through the
owner-aware controller and records owners on primary and related hits.
Repeatable `--player-watch-execute SEGMENT:OFFSET` declarations provide the
generic callback/helper watches previously available only in branch-local
tracers. Patched callback execution is protected: restoration runs after
success, timeout, or Lua error, including failures partway through applying a
multi-patch request.

## Engine conflict policy

Confirmed standalone rendering primitives were retained: PCX decoding,
palette DAC/fade operations, renderer helpers, and player-animation tables.
Rendering's provisional goal/death/checkpoint frontend policy was not allowed
to replace the newer queued level-event, object-effect, and SFX runtime. Its
evidence and fidelity audit remain available for a later evidence-driven
implementation.

## Validation

- `python3 -m unittest discover -s research/tests -q`: 140 passing.
- Fresh CMake configure and build: passing.
- `ctest --test-dir build/engine --output-on-failure`: 11 passing.
- `git diff --check`: passing.

## Next infrastructure pass

1. Extract Ghidra symbols, forced boundaries, types, confidence, and evidence
   references into one machine-readable manifest.
2. Add a PyGhidra proof of concept for the player closure while retaining the
   Java scripts as the reference path.
3. Add trace-schema versions and migrate only actively needed branch-local
   controls into focused scripts.
4. Continue the player movement/collision closure from the canonical
   full-record tracer.

## Infrastructure follow-up

The first follow-up is now in place:

- `research/ghidra/quiky-analysis.json` is the initial versioned
  symbol/evidence manifest for the player closure.
- `research/tools/quiky_ghidra.py validate` checks the manifest and executable
  hash without requiring Ghidra.
- `research/tools/quiky_ghidra.py audit-project` is a read-only PyGhidra proof
  of concept that verifies and decompiles the canonical symbols in an existing
  annotated raw-segment project.
- Trace configuration and ledgers use centralized schema constants; schema
  version 1 remains compatible with existing captures.
