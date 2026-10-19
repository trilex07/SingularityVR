"""Read-only original PSK oracle for the measured muzzle and magazine mounts."""
import argparse
import json
from pathlib import Path
import numpy as np
from fit_magazine_palm import fit, read_psk

SPECS = {
    'valkyrie': ('RvGame', 'machinegun', [0, -3.49137115, 36.9269905],
                .25, [[-1, 0, 0], [0, 0, -1], [0, -1, 0]], [5.05332961, -3.56168574, .62759385]),
    'volk': ('wp_SP_ARShotgun_SF', 'shotgun', [-.000018, -5.37408447, 61.2917137],
             .25, [[-1, 0, 0], [0, 0, -1], [0, -1, 0]], [5.99999568, -8.88449250, 3.80813382]),
    'kasimov': ('wp_SP_ARSniperRifle_SF', 'sniper', [-.000018, -3.84281158, 67.725647],
                .35, [[-1, 0, 0], [0, -1, 0], [0, 0, 1]], [5.02334944, -.50722513, 3.90860947]),
}


def audit(hands, originals):
    reports = {}
    for kind, (package, stem, muzzle, curl, rotation, translation) in SPECS.items():
        gun = originals / package / 'SkeletalMesh3' / f'ch_inview_{stem}_mesh.psk'
        psk = read_psk(gun)
        names, _, _, bind, *_ = psk
        root = names.index(f'cog_{stem}')
        actual = (bind[names.index('muzzle_flash')] @ np.linalg.inv(bind[root]))[3, :3]
        if not np.allclose(actual, muzzle, rtol=0, atol=1e-5):
            raise ValueError(f'{kind} muzzle source mismatch')
        result = fit(hands, gun)
        if result['curl'] != curl or not np.allclose(result['rotation'], rotation, rtol=0, atol=1e-7) or not np.allclose(
                result['translation_uu'], translation, rtol=0, atol=1e-6):
            raise ValueError(f'{kind} authored fit is not reproducible')
        reports[kind] = dict(muzzle_root_uu=actual.tolist(), magazine_fit=result)
    return reports


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--hands', type=Path, required=True)
    parser.add_argument('--originals', type=Path, required=True)
    args = parser.parse_args()
    print(json.dumps(audit(args.hands, args.originals), indent=2))
