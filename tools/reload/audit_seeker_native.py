"""Read-only archived native-call and compiled proxy ABI regression; no game access.

Explicit private paths are required. Missing evidence fails, never counts as PASS.
Reports contracts/hashes, not proprietary function bodies.
"""
import argparse
import hashlib
import json
import re
from pathlib import Path
from capstone import Cs, CS_ARCH_X86, CS_MODE_32
from inspect_native_reload import PE, u32


def fnv(data):
    h = 0xcbf29ce484222325
    for b in data:
        h = ((h ^ b) * 0x100000001b3) & 0xffffffffffffffff
    return h


def audit(exe, text, dll):
    pe = PE(exe.read_bytes())
    text_base, = [pe.base+rva for name, _, rva, _, _ in pe.sections if name == '.text']
    assert text_base == 0x402000
    raw = text.read_bytes()
    md = Cs(CS_ARCH_X86, CS_MODE_32)
    def body(a, n):
        p = a-text_base
        assert 0 <= p and p+n <= len(raw)
        return raw[p:p+n]
    def ops(a, n):
        return list(md.disasm(body(a,n),a))
    def signature(a,n,h):
        assert fnv(body(a,n)) == h, f'body mismatch {a:x}'
    contracts = [
        ('SwitchWeapon',0x1047220,447,0xBD62E75D7EBD62DB),
        ('SelectOwned',0x102f650,168,0xCB797F59504492C3),
        ('RocketProjectileFire',0xe87ba0,26,0x5B3CC29BF4ACD7E6),
        ('ProjectileFire',0xe86da0,558,0xB4AEC94DBDB7CE21),
        ('PhysicalFireStart',0xe6a1a0,1110,0x0A70C4D3EEE25B1C),
        ('GetAdjustedAim',0xe87580,213,0x538700ED62A99B3E),
        ('SpawnProjectile',0xe8fcc0,294,0x3DC3105D1A3C1A2B)]
    for _, a,n,h in contracts:
        signature(a,n,h)
    # The exact heavy getter, native constructor/vtable and two call sites.
    rocket_vt=0x1931648
    heavy=u32(pe.read(rocket_vt+0x554,4),0)
    signature(heavy,13,0xAD9AEEB598427D84)
    assert body(heavy,13) == bytes.fromhex('8b81d8020000c1e80783e001c3')
    assert u32(pe.read(0x19d4c30+0x6b8,4),0)==0x1047220
    assert u32(pe.read(0x19d0788+0xb14,4),0)==0x102f650
    assert u32(pe.read(0x19d0788+0xb18,4),0)==0x102f700
    for slot,a in [(0x504,0xe87ba0),(0x440,0xe6a1a0),(0x430,0xe87580),(0x598,0xe8fcc0)]:
        assert u32(pe.read(rocket_vt+slot,4),0)==a
    # Independent binary oracle for Actor.Instigator, not a synthetic adapter offset.
    assert pe.read(0x18236bc,22).decode('utf-16le').rstrip('\0') == 'Instigator'
    assert body(0x8b64ae,5) == bytes.fromhex('68bc368201')
    assert body(0x8b64fb,6) == bytes.fromhex('8b82cc000000')
    assert body(0xe6a1fb,6) == bytes.fromhex('8b8ecc000000')
    sw=ops(0x1047220,447)
    sites=[]
    for i,x in enumerate(sw):
        if x.address in (0x10472f0,0x1047307):
            assert x.mnemonic=='call' and x.op_str=='eax'
            assert any(y.mnemonic=='mov' and y.op_str=='eax, dword ptr [edx + 0x554]' for y in sw[max(0,i-5):i]) or any(
                y.mnemonic=='mov' and y.op_str=='eax, dword ptr [eax + 0x554]' for y in sw[max(0,i-5):i])
            sites.append(x.address+x.size-0x1047220)
    assert sites==[0xd2,0xe9]
    assert any(x.mnemonic=='call' and x.op_str=='0x104e500' for x in sw)
    select=ops(0x102f650,168)
    assert [x.op_str for x in select if x.mnemonic=='ret']==['8','8']
    assert any('+ 0x42c]' in x.op_str for x in select)
    # Native origin = hidden result pointer + by-value FVector: ret16.
    for a,n,cleanup in [(0xe6a1a0,1110,'0x10'),(0xe87580,213,'0x10'),(0xe8fcc0,294,'0xc')]:
        ret=[x.op_str for x in ops(a,n) if x.mnemonic=='ret']
        assert ret and all(x==cleanup for x in ret),(hex(a),ret)
    fire={x.address:x for x in ops(0xe86da0,558)}
    assert (fire[0xe86e05].mnemonic,fire[0xe86e05].op_str)==('sub','esp, 0xc')
    assert (fire[0xe86e23].mnemonic,fire[0xe86e24].op_str)==('push','ecx, esi')
    for caller,return_offset in [(0xe86e26,0x88),(0xe86f7b,0x1dd)]:
        x=fire[caller];assert x.mnemonic=='call' and x.address+x.size==0xe86da0+return_offset
    aim={x.address:x for x in ops(0xe87580,213)}
    assert aim[0xe87648].mnemonic=='call' and aim[0xe87648].address+aim[0xe87648].size==0xe87580+0xca
    # Actual /O2 proxy instructions. Bound each symbol by the next map address;
    # trailing alignment/data excluded by disassembling reachable function body.
    proxy=PE(dll.read_bytes())
    mapping=dll.with_suffix('.map').read_text()
    symbols=[]
    for line in mapping.splitlines():
        m=re.search(r'\s(\S+)\s+([0-9a-fA-F]{8}) f\s',line)
        if m: symbols.append((int(m[2],16),m[1]))
    a, = [a for a,s in symbols if s.startswith('?Read@NativeSeekerSlot')]
    end=min(v for v,_ in symbols if v>a)
    ownership=list(md.disasm(proxy.read(a,end-a),a))
    assert any(x.mnemonic=='cmp' and x.op_str=='eax, 0xcc' for x in ownership)
    assert any(x.mnemonic=='lea' and '+ 0xcc]' in x.op_str for x in ownership)
    assert not any('+ 0x98]' in x.op_str for x in ownership)
    compiled={}
    for name,cleanup in [('StartHook@NativeSeekerProjectile','0x10'),('SpawnHook@NativeSeekerProjectile','0xc'),
                         ('FireHook@NativeSeekerProjectile',''),('Heavy@NativeSeekerSlot','')]:
        a,= [a for a,s in symbols if '?'+name+'@' in s]
        end=min(v for v,_ in symbols if v>a)
        ins=list(md.disasm(proxy.read(a,end-a),a))
        rets=[x.op_str for x in ins if x.mnemonic=='ret']
        assert rets and all(r==cleanup for r in rets),(name,rets)
        compiled[name]={'va':hex(a),'ret_cleanup':cleanup or '0','ret_count':len(rets)}
    return dict(instigator_offset="0xCC (Actor named serialization and native fire)",native_text_base=hex(text_base),switch_return_offsets=sites,
        native_contracts=[dict(name=name,va=hex(a),bytes=n,fnv64=f'{h:016X}') for name,a,n,h in contracts],
        compiled=compiled,input_sha256={p.name:hashlib.sha256(p.read_bytes()).hexdigest() for p in (exe,text,dll)},
        limitation='Offline native and emitted x86 contracts, not live selection or headset proof')


if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__)
    for flag in ('exe','text','dll','report'):p.add_argument('--'+flag,type=Path,required=True)
    a=p.parse_args();result=audit(a.exe,a.text,a.dll)
    a.report.write_text(json.dumps(result,indent=2)+'\n')
    print(json.dumps(result,indent=2))
