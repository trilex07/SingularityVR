import unittest
import numpy as np
from prepare_knife_lighting import frames


class TangentTests(unittest.TestCase):
    def test_native_uv_and_rigid_frame(self):
        v=np.array([[0.,0.,0.],[1.,0.,0.],[0.,1.,0.]])
        uv=np.array([[0.,0.],[1.,0.],[0.,1.]])
        x=frames(v,np.array([[0,1,2]]),uv)
        np.testing.assert_array_equal(x[:,:3],v)
        np.testing.assert_array_equal(x[:,10:],uv)
        np.testing.assert_allclose(np.sum(x[:,3:6]*x[:,6:9],axis=1),0,atol=1e-6)
        np.testing.assert_allclose(np.linalg.norm(x[:,3:6],axis=1),1)
        np.testing.assert_allclose(np.linalg.norm(x[:,6:9],axis=1),1)

    def test_degenerate_uv_is_finite(self):
        v=np.array([[0.,0.,0.],[1.,0.,0.],[0.,1.,0.]])
        x=frames(v,np.array([[0,1,2]]),np.zeros((3,2)))
        self.assertTrue(np.isfinite(x).all())

    def test_bad_geometry_rejected(self):
        with self.assertRaises(ValueError):frames(np.zeros((3,3)),np.array([[0,1,2]]),np.zeros((3,2)))


if __name__=='__main__':unittest.main()
