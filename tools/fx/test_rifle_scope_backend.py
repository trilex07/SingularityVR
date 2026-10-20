"""Synthetic guard and wiring tests; no game/process or proprietary fixtures."""
from pathlib import Path
import struct
import unittest
from audit_rifle_scope_backend import body, fnv, registration, helper_contract

ROOT = Path(__file__).resolve().parents[2]

class NativeBackendTests(unittest.TestCase):
    def test_helper_target_requires_exact_call_bounds_and_body(self):
        raw = bytearray(b'\x90'*64)
        raw[8:13] = b'\xe8'+struct.pack('<i', 32-13)
        raw[32:35] = b'\x55\x5d\xc3'
        fingerprint = fnv(raw[32:35])
        receipt = helper_contract(raw, 0x1000, 0x1000, 8, 3, fingerprint)
        self.assertEqual(receipt['target'], 0x1020)
        for changed in (0, 8, 32):
            mutated = raw[:]; mutated[changed] ^= 1
            if changed == 0:  # Unrelated wrapper bytes are not part of the helper.
                self.assertEqual(helper_contract(mutated, 0x1000, 0x1000, 8, 3, fingerprint), receipt)
            else:
                with self.assertRaises(ValueError): helper_contract(mutated, 0x1000, 0x1000, 8, 3, fingerprint)
        with self.assertRaises(ValueError): helper_contract(raw, 0x1000, 0x1000, 61, 3, fingerprint)
        raw[9:13] = struct.pack('<i', -0x2000)
        with self.assertRaises(ValueError): helper_contract(raw, 0x1000, 0x1000, 8, 3, fingerprint)

    def test_hash_and_bounded_epilogue(self):
        self.assertEqual(fnv(b''), 14695981039346656037)
        self.assertEqual(fnv(b'a'), 0xAF63DC4C8601EC8C)
        ins, receipt = body(b'\x55\x8b\xec\xc2\x18\x00', 0x1000, 0x1000)
        self.assertEqual(ins[-1].op_str, '0x18')
        self.assertEqual(receipt['bytes'], 6)
        with self.assertRaises(ValueError): body(b'\x90'*16, 0x1000, 0x1000, 16)
        with self.assertRaises(ValueError): body(b'\xc3', 0x1000, 0xfff)

    def test_registration_refuses_ambiguous_or_outside_code(self):
        class FakePE:
            data = b''
            def address(self, offset): return offset + 0x400000
        pe = FakePE()
        with self.assertRaises(ValueError): registration(pe, b'\xc3', 0x1000, 'missing')
        name = 'native\0'.encode('utf-16le')
        pe.data = name + b'\0'*(32-len(name)) + struct.pack('<II', 0x400000, 0x900000)
        with self.assertRaises(ValueError): registration(pe, b'\xc3', 0x1000, 'native')

    def test_default_off_exact_identity_and_no_fullscreen_crop(self):
        source = (ROOT/'spikes/view_matrix/d3d9.cpp').read_text(encoding='utf-8')
        for key in ('"SniperPictureInPicture",0,path', '"NativeRifleMuzzleOrigin",0,path'):
            self.assertIn(key, source)
        backend = (ROOT/'spikes/view_matrix/native_scope_pip.inl').read_text()
        for guard in ('CodeContract("wrapper/Create",create,496', 'PauseRttProbe::ExactTexture',
                      'completion.epoch', 'state->Capture()', 'state->Apply()',
                      'texture->Release()', 'if(running)return;', 'int(eye)==f.eye'):
            self.assertIn(guard, backend)
        self.assertNotIn('StretchRect', backend)
        rifle = (ROOT/'spikes/view_matrix/native_rifle_fx.inl').read_text()
        self.assertIn('original(self,changed?&replacement:location,firstPerson)', rifle)
        self.assertIn('0xDEA566CA454CAFEBull', rifle)
        self.assertIn('notes++<24', rifle)
        for guard in ('BindMovement()', '0x8A32507F9F7A934Aull', '0xAA63F1F63DEFF3D8ull',
                      'movingFx+0x88', 'movingPsc+templateOffset', 'riflefx::Lease(',
                      'if(changed)Track(', 'setLocation((void*)movingEmitter,position)'):
            self.assertIn(guard, rifle)
        self.assertIn('NativeRifleFx::MaintainTick()', backend)
        self.assertIn('NativeRegistry::Find("intUTextureRenderTarget2DexecCreate")', backend)
        self.assertNotIn('*(hipknife::V*)', rifle)  # No raw actor transform writes.

if __name__ == '__main__': unittest.main()
