"""Synthetic-only provenance and native timing unit checks."""
import hashlib
import tempfile
import unittest
from pathlib import Path
from unittest.mock import patch
import prepare_timed_voice_manifest as target
from generate_native_dialogue import checked_segments
from export_native_voice_handoff import bind, selected_identity
from fit_timed_voice_candidates import guarded_silence_trim


class InventoryTests(unittest.TestCase):
    def fixture(self, pcm=48000, channels=1):
        return dict(exports=[dict(class_name='SoundNodeWave',name='SYNTHETIC_LINE',
            properties=[dict(name='SpokenText',value='Synthetic test.',value_offset=2,size=4),
                dict(name='SampleRate',value=24000),dict(name='RawPCMDataSize',value=pcm),
                dict(name='NumChannels',value=channels)])])

    def test_identity_and_budget(self):
        with tempfile.TemporaryDirectory(dir=Path.cwd()) as root:
            source = Path(root)/'SYNTHETIC_BANK.u'
            source.write_bytes(b'abcdefgh')
            with patch.object(target,'inspect',return_value=self.fixture()):
                row = target.inventory(source)[0]
            self.assertEqual(row['line_id'],'SYNTHETIC_BANK/SoundNodeWave/SYNTHETIC_LINE.ogg')
            self.assertEqual(row['native_frames'],24000)
            self.assertEqual(row['native_duration'],1.0)
            self.assertEqual(row['source_property_sha256'],hashlib.sha256(b'cdef').hexdigest())
            self.assertEqual(source.read_bytes(),b'abcdefgh')

    def test_invalid_budget_refused(self):
        with tempfile.TemporaryDirectory(dir=Path.cwd()) as root:
            source = Path(root)/'SYNTHETIC_BANK.u'
            source.write_bytes(b'abcdefgh')
            for pcm,channels in ((1,1),(-2,1),(48000,2)):
                with patch.object(target,'inspect',return_value=self.fixture(pcm,channels)):
                    with self.assertRaises(ValueError):
                        target.inventory(source)

    def test_segments_preserve_all_words(self):
        self.assertEqual(checked_segments('Synthetic one. Synthetic two.',
            ['Synthetic one.','Synthetic two.']),['Synthetic one.','Synthetic two.'])

    def test_segments_refuse_omission_and_empty(self):
        for segments in ([], ['Synthetic one.'], ['Synthetic one.','']):
            with self.assertRaises(ValueError):
                checked_segments('Synthetic one. Synthetic two.',segments)

    def test_binding_refuses_wrong_text_and_budget(self):
        selected = dict(asset='SYNTHETIC',source_english='Synthetic.',native_frames=24000,native_rate=24000)
        for native in (dict(selected,source_english='Different.'),dict(selected,native_frames=48000)):
            with self.assertRaises(ValueError):
                bind(selected,native)

    def test_selected_identity_not_historical_input_hash(self):
        import wave
        with tempfile.TemporaryDirectory(dir=Path.cwd()) as root:
            source = Path(root)/'synthetic.wav'
            with wave.open(str(source),'wb') as wav:
                wav.setparams((1,2,24000,0,'NONE','not compressed'))
                wav.writeframes(b'\1\0'*2400)
            actual = hashlib.sha256(source.read_bytes()).hexdigest()
            row = dict(asset='SYNTHETIC',file=str(source),sha256=actual,native_frames=24000,
                       native_rate=24000,candidate_wav_sha256='historical_not_selected')
            exported = selected_identity(row)
            self.assertEqual(exported['selected_wav_sha256'],actual)
            self.assertNotIn('candidate_wav_sha256',exported)
            self.assertEqual(exported['historical_input_hashes']['candidate_wav_sha256'],'historical_not_selected')
            with self.assertRaises(ValueError):
                selected_identity(dict(row,sha256='wrong'))

    def test_guarded_trim_is_one_contiguous_slice(self):
        import array
        import wave
        with tempfile.TemporaryDirectory(dir=Path.cwd()) as root:
            source,target = Path(root)/'source.wav',Path(root)/'trim.wav'
            audio = array.array('h',[0]*5000)
            audio[2800] = 10  # quiet edge sample below threshold, inside guard
            audio[3000] = 1000
            audio[3200] = 1000
            with wave.open(str(source),'wb') as wav:
                wav.setparams((1,2,24000,0,'NONE','not compressed'))
                wav.writeframes(audio.tobytes())
            original = source.read_bytes()
            evidence = guarded_silence_trim(source,target,40)
            self.assertEqual((evidence['first_source_sample'],evidence['last_source_sample']),(2040,4161))
            with wave.open(str(target)) as wav:
                self.assertEqual(wav.readframes(wav.getnframes()),audio[2040:4161].tobytes())
            self.assertEqual(source.read_bytes(),original)


if __name__ == '__main__':
    unittest.main()
