#!/usr/bin/env python3
"""Read-only PE32 checks for original-map native consumers; never loads the EXE.

This report is a prerequisite audit, not a claim of runtime or headset support.
The C++ game-build fixture separately tests core selection and fail-closed gates.
"""
import argparse
import hashlib
import json
import struct
from pathlib import Path


class Image:
    def __init__(self, path):
        self.raw = path.read_bytes()
        if self.raw[:2] != b'MZ':
            raise ValueError('not a DOS/PE image')
        pe = struct.unpack_from('<I', self.raw, 0x3c)[0]
        if self.raw[pe:pe+4] != b'PE\0\0':
            raise ValueError('invalid PE signature')
        machine, count = struct.unpack_from('<HH', self.raw, pe+4)
        optsize = struct.unpack_from('<H', self.raw, pe+20)[0]
        opt = pe+24
        if machine != 0x14c or struct.unpack_from('<H', self.raw, opt)[0] != 0x10b:
            raise ValueError('expected x86 PE32')
        self.base = struct.unpack_from('<I', self.raw, opt+28)[0]
        if self.base != 0x400000:
            raise ValueError('unexpected preferred image base')
        self.sections = []
        for i in range(count):
            at = opt+optsize+40*i
            vsize, rva, size, offset = struct.unpack_from('<IIII', self.raw, at+8)
            if offset+size > len(self.raw):
                raise ValueError('section outside file')
            self.sections.append((rva, size, offset))

    def read(self, va, size):
        rva = va-self.base
        for start, length, offset in self.sections:
            if start <= rva and rva+size <= start+length:
                return self.raw[offset+rva-start:offset+rva-start+size]
        raise ValueError(f'unbacked virtual range {va:08X}+{size}')


def fnv(data, bits):
    value = 2166136261 if bits == 32 else 14695981039346656037
    prime = 16777619 if bits == 32 else 1099511628211
    for byte in data:
        value = ((value ^ byte)*prime) & ((1 << bits)-1)
    return value


def audit(path):
    image = Image(path)
    checks = []
    # Exact existing production fingerprints, not hashes inferred from a new EXE.
    hashes = [
        ('missing-mouse init', 0x11d7439, 90, 32, 0x72189a6f),
        ('missing-mouse poll', 0x11d325b, 1629, 32, 0x7d8b63db),
        ('missing-mouse cooperative', 0x11d5f2b, 32, 32, 0xc02cc0c4),
        ('missing-mouse helper', 0x11cf3e0, 96, 32, 0x807d0696),
        ('gamepad enumeration callback', 0x11d2180, 0x313, 32, 0x8735998e),
        ('TMD power start', 0x100d790, 248, 64, 0x894f69cf191c20c7),
        ('TMD power stop', 0x100aec0, 194, 64, 0x1563d1e2018602de),
    ]
    for name, address, count, bits, expected in hashes:
        actual = fnv(image.read(address, count), bits)
        checks.append(dict(name=name, address=f'{address:08X}', bytes=count,
                           expected=f'{expected:0{bits//4}X}', actual=f'{actual:0{bits//4}X}',
                           matches=actual == expected))
    prefixes = [
        ('native UI poll', 0x11d31c0, '558bec6aff688a767001'),
        ('native gamepad enumeration', 0x11d2fc0,
         'a17cfcd10185c074138b088b51106a016a006880211d016a0450ffd2c3'),
    ]
    for name, address, expected_hex in prefixes:
        expected = bytes.fromhex(expected_hex)
        actual = image.read(address, len(expected))
        checks.append(dict(name=name, address=f'{address:08X}', bytes=len(expected),
                           expected=expected.hex(), actual=actual.hex(), matches=actual == expected))
    return dict(path=str(path.resolve()), bytes=len(image.raw),
                sha256=hashlib.sha256(image.raw).hexdigest().upper(),
                execution='NOT RUN; raw file reads only',
                original_map_prerequisites=all(c['matches'] for c in checks), checks=checks,
                limitation='Core-map fixture, remaining native consumers and runtime acceptance are separate gates.')


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('exe', type=Path)
    parser.add_argument('--output', type=Path)
    parser.add_argument('--require-original-map', action='store_true')
    args = parser.parse_args()
    result = audit(args.exe)
    value = json.dumps(result, indent=2)+'\n'
    if args.output:
        args.output.write_text(value, encoding='utf-8')
    else:
        print(value, end='')
    return 2 if args.require_original_map and not result['original_map_prerequisites'] else 0


if __name__ == '__main__':
    raise SystemExit(main())
