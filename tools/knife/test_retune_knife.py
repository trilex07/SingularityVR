import struct
import unittest
from retune_knife_grip import resize


def fixture():
    vertices=[(.01,.002,-.25),(.005,.002,-.04),(.008,.002,-.08)]
    vertices+=vertices[:1]*(662-len(vertices))
    return struct.pack('<4sIII',b'SVRK',2,662,3030)+b''.join(struct.pack('<3fI2f',*p,0xffffffff,.2,.7) for p in vertices)+struct.pack('<3030H',*([0,1,2]*1010))


class BladeTests(unittest.TestCase):
    def test_hilt_guard_uv_and_topology_unchanged(self):
        old=fixture();new=resize(old)
        self.assertEqual(old[:16],new[:16]);self.assertEqual(old[16+662*24:],new[16+662*24:])
        self.assertEqual(old[16+24:16+72],new[16+24:16+72])
        for i in range(662):self.assertEqual(old[16+i*24+12:16+(i+1)*24],new[16+i*24+12:16+(i+1)*24])
        x,y,z=struct.unpack_from('<3f',new,16)
        self.assertAlmostEqual(x,.0108,places=7);self.assertAlmostEqual(y,.00216,places=7)
        self.assertAlmostEqual(z,-.2755,places=7)

    def test_noop_and_determinism(self):
        self.assertEqual(resize(fixture(),1,1),fixture())
        self.assertEqual(resize(fixture()),resize(fixture()))

    def test_bad_data_and_parameters_rejected(self):
        for data in (b'bad',fixture()[:-1]):
            with self.assertRaises((ValueError,struct.error)):resize(data)
        for a,b in ((0,1),(1,2),(float('nan'),1)):
            with self.assertRaises(ValueError):resize(fixture(),a,b)


if __name__=='__main__':unittest.main()
