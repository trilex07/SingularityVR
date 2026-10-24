"""Prepare private 2x Volk/glove DDS from verified original true mip0.

Interpolation increases sampling density, not recovered source detail. No
synthetic scratches or texture hallucination are added. Never point --output
at a Git checkout or a game installation.
"""
from __future__ import annotations

import argparse
import hashlib
import json
from pathlib import Path

import numpy as np
from PIL import Image, ImageDraw, ImageFilter

from dds_io import load_dds, resize_rgba_independent, save_dds, save_normal_dds, validate_dds

MAPS = {
    "AR_shotgun_d": ("RvGameMPLoader_SF", "diffuse", "DXT5", "6340e3245854c8cfa9d32dc9df3b82e7ba41dcf035b0c37219c08fe9a9983c65"),
    "AR_shotgun_n": ("RvGameMPLoader_SF", "normal", "DXT1", "76a7d6ee4f44682880f28b7e1ed5c76130fe6015162d1af2cf0872401b770834"),
    "inview_glove_d": ("RvGame", "diffuse", "DXT1", "e702e96cbae6ae73b24bf8e01969d0af41f15bcf1e04ccab2b8508435faff026"),
    "inview_glove_n": ("RvGame", "normal", "DXT1", "754c1b0617db838421c6e4b85fe0a9fbdfccfd14b7dd886a8cf516497c2cf3ca"),
}


def sha(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def safe_private(path: Path) -> None:
    path = path.resolve()
    if any(part.lower() in {"steam", "steamapps", "binaries"} for part in path.parts):
        raise ValueError("game or Steam destination refused")
    if any((parent / ".git").exists() for parent in (path, *path.parents)):
        raise ValueError("Git checkout destination refused")


def upscale_normal(image: Image.Image) -> Image.Image:
    rgb = np.asarray(image.convert("RGB"), dtype=np.float32) / 127.5 - 1.0
    channels = [Image.fromarray(np.uint8(np.clip((rgb[:, :, i] + 1.0) * 127.5, 0, 255)))
                .resize((image.width * 2, image.height * 2), Image.Resampling.LANCZOS)
                for i in range(3)]
    up = np.stack([np.asarray(ch, dtype=np.float32) / 127.5 - 1.0 for ch in channels], axis=2)
    up /= np.maximum(np.linalg.norm(up, axis=2, keepdims=True), 1e-6)
    return Image.fromarray(np.uint8(np.clip((up + 1.0) * 127.5 + 0.5, 0, 255)), "RGB")


def upscale_diffuse(image: Image.Image) -> Image.Image:
    clean = resize_rgba_independent(image, (image.width * 2, image.height * 2), Image.Resampling.LANCZOS)
    # Mild edge clarity, thresholded to avoid amplifying DXT block noise.
    rgb = clean.convert("RGB").filter(ImageFilter.UnsharpMask(radius=1.0, percent=38, threshold=5))
    result = rgb.convert("RGBA")
    result.putalpha(clean.getchannel("A"))
    return result


def preview(original: Image.Image, remaster: Image.Image, output: Path, label: str) -> None:
    # Show equal physical UV coverage at both sampling densities.
    box = (original.width // 4, original.height // 4, original.width * 3 // 4, original.height * 3 // 4)
    crop_a = original.crop(box).resize((512, 512), Image.Resampling.NEAREST)
    crop_b = remaster.crop(tuple(v * 2 for v in box)).resize((512, 512), Image.Resampling.BILINEAR)
    sheet = Image.new("RGB", (1024, 540), (18, 20, 24))
    sheet.paste(crop_a.convert("RGB"), (0, 28))
    sheet.paste(crop_b.convert("RGB"), (512, 28))
    pen = ImageDraw.Draw(sheet)
    pen.text((8, 6), f"{label}: source mip0", fill="white")
    pen.text((520, 6), "2x private candidate", fill="white")
    output.parent.mkdir(parents=True, exist_ok=True)
    sheet.save(output)


def build(local: Path, output: Path) -> dict:
    safe_private(output)
    base = local / "material-remaster-d3d9ex" / "first-four"
    report: dict = {"status": "OFFLINE_ONLY", "method": "2x Lanczos of original true mip0; restrained thresholded RGB sharpening; signed normalized normal vectors", "maps": {}}
    # Validate every input before writing any output.
    for name, (package, role, fmt, expected) in MAPS.items():
        source = base / f"{name}.dds"
        original = local / "textures" / "original" / package / "Texture2D" / f"{name}.dds"
        if sha(source) != expected:
            raise ValueError(f"reviewed source hash changed: {name}")
        source_img, sh = load_dds(source)
        original_img, oh = load_dds(original)
        if (sh["width"], sh["height"], sh["fourcc"]) != (1024, 1024, fmt):
            raise ValueError(f"reviewed source layout changed: {name}")
        if (oh["width"], oh["height"], oh["fourcc"]) != (1024, 1024, fmt):
            raise ValueError(f"original mip0 layout changed: {name}")
        report["maps"][name] = {"role": role, "material": "AR_Shotgun" if name.startswith("AR_") else "inview_gloves_gauze", "source_sha256": sha(original), "reviewed_remaster_sha256": expected}
    output.mkdir(parents=True, exist_ok=True)
    for name, (package, role, fmt, _) in MAPS.items():
        source = base / f"{name}.dds"
        original = local / "textures" / "original" / package / "Texture2D" / f"{name}.dds"
        original_img, _ = load_dds(original)
        enlarged = upscale_normal(original_img) if role == "normal" else upscale_diffuse(original_img)
        dest = output / f"{name}.dds"
        if dest.exists():
            if dest.stat().st_size == 0:
                raise ValueError(f"existing empty candidate refused: {dest}")
            continue  # Never silently overwrite a private candidate.
        if role == "normal":
            save_normal_dds(dest, enlarged)
        else:
            save_dds(dest, enlarged, fmt)
        check = validate_dds(dest)
        if (check["width"], check["height"], check["fourcc"], check["mip_count"]) != (2048, 2048, fmt, 12):
            raise ValueError(f"output validation failed: {name}")
        preview(original_img, enlarged, output / "previews" / f"{name}.png", name)
    for name in MAPS:
        dest = output / f"{name}.dds"
        report["maps"][name].update({"file": str(dest), "sha256": sha(dest), "dds": validate_dds(dest)})
    (output / "manifest.private.json").write_text(json.dumps(report, indent=2), encoding="utf-8")
    return report


if __name__ == "__main__":
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument("--local", type=Path, required=True)
    p.add_argument("--output", type=Path, required=True)
    a = p.parse_args()
    result = build(a.local, a.output)
    print(json.dumps({k: {"sha256": v["sha256"], "dds": {x: v["dds"][x] for x in ("width", "height", "fourcc", "mip_count")}} for k, v in result["maps"].items()}, indent=2))
