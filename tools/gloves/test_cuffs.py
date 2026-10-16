"""Deterministic topology/cap tests, optional original-source validation."""
import argparse
from pathlib import Path
import unittest
import numpy as np
from PIL import Image
from cuff_topology import boundary_cycles, triangulate_cap, surface_components, welded
from prepare_cuffs import prepare
from prepare_native_hands import prepare as old_prepare
from preview_gloves import ray_distance, panel
from prepare_gloves import unit

INPUTS=None


def view(points,tris,basis,label,extent):
    used=np.unique(tris);remap=np.full(len(points),-1,dtype=int);remap[used]=np.arange(len(used))
    return panel(points[used],remap[tris],basis,label,extent=extent)


class Cuffs(unittest.TestCase):
    def test_concave_cap(self):
        points=np.array([[0,0,0],[3,0,0],[3,3,0],[1.5,1,0],[0,3,0]],float)
        cap=triangulate_cap(points,list(range(5)))
        self.assertEqual(len(cap),3)
        self.assertAlmostEqual(sum(np.linalg.norm(np.cross(points[b]-points[a],points[c]-points[a]))*.5 for a,b,c in cap),6)

    def test_reject_bad_source(self):
        p=np.array([[0,0,0],[1,0,0],[0,1,0],[0,0,1]],float)
        with self.assertRaises(ValueError):boundary_cycles(p,[[0,1,2],[0,1,3]])
        with self.assertRaises(ValueError):boundary_cycles(p,[[0,0,1]])
        with self.assertRaises(ValueError):triangulate_cap(np.zeros((3,3)),[0,1,2])

    def test_disconnected_arm(self):
        p=np.arange(18).reshape(6,3).astype(float)
        self.assertEqual([len(c) for c in surface_components(p,np.array([[0,1,2],[2,1,0],[3,4,5]]))],[2,1])

    def test_original_sources(self):
        if INPUTS is None:self.skipTest('private source inputs not supplied')
        no_tmd,tmd,cooked,out=INPUTS
        data,receipt,geometry=prepare(no_tmd,tmd,cooked)
        self.assertEqual(prepare(no_tmd,tmd,cooked)[0],data)
        old_data,old_receipt,old_wp,old_tris=old_prepare(no_tmd,cooked)
        views=[]
        for profile_index,(psk,tris,report) in enumerate(geometry):
            names,parents,local,bind,points,weights,wedges,faces=psk
            wp=points[[w[0] for w in wedges]];reflect=np.diag([1.,-1.,1.,1.]);raw=reflect@bind@reflect
            native_count=report['triangles']-report['cap_triangles']
            caps=tris[native_count:]
            for side,prefix in enumerate(('l_','r_')):
                sign=1 if side==0 else -1;side_faces=tris[np.all(wp[tris,0]*sign>0,axis=1)]
                self.assertEqual(boundary_cycles(wp,side_faces),[])
                wrist=raw[names.index(prefix+'arm_wrist'),3,:3]
                axis=unit(wrist-raw[names.index(prefix+'arm_elbow'),3,:3])
                side_caps=caps[np.all(wp[caps,0]*sign>0,axis=1)]
                if profile_index==0:
                    self.assertGreater(report['hands'][side]['cuff_extent_uu'],10.7)
                    old_ids=set(old_receipt['hands'][side]['source_face_ids'])
                    missing=old_ids-set(report['hands'][side]['source_face_ids'])
                    # Old right crop included a disconnected forearm strip;
                    # deliberately drop it, but never a finger-weighted face.
                    for face in missing:
                        self.assertFalse(any(any(d in names[b] for d in ('_thumb_','_index_','_middle_','_ring_','_pinky'))
                            for j in faces[face][:3] for w,b in weights[wedges[j][0]] if w>0))
                # Distinct matrices for each original skin contributor simulate
                # a changing native weapon pose. No new pose implementation.
                for frame in range(21):
                    pose=wp.copy();t=frame/20
                    for j,(point,_,_) in enumerate(wedges):
                        bone_weights=weights[point];total=sum(w for w,_ in bone_weights)
                        pose[j]=sum(w*(wp[j]+np.array([np.sin(b)*t,.2*np.cos(b)*t,.1*b*t])) for w,b in bone_weights)/total
                    # Stable ORIGINAL weld identity, not a freshly rounded
                    # dynamic pose (tiny authored seam triangles cross bins).
                    aliases={};identity=welded(wp)
                    for j in np.unique(side_faces):
                        key=identity[j]
                        if key in aliases:np.testing.assert_allclose(pose[j],aliases[key],atol=2e-5,rtol=0)
                        else:aliases[key]=pose[j]
                if len(side_caps):
                    centre=wp[np.unique(side_caps)].mean(0);origin=centre-axis*8
                    hit=ray_distance(origin,axis,wp,side_faces)
                    cap_hit=ray_distance(origin,axis,wp,side_caps)
                    self.assertTrue(np.isfinite(cap_hit));self.assertAlmostEqual(hit,cap_hit,places=5)
                    self.assertGreater(ray_distance(origin,axis,wp,tris[:native_count]),hit+.01)
                basis=np.array([unit(np.cross(axis,[0,0,1])),unit(np.cross(axis,unit(np.cross(axis,[0,0,1])))),-axis])
                if profile_index==0:
                    old_side=old_tris[np.all(old_wp[old_tris,0]*sign>0,axis=1)]
                    views.append(view((old_wp-wrist)/52.5*.75,old_side,basis,f'{prefix} old short cuff / axial',extent=.3))
                views.append(view((wp-wrist)/52.5*.75,side_faces,basis,f'{report["mesh"]} {prefix} full cuff closed / axial',extent=.3))
                side_view=np.array([basis[0],axis,basis[1]])
                views.append(view((wp-wrist)/52.5*.75,side_faces,side_view,f'{report["mesh"]} {prefix} original full cuff / side',extent=.45))
        if out:
            out.mkdir(parents=True,exist_ok=True);atlas=Image.new('RGB',(440*5,440*2))
            for i,preview in enumerate(views):atlas.paste(preview,((i%5)*440,(i//5)*440))
            atlas.save(out/'cuffs-geometry.png')


if __name__=='__main__':
    ap=argparse.ArgumentParser();ap.add_argument('--no-tmd',type=Path);ap.add_argument('--tmd',type=Path)
    ap.add_argument('--cooked',type=Path);ap.add_argument('--output',type=Path);args,rest=ap.parse_known_args()
    if any((args.no_tmd,args.tmd,args.cooked)):
        if not all((args.no_tmd,args.tmd,args.cooked)):ap.error('supply all three offline sources')
        INPUTS=(args.no_tmd,args.tmd,args.cooked,args.output)
    unittest.main(argv=[__file__]+rest)
