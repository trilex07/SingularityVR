"""Private CPU UV rasterization; not game/GPU/headset or native lighting proof."""
import argparse
from pathlib import Path
import struct
import numpy as np
from PIL import Image, ImageDraw


def preview(directory):
    raw=(directory/'knife.svrk').read_bytes()
    magic,version,n,ni=struct.unpack_from('<4sIII',raw)
    if (magic,version)!=(b'SVRK',2):raise ValueError('requires UV cache v2')
    v=np.array([struct.unpack_from('<3fI2f',raw,16+i*24) for i in range(n)])
    faces=np.frombuffer(raw,'<u2',ni,16+n*24).reshape(-1,3)
    tex=(directory/'knife.svrt').read_bytes()
    magic,version,w,h=struct.unpack_from('<4sIII',tex)
    if (magic,version)!=(b'SVRT',1):raise ValueError('texture cache')
    texture=np.frombuffer(tex,'u1',w*h*4,16).reshape(h,w,4)[:,:,[2,1,0]]
    atlas=Image.new('RGB',(700,760),(24,29,34))
    for side in range(2):
        image=np.full((760,350,3),[24,29,34],dtype=np.uint8)
        xy=v[:,[0,2]].copy();xy[:,0]*=1 if side==0 else -1
        xy-=((xy.min(0)+xy.max(0))/2);xy*=650/np.ptp(v[:,2]);xy+=[175,385]
        order=np.argsort(v[faces,1].mean(1)*(1 if side==0 else -1))
        for f in faces[order]:
            a,b,c=xy[f];lo=np.maximum(np.floor(xy[f].min(0)).astype(int),[0,0]);hi=np.minimum(np.ceil(xy[f].max(0)).astype(int),[349,759])
            if (hi<lo).any():continue
            yy,xx=np.mgrid[lo[1]:hi[1]+1,lo[0]:hi[0]+1];p=np.stack([xx+.5,yy+.5],axis=-1)
            d=(b[1]-c[1])*(a[0]-c[0])+(c[0]-b[0])*(a[1]-c[1])
            if abs(d)<1e-12:continue
            u=((b[1]-c[1])*(p[:,:,0]-c[0])+(c[0]-b[0])*(p[:,:,1]-c[1]))/d
            t=((c[1]-a[1])*(p[:,:,0]-c[0])+(a[0]-c[0])*(p[:,:,1]-c[1]))/d
            s=1-u-t;mask=(u>=0)&(t>=0)&(s>=0)
            uv=u[:,:,None]*v[f[0],4:6]+t[:,:,None]*v[f[1],4:6]+s[:,:,None]*v[f[2],4:6]
            indices=np.floor(uv*[w,h]).astype(int)%[w,h]
            colours=texture[indices[:,:,1],indices[:,:,0]]
            region=image[lo[1]:hi[1]+1,lo[0]:hi[0]+1];region[mask]=colours[mask]
        im=Image.fromarray(image);ImageDraw.Draw(im).text((12,12),'CPU original UV / '+('front' if side==0 else 'back'),fill='white')
        atlas.paste(im,(side*350,0))
    atlas.save(directory/'uv-preview.png')


if __name__=='__main__':
    ap=argparse.ArgumentParser();ap.add_argument('directory',type=Path);args=ap.parse_args();preview(args.directory)
