"""Offline original-pistol FX/light ownership. No extraction/deploy or asset edits."""
import argparse
import hashlib
import json
import struct
import sys
from pathlib import Path
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'title'))
from ue3_layout import inspect

def array_refs(data,prop,exports):
    if prop['type']!='ArrayProperty':raise ValueError('Not a tagged array')
    size=prop['value']['bytes'];at=prop['tag_offset']+24
    if size<4 or at<0 or at+size>len(data):raise ValueError('Array bounds')
    count=struct.unpack_from('<I',data,at)[0]
    if count>16 or size!=4+4*count:raise ValueError('Not a bounded object reference array')
    result=[]
    for i in range(count):
        ref=struct.unpack_from('<i',data,at+4+i*4)[0]
        if ref<=0 or ref>len(exports):raise ValueError('Expected local object reference; do not guess external import')
        entry=exports[ref-1]
        result.append(dict(index=ref,name=entry['name'],class_name=entry['class_name'],outer=entry['outer']))
    return result

def inspect_fx(path):
    data=Path(path).read_bytes()
    if len(data)>32*1024*1024:raise ValueError('Offline package cap')
    result=inspect('memory',data,include_all=True);exports=result['exports'];rows=[]
    for entry in exports:
        if entry.get('parse_error') is not None:continue
        props=entry['properties'];lights=[]
        for prop in props:
            if prop['name']=='mMuzzleFlashLights':lights=array_refs(data,prop,exports)
        if lights or entry['class_name']=='RvWeaponFX' or 'light' in entry['class_name'].lower():
            rows.append(dict(index=entry['index'],name=entry['name'],class_name=entry['class_name'],outer=entry['outer'],
                             muzzle_lights=lights,properties=props))
    return dict(status='OFFLINE_ASSET_OWNERSHIP_NOT_LIVE_DRAW_PROOF',source_sha256=hashlib.sha256(data).hexdigest(),records=rows)

if __name__=='__main__':
    ap=argparse.ArgumentParser();ap.add_argument('package',type=Path);a=ap.parse_args()
    print(json.dumps(inspect_fx(a.package),indent=2))
