#!/usr/bin/env python3
"""Decode proof PNGs from a final .bik (never from MKV or patch preview)."""
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


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("--bik", type=Path, required=True)
    ap.add_argument("--ffmpeg", type=Path, required=True)
    ap.add_argument("--ffprobe", type=Path, required=True)
    ap.add_argument("--out-dir", type=Path, required=True)
    ap.add_argument("--frames", type=int, nargs="+", default=[30, 180, 300])
    args = ap.parse_args()
    args.out_dir.mkdir(parents=True, exist_ok=True)
    meta = json.loads(
        subprocess.run(
            [str(args.ffprobe), "-v", "error", "-show_streams", "-show_format", "-of", "json", str(args.bik)],
            check=True,
            capture_output=True,
            text=True,
        ).stdout
    )
    v = next(s for s in meta["streams"] if s.get("codec_type") == "video")
    proofs = []
    for n in args.frames:
        dest = args.out_dir / f"frame_n{n:04d}.png"
        subprocess.run(
            [
                str(args.ffmpeg),
                "-y",
                "-hide_banner",
                "-loglevel",
                "error",
                "-i",
                str(args.bik),
                "-vf",
                f"select=eq(n\\,{n})",
                "-vframes",
                "1",
                str(dest),
            ],
            check=True,
        )
        proofs.append({"frame_index": n, "path": str(dest), "sha256": sha256(dest)})
    manifest = {
        "source_bik": str(args.bik.resolve()),
        "source_bik_sha256": sha256(args.bik),
        "video": {
            "width": v.get("width"),
            "height": v.get("height"),
            "r_frame_rate": v.get("r_frame_rate"),
            "duration": float(v.get("duration") or meta["format"].get("duration", 0)),
        },
        "proof_frames": proofs,
    }
    out_json = args.out_dir / "proof-manifest.json"
    out_json.write_text(json.dumps(manifest, indent=2), encoding="utf-8")
    print(json.dumps(manifest, indent=2))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
