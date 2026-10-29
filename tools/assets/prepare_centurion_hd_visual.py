"""Private Centurion original-part extraction and authored 4K PBR maps.

Keeps the original PSK/PSA and UV/bind coordinates unchanged. The 4K atlas adds
new fine material structure from a saved generated height reference; broad
generated shapes are never substituted into the fixed game UV layout.
"""
import argparse
import hashlib
import json
import shutil
import sys
from collections import defaultdict
from pathlib import Path

import numpy as np
from PIL import Image,ImageFilter,ImageDraw

sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'gloves'))
from prepare_gloves import read_psk
from prepare_valkyrie_visual import obj

PARTS={'cog_alt_pistol':'weapon_body','clip':'cylinder','clip_release':'clip_release',
       'hammer':'hammer','Trigger':'trigger'}
PSK_SHA='4e18a25d5241de1b3f18100e3bd9373d464ee16baf3e29d7c60289ae18fe73d4'


def sha(path):return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def detail_tile(path,size):
    im=Image.open(path).convert('L')
    low=np.asarray(im.filter(ImageFilter.GaussianBlur(7)),dtype=np.float32)
    high=np.asarray(im,dtype=np.float32)-low
    high=np.asarray(Image.fromarray(np.uint8(np.clip(high+128,0,255))).resize((size//2,size//2),Image.Resampling.BICUBIC),dtype=np.float32)-128
    for i in range(48):
        t=.5*(1-i/48);a=high[:,i].copy();b=high[:,-i-1].copy()
        high[:,i]=(1-t)*a+t*b;high[:,-i-1]=(1-t)*b+t*a
    for i in range(48):
        t=.5*(1-i/48);a=high[i,:].copy();b=high[-i-1,:].copy()
        high[i,:]=(1-t)*a+t*b;high[-i-1,:]=(1-t)*b+t*a
    return np.tile(high,(2,2))/128


def maps(root,baseline_diffuse,hd_diffuse,hd_normal,tile_path):
    size=4096;out=root/'maps';out.mkdir(exist_ok=True)
    baseline=Image.open(baseline_diffuse).convert('RGB')
    original=Image.open(hd_diffuse).convert('RGB')
    native=Image.open(hd_normal).convert('RGB')
    if baseline.size!=(2048,2048) or original.size!=baseline.size or native.size!=baseline.size:
        raise ValueError('unexpected Centurion atlas size')
    baseline.resize((size,size),Image.Resampling.LANCZOS).save(out/'centurion_baseline_albedo_4k.png')
    source=np.asarray(original.resize((size,size),Image.Resampling.LANCZOS),dtype=np.float32)
    lum=source.mean(2);sat=source.max(2)-source.min(2)
    steel=np.clip((lum-20)/65,0,1)
    # Hand-authored grip region from the inspected source atlas. UV and dark
    # silhouette edges are retained; this is a soft material mask only.
    grip=Image.new('L',(size,size),0);draw=ImageDraw.Draw(grip)
    draw.polygon([(2600,1600),(3630,1480),(3810,1720),(3840,3190),
                  (3380,3360),(2730,2900),(2550,2100)],fill=255)
    grip_mask=np.asarray(grip.filter(ImageFilter.GaussianBlur(22)),dtype=np.float32)/255
    grip_mask*=np.clip((lum-14)/35,0,1)
    surface=np.clip((lum-13)/32,0,1)
    tile=detail_tile(tile_path,size)
    # New brushed-metal microstructure and subdued nonmetal grip grain. The
    # generated height is band-limited; original panel edges are untouched.
    height=tile*surface*(1-.55*grip_mask)
    albedo=np.uint8(np.clip(source+(height*13)[...,None],0,255))
    Image.fromarray(albedo,'RGB').save(out/'centurion_hd_albedo_4k.png')
    source_n=np.asarray(native.resize((size,size),Image.Resampling.BILINEAR),dtype=np.float32)/127.5-1
    source_n[...,1]*=-1 # native UE3/DirectX -> Blender/glTF +Y tangent
    dy,dx=np.gradient(height)
    source_n[...,0]-=dx*.92;source_n[...,1]-=dy*.92
    length=np.linalg.norm(source_n,axis=2);short=length<.5
    source_n[short]=[0,0,1]
    source_n/=np.maximum(np.linalg.norm(source_n,axis=2,keepdims=True),1e-6)
    encoded=np.uint8(np.clip((source_n*.5+.5)*255,0,255))
    Image.fromarray(encoded,'RGB').save(out/'centurion_hd_normal_gltf_4k.png')
    Image.fromarray(np.asarray(native.resize((size,size),Image.Resampling.BILINEAR),dtype=np.uint8),'RGB').save(out/'centurion_source_normal_native_4k.png')
    neutral=np.clip((lum-58)/54,0,1)*np.clip((78-sat)/45,0,1)
    bronze=np.clip((source[...,0]-source[...,2]-15)/60,0,1)*np.clip((lum-45)/55,0,1)
    metal=np.clip(np.maximum(neutral,bronze)*(1-grip_mask),0,1)
    metallic=np.uint8(np.clip(10+metal*220,0,255))
    rough=np.uint8(np.clip(174-metal*67+grip_mask*28+np.abs(tile)*16,78,225))
    Image.fromarray(metallic,'L').save(out/'centurion_hd_metallic_4k.png')
    Image.fromarray(rough,'L').save(out/'centurion_hd_roughness_4k.png')
    Image.fromarray(np.uint8(np.clip(height*54+128,0,255)),'L').save(out/'centurion_hd_height_4k.png')
    n=np.asarray(encoded,dtype=np.float32)/127.5-1
    return {'resolution':[size,size],'source_baseline_diffuse_sha256':sha(baseline_diffuse),
            'source_candidate_diffuse_sha256':sha(hd_diffuse),
            'source_candidate_normal_sha256':sha(hd_normal),'generated_tile_sha256':sha(tile_path),
            'short_source_normal_pixels':int(short.sum()),
            'converted_normal_length_min':float(np.linalg.norm(n,axis=2).min()),
            'converted_normal_length_max':float(np.linalg.norm(n,axis=2).max()),
            'albedo_mean_abs_delta_from_2k_candidate':float(np.abs(albedo.astype(float)-source).mean()),
            'normal_orientation':'native green inverted; +Y tangent for glTF/Blender'}


def main():
    p=argparse.ArgumentParser();p.add_argument('--psk',type=Path,required=True)
    p.add_argument('--psa',type=Path,required=True);p.add_argument('--baseline-diffuse',type=Path,required=True)
    p.add_argument('--hd-diffuse',type=Path,required=True);p.add_argument('--hd-normal',type=Path,required=True)
    p.add_argument('--detail-tile',type=Path,required=True);p.add_argument('--output',type=Path,required=True)
    a=p.parse_args();root=a.output.resolve()
    for folder in ('source','clean','maps','previews'):(root/folder).mkdir(parents=True,exist_ok=True)
    if sha(a.psk)!=PSK_SHA or a.psk.stem!='ch_inview_alt_pistol_mesh':raise ValueError('wrong PSK source')
    names,parents,_,bind,points,weights,wedges,faces=read_psk(a.psk)
    if len(names)!=79 or len(wedges)!=5799 or len(faces)!=6767:raise ValueError('source topology changed')
    sources=((a.psk,a.psk.name),(a.psa,a.psa.name),(a.baseline_diffuse,'baseline_handgun_d.dds'),
             (a.hd_diffuse,'candidate_handgun_d.dds'),(a.hd_normal,'candidate_handgun_n.dds'),
             (a.detail_tile,'ai_metal_detail_tile.png'))
    for src,name in sources:
        dst=root/'source'/name
        if src.resolve()!=dst.resolve():shutil.copy2(src,dst)
    grouped=defaultdict(list)
    for face in faces:
        bone_ids=[max(weights[wedges[i][0]])[1] for i in face[:3]]
        if len(set(bone_ids))!=1 or names[bone_ids[0]] not in PARTS or face[3]!=0:
            raise ValueError('mixed/unassigned source face')
        grouped[names[bone_ids[0]]].append(face)
    objects={}
    for bone,label in PARTS.items():
        objects[label]=obj(root/'clean'/f'{label}.obj',points,wedges,grouped[bone],label)
        objects[label]['source_bone']=bone
    reflect=np.diag([1.,-1.,1.,1.]);raw_bind=reflect@bind@reflect
    bones=[{'index':i,'name':name,'parent':parents[i],
            'bind_matrix_row_major_uu':raw_bind[i].tolist(),
            'pivot_uu':raw_bind[i][3,:3].tolist()} for i,name in enumerate(names)]
    texture_report=maps(root,a.baseline_diffuse,a.hd_diffuse,a.hd_normal,a.detail_tile)
    manifest={'identity':'Centurion / ch_inview_alt_pistol_mesh','source_psk_sha256':sha(a.psk),
              'source_psa_sha256':sha(a.psa),'original_triangles':len(faces),
              'coordinate_contract':'OBJ XYZ are original PSK global bind-pose Unreal units; UV V flipped only for OBJ.',
              'objects':objects,'bones':bones,'textures':texture_report}
    (root/'clean/rig_manifest.json').write_text(json.dumps(manifest,indent=2)+'\n',encoding='utf-8')
    (root/'clean/centurion_hd.mtl').write_text(
        'newmtl centurion_hd\nKd 1 1 1\nmap_Kd ../maps/centurion_hd_albedo_4k.png\n'
        'map_Pr ../maps/centurion_hd_roughness_4k.png\nmap_Pm ../maps/centurion_hd_metallic_4k.png\n'
        'norm ../maps/centurion_hd_normal_gltf_4k.png\n',encoding='utf-8')
    print(json.dumps({'output':str(root),'parts':{k:v['triangles'] for k,v in objects.items()},
                      'textures':texture_report},indent=2))


if __name__=='__main__':main()
