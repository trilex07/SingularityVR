"""Integration guards; geometric behavior is tested by the optimized C++ tests."""
from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[2]


class Wiring(unittest.TestCase):
    def test_default_off(self):
        source = (ROOT/'spikes/view_matrix/d3d9.cpp').read_text(encoding='utf8')
        for section, key in [('Input', 'WeaponMeasuredAim'), ('Render', 'ScopeMeasuredPose'), ('Diagnostic', 'BodyEquipmentProbe')]:
            self.assertIn(f'GetPrivateProfileIntA("{section}","{key}",0,path)', source)

    def test_geometry_published_only_after_success_and_restore(self):
        source = (ROOT/'spikes/view_matrix/d3d9.cpp').read_text(encoding='utf8')
        at = source.index('g_scopeMeasuredFrame=g_scopeMeasuredStaged;')
        guard = source[:at][-500:]
        self.assertIn('SUCCEEDED(dhr)&&SUCCEEDED(nextRestore)', guard)
        self.assertIn('g_nextMuzzleStaged.valid', guard)
        self.assertIn('if(g_scopeMeasuredStaged.active)', guard)
        self.assertIn('g_scopeMeasuredFrame={}', source)
        self.assertIn('else g_scopePipFrame.active=false;', source)

    def test_spread_retained_and_fresh_witness_required(self):
        source = (ROOT/'spikes/view_matrix/d3d9.cpp').read_text(encoding='utf8')
        self.assertIn('measured&&weapontrace::Align(Start,End,from,to)', source)
        self.assertIn('witness.Direction(', source)
        self.assertIn('g_weaponMeasuredAim&&am==11&&!scoped', source)

    def test_equipment_is_a_probe_not_an_unverified_backend(self):
        source = (ROOT/'spikes/view_matrix/native_body_equipment_probe.h').read_text(encoding='utf8')
        self.assertIn('if(!g_bodyEquipmentProbe)return;', source)
        self.assertIn('reports>=32', source)
        self.assertIn('now+1000', source)
        self.assertIn('0xAD9AEEB598427D84ull', source)
        self.assertIn('0x3F880641281843C5ull', source)
        self.assertIn('0xB26B7A29EA360695ull', source)
        self.assertIn('mIndexedPowers', source)
        self.assertIn('reports>=8', source)
        for forbidden in ['SpawnActor(', 'TriggerActiveWeaponThrow(', 'Select)', 'SetTimer(']:
            self.assertNotIn(forbidden, source)
        native_input = (ROOT/'spikes/view_matrix/native_ui_input.h').read_text(encoding='utf8')
        self.assertIn('NativeBodyEquipmentProbe::Poll();', native_input)


if __name__ == '__main__':
    unittest.main()
