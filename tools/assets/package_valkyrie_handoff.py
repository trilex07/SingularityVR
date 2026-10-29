"""Assemble and hash the private Valkyrie HD handoff; no Steam/runtime writes."""
import argparse
import hashlib
import json
import shutil
import struct
from pathlib import Path

from PIL import Image
import numpy as np


def sha(path):return hashlib.sha256(path.read_bytes()).hexdigest()


def copy(source,destination):
    destination.parent.mkdir(parents=True,exist_ok=True)
    if source.resolve()!=destination.resolve():shutil.copy2(source,destination)


def glb_doc(path):
    raw=path.read_bytes()
    magic,version,total=struct.unpack_from('<4sII',raw)
    if (magic,version,total)!=(b'glTF',2,len(raw)):raise ValueError('bad GLB '+path.name)
    size,kind=struct.unpack_from('<I4s',raw,12)
    if kind!=b'JSON':raise ValueError('missing GLB JSON')
    return json.loads(raw[20:20+size])


def main():
    p=argparse.ArgumentParser();p.add_argument('--root',type=Path,required=True)
    a=p.parse_args();base=a.root.resolve();visual=base/'visual';mechanical=base/'mechanical';out=base/'final_handoff'
    for rel in ('source','maps','previews','manifests'):(out/rel).mkdir(parents=True,exist_ok=True)
    required=(out/'valkyrie_ar9_hd_animated.blend',out/'valkyrie_ar9_hd_animated.glb',
              out/'valkyrie_ar9_hd_magazine.glb',out/'valkyrie_hd_mechanical_untextured.glb',
              out/'blender_visual_validation.json',out/'blender_import_validation.json',
              out/'visual_assembly_manifest.json')
    if any(not f.is_file() for f in required):raise ValueError('Blender final outputs incomplete')
    for source in (visual/'source').iterdir():
        if source.is_file():copy(source,out/'source'/source.name)
    copy(mechanical/'source/valkyrie_reload.svwa',out/'source/valkyrie_reload.svwa')
    copy(mechanical/'valkyrie_mechanical_rig.glb',out/'source/valkyrie_mechanical_rig.glb')
    for source in (visual/'maps').glob('*.png'):copy(source,out/'maps'/source.name)
    for source in (visual/'clean').glob('*.json'):copy(source,out/'manifests'/source.name)
    for name in ('animation_socket_manifest.json','blender_mechanical_validation.json',
                 'mechanical_sha256_manifest.json'):
        copy(mechanical/name,out/'manifests'/name)
    for name in ('visual_assembly_manifest.json','blender_visual_validation.json',
                 'blender_import_validation.json'):
        copy(out/name,out/'manifests'/name)
    original=Image.open(out/'original_bind.png').convert('RGB')
    hd=Image.open(out/'hd_bind.png').convert('RGB')
    if original.size!=hd.size:raise ValueError('comparison camera size changed')
    pair=Image.new('RGB',(original.width*2,original.height),(32,32,32))
    pair.paste(original,(0,0));pair.paste(hd,(original.width,0))
    pair.save(out/'previews/original_vs_hd_identical_lighting.png')
    for name in ('original_bind.png','hd_bind.png','hd_reload_frame22.png','hd_magazine_inspection.png'):
        copy(out/name,out/'previews'/name)
    rig=json.loads((out/'manifests/rig_manifest.json').read_text())
    mech=json.loads((out/'manifests/animation_socket_manifest.json').read_text())
    blender=json.loads((out/'manifests/blender_import_validation.json').read_text())
    if (rig['source_psk_sha256']!=mech['source_psk_sha256'] or
        rig['source_psk_sha256']!=sha(out/'source/ch_inview_machinegun_mesh.psk') or
        mech['source_svwa_sha256']!=sha(out/'source/valkyrie_reload.svwa') or
        blender['status']!='OFFLINE_BLENDER_PASS'):
        raise ValueError('source or Blender gate mismatch')
    if sum(x['triangles'] for x in rig['objects'].values())!=12842:
        raise ValueError('original face count mismatch')
    animated=glb_doc(out/'valkyrie_ar9_hd_animated.glb')
    mag=glb_doc(out/'valkyrie_ar9_hd_magazine.glb')
    if (len(animated.get('meshes',[]))!=9 or len(animated.get('animations',[]))!=1 or
        animated['animations'][0]['name']!='Reload_source_52f' or
        len(animated.get('images',[]))!=3 or len(mag.get('meshes',[]))!=2):
        raise ValueError('final glTF content gate failed')
    if sum('TANGENT' in m['primitives'][0]['attributes'] for m in animated['meshes'])!=6:
        raise ValueError('source mesh tangent gate failed')
    normal=np.asarray(Image.open(out/'maps/valkyrie_hd_normal_gltf_4k.png'),dtype=np.float32)/127.5-1
    if normal.shape!=(2048,4096,3) or np.max(np.abs(np.linalg.norm(normal,axis=2)-1))>.02:
        raise ValueError('invalid converted normal map')
    file_hashes={str(f.relative_to(out)).replace('\\','/'):sha(f)
                 for f in sorted(out.rglob('*')) if f.is_file() and f.name!='final_handoff_manifest.json'}
    report={'status':'OFFLINE_BLENDER_PASS','source_psk_sha256':rig['source_psk_sha256'],
            'source_svwa_sha256':mech['source_svwa_sha256'],
            'original_faces':12842,'hd_detail_faces':sum(x['triangles'] for x in
                 json.loads((out/'manifests/hd_detail_manifest.json').read_text())['details'].values()),
            'bone_count':80,'source_reload_frames':52,'source_reload_fps':24,
            'final_meshes':9,'detached_magazine_meshes':2,
            'normal_length_max_error':float(np.max(np.abs(np.linalg.norm(normal,axis=2)-1))),
            'native_integration':'MAIN must adapt visual GLB/materials to UE3 and verify socket/material route; no runtime write performed',
            'source_limitation':'Original MachineGun.psa unavailable locally; validated PSA-derived SVWA included',
            'files_sha256':file_hashes}
    (out/'final_handoff_manifest.json').write_text(json.dumps(report,indent=2)+'\n',encoding='utf-8')
    print(json.dumps({k:v for k,v in report.items() if k!='files_sha256'},indent=2))


if __name__=='__main__':main()
