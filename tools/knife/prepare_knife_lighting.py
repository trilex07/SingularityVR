"""Private native knife UV mesh -> tangent frame + original normal/spec maps."""
import argparse
import hashlib
import json
from pathlib import Path
import struct
import numpy as np
from PIL import Image
from fit_hand_grip import read_knife


def frames(vertices,faces,uv):
    n=np.zeros_like(vertices);t=np.zeros_like(vertices);b=np.zeros_like(vertices)
    volume=float(np.sum(vertices[faces[:,0]]*np.cross(vertices[faces[:,1]],vertices[faces[:,2]]))/6)
    for face in faces:
        p=vertices[face];a=p[1]-p[0];c=p[2]-p[0];normal=np.cross(a,c)*(1 if volume>0 else -1)
        delta=uv[face[1:]]-uv[face[0]];det=np.linalg.det(delta)
        n[face]+=normal
        if abs(det)>1e-12:
            t[face]+=(a*delta[1,1]-c*delta[0,1])/det
            b[face]+=(c*delta[0,0]-a*delta[1,0])/det
    for i in range(len(vertices)):
        if np.linalg.norm(n[i])<1e-12:raise ValueError('degenerate normal')
        n[i]/=np.linalg.norm(n[i]);t[i]-=n[i]*(n[i]@t[i])
        if np.linalg.norm(t[i])<1e-12:
            axis=np.eye(3)[np.argmin(np.abs(n[i]))];t[i]=np.cross(axis,n[i])
        t[i]/=np.linalg.norm(t[i])
    sign=np.where(np.sum(np.cross(n,t)*b,axis=1)<0,-1.,1.)
    return np.c_[vertices,n,t,sign,uv].astype('<f4')


def prepare(mesh,normal,specular,output):
    v,f,uv=read_knife(mesh);rows=frames(v,f,uv);output.mkdir(parents=True,exist_ok=True)
    raw=struct.pack('<4sIII',b'SVNL',1,len(v),len(f)*3)+rows.tobytes()+f.astype('<u2').tobytes()
    (output/'knife.svnl').write_bytes(raw);report={'mesh_sha256':hashlib.sha256(Path(mesh).read_bytes()).hexdigest()}
    for src,name in ((normal,'knife_normal.svrt'),(specular,'knife_specular.svrt')):
        im=Image.open(src).convert('RGBA');w,h=im.size
        if not 0<w<=2048 or not 0<h<=2048:raise ValueError('texture bounds')
        raw=struct.pack('<4sIII',b'SVRT',1,w,h)+np.array(im)[:,:,[2,1,0,3]].tobytes()
        (output/name).write_bytes(raw)
        report[name]={'source_sha256':hashlib.sha256(Path(src).read_bytes()).hexdigest(),'sha256':hashlib.sha256(raw).hexdigest()}
    report.update(vertices=len(v),triangles=len(f),native_uv_preserved=True,normal_specular='original maps, no painted replacement')
    (output/'lighting-report.json').write_text(json.dumps(report,indent=2)+'\n')


if __name__=='__main__':
    p=argparse.ArgumentParser()
    for name in ('mesh','normal','specular','output'):p.add_argument('--'+name,type=Path,required=True)
    a=p.parse_args();prepare(a.mesh,a.normal,a.specular,a.output)
