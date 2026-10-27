#!/usr/bin/env python3
"""Holographic radial weapon wheel — layout like Alyx-style reference, Katorga labels."""
from __future__ import annotations

import json
import math
from pathlib import Path

from PIL import Image, ImageDraw, ImageFilter

import wheel_weapon_draw as wwd

ROOT = Path(__file__).resolve().parents[2]
PACK = ROOT / "assets" / "hud" / "katorga"
OUT_MAIN = PACK / "previews" / "weapon-wheel.png"
OUT_3D = ROOT / "assets" / "hud" / "katorga" / "3d" / "previews" / "weapon_wheel_generated.png"
OUT_SPEC = PACK / "wheel" / "holo_wheel_style.json"
ICON_DIR = PACK / "wheel" / "icons"
CATALOG = ROOT / "assets" / "hud" / "katorga" / "3d" / "weapon_wheel_catalog.json"

ICON_FILES = {
    "wheel-current": "obecna_alpha.png",
    "weapon-centurion": "centurion_alpha.png",
    "weapon-valkyrie": "valkyrie_alpha.png",
    "weapon-spikeshot": "spikeshot_alpha.png",
    "weapon-kasimov": "kasimov_alpha.png",
}

SLOTS = 8
# GLB ring footprint (cm) — runtime still uses ring_weapon_wheel.glb; holo panels are wider visually
R0_CM, R1_CM = 20.16, 24.84

BG = (0, 0, 0)
CYAN = (110, 205, 255)
CYAN_DIM = (40, 90, 130)
ORANGE = (255, 158, 64)
ORANGE_DIM = (120, 70, 30)
WHITE = (235, 242, 255)


def _font(size: int, bold: bool = False):
    from PIL import ImageFont

    name = "segoeuib.ttf" if bold else "segoeui.ttf"
    p = Path("C:/Windows/Fonts") / name
    if p.is_file():
        return ImageFont.truetype(str(p), size)
    return ImageFont.load_default()


def wedge_polygon(cx: float, cy: float, r0: float, r1: float, a0: float, a1: float, steps: int = 32) -> list[tuple[float, float]]:
    pts: list[tuple[float, float]] = []
    for k in range(steps + 1):
        t = a0 + (a1 - a0) * (k / steps)
        pts.append((cx + math.cos(t) * r1, cy + math.sin(t) * r1))
    for k in range(steps, -1, -1):
        t = a0 + (a1 - a0) * (k / steps)
        pts.append((cx + math.cos(t) * r0, cy + math.sin(t) * r0))
    return pts


def draw_glow_wedge(base: Image.Image, poly: list[tuple[float, float]], color: tuple, blur: int = 10, alpha: int = 100) -> None:
    layer = Image.new("RGBA", base.size, (0, 0, 0, 0))
    d = ImageDraw.Draw(layer)
    d.polygon(poly, outline=color + (alpha,), width=4)
    layer = layer.filter(ImageFilter.GaussianBlur(blur))
    base.alpha_composite(layer)


def render(selected: int = 1, size: int = 1280) -> Image.Image:
    catalog = json.loads(CATALOG.read_text(encoding="utf-8"))
    sectors = catalog["sectors"][:SLOTS]

    cx = cy = size / 2
    r_hub = size * 0.11
    r_in = size * 0.16
    r_out = size * 0.44

    img = Image.new("RGBA", (size, size), BG + (255,))
    draw = ImageDraw.Draw(img)

    # Rotate so selected sector sits at top (12 o'clock), like reference pistol slot
    rot = -math.pi / 2 - ((selected + 0.5) / SLOTS) * math.tau

    visible = [i for i, s in enumerate(sectors) if (s.get("label_pl") or "").strip() or s.get("atlas_icon") != "wheel-slot-empty"]
    n_vis = max(len(visible), 1)

    for i, sec in enumerate(sectors):
        if i not in visible:
            continue
        # Re-space visible sectors evenly (6-panel look like reference)
        vi = visible.index(i)
        a0 = (vi / n_vis) * math.tau + rot
        a1 = ((vi + 1) / n_vis) * math.tau + rot
        poly = wedge_polygon(cx, cy, r_in, r_out, a0, a1)
        active = i == selected
        fill = (45, 28, 12, 110) if active else (8, 20, 35, 55)
        edge = ORANGE if active else CYAN
        draw.polygon(poly, fill=fill)
        draw_glow_wedge(img, poly, ORANGE if active else CYAN, blur=12 if active else 8, alpha=140 if active else 90)
        draw = ImageDraw.Draw(img)
        draw.polygon(poly, outline=edge, width=4 if active else 2)

        # Outer tick marks
        mid = (a0 + a1) / 2
        tx0 = cx + math.cos(mid) * (r_out - 8)
        ty0 = cy + math.sin(mid) * (r_out - 8)
        tx1 = cx + math.cos(mid) * (r_out + 4)
        ty1 = cy + math.sin(mid) * (r_out + 4)
        draw.line([(tx0, ty0), (tx1, ty1)], fill=ORANGE if active else CYAN_DIM, width=2)

        label = (sec.get("label_pl") or "").strip()
        if not label and sec.get("atlas_icon") == "wheel-slot-empty":
            continue

        title = label.upper() if label else "—"
        mid_r_label = r_out - size * 0.055
        lx = cx + math.cos(mid) * mid_r_label
        ly = cy + math.sin(mid) * mid_r_label
        tang = mid + math.pi / 2
        font_t = _font(15 if not active else 17, bold=active)
        tw = draw.textlength(title, font=font_t)
        # Title toward outer rim
        ox = lx - (tw / 2) * math.cos(tang) * 0.15
        oy = ly - (tw / 2) * math.sin(tang) * 0.15
        draw.text((ox - tw / 2, oy - 10), title, fill=ORANGE if active else WHITE, font=font_t)

        icon_r = (r_in + r_out) * 0.52
        ix = cx + math.cos(mid) * icon_r
        iy = cy + math.sin(mid) * icon_r
        icon_id = sec.get("atlas_icon") or "wheel-slot-empty"
        col = ORANGE if active else CYAN
        icon_span = int((r_out - r_in) * 0.72)
        fname = ICON_FILES.get(icon_id)
        ipath = ICON_DIR / fname if fname else None
        if ipath and ipath.is_file():
            tile = Image.open(ipath).convert("RGBA")
            if active:
                # slight warm tint on selected icon
                r, g, b, a = tile.split()
                r = r.point(lambda p: min(255, int(p * 1.08 + 20)))
                g = g.point(lambda p: min(255, int(p * 0.95 + 10)))
                tile = Image.merge("RGBA", (r, g, b, a))
            tile = tile.resize((icon_span, icon_span), Image.Resampling.LANCZOS)
            img.paste(tile, (int(ix - icon_span / 2), int(iy - icon_span / 2)), tile)
            draw = ImageDraw.Draw(img)
        else:
            scale = wwd.weapon_scale_for_px(icon_span)
            lw = max(2, int(size / 400))
            wwd.draw_weapon_body(draw, ix, iy, scale, mid, icon_id, col, line_width=lw)

    # Central hub
    for r, col, bw in ((r_hub + 14, CYAN_DIM, 2), (r_hub + 6, CYAN, 2), (r_hub, ORANGE_DIM, 1)):
        draw.ellipse([cx - r, cy - r, cx + r, cy + r], outline=col, width=bw)
    draw.ellipse([cx - r_hub, cy - r_hub, cx + r_hub, cy + r_hub], fill=(10, 25, 40, 200), outline=ORANGE, width=2)
    hub_font = _font(16, bold=True)
    hub = "WYBIERZ\nBRO\u0143"
    lines = hub.split("\n")
    y0 = cy - 18
    for line in lines:
        tw = draw.textlength(line, font=hub_font)
        draw.text((cx - tw / 2, y0), line, fill=WHITE, font=hub_font)
        y0 += 20

    # Spokes / chevrons between wedges
    for vi in range(n_vis):
        ang = (vi / n_vis) * math.tau + rot
        r0, r1 = r_hub + 4, r_in - 6
        draw.line(
            [(cx + math.cos(ang) * r0, cy + math.sin(ang) * r0), (cx + math.cos(ang) * r1, cy + math.sin(ang) * r1)],
            fill=ORANGE_DIM,
            width=1,
        )

    return img.convert("RGB")


def write_spec() -> None:
    OUT_SPEC.parent.mkdir(parents=True, exist_ok=True)
    spec = {
        "schema": "singularityvr-holo-wheel-style-v1",
        "reference": "Alyx-style radial panels; cyan idle, orange selected",
        "colors_rgb": {"cyan": CYAN, "orange": ORANGE, "background": BG},
        "layout_normalized": {"r_hub": 0.11, "r_panel_inner": 0.16, "r_panel_outer": 0.44},
        "hub_label_pl": "WYBIERZ BRO\u0143",
        "glb_ring_cm": {"r0": R0_CM, "r1": R1_CM},
        "note": "Visual panels extend beyond GLB ring; MAIN may scale ring mesh or draw holo wedges in controller space.",
        "preview_visible_sectors": "Only catalog sectors with label_pl (5); empty slots hidden in PNG. Runtime WeaponWheelSlots may still be 8.",
    }
    OUT_SPEC.write_text(json.dumps(spec, indent=2) + "\n", encoding="utf-8")


def main() -> None:
    img = render(selected=1)
    OUT_MAIN.parent.mkdir(parents=True, exist_ok=True)
    OUT_3D.parent.mkdir(parents=True, exist_ok=True)
    img.save(OUT_MAIN, quality=95)
    img.save(OUT_3D, quality=95)
    write_spec()
    print("wrote", OUT_MAIN)
    print("wrote", OUT_3D)
    print("wrote", OUT_SPEC)


if __name__ == "__main__":
    main()
