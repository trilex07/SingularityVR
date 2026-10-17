import unittest
from audit_native_particle_transforms import half_remap, undo_half, row_mul, max_error


class CoordinateOwnership(unittest.TestCase):
    def test_half_remap_is_after_projection_not_before(self):
        # Representative translated particle position and a perspective camera
        # looking along +Y. This is synthetic, not extracted game geometry.
        local = [[1.,0,0,0], [0,1.,0,0], [0,0,1.,0], [-12000.,8000.,-100.,1.]]
        camera = [[1.2,0,0,0], [0,0,1.,1.], [0,1.1,0,0], [0,0,-1.,0]]
        vertex = [12003.,-7970.,102.,1.]
        translated = row_mul(vertex, local)
        clip = row_mul(translated, camera)
        for shift in (-0.5,0.5):
            corrected = row_mul(translated, half_remap(camera, shift))
            expected_x = clip[0] * 0.5 + shift * clip[3]
            self.assertAlmostEqual(corrected[0], expected_x)
            self.assertEqual(corrected[1:], clip[1:])
            wrong = row_mul(row_mul(vertex, half_remap(local, shift)), camera)
            self.assertGreater(abs(wrong[0]/wrong[3] - corrected[0]/corrected[3]), 0.4)
            self.assertLess(max_error(undo_half(half_remap(local, shift), shift), local), 1e-7)

    def test_correct_stereo_does_not_rewrite_local(self):
        local = [[1.,0,0,0], [0,1.,0,0], [0,0,1.,0], [-15.,27.,-2.,1.]]
        original = [r[:] for r in local]
        camera = [[1.,0,0,0], [0,0,1.,1.], [0,1.,0,0], [0,0,-1.,0]]
        for shift in (-0.5,0.5):
            row_mul(row_mul([16.,3.,5.,1.], local), half_remap(camera, shift))
        self.assertEqual(local, original)


if __name__ == '__main__':
    unittest.main()
