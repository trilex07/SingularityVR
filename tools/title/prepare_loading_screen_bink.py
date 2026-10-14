#!/usr/bin/env python3
"""Patch loading_screen.bik wordmark with V + Ya suffix; re-encode to BIKi."""
from __future__ import annotations

import argparse
import hashlib
import json
import shutil
import subprocess
import sys
from pathlib import Path

from PIL import Image

from prepare_boot_logo_bink import encode_mkv, sha256, sha256_bytes_image, try_binkenc
from vr_suffix_glyphs import append_v_ya_to_strip


def lum(px: tuple[int, int, int, int]) -> float:
    return 0.299 * px[0] + 0.587 * px[1] + 0.114 * px[2]


def top_wordmark_bbox(im: Image.Image, top_fraction: float = 0.38, threshold: float = 80.0) -> tuple[int, int, int, int]:
    rgba = im.convert("RGBA")
    w, h = rgba.size
    y_max = int(h * top_fraction)
    px = rgba.load()
    xs, ys = [], []
    for y in range(y_max):
        for x in range(w):
            if lum(px[x, y]) > threshold:
                xs.append(x)
                ys.append(y)
    if not xs:
        raise ValueError("no top wordmark region")
    return min(xs), min(ys), max(xs), max(ys)


def patch_loading_frame(frame: Image.Image) -> Image.Image:
    base = frame.convert("RGBA")
    x0, y0, x1, y1 = top_wordmark_bbox(base)
    strip = base.crop((x0, y0, x1 + 1, y1 + 1))
    extended = append_v_ya_to_strip(strip)
    max_w = int(base.size[0] * 0.92)
    if extended.size[0] > max_w:
        scale = max_w / extended.size[0]
        nh = max(1, int(extended.size[1] * scale))
        extended = extended.resize((max_w, nh), Image.Resampling.LANCZOS)
    out = base.copy()
    pad_w, pad_h = x1 - x0 + 1, y1 - y0 + 1
    out.paste(Image.new("RGBA", (pad_w, pad_h), (0, 0, 0, 0)), (x0, y0))
    paste_x = x0 + (pad_w - extended.size[0]) // 2
    paste_y = y0 + (pad_h - extended.size[1]) // 2
    out.paste(extended, (paste_x, paste_y), extended)
    return out.convert("RGB")


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("--source", type=Path, required=True)
    ap.add_argument("--out", type=Path, required=True)
    ap.add_argument("--ffmpeg", type=Path, required=True)
    ap.add_argument("--ffprobe", type=Path, required=True)
    ap.add_argument("--binkenc", type=Path, required=True)
    args = ap.parse_args()
    if args.out.exists():
        raise SystemExit("Refuse existing --out")
    meta = json.loads(
        subprocess.run(
            [str(args.ffprobe), "-v", "error", "-show_streams", "-of", "json", str(args.source)],
            check=True,
            capture_output=True,
            text=True,
        ).stdout
    )
    vstream = next(s for s in meta["streams"] if s.get("codec_name") == "binkvideo")
    fps = vstream.get("r_frame_rate", "24/1")
    duration = float(vstream.get("duration", "2"))

    args.out.mkdir(parents=True)
    work = args.out / "work"
    frames = work / "frames"
    patched = work / "patched"
    patched.mkdir(parents=True, exist_ok=True)

    pattern = str(frames / "frame_%05d.png")
    frames.mkdir(parents=True)
    subprocess.run(
        [str(args.ffmpeg), "-y", "-hide_banner", "-loglevel", "error", "-i", str(args.source), pattern],
        check=True,
    )
    paths = sorted(frames.glob("frame_*.png"))
    ref_span = None
    for path in paths:
        im = Image.open(path).convert("RGBA")
        try:
            x0, y0, x1, y1 = top_wordmark_bbox(im)
        except ValueError:
            shutil.copy2(path, patched / path.name)
            continue
        span = x1 - x0
        if ref_span is None:
            ref_span = span
        if span < ref_span * 0.7:
            shutil.copy2(path, patched / path.name)
            continue
        patch_loading_frame(im).save(patched / path.name)

    mkv = args.out / "loading_screen_vr.intermediate.mkv"
    encode_mkv(args.ffmpeg, patched, args.source, mkv, fps.split("/")[0] if "/" in fps else fps, work)
    bik = args.out / "loading_screen_vr.bik"
    enc = try_binkenc(args.binkenc, args.ffmpeg, args.ffprobe, mkv, bik)
    manifest = {
        "producer": "loading_screen.bik",
        "source": {"path": str(args.source), "sha256": sha256(args.source), "duration_s": duration, "fps": fps, "frames": len(paths)},
        "candidate": {"bik": str(bik) if bik.exists() else None, "bik_sha256": sha256(bik) if bik.exists() else None, "encode": enc},
        "patch": "top_wordmark V+Ya suffix; hand/TMD/smoke untouched",
    }
    (args.out / "loading-screen.json").write_text(json.dumps(manifest, indent=2), encoding="utf-8")
    print(json.dumps(manifest, indent=2))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
