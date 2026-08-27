#!/usr/bin/env python3
"""Import one captured runtime callback into independent Protected Mode Ghidra projects.

Runtime-loaded callback bodies are not present in QUIKY.EXE.  This runner pads
the captured entry window to its recorded runtime offset, imports it as a
standalone 16-bit protected-mode image, and exports the exact entry listing and
Ghidra decompilation twice.  The two exports must match byte-for-byte before
the result is used as static evidence.
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
TOOLS = ROOT / "research/tools"
EXTRACT = ROOT / "research/tools/extract_runtime_target.py"


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


def run(command: list[str], cwd: Path = ROOT) -> None:
    print("+", " ".join(str(part) for part in command))
    subprocess.run(command, cwd=cwd, check=True)


def sha256(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        for block in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(block)
    return digest.hexdigest()


def compare_files(left: Path, right: Path) -> None:
    if left.read_bytes() != right.read_bytes():
        raise RuntimeError(f"runtime-target export drift: {left} != {right}")


def padded_image(raw: Path, metadata: Path, output: Path) -> tuple[int, int]:
    record = json.loads(metadata.read_text(encoding="utf-8"))
    offset = int(record["entry_offset"])
    code = raw.read_bytes()
    image = b"\x00" * offset + code
    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_bytes(image)
    return offset, len(code)


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("image", type=Path,
                        help="raw image produced by extract_runtime_target.py")
    parser.add_argument("--metadata", type=Path)
    parser.add_argument("--ghidra", type=Path, default=ghidra_default())
    parser.add_argument("--output", type=Path,
                        default=ROOT / "research/build/runtime-target-static-v1")
    args = parser.parse_args()

    raw = args.image.resolve()
    metadata = (args.metadata or raw.with_suffix(raw.suffix + ".json")).resolve()
    output = args.output.resolve()
    if output.exists() and any(output.iterdir()):
        raise SystemExit(f"refusing non-empty output directory: {output}")
    output.mkdir(parents=True, exist_ok=True)

    entry_offset, code_size = padded_image(
        raw, metadata, output / "runtime-target-padded.bin")
    metadata_record = json.loads(metadata.read_text(encoding="utf-8"))
    if int(metadata_record["entry_offset"]) != entry_offset:
        raise RuntimeError("runtime target metadata changed while preparing image")

    exports: list[tuple[Path, Path]] = []
    for label in ("a", "b"):
        project_root = output / f"project-{label}"
        project_root.mkdir(parents=True, exist_ok=True)
        project_name = "QuikyRuntimeTarget"
        run([
            str(args.ghidra), str(project_root), project_name,
            "-import", str(output / "runtime-target-padded.bin"),
            "-processor", "x86:LE:16:Protected Mode",
            "-loader", "BinaryLoader", "-loader-baseAddr", "0", "-noanalysis",
            "-scriptPath", str(TOOLS),
            "-commit", "Runtime callback import",
        ])

        script_path = f"{TOOLS};{output}"
        decomp_dir = output / f"decomp-{label}"
        disasm_dir = output / f"disasm-{label}"
        program = "runtime-target-padded.bin"
        run([
            str(args.ghidra), str(project_root), project_name,
            "-process", program, "-noanalysis",
            "-postScript", "DumpExternalClosure.java", str(decomp_dir),
            f"{entry_offset:04X}",
            "-scriptPath", script_path,
            "-commit", "Runtime callback decompilation",
        ])
        run([
            str(args.ghidra), str(project_root), project_name,
            "-process", program, "-noanalysis",
            "-postScript", "DumpFocusedDisasm.java", str(disasm_dir),
            f"{entry_offset:04X}", str(code_size),
            "-scriptPath", script_path,
            "-commit", "Runtime callback instruction listing",
        ])
        exports.append((
            decomp_dir / f"{program}.c",
            disasm_dir / f"{program}.asm",
        ))

    compare_files(exports[0][0], exports[1][0])
    compare_files(exports[0][1], exports[1][1])
    manifest = {
        "schema": "quiky.runtime-loaded-target-static-export.v1",
        "raw_image": str(raw),
        "raw_sha256": sha256(raw),
        "metadata": str(metadata),
        "runtime_selector": metadata_record["runtime_selector"],
        "entry_offset": entry_offset,
        "byte_count": code_size,
        "decomp_sha256": sha256(exports[0][0]),
        "disasm_sha256": sha256(exports[0][1]),
    }
    (output / "manifest.json").write_text(
        json.dumps(manifest, indent=2) + "\n", encoding="utf-8")
    print("OK: independent runtime-target Ghidra exports matched")
    print(f"OK: raw sha256 {manifest['raw_sha256']}")
    print(f"OK: output {output}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
