#!/usr/bin/env python3
"""Preview: health plate on left wrist (Katorga variant B)."""
from __future__ import annotations

from pathlib import Path

import katorga_kit
from PIL import Image, ImageDraw

ROOT = Path(__file__).resolve().parents[2]
OUT = ROOT / "assets" / "hud" / "katorga" / "previews" / "health-wrist-left.png"


def main() -> None:
    katorga_kit.write_all()
    w, h = 960, 720
    bg = (18, 20, 22, 255)
    img = Image.new("RGBA", (w, h), bg)
    draw = ImageDraw.Draw(img)
    font = katorga_kit._font(18)
    draw.text(
        (32, 24),
        "Lewa ręka (nie celująca) — płyta zdrowia na nadgarstku · wariant B Katorga",
        fill=katorga_kit.STEEL_RGB,
        font=font,
    )
    draw.text(
        (32, 52),
        "10 kresek + liczba Health/HealthMax · przy małym HP: 2 kreski + MAŁO (bez samego koloru)",
        fill=(141, 151, 161, 255),
        font=katorga_kit._font(14),
    )
    # Seated VR: plate low, left side of view circle
    draw.ellipse([w // 2 - 280, 120, w // 2 + 280, 680], outline=(90, 111, 141, 120), width=2)
    katorga_kit._plate_pil(img, (120, 420), "health", "full", 0.55, "10/10", medial="right")
    katorga_kit._plate_pil(img, (520, 420), "health", "low", 0.55, "2/10", medial="right")
    draw.text((120, 395), "Spokój", fill=katorga_kit.PAPER_RGB, font=katorga_kit._font(15, True))
    draw.text((520, 395), "Mało życia", fill=katorga_kit.AMBER_RGB, font=katorga_kit._font(15, True))
    OUT.parent.mkdir(parents=True, exist_ok=True)
    img.convert("RGB").save(OUT, quality=95)
    print("wrote", OUT)


if __name__ == "__main__":
    main()
