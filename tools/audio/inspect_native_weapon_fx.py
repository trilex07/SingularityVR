"""Offline identities from the user's original decoded weapon package; no install.

Records cue/component/wave names and properties, not proof of live playback.
Does not extract or generate replacement sounds or effects.
"""
import argparse
import hashlib
import json
import sys
from pathlib import Path
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'title'))
from ue3_layout import inspect

def main():
    ap=argparse.ArgumentParser()
    ap.add_argument('--input',type=Path,required=True)
    ap.add_argument('--report',type=Path,required=True)
    a=ap.parse_args()
    if a.report.exists():raise ValueError('Refuse existing report')
    data=a.input.read_bytes()
    if len(data)>32*1024*1024:raise ValueError('Package cap')
    parsed=inspect('memory',data,include_all=True)
    classes={'RvWeaponShared','RvWeaponFX','ParticleSystem','SoundCue','RSoundNodeCue','SoundNodeWave'}
    rows=[]
    for e in parsed['exports']:
        if e.get('class_name') not in classes:continue
        props=[p for p in e.get('properties',[]) if p['type']=='ObjectProperty' or
               any(k in p['name'].lower() for k in ('sound','muzzle','shell','smoke','fire','sample','channel','duration'))]
        rows.append(dict(name=e['name'],index=e['index'],outer=e['outer'],kind=e['class_name'],
                         properties=props,parse_error=e.get('parse_error')))
    result=dict(status='OFFLINE_IDENTITY_ONLY',source=str(a.input.resolve()),
        sha256=hashlib.sha256(data).hexdigest(),records=rows,
        note='Stock assets found, native component/cue ownership and per-eye output still require runtime evidence; no game writes')
    a.report.parent.mkdir(parents=True,exist_ok=True)
    a.report.write_text(json.dumps(result,ensure_ascii=False,indent=2),encoding='utf-8')
    print('PASS: bounded offline weapon report; records='+str(len(rows))+'; no game writes')
if __name__=='__main__':main()
