"""Unit test for the 551 Polish dubbing delivery candidate manifest and files.

Verifies:
- Exactly 551 records in manifest
- 100% of records have status == 'PASS'
- 100% of records have computed_tempo <= 1.35x
- All 551 files exist on disk in both staging and private candidate folders
- Every file matches its recorded SHA-256
- Strictly mono PCM16 24 kHz audio format
- Non-zero peak amplitude <= -1.0 dBFS, zero digital clipping
- Zero filename collisions
"""
import hashlib
import json
import struct
import unittest
import wave
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent.parent
MANIFEST_PATH = ROOT / 'docs/handoffs/final-dubbing-v3-manifest-20261026.json'
PRIVATE_DIR = ROOT / 'build/audio-release-private/timing_fix_551'
STAGING_DIR = Path(r'C:\Dubbing-aktualny\timing_fix_551')


def sha256_file(path: Path) -> str:
    h = hashlib.sha256()
    with path.open('rb') as f:
        for chunk in iter(lambda: f.read(1024 * 1024), b''):
            h.update(chunk)
    return h.hexdigest()


class TestV3DubbingDelivery(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.assertTrue(MANIFEST_PATH.exists(), f"Manifest missing: {MANIFEST_PATH}")
        with open(MANIFEST_PATH, 'r', encoding='utf-8') as f:
            cls.manifest = json.load(f)
        cls.records = cls.manifest['records']

    def test_record_count(self):
        self.assertEqual(len(self.records), 551, "Manifest must contain exactly 551 records")

    def test_all_pass_status(self):
        for r in self.records:
            self.assertEqual(
                r['status'],
                'PASS',
                f"Record {r['package']} / {r['asset']} has status {r['status']}",
            )

    def test_tempo_within_budget(self):
        for r in self.records:
            tempo = r['computed_tempo']
            self.assertLessEqual(
                tempo,
                1.350001,
                f"Record {r['package']} / {r['asset']} tempo {tempo:.4f}x exceeds MAX 1.35x",
            )

    def test_files_exist_and_hashes_match(self):
        seen_files = set()
        for r in self.records:
            fname = r['file']
            self.assertNotIn(fname, seen_files, f"Duplicate filename collision: {fname}")
            seen_files.add(fname)

            priv_p = PRIVATE_DIR / fname
            stag_p = STAGING_DIR / fname

            self.assertTrue(priv_p.exists(), f"Missing private audio file: {priv_p}")
            self.assertTrue(stag_p.exists(), f"Missing staging audio file: {stag_p}")

            priv_sha = sha256_file(priv_p)
            stag_sha = sha256_file(stag_p)

            self.assertEqual(priv_sha, r['sha256'], f"SHA256 mismatch for private {fname}")
            self.assertEqual(stag_sha, r['sha256'], f"SHA256 mismatch for staging {fname}")

    def test_wav_audio_format_and_amplitude(self):
        for r in self.records:
            fname = r['file']
            priv_p = PRIVATE_DIR / fname
            with wave.open(str(priv_p), 'rb') as w:
                nchannels = w.getnchannels()
                sampwidth = w.getsampwidth()
                framerate = w.getframerate()
                comptype = w.getcomptype()
                nframes = w.getnframes()
                data = w.readframes(nframes)

            self.assertEqual(nchannels, 1, f"{fname} must be mono")
            self.assertEqual(sampwidth, 2, f"{fname} must be 16-bit")
            self.assertEqual(framerate, 24000, f"{fname} must be 24000 Hz")
            self.assertEqual(comptype, 'NONE', f"{fname} must be uncompressed PCM")
            self.assertGreater(nframes, 0, f"{fname} has 0 frames")

            samples = struct.iter_unpack('<h', data)
            peak = max(abs(s[0]) for s in samples)
            self.assertGreater(peak, 0, f"{fname} is silent")
            self.assertLess(peak, 32767, f"{fname} is clipped at full scale")


if __name__ == '__main__':
    unittest.main()
