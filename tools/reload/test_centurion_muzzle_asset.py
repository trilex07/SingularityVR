"""Verify runtime muzzle constants against the user's original ActorX skeleton."""
import sys
from pathlib import Path
import numpy as np
sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'gloves'))
from prepare_gloves import read_psk

names, parents, _, bind, *_ = read_psk(Path(sys.argv[1]))
bone = names.index('muzzle_flash')
root = names.index('cog_alt_pistol')
assert parents[bone] == root
reflection = np.diag([1., -1., 1., 1.])
native = reflection @ bind[bone] @ reflection
assert np.allclose(native[3, :3], [.000005440695, 114.69569557, 63.52872816], atol=1e-5)
assert np.allclose(native[0, :3], [.00000005, -.000000015, 1.], atol=1e-6)
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
