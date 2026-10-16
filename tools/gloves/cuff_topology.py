"""Original wedge topology, including a measured pinched cuff seam.

No generated exterior vertices or UVs. Caps reuse native skinned wedges.
"""
from collections import defaultdict
import numpy as np


def welded(points):
    lookup={}
    return np.array([lookup.setdefault(tuple(np.round(p,5)),len(lookup)) for p in points])


def surface_components(points, tris):
    ids=welded(points);parents=list(range(len(tris)));owner={}
    def root(i):
        while parents[i]!=i:parents[i]=parents[parents[i]];i=parents[i]
        return i
    for i,tri in enumerate(tris):
        for vertex in ids[tri]:
            if vertex in owner:parents[root(i)]=root(owner[vertex])
            else:owner[vertex]=i
    groups={}
    for i in range(len(tris)):groups.setdefault(root(i),[]).append(i)
    return sorted(groups.values(),key=lambda group:(-len(group),group[0]))


def boundary_cycles(points, tris):
    """Directed cycles; a shared vertex is legal, branching edges are not.

    Original left cuff has two boundary cycles touching at one vertex. Old
    following[vertex] rejects it. Edge-disjoint cycles keep both holes visible.
    """
    ids=welded(points);edges=defaultdict(list);representatives={}
    for tri in tris:
        a,b,c=map(int,ids[tri])
        if len({a,b,c})!=3:raise ValueError('degenerate source face')
        for raw,v in zip(tri,(a,b,c)):representatives.setdefault(v,int(raw))
        for x,y in ((a,b),(b,c),(c,a)):edges[tuple(sorted((x,y)))].append((x,y))
    border=[]
    for records in edges.values():
        if len(records)>2:raise ValueError('non-manifold source edge')
        if len(records)==2 and records[0]!=tuple(reversed(records[1])):raise ValueError('inconsistent source winding')
        if len(records)==1:border.append(records[0])
    outgoing=defaultdict(list);incoming=defaultdict(int)
    for a,b in sorted(border):outgoing[a].append(b);incoming[b]+=1
    if any(incoming[v]!=len(outs) for v,outs in outgoing.items()):raise ValueError('open boundary chain')
    loops=[]
    # Starting at shared vertices first separates the measured figure-eight.
    while outgoing:
        start=min(outgoing,key=lambda v:(-len(outgoing[v]),v));walk=[start];cur=start
        while True:
            if cur not in outgoing:raise ValueError('broken boundary chain')
            nxt=outgoing[cur].pop(0)
            if not outgoing[cur]:del outgoing[cur]
            if nxt==start:break
            if nxt in walk:raise ValueError('unexpected self-touching boundary')
            walk.append(nxt);cur=nxt
        if len(walk)<3:raise ValueError('short boundary cycle')
        loops.append([representatives[v] for v in walk])
    return loops


def triangulate_cap(points, loop, source=()):
    # Reverse the native boundary, project its best-fit plane, ear-clip. A fan
    # would incorrectly cover concave cuffs and can fold through the opening.
    indices=list(reversed(loop));rim=points[indices]
    _,_,basis=np.linalg.svd(rim-rim.mean(0),full_matrices=False)
    plane=(rim-rim.mean(0))@basis[:2].T
    def cross(a,b):return float(a[0]*b[1]-a[1]*b[0])
    area=sum(cross(plane[i],plane[(i+1)%len(plane)]) for i in range(len(plane)))
    if abs(area)<1e-9:raise ValueError('degenerate cuff projection')
    sign=1 if area>0 else -1;remaining=list(range(len(indices)));triangles=[]
    weld=welded(points)
    def edge(a,b):return tuple(sorted((int(weld[a]),int(weld[b]))))
    boundary={edge(indices[i],indices[(i+1)%len(indices)]) for i in range(len(indices))}
    existing={edge(a,b) for tri in source for a,b in ((tri[0],tri[1]),(tri[1],tri[2]),(tri[2],tri[0]))}
    def allowed(a,b):return edge(indices[a],indices[b]) in boundary or edge(indices[a],indices[b]) not in existing
    while len(remaining)>3:
        for pos,b in enumerate(remaining):
            a=remaining[pos-1];c=remaining[(pos+1)%len(remaining)]
            if not allowed(a,c):continue
            if sign*cross(plane[b]-plane[a],plane[c]-plane[a])<=1e-9:continue
            inside=False
            for q in remaining:
                if q in (a,b,c):continue
                if all(sign*cross(plane[y]-plane[x],plane[q]-plane[x])>=-1e-9 for x,y in ((a,b),(b,c),(c,a))):inside=True;break
            if inside:continue
            triangles.append([indices[a],indices[b],indices[c]]);remaining.pop(pos);break
        else:raise ValueError('non-simple cuff projection')
    if not all(allowed(remaining[i],remaining[(i+1)%3]) for i in range(3)):raise ValueError('cap would reuse an interior source edge')
    triangles.append([indices[i] for i in remaining])
    if any(np.linalg.norm(np.cross(points[b]-points[a],points[c]-points[a]))<1e-9 for a,b,c in triangles):
        raise ValueError('degenerate cap triangle')
    return triangles
