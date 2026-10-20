"""Read-only native lens oracle; no exported geometry or Steam mutations."""
import argparse
import hashlib
import json
from pathlib import Path
import sys
import numpy as np

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'knife'))
from fit_hand_grip import read_psk


def audit(source):
    names, _, _, bind, points, weights, wedges, faces = read_psk(source)
    root = names.index('cog_sniper')
    selected = [face for face in faces if face[3] == 1]
    ids = np.unique([wedges[i][0] for face in selected for i in face[:3]])
    if len(ids) != 97 or len(selected) != 168:
        raise ValueError('not the audited original sniper lens')
    if any(any(bone != root or abs(weight - 1) > 1e-6 for weight, bone in weights[i]) for i in ids):
        raise ValueError('lens is not rigid on cog_sniper')
    native = (np.c_[points[ids] * [1, -1, 1], np.ones(len(ids))] @ np.linalg.inv(bind[root]))[:, :3]
    centre = native.mean(0)
    expected = [-.00000279783, -11.1806386, -3.49776761]
    if not np.allclose(centre, expected, atol=1e-6, rtol=0):
        raise ValueError(f'lens centre changed: {centre}')
    muzzle = (bind[names.index('muzzle_flash')] @ np.linalg.inv(bind[root]))[3, :3]
    if muzzle[2] < centre[2] or np.ptp(native, axis=0)[2] > .2:
        raise ValueError('optical forward/axial thickness differs from measured source')
    return dict(source_sha256=hashlib.sha256(source.read_bytes()).hexdigest(),
                lens_points=len(ids), lens_triangles=len(selected),
                centre_native_cog_uu=centre.tolist(), extent_uu=np.ptp(native, axis=0).tolist(),
                status='OFFLINE_GEOMETRY_ONLY_NOT_HEADSET_ACCEPTANCE')


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('source', type=Path)
    print(json.dumps(audit(parser.parse_args().source), indent=2))
