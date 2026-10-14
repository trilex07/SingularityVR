"""Synthetic loudness guard tests, optional original-data graph check; no Steam."""
import argparse
import json
import sys
import tempfile
import unittest
import wave
from pathlib import Path
sys.path.insert(0, str(Path(__file__).parents[1] / 'audio'))
from prepare_radio_loudness import match, measure
from inspect_radio_conversations import conversations


class LoudnessTests(unittest.TestCase):
    def fixture(self, peaks=-1.2, loudness=-13.5, frames=48000):
        owned = (Path(__file__).parents[2]/'deployment/hands-dubbing').resolve()
        temp = tempfile.TemporaryDirectory(prefix='radio-synthetic-', dir=owned)
        assert Path(temp.name).resolve().is_relative_to(owned)
        self.addCleanup(temp.cleanup)
        root = Path(temp.name); source = root/'source.ogg'; native = root/'native.ogg'
        source.write_bytes(b'fixture'); native.write_bytes(b'fixture')
        calls = []
        def run(args):
            calls.append(args)
            if 'print_format=json' in ' '.join(args):
                value = -13.0 if str(native) in args else -23.0 if str(source) in args else loudness
                return json.dumps(dict(input_i=value, input_tp=peaks))
            dest = Path(args[-1])
            if dest.suffix == '.wav':
                with wave.open(str(dest), 'wb') as w:
                    w.setnchannels(1); w.setsampwidth(2); w.setframerate(48000)
                    w.writeframes(b'\0\0'*frames)
            else: dest.write_bytes(b'encoded-fixture')
            return ''
        return root, source, native, calls, run

    def test_match_and_exact_budget(self):
        root, source, native, calls, run = self.fixture()
        result, proof = match(run, source, native, root, 'test', 48000, 48000)
        self.assertTrue(result.exists()); self.assertEqual(proof['attempts'], 1)
        encode = next(a for a in calls if 'libvorbis' in a)
        self.assertEqual(encode[encode.index('-i')+1], str(source))
        self.assertIn('latency=1', encode[encode.index('-af')+1])
        self.assertIn('whole_len=48000', encode[encode.index('-af')+1])

    def test_peak_rejected_before_target(self):
        root, source, native, calls, run = self.fixture(peaks=0.1)
        with self.assertRaisesRegex(ValueError, 'true peak'):
            match(run, source, native, root, 'test', 48000, 48000)
        self.assertFalse((root/'test.ogg').exists())

    def test_decode_budget_refused(self):
        root, source, native, calls, run = self.fixture(frames=47999)
        with self.assertRaisesRegex(ValueError, 'PCM budget'):
            match(run, source, native, root, 'test', 48000, 48000)

    def test_bounded_failure_no_target(self):
        root, source, native, calls, run = self.fixture(loudness=-14.01)
        with self.assertRaisesRegex(ValueError, 'bounded'):
            match(run, source, native, root, 'test', 48000, 48000)
        self.assertEqual(sum('libvorbis' in a for a in calls), 8)
        self.assertFalse((root/'test.ogg').exists())

    def test_silent_measure_refused(self):
        with self.assertRaisesRegex(ValueError, 'silent'):
            measure(lambda args: '{"input_i":"-inf","input_tp":"-inf"}', Path('fixture'))


if __name__ == '__main__':
    parser = argparse.ArgumentParser(); parser.add_argument('--logical-radio', type=Path)
    args = parser.parse_args()
    result = unittest.TextTestRunner().run(unittest.defaultTestLoader.loadTestsFromTestCase(LoudnessTests))
    if not result.wasSuccessful(): sys.exit(1)
    if args.logical_radio:
        graph = conversations(args.logical_radio)
        waves = {w['name'] for node in graph for line in node['conversation'] for w in line['waves']
                 if w.get('class_name') == 'SoundNodeWave'}
        assert len(graph) == 3 and len(waves) == 17
        target = [node for node in graph if any(w['name'] == 'VI1_1005_GST'
                  for row in node['conversation'] for w in row['waves'])]
        assert len(target) == 1 and len(target[0]['conversation']) == 1
        print('ORIGINAL DATA PASS: three radio conversations, seventeen typed wave imports, exact single-line target')
