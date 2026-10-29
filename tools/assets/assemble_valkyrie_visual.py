"""Attach Valkyrie HD detail meshes to original source-bone nodes in glTF."""
import argparse
import hashlib
import json
import struct
import sys
from pathlib import Path

import numpy as np

from assemble_volk_final import read_glb,parse_obj,add_array
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'reload'))
from prepare_centurion import read_psk

UNITS=52.5


def sha(path):return hashlib.sha256(path.read_bytes()).hexdigest()


def assemble(visual,mechanical,output):
    rig=json.loads((visual/'clean/rig_manifest.json').read_text())
    detail=json.loads((visual/'clean/hd_detail_manifest.json').read_text())
    psk_path=visual/'source/ch_inview_machinegun_mesh.psk'
    if rig['source_psk_sha256']!=sha(psk_path) or detail['source_psk_sha256']!=sha(psk_path):
        raise ValueError('visual source mismatch')
    names,parents,_,absolute=read_psk(psk_path)[:4]
    if len(names)!=80 or len(rig['bones'])!=80:raise ValueError('rig count mismatch')
    reflect=np.diag([1.,-1.,1.,1.]);raw_bind=reflect@absolute@reflect
    for i,row in enumerate(rig['bones']):
        if row['name']!=names[i] or row['parent']!=parents[i] or not np.allclose(
                row['bind_matrix_row_major_uu'],raw_bind[i],rtol=0,atol=1e-5):
            raise ValueError(f'bind mismatch at {i}')
    doc,binary=read_glb(mechanical)
    if doc.get('extras',{}).get('sourcePSKSha256')!=sha(psk_path):
        raise ValueError('mechanical source mismatch')
    if len(doc['nodes'])!=86 or len(doc['meshes'])!=6 or len(doc.get('animations',[]))!=1:
        raise ValueError('unexpected mechanical source layout')
    for i,name in enumerate(names):
        if doc['nodes'][i]['name']!=name:raise ValueError('bone node mismatch')
    report={'source_psk_sha256':sha(psk_path),'mechanical_glb_sha256':sha(mechanical),
            'visual_rig_manifest_sha256':sha(visual/'clean/rig_manifest.json'),
            'details':{},'max_bind_error_uu':0.0}
    for label,entry in detail['details'].items():
        path=visual/'clean'/f'{label}.obj';v,f=parse_obj(path)
        bone_name=entry['source_bone'];index=names.index(bone_name)
        inv=np.linalg.inv(raw_bind[index]);local=(np.c_[v,np.ones(len(v))]@inv)[:,:3]/UNITS
        restored=(np.c_[local*UNITS,np.ones(len(v))]@raw_bind[index])[:,:3]
        error=float(np.abs(restored-v).max())
        if error>1e-4 or len(v)!=entry['vertices'] or len(f)!=entry['triangles']:
            raise ValueError(f'detail roundtrip/count mismatch: {label}')
        p=add_array(doc,binary,local.astype('<f4'),'POSITION',34962)
        indices=add_array(doc,binary,f.reshape(-1).astype('<u2'),'indices',34963)
        mesh_index=len(doc['meshes'])
        doc['meshes'].append({'name':label,'primitives':[{'attributes':{'POSITION':p},
                                                      'indices':indices,'mode':4}]})
        node_index=len(doc['nodes'])
        doc['nodes'].append({'name':label,'mesh':mesh_index,'extras':
                             {'parentSourceBone':bone_name,'sourceOBJ':path.name}})
        doc['nodes'][index].setdefault('children',[]).append(node_index)
        report['details'][label]={'source_bone':bone_name,'source_sha256':sha(path),
                                  'vertices':len(v),'triangles':len(f),
                                  'roundtrip_error_uu':error}
    doc['buffers'][0]['byteLength']=len(binary)
    doc.setdefault('extras',{})['hdDetails']='Original PSK bone-local rigid detail meshes; no new bones'
    payload=json.dumps(doc,separators=(',',':')).encode();payload+=b' '*((-len(payload))%4)
    binary.extend(b'\0'*((-len(binary))%4))
    glb=(struct.pack('<4sII',b'glTF',2,12+8+len(payload)+8+len(binary))+
         struct.pack('<I4s',len(payload),b'JSON')+payload+
         struct.pack('<I4s',len(binary),b'BIN\0')+binary)
    output.parent.mkdir(parents=True,exist_ok=True);output.write_bytes(glb)
    report['assembled_glb_sha256']=hashlib.sha256(glb).hexdigest()
    (output.parent/'visual_assembly_manifest.json').write_text(json.dumps(report,indent=2)+'\n',encoding='utf-8')
    return report


if __name__=='__main__':
    p=argparse.ArgumentParser();p.add_argument('--visual',type=Path,required=True)
    p.add_argument('--mechanical-glb',type=Path,required=True)
    p.add_argument('--output',type=Path,required=True)
    a=p.parse_args();print(json.dumps(assemble(a.visual,a.mechanical_glb,a.output),indent=2))
