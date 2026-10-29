"""Private Centurion native-stream bridge; no new reload or GLB runtime loader."""
import argparse, json, struct, sys
from pathlib import Path
import numpy as np
from PIL import Image, ImageDraw
from prepare_valkyrie_runtime import ROOT, unused_patch
from prepare_volk_runtime import sha, read_glb
from prepare_kasimov_runtime import hierarchy


def native_points(pos, world):
    # Centurion source builder declares original PSK raw axes (not Kasimov Y-up).
    return (np.c_[pos,np.ones(len(pos))]@world.T)[:,:3]*[52.5,-52.5,52.5]

from prepare_centurion import read_psk
from prepare_cylinder_animation import cooked_section, fnv
from ue3_layout import inspect
from dds_io import save_dds


def prepare(package, cooked, output, shared_root=None):
    manifest=json.loads((package/'SHA256_manifest.json').read_text())
    for name,row in manifest['files'].items():
        if sha(package/name)!=row['sha256'] or (package/name).stat().st_size!=row['bytes']:
            raise ValueError('handoff changed: '+name)
    names,parents,_,bind,points,weights,wedges,faces=read_psk(package/'source/ch_inview_alt_pistol_mesh.psk')
    if len(names)!=79 or len(wedges)!=5799 or len(faces)!=6767:raise ValueError('source skeleton/topology')
    data=cooked.read_bytes()
    exp=next(e for e in inspect(cooked,include_all=True)['exports'] if e['name']=='ch_inview_alt_pistol_mesh')
    mesh=data[exp['offset']:exp['offset']+exp['size']]
    palette,ib=cooked_section(mesh,6767)
    if [names[b] for b in palette]!=['clip','cog_alt_pistol','hammer','Trigger','clip_release'] or fnv(ib)!=147026316:
        raise ValueError('original palette/IB')
    expected=points[[w[0] for w in wedges]]*[1,-1,1]
    needle=struct.pack('<3f',*expected[0]);hits=[];at=mesh.find(needle)
    while at>=0:
        if at+5799*32<=len(mesh) and np.array_equal(np.ndarray((5799,3),'<f4',mesh,at,(32,4)),expected):hits.append(at)
        at=mesh.find(needle,at+1)
    if len(hits)!=1:raise ValueError('native VB identity')
    source=mesh[hits[0]:hits[0]+5799*32]
    if not np.allclose(np.ndarray((5799,2),'<f2',source,28,(32,2)),[[w[1],w[2]] for w in wedges],rtol=0,atol=.0005):raise ValueError('original UV')
    slots=np.ndarray((5799,4),'u1',source,20,(32,1));blends=np.ndarray((5799,4),'u1',source,24,(32,1))
    for i,(point,_,_) in enumerate(wedges):
        actual=sorted((palette[int(b)],w/255) for b,w in zip(slots[i],blends[i]) if w)
        want=sorted((b,w) for w,b in weights[point] if w)
        if len(actual)!=len(want) or any(a[0]!=b[0] or abs(a[1]-b[1])>1/255+.00001 for a,b in zip(actual,want)):raise ValueError('source skin weights')
    doc,access=read_glb(package/'centurion_hd_animated.glb',shared_root)
    world,ancestors=hierarchy(doc)
    x,y,tile=unused_patch(wedges,faces,height=4096);uv=((x+tile/2)/4096,(y+tile/2)/4096)
    vb=bytearray(source);indices=bytearray(ib);details={};colors=[];worst=0.
    mapping={'cylinder_hd_hubs':'clip','weapon_body_hd_fasteners':'cog_alt_pistol'}
    # Prove coordinate convention against original body/cylinder, not guessed offsets.
    reference=points*[1,-1,1]
    for i,node in enumerate(doc['nodes']):
        if 'mesh' not in node:continue
        p=doc['meshes'][node['mesh']]['primitives'][0]
        pos=native_points(access(p['attributes']['POSITION']),world[i])
        if node['name'] not in mapping:
            for v in pos:
                error=float(np.min(np.max(np.abs(reference-v),axis=1)));worst=max(worst,error)
                if error>2e-4:raise ValueError(('GLB/native convention',node['name'],error))
            continue
        owner=names.index(mapping[node['name']]);slot=palette.index(owner)
        # Derive native details from the full glTF bind hierarchy, retain native animation slots.
        normal=access(p['attributes']['NORMAL']).astype(float)@np.linalg.inv(world[i][:3,:3])
        normal=normal*[1,-1,1];normal/=np.linalg.norm(normal,axis=1)[:,None]
        fs=access(p['indices']).reshape(-1,3)[:,[1,0,2]];base=len(vb)//32
        for v,n in zip(pos,normal):
            tangent=np.cross(np.eye(3)[np.argmin(np.abs(n))],n);tangent/=np.linalg.norm(tangent)
            pack=lambda xyz,w:bytes(np.r_[np.clip(np.rint((xyz+1)*127.5),0,255),w].astype('u1'))
            vb+=struct.pack('<3f',*v)+pack(tangent,127)+pack(n,255)+bytes([slot,0,0,0,255,0,0,0])+struct.pack('<2e',*uv)
        indices+=(fs.astype('<u2')+base).astype('<u2').tobytes()
        details[node['name']]=dict(vertices=len(pos),triangles=len(fs),slot=slot,bone=names[owner])
        colors.append(doc['materials'][p['material']]['pbrMetallicRoughness']['baseColorFactor'])
    if set(details)!=set(mapping) or sum(d['triangles'] for d in details.values())!=1160 or not np.allclose(colors,colors[0]):raise ValueError('detail accounting')
    output.mkdir(parents=True,exist_ok=True);payload=bytes(vb)+bytes(indices)
    (output/'centurion_hd.svct').write_bytes(struct.pack('<4s6I',b'SVCT',1,len(vb)//32,len(indices)//2,5799,fnv(ib),fnv(payload))+payload)
    # Full remastered normal uses glTF green inversion; source-only native normal lacks HD detail.
    normal=np.array(Image.open(package/'textures/centurion_hd_normal_gltf_4k.png').convert('RGBA'));normal[:,:,1]=255-normal[:,:,1]
    normal=Image.fromarray(normal)
    albedo=Image.open(package/'textures/centurion_hd_albedo_4k.png').convert('RGBA')
    original=Image.open(package/'source/baseline_handgun_d.dds').convert('RGBA')
    albedo.putalpha(original.getchannel('A').resize(albedo.size,Image.Resampling.BILINEAR))
    ImageDraw.Draw(albedo).rectangle((x,y,x+tile-1,y+tile-1),fill=tuple(round(c*255) for c in colors[0]))
    ImageDraw.Draw(normal).rectangle((x,y,x+tile-1,y+tile-1),fill=(128,128,255,255))
    save_dds(output/'handgun_d.dds',albedo,'DXT5');save_dds(output/'handgun_n.dds',normal,'DXT1')
    report=dict(schema='centurion-native-runtime-v1',handoff_verified_files=len(manifest['files']),source_glb=sha(package/'centurion_hd_animated.glb'),cooked_sha256=sha(cooked),
        vertices=len(vb)//32,triangles=len(indices)//6,native_vb_fnv=fnv(source),native_ib_fnv=fnv(ib),palette=[names[i] for i in palette],details=details,
        source_prefix_byte_exact=True,glb_native_bind_error_max_uu=worst,native_uv_empty_patch=[x,y,tile],
        material_translation='Native AR_Pistol shader/alpha; glTF normal green inverted exactly once. No PBR shader. Original physical cylinder/root/hammer/trigger palette retained.',
        files_sha256={p.name:sha(p) for p in output.iterdir() if p.suffix in ('.svct','.dds')},physical_acceptance='PENDING')
    (output/'runtime_manifest.json').write_text(json.dumps(report,indent=2)+'\n');return report


if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__)
    for n in ('package','cooked','output'):p.add_argument('--'+n,type=Path,required=True)
    p.add_argument('--shared-root',type=Path)
    a=p.parse_args()
    if not a.output.resolve().is_relative_to((ROOT/'build/centurion-hd').resolve()):p.error('output outside private lab')
    print(json.dumps(prepare(a.package,a.cooked,a.output,a.shared_root),indent=2))
