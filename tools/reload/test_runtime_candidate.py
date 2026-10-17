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

    def test_runtime_bootstrap_independent_of_centurion_and_cinema(self):
        # Source wiring contract only: actual producer identity still needs L3.
        source = (ROOT/'spikes/view_matrix/d3d9.cpp').read_text(encoding='utf-8')
        block = source.split('if(g_centurionReloadProbe||g_centurionNativeGuardMode||NextReloadEnabled()){', 1)[1]
        block = block.split('if (InterlockedCompareExchange(&g_cutsceneAuto', 1)[0]
        for required in ('attempts<12', '++tick%240==0&&g_camera',
                         'if(next&&(g_funcOffset<0||!g_hudObject))missing=true;',
                         'if(next&&g_funcOffset<0)FindProcessInternal();',
                         'if(next&&!g_hudObject)FindHudObject(true);'):
            self.assertIn(required, block)
        self.assertNotIn('RepointScriptHook(', block)

    def test_native_magazine_opt_in_skips_overlay_and_replays_inside_stereo(self):
        source = (ROOT/'spikes/view_matrix/d3d9.cpp').read_text(encoding='utf-8')
        dispatch = (ROOT/'spikes/view_matrix/weapon_reload_dispatch.inl').read_text(encoding='utf-8')
        self.assertIn('GetPrivateProfileIntA("Input","ReloadNativeMagazine",0,path)', source)
        self.assertIn('if(g_nextNativeMagazine)return;', dispatch.split('static void DrawNextReload(',1)[1])
        body = source.split('float nextSaved[16*12];',1)[1].split('const HRESULT nativeRestore=',1)[0]
        self.assertIn('StereoPair(dev', body)
        self.assertIn('weaponreload::Replay(', body)
        for forbidden in ('SetTexture(', 'SetRenderTarget(', 'SetDepthStencilSurface(', 'Clear('):
            self.assertNotIn(forbidden, body)

if __name__ == '__main__': unittest.main()
