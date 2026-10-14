#!/usr/bin/env python3
"""Identify loading_screen.bik content via decoded frames (evidence-first)."""
from __future__ import annotations

import argparse
import hashlib
import json
import subprocess
from pathlib import Path


def sha256(path: Path) -> str:
    h = hashlib.sha256()
    with path.open("rb") as f:
        for block in iter(lambda: f.read(1 << 20), b""):
            h.update(block)
    return h.hexdigest().upper()


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
    ap.add_argument("--source", type=Path, required=True)
    ap.add_argument("--ffmpeg", type=Path, required=True)
    ap.add_argument("--ffprobe", type=Path, required=True)
    ap.add_argument("--out-dir", type=Path, required=True)
    ap.add_argument("--sample-count", type=int, default=8)
    args = ap.parse_args()
    args.out_dir.mkdir(parents=True, exist_ok=True)
    meta = probe(args.ffprobe, args.source)
    v = next(s for s in meta["streams"] if s.get("codec_type") == "video")
    duration = float(v.get("duration") or meta["format"].get("duration") or 0)
    fps_parts = str(v.get("r_frame_rate", "30/1")).split("/")
    fps = float(fps_parts[0]) / float(fps_parts[1] or 1)
    nb = int(duration * fps) if duration else 60
    indices = sorted({max(0, min(nb - 1, int(i))) for i in [0, nb // 6, nb // 3, nb // 2, 2 * nb // 3, 5 * nb // 6, nb - 1]})
    samples = []
    for n in indices[: args.sample_count]:
        dest = args.out_dir / f"sample_n{n:04d}.png"
        subprocess.run(
            [
                str(args.ffmpeg),
                "-y",
                "-hide_banner",
                "-loglevel",
                "error",
                "-i",
                str(args.source),
                "-vf",
                f"select=eq(n\\,{n})",
                "-vframes",
                "1",
                str(dest),
            ],
            check=True,
        )
        samples.append({"frame": n, "png": str(dest), "sha256": sha256(dest)})
    report = {
        "file": str(args.source.resolve()),
        "sha256": sha256(args.source),
        "duration_s": duration,
        "fps": v.get("r_frame_rate"),
        "width": v.get("width"),
        "height": v.get("height"),
        "estimated_frames": nb,
        "samples": samples,
        "notes": "Human-readable description belongs in loading-screen-evidence.md after reviewing PNGs.",
    }
    (args.out_dir / "loading-screen-analysis.json").write_text(json.dumps(report, indent=2), encoding="utf-8")
    print(json.dumps(report, indent=2))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
