"""Offline original glove/cylinder fitting; private outputs, ZERO Steam."""
import argparse
import hashlib
import json
from pathlib import Path
import sys
import numpy as np
sys.path.insert(0, str(Path(__file__).resolve().parents[1]/'knife'))
from fit_hand_grip import read_psk, posed, intersections, inside, distance, finger_deltas


def fit(hands_path, gun_path):
    hands, gun = read_psk(hands_path), read_psk(gun_path)
    names, parents, _, bind, _, weights, wedges, faces = hands
    wrist = names.index('l_arm_wrist')
    descendants = {wrist}
    for b, p in enumerate(parents):
        if b != p and p in descendants:
            descendants.add(b)
    hf = np.array([f[:3] for f in faces if all(
        all(b in descendants for _, b in weights[wedges[i][0]]) for i in f[:3])])
    gn, _, _, gb, gp, gw, gwed, gf = gun
    clip = gn.index('clip')
    selected = [i for i in range(len(gp)) if all(b == clip for _, b in gw[i])]
    points = (np.c_[gp*[1, -1, 1], np.ones(len(gp))] @ np.linalg.inv(gb[clip]))[:, :3]
    cf = np.array([[gwed[i][0] for i in f[:3]] for f in gf
                   if all(gwed[i][0] in selected for i in f[:3])])
    inv = np.linalg.inv(bind[wrist])
    candidates = []
    for curl in (.25, .35, .45, .55):
        hand = (np.c_[posed(hands, 0, [curl]*5)*[1, -1, 1], np.ones(len(wedges))] @ inv)[:, :3]
        hv = hand[np.unique(hf)]
        for x in np.arange(-6., 6.01, .5):
            for y in np.arange(-9., -3.99, .5):
                centre = np.array([x, y, 0.])
                delta = hv-centre
                radial = np.linalg.norm(delta[:, :2], axis=1)
                penetration = (radial < 2.55) & (np.abs(delta[:, 2]) < 3.2)
                surface = np.maximum(radial-2.64, np.abs(delta[:, 2])-3.3)
                cost = penetration.sum()*100 + np.sort(np.abs(surface))[:20].mean()*5 + (y+6.5)**2*.02
                candidates.append((float(cost), curl, centre, hand))
    for cost, curl, centre, hand in sorted(candidates, key=lambda c:c[0])[:64]:
        cylinder = points+centre
        hits = intersections(hand[hf], cylinder[cf])
        if hits == 0 and not inside(cylinder[selected][::12], hand[hf]).any():
            contacts={}
            for digit in ('thumb','index','middle','ring','pinky'):
                ids=[i for i,w in enumerate(wedges) if any(names[b].startswith('l_'+digit+'_') for _,b in weights[w[0]])]
                contacts[digit]=distance(hand[ids],cylinder[cf])
            return dict(curl=curl, wrist_local_centre=centre.tolist(),
                        digit_surface_distances_uu=contacts,
                        triangle_intersections=hits, sampled_cylinder_points_buried=0,
                        hand_triangles=len(hf), cylinder_triangles=len(cf),
                        native_clip_bounds=[points[selected].min(0).tolist(), points[selected].max(0).tolist()],
                        hands_sha256=hashlib.sha256(hands_path.read_bytes()).hexdigest(),
                        gun_sha256=hashlib.sha256(gun_path.read_bytes()).hexdigest(),
                        note='Exact original meshes/curl, artist fit; not actual game pixels/PSVR2'), (hand, hf, cylinder, cf)
    raise ValueError('No collision-free original fit in bounded search; do not ship guessed mount')


if __name__ == '__main__':
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--hands', type=Path, required=True);p.add_argument('--gun', type=Path, required=True)
    p.add_argument('--output', type=Path, required=True)
    p.add_argument('--native-cache', type=Path);a = p.parse_args()
    allowed=(Path(__file__).resolve().parents[2]/'build/centurion-reload').resolve()
    out=a.output.resolve()
    if not out.is_relative_to(allowed) or out==allowed or out.exists():
        p.error('output must be NEW own ignored lab directory')
    report, geometry=fit(a.hands,a.gun)
    if a.native_cache:
        import struct
        cache=a.native_cache.read_bytes()
        if len(cache)<32 or cache[:4]!=b'SVNI':
            raise ValueError('not original native hand cache')
        indices=struct.unpack_from('<I',cache,8)[0];offset=32+indices*2
        if len(cache)!=offset+90*16:
            raise ValueError('unreviewed native hand cache size')
        curve=np.frombuffer(cache,'<f4',360,offset).reshape(90,4)
        if not np.allclose(curve,finger_deltas(read_psk(a.hands)),rtol=0,atol=2e-6):
            raise ValueError('actual native curl does not match fit; do not ship')
        report.update(native_curve_roundtrip=True,native_cache_sha256=hashlib.sha256(cache).hexdigest())
    out.mkdir(parents=True)
    np.savez(out/'private-fit.npz', hand=geometry[0], hand_faces=geometry[1], cylinder=geometry[2], cylinder_faces=geometry[3])
    import matplotlib
    matplotlib.use('Agg')
    import matplotlib.pyplot as plt
    from matplotlib.collections import PolyCollection
    fig, axes=plt.subplots(1,3,figsize=(12,4))
    for ax, xy in zip(axes, ((0,1),(0,2),(2,1))):
        for verts, faces, colour in ((geometry[0],geometry[1],'#717c86'),(geometry[2],geometry[3],'#cd9b48')):
            ax.add_collection(PolyCollection(verts[faces][:,:,xy],facecolors=colour,edgecolors='#15191d',linewidths=.15,alpha=.85))
        ax.set_xlim(-10,14);ax.set_ylim(-19,9);ax.set_aspect('equal');ax.set_title(str(xy))
    fig.tight_layout();fig.savefig(out/'preview.png');plt.close(fig)
    (out/'report.json').write_text(json.dumps(report,indent=2)+'\n', encoding='utf8')
    print(json.dumps(report,indent=2))
