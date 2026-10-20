"""Read-only native equipment contracts from owned PE/archive/package inputs.

No process access, deployment or export of proprietary code/assets. Separate
native heavy classification from the still unproven player hand-grenade ABI.
"""
import argparse
import hashlib
import json
import struct
from pathlib import Path
from audit_rifle_scope_backend import PE, only, class_vtable, registration, block_contract
from ue3_layout import inspect


def audit(exe, archive, package):
    pe = PE(Path(exe).read_bytes())
    text = Path(archive).read_bytes()
    section = only([s for s in pe.sections if s[0] == '.text'], 'text')
    if len(text) != section[1]:
        raise ValueError('archive extent')
    base = pe.base + section[2]
    wrapper = registration(pe, text, base, 'intARvWeaponSharedexecIsHeavyWeapon')
    wrapper_receipt = block_contract(text, base, wrapper, 58, 0xB06FD8D75EF187D2)
    table = class_vtable(pe, text, base, 'RvWeaponShared')['vtable']
    target = struct.unpack('<I', pe.read(table + 0x554, 4))[0]
    getter_receipt = block_contract(text, base, target, 13, 0xAD9AEEB598427D84)
    if text[target-base:target-base+13] != bytes.fromhex('8b81d8020000c1e80783e001c3'):
        raise ValueError('heavy flag getter mechanism')
    data = Path(package).read_bytes()
    pkg = inspect(package, data=data, include_all=True)
    parent = lambda e: pkg['records'][e['outer']-1]['name'] if e['outer'] else ''
    enum = only([e for e in pkg['exports'] if e['name'] == 'EWeaponSlot' and
                 parent(e) == 'RvGameSharedPlayerController'], 'slot enum')
    raw = data[enum['offset']:enum['offset']+enum['size']]
    count = struct.unpack_from('<I', raw, 20)[0]
    if count != 5 or len(raw) != 24 + count*8:
        raise ValueError('slot enum shape')
    names = []
    for i in range(count):
        index, number = struct.unpack_from('<II', raw, 24+i*8)
        if number or index >= len(pkg['names']):
            raise ValueError('slot enum name')
        names.append(pkg['names'][index])
    if names != ['EWS_Custom1', 'EWS_Custom2', 'EWS_HeavyWeapon', 'EWS_GrenadeSlot', 'EWS_MAX']:
        raise ValueError('unexpected native slots')
    classes = {e['name'] for e in pkg['exports'] if e['class_name'] == 'None'}
    required = {'RvProp_DeadlockGrenade', 'RvWeaponShared_ARGrenadeLauncher',
                'RvProjectileShared_GrenadeSteerable'}
    if not required <= classes:
        raise ValueError('original grenade class identities')
    return dict(status='OFFLINE_STRUCTURAL_PASS',
                exe_sha256=hashlib.sha256(pe.data).hexdigest(),
                text_sha256=hashlib.sha256(text).hexdigest(),
                package_sha256=hashlib.sha256(data).hexdigest(),
                heavy_wrapper=wrapper_receipt, heavy_getter=getter_receipt,
                heavy_contract=dict(vtable_displacement=0x554, flag_offset=0x2d8, mask=0x80),
                native_slots=names, grenade_classes=sorted(required),
                player_hand_grenade_backend='UNPROVEN: ownership/count, pin, fuse, launch and debit',
                limitation='Slot3 or a damage-type name does not prove a throwable player grenade. No Steam/physical PASS.')


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--exe', required=True)
    parser.add_argument('--text', required=True)
    parser.add_argument('--package', required=True)
    parser.add_argument('--report', required=True)
    args = parser.parse_args()
    result = audit(args.exe, args.text, args.package)
    Path(args.report).write_text(json.dumps(result, indent=2)+'\n', encoding='utf8')
    print(result['status'])
