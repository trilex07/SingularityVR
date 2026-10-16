"""Offline-only extraction of original rigid Centurion clip with native UVs.

No Steam defaults, no generated replacement art, no asset redistribution.
The cache is for the next native material/socket integration, not loaded by the
current calibration overlay. Bone-local axes must not be called grip axes.
"""
import argparse
import hashlib
import json
from pathlib import Path
import struct
import sys
import numpy as np

sys.path.insert(0, str(Path(__file__).resolve().parents[1]/'gloves'))
from prepare_gloves import read_psk


def diffuse_cache(image):
    """Exact original albedo -> existing SVRT upload format; alpha is data."""
    w,h=image.size
    if not 0<w<=2048 or not 0<h<=2048:
        raise ValueError('diffuse dimensions exceed bounded upload')
    rgba=image.convert('RGBA')
    data=struct.pack('<4sIII',b'SVRT',1,w,h)+rgba.tobytes('raw','BGRA')
    if data[16:]!=np.array(rgba)[:,:,[2,1,0,3]].tobytes():
        raise ValueError('diffuse channel roundtrip failed')
    return data


def extract(psk, units=52.5):
    names, parents, _, bind, points, weights, wedges, faces = psk
    if not np.isfinite(units) or not 10 <= units <= 1000:
        raise ValueError('invalid explicit mesh units/metre')
    if len(set(names)) != len(names) or 'clip' not in names or 'r_arm_wrist' not in names:
        raise ValueError('missing/ambiguous Centurion bones')
    clip, wrist = names.index('clip'), names.index('r_arm_wrist')
    # Include descendants, never a spatial cut or a dominant-weight shortcut.
    subtree = {clip}
    for i, parent in enumerate(parents):
        if i and parent in subtree:
            subtree.add(i)
    selected = set()
    for point, row in weights.items():
        if not 0 <= point < len(points) or any(not np.isfinite(w) or not 0 <= b < len(names) for w,b in row):
            raise ValueError('bad weights')
        total = sum(w for w,_ in row)
        if abs(total-1) > 1e-4:
            raise ValueError('unnormalized skin')
        if sum(w for w,b in row if b in subtree) >= 1-1e-6:
            selected.add(point)
    if not selected:
        raise ValueError('no rigid clip vertices')
    reflect = np.diag([1.,-1.,1.,1.])
    raw_bind = reflect @ bind @ reflect
    inv = np.linalg.inv(raw_bind[clip])
    if not np.isfinite(inv).all() or not np.isfinite(points).all():
        raise ValueError('invalid bind/positions')
    vertices, indices, remap, material_ids = [], [], {}, set()
    crossing = 0
    for face in faces:
        if any(not 0 <= k < len(wedges) for k in face[:3]):
            raise ValueError('bad wedge index')
        corners = [wedges[k] for k in face[:3]]
        if any(not 0 <= p < len(points) or not np.isfinite([u,v]).all() for p,u,v in corners):
            raise ValueError('bad point/UV')
        inside = [p in selected for p,_,_ in corners]
        if any(inside) and not all(inside):
            crossing += 1
        if not all(inside):
            continue
        material_ids.add(face[3])
        for p,u,v in corners:
            key = (p,u,v)
            if key not in remap:
                remap[key] = len(vertices)
                xyz = (np.r_[points[p],1.] @ inv)[:3]/units
                vertices.append((*xyz,0xffffffff,u,v))
            indices.append(remap[key])
    if crossing or material_ids != {0} or not indices or len(vertices)>65535:
        raise ValueError('mixed boundary/material/empty/oversized clip')
    xyz = np.array([v[:3] for v in vertices])
    if np.linalg.norm(xyz,axis=1).max() > 1:
        raise ValueError('clip exceeds bounded cache scale')
    cache = struct.pack('<4sIII',b'SVRK',2,len(vertices),len(indices))
    cache += b''.join(struct.pack('<3fI2f',*v) for v in vertices)
    cache += struct.pack('<'+'H'*len(indices),*indices)
    # Full round trip, not a count-only proof of coordinate conversion.
    encoded_xyz=np.array([struct.unpack_from('<3f',cache,16+24*i) for i in range(len(vertices))])
    local_uu = np.c_[encoded_xyz*units,np.ones(len(xyz))]
    source_ids = [key[0] for key in remap]
    if not np.allclose((local_uu @ raw_bind[clip])[:,:3],points[source_ids],rtol=0,atol=2e-5):
        raise ValueError('bind roundtrip failed')
    relative = raw_bind[clip] @ np.linalg.inv(raw_bind[wrist])
    return cache,dict(source_wedges=len(wedges),source_triangles=len(faces),
        bones=len(names),clip_bone=clip,clip_subtree=[names[i] for i in sorted(subtree)],
        rigid_points=len(selected),vertices=len(vertices),triangles=len(indices)//3,
        crossing_triangles=crossing,materials=sorted(material_ids),
        bounds_clip_metres=[xyz.min(0).tolist(),xyz.max(0).tolist()],
        dimensions_metres=np.ptp(xyz,axis=0).tolist(),units_per_metre=units,
        clip_to_wrist_bind=relative.tolist(),uv='original PSK, upper-left V; no flip',
        axes='original clip bone-local, NOT controller grip-local',
        roundtrip='every encoded FLOAT32 cache vertex reconstructed in raw PSK bind space, absolute error <= 2e-5 UU',
        texture='not inferred; native material capture remains separate',
        output_sha256=hashlib.sha256(cache).hexdigest())


def preview(cache, destination, label='Original Centurion clip'):
    """Three-axis geometry inspection, no synthetic texture/material claim."""
    from PIL import Image, ImageDraw
    _,version,n,count=struct.unpack_from('<4sIII',cache)
    if version!=2 or len(cache)!=16+24*n+count*2:
        raise ValueError('invalid preview cache')
    rows=np.array([struct.unpack_from('<3fI2f',cache,16+24*i)[:3] for i in range(n)])
    indices=np.frombuffer(cache,'<u2',count,16+24*n).reshape(-1,3)
    image=Image.new('RGB',(1440,520),'#10171b');draw=ImageDraw.Draw(image)
    for view,axes in enumerate(((0,1),(0,2),(1,2))):
        xy=rows[:,axes];centre=(xy.min(0)+xy.max(0))*.5
        scale=380/max(np.ptp(xy,axis=0).max(),.001)
        xy=(xy-centre)*[scale,-scale]+[240+view*480,270]
        for face in indices:
            points=[tuple(xy[i]) for i in face]
            draw.line(points+[points[0]],fill='#72868d',width=1)
        draw.text((view*480+24,20),f'{label} | axes {axes}',fill='white')
    draw.text((24,490),'Geometry only / original UV preserved / no material or runtime pose proof',fill='white')
    image.save(destination)


def textured_preview(cache, texture, destination):
    """CPU orthographic z-buffer: original UV/albedo only, not UE3 lighting."""
    from PIL import Image, ImageDraw
    magic,version,n,count=struct.unpack_from('<4sIII',cache)
    tm,tv,w,h=struct.unpack_from('<4sIII',texture)
    if magic!=b'SVRK' or version!=2 or len(cache)!=16+24*n+count*2 or count%3:
        raise ValueError('invalid preview mesh')
    if tm!=b'SVRT' or tv!=1 or not 0<w<=2048 or not 0<h<=2048 or len(texture)!=16+w*h*4:
        raise ValueError('invalid preview texture')
    rows=[struct.unpack_from('<3fI2f',cache,16+24*i) for i in range(n)]
    xyz=np.array([v[:3] for v in rows]);uv=np.array([v[4:] for v in rows])
    faces=np.frombuffer(cache,'<u2',count,16+24*n).reshape(-1,3)
    if not n or faces.max()>=n or not np.isfinite(xyz).all() or not np.isfinite(uv).all():
        raise ValueError('invalid preview vertices')
    pixels=np.frombuffer(texture,'u1',w*h*4,16).reshape(h,w,4)[:,:,[2,1,0]]
    xyz-=(xyz.min(0)+xyz.max(0))*.5
    image=Image.new('RGB',(1440,540),'#10171b');draw=ImageDraw.Draw(image)
    for view,axes in enumerate(((0,1,2),(0,2,1),(2,1,0))):
        camera=xyz[:,axes];scale=350/max(np.ptp(camera[:,:2],axis=0).max(),.001)
        xy=camera[:,:2]*[scale,-scale]+[240,250]
        rgb=np.full((480,480,3),[16,23,27],dtype=np.uint8)
        depth=np.full((480,480),-np.inf)
        for face in faces:
            a,b,c=xy[face];lo=np.maximum(np.floor(xy[face].min(0)).astype(int),0)
            hi=np.minimum(np.ceil(xy[face].max(0)).astype(int),479)
            if (hi<lo).any():continue
            det=(b[1]-c[1])*(a[0]-c[0])+(c[0]-b[0])*(a[1]-c[1])
            if abs(det)<1e-12:continue
            yy,xx=np.mgrid[lo[1]:hi[1]+1,lo[0]:hi[0]+1]
            u=((b[1]-c[1])*(xx+.5-c[0])+(c[0]-b[0])*(yy+.5-c[1]))/det
            v=((c[1]-a[1])*(xx+.5-c[0])+(a[0]-c[0])*(yy+.5-c[1]))/det
            t=1-u-v;z=u*camera[face[0],2]+v*camera[face[1],2]+t*camera[face[2],2]
            region=depth[lo[1]:hi[1]+1,lo[0]:hi[0]+1]
            mask=(u>=0)&(v>=0)&(t>=0)&(z>region)
            coords=u[:,:,None]*uv[face[0]]+v[:,:,None]*uv[face[1]]+t[:,:,None]*uv[face[2]]
            texel=np.floor(coords*[w,h]).astype(int)%[w,h]
            dest=rgb[lo[1]:hi[1]+1,lo[0]:hi[0]+1]
            dest[mask]=pixels[texel[:,:,1],texel[:,:,0]][mask];region[mask]=z[mask]
        image.paste(Image.fromarray(rgb),(view*480,25))
        draw.text((view*480+20,12),f'Original Centurion cylinder | axes {axes[:2]}',fill='white')
    draw.text((20,515),'CPU original UV/albedo only / no normal, specular, scene lighting or runtime-pose claim',fill='white')
    image.save(destination)


if __name__=='__main__':
    ap=argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--psk',type=Path,required=True)
    ap.add_argument('--output',type=Path,required=True)
    ap.add_argument('--units-per-metre',type=float,default=52.5)
    ap.add_argument('--preview',action='store_true',help='write private 3-axis wireframe PNG (Pillow)')
    ap.add_argument('--diffuse',type=Path,help='original AR_Pistol_d DDS from the measured material; private only')
    args=ap.parse_args()
    if args.psk.name!='ch_inview_alt_pistol_mesh.psk':
        ap.error('requires measured Centurion asset, not generic slide pistol')
    psk=read_psk(args.psk)
    if len(psk[6])!=5799:
        ap.error('unexpected Centurion topology; review rather than guess')
    data,report=extract(psk,args.units_per_metre)
    report['source_sha256']=hashlib.sha256(args.psk.read_bytes()).hexdigest()
    args.output.mkdir(parents=True,exist_ok=True)
    if args.diffuse:
        if args.diffuse.name!='AR_Pistol_d.dds':
            ap.error('requires measured AR_Pistol_d, not a guessed generic handgun texture')
        from PIL import Image
        with Image.open(args.diffuse) as image:
            texture=diffuse_cache(image)
        (args.output/'centurion_clip.svrt').write_bytes(texture)
        report['texture']=dict(source_sha256=hashlib.sha256(args.diffuse.read_bytes()).hexdigest(),
            output_sha256=hashlib.sha256(texture).hexdigest(),channels='BGRA, unchanged alpha data',
            scope='original albedo preview only; NOT complete native material')
        if args.preview:textured_preview(data,texture,args.output/'clip-textured.png')
    (args.output/'centurion_clip.svrk').write_bytes(data)
    (args.output/'report.json').write_text(json.dumps(report,indent=2)+'\n',encoding='utf-8')
    if args.preview:preview(data,args.output/'clip-geometry.png')
    print(json.dumps(report,indent=2))
