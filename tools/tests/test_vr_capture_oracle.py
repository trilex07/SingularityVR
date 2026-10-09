import hashlib
import json
import struct
import subprocess
import sys
import tempfile
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
ORACLE = ROOT / "tools" / "vr_capture_oracle.py"
TEST_TMP = ROOT / ".test-tmp-vr-capture"
TEST_TMP.mkdir(exist_ok=True)


def bmp(path: Path, width: int, height: int, fn):
    row = ((width * 3 + 3) // 4) * 4
    pixels = bytearray()
    for y in range(height - 1, -1, -1):
        line = bytearray()
        for x in range(width):
            r, g, b = fn(x, y)
            line += bytes((b, g, r))
        line += b"\0" * (row - width * 3)
        pixels += line
    size = 54 + len(pixels)
    header = b"BM" + struct.pack("<IHHI", size, 0, 0, 54)
    dib = struct.pack("<IiiHHIIiiII", 40, width, height, 1, 24, 0, len(pixels), 2835, 2835, 0, 0)
    path.write_bytes(header + dib + pixels)


def manifest(path, run, frame, seq, left, right, scenario="synthetic", source="xr_projection_layer", pid=101):
    def entry(eye, image):
        return {"eye": eye, "path": image.name, "sha256": hashlib.sha256(image.read_bytes()).hexdigest().upper(), "cmdSeq": seq, "frameIndex": frame, "captureSeq": seq, "source": source}
    data = {"schema": "singularityvr.capture.v1", "runId": run, "scenario": scenario, "cmdSeq": seq, "frameIndex": frame, "captureSeq": seq, "pid": pid, "dllSha256": "A" * 64, "source": source, "stereoOrderKnown": True, "eyes": [entry("L", left), entry("R", right)]}
    path.write_text(json.dumps(data), encoding="utf-8")


def invoke(*args):
    return subprocess.run([sys.executable, str(ORACLE), *map(str, args)], text=True, capture_output=True)


class CaptureOracleTests(unittest.TestCase):
    def test_synthetic_pass(self):
        with tempfile.TemporaryDirectory(dir=TEST_TMP) as td:
            d = Path(td); l = d / "left.bmp"; r = d / "right.bmp"; m = d / "capture.json"
            bmp(l, 8, 8, lambda x, y: (255, 30, 30) if x < 4 else (20, 20, 20))
            bmp(r, 8, 8, lambda x, y: (30, 255, 30) if x >= 4 else (20, 20, 20))
            manifest(m, "run-a", 10, 20, l, r)
            self.assertEqual(invoke(m).returncode, 0)

    def test_stale_capture_fails(self):
        with tempfile.TemporaryDirectory(dir=TEST_TMP) as td:
            d = Path(td); l = d / "left.bmp"; r = d / "right.bmp"; old = d / "old.json"; cur = d / "cur.json"
            bmp(l, 4, 4, lambda x, y: (255, 0, 0)); bmp(r, 4, 4, lambda x, y: (0, 255, 0))
            manifest(old, "run-a", 10, 20, l, r); manifest(cur, "run-a", 10, 20, l, r)
            self.assertEqual(invoke(cur, "--previous", old).returncode, 2)

    def test_swapped_eyes_fails_against_baseline(self):
        with tempfile.TemporaryDirectory(dir=TEST_TMP) as td:
            d = Path(td); bl = d / "base-l.bmp"; br = d / "base-r.bmp"; cl = d / "cur-l.bmp"; cr = d / "cur-r.bmp"; bm = d / "base.json"; cm = d / "cur.json"
            bmp(bl, 4, 4, lambda x, y: (255, 20, 20)); bmp(br, 4, 4, lambda x, y: (20, 255, 20))
            bmp(cl, 4, 4, lambda x, y: (20, 255, 20)); bmp(cr, 4, 4, lambda x, y: (255, 20, 20))
            manifest(bm, "base", 1, 1, bl, br); manifest(cm, "cur", 2, 2, cl, cr)
            self.assertEqual(invoke(cm, "--baseline", bm).returncode, 2)

    def test_missing_effect_fails_with_trusted_baseline(self):
        with tempfile.TemporaryDirectory(dir=TEST_TMP) as td:
            d = Path(td); bl = d / "base-l.bmp"; br = d / "base-r.bmp"; cl = d / "cur-l.bmp"; cr = d / "cur-r.bmp"; bm = d / "base.json"; cm = d / "cur.json"
            bmp(bl, 4, 4, lambda x, y: (255, 255, 255) if x < 2 else (20, 20, 20)); bmp(br, 4, 4, lambda x, y: (255, 255, 255) if x < 2 else (20, 20, 20))
            bmp(cl, 4, 4, lambda x, y: (20, 20, 20)); bmp(cr, 4, 4, lambda x, y: (20, 20, 20))
            manifest(bm, "base", 1, 1, bl, br); manifest(cm, "cur", 2, 2, cl, cr)
            self.assertEqual(invoke(cm, "--baseline", bm, "--roi", "effect=0,0,2,4", "--effect-roi", "effect").returncode, 2)

    def test_unknown_baseline_is_inconclusive(self):
        with tempfile.TemporaryDirectory(dir=TEST_TMP) as td:
            d = Path(td); l = d / "l.bmp"; r = d / "r.bmp"; m = d / "m.json"
            bmp(l, 4, 4, lambda x, y: (100, 100, 100)); bmp(r, 4, 4, lambda x, y: (110, 110, 110)); manifest(m, "run", 1, 1, l, r)
            self.assertEqual(invoke(m, "--roi", "effect=0,0,4,4", "--effect-roi", "effect").returncode, 3)


if __name__ == "__main__":
    unittest.main()
