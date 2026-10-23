"""Integration guards; numerical proof is test-weapon-carry-ray + private PSKs."""
from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[2]


class Wiring(unittest.TestCase):
    def test_new_paths_default_off_and_keep_native_material(self):
        source = (ROOT/'spikes/view_matrix/d3d9.cpp').read_text(encoding='utf8')
        for key in ('ReloadPalmGrip', 'WeaponTraceRigidOrigin'):
            self.assertIn(f'GetPrivateProfileIntA("Input","{key}",0,path)', source)
        dispatch = (ROOT/'spikes/view_matrix/weapon_reload_dispatch.inl').read_text(encoding='utf8')
        self.assertIn('weaponcarry::Matrix(kind,wrist.tracking,carried)', dispatch)
        self.assertIn('weaponreload::SparePalette(g_nextAnimation[index],carried,local,controller,world', dispatch)
        self.assertIn('if(g_nextNativeMagazine)return;', dispatch)

    def test_wrist_and_muzzle_publish_only_after_restore(self):
        source = (ROOT/'spikes/view_matrix/d3d9.cpp').read_text(encoding='utf8')
        publish = source.index('g_nextWrist=g_nextWristStaged;')
        self.assertLess(source.index('const HRESULT handRestore='), publish)
        self.assertIn('SUCCEEDED(dhr)&&SUCCEEDED(nativeRestore)&&SUCCEEDED(handRestore)', source[:publish][-420:])
        publish = source.index('g_nextMuzzle=g_nextMuzzleStaged;')
        self.assertLess(source.index('const HRESULT nextRestore='), publish)
        self.assertIn('SUCCEEDED(dhr)&&SUCCEEDED(nextRestore)', source[:publish][-240:])

    def test_valkyrie_tmd_carry_reaches_the_measured_wrist_route(self):
        source = (ROOT/'spikes/view_matrix/d3d9.cpp').read_text(encoding='utf8')
        self.assertIn('weaponcarry::CarrySectionEligible(nextCarryVisual.kind,gloves)', source)
        self.assertIn('weaponcarry::Source(nextCarryVisual.kind,nextCarry,gloves,tmdGloves,', source)
        dispatch = (ROOT/'spikes/view_matrix/weapon_reload_dispatch.inl').read_text(encoding='utf8')
        self.assertIn('weaponcarry::Matrix(kind,wrist.tracking,carried)', dispatch)
        self.assertIn('kind==weaponreload::Kind::Valkyrie', dispatch)

    def test_probe_preserves_accepted_vfx_and_no_custom_substitute(self):
        source = (ROOT/'tools/reload/prepare_vr_settings_rifle_test.ps1').read_text(encoding='utf8')
        self.assertIn('[switch]$NativeFxCapture', source)
        self.assertIn("$targets['Diagnostic/NativeMuzzleProbe']='1'", source)
        self.assertIn("'Render/NativeMuzzleParticleStereo'='1'", source)
        self.assertNotIn("$targets['Diagnostic/NativeMuzzleStereoMode']", source)
        self.assertNotIn("$targets['Render/VfxHost']", source)


if __name__ == '__main__':
    unittest.main()
