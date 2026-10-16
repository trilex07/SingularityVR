"""Read-only offline cuff topology census, never a Steam path default."""
import argparse
import json
import numpy as np
from prepare_gloves import read_psk, boundary_loops, unit


def components(points, tris):
    ids={};weld=[]
    for p in points:
        key=tuple(np.round(p,5));weld.append(ids.setdefault(key,len(ids)))
    parents=list(range(len(tris)));owner={}
    def root(i):
        while parents[i]!=i:parents[i]=parents[parents[i]];i=parents[i]
        return i
    for i,tri in enumerate(tris):
        for vertex in tri:
            v=weld[vertex]
            if v in owner:parents[root(i)]=root(owner[v])
            else:owner[v]=i
    groups={}
    for i in range(len(tris)):groups.setdefault(root(i),[]).append(i)
    return sorted(groups.values(),key=len,reverse=True)


def audit(psk_path):
    names, parents, local, bind, points, weights, wedges, faces = read_psk(psk_path)
    wp=points[[w[0] for w in wedges]]
    refl=np.diag([1.,-1.,1.,1.]); raw=refl@bind@refl
    result=[]
    for side,prefix in enumerate(('l_','r_')):
        wrist=raw[9 if side==0 else 51,3,:3]
        axis=unit(wrist-raw[8 if side==0 else 50,3,:3]);depth=(wp-wrist)@axis
        sign=1 if side==0 else -1
        native=np.array([f[:3] for f in faces[:4830] if f[3]==0 and all(wp[j,0]*sign>0 for j in f[:3])])
        rows=[]
        groups=[]
        for group in components(wp,native):
            selected=native[group];row=dict(triangles=len(group),range=[float(depth[np.unique(selected)].min()),float(depth[np.unique(selected)].max())])
            try:row['loops']=[len(l) for l in boundary_loops(wp,selected)]
            except ValueError as error:row['rejected']=str(error)
            groups.append(row)
        for cut in (3.5,6,8,10,12,15,18,20,25,30,40):
            tris=native[np.min(depth[native],axis=1)>=-cut]
            row=dict(depth_uu=cut,triangles=len(tris))
            try:
                loops=boundary_loops(wp,tris)
                row['loops']=[dict(vertices=len(loop),depth=[float(((np.array(loop)-wrist)@axis).min()),
                    float(((np.array(loop)-wrist)@axis).max())]) for loop in loops]
            except ValueError as error:row['rejected']=str(error)
            bones=sorted({b for j in tris.ravel() for w,b in weights[wedges[int(j)][0]] if w>0})
            row['bones']={b:names[b] for b in bones};rows.append(row)
        result.append(dict(side=prefix,range=[float(depth[np.unique(native)].min()),float(depth[np.unique(native)].max())],components=groups,cuts=rows))
    return result


if __name__=='__main__':
    ap=argparse.ArgumentParser();ap.add_argument('--psk',required=True);args=ap.parse_args()
    print(json.dumps(audit(args.psk),indent=2))
