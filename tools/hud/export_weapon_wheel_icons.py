#!/usr/bin/env python3
"""Export standalone weapon wheel icons (wireframe only, no ammo, no hub ring)."""
from __future__ import annotations

import hashlib
import json
from pathlib import Path

from PIL import Image, ImageDraw, ImageFilter

import wheel_weapon_draw as wwd

ROOT = Path(__file__).resolve().parents[2]
KATORGA = ROOT / "assets" / "hud" / "katorga"
OUT_DIR = KATORGA / "wheel" / "icons"
OUT_3D = ROOT / "assets" / "hud" / "katorga" / "3d" / "textures" / "wheel_icons"
ATLAS_PNG = KATORGA / "atlas" / "katorga-hud-atlas.png"
ATLAS_JSON = KATORGA / "atlas" / "atlas.json"
MANIFEST = OUT_DIR / "weapon_icons.json"

CYAN = (110, 205, 255)
PLATE = (18, 20, 22, 255)

EXPORT = [
    ("wheel-current", "obecna"),
    ("weapon-centurion", "centurion"),
    ("weapon-valkyrie", "valkyrie"),
    ("weapon-spikeshot", "spikeshot"),
    ("weapon-kasimov", "kasimov"),
]


def render_icon(icon_id: str, px: int, *, background: tuple | None) -> Image.Image:
    bg = background if background is not None else (0, 0, 0, 0)
    img = Image.new("RGBA", (px, px), bg)
    glow = Image.new("RGBA", (px, px), (0, 0, 0, 0))
    gdraw = ImageDraw.Draw(glow)
    scale = wwd.weapon_scale_for_px(px)
    lw = max(2, int(px / 28))
    cx = cy = px / 2
    wwd.draw_weapon_body(gdraw, cx, cy, scale, 0.0, icon_id, CYAN, line_width=lw + 2)
    glow = glow.filter(ImageFilter.GaussianBlur(max(1, px // 64)))
    img = Image.alpha_composite(img, glow)
    draw = ImageDraw.Draw(img)
    wwd.draw_weapon_body(draw, cx, cy, scale, 0.0, icon_id, CYAN, line_width=lw)
    return img


def patch_atlas(cells: dict[str, Image.Image]) -> None:
    meta = json.loads(ATLAS_JSON.read_text(encoding="utf-8"))
    atlas = Image.open(ATLAS_PNG).convert("RGBA")
    for rect in meta["rects"]:
        icon = cells.get(rect["id"])
        if not icon:
            continue
        tile = icon.resize((rect["w"], rect["h"]), Image.Resampling.LANCZOS)
        x, y = rect["x"], rect["y"]
        patch = Image.new("RGBA", (rect["w"], rect["h"]), (0, 0, 0, 0))
        patch.paste(tile, (0, 0), tile)
        atlas.paste(patch, (x, y), patch)
    atlas.save(ATLAS_PNG)
    digest = hashlib.sha256()
    digest.update(ATLAS_PNG.read_bytes())
    meta["png_sha256"] = digest.hexdigest()
    ATLAS_JSON.write_text(json.dumps(meta, indent=2, ensure_ascii=False) + "\n", encoding="utf-8")


def main() -> None:
    OUT_DIR.mkdir(parents=True, exist_ok=True)
    OUT_3D.mkdir(parents=True, exist_ok=True)
    cells_64: dict[str, Image.Image] = {}
    entries = []

    for icon_id, stem in EXPORT:
        im256 = render_icon(icon_id, 256, background=PLATE)
        im256.save(OUT_DIR / f"{stem}.png")
        render_icon(icon_id, 256, background=None).save(OUT_DIR / f"{stem}_alpha.png")
        render_icon(icon_id, 128, background=PLATE).save(OUT_DIR / f"{stem}_128.png")
        render_icon(icon_id, 256, background=None).save(OUT_3D / f"{icon_id}.png")
        print("wrote", OUT_DIR / f"{stem}.png", "(plate bg + glow)")
        cells_64[icon_id] = render_icon(icon_id, 64, background=None)
        entries.append(
            {
                "atlas_icon": icon_id,
                "file_plate": f"{stem}.png",
                "file_alpha": f"{stem}_alpha.png",
            }
        )

    sheet = Image.new("RGBA", (256 * len(EXPORT), 256), PLATE)
    for i, (icon_id, _) in enumerate(EXPORT):
        sheet.paste(render_icon(icon_id, 256, background=PLATE), (i * 256, 0))
    sheet.save(OUT_DIR / "sheet_all.png")

    patch_atlas(cells_64)

    MANIFEST.write_text(
        json.dumps(
            {
                "schema": "singularityvr-weapon-wheel-icons-v1",
                "note": "stem.png = ciemne tło do podglądu; stem_alpha.png = do gry. Skala: weapon_scale_for_px().",
                "color_rgb": list(CYAN),
                "icons": entries,
            },
            indent=2,
            ensure_ascii=False,
        )
        + "\n",
        encoding="utf-8",
    )
    print("wrote", MANIFEST)


if __name__ == "__main__":
    main()
