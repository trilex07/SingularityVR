import unittest
import numpy as np
from fit_hand_grip import intersections,distance,inside


class GripGeometry(unittest.TestCase):
    def test_surface_crossing(self):
        a=np.array([[[0.,0,0],[2,0,0],[0,2,0]]])
        b=np.array([[[.3,.3,-1],[.3,.3,1],[1,.3,0]]])
        self.assertGreater(intersections(a,b),0)
        self.assertGreater(intersections(b,a),0)
        self.assertEqual(intersections(a,b+[0,0,3]),0)

    def test_distance_face_edge_vertex(self):
        t=np.array([[[0.,0,0],[2,0,0],[0,2,0]]])
        self.assertAlmostEqual(distance(np.array([[.2,.2,.3]]),t),.3)
        self.assertAlmostEqual(distance(np.array([[3.,0,0]]),t),1)
        self.assertAlmostEqual(distance(np.array([[-1.,1,0]]),t),1)

    def test_buried_point_not_only_surface_crossing(self):
        p=np.array([[0.,0,0],[1,0,0],[0,1,0],[0,0,1]])
        f=np.array([[0,2,1],[0,1,3],[1,2,3],[2,0,3]])
        np.testing.assert_array_equal(inside(np.array([[.1,.1,.1],[2,2,2],[-.1,.1,.1]]),p[f]),[True,False,False])


if __name__=='__main__':unittest.main()
