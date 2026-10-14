"""Compose the missing Polish outlines inside TypekaMix (WFontNote).

TypekaMix already maps ĄĆĘŁŃÓŚŹŻąćęłńóśźż to glyph slots. Ó and ó are real
outlines. The other sixteen slots are shared stubs or empty shapes, so a
code-page conversion cannot make them visible. This module keeps every other
glyph, the code table, the zeroed bounds table and the kerning table, and
replaces only those sixteen shapes.

Marks are copied from outlines already in the same font: the acute from Á/á,
the hook under Ç/ç, the period, and the hyphen. Nothing from another typeface
is embedded.
"""
from __future__ import annotations

import struct

from swf_shape import (
    contour_bbox,
    decode_shape,
    encode_shape,
    flatten_points,
    signed_width,
    split_contours,
)

# Base letter, then the mark copied from this font.
SPECS = {
    "Ą": ("A", "ogonek"),
    "Ć": ("C", "acute"),
    "Ę": ("E", "ogonek"),
    "Ł": ("L", "stroke"),
    "Ń": ("N", "acute"),
    "Ś": ("S", "acute"),
    "Ź": ("Z", "acute"),
    "Ż": ("Z", "dot"),
    "ą": ("a", "ogonek"),
    "ć": ("c", "acute"),
    "ę": ("e", "ogonek"),
    "ł": ("l", "stroke"),
    "ń": ("n", "acute"),
    "ś": ("s", "acute"),
    "ź": ("z", "acute"),
    "ż": ("z", "dot"),
}
KEEP_REAL = "Óó"
STUB_MAX_BYTES = 80
# DefineFont3 coordinates are DefineFont2 units times 20. Ascent in this
# font is 18600, under the 1024*20 em square. Marks must stay inside it.
EM_SQUARE = 20480
ASCENT_LIMIT = -18200
DESCENT_LIMIT = 5200


def parse_define_font3(payload: bytes) -> dict:
    if len(payload) < 8:
        raise ValueError("DefineFont3 payload is too short")
    font_id, flags, lang = struct.unpack_from("<HBB", payload, 0)
    if not flags & 0x80:
        raise ValueError("TypekaMix layout block is required")
    if not flags & 0x08 or not flags & 0x04:
        raise ValueError("TypekaMix wide offsets and wide codes are required")
    name_len = payload[4]
    name = payload[5 : 5 + name_len].split(b"\x00")[0].decode("latin1")
    offset = 5 + name_len
    count = struct.unpack_from("<H", payload, offset)[0]
    offset += 2
    table = offset
    if struct.unpack_from("<I", payload, table)[0] != (count + 1) * 4:
        raise ValueError("glyph data does not start immediately after the offset table")
    offsets = list(struct.unpack_from("<" + "I" * (count + 1), payload, table))
    shapes = [
        payload[table + offsets[i] : table + offsets[i + 1]] for i in range(count)
    ]
    code_at = table + offsets[-1]
    codes = list(struct.unpack_from("<" + "H" * count, payload, code_at))
    layout = code_at + 2 * count
    ascent, descent, leading = struct.unpack_from("<hhh", payload, layout)
    advance_at = layout + 6
    advances = list(struct.unpack_from("<" + "h" * count, payload, advance_at))
    tail = payload[advance_at + 2 * count :]
    return {
        "font_id": font_id,
        "flags": flags,
        "lang": lang,
        "name": name,
        "prefix": payload[:table],
        "codes": codes,
        "shapes": shapes,
        "code_bytes": payload[code_at:layout],
        "metrics": payload[layout:advance_at],
        "ascent": ascent,
        "descent": descent,
        "leading": leading,
        "advances": advances,
        "tail": tail,
    }


def rebuild_define_font3(font: dict, shapes: list[bytes], advances: list[int]) -> bytes:
    count = len(font["codes"])
    if len(shapes) != count or len(advances) != count:
        raise ValueError("glyph count changed")
    relative = [(count + 1) * 4]
    for shape in shapes:
        relative.append(relative[-1] + len(shape))
    offset_bytes = b"".join(struct.pack("<I", value) for value in relative)
    advance_bytes = b"".join(struct.pack("<h", value) for value in advances)
    return (
        font["prefix"]
        + offset_bytes
        + b"".join(shapes)
        + font["code_bytes"]
        + font["metrics"]
        + advance_bytes
        + font["tail"]
    )


def _index(font: dict, char: str) -> int:
    try:
        return font["codes"].index(ord(char))
    except ValueError as exc:
        raise ValueError(f"TypekaMix has no slot for {char}") from exc


def _decoded(font: dict, char: str) -> dict:
    return decode_shape(font["shapes"][_index(font, char)])


def _map_contour(records, fn) -> list:
    source_x = source_y = 0
    dest_x = dest_y = 0
    started = False
    output = []
    for record in records:
        name = record[0]
        if name == "style":
            info = dict(record[1])
            if info["flags"] & 1:
                _, x, y = info["move"]
                dest_x, dest_y = fn(x, y)
                info["move"] = (signed_width(dest_x, dest_y), dest_x, dest_y)
                source_x, source_y = x, y
                started = True
            output.append(("style", info))
            continue
        if not started:
            raise ValueError("shape edge appears before its move")
        if name == "line":
            _, dx, dy = record[1]
            source_x += dx
            source_y += dy
            next_x, next_y = fn(source_x, source_y)
            edge_x, edge_y = next_x - dest_x, next_y - dest_y
            output.append(("line", (signed_width(edge_x, edge_y), edge_x, edge_y)))
        elif name == "hline":
            source_x += record[1][1]
            next_x, next_y = fn(source_x, source_y)
            edge_x, edge_y = next_x - dest_x, next_y - dest_y
            if edge_y == 0:
                output.append(("hline", (signed_width(edge_x), edge_x)))
            else:
                output.append(("line", (signed_width(edge_x, edge_y), edge_x, edge_y)))
        elif name == "vline":
            source_y += record[1][1]
            next_x, next_y = fn(source_x, source_y)
            edge_x, edge_y = next_x - dest_x, next_y - dest_y
            if edge_x == 0:
                output.append(("vline", (signed_width(edge_y), edge_y)))
            else:
                output.append(("line", (signed_width(edge_x, edge_y), edge_x, edge_y)))
        elif name == "curve":
            _, cx, cy, ax, ay = record[1]
            control_x, control_y = fn(source_x + cx, source_y + cy)
            anchor_x, anchor_y = fn(source_x + cx + ax, source_y + cy + ay)
            cdx, cdy = control_x - dest_x, control_y - dest_y
            adx, ady = anchor_x - control_x, anchor_y - control_y
            width = signed_width(cdx, cdy, adx, ady)
            output.append(("curve", (width, cdx, cdy, adx, ady)))
            source_x += cx + ax
            source_y += cy + ay
            next_x, next_y = anchor_x, anchor_y
        else:
            raise ValueError(f"unknown shape record {name}")
        dest_x, dest_y = next_x, next_y
    return output


def _translate(records, dx: int, dy: int) -> list:
    return _map_contour(records, lambda x, y: (int(x) + dx, int(y) + dy))


def _fit_box(records, target) -> list:
    left, top, right, bottom = contour_bbox(records)
    width = right - left
    height = bottom - top
    target_width = target[2] - target[0]
    target_height = target[3] - target[1]
    if width <= 0 or height <= 0 or target_width <= 0 or target_height <= 0:
        raise ValueError("cannot fit an empty contour")

    def fn(x, y):
        return (
            int(round(target[0] + (x - left) * target_width / width)),
            int(round(target[1] + (y - top) * target_height / height)),
        )

    return _map_contour(records, fn)


def _highest(contours):
    return min(contours, key=lambda item: contour_bbox(item)[1])


def _lowest_mark(contours):
    return max(contours, key=lambda item: contour_bbox(item)[1])


def _require_inside(records) -> None:
    left, top, right, bottom = contour_bbox(records)
    if top < ASCENT_LIMIT or bottom > DESCENT_LIMIT:
        raise ValueError(f"mark falls outside the em box: {(left, top, right, bottom)}")


def _place_acute(base_records, donor_letter, donor_mark) -> list:
    gap = contour_bbox(donor_mark)[3] - contour_bbox(donor_letter)[1]
    base = contour_bbox(base_records)
    mark = contour_bbox(donor_mark)
    dx = int(round(((base[0] + base[2]) - (mark[0] + mark[2])) / 2))
    dy = int(round((base[1] + gap) - mark[3]))
    placed = _translate(donor_mark, dx, dy)
    _require_inside(placed)
    return placed


def _flip_horizontal(records) -> list:
    """Mirror a contour. Reflection swaps fill sides, so FillStyle0 moves to FillStyle1."""
    box = contour_bbox(records)
    pivot = box[0] + box[2]

    def fn(x, y):
        return (int(round(pivot - x)), int(y))

    flipped = []
    for record in _map_contour(records, fn):
        if record[0] != "style":
            flipped.append(record)
            continue
        info = dict(record[1])
        if info["flags"] & 2:
            info["flags"] = (info["flags"] & ~2) | 4
            info["fill1"] = info.pop("fill0")
        flipped.append(("style", info))
    return flipped


def _place_ogonek(base_records, donor_mark) -> list:
    base = contour_bbox(base_records)
    # Ç/ç own this hook, but it curls the other way. Mirror it, then hang it
    # from the right side so the Polish ogonek matches the font's stroke.
    mark = _flip_horizontal(donor_mark)
    mark_box = contour_bbox(mark)
    inset = (base[2] - base[0]) * 0.02
    right = base[2] - inset
    dx = int(round(right - mark_box[2]))
    placed = _translate(mark, dx, 0)
    _require_inside(placed)
    return placed


def _place_dot(base_records, donor_mark) -> list:
    base = contour_bbox(base_records)
    height = base[3] - base[1]
    diameter = max(1800, min(3200, int(round(height * 0.22))))
    center = (base[0] + base[2]) / 2
    bottom = base[1] - 500
    top = bottom - diameter
    if top < ASCENT_LIMIT:
        top = ASCENT_LIMIT
        diameter = bottom - top
    placed = _fit_box(
        donor_mark,
        (center - diameter / 2, top, center + diameter / 2, bottom),
    )
    _require_inside(placed)
    return placed


def _place_stroke(base_shape, donor_mark) -> list:
    points = []
    for contour in split_contours(base_shape):
        points.extend(flatten_points(contour))
    ys = [point[1] for point in points]
    top, bottom = min(ys), max(ys)
    span = bottom - top
    band_top = top + 0.34 * span
    band_bottom = top + 0.50 * span
    xs = [point[0] for point in points if band_top <= point[1] <= band_bottom]
    if len(xs) < 4:
        raise ValueError("vertical stem was not found")
    stem_left, stem_right = min(xs), max(xs)
    stem = max(1.0, stem_right - stem_left)
    thickness = max(1600.0, stem * 0.9)
    center_y = (band_top + band_bottom) / 2
    placed = _fit_box(
        donor_mark,
        (
            stem_left - stem * 1.15,
            center_y - thickness / 2,
            stem_right + stem * 1.45,
            center_y + thickness / 2,
        ),
    )
    _require_inside(placed)
    return placed


def compose_polish(font: dict) -> tuple[list[bytes], list[int], dict]:
    """Return replacement shapes, advances, and a report. Other slots are copied."""
    by_char = {chr(code): index for index, code in enumerate(font["codes"]) if code >= 32}
    for char in list(SPECS) + list(KEEP_REAL):
        if char not in by_char:
            raise ValueError(f"cmap is missing {char}")
    shapes = list(font["shapes"])
    advances = list(font["advances"])
    acute = {
        "upper": _highest(split_contours(_decoded(font, "Á"))),
        "lower": _highest(split_contours(_decoded(font, "á"))),
    }
    acute_letter = {
        "upper": [record for contour in split_contours(_decoded(font, "A")) for record in contour],
        "lower": [record for contour in split_contours(_decoded(font, "a")) for record in contour],
    }
    ogonek = {
        "upper": _lowest_mark(split_contours(_decoded(font, "Ç"))),
        "lower": _lowest_mark(split_contours(_decoded(font, "ç"))),
    }
    period = split_contours(_decoded(font, "."))
    hyphen = split_contours(_decoded(font, "-"))
    if len(period) != 1 or len(hyphen) != 1:
        raise ValueError("period or hyphen contour count changed")
    report = {"replaced": {}, "kept_real": list(KEEP_REAL)}
    for char, (base, kind) in SPECS.items():
        slot = by_char[char]
        if len(shapes[slot]) > STUB_MAX_BYTES:
            raise ValueError(f"{char} is no longer a stub; refusing to overwrite a real outline")
        case = "upper" if char.isupper() else "lower"
        base_shape = _decoded(font, base)
        base_records = [record for contour in split_contours(base_shape) for record in contour]
        if kind == "acute":
            mark = _place_acute(base_records, acute_letter[case], acute[case])
        elif kind == "ogonek":
            mark = _place_ogonek(base_records, ogonek[case])
        elif kind == "dot":
            mark = _place_dot(base_records, period[0])
        elif kind == "stroke":
            mark = _place_stroke(base_shape, hyphen[0])
        else:
            raise ValueError(kind)
        composed = {
            "nfill": base_shape["nfill"],
            "nline": base_shape["nline"],
            "records": base_records + mark + [("end",)],
        }
        encoded = encode_shape(composed)
        if encode_shape(decode_shape(encoded)) != encoded:
            raise ValueError(f"composed {char} does not round-trip")
        shapes[slot] = encoded
        advances[slot] = advances[by_char[base]]
        report["replaced"][char] = {
            "base": base,
            "mark": kind,
            "bytes": len(encoded),
            "advance": advances[slot],
        }
    for char in KEEP_REAL:
        slot = by_char[char]
        if len(shapes[slot]) <= STUB_MAX_BYTES:
            raise ValueError(f"{char} was expected to already have a real outline")
    return shapes, advances, report


def patch_typeka_payload(payload: bytes) -> tuple[bytes, dict]:
    font = parse_define_font3(payload)
    if font["name"] != "TypekaMix":
        raise ValueError(f"unexpected font {font['name']}")
    if rebuild_define_font3(font, font["shapes"], font["advances"]) != payload:
        raise ValueError("identity rebuild of TypekaMix changed bytes")
    shapes, advances, report = compose_polish(font)
    patched = rebuild_define_font3(font, shapes, advances)
    report["ascent"] = font["ascent"]
    report["descent"] = font["descent"]
    report["leading"] = font["leading"]
    report["tail_bytes"] = len(font["tail"])
    report["delta"] = len(patched) - len(payload)
    return patched, report
