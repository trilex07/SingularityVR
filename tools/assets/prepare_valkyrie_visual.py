"""Prepare private Valkyrie AR9 visual parts from original PSK and verified 4K DDS.

No runtime writes. Source geometry, UVs, skeleton and bind matrices remain intact.
"""
import argparse
import hashlib
import json
import shutil
import sys
from collections import defaultdict
from pathlib import Path

import numpy as np
from PIL import Image, ImageFilter

sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'gloves'))
from prepare_gloves import read_psk

PARTS={'cog_machinegun':'weapon_body','clip':'magazine','Slide':'moving_reload_part',
       'Trigger':'trigger','clip_release':'clip_release','fan':'fan'}
PSK_SHA=''


def sha(path): return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def obj(path,vertices,uvs,triangles,name):
    rows={};v=[];t=[];f=[]
    for tri in triangles:
        face=[]
        for index in tri[:3]:
            p,u,w=uvs[index]
            key=(p,u,w)
            if key not in rows:
                rows[key]=len(v)+1;v.append(vertices[p]);t.append((u,1-w))
            face.append(rows[key])
        f.append(face)
    with path.open('w',encoding='utf-8',newline='\n') as out:
        out.write(f'# Original PSK global bind pose, Unreal units\nmtllib valkyrie_hd.mtl\no {name}\n')
        for x,y,z in v:out.write(f'v {x:.9g} {y:.9g} {z:.9g}\n')
        for u,w in t:out.write(f'vt {u:.9g} {w:.9g}\n')
        out.write('usemtl valkyrie_hd\n')
        for a,b,c in f:out.write(f'f {a}/{a} {b}/{b} {c}/{c}\n')
    a=np.asarray(v)
    return {'vertices':len(v),'triangles':len(f),'bounds_uu':[a.min(0).tolist(),a.max(0).tolist()]}


def maps(root,baseline_diffuse,hd_diffuse,hd_normal):
    out=root/'maps';out.mkdir(exist_ok=True)
    baseline=Image.open(baseline_diffuse).convert('RGB')
    hd=Image.open(hd_diffuse).convert('RGB')
    native=Image.open(hd_normal).convert('RGB')
    if hd.size!=(4096,2048) or native.size!=hd.size or baseline.size!=hd.size:
        raise ValueError('unexpected 4K source dimensions')
    baseline.save(out/'valkyrie_baseline_albedo_4k.png')
    hd.save(out/'valkyrie_hd_albedo_4k.png')
    n=np.asarray(native,dtype=np.uint8)
    converted=n.copy();converted[:,:,1]=255-converted[:,:,1]
    vector=converted.astype(np.float32)/127.5-1
    raw_length=np.linalg.norm(vector,axis=2)
    invalid=raw_length<.5
    vector[invalid]=[0,0,1]
    vector/=np.maximum(np.linalg.norm(vector,axis=2,keepdims=True),1e-6)
    converted=np.uint8(np.clip((vector*.5+.5)*255,0,255))
    Image.fromarray(converted,'RGB').save(out/'valkyrie_hd_normal_gltf_4k.png')
    Image.fromarray(n,'RGB').save(out/'valkyrie_hd_normal_native_4k.png')
    rgb=np.asarray(hd,dtype=np.float32)
    lum=rgb.mean(2);sat=rgb.max(2)-rgb.min(2)
    # Conservative PBR authoring: coated olive/black panels remain dielectric;
    # high-confidence bright neutral metal is metallic. Material masks follow
    # actual texels, not rectangular UV regions.
    neutral=np.clip((lum-58)/64,0,1)*np.clip((77-sat)/48,0,1)
    metallic=np.uint8(np.clip(neutral*210,0,255))
    rough=np.uint8(np.clip(171-neutral*66+sat*.10,84,210))
    Image.fromarray(metallic,'L').save(out/'valkyrie_hd_metallic_4k.png')
    Image.fromarray(rough,'L').save(out/'valkyrie_hd_roughness_4k.png')
    norm=np.asarray(converted,dtype=np.float32)/127.5-1
    lengths=np.linalg.norm(norm,axis=2)
    return {'resolution':[4096,2048],'baseline_diffuse_sha256':sha(baseline_diffuse),
            'hd_diffuse_sha256':sha(hd_diffuse),'hd_normal_sha256':sha(hd_normal),
            'normal_conversion':'R/B unchanged, G=255-G for glTF/Blender +Y',
            'source_short_normal_pixels':int(invalid.sum()),
            'converted_normal_length_min':float(lengths.min()),
            'converted_normal_length_max':float(lengths.max()),
            'mean_abs_albedo_delta':float(np.abs(np.asarray(hd,dtype=np.int16)-np.asarray(baseline,dtype=np.int16)).mean()),
            'metallic_role':'conservative high-confidence exposed metal; coated regions dielectric'}


def main():
    p=argparse.ArgumentParser();p.add_argument('--psk',type=Path,required=True)
    p.add_argument('--baseline-diffuse',type=Path,required=True)
    p.add_argument('--hd-diffuse',type=Path,required=True)
    p.add_argument('--hd-normal',type=Path,required=True)
    p.add_argument('--output',type=Path,required=True)
    a=p.parse_args();root=a.output.resolve()
    for folder in ('source','clean','maps','previews'):(root/folder).mkdir(parents=True,exist_ok=True)
    names,parents,_,bind,points,weights,wedges,faces=read_psk(a.psk)
    if a.psk.stem!='ch_inview_machinegun_mesh' or len(names)!=80 or len(wedges)!=11387 or len(faces)!=12842:
        raise ValueError('wrong Valkyrie original source mesh')
    for source,destination in ((a.psk,a.psk.name),(a.baseline_diffuse,'baseline_AR_E99Rifle_alt_d.dds'),
                               (a.hd_diffuse,'hd_AR_E99Rifle_alt_d.dds'),(a.hd_normal,'hd_AR_E99Rifle_n.dds')):
        target=root/'source'/destination
        if source.resolve()!=target.resolve():shutil.copy2(source,target)
    grouped=defaultdict(list)
    for face in faces:
        bone_ids=[max(weights[wedges[i][0]])[1] for i in face[:3]]
        if len(set(bone_ids))!=1 or names[bone_ids[0]] not in PARTS:
            raise ValueError('mixed or unassigned triangle')
        grouped[names[bone_ids[0]]].append(face)
    objects={}
    for bone,label in PARTS.items():
        objects[label]=obj(root/'clean'/f'{label}.obj',points,wedges,grouped[bone],label)
        objects[label]['source_bone']=bone
    reflect=np.diag([1.,-1.,1.,1.]);raw_bind=reflect@bind@reflect
    bones=[{'index':i,'name':name,'parent':parents[i],
            'bind_matrix_row_major_uu':raw_bind[i].tolist(),
            'pivot_uu':raw_bind[i][3,:3].tolist()} for i,name in enumerate(names)]
    texture_report=maps(root,a.baseline_diffuse,a.hd_diffuse,a.hd_normal)
    manifest={'identity':'Valkyrie AR9 / ch_inview_machinegun_mesh',
              'source_psk_sha256':sha(a.psk),'original_triangles':len(faces),
              'coordinate_contract':'OBJ XYZ are original PSK PNTS0000 global bind-pose Unreal units. No recenter, reflection, rotation or scale. UV V flipped only for OBJ convention.',
              'objects':objects,'bones':bones,'textures':texture_report}
    (root/'clean'/'rig_manifest.json').write_text(json.dumps(manifest,indent=2)+'\n',encoding='utf-8')
    (root/'clean'/'valkyrie_hd.mtl').write_text(
        'newmtl valkyrie_hd\nKd 1 1 1\nmap_Kd ../maps/valkyrie_hd_albedo_4k.png\n'
        'map_Pr ../maps/valkyrie_hd_roughness_4k.png\nmap_Pm ../maps/valkyrie_hd_metallic_4k.png\n'
        'norm ../maps/valkyrie_hd_normal_gltf_4k.png\n',encoding='utf-8')
    print(json.dumps({'output':str(root),'psk_sha256':sha(a.psk),
                      'triangles':{k:v['triangles'] for k,v in objects.items()},'textures':texture_report},indent=2))


if __name__=='__main__':main()
