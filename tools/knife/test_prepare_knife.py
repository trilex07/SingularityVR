import struct
import unittest
import tempfile
from pathlib import Path
from PIL import Image
from prepare_knife import bake, parse, texture


def chunk(name, size, rows):
    return struct.pack('<20siii', name.encode(), 0, size, len(rows))+b''.join(rows)


def fixture():
    return (chunk('PNTS0000',12,[struct.pack('<3f',*p) for p in [(0,1,0),(0,-10,0),(1,-5,1)]])+
            chunk('VTXW0000',16,[struct.pack('<IffBBH',i,u,v,0,0,0) for i,(u,v) in enumerate(((0,0),(1,0),(.25,.75)))])+
            chunk('FACE0000',12,[struct.pack('<3HBBI',0,1,2,0,0,0)]))


class KnifeTests(unittest.TestCase):
    def test_deterministic(self):
        a,r=bake(fixture());b,s=bake(fixture())
        self.assertEqual(a,b);self.assertEqual(r,s)
        self.assertEqual(struct.unpack_from('<4sIII',a),(b'SVRK',2,3,3))
        self.assertAlmostEqual(r['length_metres'],.30)

    def test_axis(self):
        a,_=bake(fixture())
        v=[struct.unpack_from('<3fI2f',a,16+i*24)[:3] for i in range(3)]
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

    def test_exact_uv_and_no_placeholder_tint(self):
        data,_=bake(fixture())
        for i,uv in enumerate(((0,0),(1,0),(.25,.75))):
            v=struct.unpack_from('<3fI2f',data,16+i*24)
            self.assertEqual(v[3],0xffffffff)
            self.assertEqual(v[4:],uv)

    def test_texture_exact_bgra_and_origin(self):
        root=Path(__file__).resolve().parents[2]/'build'/'knife'/'tests'
        root.mkdir(parents=True,exist_ok=True)
        with tempfile.TemporaryDirectory(dir=root) as tmp:
            path=Path(tmp)/'texture.png'
            im=Image.new('RGBA',(2,2));im.putdata([(255,0,0,17),(0,255,0,23),(0,0,255,31),(71,83,101,47)]);im.save(path)
            data,r=texture(path)
            self.assertEqual(struct.unpack_from('<4sIII',data),(b'SVRT',1,2,2))
            self.assertEqual(data[16:],im.tobytes('raw','BGRA'))
            self.assertEqual((r['width'],r['height']),(2,2))


if __name__=='__main__':unittest.main()
