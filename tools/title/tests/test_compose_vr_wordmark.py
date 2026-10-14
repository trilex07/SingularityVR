"""Offline structural test for VR wordmark compose (no game assets in repo)."""
from __future__ import annotations

import tempfile
import unittest
from pathlib import Path

from PIL import Image, ImageDraw

ROOT = Path(__file__).resolve().parents[3]
sys_path = Path(__file__).resolve().parents[1]
import sys

sys.path.insert(0, str(sys_path))
from compose_vr_wordmark import compose  # noqa: E402


class ComposeVrWordmarkTest(unittest.TestCase):
    def test_compose_extends_width_without_generic_font(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            stock = Path(tmp) / "stock.png"
            img = Image.new("RGBA", (512, 128), (0, 0, 0, 255))
            draw = ImageDraw.Draw(img)
            x = 40
            for _ in range(11):
                draw.rectangle((x, 40, x + 30, 88), fill=(255, 255, 255, 255))
                x += 34
            img.save(stock)
            master, boot, report = compose(stock)
            self.assertEqual(master.size, (512, 128))
            self.assertEqual(boot.size, (1280, 720))
            self.assertLessEqual(report["scale_to_fit_vr"], 1.0)
            self.assertIn("cloned_from_stock_strip", report["glyph_source"])


if __name__ == "__main__":
    unittest.main()
