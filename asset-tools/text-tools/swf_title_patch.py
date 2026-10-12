#!/usr/bin/env python3
"""Deterministic offline SWF/GFx title patcher (research prototype).

Replaces the initial-text bytes of a DefineEditText tag (tag 37) with new
bytes, then repairs every length that covers the edit:
  - the tag's own length field (including short<->long format transitions),
  - the file length field in the header.

Everything else in the file is byte-identical afterwards; tags after the edit
shift as a block, which is valid because SWF tags are parsed by walking.

Inputs and outputs are explicit paths. Game-derived movies must stay outside
the repository - this tool only ever reads them from, and writes them to,
caller-chosen scratch locations.

Usage:
    python swf_title_patch.py <in.swf> <out.swf> <OLD> <NEW>
"""

from __future__ import annotations

import struct
import sys
from pathlib import Path

TAG_DEFINE_EDIT_TEXT = 37


def split_tags(data: bytes) -> tuple[dict, list]:
    """Parse header + tag table. Returns (header, [(code, tag_off, pay_off, pay_len)])."""
    if data[:3] not in (b"FWS", b"CWS", b"GFX"):
        raise ValueError("not an uncompressed SWF/GFx movie")
    if struct.unpack_from("<I", data, 4)[0] != len(data):
        raise ValueError("file length field does not match actual size")
    o = 8
    nbits = data[o] >> 3
    if not 1 <= nbits <= 31:
        raise ValueError("RECT bit width implausible")
    o += (5 + 4 * nbits + 7) // 8
    fps, frames = struct.unpack_from("<HH", data, o)
    o += 4
    header = {"fps": fps / 256, "frames": frames, "body": o}
    tags = []
    while o < len(data):
        tag_off = o
        head = struct.unpack_from("<H", data, o)[0]
        o += 2
        code, ln = head >> 6, head & 63
        hlen = 2
        if ln == 63:
            ln = struct.unpack_from("<I", data, o)[0]
            o += 4
            hlen = 6
        if o + ln > len(data):
            raise ValueError(f"tag {code} overruns file at 0x{tag_off:X}")
        tags.append({"code": code, "tag_off": tag_off, "pay_off": o,
                     "pay_len": ln, "hlen": hlen})
        o += ln
        if code == 0:
            break
    else:
        raise ValueError("missing End tag")
    return header, tags


def encode_tag_header(code: int, ln: int) -> bytes:
    if ln < 63:
        return struct.pack("<H", (code << 6) | ln)
    return struct.pack("<H", (code << 6) | 63) + struct.pack("<I", ln)


def patch_first(data: bytes, old: bytes, new: bytes, tag_code: int = TAG_DEFINE_EDIT_TEXT) -> tuple[bytes, dict]:
    """Replace the first occurrence of old inside a tag_code payload.

    Returns (patched_bytes, report). Raises ValueError if old is absent,
    spans tags, or appears more than once inside the chosen tag.
    """
    _, tags = split_tags(data)
    cands = [t for t in tags if t["code"] == tag_code
             and data[t["pay_off"]:t["pay_off"] + t["pay_len"]].count(old) > 0]
    if not cands:
        raise ValueError("title bytes not found in any DefineEditText tag")
    t = cands[0]
    payload = data[t["pay_off"]:t["pay_off"] + t["pay_len"]]
    if payload.count(old) != 1:
        raise ValueError("title bytes occur more than once in the tag - refusing ambiguous patch")
    new_payload = payload.replace(old, new, 1)
    new_head = encode_tag_header(t["code"], len(new_payload))
    out = (data[:t["tag_off"]] + new_head + new_payload
           + data[t["pay_off"] + t["pay_len"]:])
    out = out[:4] + struct.pack("<I", len(out)) + out[8:]
    report = {
        "tag_code": t["code"],
        "tag_off": f"0x{t['tag_off']:X}",
        "old_len": t["pay_len"],
        "new_len": len(new_payload),
        "delta": len(new_payload) - t["pay_len"],
        "old_file_len": len(data),
        "new_file_len": len(out),
    }
    # self-verification: the result must re-parse cleanly
    split_tags(out)
    # every remaining occurrence of old must be part of the replacement
    # (when old is a substring of new, e.g. "SINGULARITY" inside
    # "SINGULARITY VR", their counts must agree; otherwise old must be gone)
    if old in new:
        consistent = out.count(old) == out.count(new)
    else:
        consistent = out.count(old) == 0
    if not consistent:
        raise ValueError("unreplaced old bytes remain after patch")
    if out.count(new) != 1:
        raise ValueError("new bytes do not occur exactly once after patch")
    return out, report


def main() -> None:
    if len(sys.argv) != 5:
        print(__doc__)
        sys.exit(2)
    src, dst, old, new = (Path(sys.argv[1]), Path(sys.argv[2]),
                          sys.argv[3].encode("latin1"), sys.argv[4].encode("latin1"))
    out, report = patch_first(src.read_bytes(), old, new)
    dst.write_bytes(out)
    for k, v in report.items():
        print(f"{k}: {v}")


if __name__ == "__main__":
    main()
