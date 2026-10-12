#!/usr/bin/env python3
"""Tests for mip_forensics.py Reader primitives and package validation.

Uses synthetic byte buffers only - no game data. The full parse path needs
decompressed package input, which is currently blocked (see
docs/SOURCE_RECOVERY_FINDINGS.md E4); these tests pin the primitives so the
tool is ready when input becomes available.
"""

from __future__ import annotations

import struct
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
from mip_forensics import Reader, TAG  # noqa: E402


def _ansi(s: str) -> bytes:
    b = s.encode("latin1") + b"\0"
    return struct.pack("<i", len(b)) + b


def test_u32_i32_u64_roundtrip() -> None:
    r = Reader(struct.pack("<IIQ", 0x9E2A83C1, 0xFFFFFFFF, 0x1122334455667788))
    assert r.u32() == TAG
    assert r.i32() == -1
    assert r.u64() == 0x1122334455667788
    assert not r.bad


def test_fname_pair() -> None:
    r = Reader(struct.pack("<iiii", 7, 0, 3, 2))
    assert r.fname() == (7, 0)
    assert r.fname() == (3, 2)


def test_fstring_ansi() -> None:
    r = Reader(_ansi("None") + _ansi("Texture2D"))
    assert r.fstring() == "None"
    assert r.fstring() == "Texture2D"
    assert not r.bad


def test_fstring_empty() -> None:
    r = Reader(struct.pack("<i", 0))
    assert r.fstring() == ""


def test_truncated_buffer_flags_bad() -> None:
    r = Reader(b"\x01\x02")
    r.u32()
    assert r.bad


def test_wrong_tag_rejected_by_parse_package(tmp_path=None) -> None:
    import tempfile
    from mip_forensics import parse_package
    with tempfile.NamedTemporaryFile(suffix=".u", delete=False) as f:
        f.write(b"BAD!" + b"\0" * 64)
        name = f.name
    try:
        parse_package(Path(name))
        raise SystemExit("should have raised")
    except AssertionError as e:
        assert "UE3 package" in str(e)


if __name__ == "__main__":
    fns = [v for k, v in sorted(globals().items()) if k.startswith("test_")]
    fails = 0
    for fn in fns:
        try:
            if "tmp_path" in fn.__code__.co_varnames:
                import tempfile
                with tempfile.TemporaryDirectory() as td:
                    fn(Path(td))
            else:
                fn()
            print(f"PASS: {fn.__name__}")
        except (AssertionError, SystemExit) as e:
            fails += 1
            print(f"FAIL: {fn.__name__}: {e}")
    sys.exit(1 if fails else 0)
