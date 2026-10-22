"""Offline native hands-switch evidence; no game launch, hooks or installed writes.

Resolve script registrations and compare the archived x86 bodies, rather than
inferring rendering from names or the HANDS enum. JSON stdout is a receipt, not
a claim that the current GPU emitted a particular primitive.
"""
import argparse
import hashlib
import json
from pathlib import Path
import struct
import sys

sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'reload'))
from inspect_inventory_registration import resolve_wrapper
from inspect_native_reload import PE
import capstone

def audit(exe,archive):
    image=Path(exe).read_bytes();text=Path(archive).read_bytes();pe=PE(image)
    base=0x402000
    md=capstone.Cs(capstone.CS_ARCH_X86,capstone.CS_MODE_32)
    result={}
    # RvPlayerPawnSP vtable independently established by the inventory audit.
    table=0x019D0788
    for name,slot,wrapper,entry,size in (
        ('SwitchToTMD',0xaa8,0xf62c90,0xfd17f0,355),
        ('SwitchToWeapon',0xaac,0xf62d10,0xfd1960,542),
        ('IsTMD_Equipped',0xb04,0xf63470,0xfbe360,13),
        ('IsWeaponEquipped',0xb08,0xf634b0,0xfbe370,13)):
        reg=resolve_wrapper(pe,text,base,'intARvPlayerPawnexec'+name)
        assert reg['wrapper']==wrapper
        assert struct.unpack('<I',pe.read(table+slot,4))[0]==entry
        code=text[entry-base:entry-base+size]
        ins=list(md.disasm(code,entry))
        assert sum(i.size for i in ins)==size and ins[-1].mnemonic=='ret'
        if name.startswith('Switch'):
            assert ins[-1].op_str=='4'
            calls=[i.address for i in ins if i.mnemonic=='call' and i.op_str=='0xac7960']
            assert len(calls)==3 # native animation blend switch, NOT SetHiddenGame
        else:
            assert any(i.mnemonic=='cmp' and '[ecx + 0x105c]' in i.op_str for i in ins)
        result[name]={'slot':hex(slot),'wrapper':hex(wrapper),'entry':hex(entry),
                      'bytes':size,'sha256':hashlib.sha256(code).hexdigest(),
                      'ret':ins[-1].op_str or '0'}
    blend=text[0xac7960-base:0xac79c5-base]
    decoded=list(md.disasm(blend,0xac7960))
    assert decoded[-1].mnemonic=='ret' and decoded[-1].op_str=='4'
    assert any(i.mnemonic=='movss' and '[ecx + 0x58]' in i.op_str for i in decoded)
    return {'status':'OFFLINE_NATIVE_CONTRACT_PASS','exe_sha256':hashlib.sha256(image).hexdigest(),
            'text_sha256':hashlib.sha256(text).hexdigest(),'methods':result,
            'blend_sha256':hashlib.sha256(blend).hexdigest(),
            'conclusion':'HANDS predicates are mutually exclusive; switch toggles three native blend nodes.',
            'limits':'Does not prove GPU omission, pixels, simultaneous native fire or runtime ownership.'}

if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--exe',required=True);p.add_argument('--text',required=True)
    a=p.parse_args();print(json.dumps(audit(a.exe,a.text),indent=2))
