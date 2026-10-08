#!/usr/bin/env python3
"""
Replace the game's Xbox button-glyph textures (ui_icons_360_SF 360_*) with
PlayStation icons, using the runtime texture injector.

For each source glyph this:
  * exports the original DDS (to get its exact mip0 hash + dims/format),
  * builds a 64x64 DXT5 PlayStation replacement from the Kenney CC0 pack,
  * writes it as <name>__B_ibp.dds (build.ps1 deploys it as <name>.dds),
  * appends an entry to upscale_results.json so `targets` emits the injector row.

The substitutions are content-hash keyed, so only the real button glyphs match.
"""
from __future__ import annotations

import argparse
import json
import struct
from pathlib import Path

from PIL import Image

import texture_pipeline as tp

SRC_PKG = "ui_icons_360_SF"

# 360 glyph -> Kenney PlayStation icon (same action, PS button)
KENNEY = r"C:\Users\Trilex\vr-mods\singularity-vr-mod\assets\ui\kenney-input-prompts\source\PlayStation Series\Default"
MAPPING = {
    "360_A":      "playstation_button_cross.png",
    "360_B":      "playstation_button_circle.png",
    "360_X":      "playstation_button_square.png",
    "360_Y":      "playstation_button_triangle.png",
    "360_LB":     "playstation_trigger_l1.png",
    "360_RB":     "playstation_trigger_r1.png",
    "360_LT":     "playstation_trigger_l2.png",
    "360_RT":     "playstation_trigger_r2.png",
    "360_L3":     "playstation_button_l3.png",
    "360_R3":     "playstation_button_r3.png",
    "360_LS":     "playstation_stick_l.png",
    "360_RS":     "playstation_stick_r.png",
    "360_DPad":   "playstation_dpad_all.png",
    "360_DPAD_U": "playstation_dpad_up.png",
    "360_DPAD_D": "playstation_dpad_down.png",
    "360_DPAD_L": "playstation_dpad_left.png",
    "360_DPAD_R": "playstation_dpad_right.png",
    "360_Start":  "playstation5_button_options.png",
    "360_Back":   "playstation5_button_create.png",
}


def mips(img: Image.Image, floor: int = 4):
    out, cur = [], img
    while True:
        out.append(cur)
        if max(cur.size) <= floor:
            break
        cur = cur.resize((max(1, cur.width // 2), max(1, cur.height // 2)), Image.LANCZOS)
    return out


def main() -> None:
    ap = argparse.ArgumentParser()
    ap.add_argument("--game", default=str(tp.DEFAULT_GAME))
    ap.add_argument("--local-root", default=str(tp.DEFAULT_LOCAL))
    ap.add_argument("--umodel", default=str(tp.DEFAULT_UMODEL))
    a = ap.parse_args()

    game, local, umodel = Path(a.game), Path(a.local_root), Path(a.umodel)
    scratch = local / "cooked"
    dds_dir = local / "textures" / "original"
    png_dir = local / "textures" / "original-png"
    out_root = local / "textures" / "upscaled"
    for p in (dds_dir, png_dir):
        p.mkdir(parents=True, exist_ok=True)
    tp.ensure_scratch(game, scratch, umodel)

    up_path = tp.MANIFEST_DIR / "upscale_results.json"
    results = json.loads(up_path.read_text(encoding="utf-8")) if up_path.exists() else {}

    made = 0
    for name, ps_file in MAPPING.items():
        info = tp.umodel_export(umodel, scratch, dds_dir, SRC_PKG, name, "dds")
        if not info.get("ok"):
            tp.log(f"glyph {name}: source export failed - skipped")
            continue
        src = next(iter(dds_dir.rglob(f"{name}.dds")), None)
        if not src:
            tp.log(f"glyph {name}: exported dds not found")
            continue

        b = src.read_bytes()
        fourcc = b[84:88].decode("latin1")
        w = struct.unpack_from("<I", b, 16)[0]
        h = struct.unpack_from("<I", b, 12)[0]
        dds_fmt = "DXT5" if fourcc == "DXT5" else "DXT1"
        fmt = "PF_DXT5" if dds_fmt == "DXT5" else "PF_DXT1"

        ps = Image.open(Path(KENNEY) / ps_file).convert("RGBA").resize((w, h), Image.LANCZOS)
        out_path = out_root / f"{name}__B_ibp.dds"
        tp.save_dds_mips(out_path, mips(ps), dds_fmt)

        results[name] = {
            "note": f"UI glyph replacement {ps_file}",
            "kind": "glyph",
            "source": name,
            "object_dims": [w, h],
            "source_dims": [w, h],
            "source_mip_level": 0,
            "source_dds": str(src.relative_to(local)),
            "source_fnv": tp.fnv1a64_mip0(src),
            "format": fmt,
            "dds_format": dds_fmt,
            "candidates": {"B_ibp": {"dims": [w, h], "mips": len(mips(ps)),
                                     "dds_bytes": out_path.stat().st_size,
                                     "sha256": tp.sha256_file(out_path)}},
        }
        made += 1
        tp.log(f"glyph {name} -> {ps_file} ({w}x{h} {dds_fmt})")

    tp.write_json(up_path, results)
    tp.log(f"glyph replacements: {made}")


if __name__ == "__main__":
    main()
