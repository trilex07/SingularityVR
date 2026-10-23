"""Private-source L1 verification for the index-only short TMD gauntlet."""
import argparse
from pathlib import Path
import struct
import hashlib

import numpy as np

from prepare_gloves import read_psk, rotation
from prepare_tmd_short_gauntlet import generate, hardware_faces


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--tmd", type=Path, required=True)
    ap.add_argument("--cooked", type=Path, required=True)
    ap.add_argument("--asset", type=Path, required=True)
    args = ap.parse_args()
    header = Path(__file__).resolve().parents[2] / "spikes/view_matrix/glove_isolate.h"
    data, report, (wp, tris, native, short_ids, cap, wrist, axis) = generate(args.tmd, args.cooked, header)
    assert args.asset.read_bytes() == data
    assert hashlib.sha256(data).hexdigest() == "452ea0c0ddfb4721a7f9b9718629975cf021f9e079e1cd92235c076e9e376192"
    assert struct.unpack_from("<4s5I", data) == (b"SVTG", 1, 7444, 7110, 23, 36369)
    assert len(tris) == 7110 and len(short_ids) == 560 and len(cap) == 23
    names, _, _, _, _, weights, wedges, faces = read_psk(args.tmd)
    assert len(faces[12106:]) == 17
    assert {names[bone] for face in faces[12106:] for corner in face[:3]
            for _, bone in weights[wedges[corner][0]]} == {"r_arm_elbow", "r_arm_forearm"}
    original_hardware = native[np.array(sorted(hardware_faces(header))) + 4402]
    assert np.array_equal(tris[:6527], original_hardware)
    assert np.array_equal(tris[6527:7087], native[short_ids])
    assert int(tris.min()) == 2743 and int(tris.max()) == 7322
    # The original material-1 draw is absolute indices, not 0-based rebased
    # vertices. Prove every replacement index is inside its declared subrange.
    assert int(native[4402:12106].min()) >= 2626
    assert int(native[4402:12106].max()) < 2626 + 4818 == len(wedges)
    assert np.all((tris >= 2626) & (tris < 2626 + 4818))
    depth = (wp[np.unique(tris[6527:])] - wrist) @ axis
    assert depth.min() > -20 and depth.max() < 4  # no long forearm
    # Native TMD hardware remains bit-for-bit original under several wrist
    # rotations; GPU skinning supplies those rotations at runtime.
    source = wp[tris[:6527]].reshape(-1, 3) - wrist
    for degrees in (-90, -45, 0, 45, 90):
        rotated = source @ rotation(axis, np.deg2rad(degrees))
        assert np.allclose(np.linalg.norm(rotated, axis=1), np.linalg.norm(source, axis=1), atol=1e-4)
    assert report["hardware_triangles"] == 6527
    print("PASS: cooked identity, original hardware/UV index order, capped short sleeve, five wrist rotations")


if __name__ == "__main__":
    main()
