"""Check private Valkyrie mechanical handoff and write its SHA-256 manifest."""
import argparse
import hashlib
import json
import struct
from pathlib import Path


def sha(path): return hashlib.sha256(path.read_bytes()).hexdigest()


def main(root):
    psk=root/'source'/'ch_inview_machinegun_mesh.psk'
    svwa=root/'source'/'valkyrie_reload.svwa'
    glb=root/'valkyrie_mechanical_rig.glb'
    report=root/'animation_socket_manifest.json'
    blender=root/'blender_mechanical_validation.json'
    data=glb.read_bytes()
    if struct.unpack_from('<4sII',data)!=(b'glTF',2,len(data)): raise ValueError('bad GLB')
    size,tag=struct.unpack_from('<I4s',data,12)
    if tag!=b'JSON': raise ValueError('missing JSON')
    doc=json.loads(data[20:20+size])
    expected={'weapon_body','magazine','trigger','moving_reload_part','fan','clip_release'}
    mesh_names={m['name'] for m in doc['meshes']}
    if mesh_names!=expected or len(doc['nodes'])!=80+6: raise ValueError('wrong rig/parts')
    if len(doc['animations'])!=1 or doc['animations'][0]['name']!='Reload_source_52f':
        raise ValueError('wrong animation')
    moving={x['target']['node'] for x in doc['animations'][0]['channels']}
    names=[n['name'] for n in doc['nodes']]
    if moving!={names.index(n) for n in ('clip','Trigger','Slide','fan','clip_release')}:
        raise ValueError('root or unknown bone is animated')
    r=json.loads(report.read_text())
    b=json.loads(blender.read_text())
    if sha(psk)!=r['source_psk_sha256'] or sha(svwa)!=r['source_svwa_sha256'] or sha(glb)!=r['glb_sha256']:
        raise ValueError('source/hash mismatch')
    if b['status']!='BLENDER_IMPORT_EXPORT_PASS' or not b['root_stable']:
        raise ValueError('Blender validation missing')
    paths=(psk,svwa,glb,report,blender,root/'valkyrie_mechanical_blender_roundtrip.glb',
           root/'valkyrie_mechanical_preview.png')
    manifest={'status':'OFFLINE_MECHANICAL_PASS','hashes_sha256':
              {str(p.relative_to(root)).replace('\\','/'):sha(p) for p in paths},
              'source_bones':80,'source_faces':12842,'source_reload_frames':52,
              'root_animation_channels':0,'blender_import_export':'PASS',
              'original_psa_file':'UNAVAILABLE; validated PSA-derived SVWA included'}
    (root/'mechanical_sha256_manifest.json').write_text(json.dumps(manifest,indent=2)+'\n')
    print(json.dumps(manifest,indent=2))


if __name__=='__main__':
    a=argparse.ArgumentParser();a.add_argument('--package',required=True,type=Path)
    main(a.parse_args().package)
