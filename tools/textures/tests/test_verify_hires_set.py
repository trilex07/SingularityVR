import hashlib
import tempfile
import unittest
from pathlib import Path
import sys

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
from verify_hires_set import verify, dds_header


def sample_dds(mips=3):
    data = bytearray(128 + 8 * mips)
    data[:4] = b"DDS "
    data[12:16] = (4).to_bytes(4, "little")
    data[16:20] = (4).to_bytes(4, "little")
    data[28:32] = mips.to_bytes(4, "little")
    data[84:88] = b"DXT1"
    return bytes(data)


class VerifyHiresSet(unittest.TestCase):
    def test_layout_and_identity(self):
        with tempfile.TemporaryDirectory(dir=Path(__file__).resolve().parent) as tmp:
            stage = Path(tmp)
            payload = sample_dds()
            (stage / "sample.dds").write_bytes(payload)
            manifest = {"maps": {"sample": {"status": "staged", "source_layout": [4, 4],
                       "source_fnv64": 123, "width": 4, "height": 4, "fourcc": "DXT1",
                       "sha256": hashlib.sha256(payload).hexdigest()}}}
            targets = '{ "sample", 4u, 4u, D3DFMT_DXT1, 123ULL, "sample.dds"'
            self.assertIn("sample", verify(manifest, stage, targets))
            with self.assertRaisesRegex(ValueError, "target identity"):
                verify(manifest, stage, targets.replace("123ULL", "124ULL"))
            (stage / "sample.dds").write_bytes(payload[:-8])
            with self.assertRaisesRegex(ValueError, "hash mismatch"):
                verify(manifest, stage, targets)

    def test_incomplete_mip_chain_rejected(self):
        with tempfile.TemporaryDirectory(dir=Path(__file__).resolve().parent) as tmp:
            path = Path(tmp) / "a.dds"
            path.write_bytes(sample_dds(mips=2))
            with self.assertRaisesRegex(ValueError, "incomplete mip"):
                dds_header(path)


if __name__ == "__main__":
    unittest.main()
