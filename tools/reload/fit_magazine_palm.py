"""Fit original rigid magazines to the original curled left glove, offline only.

No rescaling, Steam writes, or exported game geometry in Git. A bounded search
uses exact triangle intersections, followed by a containment check. Its result
is an authored mount in native wrist UU, not physical headset acceptance.
"""
import argparse
import hashlib
import itertools
import json
from pathlib import Path
import sys
import numpy as np

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'knife'))
from fit_hand_grip import read_psk, posed, intersections, inside, distance


def fit(hands_path, gun_path, include_descendants=False):
    hands, gun = read_psk(hands_path), read_psk(gun_path)
    names, parents, _, bind, _, weights, wedges, faces = hands
    wrist = names.index('l_arm_wrist')
    descendants = {wrist}
    for i, parent in enumerate(parents):
        if i and parent in descendants:
            descendants.add(i)
    hf = np.array([f[:3] for f in faces if all(
        all(b in descendants for _, b in weights[wedges[i][0]]) for i in f[:3])])
    gn, gparents, _, gb, gp, gw, gwed, gf = gun
    clip = gn.index('clip')
    clip_bones={clip}
    if include_descendants:
        for i,parent in enumerate(gparents):
            if i and parent in clip_bones:clip_bones.add(i)
    selected = {i for i in range(len(gp)) if all(b in clip_bones for _, b in gw[i])}
    cf = np.array([[gwed[i][0] for i in f[:3]] for f in gf
                   if all(gwed[i][0] in selected for i in f[:3])])
    # Both meshes enter the SAME native UE3 axes, undoing UEViewer Y once.
    points = (np.c_[gp * [1, -1, 1], np.ones(len(gp))] @ np.linalg.inv(gb[clip]))[:, :3]
    ids = np.array(sorted(selected))
    centre = (points[ids].min(0) + points[ids].max(0)) * .5
    candidates = []
    for curl in (.25, .35, .45, .55):
        hand = (np.c_[posed(hands, 0, [curl] * 5) * [1, -1, 1], np.ones(len(wedges))]
                @ np.linalg.inv(bind[wrist]))[:, :3]
        hv = hand[np.unique(hf)]
        for permutation in itertools.permutations(range(3)):
            for signs in itertools.product((-1, 1), repeat=3):
                rotation = np.eye(3)[list(permutation)] * np.array(signs)[:, None]
                if np.linalg.det(rotation) < .9:
                    continue
                # Prefer long magazine axis down the palm's Z, not across fingers.
                size = np.ptp(points[ids] @ rotation, axis=0)
                if size[2] < max(size[0], size[1]):
                    continue
                rotated = (points - centre) @ rotation
                lo, hi = rotated[ids].min(0), rotated[ids].max(0)
                for x in np.arange(-5., 8.01, 1.):
                    for y in np.arange(-11., -3.99, 1.):
                        for z in (-3., 0., 3.):
                            target = np.array([x, y, z])
                            delta = hv - target
                            outside = np.maximum(lo - delta, delta - hi)
                            penetration = np.all(outside < -.12, axis=1).sum()
                            gap = np.maximum(outside, 0)
                            nearest = np.sort(np.linalg.norm(gap, axis=1))[:24].mean()
                            score = penetration * 50 + nearest * 8 + .02 * ((x-3)**2+(y+7)**2+z*z)
                            candidates.append((float(score), curl, rotation, target, hand))
    for score, curl, rotation, target, hand in sorted(candidates, key=lambda c: c[0])[:256]:
        translation = target - centre @ rotation
        magazine = points @ rotation + translation
        hits = intersections(hand[hf], magazine[cf])
        if hits or inside(magazine[ids][::5], hand[hf]).any() or inside(hand[np.unique(hf)][::5], magazine[cf]).any():
            continue
        proximity = float(distance(hand[np.unique(hf)], magazine[cf]))
        if proximity > 1.5:
            continue
        return dict(curl=curl, rotation=rotation.tolist(), translation_uu=translation.tolist(),
                    triangle_intersections=hits, sampled_containment=False, nearest_surface_uu=proximity,
                    glove_triangles=len(hf), magazine_triangles=len(cf), score=score,
                    hands_sha256=hashlib.sha256(hands_path.read_bytes()).hexdigest(),
                    gun_sha256=hashlib.sha256(gun_path.read_bytes()).hexdigest(),
                    status='OFFLINE_GEOMETRY_ONLY_NOT_HEADSET_ACCEPTANCE')
    raise ValueError('No collision-free connected grip in bounded search')


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--hands', type=Path, required=True)
    parser.add_argument('--gun', type=Path, required=True)
    parser.add_argument('--include-descendants', action='store_true')
    args = parser.parse_args()
    print(json.dumps(fit(args.hands, args.gun, args.include_descendants), indent=2))
