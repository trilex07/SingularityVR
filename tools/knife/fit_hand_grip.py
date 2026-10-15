"""Offline original glove/knife assembly fit. Private inputs/output, zero Steam.

No source mesh resizing/cropping: rigid knife mounting + original rig fingers.
Checks exact triangle intersections, not just bounding boxes or silhouettes.
"""
import argparse
import hashlib
import json
from pathlib import Path
import struct
import sys
import numpy as np
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'gloves'))
from prepare_gloves import read_psk, unit
from prepare_native_hands import finger_deltas


def matrix(q):
    x,y,z,w=q
    return np.array([[1-2*(y*y+z*z),2*(x*y+z*w),2*(x*z-y*w)],
                     [2*(x*y-z*w),1-2*(x*x+z*z),2*(y*z+x*w)],
                     [2*(x*z+y*w),2*(y*z-x*w),1-2*(x*x+y*y)]])


def posed(psk,side,grip):
    names,parents,local,bind,points,weights,wedges,_=psk
    qs=finger_deltas(psk);prefix='l_' if side==0 else 'r_';absolute=[]
    for i,l in enumerate(local):
        l=l.copy()
        digit=next((d for d in ('thumb','index','middle','ring','pinky')
                    if names[i].startswith(prefix+d+'_') and names[i][-1:] in '123'),None)
        t=grip[('thumb','index','middle','ring','pinky').index(digit)] if digit else 0.
        q=qs[i]*t+[0,0,0,1-t];q/=np.linalg.norm(q);l[:3,:3]=l[:3,:3]@matrix(q)
        absolute.append(l if i==0 else l@absolute[parents[i]])
    skin=np.linalg.inv(bind)@np.array(absolute)
    reflect=np.diag([1.,-1.,1.,1.]);skin=reflect@skin@reflect
    result=np.array([sum(w*(np.r_[p,1.]@skin[b])[:3] for w,b in weights[i]) /
                     sum(w for w,_ in weights[i]) for i,p in enumerate(points)])
    return result[[x[0] for x in wedges]]


def segment_hits(a,b,tri,eps=1e-7):
    """Vectorized matching segment/triangle pairs; excludes mere contact."""
    direction=b-a;e1=tri[:,1]-tri[:,0];e2=tri[:,2]-tri[:,0]
    h=np.cross(direction,e2);det=np.sum(e1*h,axis=1)
    inv=np.zeros_like(det);valid=np.abs(det)>eps;inv[valid]=1/det[valid]
    s=a-tri[:,0];u=inv*np.sum(s*h,axis=1);q=np.cross(s,e1)
    v=inv*np.sum(direction*q,axis=1);t=inv*np.sum(e2*q,axis=1)
    return valid&(u>eps)&(v>eps)&(u+v<1-eps)&(t>eps)&(t<1-eps)


def intersections(a,b):
    # Broad phase then test all six edges for each surviving triangle pair.
    alo,ahi=a.min(1),a.max(1);blo,bhi=b.min(1),b.max(1)
    overlap=np.all(ahi[:,None,:]>blo[None,:,:]+1e-7,axis=2)&np.all(bhi[None,:,:]>alo[:,None,:]+1e-7,axis=2)
    ai,bi=np.nonzero(overlap)
    if not len(ai):return 0
    aa,bb=a[ai],b[bi];hits=np.zeros(len(ai),dtype=bool)
    for i,j in ((0,1),(1,2),(2,0)):
        hits|=segment_hits(aa[:,i],aa[:,j],bb)
        hits|=segment_hits(bb[:,i],bb[:,j],aa)
    return int(hits.sum())


def distance(points,triangles):
    p=points[:,None,:];a=triangles[None,:,0];b=triangles[None,:,1];c=triangles[None,:,2]
    ab=b-a;ac=c-a;ap=p-a
    normal=np.cross(ab,ac);n2=np.sum(normal*normal,axis=2)
    d00=np.sum(ab*ab,axis=2);d01=np.sum(ab*ac,axis=2);d11=np.sum(ac*ac,axis=2)
    d20=np.sum(ap*ab,axis=2);d21=np.sum(ap*ac,axis=2)
    denominator=d00*d11-d01*d01
    valid=denominator>1e-12;denominator=np.maximum(denominator,1e-12)
    u=(d11*d20-d01*d21)/denominator;v=(d00*d21-d01*d20)/denominator
    plane=np.sum(ap*normal,axis=2)**2/np.maximum(n2,1e-12)
    nearest=np.where(valid&(u>=0)&(v>=0)&(u+v<=1),plane,np.inf)
    for x,y in ((a,b),(b,c),(c,a)):
        edge=y-x;t=np.sum((p-x)*edge,axis=2)/np.maximum(np.sum(edge*edge,axis=2),1e-12)
        delta=p-(x+np.clip(t,0,1)[:,:,None]*edge)
        nearest=np.minimum(nearest,np.sum(delta*delta,axis=2))
    return float(np.sqrt(nearest.min()))


def inside(points,triangles):
    # Three deterministic non-axis-aligned parity rays, majority vote. Chunked
    # to bound memory. Detects a buried handle even without crossing triangles.
    result=[];a=triangles[:,0];e1=triangles[:,1]-a;e2=triangles[:,2]-a
    for start in range(0,len(points),128):
        p=points[start:start+128];votes=np.zeros(len(p),dtype=int)
        for direction in (unit(np.array([1.,.371,.183])),unit(np.array([.219,1.,.413])),unit(np.array([.317,.173,1.]))):
            h=np.cross(direction,e2);det=(e1*h).sum(1);valid=np.abs(det)>1e-10
            inv=np.zeros_like(det);inv[valid]=1/det[valid]
            s=p[:,None,:]-a[None,:,:];u=inv*(s*h).sum(2);q=np.cross(s,e1)
            v=inv*(q*direction).sum(2);t=inv*(q*e2).sum(2)
            hits=valid[None,:]&(u>1e-8)&(v>1e-8)&(u+v<1-1e-8)&(t>1e-8)
            votes+=(hits.sum(1)%2).astype(int)
        result.extend(votes>=2)
    return np.array(result,dtype=bool)


def read_knife(path):
    data=Path(path).read_bytes();magic,version,n,ni=struct.unpack_from('<4sIII',data)
    if (magic,version)!=(b'SVRK',2):raise ValueError('requires textured original knife v2')
    rows=np.array([struct.unpack_from('<3fI2f',data,16+i*24) for i in range(n)])
    faces=np.frombuffer(data,'<u2',ni,16+n*24).reshape(-1,3)
    return rows[:,:3],faces,rows[:,4:6]


def fit(psk_path,native_path,knife_path,output):
    psk=read_psk(psk_path);names,_,_,bind,_,_,wedges,_=psk
    native=Path(native_path).read_bytes();header=struct.unpack_from('<4s7I',native)
    if header[:2]!=(b'SVNI',1):raise ValueError('measured native hand cache required')
    hand_faces=np.frombuffer(native,'<u2',header[2],32).reshape(-1,3)
    knife,kf,kuv=read_knife(knife_path)
    reflect=np.diag([1.,-1.,1.,1.]);raw_bind=reflect@bind@reflect
    mounts=[];poses=[];reports=[]
    # Compensate existing 75% native rig size so knife remains .30 tracking metres
    # at defaults. A changed user rig scale moves/scales BOTH models together.
    units=52.5/.75
    for side,prefix in enumerate(('l_','r_')):
        wrist=raw_bind[9 if side==0 else 51,3,:3]
        forward=unit(raw_bind[names.index(prefix+'middle_1'),3,:3]-wrist)
        across=unit(raw_bind[names.index(prefix+'index_1'),3,:3]-raw_bind[names.index(prefix+'pinky_1'),3,:3])
        forward=unit(forward-across*(forward@across))
        dorsal=unit(np.cross(across,forward))*(1 if side==0 else -1)
        basis=np.array([forward,dorsal,-across])*units
        hf=hand_faces[np.all(psk[4][[wedges[int(x)][0] for x in hand_faces.ravel()]].reshape(-1,3,3)[:,:,0]*(1 if side==0 else -1)>0,axis=1)]
        best=None
        # Coarse search is deterministic artist fitting, NOT claimed game animation.
        for curl in (.45,.55,.65,.75):
          for thumb in (.55,.70,.85):
            grip=[thumb,curl,curl,curl,curl]
            hand=posed(psk,side,grip);htri=hand[hf]
            for n in (-3.3,-3.7,-4.1,-4.5):
                for f in (5.5,6.,6.5,7.):
                    for a in (-.6,0.,.6):
                        translation=wrist+across*a+dorsal*n+forward*f
                        transformed=knife@basis+translation
                        count=intersections(htri,transformed[kf])
                        # Prefer a closed grip and central handle; actual triangle hits
                        # dominate all secondary artist preferences.
                        contacts=[]
                        if not count:
                            handle=transformed[kf[np.all(knife[kf,2]>-.08,axis=1)]]
                            for digit in ('thumb','index','middle','ring','pinky'):
                                indices=[i for i,w in enumerate(wedges) if names[max(psk[5][w[0]])[1]].startswith(prefix+digit+'_')]
                                contacts.append(distance(hand[indices],handle))
                        cost=count*10000+sum(x*x for x in contacts)*10+(curl-.65)**2*10+(n+3.7)**2+(f-6.5)**2+a*a
                        if best is None or cost<best[0]:best=(cost,count,grip,translation,transformed,hand,a,n,f,contacts)
        _,count,grip,translation,transformed,hand,a,n,f,contacts=best
        if count:
            output.mkdir(parents=True,exist_ok=True)
            np.savez(output/f'{prefix}failed-fit.npz',hand=hand,hand_faces=hf,
                     knife=transformed,knife_faces=kf,knife_uv=kuv)
            raise ValueError(f'{prefix} no collision-free grip in bounded search, best={count} curl={grip} mount={(a,n,f)}; deepen fit, do not ship')
        # Independently close each finger toward the real handle, stopping BEFORE
        # surface intersection. No runtime collision solver or trial offsets.
        handle=transformed[kf[np.all(knife[kf,2]>-.08,axis=1)]]
        digit_points=[[i for i,w in enumerate(wedges) if names[max(psk[5][w[0]])[1]].startswith(prefix+d+'_')]
                      for d in ('thumb','index','middle','ring','pinky')]
        grip=list(grip)
        for digit in range(5):
            selected=None
            for curl in np.linspace(.1,1.,37):
                candidate=grip.copy();candidate[digit]=float(curl)
                candidate_hand=posed(psk,side,candidate)
                if intersections(candidate_hand[hf],transformed[kf]):continue
                gap=distance(candidate_hand[digit_points[digit]],handle)
                if gap<.05:continue
                score=(gap-.1)**2+.001*(curl-.7)**2
                if selected is None or score<selected[0]:selected=(score,candidate,candidate_hand)
            if selected:_,grip,hand=selected
        count=intersections(hand[hf],transformed[kf])
        contacts=[distance(hand[ii],handle) for ii in digit_points]
        if count:raise ValueError('refined grip intersects original knife')
        m=np.eye(4);m[:3,:3]=basis;m[3,:3]=translation
        m=m@reflect  # ActorX geometry -> original engine component coordinates.
        mounts.append(m);poses.append(grip)
        if not contacts or max(contacts)>1.:raise ValueError(f'{prefix} nonintersecting but fingers do not grip handle: {contacts}')
        reports.append(dict(side=prefix,triangle_intersections=count,curl=grip,contact_distance_uu=contacts,
                            anatomical_mount_uu=dict(across=a,dorsal=n,forward=f)))
        output.mkdir(parents=True,exist_ok=True)
        # Private geometry for the independent CPU joint preview/regression.
        np.savez(output/f'{prefix}assembly.npz',hand=hand,hand_faces=hf,
                 knife=transformed,knife_faces=kf,knife_uv=kuv)
    raw=struct.pack('<4sIII4f',b'SVNG',1,90,2,units,75.,52.5,0.)
    for m,g in zip(mounts,poses):raw+=np.asarray(m,dtype='<f4').tobytes()+struct.pack('<5f',*g)
    (output/'knife_hand.svng').write_bytes(raw)
    report=dict(hands=reports,knife_vertices=len(knife),knife_triangles=len(kf),
                native_indices=header[2],units=units,calibration_gun_pct=75.,calibration_world_uu=52.5,
                psk_sha256=hashlib.sha256(Path(psk_path).read_bytes()).hexdigest(),
                native_sha256=hashlib.sha256(native).hexdigest(),knife_sha256=hashlib.sha256(Path(knife_path).read_bytes()).hexdigest(),
                cache_sha256=hashlib.sha256(raw).hexdigest(),
                limitation='offline rigid assembly fit, not runtime material/depth/physical proof')
    (output/'grip-report.json').write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps(report,indent=2))


if __name__=='__main__':
    ap=argparse.ArgumentParser();ap.add_argument('--psk',required=True,type=Path)
    ap.add_argument('--native',required=True,type=Path);ap.add_argument('--knife',required=True,type=Path)
    ap.add_argument('--output',required=True,type=Path);args=ap.parse_args()
    fit(args.psk,args.native,args.knife,args.output)
