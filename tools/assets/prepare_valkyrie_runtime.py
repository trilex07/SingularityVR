"""Convert the private final handoff to the EXISTING UE3 skin/material path.

No GLB/PBR loader in the game. Original cooked vertices/indices remain byte exact;
three rigid detail groups use the same six-slot body palette. Outputs stay private.
"""
import argparse
import hashlib
import json
import struct
import sys
from pathlib import Path

import numpy as np
from PIL import Image, ImageDraw

ROOT = Path(__file__).resolve().parents[2]
sys.path[:0] = [str(ROOT/'tools/reload'), str(ROOT/'tools/title'), str(ROOT/'tools/textures')]
from prepare_centurion import read_psk
from prepare_weapon_animation import cooked_layout, SPECS, fnv
from ue3_layout import inspect
from dds_io import save_dds, read_dds_header


from prepare_volk_runtime import sha, verify, read_glb


def unused_patch(wedges, faces, size=4096, height=2048, tile=64):
    # Only new detail UVs use this constant-material island. Preserve every
    # original UV and pixel outside it; include a two-texel safety expansion.
    mask = Image.new('1',(size,height))
    d = ImageDraw.Draw(mask)
    uv = np.array([[w[1],w[2]] for w in wedges])*[size,height]
    for f in faces:
        d.polygon([tuple(uv[i]) for i in f[:3]], fill=1)
    occupied = np.array(mask)
    for y in range(tile,height-tile,tile):
        for x in range(tile,size-tile,tile):
            if not occupied[y-2:y+tile+2,x-2:x+tile+2].any():
                return x,y,tile
    raise ValueError('no unoccupied native UV island for constant detail material')


def prepare(package, cooked_path, output):
    verified = verify(package)
    psk = read_psk(package/'source/ch_inview_machinegun_mesh.psk')
    names, parents, _, bind, points, weights, wedges, faces = psk
    data = cooked_path.read_bytes()
    exp = next(e for e in inspect(cooked_path,include_all=True)['exports'] if e['name']=='ch_inview_machinegun_mesh')
    layout = cooked_layout(data,SPECS['valkyrie'],exp['offset'],exp['offset']+exp['size'])
    palette = layout['chunks'][0]['palette']
    if [names[i] for i in palette] != ['cog_machinegun','Trigger','clip_release','clip','fan','Slide']:
        raise ValueError('native palette changed')
    expected = points[[w[0] for w in wedges]]*[1,-1,1]
    needle = struct.pack('<3f',*expected[0])
    candidates=[]
    for at in range(exp['offset'],exp['offset']+exp['size']-len(wedges)*32+1):
        if data[at:at+12] == needle and np.array_equal(np.ndarray((11387,3),'<f4',data,at,(32,4)),expected):
            candidates.append(at)
    if len(candidates)!=1:
        raise ValueError('complete native VB identity not unique')
    at=candidates[0]
    source=data[at:at+11387*32]
    uv=np.ndarray((11387,2),'<f2',source,28,(32,2))
    if not np.allclose(uv,[[w[1],w[2]] for w in wedges],rtol=0,atol=.0005):
        raise ValueError('native UV format not established')
    slots=np.ndarray((11387,4),'u1',source,20,(32,1))
    blends=np.ndarray((11387,4),'u1',source,24,(32,1))
    for i,(point,_,_) in enumerate(wedges):
        expected_weights=sorted((b,w) for w,b in weights[point] if w)
        actual=sorted((palette[int(b)],w/255) for b,w in zip(slots[i],blends[i]) if w)
        if i>=11378: actual=sorted((layout['chunks'][1]['palette'][int(b)],w/255) for b,w in zip(slots[i],blends[i]) if w)
        if len(actual)!=len(expected_weights) or any(a[0]!=b[0] or abs(a[1]-b[1])>1/255+.00001 for a,b in zip(actual,expected_weights)):
            raise ValueError('native weights differ')
    original_indices=np.array([f[:3] for f in faces],dtype='<u2')[:,[1,0,2]].copy().tobytes()
    if fnv(original_indices)!=941526542:
        raise ValueError('original full IB changed')
    original_indices=original_indices[:12834*6]
    if fnv(original_indices)!=615094726:
        raise ValueError('original IB changed')
    x,y,tile=unused_patch(wedges,faces)
    detail_uv=((x+tile/2)/4096,(y+tile/2)/2048)
    doc,accessor=read_glb(package/'valkyrie_ar9_hd_animated.glb')
    mapping={'magazine_hd_ribs':'clip','weapon_body_hd_fasteners':'cog_machinegun','fan_hd_hub':'fan'}
    assembly=json.loads((package/'manifests/visual_assembly_manifest.json').read_text())
    # bind from read_psk is native UE3 row-vector; GLB locals are original
    # ActorX metres. Reflect local Y once, then multiply original native bind.
    vb=bytearray(source);indices=bytearray(original_indices);counts={}
    for mesh in doc['meshes']:
        name=mesh['name']
        if name not in mapping:
            continue
        p=mesh['primitives'][0]
        if len(mesh['primitives'])!=1 or p.get('mode',4)!=4 or p['material']!=1:
            raise ValueError('detail material topology')
        b=names.index(mapping[name]);slot=palette.index(b)
        pos=accessor(p['attributes']['POSITION']).astype(float)*52.5
        normal=accessor(p['attributes']['NORMAL']).astype(float)
        pos*= [1,-1,1];normal*= [1,-1,1]
        pos=np.c_[pos,np.ones(len(pos))]@bind[b]
        normal=normal@bind[b,:3,:3]
        normal/=np.linalg.norm(normal,axis=1)[:,None]
        fs=accessor(p['indices']).reshape(-1,3)[:,[1,0,2]]
        if not np.isfinite(pos).all() or not np.isfinite(normal).all() or fs.max()>=len(pos):
            raise ValueError('detail geometry')
        base=len(vb)//32
        for point,n in zip(pos,normal):
            axis=np.eye(3)[np.argmin(np.abs(n))]
            tangent=np.cross(axis,n);tangent/=np.linalg.norm(tangent)
            pack=lambda xyz,w: bytes(np.r_[np.clip(np.rint((xyz+1)*127.5),0,255),w].astype('u1'))
            vb+=struct.pack('<3f',*point[:3])+pack(tangent,127)+pack(n,255)+bytes([slot,0,0,0,255,0,0,0])+struct.pack('<2e',*detail_uv)
        indices+=(fs.astype('<u2')+base).astype('<u2').tobytes()
        counts[name]=len(fs)
    if set(counts)!=set(mapping) or sum(counts.values())!=1282 or len(vb)//32>=65536:
        raise ValueError('HD detail accounting')
    for name,count in counts.items():
        if count!=assembly['details'][name]['triangles']:
            raise ValueError('source detail triangle count')
    output.mkdir(parents=True,exist_ok=True)
    payload=bytes(vb)+bytes(indices)
    blob=struct.pack('<4s6I',b'SVVK',1,len(vb)//32,len(indices)//2,11387,615094726,fnv(payload))+payload
    (output/'valkyrie_hd.svvk').write_bytes(blob)
    # Do not feed glTF's green-flipped normal into UE3. Verify its inverse
    # conversion byte-exact against the supplied native encoding instead.
    a=np.array(Image.open(package/'maps/valkyrie_hd_normal_gltf_4k.png').convert('RGBA'))
    a[:,:,1]=255-a[:,:,1]
    if not np.array_equal(a,np.array(Image.open(package/'maps/valkyrie_hd_normal_native_4k.png').convert('RGBA'))):
        raise ValueError('native/glTF normal provenance mismatch')
    native_normal=Image.fromarray(a)
    albedo=Image.open(package/'maps/valkyrie_hd_albedo_4k.png').convert('RGBA')
    original_path=package/'source/baseline_AR_E99Rifle_alt_d.dds'
    original=Image.open(original_path).convert('RGBA')
    # Native alpha is material data, not glTF opacity. Preserve it for all
    # original faces instead of replacing it with the opaque preview alpha.
    albedo.putalpha(original.getchannel('A').resize(albedo.size,Image.Resampling.BILINEAR))
    detail=doc['materials'][1]['pbrMetallicRoughness']['baseColorFactor']
    d=ImageDraw.Draw(albedo);d.rectangle((x,y,x+tile-1,y+tile-1),fill=tuple(round(c*255) for c in detail))
    ImageDraw.Draw(native_normal).rectangle((x,y,x+tile-1,y+tile-1),fill=(128,128,255,255))
    save_dds(output/'AR_E99Rifle_alt_d.dds',albedo,'DXT5')
    save_dds(output/'AR_E99Rifle_n.dds',native_normal,'DXT1')
    report=dict(schema='valkyrie-native-runtime-v1',source_glb=sha(package/'valkyrie_ar9_hd_animated.glb'),
        handoff_verified_files=len(verified['files_sha256']),cooked_sha256=sha(cooked_path),
        original_vertices_byte_exact=True,original_indices_byte_exact=True,
        original_vertices=11387,original_triangles=12834,detail_triangles=counts,
        vertices=len(vb)//32,triangles=len(indices)//6,palette=[names[i] for i in palette],
        native_uv_empty_patch=[x,y,tile],native_vb_fnv=fnv(source),
        material_translation='native AR_E99Rifle_alt shader; detail baseColor baked in unused UV island, neutral normal. PBR metallic/roughness are NOT native shader inputs.',
        files_sha256={p.name:sha(p) for p in output.iterdir() if p.suffix in ('.svvk','.dds')},
        physical_acceptance='PENDING')
    (output/'runtime_manifest.json').write_text(json.dumps(report,indent=2)+'\n')
    return report


if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__)
    for n in ('package','cooked','output'):p.add_argument('--'+n,type=Path,required=True)
    a=p.parse_args()
    if not a.output.resolve().is_relative_to((ROOT/'build/valkyrie-hd').resolve()):
        p.error('output must remain in the private build/valkyrie-hd lab')
    print(json.dumps(prepare(a.package,a.cooked,a.output),indent=2))
