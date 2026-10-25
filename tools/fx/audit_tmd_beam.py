"""Original aging-beam attachment producer and actual /O2 callback audit.

Read-only owned binaries. Disassembly stays in the ignored local report.
This proves native contracts, not that a particular session consumed them.
"""
import argparse
import hashlib
import json
from pathlib import Path
import re
import struct
from capstone import Cs, CS_ARCH_X86, CS_MODE_32
from audit_deadlock_backend import constructor
from audit_rifle_scope_backend import PE, only, block_contract, inspect


def audit(exe, archive, package, dll):
    pe = PE(exe.read_bytes())
    text = archive.read_bytes()
    section = only([s for s in pe.sections if s[0] == '.text'], 'text')
    assert len(text) == section[1]
    base = pe.base + section[2]
    md = Cs(CS_ARCH_X86, CS_MODE_32)
    ptr = lambda a: struct.unpack('<I', pe.read(a, 4))[0]
    age = constructor(pe, text, base, 'RvTMDPower_Age', 27, 0x240D99E91EC2402E)
    beam = constructor(pe, text, base, 'RvCurvedBeamComponent', 27, 0xE9CB2340DC792D68)
    create = ptr(age['vtable'] + 0x144)
    tick = ptr(age['vtable'] + 0x120)
    update = ptr(beam['vtable'] + 0x154)
    copy = ptr(beam['vtable'] + 0x120)
    # Entire known blocks include all early returns and branch destinations.
    contracts = {}
    for key, a, n, expected in (
        ('age_create', create, 209, 0xE896EBDDC7AA69D1),
        ('age_tick_fx', tick, 618, 0xA7CF9086B0FCC73F),
        ('attach_socket', 0xB0BE40, 123, 0x92248BACBFC2BF33),
        ('update_children', 0xB036F0, 396, 0x750AA01ABF6A6D77),
        ('update_child', update, 124, 0x81209A2B80795CAE),
        ('copy_parent', copy, 68, 0x0DBA87D91A4465CC)):
        contracts[key] = dict(address=hex(a), **block_contract(text, base, a, n, expected))

    def ins(a, n):
        return list(md.disasm(text[a-base:a-base+n], a))

    def has(rows, mnemonic, operand):
        assert any(i.mnemonic == mnemonic and i.op_str == operand for i in rows), (mnemonic, operand)

    c = ins(create, 209)
    # The actual power creates/stores its curved beam then attaches that same
    # object to mOwnerMesh using the exact mMuzzleSocket FName pair.
    # ESI is the RvTMD argument, not the power object's ECX receiver. The
    # runtime must read these native offsets from that exact owned TMD.
    has(c, 'mov', 'esi, dword ptr [ebp + 8]')
    for operand in ('dword ptr [esi + 0x530], eax', 'ecx, dword ptr [esi + 0x204]',
                    'eax, dword ptr [esi + 0x460]', 'edx, dword ptr [esi + 0x45c]',
                    'eax, dword ptr [esi + 0x530]'):
        has(c, 'mov', operand)
    has(c, 'call', '0xb0be40')
    has(ins(tick, 618), 'mov', 'ecx, dword ptr [esi + 0x530]')
    has(ins(tick, 618), 'movss', 'xmm4, dword ptr [ecx + 0xa0]')
    children = ins(0xB036F0, 396)
    has(children, 'mov', 'eax, dword ptr [edi + 0x218]')
    has(children, 'shl', 'edx, 6')  # original CPU SpaceBases[bone], 64 bytes
    at = only([j for j, i in enumerate(children) if i.address == 0xB0384F], 'child call')
    assert [(i.mnemonic, i.op_str) for i in children[at-7:at+1]] == [
        ('push', '0'), ('lea', 'eax, [ebp - 0x60]'), ('push', 'eax'),
        ('mov', 'eax, dword ptr [edi + 0x48]'), ('push', 'ecx'),
        ('push', 'eax'), ('mov', 'ecx, esi'), ('call', 'edx')]
    has(children, 'mov', 'edx, dword ptr [edx + 0x154]')
    assert children[at].op_str == 'edx' and children[at].size == 2
    assert children[at+1].address - 0xB036F0 == 0x161
    has(ins(update, 124), 'ret', '0x10')
    cp = ins(copy, 68)
    for offset in ('', ' + 0x10', ' + 0x20', ' + 0x30'):
        assert any(i.mnemonic == 'movaps' and 'ptr [ecx'+offset+']' in i.op_str for i in cp)
    has(cp, 'add', 'ecx, 0x150')  # synchronous copy, no stack pointer escape

    pkg = inspect(package, include_all=True)
    records = pkg['records']
    ref = lambda i: pkg['names'][pkg['imports'][-i-1][5]] if i < 0 else records[i-1]['name'] if i else 'None'
    tmd = only([e for e in records if e['name'] == 'RvTMD' and e['cls'] == 0], 'TMD class')
    aging = only([e for e in records if e['outer'] == tmd['index'] and e['name'] == 'mAgingBeam'], 'aging field')
    assert ref(aging['cls']) == 'ComponentProperty'
    curved = only([e for e in records if e['name'] == 'RvCurvedBeamComponent' and e['cls'] == 0], 'beam class')
    shared = records[curved['super_index']-1]
    assert shared['name'] == 'RvSharedBeamComponent' and ref(shared['super_index']) == 'StaticMeshComponent'

    proxy = PE(dll.read_bytes())
    symbols = []
    for line in dll.with_suffix('.map').read_text(errors='replace').splitlines():
        m = re.search(r'\s(\S+)\s+([0-9a-fA-F]{8}) f\s', line)
        if m:
            symbols.append((int(m[2], 16), m[1]))
    hook = only([a for a, name in symbols if '?Hook@NativeTmdBeam@' in name], 'compiled beam callback')
    end = min(a for a, _ in symbols if a > hook)
    emitted = list(md.disasm(proxy.read(hook, end-hook), hook))
    # A __fastcall adapter receives the native this in ECX and four stack
    # arguments, and returns with the SAME native RET16 cleanup.
    returns = [i.op_str for i in emitted if i.mnemonic == 'ret']
    assert returns and all(r == '0x10' for r in returns), returns
    # Verify the ACTUAL emitted adapter, including the collision argument kept
    # on the stack across the cdecl Prepare call. No extra caller cleanup after
    # native RET16 is allowed. The CPU test also executes this production Hook.
    native_call = only([j for j, i in enumerate(emitted)
                        if i.mnemonic == 'call' and i.op_str.startswith('dword ptr [')],
                       'compiled indirect native update')
    has(emitted[:native_call], 'push', 'dword ptr [ebp + 0x14]')
    has(emitted[:native_call], 'mov', 'esi, dword ptr [ebp + 0x10]')
    has(emitted[:native_call], 'mov', 'edi, dword ptr [ebp + 0xc]')
    has(emitted[:native_call], 'mov', 'eax, dword ptr [ebp + 8]')
    has(emitted[:native_call], 'mov', 'eax, dword ptr [ebp + 4]')  # native return identity
    assert [(i.mnemonic, i.op_str) for i in emitted[native_call-6:native_call]] == [
        ('add', 'esp, 0x10'), ('test', 'al, al'), ('cmove', 'edx, esi'),
        ('push', 'edx'), ('push', 'edi'), ('push', 'dword ptr [esp + 0x18]')]
    has(emitted[:native_call], 'and', 'esp, 0xfffffff0')  # aligned parent for MOVAPS
    for reg in ('esi', 'edi'):
        has(emitted[:native_call], 'push', reg)
        has(emitted[native_call+1:], 'pop', reg)
    assert not any(i.mnemonic == 'add' and i.op_str.startswith('esp,')
                   for i in emitted[native_call+1:])
    return dict(status='OFFLINE_NATIVE_AND_COMPILED_ABI_PASS', contracts=contracts,
                native_call=dict(ECX='owned aging beam', args=['scene*', 'owner*', 'parent FMatrix*', 'collision int'],
                                 ret_bytes=16, actual_return_offset='children+0x161', parent_copy_bytes=64),
                component_type='RvCurvedBeamComponent : RvSharedBeamComponent : StaticMeshComponent',
                compiled_hook=hex(hook), compiled_stack_contract=dict(
                    entry='S: return, S+4 scene, S+8 owner, S+12 parent, S+16 collision; ECX=self',
                    frame='EBP=S-4, A=align_down(EBP,16), C=A-0x60 after ESI/EDI saves',
                    prepare='collision at C-4; four Prepare args at C-20..C-8, cdecl cleanup 16 -> C-4',
                    original='ESP=C-16: scene, owner, selected aligned/original parent, collision; ECX=self',
                    return_='original RET16 -> C; EDI/ESI restored; ESP=EBP; pop EBP; RET16 -> S+20'),
                emitted=[f'{i.address:08X} {i.mnemonic} {i.op_str}' for i in emitted],
                inputs={str(p): hashlib.sha256(p.read_bytes()).hexdigest() for p in (exe, archive, package, dll)},
                limitation='No live attachment receipt, GPU pixels or physical PSVR2 acceptance')


if __name__ == '__main__':
    ap = argparse.ArgumentParser(description=__doc__)
    for key in ('exe', 'text', 'package', 'dll', 'report'):
        ap.add_argument('--'+key, type=Path, required=True)
    a = ap.parse_args()
    result = audit(a.exe, a.text, a.package, a.dll)
    a.report.write_text(json.dumps(result, indent=2)+'\n', encoding='utf8')
    print(result['status'])
