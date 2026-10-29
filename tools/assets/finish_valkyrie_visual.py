"""Author conservative rigid HD details mapped to existing Valkyrie bones."""
import argparse
import json
import math
from pathlib import Path

import numpy as np


def load_obj(path):
    v=[];f=[]
    for line in path.read_text(encoding='utf-8').splitlines():
        row=line.split()
        if not row:continue
        if row[0]=='v':v.append([float(x) for x in row[1:4]])
        elif row[0]=='f':f.append([int(x.split('/')[0])-1 for x in row[1:4]])
    return np.asarray(v,float),np.asarray(f,int)


def side_surface(vertices,faces,side,y,z):
    hits=[]
    for tri in faces:
        p=vertices[tri];yz=p[:,1:]
        m=np.array([[yz[0,0]-yz[2,0],yz[1,0]-yz[2,0]],
                    [yz[0,1]-yz[2,1],yz[1,1]-yz[2,1]]])
        det=np.linalg.det(m)
        if abs(det)<1e-7:continue
        a,b=np.linalg.solve(m,np.array([y,z])-yz[2]);c=1-a-b
        if min(a,b,c)<.03:continue
        n=np.cross(p[1]-p[0],p[2]-p[0])
        if np.linalg.norm(n)<1e-8 or abs(n[0])/np.linalg.norm(n)<.3:continue
        x=a*p[0,0]+b*p[1,0]+c*p[2,0]
        if side*x>.1:hits.append(x)
    return max(hits,key=lambda x:side*x) if hits else None


def disk(center,normal,radius,depth,sides=16):
    n=np.asarray(normal,float);n/=np.linalg.norm(n)
    a=np.array([0.,1.,0.]) if abs(n[1])<.9 else np.array([0.,0.,1.])
    u=np.cross(n,a);u/=np.linalg.norm(u);w=np.cross(n,u)
    v=[]
    for h,r in ((0.,radius*.85),(depth*.25,radius),(depth*.72,radius),(depth,radius*.72)):
        v.extend([center+n*h+r*(u*math.cos(2*math.pi*i/sides)+w*math.sin(2*math.pi*i/sides)) for i in range(sides)])
    f=[]
    for k in range(3):
        for i in range(sides):
            j=(i+1)%sides
            f.extend([(k*sides+i,k*sides+j,(k+1)*sides+j),
                      (k*sides+i,(k+1)*sides+j,(k+1)*sides+i)])
    for i in range(1,sides-1):f.append((3*sides,3*sides+i+1,3*sides+i))
    return v,f


def rib(side,x,y,z0,z1):
    y0,y1=y-.15,y+.15;ch=.08;th=.09
    outline=[(y0+ch,z0),(y1-ch,z0),(y1,z0+ch),(y1,z1-ch),
             (y1-ch,z1),(y0+ch,z1),(y0,z1-ch),(y0,z0+ch)]
    v=[(side*x,yy,zz) for yy,zz in outline]+[(side*(x+th),yy,zz) for yy,zz in outline]
    f=[]
    for i in range(1,7):f.append((8,8+i,8+i+1))
    for i in range(8):
        j=(i+1)%8;f.extend(((i,j,8+j),(i,8+j,8+i)))
    return v,f


def write(path,vertices,faces):
    with path.open('w',encoding='utf-8',newline='\n') as out:
        out.write(f'# Original PSK global bind pose; parent to manifest source_bone\nmtllib valkyrie_detail.mtl\no {path.stem}\n')
        for v in vertices:out.write('v %.8f %.8f %.8f\n'%tuple(v))
        out.write('usemtl machined_dark_metal\n')
        for a,b,c in faces:out.write(f'f {a+1} {b+1} {c+1}\n')


def append(v_all,f_all,v,f):
    offset=len(v_all);v_all.extend(v);f_all.extend([(a+offset,b+offset,c+offset) for a,b,c in f])


def main():
    p=argparse.ArgumentParser();p.add_argument('--visual',type=Path,required=True);a=p.parse_args()
    root=a.visual;rig=json.loads((root/'clean/rig_manifest.json').read_text())
    if rig['source_psk_sha256']!='b445a4a2c27e9eedfc487a2cd5358b6db1bd755b184cafd663c4a2454991b442':
        raise ValueError('unrecognized source rig')
    report={}
    # Magazine ribs follow its measured, planar side walls. They stay within
    # the original x extent and do not span the top lip or insertion stop.
    mv,mf=load_obj(root/'clean/magazine.obj');v_all=[];f_all=[];anchors=[]
    rib_rows=[(95,55,53.6,56.4),(97,55,53.4,56.2),(99,53,51.5,54.5),
              (101,53,51.2,54.3),(103,53,50.5,54.0),(105,52,50.2,53.7)]
    for side in (-1,1):
        for y,z,z0,z1 in rib_rows:
            x=side_surface(mv,mf,side,y,z)
            if x is None:raise ValueError(f'magazine surface missing at {side,y,z}')
            v,f=rib(side,abs(x)+.008,y,z0,z1);append(v_all,f_all,v,f)
            anchors.append({'side':side,'source_surface_x_uu':float(x),'y_uu':y,'z_uu':z})
    write(root/'clean/magazine_hd_ribs.obj',v_all,f_all)
    report['magazine_hd_ribs']={'source_bone':'clip','source_part':'magazine',
                                'vertices':len(v_all),'triangles':len(f_all),'anchors':anchors}
    # Four paired service fasteners on broad, actual receiver faces.
    bv,bf=load_obj(root/'clean/weapon_body.obj');v_all=[];f_all=[];anchors=[]
    for side in (-1,1):
        for y,z in ((108,65),(108,70),(112,30),(112,35)):
            x=side_surface(bv,bf,side,y,z)
            if x is None:raise ValueError(f'body surface missing at {side,y,z}')
            center=np.array([x+side*.012,y,z]);v,f=disk(center,[side,0,0],.28,.075)
            append(v_all,f_all,v,f);anchors.append({'center_uu':center.tolist(),'side':side})
    write(root/'clean/weapon_body_hd_fasteners.obj',v_all,f_all)
    report['weapon_body_hd_fasteners']={'source_bone':'cog_machinegun','source_part':'weapon_body',
                                       'vertices':len(v_all),'triangles':len(f_all),'anchors':anchors}
    # The source fan already has radial vanes; add only its central hub.
    fv,ff=load_obj(root/'clean/fan.obj');hub_center=np.array([fv[:,0].max()+.012,112.1,39.15])
    v,f=disk(hub_center,[1,0,0],.31,.12,20)
    write(root/'clean/fan_hd_hub.obj',v,f)
    report['fan_hd_hub']={'source_bone':'fan','source_part':'fan','vertices':len(v),
                          'triangles':len(f),'center_uu':hub_center.tolist()}
    (root/'clean/valkyrie_detail.mtl').write_text(
        'newmtl machined_dark_metal\nKd .18 .19 .18\nKs .52 .52 .52\nNs 160\n',encoding='utf-8')
    (root/'clean/hd_detail_manifest.json').write_text(json.dumps({'source_psk_sha256':rig['source_psk_sha256'],
        'details':report,'source_geometry_unchanged':True,
        'binding':'Convert global OBJ points through original inverse bind and parent to source_bone.'},indent=2)+'\n',encoding='utf-8')
    print(json.dumps({k:{'triangles':r['triangles'],'source_bone':r['source_bone']} for k,r in report.items()},indent=2))


if __name__=='__main__':main()
