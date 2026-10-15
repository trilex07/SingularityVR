"""CPU geometry preview, never a game or physical verification claim."""
import argparse
import json
from pathlib import Path
import struct
import numpy as np
from PIL import Image, ImageDraw
from prepare_gloves import read_glb


def cache(path):
    raw=Path(path).read_bytes()
    magic,version,nv,ni=struct.unpack_from('<4sIII',raw)
    assert magic==b'SVRG' and version in (1,2)
    start=24 if version==2 else 16
    v=np.frombuffer(raw,'<f4',nv*14,start).reshape(nv,14)
    f=np.frombuffer(raw,'<u4',ni,start+nv*56).reshape(-1,3)
    return v,f


def panel(points,faces,axes,label,cap_start=None,extent=.34):
    size=440
    image=Image.new('RGB',(size,size),(18,23,28))
    d=ImageDraw.Draw(image)
    a,b,depth=axes
    projected=np.stack([points@a,points@b],axis=1)
    centre=(projected.max(0)+projected.min(0))*.5
    projected=(projected-centre)*(size-50)/extent+[size/2,size/2]
    order=np.argsort(points[faces].mean(1)@depth)
    for i in order:
        xyz=points[faces[i]]
        normal=np.cross(xyz[1]-xyz[0],xyz[2]-xyz[0])
        n=np.linalg.norm(normal)
        light=.3+.7*abs(np.dot(normal,depth))/max(n,1e-12)
        color=tuple(int(c*light) for c in ((120,85,40) if cap_start is not None and i>=cap_start else (160,175,180)))
        d.polygon([tuple(p) for p in projected[faces[i]]],fill=color)
    d.text((12,12),label,fill='white')
    return image


def ray_distance(origin,direction,points,faces):
    triangles=points[faces]
    e1=triangles[:,1]-triangles[:,0];e2=triangles[:,2]-triangles[:,0]
    h=np.cross(np.broadcast_to(direction,e2.shape),e2)
    a=(e1*h).sum(1)
    valid=np.abs(a)>1e-10
    inv=np.zeros_like(a);inv[valid]=1/a[valid]
    s=origin-triangles[:,0]
    u=inv*(s*h).sum(1)
    q=np.cross(s,e1)
    v=inv*(q*direction).sum(1)
    t=inv*(e2*q).sum(1)
    valid &= (u>=-1e-7)&(v>=-1e-7)&(u+v<=1+1e-7)&(t>1e-7)
    return float(t[valid].min()) if valid.any() else float('inf')


def main():
    ap=argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--prepared',type=Path,required=True)
    ap.add_argument('--source',type=Path,required=True)
    args=ap.parse_args()
    report=json.loads((args.prepared/'report.json').read_text())
    views=[];rays={}
    for side in ('left','right'):
        v,f=cache(args.prepared/f'glove_{side}.svrg')
        p=v[:,:3].astype(float);target=p+v[:,3:6]
        top=np.array([[1,0,0],[0,0,1],[0,1,0]])
        side_axes=np.array([[0,0,1],[0,1,0],[1,0,0]])
        for axes,name in ((top,'dorsal'),(side_axes,'side')):
            views.append(panel(p,f,axes,f'{side} open / {name}'))
            views.append(panel(target,f,axes,f'{side} fist / {name}'))
        r=report['hands'][side]
        _,sp,sf=read_glb(args.source/f'glove_{side}.glb')
        sp=(sp-r['wrist_raw_uu'])@np.array(r['raw_to_grip_basis'])/r['units_per_metre']
        cap=f[r['cap_start_triangle']:]
        centre=p[np.unique(cap)].mean(0)
        origin=centre+np.array([0,0,.15]);direction=np.array([0,0,-1.])
        before=ray_distance(origin,direction,sp,sf)
        after=ray_distance(origin,direction,p,f)
        cap_hit=ray_distance(origin,direction,p,cap)
        rays[side]=dict(before=before,after=after,cap=cap_hit,
                        cap_is_first=abs(after-cap_hit)<1e-6,
                        source_sees_past_cuff=before>after+.001)
        axial=np.array([[1,0,0],[0,-1,0],[0,0,1]])
        views.append(panel(sp,sf,axial,f'{side} cuff BEFORE'))
        views.append(panel(p,f,axial,f'{side} cuff AFTER',r['cap_start_triangle']))
    atlas=Image.new('RGB',(4*440,3*440))
    for i,img in enumerate(views):
        atlas.paste(img,((i%4)*440,(i//4)*440))
    atlas.save(args.prepared/'geometry-preview.png')
    (args.prepared/'axial-rays.json').write_text(json.dumps(rays,indent=2)+'\n')
    print(json.dumps(rays,indent=2))
    if not all(r['cap_is_first'] and r['source_sees_past_cuff'] for r in rays.values()):
        raise SystemExit('axial falsifier failed')


if __name__=='__main__':
    main()
