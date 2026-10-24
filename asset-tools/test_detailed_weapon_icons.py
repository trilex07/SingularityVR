"""Pixel contract and actual-size visual QA for the M02 P2 icon-only update."""

from __future__ import annotations

import hashlib
import io
import json
from pathlib import Path
import subprocess

import numpy as np
from PIL import Image, ImageDraw, ImageFont

import build_detailed_weapon_icons as art


ROOT = art.ROOT
PREVIEW = ROOT / "docs/previews/ui_remaster"
NAMES = list(art.CONTENT_HEIGHT)
EXPECTED_RECTS = {
    "centurion": (1976, 852, 240, 110),
    "valkyrie": (2224, 852, 240, 110),
    "spikeshot": (2688, 852, 240, 107),
    "kasimov": (3608, 852, 240, 86),
}


def original(relative: str) -> bytes:
    return subprocess.check_output(
        ["git", "-c", f"safe.directory={ROOT.as_posix()}", "show", f"{art.BASE_COMMIT}:{relative}"],
        cwd=ROOT,
    )


def label(draw: ImageDraw.ImageDraw, x: int, y: int, text: str) -> None:
    font = ImageFont.truetype("C:/Windows/Fonts/arial.ttf", 18)
    draw.text((x, y), text, font=font, fill=(226, 220, 201, 255))


def background(selected: bool = False) -> Image.Image:
    tile = Image.new("RGBA", (270, 172), (30, 39, 48, 255))
    draw = ImageDraw.Draw(tile)
    draw.rounded_rectangle((50, 8, 220, 160), radius=18, fill=(43, 49, 54, 255),
                           outline=(229, 142, 46, 255) if selected else (91, 105, 113, 255), width=3)
    return tile


def tile_icon(sprite: Image.Image, selected: bool) -> Image.Image:
    tile = background(selected)
    icon = sprite.resize((144, 144), Image.Resampling.BILINEAR)
    tile.alpha_composite(icon, (63, 14))
    return tile


def previews(old: dict[str, Image.Image], new: dict[str, Image.Image]) -> None:
    PREVIEW.mkdir(parents=True, exist_ok=True)
    grid = Image.new("RGBA", (4 * 300 + 30, 4 * 194 + 45), (20, 27, 33, 255))
    draw = ImageDraw.Draw(grid)
    for column, weapon in enumerate(NAMES):
        x = 20 + column * 300
        label(draw, x, 6, weapon.upper())
        for row, (sprite, selected, state) in enumerate([
            (old[weapon], False, "old / idle"),
            (new[weapon], False, "new / idle"),
            (old[weapon], True, "old / selected"),
            (new[weapon], True, "new / selected"),
        ]):
            y = 35 + row * 194
            grid.alpha_composite(tile_icon(sprite, selected), (x, y))
            label(draw, x + 8, y + 170, state)
    grid.convert("RGB").save(PREVIEW / "detailed_weapon_icons_compare.png")

    wheel = Image.new("RGBA", (800, 800), (27, 32, 36, 255))
    chassis = Image.open(art.SPRITES / "wheel_chassis_outer.png").convert("RGBA")
    wheel.alpha_composite(chassis)
    for weapon, pos in zip(NAMES, ((328, 55), (600, 328), (328, 600), (55, 328))):
        icon = new[weapon].resize((144, 144), Image.Resampling.BILINEAR)
        wheel.alpha_composite(icon, pos)
    wheel.convert("RGB").save(PREVIEW / "detailed_weapon_wheel_preview.png")


def main() -> None:
    mapping = json.loads(art.MAPPING.read_text(encoding="utf-8"))
    manifest = json.loads(art.MANIFEST.read_text(encoding="utf-8"))
    base_atlas_bytes = original("assets/processed/ui_remaster/atlas/ui_atlas_neutral.png")
    old_atlas = np.asarray(Image.open(io.BytesIO(base_atlas_bytes)).convert("RGBA"))
    new_atlas = np.asarray(Image.open(art.ATLAS_PNG).convert("RGBA"))
    assert new_atlas.shape == old_atlas.shape == (2048, 4096, 4)
    assert mapping == json.loads(original("assets/processed/ui_remaster/atlas/ui_atlas_neutral.json")), "UV manifest changed"
    allowed = np.zeros(old_atlas.shape[:2], dtype=bool)
    old_sprites: dict[str, Image.Image] = {}
    new_sprites: dict[str, Image.Image] = {}
    signatures = set()
    for weapon in NAMES:
        rect = tuple(mapping["elements"][f"weapon_icon_{weapon}.png"]["pixel_rect"])
        assert rect == EXPECTED_RECTS[weapon], (weapon, rect)
        x, y, w, h = rect
        assert mapping["elements"][f"weapon_icon_{weapon}.png"]["uv_rect"] == [
            round(x / 4096, 6), round(y / 2048, 6), round((x + w) / 4096, 6), round((y + h) / 2048, 6)]
        sprite = Image.open(art.SPRITES / f"weapon_icon_{weapon}.png").convert("RGBA")
        rebuilt = art.make_sprite(art.SOURCE / f"weapon_icon_{weapon}_master.png",
                                   w, h, art.CONTENT_HEIGHT[weapon])
        assert np.array_equal(np.asarray(sprite), np.asarray(rebuilt)), "source pipeline drift"
        old = Image.open(io.BytesIO(original(f"assets/processed/ui_remaster/weapon_wheel/weapon_icon_{weapon}.png"))).convert("RGBA")
        assert sprite.size == old.size == (w, h)
        assert np.array_equal(np.asarray(sprite), new_atlas[y:y+h, x:x+w])
        assert (np.asarray(sprite)[:, :, 3] > 24).sum() > 800, weapon
        assert np.asarray(sprite)[:, :5, 3].max() == 0
        assert np.asarray(sprite)[:, -5:, 3].max() == 0
        allowed[y:y+h, x:x+w] = True
        old_sprites[weapon], new_sprites[weapon] = old, sprite
        signature = hashlib.sha256(sprite.tobytes()).hexdigest()
        assert signature not in signatures
        signatures.add(signature)
        assert manifest["weapons"][weapon]["atlas_rect"] == list(rect)
        assert manifest["weapons"][weapon]["sprite_sha256"] == art.sha256(art.SPRITES / f"weapon_icon_{weapon}.png")
    assert np.array_equal(old_atlas[~allowed], new_atlas[~allowed]), "unrelated atlas pixels changed"
    assert np.any(old_atlas[allowed] != new_atlas[allowed]), "icons did not change"
    for language in ("pl", "en"):
        relative = f"assets/processed/ui_remaster/atlas/ui_atlas_localized_{language}.png"
        assert (ROOT / relative).read_bytes() == original(relative), "localized atlas changed"
    assert manifest["old_atlas_sha256"] == hashlib.sha256(base_atlas_bytes).hexdigest().upper()
    assert manifest["new_atlas_sha256"] == art.sha256(art.ATLAS_PNG)
    previews(old_sprites, new_sprites)
    print("PASS: 4 distinct sprites, exact original UVs, source/atlas pixel match, zero non-icon pixel changes")
    print("PASS: PL/EN atlases byte-identical to accepted baseline; 144px selected/idle previews written")


if __name__ == "__main__":
    main()
