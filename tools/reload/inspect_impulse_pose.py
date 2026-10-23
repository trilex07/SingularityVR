"""Read-only inspection of archived native animation/presentation code.

No process access and no Steam access. Inputs must be private archived copies.
Disassembly is printed locally; never commit extracted game code.
"""
import argparse
import struct
from pathlib import Path
import capstone
from inspect_native_reload import PE, occurrences
from inspect_inventory_registration import resolve_wrapper


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--exe', type=Path, required=True)
    ap.add_argument('--text', type=Path, required=True)
    ap.add_argument('--name', action='append', default=[])
    ap.add_argument('--address', action='append', type=lambda x: int(x, 0), default=[])
    ap.add_argument('--xrefs', action='append', type=lambda x: int(x, 0), default=[])
    ap.add_argument('--size', type=lambda x: int(x, 0), default=256)
    args = ap.parse_args()
    pe, text = PE(args.exe.read_bytes()), args.text.read_bytes()
    base = 0x402000
    md = capstone.Cs(capstone.CS_ARCH_X86, capstone.CS_MODE_32)
    addresses = args.address
    for name in args.name:
        witness = resolve_wrapper(pe, text, base, name)
        print(witness)
        addresses.append(witness['wrapper'])
    for target in args.xrefs:
        print('direct call/jmp xrefs', hex(target))
        for p in range(len(text)-5):
            if text[p] in (0xe8, 0xe9) and base+p+5+struct.unpack_from('<i', text, p+1)[0] == target:
                print(hex(base+p), 'call' if text[p] == 0xe8 else 'jmp')
        print('file pointer references', [hex(pe.address(p)) for p in occurrences(pe.data, struct.pack('<I', target))])
    for va in addresses:
        if not base <= va < base+len(text):
            raise ValueError('address outside archived text')
        print('\nDISASSEMBLY', hex(va))
        for i in md.disasm(text[va-base:va-base+args.size], va):
            print(f'{i.address:08X} {i.bytes.hex():24} {i.mnemonic} {i.op_str}')


if __name__ == '__main__':
    main()
