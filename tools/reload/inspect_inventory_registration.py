"""Read-only registration evidence from an owned PE and archived .text.

No process access, launch, save inspection or game writes. Supports both measured
registration forms; rejects a missing/ambiguous initializer and out-of-text code.
Outputs identities and hashes, never proprietary function bodies.
"""
import argparse
import hashlib
import json
from pathlib import Path
import struct

from inspect_native_reload import PE, occurrences, only, u32


def resolve_wrapper(pe, text, base, name):
    pos = only(list(occurrences(pe.data, (name + '\0').encode('utf-16le'))), 'name')
    name_va = pe.address(pos)
    row = only([p for p in occurrences(pe.data, struct.pack('<I', name_va))
                if p % 4 == 0], 'registration row')
    fn = u32(pe.data, row + 4)
    initializer = None
    if fn:
        form = 'static-pointer'
    else:
        destination = pe.address(row) + 4
        assignments = [p for p in occurrences(text, b'\xa3' + struct.pack('<I', destination))
                       if p >= 5 and text[p - 5] == 0xb8]
        p = only(assignments, 'registration initializer')
        fn = u32(text, p - 4)
        initializer = base + p - 5
        form = 'initializer-store'
    if not base <= fn <= base + len(text) - 16:
        raise ValueError('wrapper outside archived text')
    return dict(name=name, row=pe.address(row), wrapper=fn,
                form=form, initializer=initializer,
                prefix16_sha256=hashlib.sha256(text[fn-base:fn-base+16]).hexdigest())


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--exe', type=Path, required=True)
    ap.add_argument('--text', type=Path, required=True)
    ap.add_argument('--text-base', type=lambda s: int(s, 0), required=True)
    args = ap.parse_args()
    exe, text = args.exe.read_bytes(), args.text.read_bytes()
    if not 16 <= len(text) <= 64 * 1024 * 1024:
        raise ValueError('archived text size out of bounds')
    pe = PE(exe)
    names = ['intARvSharedPawnexecHasWeapon',
             'intARvGameSharedInventoryManagerexecChangeToWeaponByName']
    names += ['intARvGameSharedPlayerControllerexec' + n for n in
              ('CycleWeapon', 'GetCycleWeapon', 'GetWeaponSlotName', 'SetWeaponSlotName')]
    print(json.dumps(dict(exe_sha256=hashlib.sha256(exe).hexdigest(),
                         text_sha256=hashlib.sha256(text).hexdigest(),
                         text_base=args.text_base,
                         methods=[resolve_wrapper(pe, text, args.text_base, n) for n in names],
                         evidence='Offline registration identity only; no ownership or runtime proof'),
                     indent=2))


if __name__ == '__main__':
    main()
