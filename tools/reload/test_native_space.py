"""Evidence/parser guards. Actual private cooked-byte oracle is separate."""
from pathlib import Path
import struct
import unittest
from inspect_reload_shader import extract

ROOT = Path(__file__).resolve().parents[2]


class NativeSpace(unittest.TestCase):
    def test_capture(self):
        lines = ['RenderBytecode id=1 stage=VS words=3',
                 'RenderWords id=1 stage=VS at=0 FFFE0300 00000000',
                 'RenderWords id=1 stage=VS at=2 0000FFFF']
        self.assertEqual(extract(lines, 1), struct.pack('<3I', 0xfffe0300, 0, 0xffff))

    def test_capture_rejects_gaps_duplicates_size_and_wrong_type(self):
        header = 'RenderBytecode id=1 stage=VS words=2'
        chunk = 'RenderWords id=1 stage=VS at=0 FFFE0300 0000FFFF'
        for lines in ([chunk], [header], [header, header, chunk], [header, chunk, chunk],
                      [header, chunk.replace('at=0', 'at=1')],
                      [header.replace('words=2', 'words=4097'), chunk],
                      [header, chunk.replace('FFFE0300', 'FFFF0300')]):
            with self.subTest(lines=lines), self.assertRaises(ValueError):
                extract(lines, 1)

    def test_conversion_is_initialization_only_and_selected_for_both_native_draws(self):
        src = (ROOT/'spikes/view_matrix/d3d9.cpp').read_text(encoding='utf8')
        self.assertEqual(src.count('centurionanim::ToNative('), 1)
        self.assertIn('g_centurionAnimation={};g_centurionNativeAnimation={};', src)
        self.assertIn('centuriontracked::Attached(g_centurionNativeAnimation,time,', src)
        self.assertIn('g_centurionTrackedCylinder?g_centurionNativeAnimation:g_centurionAnimation;', src)

    def test_native_pose_does_not_double_reflect(self):
        src = (ROOT/'spikes/view_matrix/centurion_cylinder_animation.h').read_text(encoding='utf8')
        self.assertIn('bool nativeSpace=false;', src)
        self.assertIn('if(!d.nativeSpace)for(', src)
        self.assertIn('reflect(staged.parent);reflect(staged.inverse[0]);reflect(staged.inverse[1]);', src)

    def test_next_weapon_native_conversion_is_load_time_and_shared(self):
        src = (ROOT/'spikes/view_matrix/weapon_reload_dispatch.inl').read_text(encoding='utf8')
        self.assertEqual(src.count('weaponreloadanim::ToNative('), 1)
        init, draw = src.split('static int PrepareNextReload(', 1)
        self.assertIn('weaponreloadanim::ToNative(g_nextAnimation[i])&&', init)
        self.assertNotIn('ToNative(', draw)
        animation = (ROOT/'spikes/view_matrix/weapon_reload_animation.h').read_text(encoding='utf8')
        self.assertIn('if(!d.nativeSpace)ReflectY(local);', animation)
        runtime = (ROOT/'spikes/view_matrix/weapon_reload_runtime.h').read_text(encoding='utf8')
        self.assertIn('d.nativeSpace&&axis==1?-52.5f:52.5f', runtime)
        self.assertIn('if(d.nativeSpace)metres.m[1][1]=-metres.m[1][1];', runtime)

    def test_physical_mode_is_default_off_and_keeps_legacy_gate(self):
        src=(ROOT/'spikes/view_matrix/d3d9.cpp').read_text(encoding='utf8')
        self.assertIn('g_centurionTrackedCylinder&&GetPrivateProfileIntA("Input","CenturionPhysicalPolish",0,path)',src)
        self.assertIn('g_centurionConfig.upwardEjection=g_centurionPhysicalPolish!=0;',src)
        self.assertIn('ownedContext=f.context||(g_centurionPhysicalPolish&&context&&alive&&Rd(g_weaponHolstered)',src)

    def test_palm_witness_after_successful_draw_restore_and_input_same_point(self):
        src=(ROOT/'spikes/view_matrix/d3d9.cpp').read_text(encoding='utf8')
        self.assertIn('SUCCEEDED(dhr)&&SUCCEEDED(nativeRestore)&&SUCCEEDED(handRestore)',src)
        self.assertLess(src.index('const HRESULT handRestore='),src.index('g_centurionGrip=g_centurionGripStaged;'))
        self.assertIn('carried.p=g_centurionGrip.identity.point;carriedValid=true;',src)
        self.assertIn('centurionvisual::SpareDesired(clipData,local,gunC,tracking,desired,replay.clip)',src)

    def test_health_plate_hidden_only_for_fresh_carry(self):
        src=(ROOT/'spikes/view_matrix/d3d9.cpp').read_text(encoding='utf8').split('void DrawHealthPlate(IDirect3DDevice9* dev) {',1)[1]
        gate=src.split('if (!Rd(g_healthPlate)',1)[0]
        self.assertIn('if(g_centurionPhysicalPolish)',gate)
        self.assertIn('centurionreload::Phase::Carrying',gate)
        self.assertIn('centurionclip::Current(',gate)


if __name__ == '__main__':
    unittest.main()
