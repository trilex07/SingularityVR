"""Private Kasimov native skin bridge. Preserve original weighted VB and lens.

GLB is consumed offline only. Native original bytes and six palette slots remain
authoritative; added rigid detail nodes are converted from the complete glTF bind
hierarchy, not guessed bone-local positions. No authored root/camera animation.
"""
import argparse, json, struct, sys
from pathlib import Path
import numpy as np
from PIL import Image, ImageDraw
from prepare_valkyrie_runtime import ROOT, unused_patch
from prepare_volk_runtime import sha, read_glb
from prepare_centurion import read_psk
from prepare_weapon_animation import cooked_layout, SPECS, fnv
from ue3_layout import inspect
from dds_io import save_dds


def verify(package):
    report=json.loads((package/'sha256_manifest.json').read_text())
    for name,expected in report['hashes'].items():
        if sha(package/name)!=expected: raise ValueError('handoff SHA mismatch: '+name)
    return report


def hierarchy(doc):
    """glTF column-vector TRS, including armature/bone parent transforms."""
    world={};parents={c:i for i,n in enumerate(doc['nodes']) for c in n.get('children',[])}
    def visit(i):
        if i in world:return world[i]
        n=doc['nodes'][i]
        if 'matrix' in n:m=np.array(n['matrix'],float).reshape(4,4).T
        else:
            x,y,z,w=n.get('rotation',[0,0,0,1]);s=np.array(n.get('scale',[1,1,1]))
            if abs(x*x+y*y+z*z+w*w-1)>1e-4:raise ValueError('non-unit node rotation')
            m=np.eye(4);m[:3,:3]=np.array([[1-2*y*y-2*z*z,2*x*y-2*z*w,2*x*z+2*y*w],
                [2*x*y+2*z*w,1-2*x*x-2*z*z,2*y*z-2*x*w],
                [2*x*z-2*y*w,2*y*z+2*x*w,1-2*x*x-2*y*y]])@np.diag(s)
            m[:3,3]=n.get('translation',[0,0,0])
        world[i]=(visit(parents[i])@m) if i in parents else m
        return world[i]
    for i in range(len(doc['nodes'])):visit(i)
    return world,parents


def native_points(pos,world):
    # Blender ActorX XYZ -> glTF Y-up (X,Z,-Y), then native's one Y reflection.
    p=np.c_[pos,np.ones(len(pos))]@world.T
    return p[:,[0,2,1]]*52.5


def prepare(package,cooked,baseline,output):
    verified=verify(package)
    names,parents,_,bind,points,weights,wedges,faces=read_psk(package/'ch_inview_sniper_mesh.psk')
    if len(names)!=80 or sum(len(w)==2 for w in weights.values())!=143:raise ValueError('original weighted skeleton')
    data=cooked.read_bytes();exp=next(e for e in inspect(cooked,include_all=True)['exports'] if e['name']=='ch_inview_sniper_mesh')
    layout=cooked_layout(data,SPECS['kasimov'],exp['offset'],exp['offset']+exp['size']);palette=layout['chunks'][0]['palette']
    if [names[i] for i in palette]!=['cog_sniper','wire_2','wire_1','wire_3','Trigger','clip']:raise ValueError('native palette changed')
    expected=points[[w[0] for w in wedges]]*[1,-1,1];needle=struct.pack('<3f',*expected[0]);candidates=[]
    at=data.find(needle,exp['offset'],exp['offset']+exp['size'])
    while at>=0:
        if at+11389*32<=exp['offset']+exp['size'] and np.array_equal(np.ndarray((11389,3),'<f4',data,at,(32,4)),expected):candidates.append(at)
        at=data.find(needle,at+1,exp['offset']+exp['size'])
    if len(candidates)!=1:raise ValueError('complete native VB identity not unique')
    source=data[candidates[0]:candidates[0]+11389*32]
    if not np.allclose(np.ndarray((11389,2),'<f2',source,28,(32,2)),[[w[1],w[2]] for w in wedges],rtol=0,atol=.0005):raise ValueError('UV mismatch')
    slots=np.ndarray((11389,4),'u1',source,20,(32,1));blends=np.ndarray((11389,4),'u1',source,24,(32,1))
    for i,(point,_,_) in enumerate(wedges):
        pal=palette if i<11292 else layout['chunks'][1]['palette']
        actual=sorted((pal[int(b)],w/255) for b,w in zip(slots[i],blends[i]) if w)
        want=sorted((b,w) for w,b in weights[point] if w)
        if len(actual)!=len(want) or any(a[0]!=b[0] or abs(a[1]-b[1])>1/255+.00001 for a,b in zip(actual,want)):raise ValueError('native skin weights differ')
    indices=np.array([f[:3] for f in faces],dtype='<u2')[:,[1,0,2]].copy().tobytes()
    if fnv(indices)!=906901468:raise ValueError('full lens/body index witness')
    indices=bytearray(indices[:12942*6]);source_ib=fnv(indices)
    doc,access=read_glb(package/'kasimov_hd_animated.glb');world,ancestors=hierarchy(doc)
    # Establish the complete coordinate convention against original body points.
    reference=points*[1,-1,1];rounded={tuple(p) for p in np.round(reference,3)};worst=0.
    for i,node in enumerate(doc['nodes']):
        if node.get('name') not in ('kasimov_body','kasimov_magazine','kasimov_lens'):continue
        for prim in doc['meshes'][node['mesh']]['primitives']:
            positions=native_points(access(prim['attributes']['POSITION']),world[i])
            for p in positions:
                if tuple(np.round(p,3)) not in rounded:
                    error=float(np.min(np.max(np.abs(reference-p),axis=1)));worst=max(worst,error)
                    if error>1e-4:raise ValueError(('GLB/native bind coordinate mismatch',node['name'],error))
    x,y,tile=unused_patch(wedges,faces);uv=((x+tile/2)/4096,(y+tile/2)/2048)
    vb=bytearray(source);details={};colors=[]
    for i,node in enumerate(doc['nodes']):
        if not node.get('name','').startswith('HD_'):continue
        owner=i
        while doc['nodes'][owner].get('name') not in names:
            if owner not in ancestors:raise ValueError('detail has no original bone owner')
            owner=ancestors[owner]
        bone=names.index(doc['nodes'][owner]['name']);slot=palette.index(bone)
        mesh=doc['meshes'][node['mesh']]
        if len(mesh['primitives'])!=1:raise ValueError('detail material split')
        p=mesh['primitives'][0];pos=native_points(access(p['attributes']['POSITION']),world[i])
        normal=access(p['attributes']['NORMAL']).astype(float)@np.linalg.inv(world[i][:3,:3]);normal=normal[:,[0,2,1]];normal/=np.linalg.norm(normal,axis=1)[:,None]
        fs=access(p['indices']).reshape(-1,3)[:,[1,0,2]]
        if not np.isfinite(pos).all() or not np.isfinite(normal).all() or fs.max()>=len(pos):raise ValueError('detail geometry')
        base=len(vb)//32
        for point,n in zip(pos,normal):
            tangent=np.cross(np.eye(3)[np.argmin(np.abs(n))],n);tangent/=np.linalg.norm(tangent)
            pack=lambda xyz,w:bytes(np.r_[np.clip(np.rint((xyz+1)*127.5),0,255),w].astype('u1'))
            vb+=struct.pack('<3f',*point)+pack(tangent,127)+pack(n,255)+bytes([slot,0,0,0,255,0,0,0])+struct.pack('<2e',*uv)
        indices+=(fs.astype('<u2')+base).astype('<u2').tobytes()
        details[node['name']]=dict(triangles=len(fs),vertices=len(pos),slot=slot,bone=names[bone])
        colors.append(doc['materials'][p['material']]['pbrMetallicRoughness']['baseColorFactor'])
    if len(details)!=13 or len(vb)//32>=65536 or not np.allclose(colors,colors[0]):raise ValueError('HD accounting/materials')
    output.mkdir(parents=True,exist_ok=True);payload=bytes(vb)+bytes(indices)
    (output/'kasimov_hd.svks').write_bytes(struct.pack('<4s6I',b'SVKS',1,len(vb)//32,len(indices)//2,11389,source_ib,fnv(payload))+payload)
    albedo=Image.open(package/'maps/kasimov_albedo_4k.png').convert('RGBA')
    original=Image.open(baseline/'ar_sniper_rifle_d.dds').convert('RGBA');normal=Image.open(baseline/'ar_sniper_rifle_n.dds').convert('RGBA')
    if albedo.size!=(4096,2048) or normal.size!=albedo.size:raise ValueError('native map dimensions')
    albedo.putalpha(original.getchannel('A').resize(albedo.size,Image.Resampling.BILINEAR))
    ImageDraw.Draw(albedo).rectangle((x,y,x+tile-1,y+tile-1),fill=tuple(round(c*255) for c in colors[0]))
    ImageDraw.Draw(normal).rectangle((x,y,x+tile-1,y+tile-1),fill=(128,128,255,255))
    save_dds(output/'ar_sniper_rifle_d.dds',albedo,'DXT5');save_dds(output/'ar_sniper_rifle_n.dds',normal,'DXT1')
    report=dict(schema='kasimov-native-runtime-v1',handoff_verified_files=len(verified['hashes']),cooked_sha256=sha(cooked),source_glb=sha(package/'kasimov_hd_animated.glb'),
        native_vb_fnv=fnv(source),native_ib_fnv=source_ib,vertices=len(vb)//32,triangles=len(indices)//6,palette=[names[i] for i in palette],details=details,
        source_prefix_byte_exact=True,weighted_source_points=143,lens_faces_untouched=168,glb_native_bind_error_max_uu=worst,native_uv_empty_patch=[x,y,tile],
        native_normal_source_sha256=sha(baseline/'ar_sniper_rifle_n.dds'),material_translation='Original native shader/alpha; detail constant material in proven empty UV island; PBR metallic/roughness not translated',
        files_sha256={p.name:sha(p) for p in output.iterdir() if p.suffix in ('.svks','.dds')},physical='PENDING')
    (output/'runtime_manifest.json').write_text(json.dumps(report,indent=2)+'\n');return report


if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__)
    for name in ('package','cooked','baseline','output'):p.add_argument('--'+name,type=Path,required=True)
    a=p.parse_args()
    if not a.output.resolve().is_relative_to((ROOT/'build/kasimov-hd').resolve()):p.error('private build/kasimov-hd output required')
    print(json.dumps(prepare(a.package,a.cooked,a.baseline,a.output),indent=2))
