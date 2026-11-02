"""Reconstruct the exact logged noTMD SVNC cuff in the final wrist frame.

Read-only: original private PSK + SVNC and bounded final-bound GPU receipts.
No game access, guessed offsets, or extracted geometry output.
"""
import argparse
import hashlib
import json
import re
import struct
import sys
from pathlib import Path
import numpy as np
sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'gloves'))
from prepare_gloves import read_psk


def analyze(log, psk, cuff):
    text = log.read_text(encoding='utf-8', errors='replace')
    records = []
    names, _, _, bind, points, weights, wedges, _ = read_psk(psk)
    assert names[9] == 'l_arm_wrist'
    header = Path(__file__).resolve().parents[2] / 'spikes/view_matrix/glove_isolate.h'
    mapping = re.search(r'kNoTmdCookedPalette\[\]\s*=\s*\{([^}]+)', header.read_text(encoding='utf-8'))
    bones = list(map(int, re.findall(r'\d+', mapping[1])))
    raw = cuff.read_bytes()
    assert struct.unpack_from('<4s3I', raw) == (b'SVNC', 1, 2, 0)
    profile = struct.unpack_from('<6I', raw, 16)
    assert profile[:3] == (1, 2924, 4830)
    indices = np.frombuffer(raw, '<u2', profile[4], 40)
    ids = np.unique(indices)
    ids = ids[[points[wedges[int(i)][0], 0] > 0 for i in ids]]
    for match in re.finditer(r'wrist-cuff final: frame=(\d+) eye=(\d+).*?posed=(\d+) moved=(\d+) unit=([^ ]+) offset=(-?\d+),(-?\d+),(-?\d+) scale=(\d+)', text):
        frame, eye, posed, moved, unit, ox, oy, oz, scale = match.groups()
        rows = {}
        pattern = rf'wrist-cuff palette: frame={frame} eye={eye} reg=(\d+) value=([^\r\n]+)'
        for reg, values in re.findall(pattern, text):
            rows[int(reg)] = list(map(float, values.split(',')))
        assert all(5+i in rows for i in range(len(bones)*3)), 'Incomplete actual palette receipt'
        skin = {}
        for slot, bone in enumerate(bones):
            matrix = np.eye(4)
            matrix[:, :3] = np.array([rows[5+slot*3+r] for r in range(3)]).T
            skin[bone] = matrix
        inverse_wrist = np.linalg.inv(bind[9] @ skin[9])
        final = []
        for index in ids:
            point = wedges[int(index)][0]
            native = np.r_[points[point] * [1, -1, 1], 1]
            influences = weights[point]
            assert all(b in skin for _, b in influences), 'Source influence missing from actual section'
            vertex = sum(w * (native @ skin[b]) for w, b in influences) / sum(w for w, _ in influences)
            final.append((vertex @ inverse_wrist)[:3])
        q = np.array(final)
        unit = float(unit)
        records.append(dict(frame=int(frame), eye=int(eye), posed=int(posed), moved=int(moved),
                            bound_wrist_min=q.min(axis=0).tolist(), bound_wrist_max=q.max(axis=0).tolist(),
                            plane_x_uu=int(ox)*.001*unit,
                            conservative_clearance_uu=int(ox)*.001*unit-float(q[:, 0].max())))
    assert records, 'No successful final-bound noTMD cuff receipts; do not infer a pose'
    return dict(schema='final-wrist-cuff-v1', records=records,
                hashes={p.name:hashlib.sha256(p.read_bytes()).hexdigest().upper() for p in (psk,cuff)},
                limitation='Source weights reconstructed through actual GPU skin constants. Projection/depth and physical appearance remain separate gates.')


if __name__ == '__main__':
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--log', type=Path, required=True)
    ap.add_argument('--psk', type=Path, required=True)
    ap.add_argument('--cuff', type=Path, required=True)
    args = ap.parse_args()
    print(json.dumps(analyze(args.log, args.psk, args.cuff), indent=2))
