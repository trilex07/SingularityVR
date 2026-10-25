"""Read-only original socket/query ABI and actual /O2 proxy audit. No game launch."""
import argparse
import hashlib
import json
from pathlib import Path
import re
from capstone import Cs, CS_ARCH_X86, CS_MODE_32
from audit_rifle_scope_backend import PE, only, registration, block_contract, helper_contract


def audit(exe, text_path, props, dll):
    pe=PE(exe.read_bytes());text=text_path.read_bytes()
    section=only([s for s in pe.sections if s[0]=='.text'],'text')
    assert len(text)==section[1], 'complete original text required'
    base=pe.base+section[2]
    md=Cs(CS_ARCH_X86,CS_MODE_32)
    wrapper=registration(pe,text,base,'intUSkeletalMeshComponentexecGetSocketWorldLocationAndRotation')
    w=block_contract(text,base,wrapper,257,0x3BF1257B240D565F)
    query=helper_contract(text,base,wrapper,0xee,304,0x2D4ED554DDC30DA5)
    rotation=helper_contract(text,base,query['target'],0xd5,282,0xBF658FB17AAD44DA)
    def ins(a,n):return list(md.disasm(text[a-base:a-base+n],a))
    q=ins(query['target'],304);wr=ins(wrapper,257)
    assert [i.op_str for i in q if i.mnemonic=='ret']==['0x10','0x10']
    assert [i.op_str for i in ins(rotation['target'],282) if i.mnemonic=='ret']==['4']
    call=only([n for n,i in enumerate(wr) if i.address==wrapper+0xee],'socket call')
    assert [(i.mnemonic,i.op_str) for i in wr[call-5:call]]==[
        ('push','ebx'),('push','ecx'),('mov','ecx, dword ptr [ebp - 4]'),('push','edx'),('push','eax')]
    for op in ['eax, dword ptr [ebx + 0xc]','ecx, dword ptr [ebx + 8]',
               'eax, dword ptr [ebx + 0x10]','esi, dword ptr [ebx + 0x14]']:
        assert any(i.mnemonic=='mov' and i.op_str==op for i in q),op
    # The original helper obtains the wrist's world matrix, multiplies original
    # relative socket matrix first, writes translation, then Matrix::Rotator.
    assert any(i.mnemonic=='call' and i.op_str=='0xad9480' for i in q)
    assert any(i.mnemonic=='call' and i.op_str=='0x4cf620' for i in q)
    assert any(i.mnemonic=='call' and i.op_str=='0x4cee40' for i in q)
    original=props.read_text(encoding='utf8')
    match=only(list(re.finditer(r'SocketName = TMD_Muzzle\s+BoneName = l_arm_wrist\s+'
        r'RelativeLocation = \{ X=5\.02409, Y=-7\.532, Z=-0\.710843 \}\s+'
        r'RelativeRotation = \{ Yaw=-3631, Pitch=-2495, Roll=1137 \}',original)),'original socket')
    assert match
    proxy=PE(dll.read_bytes()) # parser requires x86 PE32, rejects anything else
    symbols=[]
    for line in dll.with_suffix('.map').read_text(errors='replace').splitlines():
        m=re.search(r'\s(\S+)\s+([0-9a-fA-F]{8}) f\s',line)
        if m:symbols.append((int(m[2],16),m[1]))
    hook=only([a for a,s in symbols if '?Hook@NativeTmdSocket@' in s],'compiled socket hook')
    end=min(a for a,_ in symbols if a>hook)
    emitted=list(md.disasm(proxy.read(hook,end-hook),hook))
    returns=[i.op_str for i in emitted if i.mnemonic=='ret']
    assert returns and all(r=='0x10' for r in returns),returns
    # Check the ACTUAL /O2 sequence, including nonvolatile saves, not merely
    # the typedef. With entry ESP=S the saved ESI/EDI consume 8 bytes. Each
    # subsequent [esp+18h] therefore reads rot, position, number, index in turn.
    ops=[(i.mnemonic,i.op_str) for i in emitted]
    assert ops[0][0]=='mov' and re.fullmatch(r'eax, dword ptr \[0x[0-9a-f]+\]',ops[0][1])
    assert ops[1:9]==[
        ('push','esi'),('push','edi'),('push','dword ptr [esp + 0x18]'),
        ('mov','edi, ecx'),('push','dword ptr [esp + 0x18]'),
        ('push','dword ptr [esp + 0x18]'),('push','dword ptr [esp + 0x18]'),('call','eax')]
    # Native RET16 restores S-8. Optional Rewrite is cdecl: four arguments
    # followed by mesh/this, then exactly 20 bytes of caller cleanup.
    assert ops[9:12]==[('mov','esi, eax'),('test','esi, esi'),('je',hex(emitted[20].address))]
    assert ops[12:17]==[('push','dword ptr [esp + 0x18]')]*4+[('push','edi')]
    rewrite=only([a for a,s in symbols if '?Rewrite@NativeTmdSocket@' in s],'compiled socket rewrite')
    assert ops[17]==('call',hex(rewrite)) and ops[18:23]==[
        ('add','esp, 0x14'),('mov','eax, esi'),('pop','edi'),('pop','esi'),('ret','0x10')]
    assert all(i.mnemonic=='int3' for i in emitted[23:])
    # Exact complete disassembly is written only into ignored local evidence.
    # Inspect the stack sequence on each compiler update along with CPU ABI test.
    return dict(status='OFFLINE_NATIVE_AND_COMPILED_ABI_PASS',
        original_wrapper=w,native_query=query,native_rotation=rotation,
        entry_stack={'ESP+0':'return','ESP+4':'FName.index','ESP+8':'FName.number',
                     'ESP+12':'out position*','ESP+16':'optional out rotator*'},
        callee_ret_bytes=16,compiled_hook=hex(hook),compiled_ret_bytes=16,
        compiled_stack={'entry':'S','save_ESI_EDI':'S-8',
            'push_rot':'S-12 from S+16','push_position':'S-16 from S+12',
            'push_FName_number':'S-20 from S+8','push_FName_index':'S-24 from S+4',
            'CALL_entry':'S-28','native_RET16':'S-8',
            'Rewrite_optional_cdecl':'S-28 then add esp,20 -> S-8',
            'restore_EDI_ESI':'S','callback_RET16':'S+20'},
        emitted=[f'{i.address:08X} {i.mnemonic} {i.op_str}' for i in emitted],
        inputs={str(p):hashlib.sha256(p.read_bytes()).hexdigest() for p in (exe,text_path,props,dll)},
        limitation='Original and /O2 machine code + synthetic adapter tests; runtime emitter/PSVR2 pending')


if __name__=='__main__':
    ap=argparse.ArgumentParser(description=__doc__)
    for key in ('exe','text','props','dll','report'):ap.add_argument('--'+key,type=Path,required=True)
    a=ap.parse_args();result=audit(a.exe,a.text,a.props,a.dll)
    a.report.write_text(json.dumps(result,indent=2)+'\n',encoding='utf8')
    print(result['status'])
