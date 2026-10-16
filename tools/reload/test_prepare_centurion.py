import unittest
import struct
import numpy as np
from prepare_centurion import extract, diffuse_cache
from PIL import Image

def fixture():
    names=['r_arm_wrist','clip']
    bind=np.repeat(np.eye(4)[None],2,axis=0)
    points=np.array([[0,0,0],[1,0,0],[0,1,0]],dtype=float)
    return (names,[0,0],bind,bind,points,{i:[(1.,1)] for i in range(3)},
            [(i,float(i==1),float(i==2)) for i in range(3)],[(0,1,2,0,0,0)])

class Tests(unittest.TestCase):
    def test_diffuse_channels_alpha_and_determinism(self):
        image=Image.new('RGBA',(2,1));image.putdata([(61,47,31,0),(1,2,3,127)])
        data=diffuse_cache(image);self.assertEqual(data,diffuse_cache(image))
        self.assertEqual(struct.unpack_from('<4sIII',data),(b'SVRT',1,2,1))
        self.assertEqual(data[16:],bytes([31,47,61,0,3,2,1,127]))
    def test_diffuse_bounds(self):
        for size in ((0,1),(1,0),(2049,1)):
            with self.assertRaises(ValueError):diffuse_cache(Image.new('RGBA',size))
    def test_roundtrip_uv_and_determinism(self):
        data,report=extract(fixture());self.assertEqual((data,report),extract(fixture()))
        self.assertEqual(struct.unpack_from('<4sIII',data),(b'SVRK',2,3,3))
        rows=[struct.unpack_from('<3fI2f',data,16+24*i) for i in range(3)]
        self.assertEqual(rows[1][-2:],(1.,0.));self.assertEqual(rows[2][-2:],(0.,1.))
        self.assertAlmostEqual(rows[1][0],1/52.5)
    def test_reflected_translated_bind(self):
        p=list(fixture());p[3][1,3,:3]=[.2,-.3,.4]
        _,r=extract(p);self.assertEqual(r['crossing_triangles'],0)
    def test_mixed_boundary_refused(self):
        p=list(fixture());p[5][0]=[(.5,0),(.5,1)]
        with self.assertRaisesRegex(ValueError,'mixed'):extract(p)
    def test_bad_inputs(self):
        for units in (0,float('nan'),10001):
            with self.assertRaises(ValueError):extract(fixture(),units)
        p=list(fixture());p[7]=[(0,1,99,0,0,0)]
        with self.assertRaises(ValueError):extract(p)
        p=list(fixture());p[6][0]=(0,float('nan'),0)
        with self.assertRaises(ValueError):extract(p)

if __name__=='__main__':unittest.main()
