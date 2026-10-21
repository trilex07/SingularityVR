"""Read-only audit of owned game binaries. No process access or game writes.

This proves the VM bit-reader and script producer, not which bit is active
during Kathryn's scene. Keep extracted game packages outside Git.
"""
import argparse
import hashlib
import json
from pathlib import Path
import struct
import sys
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'title'))
from ue3_layout import inspect

def pe_code(path,rva,length):
    b=Path(path).read_bytes()
    assert b[:2]==b'MZ'
    pe=struct.unpack_from('<I',b,0x3c)[0]
    assert b[pe:pe+4]==b'PE\0\0'
    machine,count=struct.unpack_from('<HH',b,pe+4)
    opt=struct.unpack_from('<H',b,pe+20)[0]
    assert machine==0x14c and struct.unpack_from('<H',b,pe+24)[0]==0x10b
    for n in range(count):
        sec=pe+24+opt+n*40
        _,va,size,raw=struct.unpack_from('<4I',b,sec+8)
        if va<=rva and rva+length<=va+size:
            return b[raw+rva-va:raw+rva-va+length]
    raise ValueError('VM reader outside raw PE32 sections')

def audit(exe,engine,raven,text_dump):
    # Steam's on-disk .text is packed. Use the existing read-only runtime dump;
    # do not mistake encrypted disk bytes for the loaded opcode implementation.
    assert struct.unpack('<I',pe_code(exe,0x17b808c,4))[0]==0x48bce0
    text=Path(text_dump).read_bytes()
    reader=text[0x48bce0-0x402000:0x48bce0-0x402000+82]
    assert reader[:12]==bytes.fromhex('55 8b ec 8b 45 08 56 8b f1 8b 48 18')
    assert reader[67:82]==bytes.fromhex('8b 97 84 00 00 00 85 11 74 0d b9 01 00 00 00')
    results=[]
    for path,owner in ((engine,'PlayerController'),(raven,'RvPlayerController')):
        b=Path(path).read_bytes();d=inspect(path,include_all=True)
        exports=d['records']
        def ref_name(ref):
            return exports[ref-1]['name'] if ref>0 else d['names'][d['imports'][-ref-1][5]] if ref else 'None'
        functions=[]
        for e in d['exports']:
            if e['name'] not in ('ClientSetCinematicMode','SetCinematicMode') or ref_name(e['outer'])!=owner:continue
            assert e['class_name']=='Function'
            length=struct.unpack_from('<I',b,e['offset']+40)[0]
            code=b[e['offset']+44:e['offset']+44+length]
            assert len(code)==length
            if e['name']=='ClientSetCinematicMode':
                assert code[:3]==b'\x14\x2d\x01' # LetBool -> instance BoolVariable
                target=struct.unpack_from('<i',code,3)[0]
                assert ref_name(target)=='bCinematicMode'
            functions.append(dict(export=e['index'],name=e['name'],script_bytes=length,
                                  script_sha256=hashlib.sha256(code).hexdigest()))
        assert {f['name'] for f in functions}=={'ClientSetCinematicMode','SetCinematicMode'}
        results.append(dict(package=Path(path).name,sha256=hashlib.sha256(b).hexdigest(),functions=functions))
    return dict(status='OFFLINE_PRODUCER_PASS',exe_sha256=hashlib.sha256(Path(exe).read_bytes()).hexdigest(),
                text_dump_sha256=hashlib.sha256(text).hexdigest(),
                reader_rva='0008BCE0',mask_member='84',packages=results,
                physical_validation=False)

if __name__=='__main__':
    ap=argparse.ArgumentParser();ap.add_argument('--exe',required=True);ap.add_argument('--engine',required=True);ap.add_argument('--raven',required=True);ap.add_argument('--text-dump',required=True)
    a=ap.parse_args();print(json.dumps(audit(a.exe,a.engine,a.raven,a.text_dump),indent=2))
