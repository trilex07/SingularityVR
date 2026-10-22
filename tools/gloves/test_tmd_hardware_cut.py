"""Offline proof that the committed TMD face runs match original hardware only.

Pass private PSK and decompressed RvGame package paths; never copy those into git.
"""
import argparse
from pathlib import Path
import re

import numpy as np
from prepare_gloves import read_psk


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--psk", required=True, type=Path)
    parser.add_argument("--cooked", required=True, type=Path)
    args = parser.parse_args()
    names, _, _, _, _, weights, wedges, faces = read_psk(args.psk)
    assert len(faces) == 12123
    assert [sum(f[3] == section for f in faces) for section in (0, 1, 2)] == [4402, 7704, 17]
    native = np.array([f[:3] for f in faces], dtype="<u2")[:, [1, 0, 2]].tobytes()
    assert args.cooked.read_bytes().count(native) == 1, "cooked/ActorX index order mismatch"

    hardware = {i for i, name in enumerate(names) if i == names.index("tmd_base") or
                name.startswith(("hinge", "rail", "disc", "nixie", "faceplate", "wheel", "nub"))}
    actual = {i for i, face in enumerate(faces[4402:12106]) if all(
        all(bone in hardware for _, bone in weights[wedges[corner][0]])
        for corner in face[:3])}
    header = (Path(__file__).resolve().parents[2] / "spikes/view_matrix/glove_isolate.h").read_text()
    match = re.search(r"kTmdHardwareFaces\[\]\s*=\s*\{(.*?)\};", header, re.S)
    assert match, "missing committed face table"
    runs = [(int(a), int(b)) for a, b in re.findall(r"\{(\d+),(\d+)\}", match.group(1))]
    committed = {i for first, last in runs for i in range(first, last + 1)}
    assert len(runs) == 49 and len(committed) == 6527
    assert committed == actual, "committed TMD face table differs from original hardware weights"
    print("PASS: full cooked index roundtrip; 6527 hardware faces; 1177 arm faces excluded")


if __name__ == "__main__":
    main()
