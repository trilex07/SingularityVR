"""Offline original UE3 draw selection: original indices, cuff cap, finger rig.

No GLB/PNG input and no Steam defaults. Output is private game-derived data.
The native renderer keeps the game's VB, UVs, material, shaders and lighting.
"""
import argparse
import hashlib
import json
import math
from pathlib import Path
import struct
import numpy as np
from prepare_gloves import read_psk, unit, rotation, boundary_loops, sha


def quaternion(m):
    # Row-vector rotation -> conventional column-vector quaternion.
    a = m.T
    vals, vecs = np.linalg.eigh(np.array([
        [a[0,0]-a[1,1]-a[2,2], a[0,1]+a[1,0], a[0,2]+a[2,0], a[2,1]-a[1,2]],
        [a[0,1]+a[1,0], a[1,1]-a[0,0]-a[2,2], a[1,2]+a[2,1], a[0,2]-a[2,0]],
        [a[0,2]+a[2,0], a[1,2]+a[2,1], a[2,2]-a[0,0]-a[1,1], a[1,0]-a[0,1]],
        [a[2,1]-a[1,2], a[0,2]-a[2,0], a[1,0]-a[0,1], np.trace(a)],
    ]) / 3.)
    q = vecs[:, np.argmax(vals)]
    return q if q[3] >= 0 else -q


def finger_deltas(psk):
    names, parents, local, bind, *_ = psk
    local = np.array(local)
    reflect = np.diag([1., -1., 1., 1.])
    raw_bind = reflect @ bind @ reflect
    result = np.tile([0.,0.,0.,1.], (90,1))
    for prefix in ('l_', 'r_'):
        wrist = names.index(prefix+'arm_wrist')
        middle = names.index(prefix+'middle_1')
        forward = unit(raw_bind[middle,3,:3]-raw_bind[wrist,3,:3])
        across = unit(raw_bind[names.index(prefix+'index_1'),3,:3]-
                      raw_bind[names.index(prefix+'pinky_1'),3,:3])
        palm = unit(np.cross(across,forward)) * (1 if prefix=='l_' else -1)
        posed = raw_bind.copy()
        for i,name in enumerate(names):
            if not i: continue
            raw_local = reflect @ local[i] @ reflect
            posed[i] = raw_local @ posed[parents[i]]
            digit = next((d for d in ('thumb','index','middle','ring','pinky')
                          if name.startswith(prefix+d+'_') and name[-1:] in ('1','2','3')), None)
            if digit:
                segment = int(name[-1])-1
                if digit=='thumb' and segment==0:
                    child = names.index(prefix+'thumb_2')
                    child_local = reflect @ local[child] @ reflect
                    current = unit(child_local[3,:3] @ posed[i,:3,:3])
                    desired = unit(raw_bind[middle,3,:3]-raw_bind[i,3,:3])
                    axis = np.cross(current,desired)
                    angle = math.acos(float(np.clip(current@desired,-1,1)))
                else:
                    angle = (0.,12.,18.)[segment] if digit=='thumb' else (72.,92.,65.)[segment]
                    axis = (across @ np.linalg.inv(raw_bind[i,:3,:3])) @ posed[i,:3,:3]
                    if np.dot(np.cross(axis,forward),-palm)<0: angle=-angle
                    angle=math.radians(angle)
                if np.linalg.norm(axis)>1e-8:
                    posed[i,:3,:3] = posed[i,:3,:3] @ rotation(axis,angle)
                # Local delta, in the engine's unreflected coordinate system.
                closed_local = posed[i] @ np.linalg.inv(posed[parents[i]])
                closed_local = reflect @ closed_local @ reflect
                delta = np.linalg.inv(local[i,:3,:3]) @ closed_local[:3,:3]
                result[i] = quaternion(delta)
    return result


def prepare(psk_path, cooked_path, cuff_uu=3.5):
    psk = read_psk(psk_path)
    names, parents, local, bind, points, weights, wedges, faces = psk
    if len(names)!=90 or names[9]!='l_arm_wrist' or names[51]!='r_arm_wrist':
        raise ValueError('not the measured original noTMD rig')
    cooked = Path(cooked_path).read_bytes()
    pattern = struct.pack('<ihhiHhhiH',2,0,0,0,4830,1,1,14490,674)
    at = cooked.find(pattern)
    if at<0 or cooked.find(pattern,at+1)>=0: raise ValueError('ambiguous cooked section')
    start=at+len(pattern)
    if struct.unpack_from('<ii',cooked,start)!=(2,16512): raise ValueError('index layout changed')
    native=np.frombuffer(cooked,'<u2',16512,start+8).reshape(-1,3)
    if not np.array_equal(native[:,[1,0,2]],np.array([f[:3] for f in faces])):
        raise ValueError('full cooked/ActorX index roundtrip failed')
    if not 1 <= cuff_uu <= 6: raise ValueError('unreviewed cuff extent')
    raw_bind=np.diag([1.,-1.,1.,1.]) @ bind @ np.diag([1.,-1.,1.,1.])
    wp=points[[w[0] for w in wedges]]
    original, caps, report = [], [], []
    for side,prefix in enumerate(('l_', 'r_')):
        wrist=raw_bind[names.index(prefix+'arm_wrist'),3,:3]
        axis=unit(wrist-raw_bind[names.index(prefix+'arm_elbow'),3,:3])
        depth=(wp-wrist)@axis
        sign=1 if side==0 else -1
        keep=[i for i,f in enumerate(faces[:4830]) if f[3]==0 and
              all(wp[j,0]*sign>0 for j in f[:3]) and min(depth[list(f[:3])])>=-cuff_uu]
        tris=native[keep]
        loops=boundary_loops(wp,tris)
        by_pos={tuple(np.round(wp[j],5)):int(j) for j in tris.ravel()}
        side_caps=[]
        for loop in loops:
            rim=np.array(loop)
            # Every opening must be a proximal cuff opening, never a finger tear.
            if ((rim-wrist)@axis).max()>=0: raise ValueError('opening is not behind wrist')
            ids=[by_pos[tuple(np.round(p,5))] for p in loop]
            # A topological fan uses only ORIGINAL skinned vertices. Interior cap
            # inherits their original UV/normals; no new invented exterior texture.
            for j in range(1,len(ids)-1): side_caps.append([ids[0],ids[j+1],ids[j]])
        closed=np.concatenate([tris,np.array(side_caps)],axis=0)
        if boundary_loops(wp,closed): raise ValueError('cuff cap is not closed')
        original.extend(tris.tolist());caps.extend(side_caps)
        report.append(dict(side=prefix, native_faces=len(keep),cap_faces=len(side_caps),
                           boundary_loops=len(loops), cuff_rim_uu=[
                               float(min(((np.array(l)-wrist)@axis).min() for l in loops)),
                               float(max(((np.array(l)-wrist)@axis).max() for l in loops))],
                           open_edges=0, source_face_ids=keep))
    indices=np.array(original+caps,dtype='<u2').ravel()
    if indices.max()>=2924: raise ValueError('selection escaped section 0 vertex range')
    # Every retained contributor must consume an existing, correctly sided delta.
    palette={85,51,70,71,72,79,80,76,73,74,82,83,77,84,81,78,75,50,
             9,10,11,12,16,27,23,24,20,17,18,14,15,21,25,22,19}
    used_points={wedges[int(j)][0] for j in indices}
    if any(b not in palette for i in used_points for w,b in weights[i] if w>0):
        raise ValueError('cuff requires an unmapped skin contributor')
    q=finger_deltas(psk).astype('<f4')
    header=struct.pack('<4s7I',b'SVNI',1,len(indices),2924,4830,16512,len(caps),90)
    data=header+indices.tobytes()+q.tobytes()
    receipt=dict(psk_sha256=sha(psk_path), cooked_sha256=sha(cooked_path),
                 full_index_roundtrip=True, cuff_uu=cuff_uu, hands=report,
                 indices=len(indices), triangles=len(indices)//3, caps=len(caps),
                 source_material='native game draw, not a replacement texture',
                 native_index_sha256=hashlib.sha256(native.tobytes()).hexdigest(),
                 cache_sha256=hashlib.sha256(data).hexdigest(),
                 native_vertex_buffer='unchanged; includes UV/tangents/weights',
                 fingerprint_scope='offline package identity; MAIN must verify runtime path')
    return data,receipt,wp,np.array(original+caps)


if __name__=='__main__':
    ap=argparse.ArgumentParser()
    ap.add_argument('--psk',required=True,type=Path)
    ap.add_argument('--cooked',required=True,type=Path)
    ap.add_argument('--output',required=True,type=Path)
    ap.add_argument('--cuff-uu',type=float,default=3.5)
    args=ap.parse_args()
    data,receipt,_,_=prepare(args.psk,args.cooked,args.cuff_uu)
    args.output.mkdir(parents=True,exist_ok=True)
    (args.output/'native_hands.svni').write_bytes(data)
    (args.output/'report.json').write_text(json.dumps(receipt,indent=2)+'\n')
    print(json.dumps({k:v for k,v in receipt.items() if k!='hands'},indent=2))
