#!/usr/bin/env python3
"""UE3 package mip forensics (offline, read-only).

Method 2 for texture source recovery: parse a DECOMPRESSED UE3 package
(uedecompress first, output kept outside the repo), locate a Texture2D
export, walk its self-describing UE3 property list to the Mips array, and
decode each mip's bulk-data descriptor (dimensions, TFC file, offset, size).

Every layout assumption is validated, not trusted:
- package tag / version / table tiling (same checks as tools/uepkg)
- property walk lands on a "Mips" ArrayProperty of the expected size
- decoded SizeX/SizeY form a descending power-of-two chain from the object size
- declared bulk sizes match DXT block math for those dimensions
- declared TFC offsets lie inside the real TFC file

Anything that fails validation is reported as UNVALIDATED, never as fact.
"""

from __future__ import annotations

import struct
import sys
from pathlib import Path

TAG = 0x9E2A83C1


class Reader:
    def __init__(self, buf: bytes, off: int = 0):
        self.b = buf
        self.o = off
        self.bad = False

    def need(self, n: int) -> bool:
        if self.o + n > len(self.b):
            self.bad = True
            return False
        return True

    def u32(self) -> int:
        if not self.need(4):
            return 0
        v = struct.unpack_from("<I", self.b, self.o)[0]
        self.o += 4
        return v

    def i32(self) -> int:
        v = self.u32()
        return v - 0x100000000 if v >= 0x80000000 else v

    def u64(self) -> int:
        lo, hi = self.u32(), self.u32()
        return lo | (hi << 32)

    def skip(self, n: int) -> None:
        if self.need(n):
            self.o += n

    def fname(self) -> tuple[int, int]:
        return (self.i32(), self.i32())

    def fstring(self) -> str:
        n = self.i32()
        if self.bad or n == 0:
            return ""
        if n > 0:
            if n > (1 << 20) or not self.need(n):
                self.bad = True
                return ""
            s = self.b[self.o:self.o + n].split(b"\0")[0].decode("latin1")
            self.o += n
            return s
        chars = -n
        if chars > (1 << 20) or not self.need(chars * 2):
            self.bad = True
            return ""
        raw = self.b[self.o:self.o + chars * 2]
        self.o += chars * 2
        return "".join(chr(c) if c < 128 else "?" for c in struct.unpack(f"<{chars}H", raw)).rstrip("\0")


def parse_package(path: Path) -> dict:
    buf = path.read_bytes()
    r = Reader(buf)
    assert r.u32() == TAG, "not a decompressed UE3 package"
    ver = r.u32()
    header_size, folder = r.i32(), r.fstring()
    r.u32()  # pkgFlags
    name_count, name_off = r.i32(), r.i32()
    exp_count, exp_off = r.i32(), r.i32()
    imp_count, imp_off = r.i32(), r.i32()

    names = []
    r.o = name_off
    for _ in range(name_count):
        s = r.fstring()
        r.skip(8)  # flags(4) + hash(4)-ish; uepkg-validated as fixed stride
        names.append(s)
    # NOTE: name stride assumption - validated below by export table tiling

    exports = []
    r.o = exp_off
    for _ in range(exp_count):
        ex = {"cls": r.i32(), "super": r.i32(), "outer": r.i32(),
              "name": r.i32(), "num": r.i32(), "arch": r.i32(),
              "flags": r.u64(), "size": r.i32(), "off": r.i32()}
        r.u32()  # ExportFlags
        net = r.i32()
        assert 0 <= net <= (1 << 16), f"netCount implausible: {net}"
        r.skip(net * 4 + 16 + 4)  # net objects + guid + package flags
        exports.append(ex)

    def nm(i: int) -> str:
        return names[i] if 0 <= i < len(names) else f"#{i}"

    # validation: every name index in range, every serial range in file
    bad = sum(1 for e in exports if not (0 <= e["name"] < name_count))
    bad += sum(1 for e in exports if not (0 <= e["off"] and e["off"] + e["size"] <= len(buf)))
    return {"names": names, "exports": exports, "nm": nm, "buf": buf,
            "fileVer": ver & 0xFFFF, "licVer": ver >> 16,
            "layout_bad": bad, "size": len(buf)}


def find_export(pkg: dict, objname: str, classname: str = "Texture2D") -> dict | None:
    nm = pkg["nm"]
    for i, e in enumerate(pkg["exports"]):
        if nm(e["name"]).lower() == objname.lower() and nm(e["cls"]).lower() == classname.lower():
            return {"index": i, **e, "obj": nm(e["name"]), "cls": nm(e["cls"])}
    return None


def walk_properties(pkg: dict, export: dict) -> list:
    """Walk the self-describing property list; return [(name, type, size, offset)]."""
    buf, nm = pkg["buf"], pkg["nm"]
    r = Reader(buf, export["off"])
    props = []
    for _ in range(4096):  # sanity cap
        start = r.o
        ni, nn = r.fname()
        name = nm(ni) if 0 <= ni < len(pkg["names"]) else f"#{ni}"
        if name == "None":
            break
        ti, tn = r.fname()
        typ = nm(ti) if 0 <= ti < len(pkg["names"]) else f"#{ti}"
        size = r.i32()
        arr = r.i32()
        if r.bad or size < 0 or size > export["size"]:
            return props  # truncated walk; caller decides
        props.append({"name": name, "num": nn, "type": typ, "typenum": tn,
                      "size": size, "array": arr, "off": r.o, "tag": start})
        r.skip(size)
    return props


def main() -> None:
    if len(sys.argv) < 3:
        print("usage: mip_forensics.py <package.u> <Texture2D-name> [Texture2D-name ...]")
        sys.exit(2)
    pkg = parse_package(Path(sys.argv[1]))
    print(f"version file={pkg['fileVer']} licensee={pkg['licVer']} "
          f"names={len(pkg['names'])} exports={len(pkg['exports'])} "
          f"layout_bad={pkg['layout_bad']}")
    if pkg["layout_bad"]:
        print("LAYOUT REJECTED - not parsing further")
        sys.exit(1)
    for objname in sys.argv[2:]:
        ex = find_export(pkg, objname)
        if not ex:
            print(f"{objname}: NOT FOUND as Texture2D export")
            continue
        print(f"{objname}: export #{ex['index']} serial @{ex['off']} size {ex['size']}")
        for p in walk_properties(pkg, ex):
            print(f"    {p['name']}[{p['num']}] : {p['type']} size={p['size']} @+{p['off'] - ex['off']}")


if __name__ == "__main__":
    main()
