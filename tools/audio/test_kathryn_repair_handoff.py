import unittest
from export_kathryn_repair_handoff import repair_rows


class RepairTests(unittest.TestCase):
    def row(self, asset='Line'):
        return dict(package='SP_RT1_Bank', asset=asset, file=asset+'.wav', speaker_guess='kathryn',
                    text_en='Original', text_pl='Polski tekst')

    def test_frame_budget_and_priority(self):
        report = [[dict(package='SP_RT1_Bank', lines=[], retained=[dict(
            asset='Line', native_frames=96000, rate=48000, source_duration=4, tempo=2)])]]
        result = repair_rows([self.row()], report)[0]
        self.assertEqual(result['native_window_s'], 2)
        self.assertAlmostEqual(result['preferred_guarded_take_max_s'], 1.98)
        self.assertAlmostEqual(result['integration_guarded_take_absolute_max_s'], 2.673)
        self.assertEqual(result['action'], 'regenerate-shorter-natural-take')
        self.assertEqual(result['priority'], '1-RT1')

    def test_codec_failure_not_sent_for_regeneration(self):
        report = [[dict(package='SP_RT1_Bank', lines=[], retained=[dict(asset='Line',
            reason='encoded native PCM budget mismatch; no metadata/delay guess', native_duration=2)])]]
        result = repair_rows([self.row()], report)[0]
        self.assertEqual(result['action'], 'MAIN-technical-review-no-regeneration-required')

    def test_missing_bank_has_no_invented_budget(self):
        result = repair_rows([self.row()], [])[0]
        self.assertIsNone(result['native_window_s'])
        self.assertIsNone(result['preferred_guarded_take_max_s'])

    def test_already_replaced_excluded(self):
        report = [[dict(package='SP_RT1_Bank', lines=[dict(asset='Line')], retained=[])]]
        self.assertEqual(repair_rows([self.row()], report), [])


if __name__ == '__main__':
    unittest.main()
