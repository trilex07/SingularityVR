"""Offline native Kasimov spread audit; no process or installation writes."""
import argparse
import json
from pathlib import Path
from audit_rifle_scope_backend import PE, only, u32, class_vtable, block_contract


def audit(exe, archive):
    pe = PE(Path(exe).read_bytes())
    text = Path(archive).read_bytes()
    section = only([s for s in pe.sections if s[0] == '.text'], 'text')
    if len(text) != section[1]:
        raise ValueError('archive extent')
    base = pe.base + section[2]
    table = class_vtable(pe, text, base, 'RvWeaponShared_SniperRifle')['vtable']
    receipts = {}
    for name, slot, size, fingerprint, ret in (
        ('minimum', 0x4b4, 129, 0x75C0B13EC73BB9E9, 0x36),
        ('maximum', 0x4b8, 129, 0x272F8E27E1E67C39, 0x36),
        ('movement', 0x4bc, 98, 0x0BBDCA92A52B52AF, 0x33),
    ):
        method = u32(pe.read(table + slot, 4), 0)
        receipt = block_contract(text, base, method, size, fingerprint)
        body = text[method-base:method-base+size]
        if body[ret-2:ret] != bytes.fromhex('ffd0'):
            raise ValueError('exact native IsAiming call missing: ' + name)
        if bytes.fromhex('980a0000') not in body[:ret]:
            raise ValueError('Pawn.IsAiming virtual slot missing: ' + name)
        receipt['aim_return_offset'] = ret
        receipts[name] = receipt
    method = u32(pe.read(table + 0x428, 4), 0)
    spread = block_contract(text, base, method, 536, 0x90E1F845D48FC976)
    body = text[method-base:method-base+536]
    if body[6:12] != bytes.fromhex('8b450c8b5514') or body[13:16] != bytes.fromhex('8b7508') or body[19:30] != bytes.fromhex('8b4d108906894e04895608') or body[-3:] != bytes.fromhex('c21000'):
        raise ValueError('AddSpread hidden result / input rotator / callee cleanup ABI')
    return dict(status='OFFLINE_STRUCTURAL_PASS', getters=receipts, add_spread=spread,
                limitation='Native caller identity only; no physical hit/pixel proof')


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--exe', required=True)
    parser.add_argument('--text', required=True)
    parser.add_argument('--report', required=True)
    args = parser.parse_args()
    Path(args.report).write_text(json.dumps(audit(args.exe, args.text), indent=2), encoding='utf-8')
