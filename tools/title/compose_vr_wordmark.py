#!/usr/bin/env python3
"""Build SINGULARITY VR wordmark PNGs from the stock ui_logos.logo decode.

Uses geometry cloned from the stock strip (not a generic font). Outputs a 512x128
master and a 1280x720 boot card composite for Bink frame patching.
"""
from __future__ import annotations

import argparse
import hashlib
import json
from pathlib import Path

from PIL import Image, ImageFilter


def sha256_bytes(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest().upper()


def luminance(r: int, g: int, b: int) -> float:
    return 0.299 * r + 0.587 * g + 0.114 * b


def mask_from_logo(img: Image.Image, threshold: float = 45.0) -> Image.Image:
    rgba = img.convert("RGBA")
    w, h = rgba.size
    out = Image.new("L", (w, h), 0)
    px = rgba.load()
    m = out.load()
    for y in range(h):
        for x in range(w):
            r, g, b, a = px[x, y]
            if luminance(r, g, b) > threshold and a > 8:
                m[x, y] = 255
    return out.filter(ImageFilter.GaussianBlur(1.2))


def column_bounds(mask: Image.Image) -> tuple[int, int]:
    w, h = mask.size
    cols = [x for x in range(w) if any(mask.getpixel((x, y)) > 32 for y in range(h))]
    if not cols:
        raise ValueError("empty logo mask")
    return cols[0], cols[-1]


def crop_glyph(img: Image.Image, mask: Image.Image, x0: int, x1: int) -> Image.Image:
    w, h = img.size
    x0 = max(0, min(w - 1, x0))
    x1 = max(x0 + 1, min(w, x1))
    box = (x0, 0, x1, h)
    return img.crop(box), mask.crop(box)


def paste_rgba(base: Image.Image, layer: Image.Image, xy: tuple[int, int]) -> None:
    base.paste(layer, xy, layer)


def build_v_from_a(a_img: Image.Image, a_mask: Image.Image) -> Image.Image:
    w, h = a_img.size
    mid = w // 2
    left = a_img.crop((0, 0, mid, h))
    right = a_img.crop((mid, 0, w, h))
    left_m = a_mask.crop((0, 0, mid, h))
    right_m = a_mask.crop((mid, 0, w, h))
    v = Image.new("RGBA", (w, h), (0, 0, 0, 0))
    paste_rgba(v, left.transpose(Image.Transpose.FLIP_TOP_BOTTOM), (0, 0))
    paste_rgba(v, right.transpose(Image.Transpose.FLIP_TOP_BOTTOM), (mid, 0))
    vm = Image.new("L", (w, h), 0)
    paste_rgba(vm.convert("RGBA"), left_m.transpose(Image.Transpose.FLIP_TOP_BOTTOM), (0, 0))
    paste_rgba(vm.convert("RGBA"), right_m.transpose(Image.Transpose.FLIP_TOP_BOTTOM), (mid, 0))
    return v


def compose(stock_png: Path) -> tuple[Image.Image, Image.Image, dict]:
    stock = Image.open(stock_png).convert("RGBA")
    if stock.size != (512, 128):
        raise ValueError("expected decoded stock logo 512x128")
    mask = mask_from_logo(stock)
    x_left, x_right = column_bounds(mask)
    # Proportional splits for SINGULARITY (11 glyphs) on the active span.
    span = x_right - x_left + 1
    edges = [x_left + int(span * i / 11) for i in range(12)]
    glyphs = []
    masks = []
    for i in range(11):
        g, gm = crop_glyph(stock, mask, edges[i], edges[i + 1])
        glyphs.append(g)
        masks.append(gm)
    # Letter templates from the stock strip.
    a_img, a_mask = glyphs[6], masks[6]  # S-I-N-G-U-L-A-…
    r_img, r_mask = glyphs[7], masks[7]  # mirrored R / Ya template
    v_img = build_v_from_a(a_img, a_mask)

    target_w, target_h = 512, 128
    gap = max(4, span // 40)
    vr_w = v_img.size[0] + gap + r_img.size[0]
    need = span + gap + vr_w
    scale = min(1.0, (target_w - 16) / need)
    if scale < 0.55:
        raise ValueError("VR suffix does not fit 512px without excessive shrink")

    def scale_im(im: Image.Image, s: float) -> Image.Image:
        if abs(s - 1.0) < 1e-6:
            return im
        nw = max(1, int(im.size[0] * s))
        nh = max(1, int(im.size[1] * s))
        return im.resize((nw, nh), Image.Resampling.LANCZOS)

    scaled = [scale_im(g, scale) for g in glyphs]
    v_s = scale_im(v_img, scale)
    r_s = scale_im(r_img, scale)
    word_w = sum(g.size[0] for g in scaled) + gap + v_s.size[0] + r_s.size[0]
    canvas = Image.new("RGBA", (target_w, target_h), (0, 0, 0, 255))
    x = (target_w - word_w) // 2
    y = (target_h - scaled[0].size[1]) // 2
    for g in scaled:
        paste_rgba(canvas, g, (x, y))
        x += g.size[0]
    x += gap
    paste_rgba(canvas, v_s, (x, y))
    x += v_s.size[0]
    paste_rgba(canvas, r_s, (x, y))

    glow = mask_from_logo(canvas, threshold=35.0).filter(ImageFilter.GaussianBlur(3.5))
    glow_rgba = Image.merge("RGBA", (glow, glow, glow, glow.point(lambda p: min(255, int(p * 0.55)))))
    master = Image.alpha_composite(canvas, glow_rgba)

    boot = Image.new("RGBA", (1280, 720), (0, 0, 0, 255))
    fit_w = int(target_w * (720 * 0.35) / target_h)
    fit_h = int(720 * 0.35)
    boot_logo = master.resize((fit_w, fit_h), Image.Resampling.LANCZOS)
    paste_rgba(boot, boot_logo, ((1280 - fit_w) // 2, (720 - fit_h) // 2))

    report = {
        "stock_sha256": sha256_bytes(stock_png.read_bytes()),
        "scale_to_fit_vr": round(scale, 4),
        "master_size": [target_w, target_h],
        "boot_composite_size": [1280, 720],
        "glyph_source": "cloned_from_stock_strip_not_generic_font",
    }
    return master, boot, report


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("--stock-png", type=Path, required=True)
    ap.add_argument("--out-dir", type=Path, required=True)
    args = ap.parse_args()
    if args.out_dir.exists():
        raise SystemExit("Refuse existing --out-dir")
    master, boot, report = compose(args.stock_png)
    args.out_dir.mkdir(parents=True)
    master_path = args.out_dir / "wordmark-512x128.png"
    boot_path = args.out_dir / "wordmark-1280x720.png"
    master.save(master_path)
    boot.save(boot_path)
    report.update(
        master_sha256=sha256_bytes(master_path.read_bytes()),
        boot_sha256=sha256_bytes(boot_path.read_bytes()),
        status="OFFLINE_COMPOSE_NOT_L4",
    )
    (args.out_dir / "compose.json").write_text(json.dumps(report, indent=2), encoding="utf-8")
    print(json.dumps(report, indent=2))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
