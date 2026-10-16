import struct
import unittest
from inspect_native_reload import PE, only, wrapper_slot, occurrences, class_vtable, registration


def fake_pe():
    b=bytearray(1024)
    b[:2]=b'MZ';struct.pack_into('<I',b,60,128)
    b[128:132]=b'PE\0\0';struct.pack_into('<HH',b,132,0x14c,1)
    struct.pack_into('<H',b,148,96);struct.pack_into('<H',b,152,0x10b)
    struct.pack_into('<I',b,180,0x400000)
    b[248:253]=b'.text';struct.pack_into('<4I',b,256,512,0x1000,512,512)
    return b


class Tests(unittest.TestCase):
    def test_pe_mapping_and_fail_closed(self):
        b=fake_pe();p=PE(b)
        self.assertEqual(p.address(512),0x401000)
        self.assertEqual(p.read(0x401000,4),bytes(4))
        for va,n in [(0x400fff,4),(0x4011ff,4),(0,4)]:
            with self.assertRaises(ValueError):p.read(va,n)
        with self.assertRaises(ValueError):p.address(1)
        for at,value in [(0,0),(132,0),(152,0)]:
            bad=bytearray(b);bad[at]=value
            with self.assertRaises(ValueError):PE(bad)
        with self.assertRaises(ValueError):PE(bytes(100))

    def test_wrapper_shape_and_slot(self):
        b=bytearray(58);b[:9]=bytes.fromhex('558bec8b4508ff4018')
        b[36:40]=bytes.fromhex('8b068b90');struct.pack_into('<I',b,40,0x4c4)
        b[44:]=bytes.fromhex('8bceffd28b4d0c89015e5dc20800')
        self.assertEqual(wrapper_slot(b),0x4c4)
        for value in (0,0x4c5,0x2000):
            bad=bytearray(b);struct.pack_into('<I',bad,40,value)
            with self.assertRaises(ValueError):wrapper_slot(bad)
        for at in (0,8,36,44,57):
            bad=bytearray(b);bad[at]^=1
            with self.assertRaises(ValueError):wrapper_slot(bad)
        with self.assertRaises(ValueError):wrapper_slot(b[:-1])

    def test_ambiguity_and_overlapping_matches(self):
        self.assertEqual(list(occurrences(b'aaaa',b'aa')),[0,1,2])
        self.assertEqual(only([3],'one'),3)
        for rows in ([],[1,2]):
            with self.assertRaises(ValueError):only(rows,'ambiguous')

    def test_absent_name_cannot_resolve_code(self):
        p=PE(fake_pe())
        with self.assertRaises(ValueError):registration(p,bytes(512),0x401000,'missing')
        with self.assertRaises(ValueError):class_vtable(p,bytes(512),0x401000,'missing')


if __name__=='__main__':unittest.main()
