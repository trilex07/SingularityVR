"""Real codec regression: preserve the installed bank's channel/frame contract."""
import math
import os
from pathlib import Path
import struct
import subprocess
import tempfile
import unittest
import wave

from native_voice_fit import fit
from vorbis_fixed_allocation import audio_packets, pages


class NativeVoiceChannels(unittest.TestCase):
    def test_short_single_eos_page_recovery_preserves_compressed_speech(self):
        ffmpeg = os.environ.get('SINGULARITYVR_FFMPEG')
        if not ffmpeg:
            self.skipTest('SINGULARITYVR_FFMPEG required for real codec test')
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp); source = root/'voice.wav'
            with wave.open(str(source), 'wb') as w:
                w.setparams((1, 2, 24000, 0, 'NONE', 'not compressed'))
                w.writeframes(b''.join(struct.pack('<h', int(6000*math.sin(i*.13))) for i in range(6000)))
            def run(args):
                return subprocess.run([ffmpeg, '-hide_banner', '-nostats', *args],
                                      check=True, capture_output=True, text=True).stderr
            for rate, frames in ((48000,37745), (44100,28156), (44100,42183)):
                folder = root/str(frames); folder.mkdir()
                result, receipt = fit(run, source, folder, 'voice', rate, frames)
                self.assertIsNotNone(result, receipt)
                self.assertTrue(receipt['repacketized_single_page'])
                self.assertEqual(receipt['measured_tail_padding_frames'], 0)
                old = (folder/'voice.ogg').read_bytes(); new = result.read_bytes()
                self.assertEqual(audio_packets(old), audio_packets(new))
                self.assertGreater(len(pages(new)), len(pages(old)))
                for decoder in ('vorbis', 'libvorbis'):
                    decoded = folder/(decoder+'.wav')
                    run(['-v','error','-n','-c:a',decoder,'-i',str(result),'-c:a','pcm_s16le',str(decoded)])
                    with wave.open(str(decoded)) as w:
                        self.assertEqual((w.getnframes(), w.getframerate(), w.getnchannels()), (frames,rate,1))

    def test_mono_and_stereo_keep_duration_and_native_layout(self):
        ffmpeg = os.environ.get('SINGULARITYVR_FFMPEG')
        if not ffmpeg:
            self.skipTest('SINGULARITYVR_FFMPEG required for real codec test')
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            source = root / 'voice.wav'
            with wave.open(str(source), 'wb') as out:
                out.setparams((1, 2, 24000, 0, 'NONE', 'not compressed'))
                out.writeframes(b''.join(struct.pack('<h', int(6000 * math.sin(i * .13)))
                                        for i in range(6000)))
            def run(args):
                return subprocess.run([ffmpeg, '-hide_banner', '-nostats', *args],
                                      check=True, capture_output=True, text=True).stderr
            for channels in (1, 2):
                folder = root / str(channels)
                folder.mkdir()
                result, receipt = fit(run, source, folder, 'wave', 48000, 48000,
                                      channels=channels)
                self.assertIsNotNone(result, receipt)
                decoded_path = folder / 'accepted-result.wav'
                run(['-v','error','-n','-i',str(result),'-c:a','pcm_s16le',str(decoded_path)])
                with wave.open(str(decoded_path)) as decoded:
                    self.assertEqual(decoded.getnchannels(), channels)
                    self.assertEqual(decoded.getframerate(), 48000)
                    frames = decoded.getnframes()
                self.assertEqual(frames, 48000)
                self.assertEqual(receipt['channels'], channels)
                self.assertLessEqual(receipt['measured_tail_padding_frames'], 256)
            with self.assertRaises(ValueError):
                fit(run, source, root, 'invalid', 48000, 48000, channels=0)


if __name__ == '__main__':
    unittest.main()

