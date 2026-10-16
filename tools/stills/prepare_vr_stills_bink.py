#!/usr/bin/env python3
"""VR stills -> Bink 1 (BIKi) candidates for SingularityVR movie slots.

Side-only offline prep (no Steam, no game launch). Method mirrors the
boot-logo branch (`fix/boot-logo-vya-suffix`, handoff
`docs/handoffs/side-boot-logo-bik-handoff.md`):
stills -> intermediate MKV (ffmpeg) -> binkenc (LibBink open-source Bink 1
encoder, third_party/libbink-src) -> optional stock-audio remux
(`tools/stills/remux_bink_stock_audio.py`, exact copy) -> ffprobe + full
decode validation + proof PNGs decoded from the FINAL .bik + manifests.

Slots (evidence: black pre-menu card and first SINGULARITY film are one
file, `RvGame/Movies/SNG_logo.bik`):
  SNG_logo_vr2      <- Metaliczne logo SINGULARITY VR (11 s, 30 fps, 330 f,
                       stock 44.1 kHz stereo audio remuxed, like the v1 wordmark)
  loading_screen_vr2<- Singularity_VR_4K_3840x2160 downscaled to 1280x720
                       (2 s, 24 fps, 48 f, no audio; slot-proven geometry)
  loading_screen_4k <- same source at native 3840x2160 (EXPERIMENTAL: Bink 1
                       4K was never slot-proven in the 2010 UE3 decoder;
                       MAIN must runtime-test before any install)

Usage:
  py -3 tools/stills/prepare_vr_stills_bink.py [--only SNG_logo_vr2]
"""
from __future__ import annotations

import argparse
import hashlib
import json
import shutil
import struct
import subprocess
import sys
from pathlib import Path

REPO = Path(__file__).resolve().parents[2]
REFS = Path(r"C:\Users\Trilex\vr-mods\references")
SANDBOX_MOVIES = Path(r"C:\Users\Trilex\vr-mods\l3-sandbox-20261010\RvGame\Movies")
BINKENC = Path(
    r"C:\Users\Trilex\vr-mods\third_party\libbink-src\libbink-main\encoder"
    r"\src\LibBink.Encoder\bin\Release\net8.0\binkenc.exe"
)
OUT = REPO / "offline-deliverables" / "stills-vr-v1"
WORK = REPO / "work" / "stills-vr-v1"

CANDIDATES = {
    "SNG_logo_vr2": {
        "src_png": REFS / "Metaliczne logo SINGULARITY VR.png",
        "stock_bik": SANDBOX_MOVIES / "SNG_logo.bik",
        "install_as": "RvGame/Movies/SNG_logo.bik",
        "width": 1280, "height": 720, "fps": "30", "seconds": 11,
        "quality": 8, "audio": "stock-remux",
        "note": "pre-open card + first SINGULARITY film (one asset); metallic VR logo still",
    },
    "loading_screen_vr2": {
        "src_png": REFS / "Singularity_VR_4K_3840x2160.png",
        "stock_bik": SANDBOX_MOVIES / "loading_screen.bik",
        "install_as": "RvGame/Movies/loading_screen.bik",
        "width": 1280, "height": 720, "fps": "24", "seconds": 2,
        "quality": 8, "audio": "none",
        "note": "level-load card; 4K source downscaled 3x to slot-proven 720p",
    },
    "loading_screen_4k": {
        "src_png": REFS / "Singularity_VR_4K_3840x2160.png",
        "stock_bik": SANDBOX_MOVIES / "loading_screen.bik",
        "install_as": "RvGame/Movies/loading_screen.bik",
        "width": 3840, "height": 2160, "fps": "24", "seconds": 2,
        "quality": 6, "audio": "none", "experimental": True,
        "note": "EXPERIMENTAL native-4K Bink 1; never slot-proven in UE3 2010 decoder",
    },
    "Copyright_Sing_vr": {
        "src_png": REFS / "Metaliczne logo SINGULARITY VR.png",
        "stock_bik": SANDBOX_MOVIES / "Copyright_Sing.bik",
        "install_as": "RvGame/Movies/Copyright_Sing.bik",
        "width": 1280, "height": 720, "fps": "30", "seconds": 5,
        "quality": 8, "audio": "stock-remux",
        "note": "boot legal/copyright screen (5.00s, 30fps, 1280x720, stock 44.1kHz stereo audio remuxed)",
    },
}


def sha256(path: Path) -> str:
    h = hashlib.sha256()
    with path.open("rb") as f:
        for block in iter(lambda: f.read(1 << 20), b""):
            h.update(block)
    return h.hexdigest().upper()


def has_biki_sentinel(data: bytes) -> bool:
    if len(data) < 48 or data[:4] != b"BIKi":
        return False
    fc = struct.unpack_from("<I", data, 8)[0]
    tracks = struct.unpack_from("<I", data, 40)[0]
    off = 44 + 12 * tracks
    if len(data) < off + (fc + 1) * 4:
        return False
    first_off = struct.unpack_from("<I", data, off)[0] & ~1
    sentinel = struct.unpack_from("<I", data, off + fc * 4)[0] & ~1
    return first_off == off + (fc + 1) * 4 and sentinel == len(data)


def ensure_biki_sentinel(src: Path, dst: Path) -> None:
    data = src.read_bytes()
    if has_biki_sentinel(data):
        dst.write_bytes(data)
        return
    if data[:4] != b"BIKi":
        raise ValueError("not BIKi")
    fc = struct.unpack_from("<I", data, 8)[0]
    tracks = struct.unpack_from("<I", data, 40)[0]
    off = 44 + 12 * tracks
    raw_offs = [struct.unpack_from("<I", data, off + i * 4)[0] for i in range(fc)]
    table_end = off + fc * 4
    body = data[table_end:]
    new_total = len(data) + 4
    shifted = [((o & ~1) + 4) | (o & 1) for o in raw_offs] + [new_total]
    max_frame = max(((shifted[i + 1] & ~1) - (shifted[i] & ~1) for i in range(fc)), default=0)
    header = bytearray(data[:off] + b"\x00" * ((fc + 1) * 4))
    struct.pack_into("<I", header, 4, new_total - 8)
    struct.pack_into("<I", header, 12, max_frame)
    for i, val in enumerate(shifted):
        struct.pack_into("<I", header, off + i * 4, val)
    dst.parent.mkdir(parents=True, exist_ok=True)
    dst.write_bytes(bytes(header) + body)


def run(cmd: list[str]) -> str:
    p = subprocess.run(cmd, capture_output=True, text=True)
    if p.returncode != 0:
        raise RuntimeError(f"FAILED {' '.join(cmd)}\n{p.stderr[-3000:]}")
    return p.stdout


def ffprobe_json(path: Path) -> dict:
    out = run(["ffprobe", "-v", "error", "-show_streams", "-show_format",
               "-of", "json", str(path)])
    return json.loads(out)


def stream_by_codec(info: dict, ctype: str) -> dict | None:
    for s in info.get("streams", []):
        if s.get("codec_type") == ctype:
            return s
    return None


def build_mkv(ffmpeg: str, spec: dict, frames: int, mkv: Path, work: Path) -> None:
    w, h, fps = spec["width"], spec["height"], spec["fps"]
    vf = f"scale={w}:{h}"
    cmd = ["ffmpeg", "-y", "-loop", "1", "-framerate", fps, "-i",
           str(spec["src_png"])]
    if spec["audio"] == "stock-remux":
        silence = work / "silence.wav"
        if not silence.exists():
            run(["ffmpeg", "-y", "-f", "lavfi",
                 "-i", "anullsrc=r=44100:cl=stereo",
                 "-t", str(spec["seconds"]), "-c:a", "pcm_s16le",
                 str(silence)])
        cmd += ["-i", str(silence)]
    cmd += ["-vf", vf, "-frames:v", str(frames),
            "-c:v", "ffv1", "-pix_fmt", "yuv420p"]
    if spec["audio"] == "stock-remux":
        cmd += ["-frames:a", str(44100 * spec["seconds"]), "-c:a",
                "pcm_s16le", "-shortest"]
    cmd += [str(mkv)]
    run(cmd)


def build_candidate(name: str, spec: dict) -> dict:
    fps, seconds = spec["fps"], spec["seconds"]
    frames = int(fps) * seconds
    asset = OUT / name
    cand_dir = asset / "candidate"
    proof_dir = asset / "proof-from-bik"
    cand_dir.mkdir(parents=True, exist_ok=True)
    proof_dir.mkdir(parents=True, exist_ok=True)
    awork = WORK / name
    awork.mkdir(parents=True, exist_ok=True)

    for p in (spec["src_png"], spec["stock_bik"]):
        if not p.is_file():
            raise FileNotFoundError(f"missing input: {p}")
    if not BINKENC.is_file():
        raise FileNotFoundError(f"missing encoder: {BINKENC}")

    stock_info = ffprobe_json(spec["stock_bik"])
    stock_v = stream_by_codec(stock_info, "video") or {}
    stock_a = stream_by_codec(stock_info, "audio")

    mkv = awork / f"{name}.intermediate.mkv"
    build_mkv("ffmpeg", spec, frames, mkv, awork)

    raw_bik = awork / f"{name}.video-only.bik"
    enc = [str(BINKENC), str(mkv), str(raw_bik), "-y", "--overwrite",
           "--video-quality", str(spec["quality"]), "--fps", fps,
           "--keyframe-interval", "0", "--ffmpeg", "ffmpeg",
           "--ffprobe", "ffprobe"]
    if spec["audio"] == "none":
        enc.append("--no-audio")
    run(enc)

    final_bik = cand_dir / f"{name}.bik"
    if spec["audio"] == "stock-remux":
        run([sys.executable, str(REPO / "tools" / "stills" / "remux_bink_stock_audio.py"),
             "--stock", str(spec["stock_bik"]), "--candidate-video", str(raw_bik),
             "--output", str(final_bik)])
    else:
        ensure_biki_sentinel(raw_bik, final_bik)

    # ---- validate vs stock slot ----
    bik_bytes = final_bik.read_bytes()
    info = ffprobe_json(final_bik)
    v = stream_by_codec(info, "video") or {}
    a = stream_by_codec(info, "audio")
    checks = {
        "magic_biki": bik_bytes[:4] == b"BIKi",
        "offset_table_sentinel": has_biki_sentinel(bik_bytes),
        "codec_binkvideo": (v.get("codec_name") or "") == "binkvideo",
        "geometry": (v.get("width"), v.get("height")) == (spec["width"], spec["height"]),
        "fps": (v.get("avg_frame_rate") or v.get("r_frame_rate")) in (f"{fps}/1", f"{fps}.0"),
        "audio_layout": (a is not None) == (stock_a is not None),
    }
    # full decode (renders every frame; fails loudly on truncation)
    run(["ffmpeg", "-v", "error", "-i", str(final_bik), "-f", "null", "-"])
    # proof frames from the FINAL bik
    proofs = []
    for idx in (0, frames // 2, frames - 1):
        pp = proof_dir / f"frame_n{idx:04d}.png"
        run(["ffmpeg", "-v", "error", "-y", "-i", str(final_bik),
             "-vf", f"select='eq(n\\,{idx})'", "-fps_mode", "passthrough", "-frames:v", "1",
             str(pp)])
        proofs.append({"frame_index": idx, "path": str(pp), "sha256": sha256(pp)})
    verdict = "PASS" if all(checks.values()) else "FAIL"
    manifest = {
        "asset": name,
        "install_as": spec["install_as"],
        "experimental": bool(spec.get("experimental", False)),
        "note": spec["note"],
        "source_png": {"path": str(spec["src_png"]), "sha256": sha256(spec["src_png"])},
        "stock_bik": {"path": str(spec["stock_bik"]), "sha256": sha256(spec["stock_bik"])},
        "video": {"width": spec["width"], "height": spec["height"],
                  "duration_s": float(seconds), "fps": f"{fps}/1", "frames": frames,
                  "quality": spec["quality"], "audio": spec["audio"],
                  "intermediate_mkv_NOT_SHIPPED": str(mkv)},
        "candidate_bik": {"path": str(final_bik), "sha256": sha256(final_bik)},
        "validate": {"verdict": verdict, "checks": checks},
        "proof_frames_from_final_bik": proofs,
    }
    (cand_dir / f"{name}.json").write_text(json.dumps(manifest, indent=2), encoding="utf-8")
    (proof_dir / "proof-manifest.json").write_text(
        json.dumps({"source_bik": str(final_bik),
                    "source_bik_sha256": sha256(final_bik),
                    "proof_frames": proofs}, indent=2), encoding="utf-8")
    print(f"{name}: {verdict} frames={frames} bik={final_bik} sha={sha256(final_bik)[:16]}...")
    if verdict != "PASS":
        raise SystemExit(f"validation FAILED for {name}: {checks}")
    return manifest


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("--only", choices=list(CANDIDATES))
    args = ap.parse_args()
    names = [args.only] if args.only else list(CANDIDATES)
    results = {}
    for n in names:
        results[n] = build_candidate(n, CANDIDATES[n])
    (OUT / "MANIFEST.json").write_text(json.dumps(
        {"producer": "tools/stills/prepare_vr_stills_bink.py",
         "encoder": "LibBink binkenc (open-source Bink 1), third_party/libbink-src",
         "runtime_test": "NOT performed (SIDE); MAIN installs after backup + hash verify",
         "assets": results}, indent=2), encoding="utf-8")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
