#!/usr/bin/env python3
"""Prepare SNG_logo_vr.bik candidate: same duration, fps, audio; VR on the wordmark.

Reads the stock movie from --source (game Movies dir). Writes outputs under --out
only. Uses in-frame glyph cloning on a reference frame so chrome/glow stay native.
Re-encodes with caller-supplied binkenc when present; otherwise emits intermediate
MKV + manifest for MAIN/RAD encode.
"""
from __future__ import annotations

import argparse
import hashlib
import json
import shutil
import subprocess
import sys
import tempfile
from pathlib import Path

from PIL import Image

from vr_suffix_glyphs import stamp_v_ya


def sha256(path: Path) -> str:
    h = hashlib.sha256()
    with path.open("rb") as f:
        for block in iter(lambda: f.read(1 << 20), b""):
            h.update(block)
    return h.hexdigest().upper()


def lum(px: tuple[int, int, int, int]) -> float:
    return 0.299 * px[0] + 0.587 * px[1] + 0.114 * px[2]


def bright_bbox(im: Image.Image, threshold: float = 42.0) -> tuple[int, int, int, int]:
    rgba = im.convert("RGBA")
    w, h = rgba.size
    px = rgba.load()
    xs, ys = [], []
    for y in range(h):
        for x in range(w):
            if lum(px[x, y]) > threshold:
                xs.append(x)
                ys.append(y)
    if not xs:
        raise ValueError("no bright wordmark region")
    return min(xs), min(ys), max(xs), max(ys)


def patch_wordmark_frame(frame: Image.Image) -> Image.Image:
    """Stamp V + Ya to the right of the chrome wordmark. SINGULARITY pixels stay."""
    base = frame.convert("RGBA")
    _x0, y0, _x1, y1 = bright_bbox(base)
    return stamp_v_ya(base, y0, y1).convert("RGB")


def decode_frames(ffmpeg: Path, source: Path, out_dir: Path) -> int:
    out_dir.mkdir(parents=True, exist_ok=True)
    pattern = str(out_dir / "frame_%05d.png")
    subprocess.run(
        [
            str(ffmpeg),
            "-y",
            "-hide_banner",
            "-loglevel",
            "error",
            "-i",
            str(source),
            "-fps_mode",
            "passthrough",
            pattern,
        ],
        check=True,
    )
    return len(list(out_dir.glob("frame_*.png")))


def encode_mkv(ffmpeg: Path, frames: Path, source: Path, dest: Path, fps: str, work: Path) -> None:
    dest.parent.mkdir(parents=True, exist_ok=True)
    audio = work / "audio.wav"
    probe = subprocess.run(
        [str(ffmpeg), "-hide_banner", "-i", str(source)],
        capture_output=True,
        text=True,
    )
    has_audio = "Audio:" in (probe.stderr or "")
    if has_audio:
        subprocess.run(
            [
                str(ffmpeg),
                "-y",
                "-hide_banner",
                "-loglevel",
                "error",
                "-i",
                str(source),
                "-map",
                "0:a:0",
                "-c:a",
                "pcm_s16le",
                str(audio),
            ],
            check=True,
        )
    cmd = [
        str(ffmpeg),
        "-y",
        "-hide_banner",
        "-loglevel",
        "error",
        "-framerate",
        fps,
        "-i",
        str(frames / "frame_%05d.png"),
    ]
    if audio.is_file() and audio.stat().st_size > 44:
        cmd += ["-i", str(audio)]
    cmd += ["-map", "0:v:0"]
    if audio.is_file() and audio.stat().st_size > 44:
        cmd += ["-map", "1:a:0"]
    cmd += ["-c:v", "libx264", "-pix_fmt", "yuv420p", "-crf", "14"]
    if audio.is_file() and audio.stat().st_size > 44:
        cmd += ["-c:a", "pcm_s16le", "-shortest"]
    cmd.append(str(dest))
    subprocess.run(cmd, check=True)


def try_binkenc(binkenc: Path | None, ffmpeg: Path, ffprobe: Path, mkv: Path, dest: Path) -> dict:
    if not binkenc or not binkenc.is_file():
        return {"status": "ENCODER_MISSING", "note": "Pass --binkenc to emit .bik (BIKi) candidate"}
    cmd = [
        "dotnet",
        str(binkenc),
        str(mkv),
        str(dest),
        "--overwrite",
        "--video-quality",
        "4",
        "--audio-quality",
        "10",
        "--ffmpeg",
        str(ffmpeg),
        "--ffprobe",
        str(ffprobe),
    ]
    proc = subprocess.run(cmd, capture_output=True, text=True)
    if proc.returncode != 0:
        return {"status": "ENCODER_FAILED", "stderr": proc.stderr[-2000:]}
    return {"status": "BIK_CANDIDATE_WRITTEN", "sha256": sha256(dest)}


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("--source", type=Path, required=True, help="Original SNG_logo.bik")
    ap.add_argument("--out", type=Path, required=True)
    ap.add_argument("--ffmpeg", type=Path, required=True)
    ap.add_argument("--ffprobe", type=Path, required=True)
    ap.add_argument("--binkenc", type=Path, help="Path to binkenc.dll (libbink encoder)")
    ap.add_argument("--reference-frame", type=int, default=180)
    args = ap.parse_args()
    if args.out.exists():
        raise SystemExit("Refuse existing --out")
    if not args.source.is_file():
        raise SystemExit(f"Missing source: {args.source}")

    meta = json.loads(
        subprocess.run(
            [str(args.ffprobe), "-v", "error", "-show_streams", "-of", "json", str(args.source)],
            check=True,
            capture_output=True,
            text=True,
        ).stdout
    )
    vstream = next(s for s in meta["streams"] if s.get("codec_name") == "binkvideo")
    fps = vstream.get("r_frame_rate", "30/1")
    duration = float(vstream.get("duration", "11"))

    args.out.mkdir(parents=True)
    work = args.out / "work"
    frames = work / "frames"
    patched = work / "patched"
    patched.mkdir(parents=True, exist_ok=True)

    count = decode_frames(args.ffmpeg, args.source, frames)
    ref = frames / f"frame_{args.reference_frame + 1:05d}.png"
    if not ref.is_file():
        ref = sorted(frames.glob("frame_*.png"))[count // 2]
    ref_im = Image.open(ref)
    template = patch_wordmark_frame(ref_im)

    ref_bbox = bright_bbox(ref_im)

    ref_span = ref_bbox[2] - ref_bbox[0]
    for path in sorted(frames.glob("frame_*.png")):
        im = Image.open(path).convert("RGBA")
        try:
            bx0, by0, bx1, by1 = bright_bbox(im)
        except ValueError:
            shutil.copy2(path, patched / path.name)
            continue
        # Patch only when the frame still carries the centered wordmark (skip fades/black).
        if (bx1 - bx0) < ref_span * 0.55:
            shutil.copy2(path, patched / path.name)
            continue
        try:
            out = patch_wordmark_frame(im)
        except ValueError:
            shutil.copy2(path, patched / path.name)
            continue
        out.save(patched / path.name)

    mkv = args.out / "SNG_logo_vr.intermediate.mkv"
    encode_mkv(args.ffmpeg, patched, args.source, mkv, fps.split("/")[0] if "/" in fps else fps, work)

    bik = args.out / "SNG_logo_vr.bik"
    enc = try_binkenc(args.binkenc, args.ffmpeg, args.ffprobe, mkv, bik)

    manifest = {
        "status": "OFFLINE_PREP",
        "producer": "SNG_logo.bik",
        "source": {"path": str(args.source), "sha256": sha256(args.source), "duration_s": duration, "fps": fps, "frames": count},
        "candidate": {
            "intermediate_mkv": str(mkv),
            "intermediate_sha256": sha256(mkv),
            "bik": str(bik) if bik.exists() else None,
            "encode": enc,
        },
        "reference_frame": ref.name,
        "template_preview_sha256": sha256_bytes_image(template),
        "scope": "Boot card only; does not modify iis char 10 / SPMenus char 166 / DLL / INI",
    }
    (args.out / "boot-logo.json").write_text(json.dumps(manifest, indent=2), encoding="utf-8")
    template.save(args.out / "reference-vr-patch-preview.png")
    print(json.dumps(manifest, indent=2))
    return 0


def sha256_bytes_image(im: Image.Image) -> str:
    from io import BytesIO

    buf = BytesIO()
    im.save(buf, format="PNG")
    return hashlib.sha256(buf.getvalue()).hexdigest().upper()


if __name__ == "__main__":
    raise SystemExit(main())
