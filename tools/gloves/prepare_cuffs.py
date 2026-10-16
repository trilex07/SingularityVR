"""Prepare original full cuffs + closure indices for both native hand rigs.

Offline inputs only; output is private game-derived data, never commit it.
"""
import argparse
import hashlib
import json
from pathlib import Path
import struct
import numpy as np
from prepare_gloves import read_psk, sha, unit
from cuff_topology import surface_components, boundary_cycles, triangulate_cap


PROFILES=((1,'noTMD',2924,4830,16512),(2,'TMD',2626,4402,36369))


def prepare_profile(psk_path,cooked,profile):
    ident,label,vertex_limit,source_triangles,source_indices=profile
    psk=read_psk(psk_path)
    names,parents,local,bind,points,weights,wedges,faces=psk
    native=np.array([f[:3] for f in faces],dtype='<u2')[:,[1,0,2]]
    if native.size!=source_indices or len(wedges)!=(3314 if ident==1 else 7444):
        raise ValueError('source mesh identity changed')
    if cooked.count(native.tobytes())!=1:raise ValueError('full cooked index roundtrip is not unique')
    if any(f[3]!=0 for f in faces[:source_triangles]):raise ValueError('material section changed')
    wp=points[[w[0] for w in wedges]]
    reflect=np.diag([1.,-1.,1.,1.]);raw_bind=reflect@bind@reflect
    original=[];caps=[];report=[]
    for side,prefix in enumerate(('l_','r_')):
        sign=1 if side==0 else -1
        face_ids=np.array([i for i in range(source_triangles) if all(wp[j,0]*sign>0 for j in native[i])])
        candidates=native[face_ids]
        groups=surface_components(wp,candidates)
        # Select by the actual fingertips, not largest-is-hand or a guessed
        # wrist depth. The second right-side island is a native forearm strip.
        finger_points={j for j in range(len(wp)) if any(w>0 and any(d in names[b] for d in ('_index_','_middle_','_pinky','_ring_','_thumb_'))
                      for w,b in weights[wedges[j][0]]) and wp[j,0]*sign>0}
        hand_groups=[group for group in groups if finger_points.intersection(candidates[group].ravel())]
        if len(hand_groups)!=1:raise ValueError('ambiguous finger-connected surface')
        keep=face_ids[hand_groups[0]];tris=native[keep]
        wrist=raw_bind[names.index(prefix+'arm_wrist'),3,:3]
        axis=unit(wrist-raw_bind[names.index(prefix+'arm_elbow'),3,:3])
        loops=boundary_cycles(wp,tris);side_caps=[];loop_report=[]
        for loop in loops:
            depth=(wp[loop]-wrist)@axis
            if depth.max()>=0:raise ValueError('opening is not proximal to wrist')
            side_caps.extend(triangulate_cap(wp,loop,tris))
            loop_report.append(dict(vertices=len(loop),depth_uu=[float(depth.min()),float(depth.max())]))
        closed=np.concatenate((tris,np.array(side_caps,dtype=int).reshape(-1,3)))
        if boundary_cycles(wp,closed):raise ValueError('cap left an open edge')
        # Weld aliases must retain identical skin weights; position-only closure
        # could otherwise tear when a held gun animates the forearm/wrist.
        by_pos={}
        for j in np.unique(closed):
            skin=tuple(sorted((b,round(w,6)) for w,b in weights[wedges[int(j)][0]]))
            key=tuple(np.round(wp[j],5))
            if key in by_pos and by_pos[key]!=skin:raise ValueError('welded seam has different skin weights')
            by_pos[key]=skin
        original.extend(tris.tolist());caps.extend(side_caps)
        depth=(wp[np.unique(tris)]-wrist)@axis
        report.append(dict(side=prefix,source_faces=len(tris),cap_faces=len(side_caps),loops=loop_report,
            cuff_extent_uu=float(-depth.min()),source_face_ids=keep.tolist(),
            removed_components=[len(group) for group in groups if group!=hand_groups[0]]))
    indices=np.array(original+caps,dtype='<u2').ravel()
    if not len(caps) or indices.max()>=vertex_limit:raise ValueError('invalid original vertex range or absent caps')
    section=struct.pack('<6I',ident,vertex_limit,source_triangles,source_indices,len(indices),len(caps))+indices.tobytes()
    return section,dict(id=ident,mesh=label,psk_sha256=sha(psk_path),source_index_sha256=hashlib.sha256(native.tobytes()).hexdigest(),
        full_index_roundtrip=True,triangles=len(indices)//3,cap_triangles=len(caps),hands=report),psk,np.array(original+caps)


def prepare(no_tmd,tmd,cooked_path):
    cooked=Path(cooked_path).read_bytes();parts=[];reports=[];geometry=[]
    for path,profile in zip((no_tmd,tmd),PROFILES):
        section,report,psk,tris=prepare_profile(path,cooked,profile)
        parts.append(section);reports.append(report);geometry.append((psk,tris,report))
    data=struct.pack('<4s3I',b'SVNC',1,2,0)+b''.join(parts)
    return data,dict(schema='original-native-cuffs-v1',cooked_sha256=sha(cooked_path),
        cache_sha256=hashlib.sha256(data).hexdigest(),profiles=reports,
        exterior='original VB/UV/material/skin/scale unchanged; no replacement diffuse'),geometry


if __name__=='__main__':
    ap=argparse.ArgumentParser();ap.add_argument('--no-tmd',required=True,type=Path)
    ap.add_argument('--tmd',required=True,type=Path);ap.add_argument('--cooked',required=True,type=Path)
    ap.add_argument('--output',required=True,type=Path);args=ap.parse_args()
    data,report,_=prepare(args.no_tmd,args.tmd,args.cooked)
    args.output.mkdir(parents=True,exist_ok=True)
    (args.output/'native_cuffs.svnc').write_bytes(data)
    (args.output/'cuff-report.json').write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps({k:v for k,v in report.items() if k!='profiles'},indent=2))
    for profile in report['profiles']:
        print(profile['mesh'],profile['triangles'],'triangles,',profile['cap_triangles'],'cap faces')
