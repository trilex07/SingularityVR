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


if __name__ == '__main__':
    unittest.main()
