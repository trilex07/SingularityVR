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
from vr_suffix_glyphs import stamp_v_ya


def lum(px: tuple[int, int, int, int]) -> float:
    return 0.299 * px[0] + 0.587 * px[1] + 0.114 * px[2]


def _ink_spans(band: Image.Image, threshold: float = 170.0, min_width: int = 8) -> list[tuple[int, int]]:
    rgba = band.convert("RGBA")
    w, h = rgba.size
    px = rgba.load()
    spans: list[tuple[int, int]] = []
    start = None
    for x in range(w):
        hit = any(lum(px[x, y]) > threshold for y in range(h))
        if hit and start is None:
            start = x
        elif not hit and start is not None:
            if x - start >= min_width:
                spans.append((start, x - 1))
            start = None
    if start is not None and w - start >= min_width:
        spans.append((start, w - 1))
    return spans


def letter_band(frame: Image.Image) -> tuple[int, int]:
    """Thin row of the flat wordmark. Ignores the hand/TMD below it."""
    rgba = frame.convert("RGBA")
    w, h = rgba.size
    y_limit = int(h * 0.30)
    px = rgba.load()
    rows = []
    for y in range(y_limit):
        n = sum(1 for x in range(w) if lum(px[x, y]) > 170)
        if n > 40:
            rows.append(y)
    if not rows:
        raise ValueError("no letter row")
    bands: list[tuple[int, int, int]] = []
    start = prev = rows[0]
    for y in rows[1:]:
        if y == prev + 1:
            prev = y
        else:
            bands.append((start, prev, prev - start))
            start = prev = y
    bands.append((start, prev, prev - start))
    y0, y1, _ = max(bands, key=lambda b: b[2])
    return y0, y1


def _masked_glyph(band: Image.Image, x0: int, x1: int, pad: int = 7) -> Image.Image:
    """Keep the orange bloom. A hard luminance cut was the rectangular edge."""
    left = max(0, x0 - pad)
    right = min(band.size[0] - 1, x1 + pad)
    crop = band.crop((left, 0, right + 1, band.size[1])).convert("RGBA")
    px = crop.load()
    w, h = crop.size
    for y in range(h):
        for x in range(w):
            r, g, b, a = px[x, y]
            level = lum((r, g, b, a))
            if level < 34:
                px[x, y] = (0, 0, 0, 0)
            else:
                px[x, y] = (r, g, b, min(255, int((level - 34) * 5)))
    return crop


def patch_loading_frame(frame: Image.Image) -> Image.Image:
    """Stamp V+Ya on the letter row. The hand stays on the original pixels."""
    base = frame.convert("RGBA")
    y0, y1 = letter_band(base)
    return stamp_v_ya(base, y0, y1).convert("RGB")


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
    for path in paths:
        im = Image.open(path).convert("RGBA")
        try:
            patch_loading_frame(im).save(patched / path.name)
        except ValueError:
            shutil.copy2(path, patched / path.name)

    mkv = args.out / "loading_screen_vr.intermediate.mkv"
    encode_mkv(args.ffmpeg, patched, args.source, mkv, fps.split("/")[0] if "/" in fps else fps, work)
    bik = args.out / "loading_screen_vr.bik"
    enc = try_binkenc(args.binkenc, args.ffmpeg, args.ffprobe, mkv, bik)
    manifest = {
        "producer": "loading_screen.bik",
        "source": {"path": str(args.source), "sha256": sha256(args.source), "duration_s": duration, "fps": fps, "frames": len(paths)},
        "candidate": {"bik": str(bik) if bik.exists() else None, "bik_sha256": sha256(bik) if bik.exists() else None, "encode": enc},
        "patch": "stamp V+Ya on the letter row only; hand/TMD not repainted",
    }
    (args.out / "loading-screen.json").write_text(json.dumps(manifest, indent=2), encoding="utf-8")
    print(json.dumps(manifest, indent=2))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
