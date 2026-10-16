"""Physical-feedback follow-up: blade only, held thumb only. No Steam.

Preserve accepted fist cache bytes, hilt geometry, other four fingers, mount,
UVs, texture bytes and scene-light shader. Source is user's original mesh.
"""
import argparse
import hashlib
import json
from pathlib import Path
import struct
import numpy as np
from fit_thumb_poses import Rig, read_psk, intersections, distance
from fit_hand_grip import read_knife, inside


def resize(raw,length=1.15,width=1.08):
    if not 1<=length<=1.3 or not 1<=width<=1.2:raise ValueError('bounded blade adjustment only')
    magic,version,n,ni=struct.unpack_from('<4sIII',raw)
    if (magic,version,n,ni)!=(b'SVRK',2,662,3030) or len(raw)!=16+n*24+ni*2:raise ValueError('original textured knife required')
    out=bytearray(raw)
    for i in range(n):
        x,y,z=struct.unpack_from('<3f',raw,16+i*24)
        if z<-.08:
            factor=1+(width-1)*min(1,(-.08-z)/.02)
            struct.pack_into('<3f',out,16+i*24,x*factor,y*factor,-.08+(z+.08)*length)
    return bytes(out)


def retune(psk_path,native_path,mesh_path,grip_path,thumb_path,golden_path,output):
    output.mkdir(parents=True,exist_ok=True)
    original=Path(mesh_path).read_bytes();mesh=resize(original);(output/'knife.svrk').write_bytes(mesh)
    knife,kf,uv=read_knife(output/'knife.svrk');rig=Rig(read_psk(psk_path))
    native=Path(native_path).read_bytes();n=struct.unpack_from('<I',native,8)[0]
    faces=np.frombuffer(native,'<u2',n,32).reshape(-1,3);grip=Path(grip_path).read_bytes()
    thumb=Path(thumb_path).read_bytes();magic,version,count,joints=struct.unpack_from('<4sIII',thumb)
    if (magic,version,joints)!=(b'SVNT',3,3) or len(thumb)!=16+count*4+(count+1)*96:raise ValueError('accepted refined thumb cache required')
    profiles=np.frombuffer(thumb,'<f4',(count+1)*24,16+count*4).reshape(count+1,2,3,4).copy()
    report=[];golden=b'SVTK'+struct.pack('<I',2);new_groups=[]
    for side,prefix in enumerate(('l_','r_')):
        m=np.frombuffer(grip,'<f4',16,32+side*84).reshape(4,4).astype(float)@rig.reflect
        fingers=np.frombuffer(grip,'<f4',5,96+side*84).astype(float)
        transformed=(np.c_[knife,np.ones(len(knife))]@m)[:,:3]
        hf=faces[np.all(rig.psk[4][rig.wedges[faces],0]*(1 if side==0 else -1)>0,axis=1)]
        thumb_bones=[rig.names.index(prefix+'thumb_'+str(i)) for i in (1,2,3)]
        tf=hf[np.all(np.isin(rig.owner[hf],thumb_bones),axis=1)]
        other=[i for i,name in enumerate(rig.names) if name.startswith(prefix) and any(d+'_' in name for d in ('index','middle','ring','pinky'))]
        ff=hf[np.all(np.isin(rig.owner[hf],other),axis=1)]
        baseline=rig.skin(side,fingers,profiles[-1,side]);tip=np.flatnonzero(rig.owner==thumb_bones[2])
        # A PAD PATCH, not a single convenient corner nearest the guard. Use the
        # inner quarter of distal thumb surface, identified in the accepted pose.
        local=(np.c_[baseline,np.ones(len(baseline))]@np.linalg.inv(m))[:,:3]
        pad=tip[local[tip,1]<=np.quantile(local[tip,1],.25)]
        handle=transformed[kf[np.all((knife[kf,2]>-.08)&(knife[kf,2]<-.04),axis=1)]]
        if not len(handle) or len(pad)<3:raise ValueError('thumb contact producer changed')
        def metrics(qs):
            hand=rig.skin(side,fingers,qs);hits=intersections(hand[hf],transformed[kf]);self_hits=intersections(hand[tf],hand[ff])
            gaps=np.array([distance(hand[[i]],handle) for i in pad])
            return hand,hits,self_hits,gaps
        old=metrics(profiles[-1,side]);best=None
        seeds=([90,-10,-30,-85,-45.625],[60,0,0,-60,-60],[30,0,30,35,35],[-30,0,-30,-35,-35]) if side==0 else [-best_left]
        for seed in seeds:
            p=np.array(seed,dtype=float)
            def score(p):
                qs=rig.thumb(side,p);hand,hits,self_hits,gaps=metrics(qs)
                orientation=np.sum(1-np.abs(np.sum(qs*profiles[-1,side],axis=1)))
                return hits*1e5+self_hits*1e5+np.mean((gaps-.035)**2)*100+orientation*.03
            cost=score(p)
            for step in (15,7.5,3.75,1.875,.9375):
              for repeat in range(3):
                changed=False
                for j in range(5):
                  for sign in (-1,1):
                    candidate=p.copy();candidate[j]+=sign*step
                    if np.max(np.abs(candidate))>125:continue
                    c=score(candidate)
                    if c<cost:cost=c;p=candidate;changed=True
                if not changed:break
            if best is None or cost<best[0]:best=(cost,p)
        if not side:best_left=best[1]
        profiles[-1,side]=rig.thumb(side,best[1]).astype('<f4')
        hand,hits,self_hits,gaps=metrics(profiles[-1,side])
        buried=int(inside(transformed,hand[hf]).sum())
        if hits or self_hits or buried or gaps.mean()>=old[3].mean():raise ValueError('contact did not improve safely; do not ship')
        report.append(dict(side=prefix,hand_knife_crossings=hits,thumb_finger_crossings=self_hits,buried_knife_vertices=buried,
            pad_points=len(pad),old_pad_gap_mean_uu=float(old[3].mean()),pad_gap_mean_uu=float(gaps.mean()),
            pad_gap_min_uu=float(gaps.min()),params=best[1].tolist()))
        np.savez(output/f'knife_{prefix}assembly.npz',hand=hand,hand_faces=hf,knife=transformed,knife_faces=kf,knife_uv=uv)
        used=np.unique(hf);golden+=struct.pack('<II',side,len(used));records=bytearray()
        for wedge in used:
            point=rig.wedges[wedge];original_point=rig.psk[4][point]*[1,-1,1]
            record=struct.pack('<3f4f4I3f',*original_point,*rig.weights[point],*rig.bones[point],*(hand[wedge]*[1,-1,1]))
            golden+=record;records.extend(record)
        new_groups.append(struct.pack('<III',5,side,len(used))+records)
        print(json.dumps(report[-1]),flush=True)
    out=thumb[:16+count*4]+profiles.astype('<f4').tobytes()
    assert out[:16+count*4+count*96]==thumb[:16+count*4+count*96], 'accepted fist path changed'
    (output/'hand_thumb.svnt').write_bytes(out);(output/'knife-thumb-golden.bin').write_bytes(golden)
    source_golden=Path(golden_path).read_bytes()
    if source_golden[:8]!=b'SVTG'+struct.pack('<I',12):raise ValueError('accepted fist golden required')
    at=8
    for group in range(10):
        mode,side,vertices=struct.unpack_from('<III',source_golden,at)
        if mode!=group//2 or side!=group%2:raise ValueError('golden hand/mode order changed')
        at+=12+vertices*56
    (output/'thumb-golden.bin').write_bytes(source_golden[:at]+b''.join(new_groups))
    v0,_,_=read_knife(mesh_path)
    result=dict(hands=report,accepted_fist_bytes_unchanged=True,hilt_vertices_unchanged=True,
        old_length_m=float(np.ptp(v0[:,2])),length_m=float(np.ptp(knife[:,2])),blade_length_factor=1.15,blade_width_factor=1.08,
        thumb_sha256=hashlib.sha256(out).hexdigest(),knife_sha256=hashlib.sha256(mesh).hexdigest(),
        original_mesh_sha256=hashlib.sha256(original).hexdigest(),original_thumb_sha256=hashlib.sha256(thumb).hexdigest())
    (output/'retune-report.json').write_text(json.dumps(result,indent=2)+'\n')


if __name__=='__main__':
    p=argparse.ArgumentParser()
    for name in ('psk','native','mesh','grip','thumb','golden','output'):p.add_argument('--'+name,type=Path,required=True)
    a=p.parse_args();retune(a.psk,a.native,a.mesh,a.grip,a.thumb,a.golden,a.output)
