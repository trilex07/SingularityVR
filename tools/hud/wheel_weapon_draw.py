"""Wireframe weapon art for holo weapon wheel (original line art, not game assets)."""
from __future__ import annotations

import math

from PIL import ImageDraw

DIM = (70, 120, 160)


def _rot(x: float, y: float, ang: float) -> tuple[float, float]:
    c, s = math.cos(ang), math.sin(ang)
    return x * c - y * s, x * s + y * c


def _ln(draw: ImageDraw.ImageDraw, cx: float, cy: float, scale: float, ang: float, x1, y1, x2, y2, color, w: int) -> None:
    a = _rot(x1 * scale, y1 * scale, ang)
    b = _rot(x2 * scale, y2 * scale, ang)
    draw.line([(cx + a[0], cy + a[1]), (cx + b[0], cy + b[1])], fill=color, width=w)


def weapon_scale_for_px(px: float) -> float:
    """Map abstract weapon coords (~76 units wide) to canvas size."""
    return px * 0.72 / 76.0


def draw_weapon_body(
    draw: ImageDraw.ImageDraw,
    cx: float,
    cy: float,
    scale: float,
    angle: float,
    kind: str,
    color: tuple[int, int, int],
    line_width: int | None = None,
) -> None:
    """Weapon silhouette only — no selection ring, no ammo."""
    w = line_width if line_width is not None else max(2, int(2.2 * scale / 64))
    body = "weapon-centurion" if kind == "wheel-current" else kind

    if body == "weapon-centurion":
        _ln(draw, cx, cy, scale, angle, -28, 6, -28, 12, color, w)
        _ln(draw, cx, cy, scale, angle, -28, 12, -20, 12, color, w)
        _ln(draw, cx, cy, scale, angle, -20, 12, -20, 5, color, w)
        _ln(draw, cx, cy, scale, angle, -20, 5, -6, 3, color, w)
        _ln(draw, cx, cy, scale, angle, -6, 3, 10, 1, color, w)
        _ln(draw, cx, cy, scale, angle, 10, 1, 34, 0, color, w)
        _ln(draw, cx, cy, scale, angle, 34, 0, 38, 2, color, w)
        _ln(draw, cx, cy, scale, angle, -12, 5, -12, 11, color, w)

    elif body == "weapon-valkyrie":
        _ln(draw, cx, cy, scale, angle, -36, 6, -36, 12, color, w)
        _ln(draw, cx, cy, scale, angle, -36, 12, -28, 12, color, w)
        _ln(draw, cx, cy, scale, angle, -28, 12, -28, 4, color, w)
        _ln(draw, cx, cy, scale, angle, -28, 4, 12, 1, color, w)
        _ln(draw, cx, cy, scale, angle, 12, 1, 40, -1, color, w)
        _ln(draw, cx, cy, scale, angle, -6, 1, -6, 11, color, w)
        _ln(draw, cx, cy, scale, angle, 8, 3, 14, 11, color, w)
        _ln(draw, cx, cy, scale, angle, -14, -3, 6, -3, color, w - 1)

    elif body == "weapon-spikeshot":
        _ln(draw, cx, cy, scale, angle, -34, 8, -34, 14, color, w)
        _ln(draw, cx, cy, scale, angle, -34, 14, -26, 14, color, w)
        _ln(draw, cx, cy, scale, angle, -26, 14, -26, -2, color, w)
        _ln(draw, cx, cy, scale, angle, -26, -2, 10, -4, color, w)
        _ln(draw, cx, cy, scale, angle, 10, -4, 10, 4, color, w)
        _ln(draw, cx, cy, scale, angle, 10, 4, 36, 2, color, w)
        _ln(draw, cx, cy, scale, angle, -6, 5, 14, 5, color, w)
        _ln(draw, cx, cy, scale, angle, 2, 5, 2, 13, color, w - 1)

    elif body == "weapon-kasimov":
        _ln(draw, cx, cy, scale, angle, -38, 5, -38, 11, color, w)
        _ln(draw, cx, cy, scale, angle, -38, 11, -32, 11, color, w)
        _ln(draw, cx, cy, scale, angle, -32, 11, -32, 2, color, w)
        _ln(draw, cx, cy, scale, angle, -32, 2, 38, 0, color, w)
        _ln(draw, cx, cy, scale, angle, -10, -5, 10, -5, color, w)
        _ln(draw, cx, cy, scale, angle, -10, -5, -10, -2, color, w)
        _ln(draw, cx, cy, scale, angle, 10, -5, 10, -2, color, w)
        _ln(draw, cx, cy, scale, angle, 26, 0, 26, 6, color, w - 1)

    elif body == "wheel-slot-empty":
        s = 14 * scale
        draw.rectangle([cx - s, cy - s, cx + s, cy + s], outline=DIM, width=1)


def draw_wireframe(
    draw: ImageDraw.ImageDraw,
    cx: float,
    cy: float,
    scale: float,
    angle: float,
    kind: str,
    color: tuple[int, int, int],
) -> None:
    draw_weapon_body(draw, cx, cy, scale, angle, kind, color)
