#!/usr/bin/env python3
"""2x resolution showcase for remastered diffuse maps (offline, local only).

Takes the ALREADY GENERATED enhanced-native DDS files, Lanczos-upscales them
2x, and builds comparison sheets:

    ORIGINAL 1x (nearest 2x) | LANCZOS 2x of original | REMASTER 2x

Honesty note (also written into each sheet manifest entry): 2x Lanczos adds
pixels, not information above the original Nyquist frequency. The genuine new
content is the remaster micro-detail layer; the 2x step only presents it at a
higher sampling rate for close VR inspection. Files are named *_2X_LANCZOS.dds
and are NOT injector substitutes.

Outputs stay outside the repo:
  <assets-local>/material-remaster-2x/<key>/{*.dds, comparison/*.png}
"""

from __future__ import annotations

import hashlib
import json
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
from dds_io import load_dds, read_dds_header, save_dds  # noqa: E402

from PIL import Image, ImageDraw

LOCAL_ROOT = Path(r"C:\Users\Trilex\vr-mods\SingularityVR-assets-local")
B2 = LOCAL_ROOT / "material-remaster-batch2"
OUT = LOCAL_ROOT / "material-remaster-2x"

# (batch2 key, enhanced native file, original source file, fourcc)
TARGETS = [
    ("weapon_e99", "AR_E99Rifle_alt_d_2x.dds",
     r"textures\original\RvGame\Texture2D\AR_E99Rifle_alt_d.dds", "DXT5"),
    ("weapon_gatling", "ar_gatling_gun_d_2x.dds",
     r"textures\original\RvGameMPLoader_SF\Texture2D\ar_gatling_gun_d.dds", "DXT5"),
    ("enemy_revert", "revert_d2_2x.dds",
     r"textures\original\RvGameMPLoader_SF\Texture2D\revert_d2.dds", "DXT1"),
]


def sha256_file(path: Path) -> str:
    h = hashlib.sha256()
    with path.open("rb") as f:
        for chunk in iter(lambda: f.read(1 << 20), b""):
            h.update(chunk)
    return h.hexdigest()


def main() -> None:
    summary = {"variants": [], "status": "OFFLINE ONLY - NOT VR TESTED",
               "method": "LANCZOS 2x of the enhanced native replacement "
                         "(which derives from true mip0). Interpolation only: "
                         "no content above the original Nyquist frequency is genuine."}
    for key, enh_name, src_rel, fourcc in TARGETS:
        print(f"=== {key} ===", flush=True)
        enh_path = B2 / key / enh_name
        src_path = LOCAL_ROOT / src_rel
        if not enh_path.exists():
            # fall back to whatever injector-named diffuse exists (never a showcase file)
            cands = sorted((B2 / key).glob("*_d_2x.dds"))
            cands = [c for c in cands if "LANCZOS" not in c.name and "nodesc" not in c.name]
            if not cands:
                print(f"  SKIP: no enhanced diffuse for {key}")
                continue
            enh_path = cands[0]
        enh, _ = load_dds(enh_path)
        orig, _ = load_dds(src_path)

        up_enh = enh.resize((enh.width * 2, enh.height * 2), Image.LANCZOS)
        up_orig = orig.resize((orig.width * 2, orig.height * 2), Image.LANCZOS)

        out_dir = OUT / key
        out_dir.mkdir(parents=True, exist_ok=True)
        stem = enh_path.stem.replace("_diffuse_2x", "").replace("_d_2x", "")
        dds_path = out_dir / f"{stem}_diffuse_2X_LANCZOS.dds"
        payload = up_enh if fourcc == "DXT5" else up_enh.convert("RGB")
        hdr = save_dds(dds_path, payload, fourcc)
        print(f"  wrote {dds_path.name}: {hdr['width']}x{hdr['height']} "
              f"{hdr['fourcc']} mips={hdr['mip_count']} bytes={hdr['file_size']}", flush=True)

        # comparison sheet on a detail-rich crop
        w, h = orig.size
        crop = 256
        x0, y0 = (w - crop) // 2, (h - crop) // 2
        box = (x0, y0, x0 + crop, y0 + crop)
        box2 = (x0 * 2, y0 * 2, (x0 + crop) * 2, (y0 + crop) * 2)
        tiles = [
            (orig.crop(box).resize((crop * 2, crop * 2), Image.NEAREST),
             "ORIGINAL 1x (2x nearest)"),
            (up_orig.crop(box2), "LANCZOS 2x of original"),
            (up_enh.crop(box2), "REMASTER 2x (Lanczos of enhanced)"),
        ]
        sheet = Image.new("RGB", (crop * 2, crop * 2 * 3 + 40), (18, 18, 22))
        d = ImageDraw.Draw(sheet)
        y = 0
        for im, label in tiles:
            sheet.paste(im.convert("RGB"), (0, y))
            d.text((6, y + crop * 2 - 18),
                   f"{stem} diffuse - {label}", fill=(230, 230, 230))
            y += crop * 2 + 10
        png_path = out_dir / "comparison" / f"{stem}_diffuse_2x_compare.png"
        png_path.parent.mkdir(parents=True, exist_ok=True)
        sheet.save(png_path)
        print(f"  comparison sheet: {png_path.name}", flush=True)

        summary["variants"].append({
            "key": key, "source": str(src_path).replace(str(LOCAL_ROOT), "<assets-local>"),
            "enhanced_native": str(enh_path).replace(str(LOCAL_ROOT), "<assets-local>"),
            "upscaled_file": str(dds_path).replace(str(LOCAL_ROOT), "<assets-local>"),
            "upscaled_sha256": sha256_file(dds_path),
            "header": {"width": hdr["width"], "height": hdr["height"],
                       "fourcc": hdr["fourcc"], "mip_count": hdr["mip_count"],
                       "file_bytes": hdr["file_size"]},
            "comparison": str(png_path).replace(str(LOCAL_ROOT), "<assets-local>"),
            "substituted": False,
        })
    (OUT / "manifest_summary.json").write_text(json.dumps(summary, indent=2), encoding="utf-8")
    print("\ndone. local output:", OUT)


if __name__ == "__main__":
    main()
