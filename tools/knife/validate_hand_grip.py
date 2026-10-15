"""Independent source/pose replay + textured depth preview; never Steam."""
import argparse
import json
from pathlib import Path
import struct
import sys
import numpy as np
from PIL import Image, ImageDraw
from fit_hand_grip import read_psk, read_knife, posed, intersections, distance, inside, unit


def raster(points,faces,uv,texture,axes,size=420):
    camera=points@axes.T;xy=camera[:,:2]*850+[size/2,size/2]
    image=np.full((size,size,3),[20,26,31],dtype=np.uint8);depth=np.full((size,size),-np.inf)
    h,w=texture.shape[:2]
    for f in faces:
        a,b,c=xy[f];lo=np.maximum(np.floor(xy[f].min(0)).astype(int),0);hi=np.minimum(np.ceil(xy[f].max(0)).astype(int),size-1)
        if (hi<lo).any():continue
        yy,xx=np.mgrid[lo[1]:hi[1]+1,lo[0]:hi[0]+1]
        d=(b[1]-c[1])*(a[0]-c[0])+(c[0]-b[0])*(a[1]-c[1])
        if abs(d)<1e-12:continue
        q0=((b[1]-c[1])*(xx+.5-c[0])+(c[0]-b[0])*(yy+.5-c[1]))/d
        q1=((c[1]-a[1])*(xx+.5-c[0])+(a[0]-c[0])*(yy+.5-c[1]))/d;q2=1-q0-q1
        z=q0*camera[f[0],2]+q1*camera[f[1],2]+q2*camera[f[2],2]
        region=depth[lo[1]:hi[1]+1,lo[0]:hi[0]+1]
        mask=(q0>=0)&(q1>=0)&(q2>=0)&(z>region)
        texcoord=q0[:,:,None]*uv[f[0]]+q1[:,:,None]*uv[f[1]]+q2[:,:,None]*uv[f[2]]
        ix=np.floor(texcoord*[w,h]).astype(int)%[w,h]
        rgb=texture[ix[:,:,1],ix[:,:,0]]
        dest=image[lo[1]:hi[1]+1,lo[0]:hi[0]+1];dest[mask]=rgb[mask];region[mask]=z[mask]
    return image,depth


def validate(prepared,psk_path,native_path,knife_path,hand_diffuse,knife_diffuse):
    raw=(prepared/'knife_hand.svng').read_bytes()
    assert len(raw)==200 and struct.unpack_from('<4sIII',raw)==(b'SVNG',1,90,2)
    psk=read_psk(psk_path);names,_,_,bind,points,weights,wedges,_=psk
    cache=Path(native_path).read_bytes();ni=struct.unpack_from('<I',cache,8)[0]
    hand_faces=np.frombuffer(cache,'<u2',ni,32).reshape(-1,3)
    kv,kf,kuv=read_knife(knife_path)
    hand_texture=np.array(Image.open(hand_diffuse).convert('RGB'))
    tex=Path(knife_diffuse).read_bytes();_,_,w,h=struct.unpack_from('<4sIII',tex)
    knife_texture=np.frombuffer(tex,'u1',w*h*4,16).reshape(h,w,4)[:,:,[2,1,0]]
    report=[];golden_h=[];golden_k=[];views=[]
    reflect=np.diag([1.,-1.,1.,1.]);raw_bind=reflect@bind@reflect
    for side,prefix in enumerate(('l_','r_')):
        m=np.frombuffer(raw,'<f4',16,32+side*84).reshape(4,4).astype(float)
        grip=np.frombuffer(raw,'<f4',5,96+side*84).astype(float)
        hand=posed(psk,side,grip);knife=(np.c_[kv,np.ones(len(kv))]@m@reflect)[:,:3]
        sign=1 if side==0 else -1
        hf=hand_faces[np.all(psk[4][[wedges[int(x)][0] for x in hand_faces.ravel()]].reshape(-1,3,3)[:,:,0]*sign>0,axis=1)]
        hits=intersections(hand[hf],knife[kf]);assert hits==0,hits
        buried=int(inside(knife,hand[hf]).sum());assert buried==0,buried
        handle=knife[kf[np.all(kv[kf,2]>-.08,axis=1)]]
        contact=[]
        for d in ('thumb','index','middle','ring','pinky'):
            ii=[i for i,w in enumerate(wedges) if names[max(weights[w[0]])[1]].startswith(prefix+d+'_')]
            contact.append(distance(hand[ii],handle))
        assert min(contact)>.049 and max(contact)<.3,contact
        report.append(dict(side=prefix,triangle_intersections=hits,knife_vertices_inside_glove=buried,contact_uu=contact))
        used,remap=np.unique(hf,return_inverse=True)
        for wedge in used:
            point=wedges[int(wedge)][0];wp=weights[point];assert len(wp)<=4
            total=sum(w for w,_ in wp);ww=[w/total for w,_ in wp]+[0.]*(4-len(wp));bones=[b for _,b in wp]+[0]*(4-len(wp))
            original=points[point]*[1,-1,1];expected=hand[int(wedge)]*[1,-1,1]
            golden_h.append(struct.pack('<3f4f4I3f',*original,*ww,*bones,*expected))
        for original,expected in zip(kv,knife*[1,-1,1]):golden_k.append(struct.pack('<I3f3f',side,*original,*expected))
        # Software Z rendering both original diffuse maps. This is not UE3 light.
        wrist=raw_bind[9 if side==0 else 51,3,:3]
        forward=unit(raw_bind[names.index(prefix+'middle_1'),3,:3]-wrist)
        across=unit(raw_bind[names.index(prefix+'index_1'),3,:3]-raw_bind[names.index(prefix+'pinky_1'),3,:3])
        dorsal=unit(np.cross(across,forward))*(1 if side==0 else -1)
        hp=(hand[used]-wrist)/52.5*.75;kp=(knife-wrist)/52.5*.75
        centre=np.mean(hp,axis=0)
        for view,axes in enumerate((np.array([across,-forward,dorsal]),np.array([across,forward,-dorsal]),
                                    np.array([forward,dorsal,across]),np.array([-forward,dorsal,-across]))):
            h_image,h_depth=raster(hp-centre,remap.reshape(-1,3),np.array([wedges[i][1:] for i in used]),hand_texture,axes)
            k_image,k_depth=raster(kp-centre,kf,kuv,knife_texture,axes)
            mask=k_depth>h_depth;h_image[mask]=k_image[mask]
            im=Image.fromarray(h_image);ImageDraw.Draw(im).text((8,8),prefix+' original UV / view '+str(view),fill='white');views.append(im)
    out=struct.pack('<4sII',b'SVJG',len(golden_h),len(golden_k))+b''.join(golden_h)+b''.join(golden_k)
    (prepared/'joint-golden.bin').write_bytes(out)
    atlas=Image.new('RGB',(4*420,2*420))
    for i,im in enumerate(views):atlas.paste(im,((i%4)*420,(i//4)*420))
    atlas.save(prepared/'joint-uv-depth.png')
    (prepared/'validation.json').write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps(dict(validation=report,golden_hand=len(golden_h),golden_knife=len(golden_k)),indent=2))


if __name__=='__main__':
    ap=argparse.ArgumentParser();ap.add_argument('--prepared',required=True,type=Path)
    for name in ('psk','native','knife','hand-diffuse','knife-diffuse'):ap.add_argument('--'+name,required=True,type=Path)
    a=ap.parse_args();validate(a.prepared,a.psk,a.native,a.knife,a.hand_diffuse,a.knife_diffuse)
