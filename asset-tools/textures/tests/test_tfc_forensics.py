#!/usr/bin/env python3
"""Tests for tfc_forensics.py - windowed file comparison, no game data needed."""

from __future__ import annotations

import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
from tfc_forensics import sample  # noqa: E402


def _blob(path: Path, size: int, seed: int) -> None:
    import random
    rnd = random.Random(seed)
    with path.open("wb") as f:
        remaining = size
        while remaining > 0:
            chunk = bytes(rnd.randrange(256) for _ in range(min(65536, remaining)))
            f.write(chunk)
            remaining -= len(chunk)


def test_identical_files_compare_identical(tmp_path: Path) -> None:
    a, b = tmp_path / "a.bin", tmp_path / "b.bin"
    _blob(a, 3 << 20, 1)
    b.write_bytes(a.read_bytes())
    offs = [0x1000 + i * (3 << 20) // 8 for i in range(8)]
    assert sample(a, offs) == sample(b, offs)


def test_different_files_differ(tmp_path: Path) -> None:
    a, b = tmp_path / "a.bin", tmp_path / "b.bin"
    _blob(a, 3 << 20, 1)
    _blob(b, 3 << 20, 2)
    offs = [0x1000 + i * (3 << 20) // 8 for i in range(8)]
    sa, sb = sample(a, offs), sample(b, offs)
    assert any(x != y for x, y in zip(sa, sb))


def test_single_byte_difference_detected(tmp_path: Path) -> None:
    a, b = tmp_path / "a.bin", tmp_path / "b.bin"
    _blob(a, 3 << 20, 1)
    data = bytearray(a.read_bytes())
    data[2 << 20] ^= 0xFF
    b.write_bytes(bytes(data))
    offs = [0x1000 + i * (3 << 20) // 8 for i in range(8)]
    sa, sb = sample(a, offs), sample(b, offs)
    assert sum(1 for x, y in zip(sa, sb) if x != y) >= 1


if __name__ == "__main__":
    import tempfile
    fails = 0
    with tempfile.TemporaryDirectory() as td:
        t = Path(td)
        for fn in (test_identical_files_compare_identical,
                   test_different_files_differ,
                   test_single_byte_difference_detected):
            try:
                fn(t)
                print(f"PASS: {fn.__name__}")
            except AssertionError as e:
                fails += 1
                print(f"FAIL: {fn.__name__}: {e}")
    sys.exit(1 if fails else 0)
