import unittest
from unittest.mock import patch
from inspect_next_weapon_native import common_method, package_inheritance

class Tests(unittest.TestCase):
    def exports(self):
        return [dict(index=i+1, name=name, cls=0, outer=0, super_index=0 if i==0 else 1, class_name='None')
                for i,name in enumerate(('RvWeaponShared', 'RvWeaponShared_Shotgun', 'RvWeaponShared_SniperRifle'))]

    def test_inherited_script_states(self):
        with patch('inspect_next_weapon_native.inspect', return_value={'exports': self.exports()}):
            rows = package_inheritance('unused')
        self.assertEqual(len(rows), 2)
        self.assertTrue(all(not r['relevant_script_override'] and r['super_name']=='RvWeaponShared' for r in rows))

    def test_script_override_rejected(self):
        for name, kind in [('BeginFire','Function'), ('Active','State'), ('Reload','State')]:
            rows = self.exports()+[dict(index=4, name=name, cls=-1, outer=2, class_name=kind)]
            with patch('inspect_next_weapon_native.inspect', return_value={'exports': rows}):
                with self.assertRaisesRegex(ValueError, 'override'): package_inheritance('unused')

    def test_super_class_ambiguity_rejected(self):
        rows = self.exports();rows[1]['super_index']=3
        with patch('inspect_next_weapon_native.inspect', return_value={'exports': rows}):
            with self.assertRaises(ValueError): package_inheritance('unused')
        rows = self.exports()+[self.exports()[0]]
        with patch('inspect_next_weapon_native.inspect', return_value={'exports': rows}):
            with self.assertRaises(ValueError): package_inheritance('unused')
    def rows(self):
        return [dict(class_name=name, method=100) for name in
                ('RvWeaponShared', 'RvWeaponShared_Shotgun', 'RvWeaponShared_SniperRifle')]

    def test_shared_inherited_method(self):
        self.assertEqual(common_method(self.rows()), 100)

    def test_override_rejected(self):
        rows = self.rows(); rows[1]['method'] = 104
        with self.assertRaisesRegex(ValueError, 'override'): common_method(rows)

    def test_duplicate_missing_unknown_rejected(self):
        for count in (0, 1, 2):
            with self.assertRaises(ValueError): common_method(self.rows()[:count])
        rows = self.rows(); rows[1] = rows[0]
        with self.assertRaises(ValueError): common_method(rows)
        rows = self.rows(); rows[1]['class_name'] = 'RvWeaponShared_Unknown'
        with self.assertRaises(ValueError): common_method(rows)

if __name__ == '__main__': unittest.main()
