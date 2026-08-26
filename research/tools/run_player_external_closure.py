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
TOOLS = ROOT / "research/tools"


SEG3_TARGETS = [
    "0E06", "0E96", "0F3C", "0FA2", "0FDC", "1036", "1C6E", "1C92",
    "1DCA", "1DEE", "0442", "386F", "3971", "3986", "39FE", "3D02", "3DF2",
    "3FF8", "41C1", "41CF", "4416", "44DC", "44FE", "4519", "45AB",
    "470C", "5937", "5C27", "5CC3", "5D38", "5D60", "5DC3", "6328",
    "6370", "6484", "648E", "9DC7", "A075", "A0B2", "A06F", "A03D",
    "A051", "9F35", "9F4A",
]

# These are the three segment-1 main-loop regions containing the scheduler /
# nonzero-state call pairs.  The relocation JSON remains authoritative for
# the exact target identity; this listing makes the local branch/order audit
# reproducible in the same Ghidra language.
SEG1_LISTING = [("44F0", 48), ("47F0", 48), ("4860", 64)]


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
    payload = {
        "schema": "quiky.player-external-state-relocations.v1",
        "source_sha256": sha256(executable),
        "records": selected,
        "expected_order": [
            ["01D7:44FA->01F7:0E96", "01D7:4518->01F7:0FA2"],
            ["01D7:47FC->01F7:0E96", "01D7:481A->01F7:0FA2"],
            ["01D7:4872->01F7:0E96", "01D7:4890->01F7:0FA2"],
        ],
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
        disasm_dir = output / f"disasm-{label}"
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
            "-postScript", LISTING_DUMP.name, str(disasm_dir),
            *[value for offset, count in SEG1_LISTING for value in (offset, str(count))],
            "-scriptPath", script_path,
            "-commit", "Player scheduler call-site listing",
        ])
        exports.append((
            decomp_dir / "QUIKY_SEG03.bin.c",
            disasm_dir / "QUIKY_SEG01.bin.asm",
        ))

    compare_files(exports[0][0], exports[1][0])
    compare_files(exports[0][1], exports[1][1])
    print("OK: independent Ghidra external-state decompilation and scheduler listings matched")
    print(f"OK: executable sha256 {sha256(executable)}")
    print(f"OK: output {output}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
