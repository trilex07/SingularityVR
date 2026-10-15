"""Deterministic geometry/format regressions; optional private asset assertions."""
import argparse
import json
from pathlib import Path
import struct
import tempfile
import unittest
import numpy as np
from prepare_gloves import boundary_loops, read_glb, write_glb, write_cache, normals, rotation
from preview_gloves import cache, ray_distance

PREPARED=None
TEMP_ROOT=Path(__file__).resolve().parents[2]/'build'/'gloves'/'tests'
TEMP_ROOT.mkdir(parents=True,exist_ok=True)


class Geometry(unittest.TestCase):
    def test_boundary_and_cap(self):
        # Open tetrahedron, then its actual missing face (not double-sided).
        p=np.array([[0.,0,0],[1,0,0],[0,1,0],[0,0,1]])
        f=np.array([[0,2,1],[0,1,3],[1,2,3]])
        self.assertEqual(len(boundary_loops(p,f)),1)
        closed=np.concatenate([f,[[2,0,3]]])
        self.assertEqual(boundary_loops(p,closed),[])
        # Rays from the missing face see the newly added face first.
        o=np.array([-.5,.25,.25]);d=np.array([1.,0,0])
        self.assertGreater(ray_distance(o,d,p,f),ray_distance(o,d,p,closed))

    def test_uv_seams_weld_for_topology_only(self):
        p=np.array([[0.,0,0],[1,0,0],[0,1,0],[0,0,1]])
        f=np.array([[0,2,1],[0,1,3],[1,2,3],[2,0,3]])
        expanded=p[f].reshape(-1,3)
        self.assertEqual(boundary_loops(expanded,np.arange(12).reshape(-1,3)),[])

    def test_nonmanifold_rejected(self):
        p=np.array([[0.,0,0],[1,0,0],[0,1,0],[0,0,1],[0,0,-1]])
        with self.assertRaises(ValueError):
            boundary_loops(p,[[0,1,2],[1,0,3],[0,1,4]])

    def test_rodrigues_keeps_lengths_and_sign(self):
        r=rotation(np.array([0.,0,1]),np.pi/2)
        np.testing.assert_allclose(np.array([1.,0,0])@r,[0,1,0],atol=1e-12)
        np.testing.assert_allclose(r@r.T,np.eye(3),atol=1e-12)

    def test_glb_cache_round_trip(self):
        p=np.array([[0.,0,0],[.1,0,0],[0,.1,0]])
        target=p+[0,0,.02];f=np.array([[0,1,2]]);n=normals(p,f)
        with tempfile.TemporaryDirectory(dir=TEMP_ROOT) as tmp:
            tmp=Path(tmp)
            write_glb(tmp/'test.glb',p,target,n,n,np.zeros((3,2)),f)
            doc,rp,rf=read_glb(tmp/'test.glb')
            np.testing.assert_allclose(rp,p,atol=1e-7)
            np.testing.assert_array_equal(rf,f)
            self.assertEqual(len(doc['meshes'][0]['primitives'][0]['targets']),1)
            write_cache(tmp/'test.svrg',p,target,n,n,np.zeros((3,2)),f)
            v,cf=cache(tmp/'test.svrg')
            np.testing.assert_allclose(v[:,:3]+v[:,3:6],target,atol=1e-7)
            np.testing.assert_array_equal(cf,f)

    def test_asset_fail_closed(self):
        with tempfile.TemporaryDirectory(dir=TEMP_ROOT) as tmp:
            p=Path(tmp)/'bad.glb';p.write_bytes(b'not a glb')
            with self.assertRaises(ValueError):
                read_glb(p)

    def test_private_assets(self):
        if PREPARED is None:
            self.skipTest('private assets not supplied')
        report=json.loads((PREPARED/'report.json').read_text())
        rays=json.loads((PREPARED/'axial-rays.json').read_text())
        for side in ('left','right'):
            with self.subTest(side=side):
                r=report['hands'][side]
                self.assertEqual((r['source_nodes'],r['source_skins'],r['source_animations']),(1,0,0))
                self.assertEqual(r['boundary_loops'],1)
                self.assertTrue(rays[side]['cap_is_first'])
                self.assertTrue(rays[side]['source_sees_past_cuff'])
                v,f=cache(PREPARED/f'glove_{side}.svrg')
                p=v[:,:3].astype(float);t=p+v[:,3:6]
                self.assertEqual(boundary_loops(p,f),[])
                self.assertEqual(boundary_loops(t,f),[])
                self.assertGreater(np.linalg.norm(t-p,axis=1).max(),.04)
                self.assertLess(np.linalg.norm(t-p,axis=1).max(),.4)
                # Cap is motionless and stays welded at BOTH endpoints.
                cap=f[r['cap_start_triangle']:]
                self.assertLess(np.abs(v[np.unique(cap),3:6]).max(),1e-6)
                doc,gp,gf=read_glb(PREPARED/f'glove_{side}.glb')
                self.assertEqual(len(doc['meshes'][0]['primitives']),1)
                self.assertEqual(len(doc['meshes'][0]['primitives'][0]['targets']),1)
                self.assertEqual(len(doc['images']),1)
                np.testing.assert_allclose(p,gp,atol=1e-7)
                np.testing.assert_array_equal(f,gf)
                # Caps have no zero-area or reversed-winding assumption.
                area=np.linalg.norm(np.cross(p[f[:,1]]-p[f[:,0]],p[f[:,2]]-p[f[:,0]]),axis=1)
                self.assertGreater(float(area.min()),1e-12)


if __name__=='__main__':
    parser=argparse.ArgumentParser()
    parser.add_argument('--prepared',type=Path)
    args,remaining=parser.parse_known_args()
    PREPARED=args.prepared
    unittest.main(argv=[__file__]+remaining)
