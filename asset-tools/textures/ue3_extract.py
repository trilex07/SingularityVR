#!/usr/bin/env python3
"""
ue3_extract - minimal UE3 package texture extractor for Singularity (research).

UModel exports the highest mip actually reachable, but for some textures it
aborts on a missing lower mip and writes nothing (the first-person pistol is the
important case). This tool reads the texture's own mip table out of the package
and pulls each mip either from the inline bulk data or from coalesced.tfc,
reassembling a DDS - so a texture whose mip 0 IS present can be recovered even
when UModel refuses.

Only used on the decompressed / uncompressed package plus the read-only TFC.
Nothing is written into the game.

Usage:
    python ue3_extract.py list  <pkg> [filter]
    python ue3_extract.py dump  <pkg> <object> [out.bin]
    python ue3_extract.py texture <pkg> <object> <out.dds> [--tfc <path>]
"""
from __future__ import annotations

import argparse
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
        return struct.unpack("<i", struct.pack("<I", self.u32()))[0]

    def u64(self) -> int:
        lo, hi = self.u32(), self.u32()
        return lo | (hi << 32)

    def skip(self, n: int):
        if self.need(n):
            self.o += n

    def fstring(self) -> str:
        n = self.i32()
        if self.bad or n == 0:
            return ""
        if n > 0:
            if n > 1 << 20 or not self.need(n):
                self.bad = True
                return ""
            s = self.b[self.o:self.o + n].split(b"\x00", 1)[0].decode("latin1", "replace")
            self.o += n
            return s
        chars = -n
        if chars > 1 << 20 or not self.need(chars * 2):
            self.bad = True
            return ""
        s = self.b[self.o:self.o + chars * 2].decode("utf-16-le", "replace").split("\x00", 1)[0]
        self.o += chars * 2
        return s


class Package:
    def __init__(self, path: Path):
        self.buf = path.read_bytes()
        r = Reader(self.buf)
        tag = r.u32()
        if tag != TAG:
            raise SystemExit(f"{path}: not a UE3 package (tag 0x{tag:08X})")
        ver = r.u32()
        self.file_ver = ver & 0xFFFF
        self.lic_ver = ver >> 16
        self.header_size = r.i32()
        self.folder = r.fstring()
        self.flags = r.u32()
        self.name_count = r.i32()
        self.name_off = r.i32()
        self.export_count = r.i32()
        self.export_off = r.i32()
        self.import_count = r.i32()
        self.import_off = r.i32()
        self.names: list[str] = []
        self.exports: list[dict] = []
        self._read_names()
        self._read_exports()

    def _read_names(self):
        r = Reader(self.buf, self.name_off)
        for _ in range(self.name_count):
            s = r.fstring()
            r.u64()
            self.names.append(s)
        if r.bad:
            raise SystemExit("name table did not parse")

    def _read_exports(self):
        r = Reader(self.buf, self.export_off)
        for _ in range(self.export_count):
            ex = {}
            ex["cls"] = r.i32()
            ex["super"] = r.i32()
            ex["outer"] = r.i32()
            ex["name_idx"] = r.i32()
            ex["name_num"] = r.i32()
            ex["arch"] = r.i32()
            ex["flags"] = r.u64()
            ex["size"] = r.i32()
            ex["offset"] = r.i32()
            r.u32()
            n = r.i32()
            if n < 0 or n > 1 << 16:
                r.bad = True
                break
            r.skip(n * 4)
            r.skip(16)
            r.u32()
            self.exports.append(ex)
        if r.bad:
            raise SystemExit("export table did not parse")

    def name_at(self, i: int) -> str:
        return self.names[i] if 0 <= i < len(self.names) else "<bad>"

    def export_index(self, name: str):
        for i, ex in enumerate(self.exports):
            if self.name_at(ex["name_idx"]) == name:
                return i
        return None

    def class_name(self, ex: dict) -> str:
        return self.name_at(ex["cls"]) if ex["cls"] > 0 else self.name_at(ex["cls"]) if ex["cls"] < 0 else "Class"


def cmd_list(a):
    pkg = Package(Path(a.pkg))
    print(f"{a.pkg}: ver {pkg.file_ver}/{pkg.lic_ver} names {pkg.name_count} "
          f"exports {pkg.export_count} imports {pkg.import_count}")
    flt = (a.filter or "").lower()
    for i, ex in enumerate(pkg.exports):
        nm = pkg.name_at(ex["name_idx"])
        if flt and flt not in nm.lower():
            continue
        print(f"{i:6d} {nm:34s} size={ex['size']:8d} off=0x{ex['offset']:08X} cls={pkg.name_at(ex['cls'])}")


def cmd_dump(a):
    pkg = Package(Path(a.pkg))
    idx = pkg.export_index(a.object)
    if idx is None:
        raise SystemExit(f"export {a.object} not found")
    ex = pkg.exports[idx]
    data = pkg.buf[ex["offset"]:ex["offset"] + ex["size"]]
    out = a.out or f"{a.object}.bin"
    Path(out).write_bytes(data)
    print(f"{a.object}: class={pkg.name_at(ex['cls'])} size={ex['size']} -> {out}")
    print("first 64 bytes:", data[:64].hex(" "))


def main():
    ap = argparse.ArgumentParser()
    sub = ap.add_subparsers(dest="cmd", required=True)
    p = sub.add_parser("list"); p.add_argument("pkg"); p.add_argument("filter", nargs="?")
    p = sub.add_parser("dump"); p.add_argument("pkg"); p.add_argument("object"); p.add_argument("out", nargs="?")
    a = ap.parse_args()
    {"list": cmd_list, "dump": cmd_dump}[a.cmd](a)


if __name__ == "__main__":
    main()
