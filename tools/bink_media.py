#!/usr/bin/env python3
"""Offline inventory, audio extraction, and structural validation for Bink movies.

The script never edits an input movie. It invokes caller-supplied FFmpeg tools and
writes only the requested JSON/WAV/report outputs.
"""
from __future__ import annotations

import argparse
import hashlib
import json
import re
import subprocess
import sys
from pathlib import Path
from typing import Any


def run_json(command: list[str]) -> dict[str, Any]:
    proc = subprocess.run(command, check=True, capture_output=True, text=True, encoding="utf-8")
    return json.loads(proc.stdout)


def sha256(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        for block in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(block)
    return digest.hexdigest().upper()


def probe(path: Path, ffprobe: str) -> dict[str, Any]:
    data = run_json([ffprobe, "-v", "error", "-show_streams", "-show_format", "-of", "json", str(path)])
    streams = data.get("streams", [])
    audio_ordinal = 0
    for stream in streams:
        if stream.get("codec_type") == "audio":
            stream["audio_ordinal"] = audio_ordinal
            audio_ordinal += 1
    return {
        "path": str(path.resolve()),
        "sha256": sha256(path),
        "file_size": path.stat().st_size,
        "format": data.get("format", {}),
        "streams": streams,
    }


def inventory(args: argparse.Namespace) -> int:
    root = args.directory.resolve()
    movies = sorted((p for p in root.iterdir() if p.is_file() and p.suffix.lower() == ".bik"), key=lambda p: p.name.casefold())
    movie_paths = {p.resolve() for p in movies}
    if args.output.resolve() in movie_paths:
        raise ValueError("inventory output cannot overwrite a source movie")
    result = {"directory": str(root), "movie_count": len(movies), "movies": [probe(p, args.ffprobe) for p in movies]}
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(result, indent=2, ensure_ascii=False) + "\n", encoding="utf-8")
    print(f"inventoried {len(movies)} Bink movies -> {args.output}")
    return 0


def extract_audio(args: argparse.Namespace) -> int:
    if args.audio_ordinal < 0:
        raise ValueError("audio ordinal must be zero or greater")
    if args.output.resolve() == args.input.resolve():
        raise ValueError("audio output cannot overwrite the source movie")
    args.output.parent.mkdir(parents=True, exist_ok=True)
    command = [args.ffmpeg, "-y", "-hide_banner", "-v", "error", "-i", str(args.input), "-map", f"0:a:{args.audio_ordinal}", "-vn", "-c:a", "pcm_s16le"]
    if args.sample_rate:
        command += ["-ar", str(args.sample_rate)]
    if args.channels:
        command += ["-ac", str(args.channels)]
    command.append(str(args.output))
    subprocess.run(command, check=True)
    print(f"extracted audio ordinal {args.audio_ordinal}: {args.output}")
    return 0


def _video_signature(media: dict[str, Any]) -> dict[str, Any]:
    video = [s for s in media["streams"] if s.get("codec_type") == "video"]
    if not video:
        raise ValueError(f"no video stream in {media['path']}")
    keys = ("codec_name", "width", "height", "r_frame_rate", "avg_frame_rate", "nb_frames")
    return {key: video[0].get(key) for key in keys} | {"duration": float(video[0].get("duration", media["format"].get("duration", 0)) or 0)}


def validate_video(args: argparse.Namespace) -> int:
    original = probe(args.original, args.ffprobe)
    candidate = probe(args.candidate, args.ffprobe)
    before, after = _video_signature(original), _video_signature(candidate)
    failures: list[str] = []
    for key in ("codec_name", "width", "height", "r_frame_rate", "avg_frame_rate", "nb_frames"):
        if before.get(key) != after.get(key):
            failures.append(f"video {key}: {before.get(key)!r} != {after.get(key)!r}")
    fps_num, fps_den = (int(part) for part in str(before["r_frame_rate"]).split("/"))
    frame_duration = fps_den / fps_num if fps_num else 0
    if abs(before["duration"] - after["duration"]) > max(frame_duration, 0.001):
        failures.append(f"video duration differs by more than one frame: {before['duration']} != {after['duration']}")
    original_audio = [s for s in original["streams"] if s.get("codec_type") == "audio"]
    candidate_audio = [s for s in candidate["streams"] if s.get("codec_type") == "audio"]
    original_subtitles = [s for s in original["streams"] if s.get("codec_type") == "subtitle"]
    candidate_subtitles = [s for s in candidate["streams"] if s.get("codec_type") == "subtitle"]
    if len(original_audio) != len(candidate_audio):
        failures.append(f"audio stream count: {len(original_audio)} != {len(candidate_audio)}")
    else:
        for ordinal, (left, right) in enumerate(zip(original_audio, candidate_audio)):
            for key in ("codec_name", "sample_rate", "channels", "channel_layout"):
                if left.get(key) != right.get(key):
                    failures.append(f"audio {ordinal} {key}: {left.get(key)!r} != {right.get(key)!r}")
    if len(original_subtitles) != len(candidate_subtitles):
        failures.append(f"subtitle stream count: {len(original_subtitles)} != {len(candidate_subtitles)}")
    if not failures:
        subprocess.run([args.ffmpeg, "-hide_banner", "-v", "error", "-i", str(args.candidate), "-map", "0:v:0", "-map", "0:a?", "-f", "null", "-"], check=True)
    if failures:
        print("FAIL: structural comparison\n" + "\n".join(f"- {item}" for item in failures), file=sys.stderr)
        return 1
    print("PASS: candidate video metadata matches source; all audio/video streams decode")
    print(json.dumps({"original_sha256": original["sha256"], "candidate_sha256": candidate["sha256"], "video": after, "audio_streams": len(candidate_audio)}, indent=2))
    return 0


_SRT_TIME = re.compile(r"^(\d{2}):(\d{2}):(\d{2}),(\d{3})$")


def _seconds(value: str) -> float:
    match = _SRT_TIME.fullmatch(value)
    if not match:
        raise ValueError(f"invalid SRT timestamp: {value}")
    hours, minutes, seconds, millis = map(int, match.groups())
    return hours * 3600 + minutes * 60 + seconds + millis / 1000


def validate_srt(args: argparse.Namespace) -> int:
    text = args.subtitle.read_text(encoding="utf-8-sig")
    blocks = re.split(r"\r?\n\s*\r?\n", text.strip()) if text.strip() else []
    previous_end = -1.0
    errors: list[str] = []
    for expected_index, block in enumerate(blocks, 1):
        lines = block.splitlines()
        if len(lines) < 3 or lines[0].strip() != str(expected_index):
            errors.append(f"block {expected_index}: invalid index or missing text")
            continue
        times = lines[1].split(" --> ")
        if len(times) != 2:
            errors.append(f"block {expected_index}: invalid time range")
            continue
        start, end = map(_seconds, times)
        if end <= start or start < previous_end:
            errors.append(f"block {expected_index}: invalid order/range {start}..{end}")
        if args.duration is not None and end > args.duration:
            errors.append(f"block {expected_index}: ends after media duration ({end} > {args.duration})")
        previous_end = end
    if errors:
        print("FAIL: SRT validation\n" + "\n".join(f"- {item}" for item in errors), file=sys.stderr)
        return 1
    print(f"PASS: {len(blocks)} ordered SRT cues; duration limit={args.duration}")
    return 0


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    sub = parser.add_subparsers(dest="command", required=True)

    inv = sub.add_parser("inventory", help="probe and hash every .bik in one directory")
    inv.add_argument("directory", type=Path)
    inv.add_argument("--ffprobe", required=True)
    inv.add_argument("--output", type=Path, required=True)
    inv.set_defaults(func=inventory)

    extract = sub.add_parser("extract-audio", help="extract one zero-based FFmpeg audio ordinal to PCM WAV")
    extract.add_argument("input", type=Path)
    extract.add_argument("--audio-ordinal", type=int, required=True)
    extract.add_argument("--ffmpeg", required=True)
    extract.add_argument("--output", type=Path, required=True)
    extract.add_argument("--sample-rate", type=int)
    extract.add_argument("--channels", type=int)
    extract.set_defaults(func=extract_audio)

    compare = sub.add_parser("validate-video", help="compare candidate geometry/FPS/audio layout and decode it")
    compare.add_argument("original", type=Path)
    compare.add_argument("candidate", type=Path)
    compare.add_argument("--ffprobe", required=True)
    compare.add_argument("--ffmpeg", required=True)
    compare.set_defaults(func=validate_video)

    srt = sub.add_parser("validate-srt", help="check SRT ordering and optional media-duration bound")
    srt.add_argument("subtitle", type=Path)
    srt.add_argument("--duration", type=float)
    srt.set_defaults(func=validate_srt)

    args = parser.parse_args()
    try:
        return args.func(args)
    except (OSError, ValueError, subprocess.CalledProcessError, json.JSONDecodeError) as exc:
        print(f"ERROR: {exc}", file=sys.stderr)
        return 2


if __name__ == "__main__":
    raise SystemExit(main())
