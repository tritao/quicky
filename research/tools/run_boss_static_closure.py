#!/usr/bin/env python3
"""Export the bounded pooled boss/end-stage Ghidra closure.

This is intentionally separate from the player external-state runner.  The
closure contains only the world-specific pooled boss constructors, damage
callbacks, phase/effect children, and the small scheduler/completion edges
needed to determine whether those objects can feed back into simulation.
The raw NE segments are imported with Ghidra's protected-mode 16-bit x86
language; no external x86 disassembler is used.
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
SEGMENT_TOOL = ROOT / "research/tools/ghidra_ne_segments.py"
TOOLS = ROOT / "research/tools"
EXE = ROOT / "game/QUIKY.EXE"

# These are the address ranges whose raw instruction order matters most for
# the boss contract.  DumpBossDecomp.java supplies the complete bounded
# decompilation target list; this listing preserves exact Jcc/RET boundaries
# for the world constructors, damage/main callbacks, and child callbacks.
SEG3_LISTING = [
    ("B142", 192), ("B25D", 224), ("B33B", 768),
    ("B84D", 64), ("B87B", 64),
    ("B9F3", 192), ("BB0E", 224), ("BBEC", 640),
    ("C1A0", 224), ("BAD7", 224),
    ("C28A", 192), ("C328", 224), ("C40B", 768),
    ("CB11", 256),
    ("CC68", 192), ("CDA3", 224), ("CE81", 640),
    ("CD40", 224),
    ("D2F6", 192), ("D55A", 224), ("D63D", 768),
    ("D3EE", 256), ("D4D9", 256), ("D438", 256),
    ("E0F5", 256), ("E2BF", 256),
    # Shared pool/effect/animation edges used by all five families.
    ("0E06", 64), ("0E96", 192), ("0FA2", 96), ("1036", 128),
    ("1B77", 192), ("393C", 192), ("44FF", 64), ("4519", 128),
    ("45AB", 256), ("470C", 64), ("4C74", 256),
    ("5C11", 96), ("5C27", 128), ("5D38", 96), ("5D60", 128),
]

SEG1_LISTING = [
    ("4EA0", 192), ("4F10", 176), ("5010", 112),
    ("4BA4", 192), ("4CB1", 512),
]

SEG1_TARGETS = ["4EA0", "4EAA", "4F10", "5010", "4BA4", "4BAE", "4BD8", "4C43", "4CB8"]

SEG2_TARGETS = ["0CAA"]
SEG5_TARGETS = ["05CD", "1B7E", "0271"]


def sha256(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        for block in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(block)
    return digest.hexdigest()


def run(command: list[str], cwd: Path = ROOT, capture: bool = False) -> str:
    print("+", " ".join(str(part) for part in command))
    result = subprocess.run(
        command, cwd=cwd, check=True, text=True, capture_output=capture,
    )
    return result.stdout if capture else ""


def ghidra_default() -> Path:
    explicit = os.environ.get("GHIDRA_ANALYZE_HEADLESS")
    if explicit:
        return Path(explicit)
    ghidra_home = os.environ.get("GHIDRA_HOME")
    if ghidra_home:
        return Path(ghidra_home) / "support" / "analyzeHeadless"
    discovered = shutil.which("analyzeHeadless")
    if discovered:
        return Path(discovered)
    packaged = Path("/home/joao/dev/ghidra-12.1.3/support/analyzeHeadless")
    return packaged if packaged.is_file() else Path("analyzeHeadless")


def compare_files(left: Path, right: Path) -> None:
    if left.read_bytes() != right.read_bytes():
        raise RuntimeError(f"boss closure export drift: {left} != {right}")


def import_project(ghidra: Path, project_root: Path, segment_paths: list[Path]) -> None:
    project_root.mkdir(parents=True, exist_ok=True)
    run([
        str(ghidra), str(project_root), "QuikyBossClosure",
        "-import", *[str(path) for path in segment_paths],
        "-processor", "x86:LE:16:Protected Mode",
        "-loader", "BinaryLoader", "-loader-baseAddr", "0", "-noanalysis",
        "-scriptPath", str(TOOLS),
        "-commit", "Pooled boss closure import",
    ])


def dump_decomp(ghidra: Path, project_root: Path, output: Path) -> Path:
    output.mkdir(parents=True, exist_ok=True)
    run([
        str(ghidra), str(project_root), "QuikyBossClosure",
        "-process", "QUIKY_SEG03.bin", "-noanalysis",
        "-postScript", "DumpBossDecomp.java", str(output),
        "-scriptPath", f"{TOOLS};{output}",
        "-commit", "Pooled boss closure segment-3 decompilation",
    ])
    return output / "QUIKY_SEG03.bin-boss.c"


def dump_seg1_decomp(ghidra: Path, project_root: Path, output: Path) -> Path:
    output.mkdir(parents=True, exist_ok=True)
    run([
        str(ghidra), str(project_root), "QuikyBossClosure",
        "-process", "QUIKY_SEG01.bin", "-noanalysis",
        "-postScript", "DumpExternalClosure.java", str(output),
        *SEG1_TARGETS,
        "-scriptPath", f"{TOOLS};{output}",
        "-commit", "Pooled boss completion edge decompilation",
    ])
    return output / "QUIKY_SEG01.bin.c"


def dump_small_decomp(ghidra: Path, project_root: Path, segment: str,
                      targets: list[str], output: Path) -> Path:
    output.mkdir(parents=True, exist_ok=True)
    run([
        str(ghidra), str(project_root), "QuikyBossClosure",
        "-process", segment, "-noanalysis",
        "-postScript", "DumpExternalClosure.java", str(output), *targets,
        "-scriptPath", f"{TOOLS};{output}",
        "-commit", f"Pooled boss {segment} target decompilation",
    ])
    return output / f"{segment}.c"


def dump_listing(ghidra: Path, project_root: Path, segment: str,
                 ranges: list[tuple[str, int]], output: Path) -> Path:
    output.mkdir(parents=True, exist_ok=True)
    args = [value for offset, count in ranges for value in (offset, str(count))]
    run([
        str(ghidra), str(project_root), "QuikyBossClosure",
        "-process", segment, "-noanalysis",
        "-postScript", "DumpFocusedDisasm.java", str(output), *args,
        "-scriptPath", f"{TOOLS};{output}",
        "-commit", f"Pooled boss {segment} instruction listing",
    ])
    return output / f"{segment}.asm"


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--ghidra", type=Path, default=ghidra_default())
    parser.add_argument(
        "--output", type=Path,
        default=ROOT / "research/build/boss-static-closure-v1",
    )
    args = parser.parse_args()
    output = args.output.resolve()
    if output.exists() and any(output.iterdir()):
        raise SystemExit(f"refusing non-empty output directory: {output}")
    output.mkdir(parents=True, exist_ok=True)

    if not EXE.is_file():
        raise SystemExit(f"missing source executable: {EXE}")
    segments = output / "segments"
    run(["python3", str(SEGMENT_TOOL), "--pad-to-memory", str(EXE), str(segments)])
    segment_paths = [segments / f"QUIKY_SEG0{i}.bin" for i in range(1, 7)]
    if any(not path.is_file() for path in segment_paths):
        raise RuntimeError("segment extraction did not produce all file-backed segments")

    exports: list[dict[str, Path]] = []
    for label in ("a", "b"):
        project = output / f"project-{label}"
        import_project(args.ghidra, project, segment_paths)
        paths = {
            "seg3_decomp": dump_decomp(
                args.ghidra, project, output / f"decomp-seg3-{label}"),
            "seg1_decomp": dump_seg1_decomp(
                args.ghidra, project, output / f"decomp-seg1-{label}"),
            "seg2_decomp": dump_small_decomp(
                args.ghidra, project, "QUIKY_SEG02.bin", SEG2_TARGETS,
                output / f"decomp-seg2-{label}"),
            "seg5_decomp": dump_small_decomp(
                args.ghidra, project, "QUIKY_SEG05.bin", SEG5_TARGETS,
                output / f"decomp-seg5-{label}"),
            "seg3_listing": dump_listing(
                args.ghidra, project, "QUIKY_SEG03.bin", SEG3_LISTING,
                output / f"listing-seg3-{label}"),
            "seg1_listing": dump_listing(
                args.ghidra, project, "QUIKY_SEG01.bin", SEG1_LISTING,
                output / f"listing-seg1-{label}"),
        }
        exports.append(paths)

    for key in exports[0]:
        compare_files(exports[0][key], exports[1][key])
    manifest = {
        "schema": "quiky.boss-static-closure-export.v1",
        "source_executable": str(EXE.relative_to(ROOT)),
        "source_executable_sha256": sha256(EXE),
        "ghidra_language": "x86:LE:16:Protected Mode",
        "runner": str(Path(__file__).relative_to(ROOT)),
        "decomp_script": "research/tools/DumpBossDecomp.java",
        "listing_script": "research/tools/DumpFocusedDisasm.java",
        "segments": "raw NE segments from ghidra_ne_segments.py --pad-to-memory",
        "exports": {key: str(path.relative_to(output))
                    for key, path in exports[0].items()},
        "sha256": {key: sha256(path) for key, path in exports[0].items()},
        "independent_export_sha256": {
            key: sha256(path) for key, path in exports[1].items()},
        "targets": {
            "segment_3_listing": SEG3_LISTING,
            "segment_1_listing": SEG1_LISTING,
            "segment_2": SEG2_TARGETS,
            "segment_5": SEG5_TARGETS,
        },
        "static_boundary": (
            "Five world-specific pooled boss constructors, damage/main callbacks, "
            "phase/effect children, pool insertion, and completion handoff. "
            "Runtime authored timing and presentation-only leaves remain separate."
        ),
    }
    (output / "manifest.json").write_text(
        json.dumps(manifest, indent=2) + "\n", encoding="utf-8")
    print("OK: independent Ghidra pooled boss closure exports matched")
    print(f"OK: executable sha256 {sha256(EXE)}")
    print(f"OK: output {output}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
