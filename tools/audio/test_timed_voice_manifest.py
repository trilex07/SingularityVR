"""Synthetic-only provenance and native timing unit checks."""
import hashlib
import tempfile
import unittest
from pathlib import Path
from unittest.mock import patch
import prepare_timed_voice_manifest as target
from generate_native_dialogue import checked_segments


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


if __name__ == '__main__':
    unittest.main()
