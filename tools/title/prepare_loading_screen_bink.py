#!/usr/bin/env python3
"""Patch loading_screen.bik title row from user reference still; re-encode to BIKi."""
from __future__ import annotations

import argparse
import hashlib
import json
import shutil
import subprocess
import sys
from pathlib import Path

from PIL import Image

from prepare_boot_logo_bink import encode_mkv, sha256, try_binkenc
from reference_wordmark_compose import compose_loading_frame, extract_vr_suffix_layer


def patch_loading_frame(frame: Image.Image, vr_layer: Image.Image) -> Image.Image:
    return compose_loading_frame(frame, vr_layer)


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("--source", type=Path, required=True)
    ap.add_argument("--out", type=Path, required=True)
    ap.add_argument("--ffmpeg", type=Path, required=True)
    ap.add_argument("--ffprobe", type=Path, required=True)
    ap.add_argument("--binkenc", type=Path, required=True)
    ap.add_argument(
        "--reference-image",
        type=Path,
        required=True,
        help="User still: flat SINGULARITY + cyan VR loading art",
    )
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
    if not args.reference_image.is_file():
        raise SystemExit(f"Missing reference image: {args.reference_image}")
    vr_layer = extract_vr_suffix_layer(Image.open(args.reference_image))
    paths = sorted(frames.glob("frame_*.png"))
    for path in paths:
        im = Image.open(path).convert("RGBA")
        try:
            patch_loading_frame(im, vr_layer).save(patched / path.name)
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
        "reference_image": {"path": str(args.reference_image), "sha256": sha256(args.reference_image)},
        "patch": "paste cyan VR suffix from user reference still; stock SINGULARITY + hand unchanged",
    }
    (args.out / "loading-screen.json").write_text(json.dumps(manifest, indent=2), encoding="utf-8")
    print(json.dumps(manifest, indent=2))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
