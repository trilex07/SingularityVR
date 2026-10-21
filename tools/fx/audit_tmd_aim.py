"""Offline TMD aim ABI/ownership audit. No process access or proprietary output."""
import argparse
import hashlib
import json
from pathlib import Path
import struct
from audit_deadlock_backend import constructor
from audit_rifle_scope_backend import PE, only, block_contract, body, registration


def audit(exe, archive):
    pe=PE(Path(exe).read_bytes())
    text=Path(archive).read_bytes()
    section=only([s for s in pe.sections if s[0]=='.text'], 'text')
    if len(text)!=section[1]:
        raise ValueError('complete original archive required')
    base=pe.base+section[2]
    cls=constructor(pe,text,base,'RvTMD',63,0xF2C5B1B22F49E767)
    def ptr(address):
        return struct.unpack('<I',pe.read(address,4))[0]
    owner=ptr(cls['vtable']+0x43c)
    aim=ptr(cls['vtable']+0x440)
    ctor=text[cls['address']-base:cls['address']-base+63]
    # Full constructor fingerprint above proves this secondary-interface store.
    marker=bytes.fromhex('c786e0010000')
    slots=[i for i in range(len(ctor)-9) if ctor[i:i+6]==marker]
    if len(slots)!=2:
        raise ValueError('secondary interface constructor relationship')
    secondary=struct.unpack_from('<I',ctor,slots[-1]+6)[0]
    aging=ptr(secondary+12)
    receipts={}
    for key,address,n,expected in (
        ('owner',owner,7,0x940DBEC1332B5EEE),
        ('aim',aim,119,0x19B6BABB20248999),
        ('aging',aging,59,0x73C3C6D548B0AF74)):
        receipts[key]=block_contract(text,base,address,n,expected)
    if text[owner-base:owner-base+7]!=bytes.fromhex('8b8198000000c3'):
        raise ValueError('owner field mechanism')
    for key,name,slot in (
        ('aim_wrapper','intARvTMDexecGetAimVectors',0x440),
        ('aging_wrapper','intARvTMDexecGetAgingVectors',12)):
        address=registration(pe,text,base,name)
        ins,receipt=body(text,base,address)
        n,expected=(223,0xE88344532422184A) if slot==0x440 else (235,0xEE4912DC7241017F)
        block_contract(text,base,address,n,expected)
        if receipt['bytes']!=n:
            raise ValueError('complete wrapper extent')
        if ins[-1].mnemonic!='ret' or ins[-1].op_str!='8':
            raise ValueError('two reference ABI epilogue')
        # Primary getter wrapper or secondary interface wrapper must contain
        # the measured virtual dispatch displacement, not a same-name guess.
        dispatch=('edx, dword ptr [edx + 0x440]' if slot==0x440 else
                  'edx, dword ptr [edx + 0xc]')
        if not any(i.mnemonic=='mov' and i.op_str==dispatch for i in ins):
            raise ValueError('getter wrapper dispatch')
        receipts[key]=receipt
    return dict(status='OFFLINE_STRUCTURAL_PASS',contracts=receipts,
        exe_sha256=hashlib.sha256(pe.data).hexdigest(),text_sha256=hashlib.sha256(text).hexdigest(),
        primary_slot=0x440,owner_slot=0x43c,owner_offset=0x98,
        aging_this_adjust=0x1e0,aging_slot=12,
        mechanism='owner viewpoint -> forward; aging copies cached source then nests aim getter',
        limitation='No Steam execution, native emitter pixel or PSVR2 acceptance')


if __name__=='__main__':
    ap=argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--exe',required=True)
    ap.add_argument('--text',required=True)
    ap.add_argument('--report',required=True)
    a=ap.parse_args()
    result=audit(a.exe,a.text)
    Path(a.report).write_text(json.dumps(result,indent=2)+'\n',encoding='utf8')
    print(result['status'])
