import struct
import unittest
from prepare_knife import bake, parse


def chunk(name, size, rows):
    return struct.pack('<20siii', name.encode(), 0, size, len(rows))+b''.join(rows)


def fixture():
    return (chunk('PNTS0000',12,[struct.pack('<3f',*p) for p in [(0,1,0),(0,-10,0),(1,-5,1)]])+
            chunk('VTXW0000',16,[struct.pack('<IffBBH',i,0,0,0,0,0) for i in range(3)])+
            chunk('FACE0000',12,[struct.pack('<3HBBI',0,1,2,0,0,0)]))


class KnifeTests(unittest.TestCase):
    def test_deterministic(self):
        a,r=bake(fixture());b,s=bake(fixture())
        self.assertEqual(a,b);self.assertEqual(r,s)
        self.assertEqual(struct.unpack_from('<4sIII',a),(b'SVRK',1,3,3))
        self.assertAlmostEqual(r['length_metres'],.30)

    def test_axis(self):
        a,_=bake(fixture())
        v=[struct.unpack_from('<3fI',a,16+i*16)[:3] for i in range(3)]
        self.assertAlmostEqual(max(p[2] for p in v)-min(p[2] for p in v),.30,places=6)
        self.assertLess(v[1][2],0)

    def test_bounds(self):
        for b in (fixture()[:-1],b'bad',fixture()+fixture()):
            with self.assertRaises(ValueError):parse(b)

    def test_index(self):
        b=bytearray(fixture());struct.pack_into('<H',b,len(b)-12,999)
        with self.assertRaises(ValueError):bake(b)

    def test_length(self):
        for length in (0,1,float('nan')):
            with self.assertRaises(ValueError):bake(fixture(),length)


if __name__=='__main__':unittest.main()
