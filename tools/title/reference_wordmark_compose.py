"""Composite user-supplied reference stills onto Bink frames (no glyph cloning)."""
from __future__ import annotations

from PIL import Image, ImageChops


def lum(px: tuple[int, ...]) -> float:
    r, g, b = px[0], px[1], px[2]
    return 0.299 * r + 0.587 * g + 0.114 * b


def bright_bbox(im: Image.Image, threshold: float = 42.0) -> tuple[int, int, int, int]:
    rgba = im.convert("RGBA")
    w, h = rgba.size
    px = rgba.load()
    xs, ys = [], []
    for y in range(h):
        for x in range(w):
            if lum(px[x, y]) > threshold:
                xs.append(x)
                ys.append(y)
    if not xs:
        raise ValueError("no bright wordmark region")
    return min(xs), min(ys), max(xs), max(ys)


def _alpha_from_luminance(im: Image.Image, floor: float = 28.0, gain: float = 4.5, hard: bool = False) -> Image.Image:
    rgba = im.convert("RGBA")
    px = rgba.load()
    w, h = rgba.size
    for y in range(h):
        for x in range(w):
            r, g, b, a = px[x, y]
            level = lum((r, g, b, a))
            if level < floor:
                px[x, y] = (0, 0, 0, 0)
            elif hard and level > floor + 18:
                px[x, y] = (r, g, b, 255)
            else:
                alpha = min(255, int((level - floor) * gain))
                px[x, y] = (r, g, b, alpha)
    return rgba


def _is_cyan(r: int, g: int, b: int) -> bool:
    return b > 115 and b > r + 18 and g > 45 and lum((r, g, b, 255)) > 65


def extract_wordmark_layer(reference: Image.Image) -> Image.Image:
    """Full SINGULARITY+VR crop from boot reference still."""
    ref = reference.convert("RGBA")
    x0, y0, x1, y1 = bright_bbox(ref, threshold=38.0)
    pad_x = max(8, int((x1 - x0) * 0.04))
    pad_y = max(6, int((y1 - y0) * 0.12))
    w, h = ref.size
    crop = ref.crop((max(0, x0 - pad_x), max(0, y0 - pad_y), min(w - 1, x1 + pad_x), min(h - 1, y1 + pad_y)))
    return _alpha_from_luminance(crop, floor=32.0, gain=6.0, hard=True)


def letter_band_flat(frame: Image.Image, y_frac: float = 0.32) -> tuple[int, int]:
    rgba = frame.convert("RGBA")
    w, h = rgba.size
    y_limit = int(h * y_frac)
    px = rgba.load()
    rows: list[int] = []
    for y in range(y_limit):
        n = sum(1 for x in range(w) if lum(px[x, y]) > 170)
        if n > 40:
            rows.append(y)
    if not rows:
        raise ValueError("no letter row")
    bands: list[tuple[int, int, int]] = []
    start = prev = rows[0]
    for y in rows[1:]:
        if y == prev + 1:
            prev = y
        else:
            bands.append((start, prev, prev - start))
            start = prev = y
    bands.append((start, prev, prev - start))
    y0, y1, _ = max(bands, key=lambda b: b[2])
    return y0, y1


def _ink_spans(band: Image.Image, threshold: float = 170.0, min_width: int = 8) -> list[tuple[int, int]]:
    w, h = band.size
    px = band.convert("RGBA").load()
    spans: list[tuple[int, int]] = []
    start = None
    for x in range(w):
        hit = any(lum(px[x, y]) > threshold for y in range(h))
        if hit and start is None:
            start = x
        elif not hit and start is not None:
            if x - start >= min_width:
                spans.append((start, x - 1))
            start = None
    if start is not None and w - start >= min_width:
        spans.append((start, w - 1))
    return spans


def title_right_edge(frame: Image.Image) -> tuple[int, int, int]:
    """Right edge of the main wordmark (before TM tick). Returns y0,y1,x_right."""
    y0, y1 = letter_band_flat(frame)
    band = frame.crop((0, y0, frame.size[0], y1 + 1))
    spans = _ink_spans(band)
    if not spans:
        raise ValueError("no ink spans")
    # Last wide span is the Y; anything narrower after is TM.
    main = [s for s in spans if (s[1] - s[0]) >= 20]
    if not main:
        main = spans
    x_right = main[-1][1]
    return y0, y1, x_right


def extract_vr_suffix_layer(reference: Image.Image) -> Image.Image:
    """Cyan VR letters only from loading (or boot) reference still."""
    ref = reference.convert("RGBA")
    y0, y1 = letter_band_flat(ref, y_frac=0.45)
    pad_y = max(4, int((y1 - y0) * 0.35))
    strip = ref.crop((0, max(0, y0 - pad_y), ref.size[0] - 1, min(ref.size[1] - 1, y1 + pad_y)))
    w, h = strip.size
    px = strip.load()
    scores: list[int] = []
    for x in range(w):
        scores.append(sum(1 for y in range(h) if _is_cyan(*px[x, y][:3])))
    need = max(3, int(h * 0.06))
    run: list[tuple[int, int]] = []
    start = None
    for x, score in enumerate(scores):
        if score >= need and start is None:
            start = x
        elif score < need and start is not None:
            run.append((start, x - 1))
            start = None
    if start is not None:
        run.append((start, w - 1))
    if not run:
        raise ValueError("no cyan VR column run in reference")
    # V and R are separate cyan runs with a small gap; merge before cropping.
    run.sort()
    merged: list[tuple[int, int]] = []
    for seg in run:
        if not merged or seg[0] - merged[-1][1] > 18:
            merged.append(seg)
        else:
            merged[-1] = (merged[-1][0], seg[1])
    right = [m for m in merged if m[0] >= int(w * 0.55)]
    pick = max(right or merged, key=lambda r: r[1] - r[0])
    x0, x1 = pick
    pad_x = 4
    crop = strip.crop((max(0, x0 - pad_x), 0, min(w - 1, x1 + pad_x), h))
    return _alpha_from_luminance(crop, floor=36.0, gain=5.0, hard=True)


def _clear_luminous_rect(base: Image.Image, box: tuple[int, int, int, int], threshold: float = 38.0) -> None:
    x0, y0, x1, y1 = box
    px = base.load()
    for y in range(y0, y1 + 1):
        for x in range(x0, x1 + 1):
            r, g, b, a = px[x, y]
            if lum((r, g, b, a)) > threshold:
                px[x, y] = (0, 0, 0, 255)
            elif lum((r, g, b, a)) > threshold * 0.5:
                px[x, y] = (int(r * 0.08), int(g * 0.08), int(b * 0.08), 255)


def compose_boot_frame(frame: Image.Image, wordmark_layer: Image.Image, ref_span: int) -> Image.Image:
    """Replace stock chrome wordmark with the user reference still (black field + logo)."""
    try:
        bx0, by0, bx1, by1 = bright_bbox(frame)
    except ValueError:
        return frame.convert("RGB")
    span = bx1 - bx0
    if span < ref_span * 0.55:
        return frame.convert("RGB")

    base = frame.convert("RGBA")
    w, _h = base.size
    clear_x0 = max(0, bx0 - int(span * 0.08))
    clear_x1 = min(w - 1, bx1 + int(span * 0.28))
    clear_y0 = max(0, by0 - int((by1 - by0) * 0.25))
    clear_y1 = min(base.size[1] - 1, by1 + int((by1 - by0) * 0.25))
    _clear_luminous_rect(base, (clear_x0, clear_y0, clear_x1, clear_y1))

    target_w = clear_x1 - clear_x0 + 1
    aspect = wordmark_layer.size[0] / wordmark_layer.size[1]
    target_h = max(2, int(target_w / aspect))
    overlay = wordmark_layer.resize((target_w, target_h), Image.Resampling.LANCZOS)
    oy = clear_y0 + max(0, (clear_y1 - clear_y0 + 1 - target_h) // 2)
    base.paste(overlay, (clear_x0, oy), overlay)
    return base.convert("RGB")


def compose_loading_frame(frame: Image.Image, vr_layer: Image.Image) -> Image.Image:
    """Keep stock orange SINGULARITY + hand; paste cyan VR from reference only."""
    base = frame.convert("RGBA")
    y0, y1, x_right = title_right_edge(base)
    band_h = y1 - y0 + 1
    gap = max(6, int(band_h * 0.08))
    target_h = band_h + max(8, int(band_h * 0.2))
    aspect = vr_layer.size[0] / vr_layer.size[1]
    target_w = max(2, int(target_h * aspect))
    overlay = vr_layer.resize((target_w, target_h), Image.Resampling.LANCZOS)
    ox = min(base.size[0] - target_w - 2, x_right + gap)
    oy = y0 - max(4, int(band_h * 0.12))
    base.paste(overlay, (ox, oy), overlay)
    return base.convert("RGB")


def diff_mean(a: Image.Image, b: Image.Image) -> float:
    return ImageChops.difference(a.convert("RGB"), b.convert("RGB")).convert("L").resize((64, 36)).getextrema()[1] / 255.0
