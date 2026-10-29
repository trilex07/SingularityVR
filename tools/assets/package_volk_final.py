"""Hash and gate the local private Volk final handoff; no runtime writes."""
import argparse
import hashlib
import json
import struct
from pathlib import Path

import numpy as np
from PIL import Image

REQUIRED = (
    'volk_s4_hd_animated.blend','volk_s4_hd_animated.glb',
    'volk_s4_hd_magazine.glb','volk_s4_hd_mechanical_untextured.glb',
    'animation_socket_manifest.json','assembly_manifest.json',
    'blender_validation.json','blender_pose_validation.json',
    'normal_conversion_manifest.json','source/ch_inview_shotgun_mesh.psk',
    'source/volk_reload.svwa','maps/volk_s4_hd_albedo_4k.png',
    'maps/volk_s4_hd_normal_gltf_4k.png','maps/volk_s4_hd_roughness_4k.png',
    'maps/volk_s4_hd_metallic_4k.png','maps/volk_s4_hd_height_4k.png',
    'previews/original_vs_hd_side.png','previews/original_vs_hd_reload_close.png',
    'previews/material_detail_original_vs_hd.png',
    'animated_inspection_bind.png','animated_inspection_slide_move.png',
    'animated_inspection_mag_seat.png')


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def glb(path):
    data=path.read_bytes()
    magic,version,size=struct.unpack_from('<4sII',data)
    if (magic,version,size)!=(b'glTF',2,len(data)):
        raise ValueError('invalid GLB '+path.name)
    n,tag=struct.unpack_from('<I4s',data,12)
    if tag!=b'JSON':
        raise ValueError('missing GLB JSON')
    doc=json.loads(data[20:20+n])
    binary=data[28+n:]
    return doc,binary


def accessor(doc,binary,index):
    a=doc['accessors'][index]
    v=doc['bufferViews'][a['bufferView']]
    width={'SCALAR':1,'VEC2':2,'VEC3':3,'VEC4':4}[a['type']]
    dtype={5126:'<f4',5123:'<u2',5125:'<u4'}[a['componentType']]
    return np.frombuffer(binary,dtype,a['count']*width,
                         v.get('byteOffset',0)+a.get('byteOffset',0)).reshape(-1,width)


def package(root):
    for relative in REQUIRED:
        if not (root/relative).is_file():
            raise ValueError('missing final artifact '+relative)
    assembly=json.loads((root/'assembly_manifest.json').read_text())
    mechanical=json.loads((root/'animation_socket_manifest.json').read_text())
    pose=json.loads((root/'blender_pose_validation.json').read_text())
    if (assembly['source_psk_sha256']!=sha(root/'source/ch_inview_shotgun_mesh.psk') or
        mechanical['source_svwa_sha256']!=sha(root/'source/volk_reload.svwa') or
        not pose['detached_magazine_independent_0p1m'] or
        not pose['root_stable']):
        raise ValueError('source or Blender pose gate failed')
    doc,binary=glb(root/'volk_s4_hd_animated.glb')
    if len(doc.get('meshes',[]))!=11 or len(doc.get('animations',[]))!=6 or len(doc.get('images',[]))!=3:
        raise ValueError('HD GLB count gate failed')
    if set(a['name'] for a in doc['animations'])!=set(pose['actions']):
        raise ValueError('HD GLB action gate failed')
    source_meshes={'volk_body','volk_reload_part_01','volk_magazine','volk_slide','volk_trigger'}
    tangent_checks={}
    for mesh in doc['meshes']:
        if mesh['name'] not in source_meshes:
            continue
        attr=mesh['primitives'][0]['attributes']
        if not all(k in attr for k in ('POSITION','NORMAL','TEXCOORD_0','TANGENT')):
            raise ValueError('source mesh lacks normal/tangent '+mesh['name'])
        tangent=accessor(doc,binary,attr['TANGENT'])
        normals=accessor(doc,binary,attr['NORMAL'])
        if not np.isfinite(tangent).all() or not np.isfinite(normals).all() or \
           not np.isin(tangent[:,3],[-1,1]).all():
            raise ValueError('invalid tangent handedness '+mesh['name'])
        err=float(np.max(np.abs(np.linalg.norm(tangent[:,:3],axis=1)-1)))
        if err>.001:
            raise ValueError('nonunit tangent '+mesh['name'])
        tangent_checks[mesh['name']]={'vertices':len(tangent),
                                     'handedness':sorted(float(x) for x in np.unique(tangent[:,3])),
                                     'max_length_error':err}
    if set(tangent_checks)!=source_meshes:
        raise ValueError('missing source mesh tangent check')
    native=np.asarray(Image.open(root.parent/'textures/volk_s4_hd_normal_4k.png').convert('RGB'))
    converted=np.asarray(Image.open(root/'maps/volk_s4_hd_normal_gltf_4k.png').convert('RGB'))
    if not (np.array_equal(converted[:,:,0],native[:,:,0]) and
            np.array_equal(converted[:,:,1],255-native[:,:,1]) and
            np.array_equal(converted[:,:,2],native[:,:,2])):
        raise ValueError('normal green conversion mismatch')
    hashes={relative:sha(root/relative) for relative in REQUIRED}
    report={'status':'OFFLINE_BLENDER_PASS','blender_version':pose['blender_version'],
            'source_psk_sha256':assembly['source_psk_sha256'],
            'source_svwa_sha256':mechanical['source_svwa_sha256'],
            'bind_max_error_uu':assembly['max_bind_matrix_error_uu'],
            'source_triangles':10835,'hd_detail_triangles':1892,
            'actions':list(pose['actions']),'root_stable':True,
            'detachable_magazine':True,'tangent_checks':tangent_checks,
            'files_sha256':hashes,
            'limits':'No Steam/PSVR2 runtime test; UE3 geometry/shader path not integrated'}
    (root/'final_handoff_manifest.json').write_text(json.dumps(report,indent=2)+'\n')
    return report


if __name__=='__main__':
    ap=argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--root',required=True,type=Path)
    args=ap.parse_args()
    result=package(args.root)
    print(json.dumps({k:result[k] for k in ('status','bind_max_error_uu','actions','root_stable','detachable_magazine')},indent=2))
