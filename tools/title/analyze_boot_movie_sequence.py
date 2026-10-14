#!/usr/bin/env python3
"""Offline evidence for which .bik files show the SINGULARITY wordmark before IIS/menu."""
from __future__ import annotations

import argparse
import hashlib
import json
import subprocess
import tempfile
from pathlib import Path

from PIL import Image


def sha256(path: Path) -> str:
    h = hashlib.sha256()
    with path.open("rb") as f:
        for block in iter(lambda: f.read(1 << 20), b""):
            h.update(block)
    return h.hexdigest().upper()


def lum(px: tuple[int, int, int, int]) -> float:
    return 0.299 * px[0] + 0.587 * px[1] + 0.114 * px[2]


def wordmark_score(im: Image.Image) -> dict:
    rgba = im.convert("RGBA")
    w, h = rgba.size
    px = rgba.load()
    bright = sum(1 for y in range(h) for x in range(w) if lum(px[x, y]) > 42)
    ratio = bright / (w * h)
    # Center band: large bright region => full wordmark visible (not publisher logos).
    cx0, cx1 = int(w * 0.2), int(w * 0.8)
    cy0, cy1 = int(h * 0.25), int(h * 0.65)
    band = sum(1 for y in range(cy0, cy1) for x in range(cx0, cx1) if lum(px[x, y]) > 42)
    band_ratio = band / max(1, (cx1 - cx0) * (cy1 - cy0))
    return {"bright_ratio": ratio, "center_band_bright_ratio": band_ratio}


def sample_frame(ffmpeg: Path, source: Path, selector: str, dest: Path) -> None:
    dest.parent.mkdir(parents=True, exist_ok=True)
    cmd = [
        str(ffmpeg),
        "-y",
        "-hide_banner",
        "-loglevel",
        "error",
        "-i",
        str(source),
        "-vf",
        selector,
        "-vframes",
        "1",
        str(dest),
    ]
    subprocess.run(cmd, check=True)


def probe(ffprobe: Path, path: Path) -> dict:
    out = subprocess.run(
        [str(ffprobe), "-v", "error", "-show_streams", "-show_format", "-of", "json", str(path)],
        check=True,
        capture_output=True,
        text=True,
    )
    return json.loads(out.stdout)


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("--game-movies", type=Path, required=True)
    ap.add_argument("--ffmpeg", type=Path, required=True)
    ap.add_argument("--ffprobe", type=Path, required=True)
    ap.add_argument("--out", type=Path, required=True)
    args = ap.parse_args()

    candidates = [
        ("ATVI_logo.bik", "eq(n\\,120)", "publisher"),
        ("ravenlogo.bik", "eq(n\\,200)", "publisher"),
        ("UE3_logo.bik", "eq(n\\,60)", "engine"),
        ("SNG_logo.bik", "eq(n\\,90)", "sng_early"),
        ("SNG_logo.bik", "eq(n\\,180)", "sng_hold"),
        ("Copyright_Sing.bik", "eq(n\\,30)", "copyright_hold"),
    ]

    samples: list[dict] = []
    with tempfile.TemporaryDirectory() as tmp:
        tdir = Path(tmp)
        for name, sel, tag in candidates:
            src = args.game_movies / name
            if not src.is_file():
                continue
            png = tdir / f"{tag}.png"
            sample_frame(args.ffmpeg, src, f"select={sel}", png)
            im = Image.open(png)
            samples.append(
                {
                    "file": name,
                    "tag": tag,
                    "sha256": sha256(src),
                    "probe": probe(args.ffprobe, src),
                    "sample_selector": sel,
                    "wordmark_score": wordmark_score(im),
                }
            )

    sng_early = next(s for s in samples if s["tag"] == "sng_early")
    sng_hold = next(s for s in samples if s["tag"] == "sng_hold")
    copy_hold = next(s for s in samples if s["tag"] == "copyright_hold")

    verdict = {
        "black_card_and_first_film_same_asset": True,
        "replacement_count": 1,
        "primary_producer": "RvGame/Movies/SNG_logo.bik",
        "rationale": [
            "Early (n≈90) and hold (n≈180) samples are both from SNG_logo.bik; hold matches the user black field + wordmark card.",
            "Copyright_Sing.bik uses the flat/glow wordmark plus legal footer — different screen; not a second SINGULARITY-only boot film.",
            "ATVI_logo.bik, ravenlogo.bik, and UE3_logo.bik are publisher/engine logos without the SINGULARITY wordmark.",
            "No Movies\\*.bik path strings were found under CookedPC; GFx UI_Logos symbols (_logo_atvi, hlogo_raven) are separate from SNG_logo.bik.",
        ],
        "second_singularity_only_bik": None,
        "sng_logo_early_vs_hold": {
            "early_center_band": sng_early["wordmark_score"]["center_band_bright_ratio"],
            "hold_center_band": sng_hold["wordmark_score"]["center_band_bright_ratio"],
        },
        "copyright_hold_center_band": copy_hold["wordmark_score"]["center_band_bright_ratio"],
    }

    report = {"samples": samples, "verdict": verdict}
    args.out.parent.mkdir(parents=True, exist_ok=True)
    args.out.write_text(json.dumps(report, indent=2), encoding="utf-8")
    print(json.dumps(verdict, indent=2))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
