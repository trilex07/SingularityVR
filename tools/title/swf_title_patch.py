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
    if len(data) < 14 or data[:3] not in (b"FWS", b"GFX"):
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
            if ln != 0 or o != len(data):
                raise ValueError("unexpected End tag payload or trailing bytes")
            break
    else:
        raise ValueError("missing End tag")
    return header, tags


def encode_tag_header(code: int, ln: int) -> bytes:
    if ln < 63:
        return struct.pack("<H", (code << 6) | ln)
    return struct.pack("<H", (code << 6) | 63) + struct.pack("<I", ln)


def _take_bits(data: bytes, bitpos: int, width: int, signed: bool = False) -> tuple[int, int]:
    value = 0
    for _ in range(width):
        value = (value << 1) | ((data[bitpos // 8] >> (7 - (bitpos % 8))) & 1)
        bitpos += 1
    if signed and width and value & (1 << (width - 1)):
        value -= 1 << width
    return value, bitpos


def read_rect(data: bytes, offset: int) -> tuple[int, list[int], int]:
    """SWF RECT. Returns nbits, [xmin, xmax, ymin, ymax], and the next byte offset."""
    nbits = data[offset] >> 3
    if not 1 <= nbits <= 31:
        raise ValueError("RECT bit width implausible")
    bitpos = offset * 8 + 5
    vals = []
    for _ in range(4):
        value, bitpos = _take_bits(data, bitpos, nbits, signed=True)
        vals.append(value)
    return nbits, vals, (bitpos + 7) // 8


def write_rect(vals: list[int], nbits: int | None = None) -> bytes:
    """Encode a RECT. Passing the original nbits keeps a round trip byte-identical."""
    need = max(2, max(abs(int(v)).bit_length() + 1 for v in vals))
    if nbits is None:
        nbits = need
    if nbits < need or not 1 <= nbits <= 31:
        raise ValueError("RECT values do not fit the requested width")
    bits = [(nbits >> shift) & 1 for shift in range(4, -1, -1)]
    for value in vals:
        raw = value if value >= 0 else (1 << nbits) + value
        bits.extend((raw >> shift) & 1 for shift in range(nbits - 1, -1, -1))
    bits.extend([0] * ((-len(bits)) % 8))
    return bytes(sum(bits[i + j] << (7 - j) for j in range(8)) for i in range(0, len(bits), 8))


def _edit_align(payload: bytes) -> tuple[list[int], int, int]:
    """Return bounds, rect-end, and align (0 left, 1 right, 2 centre). Refuse an unknown field."""
    _nbits, vals, end = read_rect(payload, 2)
    bitpos = end * 8
    flags = []
    for _ in range(16):
        flag, bitpos = _take_bits(payload, bitpos, 1)
        flags.append(flag)
    if bitpos % 8:
        raise ValueError("DefineEditText flags are not byte aligned")
    names = (
        "HasText", "WordWrap", "Multiline", "Password", "ReadOnly", "HasTextColor",
        "HasMaxLength", "HasFont", "HasFontClass", "AutoSize", "HasLayout", "NoSelect",
        "Border", "WasStatic", "HTML", "UseOutlines",
    )
    flag = dict(zip(names, flags))
    if flag["AutoSize"]:
        raise ValueError("auto-sized title field does not clip; refusing a bounds edit")
    offset = bitpos // 8
    if flag["HasFont"]:
        offset += 4
    if flag["HasFontClass"]:
        offset = payload.index(0, offset) + 1
    if flag["HasTextColor"]:
        offset += 4
    if flag["HasMaxLength"]:
        offset += 2
    if not flag["HasLayout"]:
        raise ValueError("title field has no layout align")
    align = payload[offset]
    if align not in (0, 1, 2, 3):
        raise ValueError("unknown title align")
    return vals, end, align


def fit_title_field(data: bytes, old: bytes, new: bytes, tag_code: int = TAG_DEFINE_EDIT_TEXT) -> tuple[bytes, dict]:
    """Replace one DefineEditText string and grow its box toward the free side.

    A right-aligned field keeps its right edge, because that is the anchor the
    original layout uses. Growing only the text and not the box clips the added
    glyphs, which is what a +3 byte replacement did to the 500px boot title.
    """
    if old not in new or len(new) <= len(old):
        raise ValueError("replacement must extend the original title")
    _, tags = split_tags(data)
    cands = [t for t in tags if t["code"] == tag_code
             and data[t["pay_off"]:t["pay_off"] + t["pay_len"]].count(old) == 1]
    if len(cands) != 1:
        raise ValueError("title field is not one exclusive DefineEditText")
    tag = cands[0]
    payload = data[tag["pay_off"]:tag["pay_off"] + tag["pay_len"]]
    vals, rect_end, align = _edit_align(payload)
    xmin, xmax, ymin, ymax = vals
    width = xmax - xmin
    if width <= 0:
        raise ValueError("empty title box")
    grow = width * (len(new) - len(old)) // len(old)
    if grow <= 0:
        raise ValueError("title growth rounded away")
    if align == 1:
        vals = [xmin - grow, xmax, ymin, ymax]
    elif align == 2:
        vals = [xmin - grow // 2, xmax + (grow - grow // 2), ymin, ymax]
    else:
        vals = [xmin, xmax + grow, ymin, ymax]
    rect = write_rect(vals)
    fitted = payload[:2] + rect + payload[rect_end:]
    if fitted.count(old) != 1:
        raise ValueError("title bytes moved outside the edited field")
    fitted = fitted.replace(old, new, 1)
    out = (data[:tag["tag_off"]] + encode_tag_header(tag["code"], len(fitted)) + fitted
           + data[tag["pay_off"] + tag["pay_len"]:])
    out = out[:4] + struct.pack("<I", len(out)) + out[8:]
    split_tags(out)
    if out.count(new) != 1:
        raise ValueError("fitted title is not exclusive")
    report = {
        "tag_code": tag["code"],
        "character": struct.unpack_from("<H", payload)[0],
        "align": align,
        "old_bounds": [xmin, xmax, ymin, ymax],
        "new_bounds": vals,
        "grow_twips": grow,
        "delta": len(out) - len(data),
    }
    return out, report


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
