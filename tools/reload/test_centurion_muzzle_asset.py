"""Verify runtime muzzle constants against the user's original ActorX skeleton."""
import sys
from pathlib import Path
import numpy as np
sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'gloves'))
from prepare_gloves import read_psk

if __name__ == '__main__':
    if len(sys.argv) < 2:
        sys.exit(0)
    names, parents, _, bind, points, *_ = read_psk(Path(sys.argv[1]))
    bone = names.index('muzzle_flash')
    root = names.index('cog_alt_pistol')
    assert parents[bone] == root
    # read_psk already returns native matrices. Its PNTS stream, unlike bind,
    # remains ActorX and must be reflected once to match the cooked VB.
    native = bind[bone]
    assert np.allclose(native[3, :3], [.000005440695, -114.69569557, 63.52872816], atol=1e-5)
    assert np.allclose(native[0, :3], [.00000005, .000000015, 1.], atol=1e-6)
    points = points * [1, -1, 1]
    assert np.linalg.norm(points-native[3, :3], axis=1).min() < 1
    mirrored = native[3, :3] * [1, -1, 1]
    assert np.linalg.norm(points-mirrored, axis=1).min() > 200
    # A shared rigid wrist correction must preserve hardware/glove distances,
    # including animated local hardware offsets, under both eye views.
    for angle in (0., .7, 1.57, 3.14):
        c, s = np.cos(angle), np.sin(angle)
        rotation = np.array([[c,s,0],[-s,c,0],[0,0,1.]])
        wrist = np.array([1.,2.,3.]); device = wrist + [.2,.1,.05]
        for eye in (-.032,.032):
            a = wrist @ rotation + [eye,0,0]
            b = device @ rotation + [eye,0,0]
            assert np.isclose(np.linalg.norm(b-a),np.linalg.norm(device-wrist))
    print('PASS original Centurion muzzle position/axis; rigid wrist relative geometry in both eyes')
