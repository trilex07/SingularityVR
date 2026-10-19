"""Offline verification of the audited pawn weapon-selector virtual slot.

Reads the owned game EXE only. Never opens a process, never launches Steam and
never writes to the installation. Emits identities and hashes, not code bodies.

Purpose: the runtime wheel selector calls pawn vtable+0xB14 with an FName and
refuses unless the method body matches a 168-byte FNV fingerprint. If that
fingerprint or the slot is wrong, the wheel would select nothing at runtime and
the log would only say "refused". This proves both numbers offline:

  * vtable+0xB18 must point at the independently audited HasWeapon target
    0x102F700 (cross-check of the vtable base), and
  * vtable+0xB14 must match the 168-byte FNV 0xCB797F59504492C3 the runtime
    checks.
"""
import argparse
import hashlib
import json
import struct
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
from inspect_native_reload import PE, class_vtable, u32


def fnv1a(data):
    h = 14695981039346656037
    for b in data:
        h ^= b
        h = (h * 1099511628211) & 0xFFFFFFFFFFFFFFFF
    return h


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--exe', type=Path, required=True)
    ap.add_argument('--cls-name', default='RvPlayerPawnSP')
    ap.add_argument('--slot', type=lambda s: int(s, 0), default=0xB14)
    ap.add_argument('--cross-slot', type=lambda s: int(s, 0), default=0xB18)
    ap.add_argument('--cross-target', type=lambda s: int(s, 0), default=0x102F700)
    ap.add_argument('--fingerprint-bytes', type=int, default=168)
    ap.add_argument('--expect-fingerprint', type=lambda s: int(s, 0),
                    default=0xCB797F59504492C3)
    ap.add_argument('--output', type=Path)
    args = ap.parse_args()

    pe = PE(args.exe.read_bytes())
    text_sections = [s for s in pe.sections if s[0] == '.text']
    if len(text_sections) != 1:
        raise ValueError('expected exactly one .text section')
    _, size, rva, _, _ = text_sections[0]
    base = pe.base + rva
    text = pe.read(base, size)

    row = class_vtable(pe, text, base, args.cls_name)
    vtable = row['vtable']
    selector = u32(pe.read(vtable + args.slot, 4), 0)
    cross = u32(pe.read(vtable + args.cross_slot, 4), 0)

    if not base <= selector < base + len(text):
        raise ValueError('selector outside .text')
    body = text[selector - base: selector - base + args.fingerprint_bytes]
    digest = fnv1a(body)

    report = dict(
        scope='OFFLINE identity evidence only; NOT live execution proof',
        exe_sha256=hashlib.sha256(pe.data).hexdigest(),
        image_base=hex(pe.base),
        cls=row['name'] if 'name' in row else args.cls_name,
        vtable=hex(vtable),
        selector_slot=hex(args.slot),
        selector_va=hex(selector),
        selector_bytes=args.fingerprint_bytes,
        selector_fingerprint=hex(digest),
        expected_fingerprint=hex(args.expect_fingerprint),
        fingerprint_matches=digest == args.expect_fingerprint,
        cross_slot=hex(args.cross_slot),
        cross_va=hex(cross),
        cross_expected=hex(args.cross_target),
        cross_matches=cross == args.cross_target,
    )
    text_out = json.dumps(report, indent=2)
    if args.output:
        args.output.parent.mkdir(parents=True, exist_ok=True)
        args.output.write_text(text_out + '\n', encoding='utf-8')
    print(text_out)
    if not report['fingerprint_matches'] or not report['cross_matches']:
        raise SystemExit(2)


if __name__ == '__main__':
    main()
