"""Asset-only M02 P2 icon update using the accepted atlas UV rectangles.

The renderer draws each source rectangle into a square. The narrower source
heights below compensate for that accepted transform without changing code.
"""

from __future__ import annotations

import hashlib
import io
import json
from pathlib import Path
import subprocess

import numpy as np
from PIL import Image


ROOT = Path(__file__).resolve().parents[1]
SOURCE = ROOT / "assets/source/ui_remaster/weapon_wheel"
SPRITES = ROOT / "assets/processed/ui_remaster/weapon_wheel"
ATLAS = ROOT / "assets/processed/ui_remaster/atlas"
MAPPING = ATLAS / "ui_atlas_neutral.json"
ATLAS_PNG = ATLAS / "ui_atlas_neutral.png"
MANIFEST = ATLAS / "detailed_weapon_icons_manifest.json"
BASE_COMMIT = "e379b7afbcc40eb6e2bf6331b49feee833330820"

# Content heights are deliberately below the atlas rectangle heights because
# DrawUiRemasterWheelEye maps each complete rectangle onto a square icon quad.
CONTENT_HEIGHT = {
    "centurion": 64,
    "valkyrie": 43,
    "spikeshot": 50,
    "kasimov": 29,
}


def sha256(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest().upper()


def make_sprite(master: Path, width: int, height: int, content_height: int) -> Image.Image:
    source = Image.open(master).convert("RGBA")
    raw = np.asarray(source)
    mask = raw[:, :, 3] > 24
    ys, xs = np.where(mask)
    if len(xs) == 0:
        raise ValueError(f"empty master image: {master}")
    crop = source.crop((int(xs.min()), int(ys.min()), int(xs.max()) + 1, int(ys.max()) + 1))
    content_width = width - 12
    # Intentional vertical pre-compression compensates for the square draw quad.
    crop = crop.resize((content_width, content_height), Image.Resampling.LANCZOS)
    rgba = np.array(crop, dtype=np.uint8, copy=True)
    rgb = rgba[:, :, :3].astype(np.float32)
    luminance = rgb[:, :, 0] * 0.299 + rgb[:, :, 1] * 0.587 + rgb[:, :, 2] * 0.114
    # Katorga ivory-steel palette with a visible dark mechanical interior.
    tone = np.clip(105 + luminance * 0.57, 105, 240)
    rgba[:, :, 0] = np.clip(tone + 7, 0, 255).astype(np.uint8)
    rgba[:, :, 1] = np.clip(tone + 2, 0, 255).astype(np.uint8)
    rgba[:, :, 2] = np.clip(tone - 10, 0, 255).astype(np.uint8)
    content = Image.fromarray(rgba, "RGBA")
    sprite = Image.new("RGBA", (width, height), (0, 0, 0, 0))
    sprite.paste(content, (6, (height - content_height) // 2))
    return sprite


def main() -> None:
    mapping = json.loads(MAPPING.read_text(encoding="utf-8"))
    baseline_bytes = subprocess.check_output(
        ["git", "-c", f"safe.directory={ROOT.as_posix()}", "show",
         f"{BASE_COMMIT}:assets/processed/ui_remaster/atlas/ui_atlas_neutral.png"],
        cwd=ROOT,
    )
    before = np.asarray(Image.open(io.BytesIO(baseline_bytes)).convert("RGBA"), dtype=np.uint8).copy()
    after = before.copy()
    touched = np.zeros(before.shape[:2], dtype=bool)
    old_atlas_sha = hashlib.sha256(baseline_bytes).hexdigest().upper()
    items = {}
    for weapon, content_height in CONTENT_HEIGHT.items():
        name = f"weapon_icon_{weapon}.png"
        x, y, w, h = mapping["elements"][name]["pixel_rect"]
        if (w, h) != Image.open(SPRITES / name).size:
            raise ValueError(f"source size drift: {name}")
        image = make_sprite(SOURCE / f"weapon_icon_{weapon}_master.png", w, h, content_height)
        image.save(SPRITES / name)
        pixels = np.asarray(image)
        after[y:y+h, x:x+w] = pixels
        touched[y:y+h, x:x+w] = True
        items[weapon] = {
            "atlas_rect": [x, y, w, h],
            "source_master_sha256": sha256(SOURCE / f"weapon_icon_{weapon}_master.png"),
            "sprite_sha256": sha256(SPRITES / name),
            "content_height": content_height,
        }
    if not np.array_equal(before[~touched], after[~touched]):
        raise AssertionError("unrelated atlas pixels changed")
    Image.fromarray(after, "RGBA").save(ATLAS_PNG)
    for weapon, item in items.items():
        x, y, w, h = item["atlas_rect"]
        if not np.array_equal(after[y:y+h, x:x+w], np.asarray(Image.open(SPRITES / f"weapon_icon_{weapon}.png"))):
            raise AssertionError(f"atlas source mismatch: {weapon}")
    manifest = {
        "base_commit": BASE_COMMIT,
        "atlas_name": "ui_atlas_neutral.png",
        "atlas_dimensions": list(Image.open(ATLAS_PNG).size),
        "old_atlas_sha256": old_atlas_sha,
        "new_atlas_sha256": sha256(ATLAS_PNG),
        "uv_manifest_sha256": sha256(MAPPING),
        "changed_rectangles_only": True,
        "weapons": items,
    }
    MANIFEST.write_text(json.dumps(manifest, indent=2) + "\n", encoding="utf-8")
    print(json.dumps({"atlas": manifest["new_atlas_sha256"], "sprites": len(items)}, indent=2))


if __name__ == "__main__":
    main()
