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


def _core(px: tuple[int, int, int, int]) -> bool:
    r, g, b, a = px
    return a > 15 and min(r, g, b) > 110


def _clusters(cols: list[int]) -> list[tuple[int, int]]:
    if not cols:
        return []
    groups: list[tuple[int, int]] = []
    start = prev = cols[0]
    for x in cols[1:]:
        if x > prev + 3:
            groups.append((start, prev))
            start = x
        prev = x
    groups.append((start, prev))
    return groups


def erase_crossbar(glyph: Image.Image) -> Image.Image:
    """Remove the A's bar after a vertical flip. Keep the tip of the V."""
    im = glyph.convert("RGBA").copy()
    w, h = im.size
    px = im.load()

    def core_cols(y: int) -> list[int]:
        return [x for x in range(w) if min(px[x, y][:3]) > 165 and px[x, y][3] > 20]

    row_clusters = [_clusters(core_cols(y)) for y in range(h)]
    strokes: list[int] = []
    for clusters in row_clusters:
        if len(clusters) == 2:
            strokes.append(clusters[0][1] - clusters[0][0] + 1)
            strokes.append(clusters[1][1] - clusters[1][0] + 1)
    stroke = max(4, sorted(strokes)[len(strokes) // 2]) if strokes else max(4, w // 7)

    def two_legs(y: int) -> bool:
        return 0 <= y < h and len(row_clusters[y]) == 2

    y = 0
    while y < h:
        clusters = row_clusters[y]
        if len(clusters) != 1 or clusters[0][1] - clusters[0][0] + 1 < int(w * 0.45):
            y += 1
            continue
        start = y
        while y < h and len(row_clusters[y]) == 1 and row_clusters[y][0][1] - row_clusters[y][0][0] + 1 >= int(w * 0.45):
            y += 1
        end = y - 1
        # Tip of the V is a solid run with legs on only one side.
        if not any(two_legs(k) for k in range(start - 8, start)) or not any(two_legs(k) for k in range(end + 1, end + 9)):
            continue
        for yy in range(start, end + 1):
            left_edge, right_edge = row_clusters[yy][0]
            gap_l = left_edge + stroke
            gap_r = right_edge - stroke
            if gap_r <= gap_l:
                continue
            for x in range(gap_l, gap_r + 1):
                px[x, yy] = (0, 0, 0, 0)
            # Drop the bar's bloom in the opening without eating the leg edges.
            inner_l = gap_l + max(1, stroke // 5)
            inner_r = gap_r - max(1, stroke // 5)
            for ny in range(max(0, yy - 2), min(h, yy + 3)):
                for x in range(inner_l, inner_r + 1):
                    if min(px[x, ny][:3]) < 210:
                        px[x, ny] = (0, 0, 0, 0)
    return im


def build_v_from_a(a_glyph: Image.Image) -> Image.Image:
    """Symmetric stencil V: mirror the A's left leg, then point it downward."""
    im = a_glyph.convert("RGBA")
    w, h = im.size
    px = im.load()
    left = Image.new("RGBA", (w, h), (0, 0, 0, 0))
    out = left.load()
    for y in range(h):
        cols = [x for x in range(w) if px[x, y][3] > 20 and min(px[x, y][:3]) > 150]
        clusters = _clusters(cols)
        if len(clusters) == 2:
            edge_l, edge_r = clusters[0]
        elif len(clusters) == 1 and y < int(h * 0.28):
            edge_l, edge_r = clusters[0][0], (clusters[0][0] + clusters[0][1]) // 2
        else:
            continue
        x0 = max(0, edge_l - 5)
        x1 = min(w - 1, edge_r + 3)
        for x in range(x0, x1 + 1):
            out[x, y] = px[x, y]
    mirrored = left.transpose(Image.Transpose.FLIP_LEFT_RIGHT)
    joined = Image.new("RGBA", (w, h), (0, 0, 0, 0))
    joined.paste(left, (0, 0), left)
    joined.paste(mirrored, (0, 0), mirrored)
    return joined.transpose(Image.Transpose.FLIP_TOP_BOTTOM)


def build_latin_r(ya_glyph: Image.Image) -> Image.Image:
    """Game R is the mirrored Ya. Flip it so the suffix reads as a Latin R."""
    return ya_glyph.convert("RGBA").transpose(Image.Transpose.FLIP_LEFT_RIGHT)


def tint_cyan(glyph: Image.Image) -> Image.Image:
    """Recolor a cloned glyph to the ice-blue VR in the reference stills."""
    im = glyph.convert("RGBA")
    px = im.load()
    w, h = im.size
    for y in range(h):
        for x in range(w):
            r, g, b, a = px[x, y]
            if a < 8:
                px[x, y] = (0, 0, 0, 0)
                continue
            level = lum((r, g, b, a)) / 255.0
            # White core, cyan shoulder, blue bloom.
            nr = int(30 + level * 210)
            ng = int(110 + level * 140)
            nb = int(190 + level * 65)
            px[x, y] = (min(255, nr), min(255, ng), min(255, nb), a)
    return im


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


def ink_spans(band: Image.Image, min_width: int = 10) -> list[tuple[int, int]]:
    rgba = band.convert("RGBA")
    w, h = rgba.size
    px = rgba.load()
    spans: list[tuple[int, int]] = []
    start = None
    for x in range(w):
        hit = any(_core(px[x, y]) for y in range(h))
        if hit and start is None:
            start = x
        elif not hit and start is not None:
            if x - start >= min_width:
                spans.append((start, x - 1))
            start = None
    if start is not None and w - start >= min_width:
        spans.append((start, w - 1))
    return spans


def soft_glyph(band: Image.Image, x0: int, x1: int, pad: int = 6) -> Image.Image:
    left = max(0, x0 - pad)
    right = min(band.size[0] - 1, x1 + pad)
    crop = band.crop((left, 0, right + 1, band.size[1])).convert("RGBA")
    px = crop.load()
    w, h = crop.size
    for y in range(h):
        for x in range(w):
            r, g, b, a = px[x, y]
            level = lum((r, g, b, a))
            if level < 32:
                px[x, y] = (0, 0, 0, 0)
            else:
                px[x, y] = (r, g, b, min(255, int((level - 32) * 5)))
    return crop


def stamp_v_ya(frame: Image.Image, y0: int, y1: int, pad: int = 6) -> Image.Image:
    """Add V and Ya on the letter row. Leaves every other pixel alone."""
    base = frame.convert("RGBA")
    band = base.crop((0, y0, base.size[0], y1 + 1))
    letters = [s for s in ink_spans(band) if (s[1] - s[0]) >= 12][:11]
    if len(letters) < 11:
        raise ValueError(f"expected 11 letters, saw {len(letters)}")
    glyphs = [soft_glyph(band, a, b, pad) for a, b in letters]
    v_glyph = tint_cyan(build_v_from_a(glyphs[6]))
    ya_glyph = tint_cyan(build_latin_r(glyphs[7]))
    gaps = [letters[i + 1][0] - letters[i][1] - 1 for i in range(10)]
    gap = max(4, sorted(gaps)[len(gaps) // 2])
    out = base.copy()
    x = letters[-1][1] + 1 + gap
    aw = letters[6][1] - letters[6][0] + 1
    rw = letters[7][1] - letters[7][0] + 1
    need = aw + gap + rw
    room = out.size[0] - 4 - x
    scale = 1.0 if need <= room else room / need
    if scale < 0.72:
        raise ValueError("suffix does not fit")
    if scale < 1.0:
        def shrink(glyph: Image.Image) -> Image.Image:
            return glyph.resize((max(1, int(glyph.size[0] * scale)), max(1, int(glyph.size[1] * scale))), Image.Resampling.LANCZOS)

        v_glyph = shrink(v_glyph)
        ya_glyph = shrink(ya_glyph)
        gap = max(2, int(gap * scale))
        y_paste = y0 + (y1 - y0 + 1 - v_glyph.size[1]) // 2
    else:
        y_paste = y0
    for glyph, width in ((v_glyph, max(1, int(aw * scale))), (ya_glyph, max(1, int(rw * scale)))):
        out.paste(glyph, (x - int(pad * scale), y_paste), glyph)
        x += width + gap
    return out


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
