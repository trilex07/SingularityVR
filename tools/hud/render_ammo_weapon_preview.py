#!/usr/bin/env python3
"""Preview: ammo plate on right aim hand / weapon side (Katorga variant B)."""
from __future__ import annotations

from pathlib import Path

import katorga_kit
from PIL import Image, ImageDraw

ROOT = Path(__file__).resolve().parents[2]
OUT = ROOT / "assets" / "hud" / "katorga" / "previews" / "ammo-weapon-right.png"


def main() -> None:
    katorga_kit.write_all()
    w, h = 960, 720
    img = Image.new("RGBA", (w, h), (18, 20, 22, 255))
    draw = ImageDraw.Draw(img)
    draw.text(
        (32, 24),
        "Prawa ręka (celująca) — płyta amunicji przy broni / slide · wariant B Katorga",
        fill=katorga_kit.STEEL_RGB,
        font=katorga_kit._font(18),
    )
    draw.text(
        (32, 52),
        "Mag + rezerwa · komora zamknięta/otwarta · pusty mag: PRZEŁADUJ · zmiana broni: krótka nazwa",
        fill=(141, 151, 161, 255),
        font=katorga_kit._font(14),
    )
    draw.ellipse([w // 2 - 280, 120, w // 2 + 280, 680], outline=(90, 111, 141, 120), width=2)

    # Right side, medial notch toward body (left edge of plate)
    katorga_kit._plate_pil(img, (380, 400), "ammo", "template", 0.52, "12/48", medial="left")
    katorga_kit._plate_pil(img, (80, 400), "ammo", "empty", 0.52, "0/20", medial="left")
    katorga_kit._plate_pil(
        img, (230, 400), "ammo", "switch", 0.52, "12/48", name="Centurion", medial="left"
    )

    draw.text((380, 375), "Spokój", fill=katorga_kit.PAPER_RGB, font=katorga_kit._font(15, True))
    draw.text((80, 375), "Pusty magazynek", fill=katorga_kit.AMBER_RGB, font=katorga_kit._font(15, True))
    draw.text((230, 375), "Zmiana broni", fill=katorga_kit.STEEL_RGB, font=katorga_kit._font(15, True))

    OUT.parent.mkdir(parents=True, exist_ok=True)
    img.convert("RGB").save(OUT, quality=95)
    print("wrote", OUT)


if __name__ == "__main__":
    main()
