"""First-refusal codec errors must not conceal timing work from SIDE."""
import unittest

from native_voice_fit import native_tempo
from reconcile_dubbing_timing import reconcile


class TimingReconciliation(unittest.TestCase):
    def test_codec_overlap_and_exact_guarded_limit(self):
        rows = [dict(package='bank', asset='long', native_duration=1.,
                     source_duration=1.5, tempo=native_tempo(1.5, 1.),
                     reason='encoded native PCM budget mismatch; bounded silence retry refused'),
                dict(package='bank', asset='short', native_frames=48000, rate=48000,
                     source_duration=.5, tempo=1.,
                     reason='encoded native PCM budget mismatch; bounded silence retry refused')]
        out = reconcile(dict(retained=rows), [dict(package='bank', asset=r['asset']) for r in rows])
        self.assertEqual(out['summary']['timing_cases'], 1)
        self.assertEqual(out['summary']['codec_cases'], 2)
        self.assertEqual(out['summary']['timing_and_codec_cases'], 1)
        self.assertEqual(out['summary']['codec_only_cases'], 1)
        self.assertAlmostEqual(out['timing_cases'][0]['max_allowed_source_duration'], 1.323)
        self.assertAlmostEqual(native_tempo(1.323, 1.), 1.35)
        # The old analyzer subtracted guards twice and omitted the 20ms reserve.
        self.assertNotAlmostEqual(out['timing_cases'][0]['max_allowed_source_duration'], 1.27)

    def test_receipt_contradiction_and_duplicate_are_rejected(self):
        row = dict(package='bank', asset='a', source_duration=2., native_duration=1., tempo=2.)
        lookup = [dict(package='bank', asset='a')]
        with self.assertRaisesRegex(ValueError, 'contradiction'):
            reconcile(dict(retained=[row]), lookup)
        row['tempo'] = native_tempo(2., 1.)
        with self.assertRaisesRegex(ValueError, 'duplicate retained'):
            reconcile(dict(retained=[row, row]), lookup)


if __name__ == '__main__':
    unittest.main()
