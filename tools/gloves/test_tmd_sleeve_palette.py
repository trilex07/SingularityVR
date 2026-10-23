"""Private PSK/SVTG proof that all mixed-weight sleeve faces share one rigid delta."""
import argparse
from pathlib import Path

import numpy as np

from prepare_gloves import read_psk
from prepare_tmd_short_gauntlet import generate


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--tmd", type=Path, required=True)
    ap.add_argument("--cooked", type=Path, required=True)
    args = ap.parse_args()
    header = Path(__file__).resolve().parents[2] / "spikes/view_matrix/glove_isolate.h"
    _, _, (_, triangles, _, _, _, _, _) = generate(args.tmd, args.cooked, header)
    names, _, _, _, points, weights, wedges, _ = read_psk(args.tmd)
    hardware = {i for i, name in enumerate(names) if name == "tmd_base" or
                name.startswith(("hinge", "rail", "disc", "nixie", "faceplate", "wheel", "nub"))}
    sleeve_names = {"l_arm_elbow", "l_arm_forearm2", *(f"sleeve_fix_{i}" for i in range(1, 6))}
    sleeve = {i for i, name in enumerate(names) if name in sleeve_names}
    assert len(sleeve) == 7
    mixed = []
    for face in triangles[6527:]:
        influences = {bone for corner in face for _, bone in weights[wedges[corner][0]]}
        assert influences <= hardware | sleeve, {names[i] for i in influences - hardware - sleeve}
        if influences & hardware and influences & sleeve:
            mixed.append(face)
    assert len(mixed) == 414, len(mixed)
    # Row-vector skinning: weighted sum(P @ skin_bone @ delta) must equal
    # weighted sum(P @ skin_bone) @ delta, for every mixed-face corner.
    for degrees in (-70, 55):
        r = np.deg2rad(degrees)
        delta = np.eye(4)
        delta[:2, :2] = [[np.cos(r), np.sin(r)], [-np.sin(r), np.cos(r)]]
        delta[3, :3] = [13, -5, 2]
        for face in mixed:
            for corner in face:
                point = wedges[corner][0]
                p = np.append(points[point], 1.)
                before = np.zeros(4)
                after = np.zeros(4)
                for weight, bone in weights[point]:
                    native = np.eye(4)
                    native[3, :3] = [bone*.17, bone*-.11, bone*.07]
                    before += weight * (p @ native)
                    after += weight * (p @ native @ delta)
                assert np.allclose(after, before @ delta, atol=1e-4)
    print("PASS: 414/583 mixed SVTG sleeve/cap triangles; both wrist rotations share one rigid delta")


if __name__ == "__main__":
    main()
