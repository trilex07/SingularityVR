"""Read-only reproducer for the knife damage/trace identity from offline RvGame.
No native code, asset bytes or game dialogue are emitted into the report.
"""
import argparse
import hashlib
import json
import struct
import sys
from pathlib import Path
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'title'))
from ue3_layout import inspect


def evidence(path):
    b=path.read_bytes()
    d=inspect(path,data=b,include_all=True)
    records=d['records']
    def ancestry(e):
        names=[e['name']]
        for _ in range(16):
            if e['outer']<=0:return '.'.join(reversed(names))
            e=records[e['outer']-1];names.append(e['name'])
        raise ValueError('outer cycle')
    events=[e for e in d['exports'] if e['class_name']=='RvCE_AttackMelee' and
            any(p['name']=='mDamageType' and p['value']=='RvDamageType_Knife' for p in e['properties'])]
    if len(events)!=1:raise ValueError('expected one uniquely identified knife event')
    event=events[0]
    def prop(e,name):
        ps=[p for p in e['properties'] if p['name']==name]
        if len(ps)!=1:raise ValueError('property missing/ambiguous '+name)
        p=ps[0];end=p['tag_offset']+p['tag_bytes']
        size=struct.unpack_from('<i',b,p['tag_offset']+16)[0]
        if p['type']=='NameProperty':return d['names'][struct.unpack_from('<i',b,end-size)[0]]
        if p['type']=='FloatProperty':return struct.unpack_from('<f',b,end-4)[0]
        return p['value']
    parent=next(e for e in d['exports'] if e['index']==event['outer'])
    return dict(source_sha256=hashlib.sha256(b).hexdigest(),path=ancestry(event),
                node=prop(parent,'mNodeName'),damage_type=prop(event,'mDamageType'),
                damage=prop(event,'mDamage'),length=prop(event,'mLength'),
                trace_bone=prop(event,'mStartBone'),bone_rotation=prop(event,'mbTraceUsingBoneRotation'),
                tag=prop(event,'mEventTag'),
                limitation='offline identity only; input-to-node runtime link and hit require MAIN L3/L4')


if __name__=='__main__':
    ap=argparse.ArgumentParser();ap.add_argument('package',type=Path)
    ap.add_argument('--output',type=Path,required=True);args=ap.parse_args()
    result=evidence(args.package)
    args.output.parent.mkdir(parents=True,exist_ok=True)
    args.output.write_text(json.dumps(result,indent=2)+'\n',encoding='utf-8')
    print(json.dumps(result,indent=2))
