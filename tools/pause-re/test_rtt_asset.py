"""Pinned real-asset tests. Assets/decoder outputs remain outside Git.
Usage: python test_rtt_asset.py LAB_DIRECTORY
Requires native-rtt-{control,dedicated}-v1/cpp-decoded.u from the independent decoder.
"""
import struct
import sys
import unittest
from pathlib import Path
from build_rtt_asset import build, pack, texture_offset_fields, PKG_COMPRESSED
from inspect_package import inspect

LAB = Path(sys.argv.pop(1))

class PrototypeTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.raw = (LAB/'UI_SinglePlayer_SF.u').read_bytes()
        cls.original = inspect('original', cls.raw, include_all=True)
        cls.output, cls.meta = build(cls.raw, 'dedicated')
        cls.result = inspect('result', cls.output, include_all=True)
        cls.old = {e['name']:e for e in cls.original['exports']}
        cls.new = {e['name']:e for e in cls.result['exports']}

    def test_tables_and_serial_tiling(self):
        self.assertEqual(self.result['counts'], dict(names=227, exports=45, imports=26))
        self.assertEqual(self.original['names'], self.result['names'][:-1])
        self.assertEqual(self.original['imports'], self.result['imports'])
        end = struct.unpack_from('<I', self.output, 8)[0]
        for e in sorted(self.result['exports'], key=lambda e:e['offset']):
            self.assertNotIn('parse_error', e)
            self.assertEqual(e['offset'], end)
            end += e['size']
        self.assertEqual(end, len(self.output))
        for name, old in self.old.items():
            new = self.new[name]
            for key in ('index','cls','super_index','outer','archetype'):
                self.assertEqual(old[key], new[key], (name,key))

    def test_only_pause_uses_dedicated_target(self):
        def props(e): return {p['name']:p['value'] for p in e['properties']}
        self.assertEqual(props(self.new['SPPause']), props(self.old['SPPause']) |
                         dict(RenderTexture='RTT_VRPause', RenderTextureMode='RTM_AlphaComposite'))
        users = [e['name'] for e in self.result['exports']
                 if props(e).get('RenderTexture') == 'RTT_VRPause']
        self.assertEqual(users, ['SPPause'])
        target = self.new['RTT_VRPause']
        self.assertEqual(target['index'],45)
        self.assertEqual(target['class_name'],'TextureRenderTarget2D')
        self.assertEqual(target['outer'],self.old['RTT_UI2']['outer'])
        self.assertEqual(props(target),props(self.old['RTT_UI2']))
        self.assertEqual(struct.unpack_from('<i',self.output,target['offset'])[0],-1)

    def test_byte_preservation_and_structured_relocations(self):
        for name, old in self.old.items():
            new = self.new[name]
            expected = bytearray(self.raw[old['offset']:old['offset']+old['size']])
            for rr in self.meta['relocations']:
                if rr['export'] == old['index']:
                    self.assertEqual(rr['new']-rr['old'],new['offset']-old['offset'])
                    struct.pack_into('<I',expected,rr['local'],rr['new'])
            if name != 'SPPause':
                self.assertEqual(self.output[new['offset']:new['offset']+new['size']],expected,name)
            if new['class_name'] in ('Texture2D','TextureRenderTarget2D'):
                texture_offset_fields(self.output,new)
        texture_offset_fields(self.output,self.new['RTT_VRPause'])

    def test_cpp_lzo_roundtrip_and_deterministic_artifacts(self):
        for mode in ('control','dedicated'):
            expected,_ = build(self.raw,mode)
            root = LAB/f'native-rtt-{mode}-v1'
            self.assertEqual((root/'UI_SinglePlayer_SF.u').read_bytes(),expected)
            self.assertEqual((root/'UI_SinglePlayer_SF.xxx').read_bytes(),pack(expected))
            decoded = bytearray((root/'cpp-decoded.u').read_bytes())
            flags = struct.unpack_from('<I',decoded,0x15)[0]
            struct.pack_into('<I',decoded,0x15,flags & ~PKG_COMPRESSED)
            self.assertEqual(decoded,expected)
        # Repack-only control restores every source byte when decoder preserves store-compressed flag.
        self.assertEqual((LAB/'native-rtt-control-v1/cpp-decoded.u').read_bytes(),self.raw)

    def test_unknown_source_and_native_layout_refused(self):
        altered=bytearray(self.raw);altered[-1]^=1
        with self.assertRaisesRegex(ValueError,'source hash mismatch'):build(altered,'dedicated')
        e=self.old['RTT_UI2'];altered=bytearray(self.raw)
        struct.pack_into('<I',altered,e['property_end'],1)
        with self.assertRaisesRegex(ValueError,'unsupported bulk'):texture_offset_fields(altered,e)
        with self.assertRaisesRegex(ValueError,'unknown build mode'):build(self.raw,'unsupported')

    def test_refusal_to_overwrite_or_write_into_git_or_steam(self):
        import subprocess
        tool = Path(__file__).with_name('build_rtt_asset.py')
        for dest, reason in ((LAB/'native-rtt-dedicated-v1','refuses overwrite'),
                             (tool.parent/'test-assets','assets must stay outside Git'),
                             (LAB/'SteamLibrary/test-assets','refuses Steam output')):
            run = subprocess.run([sys.executable,str(tool),str(LAB/'UI_SinglePlayer_SF.u'),
                                  '--mode','dedicated','--output-dir',str(dest)],capture_output=True,text=True)
            self.assertNotEqual(run.returncode,0)
            self.assertIn(reason,run.stderr)
        self.assertFalse((tool.parent/'test-assets').exists())
        self.assertFalse((LAB/'SteamLibrary/test-assets').exists())

if __name__=='__main__':unittest.main(verbosity=2)
