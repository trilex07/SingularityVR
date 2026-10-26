import math
from pathlib import Path
import struct
import tempfile
import unittest
import wave

from integrate_timing_delivery import sha, validate_rows, verify_wav


class TimingDeliveryTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.temp_base = Path(__file__).resolve().parents[2]/'build/audio-release-private/tests'
        cls.temp_base.mkdir(parents=True,exist_ok=True)

    def setUp(self):
        self.row = dict(package='SP_A_LOC_INT',asset='voice',file='one.wav',
                        text_en='Go.',native_duration=1.,guarded_duration=.25,
                        computed_tempo=1.,status='PASS',sha256='a'*64,tts_text='Idz.')

    def validate(self, rows, plan=None, blocked=None, lookup=None):
        plan = plan if plan is not None else [self.row]
        lookup = lookup if lookup is not None else {(r['package'].casefold(),r['asset']):r for r in plan}
        return validate_rows(dict(records=rows),dict(timing_cases=plan),
                             dict(blocked=blocked or []),lookup)

    def test_exact_identity_includes_package_not_only_asset(self):
        other=dict(self.row,package='SP_B_LOC_INT',file='two.wav')
        rows,blocked=self.validate([self.row,other],plan=[self.row,other])
        self.assertEqual(len(rows),2)
        self.assertFalse(blocked)

    def test_unknown_missing_duplicate_and_filename_alias_refused(self):
        for rows in ([],[self.row,self.row],[dict(self.row,asset='unknown')]):
            with self.assertRaises(ValueError): self.validate(rows)
        other=dict(self.row,package='SP_B_LOC_INT')
        with self.assertRaisesRegex(ValueError,'filename collision'):
            self.validate([self.row,other],plan=[self.row,other])

    def test_bad_path_and_mismatched_native_budget_or_lookup_refused(self):
        for changes in (dict(file='../one.wav'),dict(package='MP_A_LOC_INT'),
                        dict(native_duration=.2),dict(text_en='Stop.'),dict(sha256='bad')):
            with self.assertRaises(ValueError): self.validate([dict(self.row,**changes)])
        with self.assertRaisesRegex(ValueError,'lookup/filename'):
            self.validate([self.row],lookup={('sp_a_loc_int','voice'):dict(file='other.wav')})

    def test_content_refusal_bound_to_exact_take_not_just_asset_name(self):
        _,blocked=self.validate([self.row],blocked=[self.row])
        self.assertEqual(list(blocked),[('sp_a_loc_int','voice')])
        with self.assertRaisesRegex(ValueError,'review no longer matches'):
            self.validate([self.row],blocked=[dict(self.row,sha256='b'*64)])

    def wav(self, path, seconds=.25, rate=24000, amplitude=6000):
        with wave.open(str(path),'wb') as w:
            w.setparams((1,2,rate,0,'NONE','not compressed'))
            w.writeframes(b''.join(struct.pack('<h',round(amplitude*math.sin(i*.13)))
                                  for i in range(round(rate*seconds))))
        return dict(self.row,sha256=sha(path.read_bytes()),guarded_duration=seconds)

    def test_pcm_recomputed_not_trusting_side_pass(self):
        with tempfile.TemporaryDirectory(dir=self.temp_base) as temp:
            p=Path(temp);row=self.wav(p/'one.wav')
            self.assertEqual(verify_wav(row,p/'one.wav',p/'trim.wav')['tempo'],1.)
            row=self.wav(p/'long.wav',seconds=2.)
            row.update(computed_tempo=2/.98)
            with self.assertRaisesRegex(ValueError,'exceeds MAIN limit'):
                verify_wav(row,p/'long.wav',p/'long-trim.wav')

    def test_false_measured_duration_and_hash_refused(self):
        with tempfile.TemporaryDirectory(dir=self.temp_base) as temp:
            p=Path(temp);row=self.wav(p/'one.wav')
            for changes in (dict(sha256='b'*64),dict(guarded_duration=.1),dict(computed_tempo=1.3)):
                with self.assertRaises(ValueError):
                    verify_wav(dict(row,**changes),p/'one.wav',p/'trim.wav')

    def test_wrong_rate_silence_and_excess_peak_refused(self):
        with tempfile.TemporaryDirectory(dir=self.temp_base) as temp:
            p=Path(temp)
            for changes in (dict(rate=48000),dict(amplitude=0),dict(amplitude=32000)):
                row=self.wav(p/'one.wav',**changes)
                with self.assertRaises(ValueError): verify_wav(row,p/'one.wav',p/'trim.wav')

    def test_truncated_pcm_is_not_a_valid_take_even_with_matching_file_hash(self):
        with tempfile.TemporaryDirectory(dir=self.temp_base) as temp:
            p=Path(temp);source=p/'one.wav';row=self.wav(source)
            source.write_bytes(source.read_bytes()[:-10])
            row['sha256']=sha(source.read_bytes())
            with self.assertRaisesRegex(ValueError,'truncated delivery PCM'):
                verify_wav(row,source,p/'trim.wav')


if __name__ == '__main__':
    unittest.main()
