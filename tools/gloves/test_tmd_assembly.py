"""Private original topology/attachment check; no headset claim."""
import argparse
from pathlib import Path
import numpy as np
from prepare_cuffs import prepare_profile, PROFILES
from prepare_tmd_short_gauntlet import generate
from cuff_topology import boundary_cycles

ap=argparse.ArgumentParser()
ap.add_argument('--tmd',type=Path,required=True)
ap.add_argument('--cooked',type=Path,required=True)
a=ap.parse_args()
_,report,psk,closed=prepare_profile(a.tmd,a.cooked.read_bytes(),PROFILES[1])
names,parents,local,bind,points,weights,wedges,faces=psk
wp=points[[w[0] for w in wedges]]
left=closed[np.all(wp[closed,0]>0,axis=1)]
assert len(left)==1786 and not boundary_cycles(wp,left)
header=Path(__file__).resolve().parents[2]/'spikes/view_matrix/glove_isolate.h'
_,_,(_,svtg,*_)=generate(a.tmd,a.cooked,header)
glove=wp[np.unique(left)]
sleeve=wp[np.unique(svtg[6527:])]
dist=((glove[:,None,:]-sleeve[None,:,:])**2).sum(axis=2)
i,j=np.unravel_index(dist.argmin(),dist.shape)
original=glove[i]-sleeve[j]
for degrees in (-70,55):
    r=np.deg2rad(degrees)
    rotation=np.array([[np.cos(r),np.sin(r),0],[-np.sin(r),np.cos(r),0],[0,0,1]])
    translation=np.array([3.,7.,-4.])
    shown_glove=glove[i]@rotation+translation
    shown_sleeve=sleeve[j]@rotation+translation
    np.testing.assert_allclose(shown_glove-shown_sleeve,original@rotation,atol=1e-6)
    displaced=shown_glove+np.array([12.,0,0])
    assert np.isclose(np.linalg.norm((displaced-shown_sleeve)-(original@rotation)),12.)
print(f'PASS: original left SVNC is closed (1786 faces); nearest sleeve distance={np.sqrt(dist[i,j]):.6f} UU; glove-only shift violates attachment by 12 UU at both rotations')
