import unittest
from audit_native_voice_coverage import audit


class CoverageTests(unittest.TestCase):
    def row(self, package='Bank', asset='Line'):
        return dict(package=package, asset=asset, speaker_guess='kathryn', native_replaced='1')

    def test_overlay_hint_is_not_delivery(self):
        self.assertEqual(audit([self.row()], [], 'kathryn')['counts'], {'not-covered': 1})

    def test_package_is_part_of_identity(self):
        report = [[dict(package='BANK', lines=[dict(asset='LINE')], retained=[])]]
        r = audit([self.row(), self.row('Other')], report, 'Kathryn')
        self.assertEqual(r['counts'], {'replaced': 1, 'not-covered': 1})

    def test_retained_is_not_polish(self):
        report = [[dict(package='Bank', lines=[], retained=[dict(asset='Line', tempo=2.3)])]]
        r = audit([self.row()], report, 'kathryn')
        self.assertEqual(r['counts'], {'retained': 1})
        self.assertEqual(r['lines'][0]['tempo'], 2.3)

    def test_ambiguity_refused(self):
        with self.assertRaises(ValueError):
            audit([self.row(), self.row('BANK', 'LINE')], [], 'kathryn')
        bank = dict(package='Bank', lines=[dict(asset='Line')], retained=[])
        with self.assertRaises(ValueError):
            audit([self.row()], [[bank], [bank]], 'kathryn')


if __name__ == '__main__':
    unittest.main()
