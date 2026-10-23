#!/usr/bin/env python3
"""Enemy/gore texture upscale batch (Side, offline, no Steam).

Scope (user-approved order, evidence-corrected):
  PhaseTick d/n/s/e 1024 -> 2048 | blood decals -> 2048 |
  agemap_body/head 256 -> 1024 | revert_t 512 -> 1024.
Deliberately NOT included: Revert specular (material
ch_revert.Materials.ch_revert_env has NO specularmap slot, so a _s texture
could never be sampled) and anything 8K (see docs/enemy-detail-proposal.md).

Method mirrors asset-tools/textures/texture_pipeline.py (plates/assets
branches): conservative non-generative 2x steps (Lanczos / restrained
unsharp for albedo-emissive-spec / renormalized Lanczos for normals /
plain Lanczos for data masks), original alpha preserved via Lanczos,
mip chain to min 4, custom DDS header (DXT1/DXT5 fourcc), FNV-1a64 mip0
recorded for the injector gate. Primitives vendored below (credit:
texture_pipeline.py); no cross-branch runtime dependency.

Sources are read-only local exports (never the Steam install):
  SingularityVR-assets-local/textures/{original-png,original}/...
Outputs: processed/enemy-upscale-batch/<name>.dds (NOT shipped to game).
Game-derived pixels stay out of git ONLY if huge -- here outputs ARE the
deliverable candidates (like offline-deliverables/*.bik precedent) and are
committed with manifests; MAIN/assets-owner merges them into the injector
targets (snippet emitted into MANIFEST.json, targets file itself untouched).

Usage: py -3 tools/enemy_upscale_batch.py [--only phasetick_d]
"""
from __future__ import annotations

import argparse
import hashlib
import io
import json
import struct
import sys
from pathlib import Path

import numpy as np
from PIL import Image, ImageFilter

REPO = Path(__file__).resolve().parents[1]
LOCAL_TEX = Path(r"C:\Users\Trilex\vr-mods\SingularityVR-assets-local\textures")
OUT = REPO / "processed" / "enemy-upscale-batch"

# name, source (relative to LOCAL_TEX), kind, target px, fourcc
TARGETS = [
    ("phasetick_d", "original-png/RvGameMPLoader_SF/Texture2D/phasetick_d.png", "albedo", 2048, "DXT1"),
    ("phasetick_n", "original-png/RvGameMPLoader_SF/Texture2D/phasetick_n.png", "normal", 2048, "DXT5"),
    ("phasetick_s", "original-png/RvGameMPLoader_SF/Texture2D/phasetick_s.png", "spec", 2048, "DXT1"),
    ("phasetick_e", "original-png/RvGameMPLoader_SF/Texture2D/phasetick_e.png", "emissive", 2048, "DXT1"),
    ("bsplat3_d", "original/RvGame/Texture2D/bsplat3_d.dds", "albedo", 2048, "DXT5"),
    ("bsplat_decal_d", "original/RvGame/Texture2D/bsplat_decal_d.dds", "albedo", 2048, "DXT1"),
    ("t_blood_splat02_d", "original/RvGame/Texture2D/t_blood_splat02_d.dds", "albedo", 2048, "DXT5"),
    ("agemap_body", "original-png/RvGameMPLoader_SF/Texture2D/agemap_body.png", "mask", 1024, "DXT1"),
    ("agemap_head", "original-png/RvGameMPLoader_SF/Texture2D/agemap_head.png", "mask", 1024, "DXT1"),
    ("revert_t", "original-png/RvGameMPLoader_SF/Texture2D/revert_t.png", "mask", 1024, "DXT1"),
]


def sha256(p: Path) -> str:
    h = hashlib.sha256()
    with p.open("rb") as f:
        for b in iter(lambda: f.read(1 << 20), b""):
            h.update(b)
    return h.hexdigest().upper()


def load_source(rel: str) -> Image.Image:
    p = LOCAL_TEX / rel
    if not p.is_file():
        raise FileNotFoundError(f"missing source: {p}")
    im = Image.open(p)
    im.load()
    return im.convert("RGBA")


# ---- vendored primitives (credit: texture_pipeline.py) ----
def up_lanczos(img: Image.Image, scale: int) -> Image.Image:
    w, h = img.size
    return img.resize((w * scale, h * scale), Image.LANCZOS)


def up_restrained_sharpen(img: Image.Image, scale: int, amount: float = 0.35,
                          radius: float = 1.0, threshold: float = 3.0) -> Image.Image:
    base = up_lanczos(img, scale)
    rgb = base.convert("RGB")
    blur = rgb.filter(ImageFilter.GaussianBlur(radius))
    b = np.asarray(rgb, dtype=np.float32)
    bl = np.asarray(blur, dtype=np.float32)
    detail = b - bl
    mask = (np.abs(detail) > threshold).astype(np.float32)
    out = np.clip(b + amount * detail * mask, 0, 255).astype(np.uint8)
    res = Image.fromarray(out, "RGB").convert("RGBA")
    res.putalpha(img.convert("RGBA").getchannel("A").resize(res.size, Image.LANCZOS))
    return res


def normal_reconstruct(rgb: np.ndarray) -> np.ndarray:
    v = rgb.astype(np.float32) / 127.5 - 1.0
    n = np.linalg.norm(v, axis=2, keepdims=True)
    n[n == 0] = 1.0
    return np.clip((v / n + 1.0) * 127.5, 0, 255).astype(np.uint8)


def up_normal(img: Image.Image, scale: int) -> Image.Image:
    up = up_lanczos(img, scale)
    arr = normal_reconstruct(np.asarray(up.convert("RGB")))
    res = Image.fromarray(arr, "RGB").convert("RGBA")
    res.putalpha(img.convert("RGBA").getchannel("A").resize(res.size, Image.LANCZOS))
    return res


def upscale_kind(img: Image.Image, kind: str, target: int) -> Image.Image:
    cur = img
    step = {"albedo": lambda i: up_restrained_sharpen(i, 2),
            "spec": lambda i: up_restrained_sharpen(i, 2),
            "emissive": lambda i: up_restrained_sharpen(i, 2),
            "normal": lambda i: up_normal(i, 2),
            "mask": lambda i: up_lanczos(i.convert("RGB"), 2).convert("RGBA")}[kind]
    while cur.size[0] < target:
        cur = step(cur)
        if cur.size[0] > target:
            raise ValueError(f"overshoot {cur.size} vs {target} (pow2 steps only)")
    return cur


def make_mips(img: Image.Image, is_normal: bool, min_size: int = 4):
    mips, cur = [], img
    while True:
        rgb = np.asarray(cur.convert("RGB"))
        out = normal_reconstruct(rgb) if is_normal else rgb
        mips.append(Image.fromarray(out, "RGB").convert("RGBA"))
        w, h = cur.size
        if w <= min_size and h <= min_size:
            break
        cur = cur.resize((max(1, w // 2), max(1, h // 2)), Image.LANCZOS)
    return mips


def encode_level(img: Image.Image, fmt: str) -> bytes:
    buf = io.BytesIO()
    img.convert("RGBA").save(buf, format="DDS", pixel_format=fmt)
    return buf.getvalue()[128:]


def save_dds_mips(path: Path, mips, fmt: str) -> None:
    fourcc = fmt.encode("latin1")
    w, h = mips[0].size
    hdr = bytearray(128)
    hdr[0:4] = b"DDS "
    struct.pack_into("<I", hdr, 4, 124)
    struct.pack_into("<I", hdr, 8, 0x1 | 0x2 | 0x4 | 0x1000 | 0x80000)
    struct.pack_into("<I", hdr, 12, h)
    struct.pack_into("<I", hdr, 16, w)
    struct.pack_into("<I", hdr, 28, len(mips))
    struct.pack_into("<I", hdr, 76, 32)
    struct.pack_into("<I", hdr, 80, 0x4)
    hdr[84:88] = fourcc
    struct.pack_into("<I", hdr, 108, 0x1000)
    path.parent.mkdir(parents=True, exist_ok=True)
    with path.open("wb") as f:
        f.write(hdr)
        for m in mips:
            f.write(encode_level(m, fmt))


def fnv1a64_mip0(path: Path) -> int:
    b = path.read_bytes()
    fourcc = b[84:88].decode("latin1")
    w = struct.unpack_from("<I", b, 16)[0]
    h = struct.unpack_from("<I", b, 12)[0]
    blk = 8 if fourcc == "DXT1" else 16 if fourcc == "DXT5" else None
    if blk is None:
        raise ValueError(f"unsupported fourcc {fourcc}")
    need = (w // 4) * blk * (h // 4)
    hv = 0xCBF29CE484222325
    for x in b[128:128 + need]:
        hv ^= x
        hv = (hv * 0x100000001B3) & 0xFFFFFFFFFFFFFFFF
    return hv


def expected_bytes(w: int, h: int, fmt: str) -> int:
    blk = 8 if fmt == "DXT1" else 16
    total, cw, ch = 128, w, h
    while True:
        total += max(1, cw // 4) * blk * max(1, ch // 4)
        if cw <= 4 and ch <= 4:
            break
        cw, ch = max(1, cw // 2), max(1, ch // 2)
    return total


def process(name: str, rel: str, kind: str, target: int, fmt: str) -> dict:
    src = load_source(rel)
    native = src.size
    img = upscale_kind(src, kind, target)
    assert img.size == (target, target), f"{name}: {img.size}"
    mips = make_mips(img, is_normal=(kind == "normal"))
    out = OUT / f"{name}.dds"
    save_dds_mips(out, mips, fmt)
    # validate: header, byte-exact size, re-open, mip0 FNV
    raw = out.read_bytes()
    assert raw[:4] == b"DDS " and raw[84:88] == fmt.encode("latin1")
    assert len(raw) == expected_bytes(target, target, fmt), f"{name}: size {len(raw)}"
    chk = Image.open(out)
    chk.load()
    assert chk.size == (target, target)
    vram = len(raw) - 128
    rec = {"name": name, "kind": kind, "format": fmt,
           "native_px": list(native), "target_px": [target, target],
           "mip_levels": len(mips), "bytes": len(raw), "vram_bytes": vram,
           "fnv1a64_mip0": f"{fnv1a64_mip0(out):016X}",
           "src_sha256": sha256(LOCAL_TEX / rel) if (LOCAL_TEX / rel).is_file() else None,
           "out_sha256": sha256(out)}
    print(f"{name}: {native[0]}->{target} {fmt} mips={len(mips)} "
          f"vram={vram/1048576:.1f}MB fnv={rec['fnv1a64_mip0'][:8]}...")
    return rec


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("--only", choices=[t[0] for t in TARGETS])
    args = ap.parse_args()
    recs, pending = {}, []
    for name, rel, kind, target, fmt in TARGETS:
        if args.only and name != args.only:
            continue
        try:
            recs[name] = process(name, rel, kind, target, fmt)
        except FileNotFoundError as e:
            pending.append({"name": name, "missing_source": str(e)})
            print(f"{name}: SKIPPED ({e})")
    if not recs:
        raise SystemExit("nothing built: all sources missing")
    man = {"producer": "tools/enemy_upscale_batch.py (side/enemy-upscale-batch)",
           "method": "2x steps: restrained-sharpen albedo/spec/emissive, "
                     "renormalized normals, plain Lanczos masks; alpha preserved; "
                     "mirrors texture_pipeline.py primitives",
           "pending_extraction__needs_MAIN_readonly_umodel_run": pending,
           "injector_note": "merge the targets[] entries below into "
                            "asset-tools/texture-injector/targets_generated.h "
                            "(file itself untouched by Side)",
           "targets": [{"name": r["name"], "format": r["format"],
                        "px": r["target_px"][0],
                        "fnv1a64_mip0": r["fnv1a64_mip0"]} for r in recs.values()],
           "assets": recs}
    (OUT / "MANIFEST.json").write_text(json.dumps(man, indent=2), encoding="utf-8")
    total_vram = sum(r["vram_bytes"] for r in recs.values())
    print(f"MANIFEST ok; batch VRAM {total_vram/1048576:.1f} MB")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
