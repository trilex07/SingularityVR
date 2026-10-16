import struct
import unittest
import numpy as np
from inspect_centurion_animation import read_psa, describe


def fixture():
    def chunk(name, size, count, raw=b''):
        return struct.pack('<20siii',name.encode(),20100422,size,count)+raw
    bones = b''.join(name.encode().ljust(120,b'\0') for name in ['root','clip'])
    info = b'reload'.ljust(64,b'\0')+b'None'.ljust(64,b'\0')
    info += struct.pack('<4i3f3i',2,0,0,6,0,3,30,0,0,3)
    keys = np.zeros((3,2,8),dtype='<f4');keys[:,:,6:]=1
    keys[1,1,0] = 5
    keys[1,1,3:7] = [0,0,1,0]  # known 180-degree local rotation
    return (chunk('ANIMHEAD',0,0)+chunk('BONENAMES',120,2,bones)+
            chunk('ANIMINFO',168,1,info)+chunk('ANIMKEYS',32,6,keys.tobytes())+
            chunk('SCALEKEYS',16,0))


class Tests(unittest.TestCase):
    def test_exact_roundtrip_and_known_track(self):
        data = fixture();names,seq,keys = read_psa(data)
        self.assertEqual(names,['root','clip']);self.assertEqual(keys.shape,(3,2,8))
        report = describe(data,['root','extra','clip'],[-1,0,0])
        self.assertEqual(report['clip_psa_track'],1)
        self.assertEqual(report['clip_psk_bone'],2)  # never confuse palette indices
        self.assertEqual(report['clip_parent'],'root')
        row = report['sequences'][0]
        self.assertEqual(row['max_clip_local_travel_uu'],5)
        self.assertEqual(row['max_clip_local_rotation_degrees'],180)
        self.assertEqual(row['end_clip_local_travel_uu'],0)
        self.assertEqual(seq[0]['rate'],30)

    def test_truncated_duplicate_and_oversized_chunks(self):
        for data in (fixture()[:-1],fixture()+fixture()[:32],fixture()[:10]):
            with self.assertRaises(ValueError):read_psa(data)
        data = bytearray(fixture());struct.pack_into('<i',data,60,1000000000)
        with self.assertRaises(ValueError):read_psa(data)

    def test_bad_rotation_nonfinite_and_range(self):
        data = fixture();start = data.index(b'ANIMKEYS')+32
        for offset,value in ((start+24,0),(start,float('nan'))):
            bad = bytearray(data);struct.pack_into('<f',bad,offset,value)
            with self.assertRaises(ValueError):read_psa(bad)
        bad = bytearray(data)
        struct.pack_into('<i',bad,data.index(b'ANIMINFO')+32+160,999)
        with self.assertRaises(ValueError):read_psa(bad)

    def test_name_mapping_not_psa_placeholder_hierarchy(self):
        with self.assertRaises(ValueError):describe(fixture(),['root','other'],[-1,0])
        with self.assertRaises(ValueError):describe(fixture(),['root','clip'],[-1,99])


if __name__ == '__main__':unittest.main()
