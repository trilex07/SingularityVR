"""Adaptive offline collision-constrained original thumb path; no runtime IK.

Five isolated endpoints are not enough: moving curled fingers sweep through
the thumb BETWEEN keys. Refine the measured failing intervals, not the mesh.
"""
import argparse
import hashlib
import json
from pathlib import Path
import struct
import numpy as np
from fit_thumb_poses import Rig, read_psk, intersections


def blend(a,b,t):
    b=b.copy();b[np.sum(a*b,axis=1)<0]*=-1;q=a*(1-t)+b*t
    return q/np.linalg.norm(q,axis=1)[:,None]


def refine(prepared,psk_path):
    rig=Rig(read_psk(psk_path));raw=(prepared/'hand_thumb.svnt').read_bytes()
    if struct.unpack_from('<4sIII',raw)!=(b'SVNT',2,6,3):raise ValueError('requires v2 fit, not already-refined output')
    q=np.frombuffer(raw,'<f4',144,16).reshape(6,2,3,4).astype(float)
    times=[0.,.2,.4,.6,.8,1.];keys=[np.tile([0.,0.,0.,1.],(2,3,1))]+list(q[:5])
    geometry=[np.load(prepared/f'fist_{prefix}assembly.npz') for prefix in ('l_','r_')]
    report=[]
    for iteration in range(58):
        worst=None
        for g in np.linspace(0,1,201):
            j=min(len(times)-2,max(0,int(np.searchsorted(times,g,side='right')-1)))
            t=(g-times[j])/(times[j+1]-times[j]);qs=np.array([blend(keys[j][s],keys[j+1][s],t) for s in (0,1)])
            for side in (0,1):
                hand=rig.skin(side,np.ones(5)*g,qs[side]);d=geometry[side]
                hits=intersections(hand[d['thumb_faces']],hand[d['finger_faces']])
                if hits and (worst is None or hits>worst[0]):worst=(hits,float(g),j,qs)
        if worst is None:break
        count,g,j,target=worst
        fitted=[];params=[]
        for side in (0,1):
            d=geometry[side]
            def cost(p):
                qs=rig.thumb(side,p);hand=rig.skin(side,np.ones(5)*g,qs)
                hits=intersections(hand[d['thumb_faces']],hand[d['finger_faces']])
                # Shortest quaternion distance to the CURRENT interpolated path
                # limits unrelated motion. Only thumb rotations can change.
                dots=np.abs(np.sum(qs*target[side],axis=1));return hits*1e5+np.sum((1-dots))*100
            seeds=[-params[0]] if side else [np.array([0,60*g,0,-5*g,6.25*g]),np.zeros(5),np.array([30,0,30,35,35]),np.array([-30,0,-30,-35,-35])]
            best=None
            for seed in seeds:
                p=seed.copy();c=cost(p)
                for step in (20,10,5,2.5,1.25):
                  for repeat in range(3):
                    changed=False
                    for k in range(5):
                      for sign in (-1,1):
                        pp=p.copy();pp[k]+=sign*step
                        if np.max(np.abs(pp))>120:continue
                        cc=cost(pp)
                        if cc<c:c=cc;p=pp;changed=True
                    if not changed:break
                if best is None or c<best[0]:best=(c,p)
            if best[0]>=1e5:raise ValueError('no collision-free original thumb path; do not ship')
            params.append(best[1]);fitted.append(rig.thumb(side,best[1]))
        times.insert(j+1,g);keys.insert(j+1,np.array(fitted))
        report.append(dict(closure=g,previous_crossings=count,keys=len(times)))
        print(json.dumps(report[-1]),flush=True)
    else:raise ValueError('bounded refinement budget exhausted; do not ship')
    # Existing full fist/knife endpoints and original rig remain unchanged.
    profiles=np.array(keys+[q[5]],dtype='<f4')
    data=struct.pack('<4sIII',b'SVNT',3,len(times),3)+np.array(times,dtype='<f4').tobytes()+profiles.tobytes()
    # Recheck the emitted FLOAT32 data, not just optimizer doubles.
    baked_times=np.frombuffer(data,'<f4',len(times),16).astype(float)
    baked=np.frombuffer(data,'<f4',(len(times)+1)*24,16+len(times)*4).reshape(len(times)+1,2,3,4).astype(float)
    for g0 in np.linspace(0,1,201):
        g=float(np.float32(g0));j=min(len(times)-2,max(0,int(np.searchsorted(baked_times,g,side='right')-1)))
        t=(g-baked_times[j])/(baked_times[j+1]-baked_times[j])
        for side,d in enumerate(geometry):
            hand=rig.skin(side,np.ones(5)*g,blend(baked[j,side],baked[j+1,side],t))
            if intersections(hand[d['thumb_faces']],hand[d['finger_faces']]):raise ValueError('quantized path collision; do not ship')
    (prepared/'hand_thumb_refined.svnt').write_bytes(data)
    (prepared/'thumb-path-report.json').write_text(json.dumps(dict(free_keys=len(times),samples_per_hand=201,thumb_finger_crossings=0,refinements=report,
        cache_sha256=hashlib.sha256(data).hexdigest(),psk_sha256=hashlib.sha256(Path(psk_path).read_bytes()).hexdigest(),
        source_fit_sha256=hashlib.sha256(raw).hexdigest(),limitation='sampled original mesh geometry, not continuous collision or headset proof'),indent=2)+'\n')


if __name__=='__main__':
    p=argparse.ArgumentParser();p.add_argument('--prepared',type=Path,required=True);p.add_argument('--psk',type=Path,required=True)
    a=p.parse_args();refine(a.prepared,a.psk)
