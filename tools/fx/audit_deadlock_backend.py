"""Read-only Deadlock producer contracts from owned original inputs.

Records complete native extents, not first-return prefixes. No process access,
native calls, installed-file writes or export of proprietary code/assets.
"""
import argparse
from pathlib import Path
import struct
from audit_body_equipment import audit as equipment_audit
from audit_rifle_scope_backend import (PE, only, registration, block_contract,
                                       occurrences, u32, inspect)


def constructor(pe, text, base, name, length, fingerprint):
    """Measured Raven inlined placement ctor; final [esi] store is the type.

    Unlike the older two-store ctor mapper, supports a single final store and
    ignores interface-vptr stores at nonzero offsets. Fingerprint first.
    """
    from capstone import Cs, CS_ARCH_X86, CS_MODE_32
    from capstone.x86 import X86_OP_MEM, X86_OP_IMM, X86_REG_ESI
    pos = only(list(occurrences(pe.data, (name+'\0').encode('utf-16le'))), name)
    push = only(list(occurrences(text, b'\x68'+struct.pack('<I', pe.address(pos)))), name+' push')
    candidates = []
    for p in range(max(0, push-48), push-4):
        if text[p] != 0x68:
            continue
        address = u32(text, p+1)
        try:
            receipt = block_contract(text, base, address, length, fingerprint)
        except ValueError:
            continue
        md = Cs(CS_ARCH_X86, CS_MODE_32)
        md.detail = True
        ins = list(md.disasm(text[address-base:address-base+length], address))
        if not ins or ins[-1].mnemonic != 'ret':
            raise ValueError('incomplete constructor epilogue')
        stores = []
        for i in ins:
            if i.mnemonic != 'mov' or len(i.operands) != 2:
                continue
            dest, value = i.operands
            if (dest.type == X86_OP_MEM and dest.mem.base == X86_REG_ESI and
                    not dest.mem.index and dest.mem.disp == 0 and value.type == X86_OP_IMM):
                stores.append(value.imm)
        if not stores:
            raise ValueError('no primary constructor vptr')
        pe.read(stores[-1], 4)  # must be an owned PE-backed table
        candidates.append(dict(address=address, vtable=stores[-1], receipt=receipt))
    return only(candidates, name+' final native constructor')


def audit(exe, archive, package):
    result = equipment_audit(exe, archive, package)
    pe = PE(Path(exe).read_bytes())
    text = Path(archive).read_bytes()
    base = pe.base+only([s for s in pe.sections if s[0] == '.text'], 'text')[2]
    tmd = constructor(pe, text, base, 'RvTMD', 63, 0xF2C5B1B22F49E767)
    power = constructor(pe, text, base, 'RvTMDPower_Deadlock', 27, 0x4C7198611BD3DB7C)
    contracts = {}
    for key, name, n, expected in (
        ('input_wrapper', 'intARvTMDexecHandleTMDInputClientside', 147, 0xA8D96AB6FE796701),
        ('start_wrapper', 'intURvTMDPowerexecStartFirePower', 96, 0x7B8C05DBDEEC95C5),
        ('stop_wrapper', 'intURvTMDPowerexecStopFirePower', 96, 0xD5110E0A85330314),
        ('slot_wrapper', 'intARvTMDexecGetTMDESlot', 115, 0x308855DE30F66D90)):
        address = registration(pe, text, base, name)
        contracts[key] = block_contract(text, base, address, n, expected)
    for key, table, offset, n, expected in (
        ('input_all_paths', tmd['vtable'], 0x4e8, 241, 0x3F880641281843C5),
        ('deadlock_input_all_paths', power['vtable'], 0x128, 1504, 0xB26B7A29EA360695),
        ('slot_getter', tmd['vtable'], 0x488, 34, 0x82228FD9B33C3CCC)):
        address = struct.unpack('<I', pe.read(table+offset, 4))[0]
        contracts[key] = block_contract(text, base, address, n, expected)
        if text[address-base+n-3] != 0xc2:
            raise ValueError(key+' incomplete final retN')
    data = Path(package).read_bytes()
    pkg = inspect(package, data=data, include_all=True)
    default = only([e for e in pkg['exports'] if e['name'] == 'Default__RvTMDPower_Deadlock'], 'Deadlock default')
    archetype = only([e for e in pkg['exports'] if e['name'] == 'RvTMDPower_Deadlock' and
                      e['class_name'] == 'RvTMDPower_Deadlock'], 'Deadlock power archetype')
    def prop(e, name):
        return only([p for p in e['properties'] if p['name'] == name], name)
    if (prop(default, 'mPowerType')['value'] != 'ETP_Deadlock' or
            prop(default, 'mControlType')['value'] != 'EPCT_Constant' or
            prop(archetype, 'mProjectileArchetype')['value'] != 'DeadlockGrenade'):
        raise ValueError('Deadlock type/control/projectile relationship')
    enum = only([e for e in pkg['exports'] if e['name'] == 'ETMDPower' and e['class_name'] == 'Enum'], 'power enum')
    raw = data[enum['offset']:enum['offset']+enum['size']]
    count = struct.unpack_from('<I', raw, 20)[0]
    if count != 9 or len(raw) != 24+count*8:
        raise ValueError('power enum extent')
    index, number = struct.unpack_from('<II', raw, 24+3*8)
    if number or pkg['names'][index] != 'ETP_Deadlock':
        raise ValueError('Deadlock power index')
    result.update(deadlock_contracts=contracts,
        tmd_constructor=tmd['receipt'], power_constructor=power['receipt'],
        deadlock_power_index=3, native_power='RvTMDPower_Deadlock',
        projectile_archetype='DeadlockGrenade',
        power_is_not_grenade_inventory=True,
        limitation='Native contracts and original relationship only. No pin/spawn/velocity/debit runtime or physical acceptance.')
    return result


if __name__ == '__main__':
    import json
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--exe', required=True)
    ap.add_argument('--text', required=True)
    ap.add_argument('--package', required=True)
    ap.add_argument('--report', required=True)
    args = ap.parse_args()
    result = audit(args.exe, args.text, args.package)
    Path(args.report).write_text(json.dumps(result, indent=2)+'\n', encoding='utf8')
    print('OFFLINE_STRUCTURAL_PASS: original Deadlock TMD producer, complete native contracts; no gameplay claim')
