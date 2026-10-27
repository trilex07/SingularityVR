#!/usr/bin/env python3
"""Crop weapon wireframes from the holo wheel reference (user-provided PNG)."""
from __future__ import annotations

import hashlib
import json
import math
import shutil
from pathlib import Path

import numpy as np
from PIL import Image

ROOT = Path(__file__).resolve().parents[2]
REF_IN = (
    Path(r"C:\Users\Trilex\.cursor\projects\c-Users-Trilex-vr-mods\assets")
    / "c__Users_Trilex_AppData_Roaming_Cursor_User_workspaceStorage_7d01dd817cbb49c44236a9ba1cf98989_images_image-d483c29d-8d05-436d-89ca-8c26559374b2.png"
)
REF_STORE = ROOT / "assets" / "hud" / "katorga" / "wheel" / "reference" / "weapon-wheel-ref.png"
OUT_DIR = ROOT / "assets" / "hud" / "katorga" / "wheel" / "icons"
OUT_3D = ROOT / "assets" / "hud" / "katorga" / "3d" / "textures" / "wheel_icons"
ATLAS_PNG = ROOT / "assets" / "hud" / "katorga" / "atlas" / "katorga-hud-atlas.png"
ATLAS_JSON = ROOT / "assets" / "hud" / "katorga" / "atlas" / "atlas.json"

# sector index on 6-panel ref → export id
SECTOR_MAP = [
    (0, "weapon-centurion", "centurion", "Pistol / Centurion"),
    (1, "weapon-valkyrie", "valkyrie", "Assault rifle"),
    (5, "weapon-spikeshot", "spikeshot", "Shotgun"),
    (4, "weapon-kasimov", "kasimov", "Sniper"),
    (0, "wheel-current", "obecna", "Current (same crop as pistol)"),
]

# Normalized x0,y0,x1,y1 on reference wheel (655×655) — weapon art only.
MANUAL_CROP_FRAC: dict[int, tuple[float, float, float, float]] = {
    0: (0.47, 0.285, 0.66, 0.355),  # pistol (top)
    1: (0.60, 0.385, 0.78, 0.515),  # assault rifle
    5: (0.22, 0.285, 0.41, 0.355),  # shotgun
    4: (0.22, 0.455, 0.41, 0.565),  # sniper
}

N_SECTORS = 6
LUM_THRESH = 55


def load_ref() -> Image.Image:
    src = REF_IN if REF_IN.is_file() else REF_STORE
    if not src.is_file():
        raise FileNotFoundError(f"Reference wheel not found: {REF_IN} or {REF_STORE}")
    REF_STORE.parent.mkdir(parents=True, exist_ok=True)
    if src != REF_STORE:
        shutil.copy2(src, REF_STORE)
    return Image.open(REF_STORE).convert("RGBA")


def sector_masks(w: int, h: int) -> list[np.ndarray]:
    cx, cy = (w - 1) / 2.0, (h - 1) / 2.0
    yy, xx = np.mgrid[0:h, 0:w]
    dx = xx - cx
    dy = yy - cy
    r = np.hypot(dx, dy)
    ang = np.arctan2(dy, dx)
    r_norm = r / (min(w, h) * 0.5)
    masks = []
    for i in range(N_SECTORS):
        center = -math.pi / 2 + (i + 0.5) * (2 * math.pi / N_SECTORS)
        da = np.arctan2(np.sin(ang - center), np.cos(ang - center))
        in_wedge = np.abs(da) < (math.pi / N_SECTORS) * 0.72
        in_band = (r_norm > 0.345) & (r_norm < 0.455)
        masks.append(in_wedge & in_band)
    return masks


def bbox_from_mask(mask: np.ndarray, rgba: np.ndarray) -> tuple[int, int, int, int] | None:
    lum = rgba[..., :3].astype(np.int16).max(axis=2)
    hit = mask & (lum >= LUM_THRESH)
    if not hit.any():
        return None
    ys, xs = np.where(hit)
    pad = 4
    x0, y0 = int(xs.min()) - pad, int(ys.min()) - pad
    x1, y1 = int(xs.max()) + pad + 1, int(ys.max()) + pad + 1
    # Trim HUD chrome at bbox edges — keep central mass of line art.
    w, h = x1 - x0, y1 - y0
    mx, my = int(w * 0.12), int(h * 0.10)
    return (
        max(0, x0 + mx),
        max(0, y0 + my),
        min(rgba.shape[1], x1 - mx),
        min(rgba.shape[0], y1 - my),
    )


def to_alpha_icon(crop: Image.Image) -> Image.Image:
    rgba = np.array(crop.convert("RGBA"))
    r = rgba[..., 0].astype(np.int16)
    g = rgba[..., 1].astype(np.int16)
    b = rgba[..., 2].astype(np.int16)
    lum = np.maximum(np.maximum(r, g), b)
    # Drop orange HUD chrome; keep cyan/white line art from reference.
    orange_ui = (r > 150) & (g > 90) & (b < 130) & (r > b + 40)
    blue_line = (b >= r - 10) & (lum >= LUM_THRESH)
    keep = blue_line & ~orange_ui
    out = np.zeros_like(rgba)
    out[..., 0] = np.clip(r, 0, 255)
    out[..., 1] = np.clip(g, 0, 255)
    out[..., 2] = np.clip(b, 0, 255)
    out[..., 3] = np.where(keep, np.clip((lum - 30) * 5, 0, 255), 0).astype(np.uint8)
    out[~keep] = (0, 0, 0, 0)
    return Image.fromarray(out)


def fit_square(icon: Image.Image, px: int, plate: bool) -> Image.Image:
    icon = icon.convert("RGBA")
    w, h = icon.size
    side = max(w, h)
    canvas = Image.new("RGBA", (side, side), (0, 0, 0, 0))
    canvas.paste(icon, ((side - w) // 2, (side - h) // 2), icon)
    canvas = canvas.resize((px, px), Image.Resampling.LANCZOS)
    if plate:
        bg = Image.new("RGBA", (px, px), (18, 20, 22, 255))
        bg.paste(canvas, (0, 0), canvas)
        return bg
    return canvas


def patch_atlas(cells: dict[str, Image.Image]) -> None:
    meta = json.loads(ATLAS_JSON.read_text(encoding="utf-8"))
    atlas = Image.open(ATLAS_PNG).convert("RGBA")
    for rect in meta["rects"]:
        icon = cells.get(rect["id"])
        if not icon:
            continue
        tile = icon.resize((rect["w"], rect["h"]), Image.Resampling.LANCZOS)
        patch = Image.new("RGBA", (rect["w"], rect["h"]), (0, 0, 0, 0))
        patch.paste(tile, (0, 0), tile)
        atlas.paste(patch, (rect["x"], rect["y"]), patch)
    atlas.save(ATLAS_PNG)
    digest = hashlib.sha256()
    digest.update(ATLAS_PNG.read_bytes())
    meta["png_sha256"] = digest.hexdigest()
    ATLAS_JSON.write_text(json.dumps(meta, indent=2, ensure_ascii=False) + "\n", encoding="utf-8")


def main() -> None:
    im = load_ref()
    rgba = np.array(im)
    masks = sector_masks(im.width, im.height)
    crops: dict[int, Image.Image] = {}
    w, h = im.size
    for i, frac in MANUAL_CROP_FRAC.items():
        x0, y0, x1, y1 = frac
        box = (int(x0 * w), int(y0 * h), int(x1 * w), int(y1 * h))
        crops[i] = to_alpha_icon(im.crop(box))
        print("sector", i, "manual box", box, "size", crops[i].size)

    OUT_DIR.mkdir(parents=True, exist_ok=True)
    OUT_3D.mkdir(parents=True, exist_ok=True)
    cells_64: dict[str, Image.Image] = {}
    manifest_icons = []

    for sector_i, atlas_id, stem, note in SECTOR_MAP:
        if sector_i not in crops:
            raise RuntimeError(f"Missing crop for sector {sector_i}")
        base = crops[sector_i]
        fit_square(base, 256, plate=True).save(OUT_DIR / f"{stem}.png")
        fit_square(base, 256, plate=False).save(OUT_DIR / f"{stem}_alpha.png")
        fit_square(base, 128, plate=True).save(OUT_DIR / f"{stem}_128.png")
        fit_square(base, 256, plate=False).save(OUT_3D / f"{atlas_id}.png")
        cells_64[atlas_id] = fit_square(base, 64, plate=False)
        manifest_icons.append({"atlas_icon": atlas_id, "file": f"{stem}.png", "ref_sector": sector_i, "note": note})
        print("wrote", stem, "<- sector", sector_i)

    sheet = Image.new("RGBA", (256 * len(SECTOR_MAP), 256), (18, 20, 22, 255))
    for j, (_, _, stem, _) in enumerate(SECTOR_MAP):
        sheet.paste(Image.open(OUT_DIR / f"{stem}.png"), (j * 256, 0))
    sheet.save(OUT_DIR / "sheet_all.png")

    patch_atlas(cells_64)

    meta = {
        "schema": "singularityvr-weapon-wheel-icons-v1",
        "source": "wheel/reference/weapon-wheel-ref.png (user reference crop)",
        "method": "angular wedge mask + luminance bbox; ammo/text bands excluded",
        "icons": manifest_icons,
    }
    (OUT_DIR / "weapon_icons.json").write_text(json.dumps(meta, indent=2) + "\n", encoding="utf-8")
    print("patched atlas; manifest updated")


if __name__ == "__main__":
    main()
