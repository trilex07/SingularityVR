"""Offline falsification of texture class resolution; no game bytes required."""
import importlib.util
from pathlib import Path
import struct
import unittest

spec = importlib.util.spec_from_file_location('native_audit', Path(__file__).resolve().parents[1] / 'audit-singularity-native-consumers.py')
audit = importlib.util.module_from_spec(spec)
spec.loader.exec_module(audit)


class TextureIdentity(unittest.TestCase):
    def setUp(self):
        self.image = bytearray(4096)
        self.base = audit.BASE
        self.constructor, self.destructor, self.entry, self.vtable = [self.base + p for p in (0x100, 0x200, 0x400, 0x600)]
        self.image[0x167:0x169] = b'\xc7\x06'
        self.word(0x169, self.vtable)
        self.word(0x600, self.entry)
        self.image[0x406] = 0xe8
        struct.pack_into('<i', self.image, 0x407, self.destructor - self.entry - 11)
        self.image[0x229:0x22b] = b'\xc7\x06'
        self.word(0x22b, self.vtable)

    def word(self, offset, value):
        struct.pack_into('<I', self.image, offset, value)

    def resolve(self):
        return audit.texture_class(self.image, self.constructor, (0x100, 0x400))

    def test_backwards_direct_call_and_class_agreement(self):
        self.assertEqual(self.resolve()['destructor'], f'{self.destructor:08X}')

    def test_adjacent_vtable_slot_refused(self):
        # The original ambiguity consisted of adjacent slots of similar classes.
        self.word(0x169, self.vtable + 4)
        self.word(0x604, self.entry)
        with self.assertRaisesRegex(ValueError, 'identities disagree'):
            self.resolve()

    def test_out_of_image_vtable_refused(self):
        self.word(0x169, 0xffffffff)
        with self.assertRaisesRegex(ValueError, 'outside image'):
            self.resolve()

    def test_out_of_text_deleting_entry_refused(self):
        self.word(0x600, self.base + 0x700)
        with self.assertRaisesRegex(ValueError, 'outside text'):
            self.resolve()

    def test_out_of_text_destructor_refused(self):
        struct.pack_into('<i', self.image, 0x407, 0x700 - 0x40b)
        with self.assertRaisesRegex(ValueError, 'outside text'):
            self.resolve()

    def test_virtual_or_truncated_call_refused(self):
        self.image[0x406] = 0xff
        with self.assertRaisesRegex(ValueError, 'direct call'):
            self.resolve()
        for address in (self.base-1, self.base+4093):
            with self.assertRaises(ValueError):
                audit.call_target(self.image, address)

    def test_destructor_store_shape_refused(self):
        self.image[0x229] ^= 1
        with self.assertRaisesRegex(ValueError, 'identities disagree'):
            self.resolve()


if __name__ == '__main__':
    unittest.main()
