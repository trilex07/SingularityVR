"""Offline only: existing rigid GLB + private PSK -> capped fist-morph GLB/cache.

No game paths/defaults. Requires numpy; optional Pillow for the axial preview.
Game-derived output is private and must not be committed/distributed.
"""
from __future__ import annotations
import argparse
from collections import Counter, defaultdict
import hashlib
import json
import math
from pathlib import Path
import struct
import time
import numpy as np


def sha(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def read_glb(path):
    data = Path(path).read_bytes()
    if len(data) < 20 or struct.unpack_from('<III', data) != (0x46546C67, 2, len(data)):
        raise ValueError('not GLB2')
    chunks, offset = {}, 12
    while offset < len(data):
        size, kind = struct.unpack_from('<II', data, offset)
        offset += 8
        if offset + size > len(data) or kind in chunks:
            raise ValueError('bad GLB chunks')
        chunks[kind] = data[offset:offset+size]
        offset += size
    doc = json.loads(chunks[0x4E4F534A])
    raw = chunks[0x004E4942]
    if len(doc.get('nodes', [])) != 1 or doc.get('skins') or doc.get('animations'):
        raise ValueError('expected measured one-node rigid glove')
    node = doc['nodes'][0]
    if any(k in node for k in ('matrix', 'translation', 'rotation', 'scale')):
        raise ValueError('unbaked node transform')
    prims = doc['meshes'][node['mesh']]['primitives']
    if len(prims) != 1 or prims[0].get('mode', 4) != 4:
        raise ValueError('expected one triangle primitive')
    def accessor(i):
        a = doc['accessors'][i]
        v = doc['bufferViews'][a['bufferView']]
        dtype = {5126: '<f4', 5125: '<u4', 5123: '<u2'}[a['componentType']]
        width = {'VEC3': 3, 'VEC2': 2, 'SCALAR': 1}[a['type']]
        stride = v.get('byteStride', np.dtype(dtype).itemsize * width)
        start = v.get('byteOffset', 0) + a.get('byteOffset', 0)
        size = np.dtype(dtype).itemsize * width
        if a.get('sparse') or stride < size or start+(a['count']-1)*stride+size > len(raw):
            raise ValueError('invalid accessor bounds')
        return np.array([np.frombuffer(raw, dtype, width, start+i*stride)
                         for i in range(a['count'])])
    p = accessor(prims[0]['attributes']['POSITION']).astype(float)
    f = accessor(prims[0]['indices']).reshape(-1, 3).astype(int)
    if not np.isfinite(p).all() or f.min() < 0 or f.max() >= len(p):
        raise ValueError('invalid mesh')
    return doc, p, f


def read_psk(path):
    data, offset, chunks = Path(path).read_bytes(), 0, {}
    while offset < len(data):
        name, _, size, count = struct.unpack_from('<20siii', data, offset)
        offset += 32
        if size < 0 or count < 0 or offset+size*count > len(data):
            raise ValueError('invalid PSK chunk')
        chunks[name.split(b'\0')[0].decode()] = (size, count, data[offset:offset+size*count])
        offset += size*count
    size, count, raw = chunks['REFSKELT']
    names, parents, local = [], [], []
    # Exact armbind::PskLocal conversion. Row vectors, UEViewer Y reflection.
    for i in range(count):
        r = raw[i*size:(i+1)*size]
        names.append(r[:64].split(b'\0')[0].decode())
        parents.append(struct.unpack_from('<i', r, 72)[0])
        x, y, z, w = struct.unpack_from('<4f', r, 76)
        x, y, z, w = np.array([x, -y, z, -w]) / np.linalg.norm([x, y, z, w])
        if i == 0:
            x, y, z = -x, -y, -z
        m = np.eye(4)
        m[:3, :3] = [[1-2*(y*y+z*z), 2*(x*y-w*z), 2*(x*z+w*y)],
                       [2*(x*y+w*z), 1-2*(x*x+z*z), 2*(y*z-w*x)],
                       [2*(x*z-w*y), 2*(y*z+w*x), 1-2*(x*x+y*y)]]
        px, py, pz = struct.unpack_from('<3f', r, 92)
        m[3, :3] = [px, -py, pz]
        local.append(m)
    absolute = []
    for i, m in enumerate(local):
        if i and not 0 <= parents[i] < i:
            raise ValueError('non-topological skeleton')
        absolute.append(m if i == 0 else m @ absolute[parents[i]])
    _, count, raw = chunks['PNTS0000']
    points = np.frombuffer(raw, '<f4', count*3).reshape(-1, 3).astype(float)
    size, count, raw = chunks['RAWWEIGHTS']
    weights = defaultdict(list)
    for i in range(count):
        weight, point, bone = struct.unpack_from('<fii', raw, i*size)
        if weight > 0:
            weights[point].append((weight, bone))
    size, count, raw = chunks['VTXW0000']
    wedges = [struct.unpack_from('<Iff', raw, i*size) for i in range(count)]
    size, count, raw = chunks['FACE0000']
    faces = [struct.unpack_from('<3HBBI', raw, i*size) for i in range(count)]
    return names, parents, local, np.array(absolute), points, weights, wedges, faces


def unit(v):
    n = np.linalg.norm(v)
    if n < 1e-9:
        raise ValueError('degenerate basis')
    return v/n


def rotation(axis, angle):
    x, y, z = unit(axis)
    c, s, t = math.cos(angle), math.sin(angle), 1-math.cos(angle)
    # row-vector Rodrigues
    return np.array([[t*x*x+c, t*x*y+s*z, t*x*z-s*y],
                     [t*x*y-s*z, t*y*y+c, t*y*z+s*x],
                     [t*x*z+s*y, t*y*z-s*x, t*z*z+c]])


def boundary_loops(points, faces):
    # Weld UV seams geometrically, retaining original winding.
    lookup, ids, representatives = {}, [], []
    for p in points:
        key = tuple(np.round(p, 5))
        if key not in lookup:
            lookup[key] = len(lookup)
            representatives.append(np.array(p))
        ids.append(lookup[key])
    edges, directed = Counter(), {}
    for tri in faces:
        a, b, c = [ids[i] for i in tri]
        if len({a, b, c}) != 3:
            continue
        for x, y in ((a,b), (b,c), (c,a)):
            k = tuple(sorted((x,y)))
            edges[k] += 1
            directed[k] = (x,y)
    if any(n > 2 for n in edges.values()):
        raise ValueError('non-manifold source')
    following = {}
    for k, n in edges.items():
        if n == 1:
            a,b = directed[k]
            if a in following:
                raise ValueError('branching boundary')
            following[a] = b
    loops = []
    while following:
        start = next(iter(following))
        loop, cur = [], start
        while cur not in loop:
            loop.append(cur)
            cur = following.pop(cur)
        if cur != start or len(loop) < 3:
            raise ValueError('open/degenerate boundary')
        loops.append(loop)
    return [[representatives[i].copy() for i in loop] for loop in loops]


def normals(points, faces):
    n = np.zeros_like(points)
    for a,b,c in faces:
        v = np.cross(points[b]-points[a], points[c]-points[a])
        for i in (a,b,c):
            n[i] += v
    lengths = np.linalg.norm(n, axis=1)
    n[lengths > 1e-10] /= lengths[lengths > 1e-10, None]
    n[lengths <= 1e-10] = [0, 1, 0]
    return n


def author(source, psk, side, units_per_metre):
    doc, source_p, source_f = read_glb(source)
    names, parents, local, bind, pnts, weights, wedges, pfaces = psk
    prefix = 'l_' if side == 'left' else 'r_'
    wrist = names.index(prefix+'arm_wrist')
    idx = names.index(prefix+'index_1')
    pinky = names.index(prefix+'pinky_1')
    middle = names.index(prefix+'middle_1')
    # All raw PSK positions/GLB use reflected Y relative to armbind matrices.
    raw_bind = bind.copy()
    reflect = np.diag([1., -1., 1., 1.])
    raw_bind = reflect @ bind @ reflect
    wrist_pos = raw_bind[wrist, 3, :3]
    forward = unit(raw_bind[middle, 3, :3]-wrist_pos)
    across = unit(raw_bind[idx, 3, :3]-raw_bind[pinky, 3, :3])
    palm = unit(np.cross(across, forward)) * (1 if side == 'left' else -1)
    right = unit(np.cross(forward, palm))
    up = unit(np.cross(right, forward))
    # Grip-local convention: +X right, +Y dorsal, -Z fingers. No view Euler angles.
    basis = np.stack([right, up, -forward], axis=1)
    # Author joint curls in raw bind space about the across-palm axis, with
    # descendants following their real parent. Angles are artist controls,
    # not purportedly measured game animation or UE offsets.
    posed = raw_bind.copy()
    angles = (72., 92., 65.)
    authored = {}
    for i, name in enumerate(names):
        if i == 0:
            continue
        raw_local = reflect @ local[i] @ reflect
        posed[i] = raw_local @ posed[parents[i]]
        digit = next((d for d in ('thumb','index','middle','ring','pinky')
                      if name.startswith(prefix+d+'_') and name[-1:] in ('1','2','3')), None)
        if not digit:
            continue
        segment = int(name[-1])-1
        if digit == 'thumb' and segment == 0:
            # Oppose the thumb across the palm, not around the four fingers'
            # flexion axis (which produces an unwanted thumbs-up silhouette).
            child = names.index(prefix+'thumb_2')
            child_local = reflect @ local[child] @ reflect
            current = unit(child_local[3,:3] @ posed[i,:3,:3])
            desired = unit(raw_bind[middle,3,:3]-raw_bind[i,3,:3])
            axis = np.cross(current,desired)
            angle = math.acos(float(np.clip(np.dot(current,desired),-1,1)))
            if np.linalg.norm(axis)>1e-8:
                posed[i,:3,:3] = posed[i,:3,:3] @ rotation(axis,angle)
            authored[name] = dict(opposition_degrees=math.degrees(angle))
            continue
        angle = (0., 12., 18.)[segment] if digit == 'thumb' else angles[segment]
        # Local hinge reconstructed from the across-palm reference, transformed
        # with the current parent (not a fixed world-space axis).
        axis_local = across @ np.linalg.inv(raw_bind[i, :3, :3])
        axis = axis_local @ posed[i, :3, :3]
        direction = -palm
        finger_dir = forward
        if np.dot(np.cross(axis, finger_dir), direction) < 0:
            angle = -angle
        posed[i, :3, :3] = posed[i, :3, :3] @ rotation(axis, math.radians(angle))
        authored[name] = angle
    skin = np.linalg.inv(raw_bind) @ posed
    # Rebuild per-corner UVs from exact source triangles. Match PSK vertices to
    # GLB positions before adding any geometry; fail on an unexpected source.
    source_set = {tuple(np.round(p, 4)) for p in source_p}
    dom = {i:max(w)[1] for i,w in weights.items()}
    output, uv, closed, faces, mapped, removed = [], [], [], [], {}, 0
    for a,b,c,mat,_,_ in pfaces:
        if mat != 0:
            continue
        ws = [wedges[x] for x in (a,b,c)]
        ids = [w[0] for w in ws]
        if not all(names[dom[i]].startswith(prefix) and
                   any(k in names[dom[i]] for k in ('_wrist','_thumb_','_index_','_middle_','_ring_','_pinky'))
                   for i in ids):
            continue
        xyz = pnts[ids]
        if not all(tuple(np.round(p, 4)) in source_set for p in xyz):
            raise ValueError('PSK/GLB source mismatch')
        if np.linalg.norm(np.cross(xyz[1]-xyz[0], xyz[2]-xyz[0])) < 1e-7:
            removed += 1
            continue
        face = []
        for point,u,v in ws:
            key = (point,u,v)
            if key not in mapped:
                mapped[key] = len(output)
                output.append(pnts[point])
                # PSK's native Direct3D V and glTF both address image top-left.
                # The older OBJ preview flipped V AND flipped it again while
                # sampling; carrying only its first flip into D3D inverts art.
                uv.append([u, v])
                wp = np.r_[pnts[point],1.]
                total = sum(w for w,_ in weights[point])
                closed.append(sum(w*(wp @ skin[b])[:3] for w,b in weights[point])/total)
            face.append(mapped[key])
        faces.append(face)
    points, target = np.array(output), np.array(closed)
    loops = boundary_loops(points, faces)
    cap_triangles = 0
    # Cap ONLY cuffs: all loop points must be close to the wrist and farther
    # from finger tips. Unexpected finger/asset tears fail rather than fill.
    for loop in loops:
        centre = np.mean(loop, axis=0)
        if np.dot(centre-wrist_pos, forward) > 3.5 or np.linalg.norm(centre-wrist_pos) > 10.:
            raise ValueError('unexpected non-cuff boundary')
        centre_id = len(output)
        output.append(centre); closed.append(centre); uv.append([.5,.5])
        ring_ids = []
        for p in loop:
            ring_ids.append(len(output)); output.append(p); closed.append(p); uv.append([.5,.5])
        for k,a in enumerate(ring_ids):
            faces.append([centre_id, ring_ids[(k+1)%len(ring_ids)], a])
            cap_triangles += 1
    points, target, faces = np.array(output), np.array(closed), np.array(faces)
    if boundary_loops(points, faces):
        raise ValueError('cap did not close mesh')
    points = (points-wrist_pos) @ basis / units_per_metre
    target = (target-wrist_pos) @ basis / units_per_metre
    n, nt = normals(points, faces), normals(target, faces)
    report = dict(source_sha256=sha(source), source_bytes=Path(source).stat().st_size,
                  source_vertices=len(source_p), source_triangles=len(source_f),
                  source_bounds_uu=[source_p.min(0).tolist(), source_p.max(0).tolist()],
                  source_nodes=len(doc['nodes']), source_skins=len(doc.get('skins', [])),
                  source_animations=len(doc.get('animations', [])),
                  source_attributes=list(doc['meshes'][0]['primitives'][0]['attributes']),
                  source_double_sided=doc['materials'][0].get('doubleSided',False),
                  psk_bones=len(names), authored_joints=authored, boundary_loops=len(loops),
                  boundary_lengths=[len(x) for x in loops], cap_triangles=cap_triangles,
                  removed_degenerate=removed, vertices=len(points), triangles=len(faces),
                  closed_boundary_edges=0, units_per_metre=units_per_metre,
                  bounds_metres=[points.min(0).tolist(), points.max(0).tolist()],
                  maximum_morph_metres=float(np.linalg.norm(target-points, axis=1).max()),
                  runtime_draws_per_visible_hand_per_eye=1, runtime_mesh_swaps=0)
    report['wrist_raw_uu'] = wrist_pos.tolist()
    report['raw_to_grip_basis'] = basis.tolist()
    report['cap_start_triangle'] = len(faces)-cap_triangles
    report['uv_origin'] = 'upper-left, raw PSK V (no OBJ flip)'
    return points, target, n, nt, np.array(uv), faces, report


def write_glb(path, p, target, n, nt, uv, f, texture=None):
    raw, views, accessors = bytearray(), [], []
    def add(a, kind, component=5126):
        while len(raw)%4:
            raw.append(0)
        data = np.asarray(a, dtype='<f4' if component==5126 else '<u4').tobytes()
        views.append(dict(buffer=0,byteOffset=len(raw),byteLength=len(data)))
        raw.extend(data)
        record = dict(bufferView=len(views)-1,componentType=component,count=len(a),type=kind)
        if kind == 'VEC3':
            record.update(min=np.min(a,axis=0).tolist(),max=np.max(a,axis=0).tolist())
        accessors.append(record)
        return len(accessors)-1
    pos, normal, tex = add(p,'VEC3'), add(n,'VEC3'), add(uv,'VEC2')
    dp, dn, indices = add(target-p,'VEC3'), add(nt-n,'VEC3'), add(f.reshape(-1),'SCALAR',5125)
    doc = dict(asset=dict(version='2.0',generator='SingularityVR offline glove morph'),
               scene=0,scenes=[dict(nodes=[0])],nodes=[dict(mesh=0)],
               meshes=[dict(weights=[0.],extras=dict(targetNames=['fist']),primitives=[dict(
                   attributes=dict(POSITION=pos,NORMAL=normal,TEXCOORD_0=tex),indices=indices,
                   material=0,mode=4,targets=[dict(POSITION=dp,NORMAL=dn)])])],
               materials=[dict(doubleSided=True,pbrMetallicRoughness=dict(
                   baseColorFactor=[.4,.4,.4,1],metallicFactor=0,roughnessFactor=.9))],
               accessors=accessors,bufferViews=views,buffers=[dict(byteLength=len(raw))])
    if texture:
        while len(raw)%4:
            raw.append(0)
        views.append(dict(buffer=0,byteOffset=len(raw),byteLength=len(texture)))
        raw.extend(texture)
        doc['buffers'][0]['byteLength'] = len(raw)
        doc['images'] = [dict(bufferView=len(views)-1,mimeType='image/png')]
        doc['textures'] = [dict(source=0)]
        doc['materials'][0]['pbrMetallicRoughness']['baseColorFactor'] = [1,1,1,1]
        doc['materials'][0]['pbrMetallicRoughness']['baseColorTexture'] = dict(index=0)
    j = json.dumps(doc,separators=(',',':')).encode()
    j += b' '*((-len(j))%4)
    raw += b'\0'*((-len(raw))%4)
    Path(path).write_bytes(struct.pack('<III',0x46546c67,2,28+len(j)+len(raw))+
                           struct.pack('<II',len(j),0x4e4f534a)+j+
                           struct.pack('<II',len(raw),0x004e4942)+raw)


def write_cache(path, p, target, n, nt, uv, f):
    # SVRG v1, 16-byte header, 14 little-endian floats/vertex, uint32 indices.
    rows = np.column_stack([p,target-p,n,nt-n,uv]).astype('<f4')
    Path(path).write_bytes(struct.pack('<4sIII',b'SVRG',1,len(p),f.size)+
                           rows.tobytes()+f.astype('<u4').tobytes())


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--gloves',type=Path,required=True)
    ap.add_argument('--psk',type=Path,required=True)
    ap.add_argument('--output',type=Path,required=True)
    ap.add_argument('--texture',type=Path,required=True,help='private inview_glove_d PNG')
    ap.add_argument('--units-per-metre',type=float,default=52.5,
                    help='explicit mesh scale, not headset calibration')
    args = ap.parse_args()
    if not math.isfinite(args.units_per_metre) or not 10 <= args.units_per_metre <= 1000:
        ap.error('invalid scale')
    # Never overwrite source assets. Steam operations are intentionally absent.
    out = args.output.resolve()
    if out == args.gloves.resolve() or args.gloves.resolve() in out.parents:
        ap.error('output must be outside source pack')
    out.mkdir(parents=True,exist_ok=True)
    psk = read_psk(args.psk)
    from PIL import Image
    import io
    image = Image.open(args.texture).convert('RGBA')
    if max(image.size)>2048:
        ap.error('texture maximum 2048x2048')
    png = io.BytesIO()
    image.save(png,format='PNG')
    (out/'glove.svrt').write_bytes(struct.pack('<4sIII',b'SVRT',1,*image.size)+image.tobytes('raw','BGRA'))
    report = dict(psk_sha256=sha(args.psk),texture_sha256=sha(args.texture),
                  runtime_texture_sha256=sha(out/'glove.svrt'),hands={})
    for side in ('left','right'):
        start = time.perf_counter()
        result = author(args.gloves/f'glove_{side}.glb',psk,side,args.units_per_metre)
        p,target,n,nt,uv,f,r = result
        write_glb(out/f'glove_{side}.glb',p,target,n,nt,uv,f,png.getvalue())
        write_cache(out/f'glove_{side}.svrg',p,target,n,nt,uv,f)
        r['output_glb_sha256'] = sha(out/f'glove_{side}.glb')
        r['output_cache_sha256'] = sha(out/f'glove_{side}.svrg')
        r['offline_prepare_ms'] = (time.perf_counter()-start)*1000
        report['hands'][side] = r
    (out/'report.json').write_text(json.dumps(report,indent=2)+'\n',encoding='utf-8')
    print(json.dumps(report,indent=2))


if __name__ == '__main__':
    main()
