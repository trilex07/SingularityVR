#!/usr/bin/env python3
"""Read-only evidence: which asset draws the pre-menu SINGULARITY wordmark.

Does not write into the game tree. Optional --out receives JSON only (no game
pixels). Uses caller-supplied ffprobe/ffmpeg paths when sampling Bink frames.
"""
from __future__ import annotations

import argparse
import hashlib
import json
import subprocess
import sys
import tempfile
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
from ue3_layout import inspect  # noqa: E402


def sha256(path: Path) -> str:
    h = hashlib.sha256()
    with path.open("rb") as f:
        for block in iter(lambda: f.read(1 << 20), b""):
            h.update(block)
    return h.hexdigest().upper()


def ffprobe_json(ffprobe: Path, movie: Path) -> dict:
    raw = subprocess.run(
        [str(ffprobe), "-v", "error", "-show_streams", "-show_format", "-of", "json", str(movie)],
        check=True,
        capture_output=True,
        text=True,
    ).stdout
    return json.loads(raw)


def sample_frame(ffmpeg: Path, movie: Path, frame: int, dest: Path) -> None:
    subprocess.run(
        [
            str(ffmpeg),
            "-y",
            "-hide_banner",
            "-loglevel",
            "error",
            "-i",
            str(movie),
            "-vf",
            f"select=eq(n\\,{frame})",
            "-vframes",
            "1",
            str(dest),
        ],
        check=True,
    )


def frame_stats(path: Path) -> dict:
    from PIL import Image

    im = Image.open(path).convert("RGBA")
    w, h = im.size
    px = im.load()
    bright = 0
    rows = [0] * h
    for y in range(h):
        for x in range(w):
            r, g, b, _a = px[x, y]
            lum = 0.299 * r + 0.587 * g + 0.114 * b
            if lum > 40:
                bright += 1
                rows[y] += 1
    active = [i for i, v in enumerate(rows) if v > w * 0.02]
    return {
        "size": [w, h],
        "bright_ratio": round(bright / (w * h), 5),
        "text_band_rows": [active[0], active[-1]] if active else None,
        "sha256": hashlib.sha256(path.read_bytes()).hexdigest().upper(),
    }


def iis_has_wfont_logo(ui_logical: Path) -> dict:
    data = ui_logical.read_bytes()
    doc = inspect("memory", data, include_all=True)
    logo = next(e for e in doc["exports"] if e["name"] == "iis")
    raw = next(p for p in logo["properties"] if p["name"] == "RawData")
    off = raw["tag_offset"] + 24
    size = raw["value"]["bytes"] - 4
    movie = data[off + 4 : off + 4 + size]
    return {
        "export": "iis/GFxMovieInfo",
        "bytes": size,
        "has_wfont_logo_html": b"WFontLogo" in movie and b"SINGULARITY" in movie,
        "note": "RvUIGFxMovie_IIS menu field; not the pre-menu Bink card per user scope",
    }


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("--game-movies", type=Path, required=True, help="RvGame/Movies directory")
    ap.add_argument("--ui-logical", type=Path, required=True, help="Decompressed UI_SinglePlayer_SF.u")
    ap.add_argument("--ui-logos-logical", type=Path, required=True, help="Decompressed ui_logos_SF.u")
    ap.add_argument("--ffprobe", type=Path, required=True)
    ap.add_argument("--ffmpeg", type=Path, required=True)
    ap.add_argument("--out", type=Path, required=True)
    args = ap.parse_args()
    if args.out.exists():
        raise SystemExit("Refuse existing --out")

    sng = args.game_movies / "SNG_logo.bik"
    copyright = args.game_movies / "Copyright_Sing.bik"
    for p in (sng, copyright, args.ui_logical, args.ui_logos_logical):
        if not p.is_file():
            raise SystemExit(f"Missing input: {p}")

    logo_doc = inspect("memory", args.ui_logos_logical.read_bytes(), include_all=True)
    logo_exports = [e for e in logo_doc["exports"] if e["name"] == "logo" and e["class_name"] == "Texture2D"]
    if len(logo_exports) != 1:
        raise SystemExit("ui_logos.logo identity failed")

    with tempfile.TemporaryDirectory(prefix="boot-logo-proof-") as tmp:
        tmp_path = Path(tmp)
        sng_frame = tmp_path / "sng-f180.png"
        copy_frame = tmp_path / "copyright-f60.png"
        sample_frame(args.ffmpeg, sng, 180, sng_frame)
        sample_frame(args.ffmpeg, copyright, 60, copy_frame)
        sng_stats = frame_stats(sng_frame)
        copy_stats = frame_stats(copy_frame)

    conclusion = {
        "producer": "SNG_logo.bik",
        "confidence": "OFFLINE_FRAME_SAMPLE",
        "rationale": [
            "SNG_logo.bik sample frame is black field + centered SINGULARITY wordmark only (matches user boot card).",
            "Copyright_Sing.bik sample adds legal footer text (different screen).",
            "ui_logos.logo is 512x128 DXT5 HUD strip; not fullscreen; runtime boot draw unproven.",
            "iis WFontLogo HTML is RvUIGFxMovie_IIS (MAIN title field); explicitly out of SIDE scope.",
        ],
        "falsifiers": [
            "Live BinkOpen log shows a different .bik owns the first SINGULARITY-only frame before IIS focus.",
            "Replacing ui_logos.logo alone changes the boot card without touching SNG_logo.bik.",
        ],
    }

    payload = {
        "status": "OFFLINE_PRODUCER_EVIDENCE_NOT_L4",
        "conclusion": conclusion,
        "inputs": {
            "SNG_logo.bik": {"path": str(sng), "sha256": sha256(sng), **ffprobe_json(args.ffprobe, sng)},
            "Copyright_Sing.bik": {"path": str(copyright), "sha256": sha256(copyright), **ffprobe_json(args.ffprobe, copyright)},
            "ui_logos_SF.u": {"path": str(args.ui_logos_logical), "sha256": sha256(args.ui_logos_logical)},
            "UI_SinglePlayer_SF.u": {"path": str(args.ui_logical), "sha256": sha256(args.ui_logical)},
        },
        "samples": {
            "SNG_logo_frame_180": sng_stats,
            "Copyright_Sing_frame_60": copy_stats,
            "copyright_has_footer_band": copy_stats.get("text_band_rows") != sng_stats.get("text_band_rows"),
        },
        "iis": iis_has_wfont_logo(args.ui_logical),
        "ui_logos_logo": {
            "export": "ui_logos.logo",
            "class": "Texture2D",
            "note": "Decoded offline shows same typography; not proven as boot card producer",
        },
    }
    args.out.parent.mkdir(parents=True, exist_ok=True)
    args.out.write_text(json.dumps(payload, indent=2), encoding="utf-8")
    print(json.dumps({"status": payload["status"], "producer": conclusion["producer"], "out": str(args.out)}, indent=2))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
