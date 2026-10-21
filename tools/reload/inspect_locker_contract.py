"""Offline locker contract witness; reads owned EXE + archived text, never patches.

Reports identities and instruction locations, not proprietary function bodies.
Requires capstone. A matched contract is NOT a runtime four-slot implementation.
"""
import argparse
import hashlib
import json
import struct
from pathlib import Path

from inspect_native_reload import PE
from inspect_inventory_registration import resolve_wrapper


def inspect(exe, text, base):
    import capstone
    pe = PE(exe)
    md = capstone.Cs(capstone.CS_ARCH_X86, capstone.CS_MODE_32)
    # Measured class constructor stores this vtable; refuse other executables.
    table = 0x019A0948
    expected = {0x11C: 0x00F11430, 0x190: 0x00F17F60,
                0x1F4: 0x00F23840, 0x1F8: 0x00F00F50}
    for slot, target in expected.items():
        if struct.unpack('<I', pe.read(table + slot, 4))[0] != target:
            raise ValueError('unrecognized locker vtable; no conclusion')
    if not (base <= 0xF11430 < 0xF23840 < base + len(text)):
        raise ValueError('archived text does not cover locker')
    def code(address, size):
        return list(md.disasm(text[address-base:address-base+size], address))
    start = code(expected[0x11C], 0x13B)
    # Start explicitly calls GetWeaponSlotName(0), then (1), not a four-slot loop.
    getter = [i.address for i in start if i.mnemonic == 'mov' and
              i.op_str in ('edx, dword ptr [edx + 0x740]',
                           'edx, dword ptr [eax + 0x740]')]
    indices = [i.op_str for i in start if i.mnemonic == 'push' and i.op_str in ('0', '1')]
    if len(getter) != 2 or not all(str(n) in indices for n in (0, 1)):
        raise ValueError('Start contract changed; no conclusion')
    commit = code(expected[0x190], 0x42B)
    setters = [i.address for i in commit if i.mnemonic == 'mov' and
               i.op_str == 'edx, dword ptr [edx + 0x744]']
    if len(setters) != 1:
        raise ValueError('PopMenu setter contract changed; no conclusion')
    return dict(exe_sha256=hashlib.sha256(exe).hexdigest(),
                text_sha256=hashlib.sha256(text).hexdigest(),
                vtable=hex(table), start_getter_calls=[hex(a) for a in getter],
                popmenu_setter_dispatch=hex(setters[0]),
                methods=[resolve_wrapper(pe, text, base,
                    'intURvUIGFxMovie_WeaponLockerexec' + n)
                    for n in ('Start', 'PopMenu', 'LoadEquippedWeapons', 'equip')],
                conclusion='Native locker starts with slots 0/1. PopMenu writes '
                'every equipped entry through SetWeaponSlotName(index). '
                'Do not resize to four without virtualizing the commit contract.',
                evidence='Offline structural witness only; no process or game writes')


if __name__ == '__main__':
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--exe', type=Path, required=True)
    ap.add_argument('--text', type=Path, required=True)
    ap.add_argument('--text-base', type=lambda x: int(x, 0), required=True)
    args = ap.parse_args()
    print(json.dumps(inspect(args.exe.read_bytes(), args.text.read_bytes(),
                             args.text_base), indent=2))
