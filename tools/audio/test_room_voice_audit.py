"""Synthetic-only room bank/hash/timing checks; no original game fixtures."""
import hashlib
import tempfile
import unittest
import wave
from pathlib import Path
from unittest.mock import patch
import prepare_room_voice_audit as target
from assemble_room_voice_handoff import bind_voice
from generate_native_dialogue import checked_segments


class RoomAuditTests(unittest.TestCase):
    def props(self,pcm=96000,rate=48000):
        return dict(exports=[dict(class_name='SoundNodeWave',name='SYNTHETIC',properties=[
            dict(name='SpokenText',value='Synthetic source.',value_offset=2,size=4),
            dict(name='SampleRate',value=rate),dict(name='RawPCMDataSize',value=pcm),
            dict(name='NumChannels',value=1)])])

    def test_exact_native_pin_and_property_span(self):
        with tempfile.TemporaryDirectory(dir=Path.cwd()) as root:
            source,logical = Path(root)/'SOURCE.xxx',Path(root)/'SYNTHETIC_BANK.u'
            source.write_bytes(b'original-source')
            logical.write_bytes(b'abcdefgh')
            pin = dict(source_sha256=target.sha(source),logical_sha256=target.sha(logical))
            with patch.object(target,'inspect',return_value=self.props()):
                row = target.inventory(logical,source,pin)[0]
            self.assertEqual(row['native_frames'],48000)
            self.assertEqual(row['source_property_sha256'],hashlib.sha256(b'cdef').hexdigest())
            self.assertEqual(source.read_bytes(),b'original-source')
            self.assertEqual(logical.read_bytes(),b'abcdefgh')
            with self.assertRaises(ValueError):
                target.inventory(logical,source,dict(pin,source_sha256='incorrect'))

    def test_odd_pcm_budget_refused(self):
        with tempfile.TemporaryDirectory(dir=Path.cwd()) as root:
            source,logical = Path(root)/'SOURCE.xxx',Path(root)/'SYNTHETIC_BANK.u'
            source.write_bytes(b'original-source');logical.write_bytes(b'abcdefgh')
            pin = dict(source_sha256=target.sha(source),logical_sha256=target.sha(logical))
            with patch.object(target,'inspect',return_value=self.props(pcm=3)):
                with self.assertRaises(ValueError):
                    target.inventory(logical,source,pin)

    def test_reuse_sha_and_format(self):
        with tempfile.TemporaryDirectory(dir=Path.cwd()) as root:
            path = Path(root)/'synthetic.wav'
            with wave.open(str(path),'wb') as wav:
                wav.setparams((1,2,24000,0,'NONE','not compressed'));wav.writeframes(b'\1\0'*2400)
            actual,duration = target.verify_wave(path,target.sha(path))
            self.assertEqual(actual,target.sha(path));self.assertEqual(duration,.1)
            with self.assertRaises(ValueError):
                target.verify_wave(path,'wrong')

    def test_voice_binding_rejects_changed_source_and_timing(self):
        native = dict(asset='SYNTHETIC',source_english='Synthetic complete.',native_frames=48000,native_rate=48000)
        for voice in (dict(native,source_english='Different.'),dict(native,native_frames=24000)):
            with self.assertRaises(ValueError):
                bind_voice(native,voice)

    def test_segments_preserve_complete_translated_content(self):
        self.assertEqual(checked_segments('Synthetic one. Synthetic two.',
            ['Synthetic one.','Synthetic two.']),['Synthetic one.','Synthetic two.'])
        with self.assertRaises(ValueError):
            checked_segments('Synthetic one. Synthetic two.',['Synthetic one.'])


if __name__=='__main__':
    unittest.main()
