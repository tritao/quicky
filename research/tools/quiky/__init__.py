"""Shared, compatibility-preserving tooling APIs for Quiky research.

The command files in ``research/tools`` remain the stable entry points for
existing notes and scripts.  This package is the new home for shared loading,
normalization, parity, and artifact helpers while the migration is in progress.
"""

from .trace import (
    NormalizedSample,
    NormalizedTrace,
    TraceError,
    extract_samples,
    load_payload,
    load_trace,
    normalize_payload,
    normalize_sample,
)

__all__ = [
    "NormalizedSample",
    "NormalizedTrace",
    "TraceError",
    "extract_samples",
    "load_payload",
    "load_trace",
    "normalize_payload",
    "normalize_sample",
]
