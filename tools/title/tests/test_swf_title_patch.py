#!/usr/bin/env python3
"""Tests for swf_title_patch.py.

All fixtures are synthetic SWF files built in code - no game bytes anywhere.
Covers: short tags, long-format tags, short<->long transitions both ways,
multi-tag files (later tags shift intact), missing/ambiguous titles rejected.
"""

from __future__ import annotations

import struct
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
from swf_title_patch import encode_tag_header, patch_first, split_tags  # noqa: E402


def make_movie(tags: list[tuple[int, bytes]]) -> bytes:
    """Minimal uncompressed SWF: header + tags + End. 1x1 stage, 1 frame."""
    body = b"".join(encode_tag_header(code, len(p)) + p for code, p in tags)
    body += encode_tag_header(0, 0)
    rect = bytes([0b00001000])  # nbits=1, coords all zero -> 2 bytes total
    rect += b"\x00"
    head = b"GFX" + b"\x09" + b"\x00\x00\x00\x00" + rect + struct.pack("<HH", 24 * 256, 1)
    out = head + body
    return out[:4] + struct.pack("<I", len(out)) + out[8:]


def edittext_payload(text: bytes, char_id: int = 7) -> bytes:
    """Bare-bones DefineEditText payload holding initial text (flags=HasText)."""
    return (struct.pack("<H", char_id) + bytes([0x08, 0x00])  # 1x1 RECT zeroed-ish
            + struct.pack("<H", 0x0001) + b"\x00" + text + b"\x00")


def test_short_tag_patch() -> None:
    src = make_movie([(37, edittext_payload(b"<b>SINGULARITY</b>"))])
    out, rep = patch_first(src, b"SINGULARITY", b"SINGULARITY VR")
    assert rep["delta"] == 3
    assert out.count(b"SINGULARITY VR") == 1 and out.count(b"SINGULARITY</b>") == 0
    _, tags = split_tags(out)
    assert sum(1 for t in tags if t["code"] == 37) == 1


def test_long_tag_stays_long() -> None:
    big = b"<b>SINGULARITY</b>" + b"." * 200
    src = make_movie([(37, edittext_payload(big))])
    out, rep = patch_first(src, b"SINGULARITY", b"SINGULARITY VR")
    assert rep["new_len"] >= 63
    _, tags = split_tags(out)
    assert tags[0]["code"] == 37


def test_short_to_long_transition() -> None:
    base = b"<b>SINGULARITY</b>"  # payload lands just under 63
    src = make_movie([(37, edittext_payload(base))])
    _, tags0 = split_tags(src)
    assert tags0[0]["pay_len"] < 63
    out, rep = patch_first(src, b"SINGULARITY", b"SINGULARITY VR" + b"." * 60)
    _, tags = split_tags(out)
    assert tags[0]["pay_len"] >= 63
    assert out.count(b"SINGULARITY VR" + b"." * 60) == 1


def test_long_to_short_transition() -> None:
    big = b"SINGULARITY" + b"." * 100
    src = make_movie([(37, edittext_payload(big))])
    _, tags0 = split_tags(src)
    assert tags0[0]["pay_len"] >= 63
    out, _ = patch_first(src, b"SINGULARITY" + b"." * 100, b"QzQx")
    _, tags = split_tags(out)
    assert tags[0]["pay_len"] < 63
    assert out.count(b"QzQx") == 1


def test_later_tags_shift_intact() -> None:
    marker = b"DO-NOT-TOUCH-1234"
    src = make_movie([(37, edittext_payload(b"SINGULARITY")), (12, marker)])
    out, rep = patch_first(src, b"SINGULARITY", b"SINGULARITY VR")
    assert rep["delta"] == 3
    _, tags = split_tags(out)
    doaction = [t for t in tags if t["code"] == 12][0]
    assert out[doaction["pay_off"]:doaction["pay_off"] + doaction["pay_len"]] == marker


def test_missing_title_rejected() -> None:
    src = make_movie([(37, edittext_payload(b"nothing here"))])
    try:
        patch_first(src, b"SINGULARITY", b"SINGULARITY VR")
    except ValueError:
        return
    raise SystemExit("should have raised")


def test_ambiguous_title_rejected() -> None:
    src = make_movie([(37, edittext_payload(b"SINGULARITY and SINGULARITY"))])
    try:
        patch_first(src, b"SINGULARITY", b"SINGULARITY VR")
    except ValueError:
        return
    raise SystemExit("should have raised")


def test_wrong_tag_code_ignored() -> None:
    src = make_movie([(12, b"SINGULARITY in action code")])
    try:
        patch_first(src, b"SINGULARITY", b"SINGULARITY VR")
    except ValueError:
        return
    raise SystemExit("should have raised")


if __name__ == "__main__":
    fns = [v for k, v in sorted(globals().items()) if k.startswith("test_")]
    fails = 0
    for fn in fns:
        try:
            fn()
            print(f"PASS: {fn.__name__}")
        except (AssertionError, SystemExit, ValueError) as e:
            fails += 1
            print(f"FAIL: {fn.__name__}: {e}")
    sys.exit(1 if fails else 0)
