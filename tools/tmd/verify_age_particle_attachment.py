"""Verify Age attachment contracts against archived ORIGINAL x86 machine code.

Read-only; no live process/game access. Inputs and disassembly remain private.
"""
import argparse
import hashlib
import struct
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'fx'))
from audit_rifle_scope_backend import PE, fnv
from capstone import Cs, CS_ARCH_X86, CS_MODE_32


def verify(exe, text_path):
    pe = PE(exe.read_bytes())
    text = text_path.read_bytes()
    assert hashlib.sha256(text).hexdigest().upper() == '070D20A053880F4164A43A348AF3C043A007716663B6EE71A1D829781757B0E1'
    base = 0x402000
    dis = Cs(CS_ARCH_X86, CS_MODE_32)

    def block(address, length, fingerprint=None):
        raw = text[address-base:address-base+length]
        assert len(raw) == length
        if fingerprint is not None:
            assert fnv(raw) == fingerprint
        ops = list(dis.disasm(raw, address))
        assert sum(i.size for i in ops) == length
        return [(i.mnemonic, i.op_str) for i in ops]

    ctor = block(0x78A880, 27)
    assert ('mov', 'dword ptr [esi], 0x17ef2b8') in ctor
    # Read original vtable contents through the PE's address mapping.
    def word(va):
        return struct.unpack('<I', pe.read(va, 4))[0]
    assert word(0x17EF2B8+0x154) == 0x897240
    assert word(0x17EF2B8+0x120) == 0x7C0E30
    update = block(0x897240, 124, 0x81209A2B80795CAE)
    assert ('ret', '0x10') in update
    block(0x7C0E30, 68, 0x0DBA87D91A4465CC)
    setter = block(0x754480, 384)
    assert ('mov', 'dword ptr [esi + 0x1d4], edi') in setter
    assert ('ret', '4') in setter
    children = block(0xB036F0, 396, 0x750AA01ABF6A6D77)
    for instruction in [('mov', 'esi, dword ptr [edi + 0x27c]'),
                        ('cmp', 'dword ptr [edi + 0x280], eax')]:
        assert instruction in children
    assert any(m == 'add' and '0x30' in o for m, o in children)
    print('PASS original PSC vtable/shared update RET16, template +1D4, owned attachment +27C/+280 stride30h; no game launch')


if __name__ == '__main__':
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--exe', type=Path, required=True)
    p.add_argument('--text', type=Path, required=True)
    a = p.parse_args()
    verify(a.exe, a.text)
