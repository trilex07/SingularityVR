"""Build a private, original-material TMD short-gauntlet index asset.

No vertex positions, UVs, weights, material, or hardware animation are replaced.
The game-derived SVTG file belongs outside Git and is loaded only by opt-in.
"""
import argparse
import hashlib
import json
from pathlib import Path
import re
import struct

import numpy as np

from prepare_gloves import read_psk, sha
from cuff_topology import boundary_cycles, surface_components, triangulate_cap


def hardware_faces(header):
    text = Path(header).read_text()
    match = re.search(r"kTmdHardwareFaces\[\]\s*=\s*\{(.*?)\};", text, re.S)
    if not match:
        raise ValueError("missing audited TMD hardware list")
    runs = [(int(a), int(b)) for a, b in re.findall(r"\{(\d+),(\d+)\}", match.group(1))]
    faces = {i for a, b in runs for i in range(a, b + 1)}
    if len(runs) != 49 or len(faces) != 6527:
        raise ValueError("audited hardware selection changed")
    return faces


def generate(psk_path, cooked_path, header):
    names, _, _, bind, points, weights, wedges, faces = read_psk(psk_path)
    if len(faces) != 12123 or len(wedges) != 7444:
        raise ValueError("unexpected TMD mesh")
    if [sum(f[3] == i for f in faces) for i in range(3)] != [4402, 7704, 17]:
        raise ValueError("TMD material sections changed")
    native = np.asarray([f[:3] for f in faces], dtype="<u2")[:, [1, 0, 2]]
    if Path(cooked_path).read_bytes().count(native.tobytes()) != 1:
        raise ValueError("cooked/ActorX index order mismatch")
    hw = hardware_faces(header)
    hw_bones = {i for i, name in enumerate(names) if name == "tmd_base" or
                name.startswith(("hinge", "rail", "disc", "nixie", "faceplate", "wheel", "nub"))}
    actual_hw = {i for i, face in enumerate(faces[4402:12106]) if all(
        all(bone in hw_bones for _, bone in weights[wedges[corner][0]])
        for corner in face[:3])}
    if actual_hw != hw:
        raise ValueError("hardware skin ownership changed")

    wp = points[[w[0] for w in wedges]]
    reflect = np.diag([1., -1., 1., 1.])
    pose = reflect @ bind @ reflect
    wrist = pose[names.index("l_arm_wrist"), 3, :3]
    elbow = pose[names.index("l_arm_elbow"), 3, :3]
    axis = wrist - elbow
    axis /= np.linalg.norm(axis)
    arm = np.array([i + 4402 for i in range(7704) if i not in hw])
    left = arm[np.all(wp[native[arm]][:, :, 0] > 0, axis=1)]
    groups = surface_components(wp, native[left])
    measured = []
    for group in groups:
        ids = left[np.asarray(group)]
        depth = (wp[native[ids]] - wrist) @ axis
        measured.append((ids, float(depth.min()), float(depth.max())))
    # Group 0 is the long ~43 UU forearm. Groups 1..6 are the original short
    # cuff, ring and attachment islands. These are source-derived, not a new
    # generic glove. Refuse on changed topology instead of selecting by guess.
    if [len(g[0]) for g in measured] != [280, 261, 176, 64, 47, 6, 6]:
        raise ValueError("TMD arm islands changed")
    if not (-44 < measured[0][1] < -41 and -20 < measured[2][1] < -17):
        raise ValueError("TMD sleeve measurements changed")
    short_ids = np.concatenate([g[0] for g in measured[1:]])
    sleeve = native[short_ids]
    primary = native[measured[2][0]]
    loops = boundary_cycles(wp, primary)
    if sorted(len(loop) for loop in loops) != [25, 25]:
        raise ValueError("proximal cuff ring changed")
    proximal = min(loops, key=lambda loop: float(((wp[loop] - wrist) @ axis).mean()))
    cap = np.asarray(triangulate_cap(wp, proximal, primary), dtype="<u2")
    if len(cap) != 23 or len(boundary_cycles(wp, np.vstack((primary, cap)))) != 1:
        raise ValueError("short cuff cap failed")
    selected = np.concatenate((np.array(sorted(hw)) + 4402, short_ids))
    combined = np.vstack((native[selected], cap)).astype("<u2")
    if len(np.unique(selected)) != len(selected) or int(combined.max()) >= 7444:
        raise ValueError("duplicate/invalid TMD indices")
    payload = combined.tobytes()
    data = struct.pack("<4s5I", b"SVTG", 1, 7444, len(combined), len(cap), 36369) + payload
    report = dict(schema="original-tmd-short-gauntlet-v1", psk_sha256=sha(psk_path),
                  cooked_sha256=sha(cooked_path), source_index_sha256=hashlib.sha256(native.tobytes()).hexdigest(),
                  asset_sha256=hashlib.sha256(data).hexdigest(), hardware_triangles=6527,
                  short_sleeve_triangles=len(sleeve), cap_triangles=len(cap),
                  long_forearm_triangles_excluded=len(measured[0][0]),
                  short_extent_uu=[min(g[1] for g in measured[1:]), max(g[2] for g in measured[1:])],
                  material="original TMD material 1; original VB/UV/weights/palette")
    return data, report, (wp, combined, native, short_ids, cap, wrist, axis)


def render_previews(geometry, output):
    """Geometry-only previews; they do not claim native texture/lighting proof."""
    import matplotlib
    matplotlib.use("Agg")
    import matplotlib.pyplot as plt
    from mpl_toolkits.mplot3d.art3d import Poly3DCollection

    wp, combined, native, _, _, wrist, _ = geometry
    # Include the original left glove section in the offline composition.
    glove_groups = surface_components(wp, native[:4402])
    glove = native[np.asarray(glove_groups[1])]
    if len(glove) != 1786:
        raise ValueError("original TMD left glove changed")
    triangles = np.vstack((glove, combined))
    colors = (["#1a1c22"] * len(glove) + ["#5f6060"] * 6527 +
              ["#292c30"] * (len(combined) - 6527))
    xyz = wp[triangles] - wrist
    low = xyz.min(axis=(0, 1)); high = xyz.max(axis=(0, 1))
    center = (low + high) / 2
    half = float(max(high - low)) * .62
    # Three orthographic views around the tracked wrist, cropped to the
    # gauntlet rather than the entire original full-arm skeleton.
    for name, elev, azim in (("outer", 16, -54), ("inner", 18, 128), ("axial", 76, -54)):
        fig = plt.figure(figsize=(7, 7), dpi=135)
        ax = fig.add_subplot(111, projection="3d", proj_type="ortho")
        ax.add_collection3d(Poly3DCollection(xyz, facecolors=colors, edgecolors="none"))
        ax.set(xlim=(center[0]-half, center[0]+half),
               ylim=(center[1]-half, center[1]+half),
               zlim=(center[2]-half, center[2]+half))
        ax.set_box_aspect((1, 1, 1))
        ax.view_init(elev=elev, azim=azim)
        ax.set_axis_off()
        fig.savefig(output / f"tmd-short-gauntlet-{name}.png", bbox_inches="tight", pad_inches=0.02)
        plt.close(fig)


if __name__ == "__main__":
    ap = argparse.ArgumentParser()
    ap.add_argument("--tmd", required=True, type=Path)
    ap.add_argument("--cooked", required=True, type=Path)
    ap.add_argument("--output", required=True, type=Path, help="private path outside repository")
    ap.add_argument("--preview", action="store_true")
    args = ap.parse_args()
    repo = Path(__file__).resolve().parents[2]
    if repo == args.output.resolve() or repo in args.output.resolve().parents:
        raise SystemExit("game-derived output must be outside repository")
    data, report, geometry = generate(args.tmd, args.cooked, repo / "spikes/view_matrix/glove_isolate.h")
    args.output.mkdir(parents=True, exist_ok=True)
    (args.output / "tmd_short_gauntlet.svtg").write_bytes(data)
    (args.output / "tmd_short_gauntlet_report.json").write_text(json.dumps(report, indent=2) + "\n")
    if args.preview:
        render_previews(geometry, args.output)
    print(json.dumps(report, indent=2))
