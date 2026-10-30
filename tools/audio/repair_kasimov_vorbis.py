"""Repair pinned Kasimov/Centurion remasters offline; preserve audio and offsets.
Use unpack_chunks.exe first, then pass its logical output. Never writes Steam.
"""
import argparse,hashlib,json
from pathlib import Path
from build_native_remaster_candidate import offline_path,replace_payload,pack
from patch_opening_dialogue import bulks,require
from ue3_sound_package import inspect
from vorbis_fixed_allocation import pad_to_allocation
PINS={
    'e2d5dcae9fa7815d8ee944c4ce62c0fa2bbde60a1303f45c61221c9cab60362b':'sniperFire08',
    'a5fa0d72bb6b32d845e6d0fc89990f18d3a87a9fac444d698689d74233756d39':'arPistolFire03',
}
sha=lambda b:hashlib.sha256(b).hexdigest()
def repair(bank,logical,output):
    bank,logical,output=map(offline_path,(bank,logical,output))
    source=bank.read_bytes();asset=PINS.get(sha(source));require(asset is not None,'unreviewed source bank')
    data=logical.read_bytes();waves=[e for e in inspect(data)['exports'] if e['class_name']=='SoundNodeWave' and e['name']==asset]
    require(len(waves)==1,'ambiguous sound')
    old=bulks(data,waves[0])[1][1];new=pad_to_allocation(old,len(old))
    fixed,previous,props,offset=replace_payload(data,asset,new);require(previous==old,'payload drift')
    candidate=pack(fixed,source);output.mkdir(parents=True,exist_ok=True)
    (output/bank.name).write_bytes(candidate);(output/('fixed-'+asset+'.ogg')).write_bytes(new);(output/'fixed-logical.u').write_bytes(fixed)
    report=dict(source_sha256=sha(source),candidate_sha256=sha(candidate),asset=asset,payload_offset=offset,payload_bytes=len(old),properties=props,
        source_ogg_sha256=sha(old),candidate_ogg_sha256=sha(new),encoded_audio_unchanged=True,all_non_target_logical_bytes_unchanged=True,
        validation='Requires independent package roundtrip + shipped Vorbis header fixture; no gameplay claim')
    (output/'audio-repair.json').write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(report,indent=2))
if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__)
    for k in ('bank','logical','output'):p.add_argument('--'+k,type=Path,required=True)
    a=p.parse_args();repair(a.bank,a.logical,a.output)
