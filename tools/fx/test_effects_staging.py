import importlib.util
import io
import struct
import tempfile
import unittest
from pathlib import Path
from unittest.mock import patch

from PIL import Image

SPEC = importlib.util.spec_from_file_location("effects", Path(__file__).resolve().parents[1] / "build-2k-effects.py")
fx = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(fx)
TEST_TEMP = fx.ROOT / "build" / "effects-review" / "tests"
TEST_TEMP.mkdir(parents=True, exist_ok=True)


class EffectsStagingTests(unittest.TestCase):
    def test_formats_rectangles_full_mips(self):
        for fmt in ("DXT1", "DXT5"):
            for size in ((32, 64), (4, 4), (1, 1)):
                raw = fx.build_dds(Image.new("RGBA", size, (90, 100, 110, 96)), fmt)
                self.assertEqual(fx.dds_validate(raw)["mips"], max(size).bit_length())
                self.assertEqual(struct.unpack_from("<I", raw, 20)[0], fx.mip_size(*size, fmt))

    def test_bad_header_rejected(self):
        raw = bytearray(fx.build_dds(Image.new("RGBA", (8, 8)), "DXT5"))
        for offset, value in ((8, 0x2100F), (20, 32), (28, 0), (76, 0), (108, 0)):
            bad = bytearray(raw)
            struct.pack_into("<I", bad, offset, value)
            with self.assertRaises(ValueError):
                fx.dds_validate(bad)
        with self.assertRaises(ValueError):
            fx.dds_validate(raw[:-1])

    def test_dxt5_alpha_decodes(self):
        raw = fx.build_dds(Image.new("RGBA", (16, 16), (240, 50, 10, 80)), "DXT5")
        with Image.open(io.BytesIO(raw)) as image:
            self.assertLess(abs(image.convert("RGBA").getpixel((2, 2))[3] - 80), 4)

    def test_fails_before_writes_missing_original_or_source(self):
        with tempfile.TemporaryDirectory(dir=TEST_TEMP) as temp:
            root = Path(temp)
            with patch.object(fx, "ROOT", root), patch.object(fx, "targets", return_value={"explosion": (4, 4, "DXT5", 42)}):
                out = root / "build" / "candidate"
                with self.assertRaisesRegex(ValueError, "identity-matching"):
                    fx.prepare(root, root, out, ("explosion",))
                self.assertFalse(out.exists())

    def test_output_boundaries_and_no_overwrite(self):
        with tempfile.TemporaryDirectory(dir=TEST_TEMP) as temp:
            root = Path(temp)
            with patch.object(fx, "ROOT", root):
                for out in (root, root / "build", root / "outside"):
                    with self.assertRaises(ValueError):
                        fx.prepare(root, root, out)
                out = root / "build" / "old"
                out.mkdir(parents=True)
                with self.assertRaisesRegex(ValueError, "already exists"):
                    fx.prepare(root, root, out)

    def test_registry_coverage_and_exclusions(self):
        self.assertTrue(set(fx.NAMES).issubset(fx.targets()))
        self.assertFalse(set(("tracer2", "smoketrail_01", "bulletImpactFlesh_d")) & set(fx.NAMES))

    def test_missing_enhancement_and_bad_aspect_are_fatal(self):
        with tempfile.TemporaryDirectory(dir=TEST_TEMP) as temp:
            root = Path(temp)
            raw = fx.build_dds(Image.new("RGBA", (4, 4), (12, 40, 90, 80)), "DXT5")
            (root / "explosion.dds").write_bytes(raw)
            identity = (4, 4, "DXT5", fx.fnv64(raw[128:144]))
            with patch.object(fx, "ROOT", root), patch.object(fx, "targets", return_value={"explosion": identity}):
                out = root / "build" / "candidate"
                with self.assertRaisesRegex(ValueError, "enhanced source missing"):
                    fx.prepare(root, root, out, ("explosion",))
                Image.new("RGBA", (4, 8)).save(root / "explosion.dds.8K.png")
                with self.assertRaisesRegex(ValueError, "aspect ratio"):
                    fx.prepare(root, root, out, ("explosion",))
                self.assertFalse(out.exists())


if __name__ == "__main__":
    unittest.main()
