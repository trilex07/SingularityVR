"""Validate/disassemble the bounded pre-detour owned-code receipt, offline only.

Output contains proprietary game code; use an ignored private build directory.
"""
import argparse
import hashlib
import struct
from pathlib import Path


def decode(data):
    if len(data) != 1067:
        raise ValueError('capture must contain exactly 1067 bytes')
    magic, version, wrapper, wn, method, mn = struct.unpack_from('<6I', data)
    if (magic, version, wn, mn) != (0x31434D53, 1, 197, 846) or not wrapper or not method:
        raise ValueError('invalid capture header')
    result = []
    for name, address, raw, expected in (
        ('wrapper', wrapper, data[24:221], 0x1153B073558EDE80),
        ('method', method, data[221:], 0xDEA566CA454CAFEB),
    ):
        value = 14695981039346656037
        for byte in raw:
            value = ((value ^ byte) * 1099511628211) & 0xffffffffffffffff
        if value != expected:
            raise ValueError(f'{name}: fingerprint mismatch or detoured code')
        result.append((name, address, raw))
    return result


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('capture', type=Path)
    p.add_argument('output', type=Path)
    args = p.parse_args()
    data = args.capture.read_bytes()
    bodies = decode(data)
    from capstone import Cs, CS_ARCH_X86, CS_MODE_32
    lines = [f'capture SHA256 {hashlib.sha256(data).hexdigest()}']
    decoder = Cs(CS_ARCH_X86, CS_MODE_32)
    for name, address, raw in bodies:
        lines.append(f'\n{name} {address:08X} bytes={len(raw)} SHA256={hashlib.sha256(raw).hexdigest()}')
        used = 0
        for ins in decoder.disasm(raw, address):
            lines.append(f'{ins.address:08X}  {ins.bytes.hex():24s} {ins.mnemonic} {ins.op_str}')
            used += ins.size
        if used != len(raw):
            raise ValueError(f'{name}: incomplete instruction decoding ({used}/{len(raw)})')
    with args.output.open('x', encoding='utf-8') as f:
        f.write('\n'.join(lines) + '\n')
    print(f'Validated both complete original bodies; private disassembly: {args.output}')


if __name__ == '__main__':
    main()
