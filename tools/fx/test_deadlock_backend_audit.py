"""Synthetic CPU tests for ctor identity, not a game/Deadlock gameplay test."""
import struct
import unittest
from audit_deadlock_backend import constructor
from audit_rifle_scope_backend import fnv


class FakePE:
    data = b'\0'*32+('SyntheticPower\0').encode('utf-16le')

    def address(self, offset):
        return 0x1000+offset

    def read(self, address, size):
        if address not in (0x2000, 0x3000, 0x4000) or size != 4:
            raise ValueError('not a backed table')
        return b'\0'*4


def fixture(code):
    archive = bytearray(b'\xcc'*512)
    archive[128:128+len(code)] = code
    archive[390:395] = b'\x68'+struct.pack('<I', 0x5080)
    archive[400:405] = b'\x68'+struct.pack('<I', 0x1020)
    return archive


class ConstructorIdentity(unittest.TestCase):
    def setUp(self):
        # Public x86 instruction forms only. No game code or assets.
        self.code = (b'\xc7\x06'+struct.pack('<I', 0x2000)+
                     b'\xc7\x46\x04'+struct.pack('<I', 0x3000)+
                     b'\xc7\x06'+struct.pack('<I', 0x4000)+b'\xc3')

    def test_final_primary_store_not_interface_or_base(self):
        result = constructor(FakePE(), fixture(self.code), 0x5000, 'SyntheticPower',
                             len(self.code), fnv(self.code))
        self.assertEqual(result['vtable'], 0x4000)

    def test_fingerprint_drift_refuses(self):
        data = fixture(self.code)
        data[130] ^= 1
        with self.assertRaises(ValueError):
            constructor(FakePE(), data, 0x5000, 'SyntheticPower', len(self.code), fnv(self.code))

    def test_duplicate_matching_callback_refuses(self):
        data = fixture(self.code)
        data[380:385] = data[390:395]
        with self.assertRaises(ValueError):
            constructor(FakePE(), data, 0x5000, 'SyntheticPower', len(self.code), fnv(self.code))

    def test_truncated_epilogue_refuses_even_with_matching_hash(self):
        code = self.code[:-1]
        with self.assertRaises(ValueError):
            constructor(FakePE(), fixture(code), 0x5000, 'SyntheticPower', len(code), fnv(code))


if __name__ == '__main__':
    unittest.main()
