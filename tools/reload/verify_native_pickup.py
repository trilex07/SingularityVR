"""Offline proof of the native pickup keep-existing argument. No process access.

Checks owned class/vtable identities, complete body hash, both TossInventory
calls and the branch which bypasses them. Emits receipts, not game code.
"""
import argparse
import hashlib
import re
import struct
from pathlib import Path
from inspect_native_reload import PE, occurrences, only, u32


def pawn_vtable(pe, text, base):
    # The SP constructor is longer than the generic mapper's 80-byte/ret
    # window. Follow the same class-name registration -> ctor thunk chain,
    # then require its exact initial base-ctor/object-vtable-store form.
    name = only(list(occurrences(pe.data, 'RvPlayerPawnSP\0'.encode('utf-16-le'))), 'SP name')
    push = only(list(occurrences(text, b'\x68'+struct.pack('<I', pe.address(name)))), 'SP name push')
    tables = []
    for pos in range(max(0, push-48), push-4):
        if text[pos] != 0x68:
            continue
        thunk = u32(text, pos+1)
        if not base <= thunk < base+len(text)-32:
            continue
        raw = text[thunk-base:thunk-base+32]
        if raw[:12] != bytes.fromhex('558bec8b4d0885c974065de9'):
            continue
        ctor = thunk+16+struct.unpack_from('<i', raw, 12)[0]
        if not base <= ctor < base+len(text)-16:
            continue
        body = text[ctor-base:ctor-base+16]
        if body[:4] != bytes.fromhex('568bf1e8') or body[8:10] != b'\xc7\x06':
            continue
        table = u32(body, 10)
        if u32(pe.read(table+0xB18, 4), 0) == 0x102F700:
            tables.append(table)
    return only(tables, 'SP constructor/vtable')


def verify(body, address):
    if len(body) != 748:
        raise ValueError('pickup body length')
    h = 14695981039346656037
    for b in body:
        h = ((h ^ b) * 1099511628211) & ((1 << 64) - 1)
    if h != 0xCC78CCB4EE3651A2:
        raise ValueError('pickup full-body identity')
    # cmp [ebp+14],0; jne +19CA, before either toss.
    if body[0x1A4:0x1AA] != bytes.fromhex('837d14007560'):
        raise ValueError('keep-existing branch')
    if body.count(bytes.fromhex('837d1400')) != 1:
        raise ValueError('additional fourth-argument condition')
    for off in (0x1D4, 0x205):
        if body[off] != 0xE8 or address + off + 5 + struct.unpack_from('<i', body, off+1)[0] != 0x104E500:
            raise ValueError('toss call identity')
    if body[-3:] != bytes.fromhex('c21000'):
        raise ValueError('four-argument callee cleanup')
    return f'{h:016X}'


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--exe', type=Path, required=True)
    ap.add_argument('--text', type=Path, required=True)
    ap.add_argument('--text-base', type=lambda s: int(s, 0), required=True)
    args = ap.parse_args()
    policy = (Path(__file__).resolve().parents[2] / 'spikes/view_matrix/inventory_pickup_policy.h').read_text()
    if not re.search(r'Slot\s*=\s*0xd70\s*,\s*Bytes\s*=\s*748', policy) or not re.search(
            r'Fingerprint\s*=\s*0xCC78CCB4EE3651A2ull', policy):
        raise ValueError('runtime policy/verifier identity drift')
    pe = PE(args.exe.read_bytes())
    text = args.text.read_bytes()
    table = pawn_vtable(pe, text, args.text_base)
    target = u32(pe.read(table+0xD70, 4), 0)
    off = target-args.text_base
    body = text[off:off+748]
    print('PASS native pickup', hex(target), 'slot=D70', 'fnv='+verify(body, target))
    # Independently pin the shared toss wrapper to its learned FName globals.
    toss = text[0x104E500-args.text_base:0x104E565-args.text_base]
    if bytes.fromhex('8b0d6c4fcf01') not in toss or toss[-3:] != bytes.fromhex('c21400'):
        raise ValueError('TossInventory wrapper identity')
    # Game-authored restore caller: archetype, select=0, instant=0, keep=1.
    restore = text[0xFF7804-args.text_base:0xFF7815-args.text_base]
    if restore != bytes.fromhex('8b90700d00006a016a006a00518bcbffd2'):
        raise ValueError('restore caller keep-existing argument')
    for pos in (0x1A4, 0x1A9, 0x1D4, 0x205, 745):
        bad = bytearray(body)
        bad[pos] ^= 1
        try:
            verify(bad, target)
        except ValueError:
            continue
        raise AssertionError('mutated body accepted')
    print('PASS toss wrapper, restore caller and 5 refusal mutations; OFFLINE ONLY')
    print('text SHA256', hashlib.sha256(text).hexdigest())


if __name__ == '__main__':
    main()
