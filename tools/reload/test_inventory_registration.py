import struct
import unittest
from inspect_inventory_registration import resolve_wrapper


class FakePE:
    def __init__(self, pointer=0):
        self.data = bytearray(160)
        self.data[16:28] = 'entry\0'.encode('utf-16le')
        struct.pack_into('<II', self.data, 64, 0x1000 + 16, pointer)

    def address(self, offset):
        return 0x1000 + offset


class RegistrationTests(unittest.TestCase):
    def test_static(self):
        r = resolve_wrapper(FakePE(0x2010), bytes(128), 0x2000, 'entry')
        self.assertEqual(r['wrapper'], 0x2010)
        self.assertEqual(r['form'], 'static-pointer')

    def test_initializer(self):
        text = bytearray(128)
        text[0:10] = b'\xb8' + struct.pack('<I', 0x2020) + b'\xa3' + struct.pack('<I', 0x1044)
        r = resolve_wrapper(FakePE(), text, 0x2000, 'entry')
        self.assertEqual(r['wrapper'], 0x2020)
        self.assertEqual(r['form'], 'initializer-store')

    def test_missing_rejected(self):
        with self.assertRaises(ValueError):
            resolve_wrapper(FakePE(), bytes(128), 0x2000, 'entry')

    def test_outside_rejected(self):
        with self.assertRaises(ValueError):
            resolve_wrapper(FakePE(0x9999), bytes(128), 0x2000, 'entry')

    def test_ambiguous_rejected(self):
        pe = FakePE(0x2010)
        struct.pack_into('<II', pe.data, 80, 0x1010, 0x2020)
        with self.assertRaises(ValueError):
            resolve_wrapper(pe, bytes(128), 0x2000, 'entry')


if __name__ == '__main__':
    unittest.main()
