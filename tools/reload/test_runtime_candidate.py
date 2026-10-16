import copy
import hashlib
from pathlib import Path
import struct
import unittest
from prepare_runtime_candidate import ROOT, build_receipt, checked_output, pe32, fnv

def image():
    data = bytearray(512);data[:2] = b'MZ';struct.pack_into('<I', data, 60, 128)
    data[128:132] = b'PE\0\0';struct.pack_into('<H', data, 132, 0x14c);struct.pack_into('<H', data, 152, 0x10b)
    return bytes(data)

class Tests(unittest.TestCase):
    def test_pe_architecture_bounds(self):
        pe32(image())
        for offset, value in ((0, 0), (128, 0), (132, 0), (152, 0), (63, 255)):
            data = bytearray(image());data[offset] = value
            with self.assertRaises(ValueError): pe32(data)
        with self.assertRaises(ValueError): pe32(b'')

    def test_receipt_clean_identity_sha_optimization(self):
        data = image();receipt = dict(commit='a', workingTreeStatus=[], optimization='/O2',
            architecture='x86 PE32', performanceInstrumentation=False, sha256=hashlib.sha256(data).hexdigest().upper())
        build_receipt(receipt, data, 'a', '')
        for key, value in [('commit', 'b'), ('workingTreeStatus', [' M x']), ('optimization', '/Od'),
                           ('architecture', 'x64'), ('performanceInstrumentation', True), ('sha256', 'wrong')]:
            bad = copy.deepcopy(receipt);bad[key] = value
            with self.assertRaises(ValueError): build_receipt(bad, data, 'a', '')
        with self.assertRaises(ValueError): build_receipt(receipt, data, 'a', ' M file')

    def test_output_cannot_leave_lab_or_overwrite(self):
        root = ROOT/'build/next-weapon-reload'
        for path in (ROOT, ROOT/'build', root, root/'../../not-allowed', Path('C:/Steam')):
            with self.assertRaises(ValueError): checked_output(path)
        with self.assertRaises(ValueError): checked_output(root/'prepared')
        self.assertEqual(checked_output(root/'not-created-by-this-test'), (root/'not-created-by-this-test').resolve())

    def test_fnv_known_vector(self):
        self.assertEqual(fnv(b''), 2166136261)
        self.assertEqual(fnv(b'hello'), 0x4f9f2cab)

if __name__ == '__main__': unittest.main()
