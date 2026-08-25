#!/usr/bin/env python3
"""Rebuild and export the raw-segment Ghidra player-callback baseline.

The executable is split into its six NE segment images, imported at address
zero with the protected-mode 16-bit x86 language, and annotated from the
authoritative closure ledger.  Two independent projects are produced and
their decompilation/call-graph exports are byte-compared to detect drift.
"""

from __future__ import annotations

import argparse
import hashlib
import json
import subprocess
from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]
DEFAULT_GHIDRA = Path("/home/joao/dev/ghidra-12.1.3/support/analyzeHeadless")
MANIFEST = ROOT / "research/ghidra/player-callback-closure.json"
SEGMENT_TOOL = ROOT / "research/tools/ghidra_ne_segments.py"
GENERATOR = ROOT / "research/tools/generate_player_closure_ghidra.py"
GRAPH_EXPORTER = ROOT / "research/tools/export_player_callback_graph.py"
TOOLS = ROOT / "research/tools"


def sha256(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        for block in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(block)
    return digest.hexdigest()


def run(command: list[str], cwd: Path = ROOT) -> None:
    print("+", " ".join(str(part) for part in command))
    subprocess.run(command, cwd=cwd, check=True)


def load_offsets() -> list[str]:
    payload = json.loads(MANIFEST.read_text(encoding="utf-8"))
    offsets: list[str] = []
    for item in payload["functions"]:
        selector, offset = item["address"].split(":", 1)
        if selector.upper() == "01F7":
            offsets.append(offset.upper())
    return offsets


def compare_files(left: Path, right: Path) -> None:
    left_bytes = left.read_bytes()
    right_bytes = right.read_bytes()
    if left_bytes != right_bytes:
        raise RuntimeError(f"baseline export drift: {left} != {right}")


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--ghidra", type=Path, default=DEFAULT_GHIDRA)
    parser.add_argument("--output", type=Path,
                        default=ROOT / "research/build/player-callback-baseline")
    args = parser.parse_args()

    output = args.output.resolve()
    if output.exists() and any(output.iterdir()):
        raise SystemExit(f"refusing non-empty output directory: {output}")
    output.mkdir(parents=True, exist_ok=True)

    segments = output / "segments"
    segments.mkdir()
    run(["python3", str(SEGMENT_TOOL), str(ROOT / "game/QUIKY.EXE"), str(segments)])

    generated = output / "ApplyPlayerCallbackClosure.java"
    run(["python3", str(GENERATOR), "--output", str(generated)])
    run(["python3", str(GENERATOR), "--output", str(generated), "--check"])

    segment_paths = [segments / f"QUIKY_SEG0{i}.bin" for i in range(1, 7)]
    offsets = load_offsets()
    exports: list[tuple[Path, Path, Path]] = []
    for label in ("a", "b"):
        project_root = output / f"project-{label}"
        project_root.mkdir(parents=True, exist_ok=True)
        project_name = "QuikySegments"
        run([
            str(args.ghidra), str(project_root), project_name,
            "-import", *[str(path) for path in segment_paths],
            "-processor", "x86:LE:16:Protected Mode",
            "-loader", "BinaryLoader", "-loader-baseAddr", "0", "-noanalysis",
            "-postScript", "AnnotateQuiky.java",
            "-scriptPath", str(TOOLS),
            "-commit", "Player callback closure baseline import",
        ])

        script_path = f"{TOOLS};{generated.parent}"
        decomp_dir = output / f"decomp-{label}"
        graph = output / f"callgraph-{label}.json"
        ghidra_graph = output / f"ghidra-callgraph-{label}.json"
        run([
            str(args.ghidra), str(project_root), project_name,
            "-process", "QUIKY_SEG03.bin", "-noanalysis",
            "-postScript", "AnnotatePlayerCollision.java",
            "-postScript", generated.name,
            "-postScript", "DumpPlayerCallbackClosure.java", str(decomp_dir), *offsets,
            "-postScript", "ExportPlayerCallbackCallGraph.java", str(ghidra_graph), *offsets,
            "-scriptPath", script_path,
            "-commit", "Player callback closure annotation export",
        ])
        run([
            str(args.ghidra), str(project_root), project_name,
            "-process", "QUIKY_SEG02.bin", "-noanalysis",
            "-postScript", generated.name,
            "-scriptPath", script_path,
            "-commit", "Player callback external contract annotation",
        ])
        run(["python3", str(GRAPH_EXPORTER), "--output", str(graph)])
        exports.append((decomp_dir / "QUIKY_SEG03.bin.c", graph, ghidra_graph))

    compare_files(exports[0][0], exports[1][0])
    compare_files(exports[0][1], exports[1][1])
    compare_files(exports[0][2], exports[1][2])
    print("OK: independent Ghidra imports produced identical decompilation and call graph")
    print(f"OK: executable sha256 {sha256(ROOT / 'game/QUIKY.EXE')}")
    print(f"OK: output {output}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
