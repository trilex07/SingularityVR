import struct
import sys
import unittest
from pathlib import Path
sys.path.insert(0, str(Path(__file__).resolve().parents[1]/'audio'))
from prepare_quiet_ui import attenuate


def wave(samples):
    pcm = struct.pack('<'+'h'*len(samples), *samples)
    chunks = b'fmt '+struct.pack('<IHHIIHH', 16, 1, 1, 24000, 48000, 2, 16)
    chunks += b'JUNK'+struct.pack('<I', 3)+b'abc\0'
    chunks += b'data'+struct.pack('<I', len(pcm))+pcm
    return b'RIFF'+struct.pack('<I', len(chunks)+4)+b'WAVE'+chunks


class Tests(unittest.TestCase):
    def test_gain_timing_identity(self):
        data = wave([-32768, -100, -1, 0, 1, 100, 32767])
        out = attenuate(data)
        self.assertEqual(out[:-14], data[:-14]) # format, optional chunks, frame count unchanged
        self.assertEqual(struct.unpack('<7h', out[-14:]), (-11468, -35, 0, 0, 0, 35, 11468))
        self.assertEqual(attenuate(data, 100), data)
        self.assertEqual(attenuate(data, 0)[-14:], b'\0'*14)

    def test_refuse_unknown_or_malformed(self):
        data = wave([1, 2, 3])
        for bad in (data[:-1], data+b'x', b'', data.replace(b'WAVE', b'MP3!'), data.replace(b'data', b'xxxx')):
            with self.assertRaises(ValueError): attenuate(bad)
        with self.assertRaises(ValueError): attenuate(data, 101)
        with self.assertRaises(ValueError): attenuate(data, -1)


if __name__ == '__main__': unittest.main()
