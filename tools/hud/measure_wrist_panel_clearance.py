"""Measure private original cuff/SVTG vertices in native wrist-local space.

Only hashes, bounds and counts are emitted, never proprietary mesh data.
This is a bind-surface measurement, not animated/headset placement proof.
"""
import argparse
import hashlib
import json
from pathlib import Path
import struct
import sys

import numpy as np
sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'gloves'))
from prepare_gloves import read_psk


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--psk-dir', type=Path, required=True)
    ap.add_argument('--assets', type=Path, required=True)
    ap.add_argument('--units-per-metre', type=float, default=65.625)
    ap.add_argument('--output', type=Path, required=True)
    args = ap.parse_args()
    paths = [args.psk_dir / 'ch_inview_noTMD.psk', args.psk_dir / 'ch_inview_tmd_mesh.psk',
             args.assets / 'native_cuffs.svnc', args.assets / 'tmd_short_gauntlet.svtg']
    raw = paths[2].read_bytes()
    assert struct.unpack_from('<4s3I', raw) == (b'SVNC', 1, 2, 0)
    offset = 16
    rows = []
    for profile, path in enumerate(paths[:2]):
        header = struct.unpack_from('<6I', raw, offset)
        assert header[0] == profile + 1
        offset += 24
        indices = np.frombuffer(raw, '<u2', header[4], offset)
        offset += header[4] * 2
        names, _, _, bind, points, _, wedges, _ = read_psk(path)
        assert names[9] == 'l_arm_wrist'
        # Match native wrist's row-vector convention exactly once.
        native = np.c_[points[:, 0], -points[:, 1], points[:, 2], np.ones(len(points))]
        wrist = native @ np.linalg.inv(bind[9])
        wp = wrist[[w[0] for w in wedges], :3]
        source = points[[w[0] for w in wedges]]
        selections = [('SVNC', indices)]
        if profile:
            short = paths[3].read_bytes()
            assert struct.unpack_from('<4s5I', short) == (b'SVTG', 1, 7444, 7110, 23, 36369)
            selections.append(('SVTG', np.frombuffer(short, '<u2', offset=24)))
        for route, selected in selections:
            ids = np.unique(selected)
            assert ids.max() < len(wedges)
            ids = ids[source[ids, 0] > 0]  # actual left half, not vertex-count identity
            q = wp[ids]
            unit = args.units_per_metre
            q = q[(q[:, 1] >= -.1575 * unit) & (q[:, 1] <= -.0025 * unit) &
                  (np.abs(q[:, 2]) <= .052 * unit)]
            assert len(q)
            rows.append(dict(profile=profile, route=route, vertices=len(q),
                             min_wrist_uu=q.min(axis=0).tolist(), max_wrist_uu=q.max(axis=0).tolist()))
    front = min(row['min_wrist_uu'][0] for row in rows)
    # One-millimetre surface separation is a documented panel thickness;
    # no hand/controller transform or source geometry is changed.
    normal_mm = int(np.floor(front / args.units_per_metre * 1000 - 1))
    result = dict(schema='wrist-panel-source-clearance-v1', units_per_metre=args.units_per_metre,
                  footprint_mm=[155, 104], centre_from_wrist_mm=80, panel_thickness_mm=1,
                  source_hashes={p.name: hashlib.sha256(p.read_bytes()).hexdigest().upper() for p in paths},
                  bounds=rows, candidate_offset_x_mm=normal_mm,
                  limitation='Original bind surface only; animated final-palette/headset acceptance remains pending')
    args.output.write_text(json.dumps(result, indent=2) + '\n', encoding='utf-8')
    print(json.dumps(result, indent=2))


if __name__ == '__main__':
    main()
