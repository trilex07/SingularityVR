import unittest
from prepare_xtts_native_banks import select_rows


class NativeIdentityTests(unittest.TestCase):
    def test_exact_package_not_speaker_guess(self):
        rows = [dict(package='SP_A', asset='KAT_1', file='a.wav'),
                dict(package='SP_B', asset='KAT_1', file='b.wav')]
        self.assertEqual(select_rows(rows, 'sp_a')['KAT_1']['file'], 'a.wav')

    def test_duplicates_refused(self):
        row = dict(package='A', asset='one', file='a.wav')
        with self.assertRaises(ValueError):
            select_rows([row, row], 'A')

    def test_traversal_refused(self):
        with self.assertRaises(ValueError):
            select_rows([dict(package='A', asset='one', file='../a.wav')], 'A')

    def test_absent_identity_refused(self):
        with self.assertRaises(ValueError):
            select_rows([], 'A')


if __name__ == '__main__':
    unittest.main()
