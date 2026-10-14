"""Build V + mirrored-R (Ya) suffix glyphs from an in-frame SINGULARITY strip."""
from __future__ import annotations

from PIL import Image


def lum(px: tuple[int, int, int, int]) -> float:
    return 0.299 * px[0] + 0.587 * px[1] + 0.114 * px[2]


def content_span(strip: Image.Image, threshold: float = 42.0) -> tuple[int, int]:
    rgba = strip.convert("RGBA")
    w, h = rgba.size
    px = rgba.load()
    cols = [x for x in range(w) if any(lum(px[x, y]) > threshold for y in range(h))]
    if not cols:
        raise ValueError("empty strip")
    return cols[0], cols[-1]


def split_eleven_glyphs(strip: Image.Image) -> list[Image.Image]:
    """SINGULARITY = 11 glyphs on the active horizontal span."""
    c0, c1 = content_span(strip)
    span = c1 - c0 + 1
    edges = [c0 + (span * i) // 11 for i in range(12)]
    return [strip.crop((edges[i], 0, edges[i + 1], strip.size[1])) for i in range(11)]


def build_v_from_a(a_glyph: Image.Image) -> Image.Image:
    """Stencil V from A: flip halves vertically (matches ui_logos compose path)."""
    a = a_glyph.convert("RGBA")
    w, h = a.size
    mid = max(1, w // 2)
    left = a.crop((0, 0, mid, h)).transpose(Image.Transpose.FLIP_TOP_BOTTOM)
    right = a.crop((mid, 0, w, h)).transpose(Image.Transpose.FLIP_TOP_BOTTOM)
    v = Image.new("RGBA", (w, h), (0, 0, 0, 0))
    v.paste(left, (0, 0), left.split()[3])
    v.paste(right, (mid, 0), right.split()[3])
    return v


def letter_gap(strip: Image.Image) -> int:
    c0, c1 = content_span(strip)
    return max(3, (c1 - c0 + 1) // 55)


def build_v_ya_suffix(strip: Image.Image) -> tuple[Image.Image, Image.Image, int]:
    """Return (v_glyph, ya_glyph, gap_px). Ya clones in-word R (index 7), not the final Y."""
    glyphs = split_eleven_glyphs(strip)
    # S-I-N-G-U-L-A-R-I-T-Y
    a_glyph = glyphs[6]
    ya_glyph = glyphs[7]  # in-word mirrored R (SINGULAЯ…)
    v_glyph = build_v_from_a(a_glyph)
    return v_glyph, ya_glyph, letter_gap(strip)


def append_v_ya_to_strip(strip: Image.Image) -> Image.Image:
    """Full strip + exactly two suffix glyphs (V, Ya). Word geometry unchanged."""
    v_glyph, ya_glyph, gap = build_v_ya_suffix(strip)
    w, h = strip.size
    suffix_w = v_glyph.size[0] + gap + ya_glyph.size[0]
    extended = Image.new("RGBA", (w + gap + suffix_w, h), (0, 0, 0, 255))
    extended.paste(strip.convert("RGBA"), (0, 0))
    x = w + gap
    extended.paste(v_glyph, (x, 0), v_glyph)
    extended.paste(ya_glyph, (x + v_glyph.size[0] + gap, 0), ya_glyph)
    return extended
