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


def preview(cache, destination):
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
        draw.text((view*480+24,20),f'Original Centurion clip | axes {axes}',fill='white')
    draw.text((24,490),'Geometry only / original UV preserved / no material or runtime pose proof',fill='white')
    image.save(destination)


if __name__=='__main__':
    ap=argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--psk',type=Path,required=True)
    ap.add_argument('--output',type=Path,required=True)
    ap.add_argument('--units-per-metre',type=float,default=52.5)
    ap.add_argument('--preview',action='store_true',help='write private 3-axis wireframe PNG (Pillow)')
    args=ap.parse_args()
    if args.psk.name!='ch_inview_alt_pistol_mesh.psk':
        ap.error('requires measured Centurion asset, not generic slide pistol')
    psk=read_psk(args.psk)
    if len(psk[6])!=5799:
        ap.error('unexpected Centurion topology; review rather than guess')
    data,report=extract(psk,args.units_per_metre)
    report['source_sha256']=hashlib.sha256(args.psk.read_bytes()).hexdigest()
    args.output.mkdir(parents=True,exist_ok=True)
    (args.output/'centurion_clip.svrk').write_bytes(data)
    (args.output/'report.json').write_text(json.dumps(report,indent=2)+'\n',encoding='utf-8')
    if args.preview:preview(data,args.output/'clip-geometry.png')
    print(json.dumps(report,indent=2))
