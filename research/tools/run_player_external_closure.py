#!/usr/bin/env python3
"""Rebuild and export the focused player external-state Ghidra closure.

This is deliberately smaller than the player callback baseline.  It imports
the raw NE segments with Ghidra's protected-mode 16-bit x86 language, applies
the existing evidence-backed annotations, exports only the selected external
functions/listings, and compares two independent projects byte-for-byte.
NE relocation records are exported separately because raw BinaryLoader
projects do not resolve the executable's far-call slots.
"""

from __future__ import annotations

import argparse
import hashlib
import json
import os
import shutil
import subprocess
from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]
LEDGER = ROOT / "research/ghidra/player-external-state-closure.json"
SEGMENT_TOOL = ROOT / "research/tools/ghidra_ne_segments.py"
RELOCS = ROOT / "research/tools/ne_relocs.py"
ANNOTATE = ROOT / "research/tools/AnnotateQuiky.java"
PLAYER_ANNOTATE = ROOT / "research/tools/AnnotatePlayerCollision.java"
EXTERNAL_DUMP = ROOT / "research/tools/DumpExternalClosure.java"
LISTING_DUMP = ROOT / "research/tools/DumpFocusedDisasm.java"
DATA_DUMP = ROOT / "research/tools/DumpDataWords.java"
TOOLS = ROOT / "research/tools"


SEG3_TARGETS = [
    "0E06", "0B56", "0A43", "0E96", "0F3C", "0FA2", "0FDC", "1036", "0517", "04DF", "1C6E", "1C92",
    "1CDA", "1E04", "1997", "3529", "34E3", "3808", "321F", "1749", "178D", "1798", "17A3", "1892", "5C11", "05A0",
    "1DCA", "1DEE", "1C4D", "1BD1", "1B07", "1B5D", "0442", "0598", "386F", "3971", "3986", "39FE", "3D02", "3DF2",
    "1B77", "393C", "3A8A", "38CA", "38EC", "34BC",
    "3FF8", "41C1", "41CF", "4416", "44DC", "44FE", "4519", "45AB",
    "470C", "5937", "5C27", "5CC3", "5D38", "5D60", "5DC3", "6328", "16CE",
    "10B5", "1693",
    # Player death/recovery and the callback's boundary-triggered reset path.
    # These are the smallest additional simulation closure; presentation-only
    # menu and renderer consumers remain outside this export.
    "199D", "19E6", "1AAA", "1AE6", "1AF5",
    "3376", "6370", "6484", "648E", "684A", "68C0", "689F", "68AD",
    # Shared normal-enemy contact response and its effect children.
    "4AB3", "4BA0", "4C5D", "4C8B", "4D44", "4DCE", "4EC9", "4F82",
    "9DC7", "A075", "A0B2", "A06F", "A03D",
    "A051", "9F35", "9F4A", "9C70", "9CF5", "9D19", "9D5E", "9D82", "5DA1",
    # BUMP is the next concrete hazard family after the W1L1 closure.  Keep
    # its initializer and callback bounded to the exact type-0x34 path.
    "9BEE", "9C0C",
    # W1L1 object callbacks that can publish player-facing globals, MAP
    # effects, or pooled children.  Keep the family bounded to the actual
    # level inventory; unrelated enemy families remain outside this export.
    "4727", "474D", "47E7", "8BC2", "8BE5", "8C08", "8C2B", "8C4E",
    "8C71", "8C8A", "8CA3", "8CBC", "8CD5", "8CEE", "8D07", "8D20", "8D31", "8E42", "8E4B",
    "9256", "9269", "92A9",
    # W1L1's WURM2 family: both ARE wrappers, their shared initializer, and
    # the complete callback including the player-contact tail.
    "6D5F", "6DA3", "6DB1", "6DC4",
    # Remaining normal-enemy families.  These are the exact initializer and
    # callback entry points used by the archive catalog; their family-specific
    # state machines are exported here so native behavior can be implemented
    # from the same Protected Mode decompilation instead of the provisional
    # hazard fallback.  Shared visibility/MAP/contact helpers are listed above.
    "70D9", "713D", "714B", "715E",
    "771D", "776B", "7779", "778C",
    "7AE3", "7B50", "7B5E", "7B71",
    "6651", "6699", "66E1", "6757", "6838",
    "7E78", "7ED7", "7EE5", "7EF8",
    "840D", "8451", "845F", "8472",
    "500C", "5050", "505E", "5071",
    "5EAC", "5F07", "5F15", "5F28",
]

# These are the three segment-1 main-loop regions containing the scheduler /
# nonzero-state call pairs.  The relocation JSON remains authoritative for
# the exact target identity; this listing makes the local branch/order audit
# reproducible in the same Ghidra language.
SEG1_LISTING = [
    ("3FAD", 1156),
    ("44F0", 48), ("47F0", 48), ("4860", 64),
    # Selector input/launch handoff used by the targeted object traces.  The
    # range includes the loop entry and the post-dispatch cleanup so 4B18 is
    # represented as a handoff edge rather than a synthetic level entry.
    ("4A39", 323),
    # Selector launch / level-load wait gate used by the object and platform
    # tracers.  This is a focused state-machine expansion, not a general
    # main-loop export.
    ("48A0", 192),
    # Main-loop life/death/transition consumers of DS:89EA and DS:880A.
    ("39ED", 448), ("34C7", 128), ("3861", 128),
    # Full 4BA4-4CAE lifecycle branch, including the terminal branch at
    # 4C43-4CAE.  The prior 4B80/192 slice stopped before 4CB1 and could not
    # audit the final gate predicate mechanically.
    ("4B80", 192), ("4CB1", 512),
    # W1L1 completion gate and its post-completion stage handoff. The
    # overlapping windows retain the exact branch joins at 4EA0/4F10/5010.
    ("4EA0", 192), ("4F10", 176), ("5010", 112),
    # 14E1 is the near completion accounting body called from 4F0D; 16C6 is
    # its all-seven completion branch, not an independent function.
    ("14E1", 595),
]

# The lifecycle wait is not player behavior, but its timer ownership is the
# only external writer of the gate observed by 01D7:4BA4. Keep this small
# segment-4 listing in the focused export so the harness boundary remains
# mechanically auditable without importing the timer system.
SEG4_LISTING = [
    ("0002", 48), ("101F", 96), ("10A3", 48),
]

SEG1_TARGETS = [
    "3FAD", "39ED", "34C7", "3861", "4BA4", "4BAE", "4BD8", "4C43", "4CB8", "4EA0", "4EAA",
    # This near body is called by the completion/progression branch and
    # selects gameplay state, so keep it separate from presentation-only
    # callers.  16C6 is an internal branch within 14E1.
    "14E1",
]

# Relocated targets reached only from the focused 01D7:4BA4-4EFE death,
# recovery, and transition gate.  Keep this list address-qualified and small:
# the purpose is to recover simulation-affecting writes hidden behind the
# protected-mode far-call slots, not to decompile the resource/audio system.
SEG2_LIFECYCLE_TARGETS = [
    "082D",  # transition substate/resource selector called by 4D0F-4E7D
    "0C71",  # recovery-side lifecycle helper
    "0CAA",  # shared lifecycle/effect helper
    "0CE3",  # player 199D boundary/death effect target; keep its writes auditable
    "33D5",  # same-segment leaf called after 0x504C=0x18 is published
    "0D18",  # terminal/recovery loader target
]

SEG3_LIFECYCLE_TARGETS = [
    "0908",  # transition-side common helper
    "106A",  # recovery presentation/setup helper
    "17D4",  # scheduler/object teardown callback used by 106A
    "17AE",  # MAP page refresh helper called by 321F
    "1ED7",  # MAP stream/page helper called by 321F
    "1AF5",  # health restore and respawn-row reset
    "20AF",  # MAP page-copy helper called by 321F
    "3062",  # VGA/MAP refresh helper called by 321F
    "31D1",  # MAP page setup helper called by 321F
    "321F",  # world/object rebuild helper
    "5BEF",  # recovery-side callback/resource helper
    "F07B",  # lifecycle state helper
    "F111",  # recovery resource-stage helper
]

SEG4_LIFECYCLE_TARGETS = [
    "0002",  # timer/resource boundary called by 4EDD/4EE6
    "08D8",  # terminal/transition helper
    "17A0",  # terminal/transition helper
    "022A",  # lifecycle wait/effect helper
]

# 01F7:199D reaches 01E7:0CE3 on the player boundary/death path.  Keep the
# short instruction slice next to its focused decompilation so the exact
# RETF/call and flag-preservation contract is independently auditable.
SEG2_LISTING = [
    ("0CE3", 32), ("33D5", 64),
]

# 01E7:0CE8 is relocated to 0227:05CD.  This is the only cross-segment leaf
# reached by the 0CE3 boundary and is kept as a short listing/decompilation
# so the effect-vs-simulation classification is evidence-backed.
SEG5_LIFECYCLE_TARGETS = [
    "05CD",
]

# 01F7:0A43 builds the runtime 0x800-byte table consumed by BIENE's
# nonzero states at 01F7:68C0. These relocated helpers are the smallest
# static expansion that can close that transition without guessing a wave.
SEG5_BIENE_TARGETS = [
    "1185", "1189", "124C", "134F", "13C6", "13F0", "142C",
    "14AC", "14B2", "14C0", "14CC", "14E6", "14F0", "14FA", "1504",
    "150E", "155F", "15E5", "18F1", "1959", "19B6", "19EE", "1B7E",
]

SEG5_LISTING = [
    ("05CD", 64),
    ("1185", 128), ("1189", 128), ("124C", 128), ("134F", 128),
    ("13C6", 128), ("13F0", 128), ("142C", 128),
    ("14AC", 64), ("14B2", 64), ("14C0", 64), ("14CC", 64),
    ("14E6", 64), ("14F0", 64), ("14FA", 64), ("1504", 64),
    ("150E", 192), ("155F", 128), ("15E5", 128), ("18F1", 128),
    ("1959", 96), ("19B6", 96), ("19EE", 96), ("1B7E", 96),
]

# The PRNG multiplier is read as CS:19EC by 19B6.  Keep this data export
# separate from the segment-3 runtime storage dump: 7974 is overwritten by
# 0A43 during startup, while 19EC is a literal consumed by the generator.
SEG5_DATA_RANGES = [
    # 0227:18F1/190A consumes seven six-byte software-float coefficients
    # from CS:1646 while building the runtime table.
    ("1646", 21),
    ("19EC", 8),
]

# Exact instruction listings for the two 5937-selected dispatcher bodies and
# the contact-effect writer path.  The decompiler export remains the primary
# C-like view; these short Ghidra listings preserve RET/RETF and flag-setting
# instructions that are easy to lose in 16-bit decompiler output.
SEG3_LISTING = [
    ("0442", 360), ("05A0", 112), ("3376", 160), ("386F", 64), ("04DF", 40), ("0517", 40),
    ("1B77", 128), ("1BD1", 64), ("1B07", 86), ("1B5D", 128), ("1C4D", 64), ("1DCA", 64), ("393C", 128), ("3A8A", 96),
    ("38CA", 40), ("38EC", 96),
    # Exact scheduler/carry boundary listings.  These preserve the phase,
    # bank-selection, RET/RETF, and flag details that the C-like export can
    # normalize away while remaining inside the player/platform closure.
    ("0E96", 176), ("0FDC", 80), ("0FA2", 64), ("1036", 112),
    # ARE stream walker and the reload-side map rebuild that reaches it.
    ("1CDA", 128), ("1E04", 448), ("1997", 128), ("3529", 128), ("34E3", 128), ("3808", 64), ("321F", 384),
    # Camera/map refresh closure called by 321F. Keep the caller-local 2064
    # helper and the map/page preparation/copy leaves alongside the main
    # body so their simulation-vs-presentation boundary is reproducible.
    ("17AE", 64), ("1ED7", 640), ("2064", 96), ("20C8", 640),
    ("2CB2", 640), ("2F36", 96), ("2F71", 256), ("2FE9", 256),
    ("3062", 640),
    # W1L1 uses all three dedicated ARE event wrappers.  Keep the common
    # producer and wrappers in the focused listing; the renderer/effect
    # consumer remains covered by the existing 16CE/10B5 contract.
    ("1749", 128), ("178D", 24), ("1798", 16), ("17A3", 16),
    ("1892", 256), ("5C11", 96),
    # Descriptor backends and the exact platform cull tail.  These short
    # listings preserve TEST/RETF flag contracts and the A06F -> 1DEE
    # lifetime edge that the decompiler otherwise presents as an opaque call.
    ("1C6E", 72), ("5C27", 96), ("5CC3", 96), ("5DC3", 48), ("34BC", 48),
    ("1DEE", 32), ("A06F", 16), ("9DC7", 256), ("A075", 64), ("A0B2", 96),
    # Callback-side vertical response leaves used by 41C1/41CF.
    # Export the complete callback body as one contiguous protected-mode
    # listing.  The focused leaf windows above preserve individual flag
    # contracts; this window preserves the negative-mode second-probe order
    # (including the 4323/4363 call sites) that the decompiler can split.
    ("3FF8", 1280),
    ("3D02", 128), ("3DF2", 128),
    # Runtime BIENE table construction and remaining simulation-affecting
    # indirect/transition boundaries.
    ("0A43", 280),
    ("41C1", 40), ("4416", 232), ("5937", 624),
    ("4519", 96), ("45AB", 192), ("6370", 160), ("6484", 80), ("648E", 96),
    # Shared normal-enemy action-0x0D/0x02 responses and the 4DCE child
    # callback. These preserve the exact 0E06, PRNG, timer, and 5D60 edges.
    ("4AB3", 256), ("4BA0", 96), ("4C5D", 32), ("4C8B", 128),
    ("4D44", 128), ("4DCE", 192), ("4EC9", 128), ("4F82", 128),
    # W1L1's WURM2 family. Keep both initializer wrappers and the shared
    # initializer alongside the complete callback/state machine.
    ("6D5F", 96), ("6DA3", 64), ("6DB1", 32), ("6DC4", 800),
    # W1L1 ambient, collectible, puzzle, and cloud callbacks. These are the
    # gameplay-facing object families present in the first level inventory.
    ("4727", 192), ("474D", 128), ("47E7", 256),
    ("8BC2", 96), ("8BE5", 32), ("8C08", 32), ("8C2B", 32), ("8C4E", 64),
    ("8C71", 32), ("8C8A", 32), ("8CA3", 32), ("8CBC", 32), ("8CD5", 32),
    ("8CEE", 32), ("8D07", 32), ("8D20", 384), ("8D31", 256), ("8E42", 64), ("8E4B", 1803),
    ("9256", 64), ("9269", 128), ("92A9", 160),
    # W1L1's second normal-enemy family.  Keep both initializer wrappers and
    # the complete callback state machine so the enemy->player damage edge is
    # reproducible from the same Protected Mode export.
    ("684A", 96), ("689F", 64), ("68AD", 32), ("68C0", 512),
    # Remaining normal-enemy family initializers and callbacks.  The windows
    # intentionally stop at the next catalog family boundary; exact branch
    # joins and flag-setting instructions remain visible in the raw listing.
    ("70D9", 640), ("713D", 96), ("714B", 96), ("715E", 640),
    ("771D", 640), ("776B", 96), ("7779", 96), ("778C", 640),
    ("7AE3", 640), ("7B50", 96), ("7B5E", 96), ("7B71", 640),
    ("6651", 560), ("6699", 96), ("66E1", 560), ("6757", 320), ("6838", 64),
    ("7E78", 640), ("7ED7", 96), ("7EE5", 96), ("7EF8", 640),
    ("840D", 640), ("8451", 96), ("845F", 96), ("8472", 640),
    ("500C", 640), ("5050", 96), ("505E", 96), ("5071", 640),
    ("5EAC", 640), ("5F07", 96), ("5F15", 96), ("5F28", 640),
    # Family-specific contact tails are short but affect the response action
    # selected by the callback; retain them separately for an auditable call
    # edge even when the containing callback window overlaps them.
    ("53E7", 48), ("5911", 48), ("6318", 48), ("6D4F", 48),
    ("70C9", 48),
    ("770D", 48), ("7AD3", 48), ("7E68", 48), ("83FD", 48), ("87C1", 48),
    # BUMP type 0x34 initializer/callback; retain the exact overlap and
    # animation-counter instructions for the next W1L2 closure.
    ("9BEE", 96), ("9C0C", 160),
    ("6328", 40), ("16CE", 40), ("10B5", 72), ("1693", 40),
    # Distinct ARE platform initializers that publish the shared 9DC7 callback.
    ("9C70", 128), ("5DA1", 64), ("9CF5", 96), ("9D19", 96), ("9D5E", 128), ("9D82", 128),
    # 1AAA's first relocated call reaches this resource/object declaration
    # helper; keep it in the same narrow recovery/selector closure.
    ("0B56", 192), ("199D", 200), ("1AAA", 96),
]

# Runtime-initialized dispatch/data pointers are not semantic constants in the
# executable.  Preserve their raw imported words so the table boundary remains
# reproducible without assigning guessed meanings to them.
DATA_RANGES = [
    ("657A", 8), ("6D8A", 40), ("30D2", 2),
    # 01F7:0A43 overwrites this 0x800-byte runtime storage during startup.
    # The raw imported words below are therefore only an address/data-image
    # record, not the initialized BIENE table and not a native waveform source.
    ("7974", 512),
    # Temporary view/object dispatch words saved and cleared by 386F.
    ("817C", 2), ("81A6", 4), ("81AA", 4), ("81BE", 4), ("81C2", 4),
    # Runtime aliases consumed by the death/recovery contract.
    ("85D2", 2), ("881A", 2), ("8822", 2), ("8824", 2),
    ("8828", 8), ("89EA", 2),
    # Shared frame-wait flag written by 01F7:F049 and cleared by 01D7:48B5,
    # 0207:0002, and 0207:101F.
    ("819E", 2),
]

# 5D38/5D60 read DS:SI.  At callback time DS is the runtime data segment
# (QUIKY_SEG06, selector 0237 in the traced W1L1 process), not the code
# segment containing 5D38.  Keep these ranges separate so a raw code-segment
# byte dump cannot be mistaken for animation-table data.
ANIMATION_DATA_RANGES = [
    ("3142", 8), ("3156", 8), ("316A", 16), ("3186", 8),
    ("3190", 8), ("31A4", 12), ("31BA", 16),
]


def sha256(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        for block in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(block)
    return digest.hexdigest()


def run(command: list[str], cwd: Path = ROOT, capture: bool = False) -> str:
    print("+", " ".join(str(part) for part in command))
    result = subprocess.run(
        command, cwd=cwd, check=True,
        text=True, capture_output=capture,
    )
    return result.stdout if capture else ""


def ghidra_default() -> Path:
    explicit = os.environ.get("GHIDRA_ANALYZE_HEADLESS")
    if explicit:
        return Path(explicit)
    home = os.environ.get("GHIDRA_HOME")
    if home:
        return Path(home) / "support" / "analyzeHeadless"
    discovered = shutil.which("analyzeHeadless")
    if discovered:
        return Path(discovered)
    packaged = Path("/home/joao/dev/ghidra-12.1.3/support/analyzeHeadless")
    return packaged if packaged.is_file() else Path("analyzeHeadless")


def compare_files(left: Path, right: Path) -> None:
    if left.read_bytes() != right.read_bytes():
        raise RuntimeError(f"focused export drift: {left} != {right}")


def check_sources() -> Path:
    payload = json.loads(LEDGER.read_text(encoding="utf-8"))
    executable = ROOT / payload["source"]["executable"]
    if sha256(executable) != payload["source"]["executable_sha256"]:
        raise RuntimeError(f"executable SHA-256 mismatch: {executable}")
    return executable


def export_relocations(executable: Path, output: Path) -> None:
    raw = run(["python3", str(RELOCS), str(executable), "--json"], capture=True)
    records = json.loads(raw)
    selected = [
        item for item in records
        if item.get("segment") == 1
        and item.get("instruction") in {0x44FA, 0x4518, 0x47FC, 0x481A, 0x4872, 0x4890}
    ]
    death_recovery = [
        item for item in records
        if item.get("segment") == 3 and item.get("instruction") == 0x1B01
    ]
    if len(death_recovery) != 1:
        raise RuntimeError("expected exactly one 01F7:1B01 recovery relocation")
    recovery_target = death_recovery[0]
    if recovery_target.get("target_segment") != 3 or recovery_target.get("target_offset") != 0x1AAA:
        raise RuntimeError("01F7:1B01 does not resolve to 01F7:1AAA")
    payload = {
        "schema": "quiky.player-external-state-relocations.v1",
        "source_sha256": sha256(executable),
        "records": selected,
        "death_recovery_records": death_recovery,
        "expected_order": [
            ["01D7:44FA->01F7:0E96", "01D7:4518->01F7:0FA2"],
            ["01D7:47FC->01F7:0E96", "01D7:481A->01F7:0FA2"],
            ["01D7:4872->01F7:0E96", "01D7:4890->01F7:0FA2"],
        ],
        "expected_death_recovery": "01F7:1B01->01F7:1AAA",
    }
    output.write_text(json.dumps(payload, indent=2) + "\n", encoding="utf-8")


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--ghidra", type=Path, default=ghidra_default())
    parser.add_argument(
        "--output", type=Path,
        default=ROOT / "research/build/player-external-state-closure",
    )
    args = parser.parse_args()

    output = args.output.resolve()
    if output.exists() and any(output.iterdir()):
        raise SystemExit(f"refusing non-empty output directory: {output}")
    output.mkdir(parents=True, exist_ok=True)

    executable = check_sources()
    segments = output / "segments"
    run(["python3", str(SEGMENT_TOOL), "--pad-to-memory", str(executable), str(segments)])
    export_relocations(executable, output / "scheduler-callsites.json")

    segment_paths = [segments / f"QUIKY_SEG0{i}.bin" for i in range(1, 7)]
    exports: list[tuple[Path, Path]] = []
    for label in ("a", "b"):
        project_root = output / f"project-{label}"
        project_root.mkdir(parents=True, exist_ok=True)
        project_name = "QuikyExternalState"
        run([
            str(args.ghidra), str(project_root), project_name,
            "-import", *[str(path) for path in segment_paths],
            "-processor", "x86:LE:16:Protected Mode",
            "-loader", "BinaryLoader", "-loader-baseAddr", "0", "-noanalysis",
            "-scriptPath", str(TOOLS),
            "-commit", "Player external-state closure import",
        ])

        script_path = f"{TOOLS};{output}"
        decomp_dir = output / f"decomp-{label}"
        mainloop_decomp_dir = output / f"decomp-mainloop-{label}"
        lifecycle_seg2_decomp_dir = output / f"decomp-lifecycle-seg2-{label}"
        lifecycle_seg3_decomp_dir = output / f"decomp-lifecycle-seg3-{label}"
        lifecycle_seg4_decomp_dir = output / f"decomp-lifecycle-seg4-{label}"
        disasm_dir = output / f"disasm-{label}"
        seg3_disasm_dir = output / f"disasm-seg3-{label}"
        seg2_disasm_dir = output / f"disasm-seg2-{label}"
        seg5_disasm_dir = output / f"disasm-seg5-{label}"
        seg4_disasm_dir = output / f"disasm-seg4-{label}"
        data_dir = output / f"data-{label}"
        animation_data_dir = output / f"animation-data-{label}"
        run([
            str(args.ghidra), str(project_root), project_name,
            "-process", "QUIKY_SEG03.bin", "-noanalysis",
            "-postScript", "AnnotateQuiky.java",
            "-postScript", "AnnotatePlayerCollision.java",
            "-postScript", EXTERNAL_DUMP.name, str(decomp_dir), *SEG3_TARGETS,
            "-scriptPath", script_path,
            "-commit", "Player external-state closure decompilation",
        ])
        run([
            str(args.ghidra), str(project_root), project_name,
            "-process", "QUIKY_SEG01.bin", "-noanalysis",
            "-postScript", EXTERNAL_DUMP.name, str(mainloop_decomp_dir),
            *SEG1_TARGETS,
            "-scriptPath", script_path,
            "-commit", "Player death lifecycle consumer decompilation",
        ])
        run([
            str(args.ghidra), str(project_root), project_name,
            "-process", "QUIKY_SEG02.bin", "-noanalysis",
            "-postScript", EXTERNAL_DUMP.name, str(lifecycle_seg2_decomp_dir),
            *SEG2_LIFECYCLE_TARGETS,
            "-scriptPath", script_path,
            "-commit", "Player lifecycle segment-2 target decompilation",
        ])
        run([
            str(args.ghidra), str(project_root), project_name,
            "-process", "QUIKY_SEG03.bin", "-noanalysis",
            "-postScript", EXTERNAL_DUMP.name, str(lifecycle_seg3_decomp_dir),
            *SEG3_LIFECYCLE_TARGETS,
            "-scriptPath", script_path,
            "-commit", "Player lifecycle segment-3 target decompilation",
        ])
        run([
            str(args.ghidra), str(project_root), project_name,
            "-process", "QUIKY_SEG04.bin", "-noanalysis",
            "-postScript", EXTERNAL_DUMP.name, str(lifecycle_seg4_decomp_dir),
            *SEG4_LIFECYCLE_TARGETS,
            "-scriptPath", script_path,
            "-commit", "Player lifecycle segment-4 target decompilation",
        ])
        run([
            str(args.ghidra), str(project_root), project_name,
            "-process", "QUIKY_SEG01.bin", "-noanalysis",
            "-postScript", LISTING_DUMP.name, str(disasm_dir),
            *[value for offset, count in SEG1_LISTING for value in (offset, str(count))],
            "-scriptPath", script_path,
            "-commit", "Player scheduler call-site listing",
        ])
        run([
            str(args.ghidra), str(project_root), project_name,
            "-process", "QUIKY_SEG03.bin", "-noanalysis",
            "-postScript", LISTING_DUMP.name, str(seg3_disasm_dir),
            *[value for offset, count in SEG3_LISTING for value in (offset, str(count))],
            "-scriptPath", script_path,
            "-commit", "Player external boundary instruction listing",
        ])
        run([
            str(args.ghidra), str(project_root), project_name,
            "-process", "QUIKY_SEG04.bin", "-noanalysis",
            "-postScript", LISTING_DUMP.name, str(seg4_disasm_dir),
            *[value for offset, count in SEG4_LISTING for value in (offset, str(count))],
            "-scriptPath", script_path,
            "-commit", "Player lifecycle timer boundary listing",
        ])
        run([
            str(args.ghidra), str(project_root), project_name,
            "-process", "QUIKY_SEG02.bin", "-noanalysis",
            "-postScript", LISTING_DUMP.name, str(seg2_disasm_dir),
            *[value for offset, count in SEG2_LISTING for value in (offset, str(count))],
            "-scriptPath", script_path,
            "-commit", "Player transition effect boundary listing",
        ])
        run([
            str(args.ghidra), str(project_root), project_name,
            "-process", "QUIKY_SEG05.bin", "-noanalysis",
            "-postScript", EXTERNAL_DUMP.name, str(output / f"decomp-lifecycle-seg5-{label}"),
            *SEG5_LIFECYCLE_TARGETS,
            "-scriptPath", script_path,
            "-commit", "Player transition effect target decompilation",
        ])
        run([
            str(args.ghidra), str(project_root), project_name,
            "-process", "QUIKY_SEG05.bin", "-noanalysis",
            "-postScript", EXTERNAL_DUMP.name, str(output / f"decomp-biene-seg5-{label}"),
            *SEG5_BIENE_TARGETS,
            "-scriptPath", script_path,
            "-commit", "BIENE runtime-table helper decompilation",
        ])
        run([
            str(args.ghidra), str(project_root), project_name,
            "-process", "QUIKY_SEG05.bin", "-noanalysis",
            "-postScript", LISTING_DUMP.name, str(seg5_disasm_dir),
            *[value for offset, count in SEG5_LISTING for value in (offset, str(count))],
            "-scriptPath", script_path,
            "-commit", "Player transition effect target listing",
        ])
        biene_data_dir = output / f"biene-data-{label}"
        run([
            str(args.ghidra), str(project_root), project_name,
            "-process", "QUIKY_SEG05.bin", "-noanalysis",
            "-postScript", DATA_DUMP.name,
            str(biene_data_dir / "runtime-table-constants.txt"),
            *[value for offset, count in SEG5_DATA_RANGES
              for value in (offset, str(count))],
            "-scriptPath", script_path,
            "-commit", "BIENE runtime-table constant listing",
        ])
        run([
            str(args.ghidra), str(project_root), project_name,
            "-process", "QUIKY_SEG03.bin", "-noanalysis",
            "-postScript", DATA_DUMP.name, str(data_dir / "dispatch-tables.txt"),
            *[value for offset, count in DATA_RANGES for value in (offset, str(count))],
            "-scriptPath", script_path,
            "-commit", "Player external boundary data listing",
        ])
        run([
            str(args.ghidra), str(project_root), project_name,
            "-process", "QUIKY_SEG06.bin", "-noanalysis",
            "-postScript", DATA_DUMP.name,
            str(animation_data_dir / "player-animation-words.txt"),
            *[value for offset, count in ANIMATION_DATA_RANGES
              for value in (offset, str(count))],
            "-scriptPath", script_path,
            "-commit", "Player runtime animation data listing",
        ])
        exports.append((
            decomp_dir / "QUIKY_SEG03.bin.c",
            mainloop_decomp_dir / "QUIKY_SEG01.bin.c",
            lifecycle_seg2_decomp_dir / "QUIKY_SEG02.bin.c",
            lifecycle_seg3_decomp_dir / "QUIKY_SEG03.bin.c",
            lifecycle_seg4_decomp_dir / "QUIKY_SEG04.bin.c",
            disasm_dir / "QUIKY_SEG01.bin.asm",
            seg3_disasm_dir / "QUIKY_SEG03.bin.asm",
            seg4_disasm_dir / "QUIKY_SEG04.bin.asm",
            seg2_disasm_dir / "QUIKY_SEG02.bin.asm",
            output / f"decomp-lifecycle-seg5-{label}" / "QUIKY_SEG05.bin.c",
            seg5_disasm_dir / "QUIKY_SEG05.bin.asm",
            output / f"decomp-biene-seg5-{label}" / "QUIKY_SEG05.bin.c",
            data_dir / "dispatch-tables.txt",
            animation_data_dir / "player-animation-words.txt",
            biene_data_dir / "runtime-table-constants.txt",
        ))

    compare_files(exports[0][0], exports[1][0])
    compare_files(exports[0][1], exports[1][1])
    compare_files(exports[0][2], exports[1][2])
    compare_files(exports[0][3], exports[1][3])
    compare_files(exports[0][4], exports[1][4])
    compare_files(exports[0][5], exports[1][5])
    compare_files(exports[0][6], exports[1][6])
    compare_files(exports[0][7], exports[1][7])
    compare_files(exports[0][8], exports[1][8])
    compare_files(exports[0][9], exports[1][9])
    compare_files(exports[0][10], exports[1][10])
    compare_files(exports[0][11], exports[1][11])
    compare_files(exports[0][12], exports[1][12])
    compare_files(exports[0][13], exports[1][13])
    compare_files(exports[0][14], exports[1][14])
    print("OK: independent Ghidra external-state decompilation and scheduler listings matched")
    print(f"OK: executable sha256 {sha256(executable)}")
    print(f"OK: output {output}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
