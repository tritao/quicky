"""Allow ``python -m quiky`` when research/tools is on PYTHONPATH."""

from __future__ import annotations

from .cli import main


raise SystemExit(main())
