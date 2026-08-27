"""Small dependency-free helpers shared by research tools."""

from __future__ import annotations

import hashlib
import json
import subprocess
import sys
from pathlib import Path
from typing import Any, Iterable


class ToolError(Exception):
    """Raised when a shared tooling operation cannot be completed."""


def read_json(path: Path) -> Any:
    """Read a UTF-8 JSON file and preserve the caller's error boundary."""

    try:
        return json.loads(path.read_text(encoding="utf-8"))
    except (OSError, json.JSONDecodeError) as exc:
        raise ToolError(f"{path}: cannot read JSON: {exc}") from exc


def write_json(path: Path, value: Any, *, indent: int = 2) -> None:
    """Write stable, newline-terminated JSON, creating only the parent tree."""

    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(
        json.dumps(value, indent=indent, sort_keys=True) + "\n",
        encoding="utf-8",
    )


def sha256_file(path: Path, *, chunk_size: int = 1024 * 1024) -> str:
    """Return the SHA-256 digest of a file without loading it all at once."""

    digest = hashlib.sha256()
    with path.open("rb") as stream:
        for chunk in iter(lambda: stream.read(chunk_size), b""):
            digest.update(chunk)
    return digest.hexdigest()


def file_fingerprint(path: Path) -> dict[str, Any]:
    """Return the stable file facts used by recorded-run manifests."""

    return {
        "path": str(path),
        "size": path.stat().st_size,
        "sha256": sha256_file(path),
    }


def tools_root() -> Path:
    """Locate ``research/tools`` from this package, independent of cwd."""

    return Path(__file__).resolve().parents[1]


def run_compat_script(script: str | Path, args: Iterable[str]) -> int:
    """Run one legacy command file with the current Python interpreter.

    This is intentionally a subprocess boundary.  Several historical
    verifier files expose ``main()`` methods that parse ``sys.argv`` directly;
    importing and mutating that global would make the new dispatcher fragile.
    """

    script_path = Path(script)
    if not script_path.is_absolute():
        script_path = tools_root() / script_path
    if not script_path.is_file():
        raise ToolError(f"compatibility tool does not exist: {script_path}")
    completed = subprocess.run(
        [sys.executable, str(script_path), *list(args)],
        check=False,
    )
    return completed.returncode
