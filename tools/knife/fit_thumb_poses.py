"""Separate original-rig thumb poses. Private offline exports only; no Steam.

Keep the accepted knife mount and other four fingers byte-for-byte. Optimize
only three thumb joint deltas, checking thumb/finger AND thumb/handle surfaces.
"""
import argparse
import json
from pathlib import Path
import struct
import numpy as np
from fit_hand_grip import read_psk, read_knife, matrix, intersections, distance, unit
from prepare_native_hands import finger_deltas, quaternion, rotation


class Rig:
    def __init__(self, psk):
        self.psk=psk
        names,parents,local,bind,points,weights,wedges,_=psk
        self.names=names;self.parents=parents;self.local=local
        self.inverse=np.linalg.inv(bind);self.base=finger_deltas(psk)
        self.reflect=np.diag([1.,-1.,1.,1.])
        self.bind=self.reflect@bind@self.reflect
        self.points=np.c_[points*[1,-1,1],np.ones(len(points))]
        self.bones=np.zeros((len(points),4),dtype=int);self.weights=np.zeros((len(points),4))
        for i in range(len(points)):
            total=sum(w for w,b in weights[i])
            for j,(w,b) in enumerate(weights[i]):self.bones[i,j]=b;self.weights[i,j]=w/total
        self.wedges=np.array([w[0] for w in wedges]);self.owner=np.array([max(weights[w[0]])[1] for w in wedges])

    def skin(self,side,grip,thumb=None):
        absolute=[];prefix=('l_','r_')[side]
        for i,l0 in enumerate(self.local):
            l=l0.copy();name=self.names[i]
            digit=next((d for d in ('thumb','index','middle','ring','pinky') if name.startswith(prefix+d+'_') and name[-1:] in '123'),None)
            t=grip[('thumb','index','middle','ring','pinky').index(digit)] if digit else 0
            q=self.base[i]*t+[0,0,0,1-t]
            if thumb is not None and digit=='thumb':q=thumb[int(name[-1])-1]
            q=q/np.linalg.norm(q);l[:3,:3]=l[:3,:3]@matrix(q)
            absolute.append(l if i==0 else l@absolute[self.parents[i]])
        skins=self.inverse@np.array(absolute)
        transformed=np.einsum('vi,vkij->vkj',self.points,skins[self.bones])[:,:,:3]
        result=np.einsum('vk,vkj->vj',self.weights,transformed)*[1,-1,1]
        return result[self.wedges]

    def thumb(self,side,params):
        prefix=('l_','r_')[side];wrist=self.bind[9 if side==0 else 51,3,:3]
        forward=unit(self.bind[self.names.index(prefix+'middle_1'),3,:3]-wrist)
        across=unit(self.bind[self.names.index(prefix+'index_1'),3,:3]-self.bind[self.names.index(prefix+'pinky_1'),3,:3])
        dorsal=unit(np.cross(across,forward))*(1 if side==0 else -1)
        absolute=self.bind.copy();out=[]
        for segment in range(3):
            b=self.names.index(prefix+'thumb_'+str(segment+1));parent=self.parents[b]
            absolute[b]=self.reflect@self.local[b]@self.reflect@absolute[parent]
            if not segment:
                for axis,angle in zip((across,forward,dorsal),params[:3]):
                    absolute[b,:3,:3]=absolute[b,:3,:3]@rotation(axis,np.deg2rad(angle))
            else:
                absolute[b,:3,:3]=absolute[b,:3,:3]@rotation(across,np.deg2rad(params[segment+2]))
            local=self.reflect@(absolute[b]@np.linalg.inv(absolute[parent]))@self.reflect
            out.append(quaternion(np.linalg.inv(self.local[b][:3,:3])@local[:3,:3]))
        return np.array(out)


def run(psk_path,native_path,knife_path,grip_path,output):
    rig=Rig(read_psk(psk_path));raw=Path(native_path).read_bytes();n=struct.unpack_from('<I',raw,8)[0]
    faces=np.frombuffer(raw,'<u2',n,32).reshape(-1,3)
    kv,kf,kuv=read_knife(knife_path);gripraw=Path(grip_path).read_bytes()
    output.mkdir(parents=True,exist_ok=True);profiles=[];report=[];golden=b'SVTG'+struct.pack('<I',12)
    modes_to_fit=('fist-1','fist-2','fist-3','fist-4','fist','knife')
    for profile_index,mode in enumerate(modes_to_fit):
      modes=[]
      for side,prefix in enumerate(('l_','r_')):
        wrist=rig.bind[9 if side==0 else 51,3,:3]
        forward=unit(rig.bind[rig.names.index(prefix+'middle_1'),3,:3]-wrist)
        across=unit(rig.bind[rig.names.index(prefix+'index_1'),3,:3]-rig.bind[rig.names.index(prefix+'pinky_1'),3,:3])
        dorsal=unit(np.cross(across,forward))*(1 if side==0 else -1)
        closure=(profile_index+1)/5 if mode!='knife' else 1.
        grip=np.ones(5)*closure if mode!='knife' else np.frombuffer(gripraw,'<f4',5,96+side*84).copy()
        m=np.frombuffer(gripraw,'<f4',16,32+side*84).reshape(4,4)
        knife=(np.c_[kv,np.ones(len(kv))]@m@rig.reflect)[:,:3]
        hf=faces[np.all(rig.psk[4][rig.wedges[faces],0]*(1 if side==0 else -1)>0,axis=1)]
        thumb_bones=[rig.names.index(prefix+'thumb_'+str(i)) for i in (1,2,3)]
        tf=hf[np.all(np.isin(rig.owner[hf],thumb_bones),axis=1)]
        other=[i for i,name in enumerate(rig.names) if name.startswith(prefix) and any(d+'_' in name for d in ('index','middle','ring','pinky'))]
        ff=hf[np.all(np.isin(rig.owner[hf],other),axis=1)]
        tip=np.flatnonzero(rig.owner==thumb_bones[2]);handle=knife[kf[np.all(kv[kf,2]>-.08,axis=1)]]
        baseline=rig.skin(side,grip);baseline_hits=intersections(baseline[tf],baseline[ff])
        def evaluate(params,details=False):
            qs=rig.thumb(side,params);hand=rig.skin(side,grip,qs)
            hits=intersections(hand[tf],hand[ff]);knife_hits=intersections(hand[hf],knife[kf]) if mode=='knife' else 0
            gap=distance(hand[tip],handle if mode=='knife' else hand[ff])
            # Empty fist: thumb lies palmward/below the curled fingertips, not
            # inserted between them. A real positive surface gap is mandatory.
            thumb_palm=float((hand[tip].mean(0)-wrist)@dorsal)
            finger_palm=float((hand[np.unique(ff)].mean(0)-wrist)@dorsal)
            tuck=max(0,thumb_palm-finger_palm+.45) if mode=='fist' else 0
            ideal=np.array([0.,60.,0.,-5.,6.25])*closure*(1 if side==0 else -1)
            cost=hits*1e4+knife_hits*1e4+(gap-.14)**2*(20 if mode in ('knife','fist') else .01)+tuck*tuck*20
            cost+=np.sum((np.array(params)-ideal)**2)*(.000005 if mode=='knife' else .01)
            if gap<.05:cost+=100
            if details:return dict(intersections=hits,knife_intersections=knife_hits,gap_uu=gap,thumb_palm_uu=thumb_palm,finger_palm_uu=finger_palm),hand,qs
            return cost
        best=None
        mirrored=-np.array(report[-1]['params_degrees']) if side else None
        # Bounded deterministic coordinate descent, independent starts. Never
        # solve/re-fit per frame; output is 12 original-rig quaternions only.
        seeds=[mirrored] if side else ([0,60*closure,0,-5*closure,6.25*closure],[0,0,0,12,18],[30,0,30,35,35],[-30,0,-30,-35,-35],[0,40,60,25,40],[0,-40,-60,-25,-40])
        for seed in seeds:
            p=np.array(seed,dtype=float);cost=evaluate(p)
            for step in (30,15,7.5,3.75,1.875):
              for iteration in range(3):
                changed=False
                for j in range(5):
                  for sign in (-1,1):
                    q=p.copy();q[j]+=sign*step
                    if abs(q[j])>120 or (side and np.max(np.abs(q-mirrored))>7.5):continue
                    c=evaluate(q)
                    if c<cost:cost=c;p=q;changed=True
                if not changed:break
            if best is None or cost<best[0]:best=(cost,p)
        metrics,hand,qs=evaluate(best[1],True)
        metrics.update(mode=mode,side=prefix,baseline_thumb_finger_intersections=baseline_hits,params_degrees=best[1].tolist())
        report.append(metrics);modes.append(qs)
        np.savez(output/f'{mode}_{prefix}assembly.npz',hand=hand,hand_faces=hf,knife=knife,knife_faces=kf,knife_uv=kuv,thumb_faces=tf,finger_faces=ff)
        used=np.unique(hf);golden+=struct.pack('<III',profile_index,side,len(used))
        for wedge in used:
            point=rig.wedges[wedge];original=rig.psk[4][point]*[1,-1,1]
            golden+=struct.pack('<3f4f4I3f',*original,*rig.weights[point],*rig.bones[point],*(hand[wedge]*[1,-1,1]))
        print(json.dumps(metrics),flush=True)
        if metrics['intersections'] or metrics['knife_intersections'] or metrics['gap_uu']<.05:
            raise ValueError('no collision-free thumb fit; do not ship')
      profiles.append(modes)
    data=struct.pack('<4sIII',b'SVNT',2,6,3)+np.array(profiles,dtype='<f4').tobytes()
    (output/'hand_thumb.svnt').write_bytes(data)
    (output/'thumb-golden.bin').write_bytes(golden)
    (output/'thumb-report.json').write_text(json.dumps(report,indent=2)+'\n')


if __name__=='__main__':
    p=argparse.ArgumentParser()
    for name in ('psk','native','knife','grip','output'):p.add_argument('--'+name,type=Path,required=True)
    a=p.parse_args();run(a.psk,a.native,a.knife,a.grip,a.output)
