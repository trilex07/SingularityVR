import unittest
from stage_xtts_native_delta import entries


class DeltaTests(unittest.TestCase):
    def setUp(self):
        self.pin = '1' * 64
        self.pins = {'rvgame/cookedpc/sp_test_loc_int.xxx': self.pin}
        self.row = dict(package='SP_test_LOC_INT', before=self.pin, after='2'*64,
                        lines=[dict(asset='Kathryn')], status='OFFLINE_CONTAINER_PASS_RUNTIME_PENDING')

    def test_success_pins_exact_native_bank(self):
        self.assertEqual(entries([self.row], self.pins)[0]['before'], self.pin)

    def test_no_safe_take_never_deploys_partial_output(self):
        row = dict(self.row, status='NO_SAFE_TAKES_ACCEPTED_BANK_RETAINED', lines=[], after=None)
        self.assertEqual(entries([row], self.pins), [])
        with self.assertRaises(ValueError):
            entries([dict(row, after='2'*64)], self.pins)

    def test_unpinned_duplicate_multiplayer_and_unvalidated_refused(self):
        for rows, pins in (([self.row], {}), ([self.row, self.row], self.pins),
                           ([dict(self.row, package='MP_test_LOC_INT')], self.pins),
                           ([dict(self.row, status='BUILT')], self.pins)):
            with self.assertRaises(ValueError):
                entries(rows, pins)


if __name__ == '__main__':
    unittest.main()
