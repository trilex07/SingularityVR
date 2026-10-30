"""Verify the audited x86 contract from private owned-game captures; no runtime writes."""
import argparse
import hashlib
from pathlib import Path
from capstone import Cs, CS_ARCH_X86, CS_MODE_32
from disassemble_native_muzzle_capture import decode


def verify(capture, text, text_base):
    method_name, method_base, method = decode(capture)[1]
    assert method_name == 'method'
    decoder = Cs(CS_ARCH_X86, CS_MODE_32)
    parent = {i.address - method_base: i for i in decoder.disasm(method, method_base)}
    tail = list(parent.values())[-1]
    assert (tail.mnemonic, tail.op_str) == ('ret', '8')
    assert tail.address + tail.size == method_base + len(method)
    assert (parent[0x1d4].mnemonic, parent[0x1d4].op_str) == ('push', 'eax')
    assert (parent[0x1d8].mnemonic, parent[0x1d8].op_str) == ('push', 'eax')
    assert (parent[0x1e1].mnemonic, parent[0x1e1].op_str) == ('call', 'eax')
    assert parent[0x1e1].address + parent[0x1e1].size == method_base + 0x1e3
    for address, size, sha, cleanup in (
        (0xE7E5A0, 431, '47218b694af0a504b038b6a1eb621c8413c2012c0471494af798be25a6cb8395', '0x2c'),
        (0x493480, 96, '68cca9dd869623f066cf6a5488a1f7524069d6f5c45c675efeb7527afe2e3891', '4'),
    ):
        raw = text[address-text_base:address-text_base+size]
        assert hashlib.sha256(raw).hexdigest() == sha
        ins = list(decoder.disasm(raw, address))
        assert sum(i.size for i in ins) == size
        assert (ins[-1].mnemonic, ins[-1].op_str) == ('ret', cleanup)
        if address == 0xE7E5A0:
            by_address = {i.address: i for i in ins}
            assert by_address[0xE7E66D].op_str == 'eax, dword ptr [ebp + 8]'
            assert by_address[0xE7E695].op_str == 'ecx, dword ptr [ebp + 0xc]'
            assert (by_address[0xE7E6A7].mnemonic, by_address[0xE7E6A7].op_str) == ('call', '0x493480')
    print('PASS: original parent caller +1E3; position/direction args; complete spawn/vector bodies; ret8/ret2C/ret4')


if __name__ == '__main__':
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('capture', type=Path)
    p.add_argument('text', type=Path)
    p.add_argument('--text-base', type=lambda s: int(s, 0), default=0x402000)
    a = p.parse_args()
    verify(a.capture.read_bytes(), a.text.read_bytes(), a.text_base)
