"""Deterministic offline stereo layout receipt; all numbers are synthetic.

This checks eye separation and layout readability. It is not a game capture.
"""
from __future__ import annotations
import argparse
from pathlib import Path
from PIL import Image, ImageDraw, ImageFont

def project(x: float, y: float, eye: float) -> tuple[int, int]:
    # OpenXR metre space: wrist centre (-.22, -.12, -.42), 64 mm IPD.
    wx = -.22 + (x - .5) * .155
    wy = -.12 + (.5 - y) * .104
    z = .42
    u = .5 + .5 * (wx - eye) / z
    v = .5 - .5 * wy / z
    return (round((0 if eye < 0 else 1920) + u * 1920), round(v * 2160))

def render(path: Path) -> None:
    im = Image.new("RGB", (3840, 2160), "#172027")
    d = ImageDraw.Draw(im)
    font = ImageFont.truetype("C:/Windows/Fonts/consola.ttf", 20)
    for eye in (-.032, .032):
        x0, y0 = project(0, 0, eye)
        x1, y1 = project(1, 1, eye)
        d.rectangle((x0, y0, x1, y1), fill="#213036", outline="#75817d", width=3)
        for sx in (x0 + 8, x1 - 13):
            for sy in (y0 + 8, y1 - 13):
                d.ellipse((sx, sy, sx + 5, sy + 5), fill="#8a8b7b")
        d.text((x0 + 24, y0 + 18), "KATORGA-12      STATUS", fill="#c3d3ce", font=font)
        d.line((x0 + 20, y0 + 50, x1 - 20, y0 + 50), fill="#9b6936", width=2)
        d.text((x0 + 24, y0 + 66), "HEALTH       50/100", fill="#edaa9f", font=font)
        for i in range(10):
            px = x0 + 24 + i * (x1 - x0 - 48) // 10
            d.rectangle((px, y0 + 101, px + 20, y0 + 112), fill="#bc453a" if i < 5 else "#353e40")
        d.text((x0 + 24, y0 + 136), "E99 TECH       4216", fill="#9ae2e7", font=font)
        d.text((x0 + 24, y0 + 171), "MEDKIT 3     VIAL 2", fill="#cdd6d2", font=font)
    d.text((30, 30), "OFFLINE FIXTURE - SYNTHETIC DATA - NOT A GAME CAPTURE", fill="#e0b879", font=font)
    path.parent.mkdir(parents=True, exist_ok=True)
    im.save(path)

if __name__ == "__main__":
    ap = argparse.ArgumentParser()
    ap.add_argument("output", type=Path)
    render(ap.parse_args().output)
