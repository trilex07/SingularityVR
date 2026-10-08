#!/usr/bin/env python3
"""
Build the SingularityVfx atlases from the CC0 source packs.

Produces, in --out:
  vfx_muzzle.dds       additive muzzle flash, 4x4 grid (3 frames used)
  vfx_smoke_white.dds  alpha white smoke,      4x4 grid (16 frames)
  vfx_smoke_black.dds  alpha black smoke,      4x4 grid (16 frames)
  vfx_blood.dds        alpha blood,            4x4 grid (7 frames used)
plus PNG previews and vfx_atlas_manifest.json.

All atlases are 1024x1024 RGBA DXT5 with a full mip chain, so the D3D9 host can
upload them with a tiny raw-DDS loader (no PNG decoder, no d3dx).

Frames are trimmed to their alpha content and centred, and horizontal source
strips (the Reactorcore muzzle sheets) are sliced into individual frames.

Sources (CC0, recorded in downloads/vfx-20261008/README.md):
  Reactorcore muzzle effects, Kenney smoke particles, para blood hit splashes.
Nothing here is game content; the repo only tracks this script and the manifest.
"""
from __future__ import annotations

import argparse
import io
import json
import struct
from pathlib import Path

from PIL import Image

CELL = 256
GRID = 4
ATLAS = CELL * GRID  # 1024


def open_rgba(path: Path) -> Image.Image:
    return Image.open(path).convert("RGBA")


def slice_strip(img: Image.Image, frames: int) -> list[Image.Image]:
    """Split a horizontal strip into `frames` equal columns (Reactorcore sheets)."""
    if frames <= 1:
        return [img]
    w, h = img.size
    step = w / frames
    return [img.crop((int(round(i * step)), 0, int(round((i + 1) * step)), h)) for i in range(frames)]


def auto_slice(img: Image.Image) -> list[Image.Image]:
    w, h = img.size
    n = max(1, int(round(w / h))) if h > 0 and w >= h * 1.4 else 1
    return slice_strip(img, n)


def trim(img: Image.Image) -> Image.Image:
    bbox = img.getbbox()
    if bbox:
        pad = 2
        bbox = (max(0, bbox[0] - pad), max(0, bbox[1] - pad),
                min(img.width, bbox[2] + pad), min(img.height, bbox[3] + pad))
        img = img.crop(bbox)
    return img


def fit_cell(img: Image.Image, over_black: bool) -> Image.Image:
    """Trim, scale to fit ~92% of the cell, and centre it on a cell canvas."""
    img = trim(img)
    limit = int(CELL * 0.92)
    scale = min(limit / max(1, img.width), limit / max(1, img.height))
    new = (max(1, int(img.width * scale)), max(1, int(img.height * scale)))
    img = img.resize(new, Image.LANCZOS)

    bg = Image.new("RGBA", (CELL, CELL), (0, 0, 0, 255) if over_black else (0, 0, 0, 0))
    if over_black:
        bg.alpha_composite(img, ((CELL - new[0]) // 2, (CELL - new[1]) // 2))
        return bg
    bg.paste(img, ((CELL - new[0]) // 2, (CELL - new[1]) // 2), img)
    return bg


def compose(frames: list[Image.Image], over_black: bool) -> Image.Image:
    limit = GRID * GRID
    if len(frames) > limit:
        frames = frames[:limit]
    atlas = Image.new("RGBA", (ATLAS, ATLAS), (0, 0, 0, 255 if over_black else 0))
    for i, f in enumerate(frames):
        atlas.paste(fit_cell(f, over_black), ((i % GRID) * CELL, (i // GRID) * CELL))
    return atlas


def encode_level(img: Image.Image, fmt: str) -> bytes:
    buf = io.BytesIO()
    img.convert("RGBA").save(buf, format="DDS", pixel_format=fmt)
    return buf.getvalue()[128:]


def save_dds_mips(path: Path, mips: list[Image.Image], fourcc: str = "DXT5") -> None:
    w, h = mips[0].size
    hdr = bytearray(128)
    hdr[0:4] = b"DDS "
    struct.pack_into("<I", hdr, 4, 124)
    struct.pack_into("<I", hdr, 8, 0x1 | 0x2 | 0x4 | 0x1000 | 0x80000)
    struct.pack_into("<I", hdr, 12, h)
    struct.pack_into("<I", hdr, 16, w)
    struct.pack_into("<I", hdr, 20, 0)
    struct.pack_into("<I", hdr, 24, 0)
    struct.pack_into("<I", hdr, 28, len(mips))
    struct.pack_into("<I", hdr, 76, 32)
    struct.pack_into("<I", hdr, 80, 0x4)
    hdr[84:88] = fourcc.encode("latin1")
    struct.pack_into("<I", hdr, 108, 0x1000)
    path.parent.mkdir(parents=True, exist_ok=True)
    with path.open("wb") as f:
        f.write(hdr)
        for m in mips:
            f.write(encode_level(m, fourcc))


def mips_of(img: Image.Image, floor: int = 4) -> list[Image.Image]:
    out, cur = [], img
    while True:
        out.append(cur)
        if max(cur.size) <= floor:
            break
        cur = cur.resize((max(1, cur.width // 2), max(1, cur.height // 2)), Image.LANCZOS)
    return out


def main() -> None:
    ap = argparse.ArgumentParser()
    ap.add_argument("--prepared", default=r"C:\Users\Trilex\vr-mods\downloads\vfx-20261008\prepared")
    ap.add_argument("--out", default=r"C:\Users\Trilex\vr-mods\downloads\vfx-20261008\built")
    a = ap.parse_args()
    prep, out = Path(a.prepared), Path(a.out)
    out.mkdir(parents=True, exist_ok=True)

    # Muzzle: one 3-frame Reactorcore strip (clean 80px frames).
    muzzle_src = open_rgba(prep / "muzzle" / "reactorcore" / "Muzzle Large Fire.png")
    muzzle = auto_slice(muzzle_src)

    def frames_from(folder: Path, glob: str, n: int) -> list[Image.Image]:
        files = sorted(folder.glob(glob))[:n]
        return [open_rgba(f) for f in files]

    groups = {
        "vfx_muzzle": (muzzle, True),
        "vfx_smoke_white": (frames_from(prep / "smoke" / "kenney" / "White puff", "*.png", 16), False),
        "vfx_smoke_black": (frames_from(prep / "smoke" / "kenney" / "Black smoke", "*.png", 16), False),
        # Single-frame splats and trails (the para blood_hit_* files are 16-frame
        # sheets, so they are not used directly here).
        "vfx_blood": (
            [open_rgba(p) for p in
             sorted((prep / "blood" / "bloodsplat-animation" / "blooddropsplash").glob("splattersplash_*.png"))
             + sorted((prep / "blood" / "bloodsplat-animation").glob("bloodtrail_*.png"))][:16],
            False,
        ),
    }

    counts = {}
    for name, (frames, over_black) in groups.items():
        if not frames:
            raise SystemExit(f"no source frames for {name}")
        atlas = compose(frames, over_black)
        atlas.save(out / f"{name}.png")
        save_dds_mips(out / f"{name}.dds", mips_of(atlas), "DXT5")
        counts[name] = min(len(frames), GRID * GRID)
        print(f"{name}: {counts[name]} frames -> {name}.dds")

    manifest = {
        "version": 1,
        "abi": 1,
        "note": "Generated by make_vfx_atlases.py from CC0 packs. Atlases are 1024x1024 DXT5, 4x4 grid.",
        "grid": [GRID, GRID],
        "atlases": [
            {"id": "muzzle", "file": "vfx_muzzle.dds", "blend": "additive"},
            {"id": "smoke_white", "file": "vfx_smoke_white.dds", "blend": "alpha"},
            {"id": "smoke_black", "file": "vfx_smoke_black.dds", "blend": "alpha"},
            {"id": "blood", "file": "vfx_blood.dds", "blend": "alpha"},
        ],
        "clips": [
            {"name": "MuzzleFireSmall", "clipId": 0, "atlas": "muzzle", "grid": [GRID, GRID], "frames": 3, "fps": 30, "blend": "additive"},
            {"name": "MuzzleFireLarge", "clipId": 1, "atlas": "muzzle", "grid": [GRID, GRID], "frames": 3, "fps": 30, "blend": "additive"},
            {"name": "MuzzleFireHeavy", "clipId": 2, "atlas": "muzzle", "grid": [GRID, GRID], "frames": 3, "fps": 24, "blend": "additive"},
            {"name": "MuzzleFireEnergy", "clipId": 3, "atlas": "muzzle", "grid": [GRID, GRID], "frames": 3, "fps": 30, "blend": "additive"},
            {"name": "SmokeWhite", "clipId": 4, "atlas": "smoke_white", "grid": [GRID, GRID], "frames": 16, "fps": 30, "blend": "alpha"},
            {"name": "SmokeBlack", "clipId": 5, "atlas": "smoke_black", "grid": [GRID, GRID], "frames": 16, "fps": 30, "blend": "alpha"},
            {"name": "BloodSplash", "clipId": 6, "atlas": "blood", "grid": [GRID, GRID], "frames": 6, "fps": 30, "blend": "alpha"},
            {"name": "BloodHitAlt", "clipId": 7, "atlas": "blood", "grid": [GRID, GRID], "frames": 14, "fps": 30, "blend": "alpha"},
        ],
    }
    (out / "vfx_atlas_manifest.json").write_text(json.dumps(manifest, indent=2) + "\n", encoding="utf-8")
    print(f"wrote {out / 'vfx_atlas_manifest.json'}")


if __name__ == "__main__":
    main()
