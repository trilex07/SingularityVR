"""Offline assembly of private Volk HD details onto the original mechanical rig.

Source assets stay outside Git. This tool only moves global original-PSK OBJ
points into their named original bone-local space; it never edits source files.
"""
import argparse
import hashlib
import json
import struct
import sys
from pathlib import Path

import numpy as np

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'reload'))
from build_volk_mechanical_glb import build
from prepare_centurion import read_psk

UNITS = 52.5
DETAILS = {
    'weapon_body_machined_fasteners': 'cog_shotgun',
    'moving_reload_part_machined_fasteners': 'Slide',
    'magazine_machined_fasteners': 'clip',
    'magazine_rib_lower': 'clip',
    'magazine_rib_middle': 'clip',
    'magazine_rib_upper': 'clip',
}


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def parse_obj(path):
    vertices, faces = [], []
    for line in path.read_text(encoding='utf-8').splitlines():
        words = line.split()
        if not words:
            continue
        if words[0] == 'v':
            vertices.append([float(v) for v in words[1:4]])
        elif words[0] == 'f':
            if len(words) != 4 or any('/' in w for w in words[1:]):
                raise ValueError(f'unreviewed OBJ face: {path}')
            faces.append([int(v)-1 for v in words[1:]])
    v = np.asarray(vertices,dtype=np.float64)
    f = np.asarray(faces,dtype=np.uint16)
    if not len(v) or not len(f) or not np.isfinite(v).all() or f.max() >= len(v):
        raise ValueError(f'bad OBJ geometry: {path}')
    return v,f


def read_glb(path):
    data = path.read_bytes()
    magic,version,total = struct.unpack_from('<4sII',data)
    if (magic,version,total) != (b'glTF',2,len(data)):
        raise ValueError('invalid mechanical GLB')
    json_size,tag = struct.unpack_from('<I4s',data,12)
    if tag != b'JSON':
        raise ValueError('missing GLB JSON')
    doc = json.loads(data[20:20+json_size])
    bin_size,tag = struct.unpack_from('<I4s',data,20+json_size)
    if tag != b'BIN\0':
        raise ValueError('missing GLB BIN')
    binary = bytearray(data[28+json_size:28+json_size+bin_size])
    return doc,binary


def add_array(doc,binary,array,kind,target):
    a=np.ascontiguousarray(array)
    while len(binary)%4:
        binary.append(0)
    offset=len(binary)
    binary.extend(a.tobytes())
    doc['bufferViews'].append({'buffer':0,'byteOffset':offset,
                               'byteLength':a.nbytes,'target':target})
    width=1 if a.ndim==1 else a.shape[1]
    acc={'bufferView':len(doc['bufferViews'])-1,
         'componentType':5126 if a.dtype==np.dtype('<f4') else 5123,
         'count':len(a),'type':{1:'SCALAR',3:'VEC3'}[width]}
    if kind=='POSITION':
        acc.update(min=a.min(axis=0).tolist(),max=a.max(axis=0).tolist())
    doc['accessors'].append(acc)
    return len(doc['accessors'])-1


def assemble(package,svwa,output):
    psk_path=package/'source/ch_inview_shotgun_mesh.psk'
    visual=json.loads((package/'clean/rig_manifest.json').read_text())
    details=json.loads((package/'clean/hd_detail_manifest.json').read_text())
    psk=read_psk(psk_path)
    names,parents,_,absolute=psk[:4]
    reflect=np.diag([1.,-1.,1.,1.])
    if visual['source_psk_sha256']!=sha(psk_path) or details['source_psk_sha256']!=sha(psk_path):
        raise ValueError('visual package PSK identity mismatch')
    if len(names)!=79 or len(visual['bones'])!=79:
        raise ValueError('source rig count mismatch')
    max_error=0.0
    for i,row in enumerate(visual['bones']):
        if (row['name'],row['parent'])!=(names[i],parents[i]):
            raise ValueError(f'original bone hierarchy mismatch at {i}')
        error=float(np.max(np.abs(reflect@absolute[i]@reflect-
                                  np.array(row['bind_matrix_row_major_uu']))))
        max_error=max(max_error,error)
    if max_error>1e-5:
        raise ValueError('original bind mismatch')
    output.mkdir(parents=True,exist_ok=True)
    mechanical=build(psk_path,svwa,output)
    doc,binary=read_glb(output/'volk_mechanical_rig.glb')
    if doc['extras']['sourcePSKSha256']!=visual['source_psk_sha256']:
        raise ValueError('mechanical rig identity mismatch')
    report={'source_psk_sha256':sha(psk_path),'source_svwa_sha256':sha(svwa),
            'visual_manifest_sha256':sha(package/'clean/rig_manifest.json'),
            'hd_detail_manifest_sha256':sha(package/'clean/hd_detail_manifest.json'),
            'max_bind_matrix_error_uu':max_error,'details':{},
            'source_material':'AR_Shotgun / AR_shotgun_d; runtime overrides require separate verification'}
    for label,bone_name in DETAILS.items():
        path=package/'clean'/f'{label}.obj'
        global_vertices,faces=parse_obj(path)
        bone=names.index(bone_name)
        inv=np.linalg.inv(reflect@absolute[bone]@reflect)
        local=(np.c_[global_vertices,np.ones(len(global_vertices))]@inv)[:,:3]/UNITS
        restored=(np.c_[local*UNITS,np.ones(len(local))] @
                  (reflect@absolute[bone]@reflect))[:,:3]
        error=float(np.max(np.abs(restored-global_vertices)))
        if error>1e-4:
            raise ValueError(f'bad bone-local roundtrip: {label}')
        p=add_array(doc,binary,local.astype('<f4'),'POSITION',34962)
        f=add_array(doc,binary,faces.reshape(-1).astype('<u2'),'indices',34963)
        mesh_index=len(doc['meshes'])
        doc['meshes'].append({'name':label,'primitives':[
            {'attributes':{'POSITION':p},'indices':f,'mode':4}]})
        node_index=len(doc['nodes'])
        doc['nodes'].append({'name':label,'mesh':mesh_index,
                             'extras':{'parentSourceBone':bone_name,
                                       'sourceOBJ':path.name}})
        doc['nodes'][bone].setdefault('children',[]).append(node_index)
        expected=details['new_geometry'].get(label.replace('_machined_fasteners',''))
        if expected is None or expected['source_bone']!=bone_name or expected['added_triangles']!=len(faces):
            raise ValueError(f'HD detail manifest mismatch: {label}')
        report['details'][label]={'source_bone':bone_name,'source_sha256':sha(path),
                                   'vertices':len(local),'triangles':len(faces),
                                   'max_local_roundtrip_error_uu':error}
    doc['buffers'][0]['byteLength']=len(binary)
    doc['extras']['hdDetails']='Original PSK bind, separate source-bone rigid detail meshes; no new bones'
    payload=json.dumps(doc,separators=(',',':')).encode()
    payload+=b' '*((-len(payload))%4)
    binary.extend(b'\0'*((-len(binary))%4))
    glb=(struct.pack('<4sII',b'glTF',2,12+8+len(payload)+8+len(binary))+
         struct.pack('<I4s',len(payload),b'JSON')+payload+
         struct.pack('<I4s',len(binary),b'BIN\0')+binary)
    (output/'volk_s4_hd_mechanical_untextured.glb').write_bytes(glb)
    report['untextured_glb_sha256']=hashlib.sha256(glb).hexdigest()
    report['mechanical_manifest_sha256']=sha(output/'animation_socket_manifest.json')
    (output/'assembly_manifest.json').write_text(json.dumps(report,indent=2)+'\n')
    return report


if __name__=='__main__':
    ap=argparse.ArgumentParser(description=__doc__)
    for key in ('package','svwa','output'):
        ap.add_argument('--'+key,required=True,type=Path)
    args=ap.parse_args()
    print(json.dumps(assemble(args.package,args.svwa,args.output),indent=2))
