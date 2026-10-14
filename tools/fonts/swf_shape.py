"""SWF SHAPE records used by DefineFont3 glyphs.

Bit order follows the SWF specification: after a non-edge type flag the five
state flags are NewStyles, LineStyle, FillStyle1, FillStyle0, MoveTo.
"""
from __future__ import annotations


class BitReader:
    def __init__(self, data: bytes):
        self.data = data
        self.bit = 0

    def ub(self, n: int) -> int:
        value = 0
        for _ in range(n):
            byte = self.data[self.bit >> 3]
            value = (value << 1) | ((byte >> (7 - (self.bit & 7))) & 1)
            self.bit += 1
        return value

    def sb(self, n: int) -> int:
        value = self.ub(n)
        sign = 1 << (n - 1)
        return value - (1 << n) if value & sign else value


class BitWriter:
    def __init__(self):
        self.bits = []

    def ub(self, n: int, value: int) -> None:
        if value < 0 or value >= (1 << n):
            raise ValueError(f"unsigned {value} does not fit in {n} bits")
        for shift in range(n - 1, -1, -1):
            self.bits.append((value >> shift) & 1)

    def sb(self, n: int, value: int) -> None:
        limit = 1 << (n - 1)
        if value < -limit or value >= limit:
            raise ValueError(f"signed {value} does not fit in {n} bits")
        self.ub(n, value & ((1 << n) - 1))

    def bytes(self) -> bytes:
        out = bytearray()
        acc = 0
        n = 0
        for bit in self.bits:
            acc = (acc << 1) | bit
            n += 1
            if n == 8:
                out.append(acc)
                acc = 0
                n = 0
        if n:
            out.append(acc << (8 - n))
        return bytes(out)


def _read_record(bits: BitReader, nfill: int, nline: int):
    if bits.ub(1) == 0:
        flags = bits.ub(5)
        if flags == 0:
            return ("end",), nfill, nline
        rec = {"type": "style", "flags": flags}
        if flags & 1:
            move_bits = bits.ub(5)
            rec["move"] = (move_bits, bits.sb(move_bits), bits.sb(move_bits))
        if flags & 2:
            rec["fill0"] = bits.ub(nfill) if nfill else 0
        if flags & 4:
            rec["fill1"] = bits.ub(nfill) if nfill else 0
        if flags & 8:
            rec["line"] = bits.ub(nline) if nline else 0
        if flags & 16:
            raise ValueError("DefineFont glyphs must not contain NewStyles")
        return ("style", rec), nfill, nline
    if bits.ub(1):
        nbits = bits.ub(4) + 2
        if bits.ub(1):
            return ("line", (nbits, bits.sb(nbits), bits.sb(nbits))), nfill, nline
        vertical = bits.ub(1)
        delta = bits.sb(nbits)
        return ("hline" if vertical == 0 else "vline", (nbits, delta)), nfill, nline
    nbits = bits.ub(4) + 2
    return (
        "curve",
        (nbits, bits.sb(nbits), bits.sb(nbits), bits.sb(nbits), bits.sb(nbits)),
    ), nfill, nline


def decode_shape(data: bytes):
    bits = BitReader(data)
    nfill = bits.ub(4)
    nline = bits.ub(4)
    records = []
    while True:
        kind, nfill, nline = _read_record(bits, nfill, nline)
        records.append(kind)
        if kind[0] == "end":
            break
    return {"nfill": nfill, "nline": nline, "records": records, "end_bit": bits.bit}


def _write_record(out: BitWriter, kind, nfill: int, nline: int) -> None:
    name = kind[0]
    if name == "end":
        out.ub(6, 0)
        return
    if name == "style":
        rec = kind[1]
        out.ub(1, 0)
        out.ub(5, rec["flags"])
        if rec["flags"] & 1:
            move_bits, x, y = rec["move"]
            out.ub(5, move_bits)
            out.sb(move_bits, x)
            out.sb(move_bits, y)
        if rec["flags"] & 2:
            out.ub(nfill, rec["fill0"])
        if rec["flags"] & 4:
            out.ub(nfill, rec["fill1"])
        if rec["flags"] & 8:
            out.ub(nline, rec["line"])
        return
    out.ub(1, 1)
    if name == "curve":
        nbits, cx, cy, ax, ay = kind[1]
        out.ub(1, 0)
        out.ub(4, nbits - 2)
        out.sb(nbits, cx)
        out.sb(nbits, cy)
        out.sb(nbits, ax)
        out.sb(nbits, ay)
        return
    out.ub(1, 1)
    if name == "line":
        nbits, dx, dy = kind[1]
        out.ub(4, nbits - 2)
        out.ub(1, 1)
        out.sb(nbits, dx)
        out.sb(nbits, dy)
        return
    nbits, delta = kind[1]
    out.ub(4, nbits - 2)
    out.ub(1, 0)
    out.ub(1, 0 if name == "hline" else 1)
    out.sb(nbits, delta)


def encode_shape(shape: dict) -> bytes:
    out = BitWriter()
    out.ub(4, shape["nfill"])
    out.ub(4, shape["nline"])
    for record in shape["records"]:
        _write_record(out, record, shape["nfill"], shape["nline"])
    return out.bytes()


def signed_width(*values: int) -> int:
    peak = 0
    for value in values:
        peak = max(peak, abs(int(value)))
    bits = 2
    while peak >= (1 << (bits - 1)):
        bits += 1
        if bits > 30:
            raise ValueError(f"coordinate does not fit in a SWF shape: {values}")
    return bits


def split_contours(shape: dict) -> list:
    groups = []
    current = []
    for record in shape["records"]:
        if record[0] == "end":
            if current:
                groups.append(current)
            break
        if record[0] == "style" and record[1]["flags"] & 1 and current:
            groups.append(current)
            current = []
        current.append(record)
    return groups


def contour_bbox(records) -> tuple:
    x = y = 0
    minx = miny = 1 << 30
    maxx = maxy = -(1 << 30)
    seen = False
    for record in records:
        name = record[0]
        if name == "style" and record[1]["flags"] & 1:
            _, x, y = record[1]["move"]
        elif name == "line":
            _, dx, dy = record[1]
            x += dx
            y += dy
        elif name == "hline":
            x += record[1][1]
        elif name == "vline":
            y += record[1][1]
        elif name == "curve":
            _, cx, cy, ax, ay = record[1]
            x += cx
            y += cy
            minx, miny = min(minx, x), min(miny, y)
            maxx, maxy = max(maxx, x), max(maxy, y)
            x += ax
            y += ay
        else:
            continue
        seen = True
        minx, miny = min(minx, x), min(miny, y)
        maxx, maxy = max(maxx, x), max(maxy, y)
    if not seen:
        return (0, 0, 0, 0)
    return (minx, miny, maxx, maxy)


def flatten_points(records, steps: int = 8):
    x = y = 0
    points = []
    for record in records:
        name = record[0]
        if name == "style" and record[1]["flags"] & 1:
            _, x, y = record[1]["move"]
            points.append((x, y))
        elif name == "line":
            _, dx, dy = record[1]
            x += dx
            y += dy
            points.append((x, y))
        elif name == "hline":
            x += record[1][1]
            points.append((x, y))
        elif name == "vline":
            y += record[1][1]
            points.append((x, y))
        elif name == "curve":
            _, cx, cy, ax, ay = record[1]
            x0, y0 = x, y
            x1, y1 = x + cx, y + cy
            x2, y2 = x1 + ax, y1 + ay
            for step in range(1, steps + 1):
                t = step / steps
                u = 1 - t
                points.append(
                    (
                        u * u * x0 + 2 * u * t * x1 + t * t * x2,
                        u * u * y0 + 2 * u * t * y1 + t * t * y2,
                    )
                )
            x, y = x2, y2
    return points


def shape_bounds(shape: dict):
    x = y = 0
    minx = miny = 1 << 30
    maxx = maxy = -(1 << 30)
    seen = False
    for record in shape["records"]:
        name = record[0]
        if name == "end":
            continue
        payload = record[1]
        if name == "style" and payload["flags"] & 1:
            _, x, y = payload["move"]
        elif name == "line":
            _, dx, dy = payload
            x += dx
            y += dy
        elif name == "hline":
            _, delta = payload
            x += delta
        elif name == "vline":
            _, delta = payload
            y += delta
        elif name == "curve":
            _, cx, cy, ax, ay = payload
            x += cx
            y += cy
            if seen or True:
                minx = min(minx, x)
                miny = min(miny, y)
                maxx = max(maxx, x)
                maxy = max(maxy, y)
            x += ax
            y += ay
        else:
            continue
        seen = True
        minx = min(minx, x)
        miny = min(miny, y)
        maxx = max(maxx, x)
        maxy = max(maxy, y)
    if not seen:
        return (0, 0, 0, 0)
    return (minx, miny, maxx, maxy)
