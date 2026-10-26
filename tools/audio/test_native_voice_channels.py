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


class NativeVoiceChannels(unittest.TestCase):
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
                decoded_path = folder / ('wave.timing-retry.wav' if receipt['measured_tail_padding_frames'] else 'wave.decoded.wav')
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

