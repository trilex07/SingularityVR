"""Offline original three-weapon Reload cache and material witness. ZERO Steam.

No proprietary animation/texture/mesh bytes in Git. Explicit package, PSK, PSA,
and original diffuse inputs. Native gun root is deliberately frozen; animation
is consumed relative to the native/controller parent, not the authored hands.
"""
import argparse
import hashlib
import json
from pathlib import Path
import struct
import sys
import numpy as np
from PIL import Image, ImageDraw
from prepare_centurion import read_psk, extract, diffuse_cache, textured_preview
from inspect_centurion_animation import read_psa
sys.path.insert(0, str(Path(__file__).resolve().parents[1]/'title'))
from ue3_layout import inspect

SPECS = {
    'valkyrie': dict(id=1, label='AR9 Valkyrie', mesh='ch_inview_machinegun_mesh', root='cog_machinegun',
        material='AR_E99Rifle_alt', diffuse='AR_E99Rifle_alt_d', vertices=11387, triangles=[12834,8], frames=52, fps=24),
    'volk': dict(id=2, label='Volk S4', mesh='ch_inview_shotgun_mesh', root='cog_shotgun',
        material='AR_Shotgun', diffuse='AR_shotgun_d', vertices=8297, triangles=[10835], frames=108, fps=36),
    'kasimov': dict(id=3, label='Kasimov SNV-E99', mesh='ch_inview_sniper_mesh', root='cog_sniper',
        material='AR_SniperRifle', diffuse='ar_sniper_rifle_d', vertices=11389, triangles=[12942,168], frames=239, fps=60),
    'seeker': dict(id=4, label='Seeker', mesh='ch_inview_e99_steerable_mesh', root='cog_e99_steerable',
        material='e99_steerable', diffuse='ar_steerable_d', vertices=12444, triangles=[14990,6], frames=54, fps=24),
}


def fnv(data):
    h=2166136261
    for b in data:h=((h^b)*16777619)&0xffffffff
    return h


def cooked_layout(data, spec, start, end):
    """Measured v584 section/chunk format; bounded inside named mesh export."""
    if not 0<=start<end<=len(data):raise ValueError('invalid export range')
    pattern=struct.pack('<i',len(spec['triangles']));first=0
    for material, count in enumerate(spec['triangles']):
        pattern+=struct.pack('<hhiH',material,material,first,count);first+=count*3
    at=data.find(pattern,start,end)
    if at<0 or data.find(pattern,at+1,end)!=-1:raise ValueError('not one audited section table')
    p=at+len(pattern)
    def integer():
        nonlocal p
        if p+4>end:raise ValueError('short cooked integer')
        value=struct.unpack_from('<i',data,p)[0];p+=4
        return value
    def array(fmt):
        nonlocal p
        n=integer();size=struct.calcsize(fmt)
        if not 0<=n<=100000 or p+n*size>end:raise ValueError('cooked array bounds')
        result=[struct.unpack_from(fmt,data,p+i*size)[0] for i in range(n)];p+=n*size
        return result
    if integer()!=2:raise ValueError('not INDEX16')
    indices=array('<H')
    if len(indices)!=first:raise ValueError('cooked indices/count mismatch')
    array('<H');array('<H');array('<B')
    n=integer()
    if n!=len(spec['triangles']):raise ValueError('unreviewed chunks')
    chunks=[]
    for i in range(n):
        base=integer()
        if integer()!=0 or integer()!=0:raise ValueError('unexpected CPU vertices')
        palette=array('<H');rigid,soft,influences=integer(),integer(),integer()
        if not palette or len(palette)>16 or len(set(palette))!=len(palette) or rigid<0 or soft<0 or not 1<=influences<=4:
            raise ValueError('invalid bone palette/chunk')
        chunks.append(dict(first=base,vertices=rigid+soft,palette=palette))
    # Runtime witness is the full original IB, including the second lens section.
    if sum(c['vertices'] for c in chunks)!=spec['vertices'] or chunks[0]['first']!=0:
        raise ValueError('chunk vertex total mismatch')
    first=0
    for i,c in enumerate(chunks):
        if i and c['first']!=chunks[i-1]['first']+chunks[i-1]['vertices']:raise ValueError('noncontiguous chunks')
        sub=indices[first:first+spec['triangles'][i]*3];first+=len(sub)
        if min(sub)<c['first'] or max(sub)>=c['first']+c['vertices']:raise ValueError('section/chunk index disagreement')
    return dict(offset=at,chunks=chunks,index_fnv=fnv(struct.pack('<'+'H'*len(indices),*indices)),index_count=len(indices))


def material_witness(data, package, spec):
    def unique(name,kind):
        rows=[e for e in package['exports'] if e['name']==name and e['class_name']==kind]
        if len(rows)!=1 or rows[0].get('parse_error'):raise ValueError('missing/ambiguous material witness '+name)
        return rows[0]
    mesh=unique(spec['mesh'],'SkeletalMesh');mat=unique(spec['material'],'MaterialInstanceConstant')
    texture=unique(spec['diffuse'],'Texture2D')
    p=mesh['property_end']
    # Actual native FSkeletalMesh serialization: FBoxSphereBounds (28 bytes)
    # then TArray<UMaterialInterface*>. No guessed UObject/runtime offsets.
    end=mesh['offset']+mesh['size']
    if not mesh['offset']<=p or not p+32<=end<=len(data):raise ValueError('short mesh bounds/material table')
    count=struct.unpack_from('<i',data,p+28)[0]
    if count!=len(spec['triangles']) or p+32+4*count>end:raise ValueError('unexpected material slots')
    materials=struct.unpack_from('<'+'i'*count,data,p+32)
    if materials[0]!=mat['index']:raise ValueError('magazine not assigned audited material0')
    props=[p for p in mat['properties'] if p['name']=='ReferencedTextures' and p['type']=='ArrayProperty']
    if len(props)!=1:raise ValueError('missing referenced textures')
    p=props[0];start=p['tag_offset']+24;end=p['tag_offset']+p['tag_bytes']
    if not mat['offset']<=start<=end<=mat['offset']+mat['size']<=len(data) or end-start<4:
        raise ValueError('referenced texture tag bounds')
    raw=data[start:end]
    count=struct.unpack_from('<i',raw)[0]
    if not 0<count<=128 or len(raw)!=4+4*count:raise ValueError('referenced texture bounds')
    if texture['index'] not in struct.unpack_from('<'+'i'*count,raw,4):raise ValueError('diffuse not referenced by native material')
    return mesh,dict(mesh=mesh['index'],material=mat['index'],diffuse=texture['index'],slots=list(materials),
                     scope='native mesh material0 references original diffuse; runtime component overrides still UNKNOWN')


def key_local(k):
    x,y,z,w=np.array(k[3:7],dtype=float)*[1,-1,1,-1]
    x,y,z,w=np.array([x,y,z,w])/np.linalg.norm([x,y,z,w])
    m=np.eye(4)
    m[:3,:3]=[[1-2*(y*y+z*z),2*(x*y-w*z),2*(x*z+w*y)],
               [2*(x*y+w*z),1-2*(x*x+z*z),2*(y*z-w*x)],
               [2*(x*z-w*y),2*(y*z+w*x),1-2*(x*x+y*y)]]
    m[3,:3]=np.array(k[:3])*[1,-1,1]
    reflect=np.diag([1.,-1.,1.,1.])
    return reflect@m@reflect


def deltas_at(animation, frame, root):
    raw_bind,bones,parent_tracks,inverse,tracks=animation
    if not np.isfinite(frame) or not 0<=frame<=len(tracks)-1:raise ValueError('invalid frame')
    a=int(frame);b=min(a+1,len(tracks)-1);t=frame-a
    absolute=[];deltas=[]
    for i,_ in enumerate(bones):
        ka,kb=tracks[a,i],tracks[b,i]
        k=np.empty(7,dtype=float);k[:3]=(1-t)*ka[:3].astype(float)+t*kb[:3].astype(float)
        sign=-1 if float(ka[3:7].astype(float)@kb[3:7].astype(float))<0 else 1
        k[3:7]=(1-t)*ka[3:7].astype(float)+t*kb[3:7].astype(float)*sign
        parent=raw_bind[root] if parent_tracks[i]<0 else absolute[parent_tracks[i]]
        absolute.append(key_local(k)@parent);delta=inverse[i]@absolute[-1]
        if not np.isfinite(delta).all() or not np.allclose(delta[:3,:3]@delta[:3,:3].T,np.eye(3),atol=2e-6):
            raise ValueError('nonrigid interpolated original key')
        deltas.append(delta)
    return deltas


def oracle(kind, animation, root):
    """Independent NumPy double precision -> C++ FLOAT32 matrix consumer."""
    frames=(len(animation[-1])-1)*4+1;tracks=len(animation[1])
    blob=struct.pack('<4s4I',b'SVWO',1,SPECS[kind]['id'],tracks,frames)
    for quarter in range(frames):
        frame=quarter*.25
        blob+=struct.pack('<f',frame)+np.array(deltas_at(animation,frame,root),dtype='<f4').tobytes()
    return blob


def prepare(kind,psk,psa,data,package):
    if kind not in SPECS:raise ValueError('unknown profile')
    spec=SPECS[kind];names,parents,_,bind,points,weights,wedges,faces=psk
    if len(wedges)!=spec['vertices'] or len(faces)!=sum(spec['triangles']):raise ValueError('PSK topology mismatch')
    if not parents or parents[0]!=0 or len(set(names))!=len(names) or len(names)!=len(parents) or any(not 0<=p<i for i,p in enumerate(parents) if i):
        raise ValueError('ambiguous/non-topological PSK hierarchy')
    mesh,material=material_witness(data,package,spec)
    layout=cooked_layout(data,spec,mesh['property_end'],mesh['offset']+mesh['size'])
    for material_id,c in enumerate(layout['chunks']):
        pts={wedges[w][0] for f in faces if f[3]==material_id for w in f[:3]}
        contributors={b for p in pts for weight,b in weights[p] if weight>0}
        if contributors!=set(c['palette']):raise ValueError('cooked palette/PSK contributors disagree')
    root=names.index(spec['root']);palette=layout['chunks'][0]['palette']
    if root not in palette or len(palette)<2:raise ValueError('missing root palette slot')
    selected=set(palette)-{root}
    # Include unweighted ancestors if ever needed; refuse an unmapped ancestor
    # rather than treating a PSK index as a GPU register slot.
    for b in selected:
        if parents[b]!=root and parents[b] not in selected and not (
                kind=='seeker' and names[parents[b]]=='bullet_slide' and names[b] in ('bullet_1','bullet_2','bullet_3')
                and parents[parents[b]]==names.index('clip')):
            raise ValueError('unmapped animated ancestor')
    bones=sorted(selected)
    if not bones or len(bones)>8 or 'clip' not in [names[b] for b in bones]:raise ValueError('track budget/clip identity')
    pn,seqs,keys=read_psa(psa)
    seq=[s for s in seqs if s['name']=='Reload']
    if len(seq)!=1:raise ValueError('ambiguous Reload')
    seq=seq[0]
    if seq['frames']!=spec['frames'] or seq['rate']!=spec['fps']:raise ValueError('unreviewed reload timing')
    tracks=keys[seq['first']:seq['first']+seq['frames'],[pn.index(names[b]) for b in bones],:7].copy()
    if kind=='seeker':
        # bullet_slide is an unweighted parent absent from the cooked palette.
        # Compose its original PSA key into each weighted bullet child, then
        # parent those effective tracks to clip. This preserves the exact
        # source motion without inventing a ninth GPU slot.
        from build_volk_mechanical_glb import quaternion
        slide=keys[seq['first']:seq['first']+seq['frames'],pn.index('bullet_slide'),:7]
        for child in ('bullet_1','bullet_2','bullet_3'):
            j=bones.index(names.index(child))
            for frame in range(spec['frames']):
                composed=key_local(tracks[frame,j])@key_local(slide[frame])
                q=quaternion(composed);q[3]*=-1
                tracks[frame,j,:3]=composed[3,:3]
                tracks[frame,j,3:7]=q
    if not np.isfinite(tracks).all() or np.any(np.abs(tracks[:,:,:3]).sum(axis=2)>500):
        raise ValueError('animation local position budget')
    reflect=np.diag([1.,-1.,1.,1.]);raw_bind=reflect@bind@reflect
    # Freeze native/controller root. Compose original descendants hierarchically.
    inverse=[np.linalg.inv(raw_bind[b]) for b in bones]
    parent_tracks=[-1 if parents[b]==root else bones.index(
        parents[parents[b]] if kind=='seeker' and names[parents[b]]=='bullet_slide' else parents[b]) for b in bones]
    slots=[palette.index(b) for b in bones]
    root_slot=palette.index(root)
    header=struct.pack('<4s10If',b'SVWA',1,spec['id'],spec['vertices'],0,spec['triangles'][0],
                       layout['index_fnv'],len(palette),root_slot,len(bones),spec['frames'],float(spec['fps']))
    cache=header+raw_bind[root].astype('<f4').tobytes()
    for slot,parent,inv in zip(slots,parent_tracks,inverse):cache+=struct.pack('<Ii',slot,parent)+inv.astype('<f4').tobytes()
    cache+=tracks.astype('<f4').tobytes()
    report=dict(profile=kind,palette=palette,palette_names=[names[b] for b in palette],root_slot=root_slot,
        tracks=[dict(name=names[b],psk_bone=b,psa_track=pn.index(names[b]),slot=slot,parent=parent)
                for b,slot,parent in zip(bones,slots,parent_tracks)],frames=spec['frames'],fps=spec['fps'],
        material=material,cooked=layout,output_sha256=hashlib.sha256(cache).hexdigest(),
        scope='original descendant animation, root FROZEN, original attached mesh/material retained by future caller; NO ammo/native caller')
    return cache,report,(raw_bind,bones,parent_tracks,inverse,tracks)


def inspect_poses(psk,animation,destination,label):
    """Offline full source geometry, six COMMON-view frames; not game lighting."""
    raw_bind,bones,parent_tracks,inverse,tracks=animation
    names,parents,_,_,points,weights,wedges,faces=psk
    root=parents[bones[0]]
    while root in bones:root=parents[root]
    chosen=sorted(set([0,len(tracks)//5,2*len(tracks)//5,3*len(tracks)//5,4*len(tracks)//5,len(tracks)-1]))
    poses=[];errors=[]
    for frame in chosen:
        absolute=[];deltas={}
        for i,b in enumerate(bones):
            parent=raw_bind[root] if parent_tracks[i]<0 else absolute[parent_tracks[i]]
            absolute.append(key_local(tracks[frame,i])@parent)
            deltas[b]=inverse[i]@absolute[-1]
        output=np.zeros_like(points)
        for p,rows in weights.items():
            for w,b in rows:output[p]+=w*((np.r_[points[p],1.]@deltas[b])[:3] if b in deltas else points[p])
        if not np.isfinite(output).all():raise ValueError('invalid posed geometry')
        errors.append(float(np.linalg.norm(output-points,axis=1).max()));poses.append(output)
    # Source animated Idle isn't assumed identical to bind pose (verified in
    # reports); do not overwrite data to manufacture an identity-at-frame0 pass.
    axes=np.argsort(np.ptp(points,axis=0))[-2:]
    all_xy=np.concatenate([p[:,axes] for p in poses]);extent=np.ptp(all_xy,axis=0)
    centre=(all_xy.min(0)+all_xy.max(0))*.5;scale=min(420/extent[0],290/extent[1])
    image=Image.new('RGB',(1440,760),'#10171b');draw=ImageDraw.Draw(image)
    for panel,(frame,output) in enumerate(zip(chosen,poses)):
        xy=(output[:,axes]-centre)*[scale,-scale]+[240+panel%3*480,200+panel//3*380]
        for face in faces:
            ids=[wedges[i][0] for i in face[:3]]
            moved=any(any(b in bones for _,b in weights[i]) for i in ids)
            coords=[tuple(xy[i]) for i in ids]
            draw.line(coords+[coords[0]],fill='#43a6ae' if moved else '#6a7379',width=1)
        draw.text((20+panel%3*480,14+panel//3*380),f'{label} Reload frame {frame}; root FROZEN',fill='white')
    image.save(destination)
    return dict(inspected_frames=chosen,max_vertex_motion_from_bind_uu=errors,
                note='original local tracks + hierarchy; common camera/scale, not controller/Steam proof')


if __name__=='__main__':
    ap=argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--weapon',choices=SPECS,required=True)
    for key in ('psk','psa','package','diffuse','material','output'):ap.add_argument('--'+key,type=Path,required=True)
    ap.add_argument('--preview',action='store_true');args=ap.parse_args()
    spec=SPECS[args.weapon]
    if args.psk.stem!=spec['mesh'] or args.diffuse.stem!=spec['diffuse'] or args.material.stem!=spec['material']:
        ap.error('not selected profile original export names')
    mat_lines=args.material.read_text(encoding='utf-8').splitlines()
    if [line.strip() for line in mat_lines if line.startswith('Diffuse=')]!=['Diffuse='+spec['diffuse']]:
        ap.error('material exporter diffuse role disagrees with package reference')
    psk=read_psk(args.psk);data=args.package.read_bytes();package=inspect(args.package,data=data,include_all=True)
    cache,report,animation=prepare(args.weapon,psk,args.psa.read_bytes(),data,package)
    golden=oracle(args.weapon,animation,psk[0].index(spec['root']))
    clip,clip_report=extract(psk)
    with Image.open(args.diffuse) as image:texture=diffuse_cache(image)
    args.output.mkdir(parents=True,exist_ok=True)
    for suffix,blob in (('reload.svwa',cache),('oracle.svwo',golden),('clip.svrk',clip),('clip.svrt',texture)):
        (args.output/(args.weapon+'_'+suffix)).write_bytes(blob)
    report['clip']=clip_report;report['texture_sha256']=hashlib.sha256(texture).hexdigest()
    report['source_sha256']={key:hashlib.sha256(getattr(args,key).read_bytes()).hexdigest()
                             for key in ('psk','psa','package','diffuse','material')}
    report['texture']='original albedo/alpha only; no full native material or current runtime override acceptance'
    report['oracle']=dict(quarter_frames=(spec['frames']-1)*4+1,track_poses=((spec['frames']-1)*4+1)*len(animation[1]),
                          sha256=hashlib.sha256(golden).hexdigest(),scope='offline NumPy reference, NOT engine poses')
    if args.preview:
        report['geometry']=inspect_poses(psk,animation,args.output/'animation-geometry.png',spec['label'])
        textured_preview(clip,texture,args.output/'clip-original-texture.png',spec['label']+' original clip')
    (args.output/'animation-report.json').write_text(json.dumps(report,indent=2)+'\n',encoding='utf-8')
    print(json.dumps(report,indent=2))
