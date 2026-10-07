#!/usr/bin/env python3
"""
Singularity VR - first-person texture upscale research pipeline.

Scope (deliberately narrow, see research/texture-upscale/TEXTURE_UPSCALE_PLAN.md):
  * first-person TMD hands / arms  (ch_inview_tmd_mesh, ch_inview_noTMD)
  * first-person player weapons    (ch_inview_* weapon meshes)

This tool NEVER writes to the game installation, CookedPC packages, or the
production worktree. It only:
  1. builds a scratch search dir with hard-links to the read-only packages
     plus aliases of coalesced.tfc named Textures_P0..P3.tfc,
  2. drives UModel to dump material/texture metadata and export top-mip DDS/PNG,
  3. classifies textures from the authoritative material parameter names,
  4. produces 2x candidates with conservative (non-generative) methods,
  5. validates original-vs-candidate metadata and estimates VRAM.

Game-derived pixels are written ONLY under --local-root (default
C:\\Users\\Trilex\\vr-mods\\SingularityVR-assets-local) and must never be
committed. Only JSON manifests / hashes / docs live under the repo.
"""
from __future__ import annotations

import argparse
import hashlib
import io
import json
import os
import re
import shutil
import struct
import subprocess
import sys
from dataclasses import dataclass, field, asdict
from pathlib import Path

import numpy as np
from PIL import Image, ImageFilter

# --------------------------------------------------------------------------- #
# locations / defaults
# --------------------------------------------------------------------------- #
DEFAULT_GAME = Path(r"C:\Program Files (x86)\Steam\steamapps\common\Singularity")
DEFAULT_UMODEL = Path(r"C:\Users\Trilex\vr-mods\research\tools\UEViewer\umodel.exe")
DEFAULT_LOCAL = Path(r"C:\Users\Trilex\vr-mods\SingularityVR-assets-local")

HERE = Path(__file__).resolve().parent            # <repo>/asset-tools/textures
REPO = HERE.parents[1]                             # <repo>
MANIFEST_DIR = REPO / "research" / "texture-upscale" / "manifests"

# first-person meshes we care about (package, mesh)
FP_MESHES = [
    ("RvGame", "ch_inview_tmd_mesh"),
    ("RvGame", "ch_inview_noTMD"),
    ("RvGame", "ch_inview_machinegun_mesh"),
    ("RvGame", "ch_inview_alt_pistol_mesh"),
    ("RvGame", "ch_inview_MP_mesh"),
    ("RvGameMPLoader_SF", "ch_inview_pistol_mesh"),
    ("RvGameMPLoader_SF", "ch_inview_shotgun_mesh"),
    ("RvGameMPLoader_SF", "ch_inview_sniper_mesh"),
    ("RvGameMPLoader_SF", "ch_inview_gatling_mesh"),
    ("RvGameMPLoader_SF", "ch_inview_railgun_mesh"),
]

# packages tried when resolving an object by name
PROBE_PACKAGES = ["RvGame", "RvGameMPLoader_SF", "RvGameMP"]

# packages that must be present in the scratch dir for import resolution
SCRATCH_PACKAGES = [
    "RvGame.xxx", "RvGameMP.xxx", "Startup_INT.xxx", "RvGameMPLoader_SF.xxx", "Core.xxx",
    "Engine.xxx", "GameFramework.xxx", "RavenShared.xxx", "GFxUI.xxx",
    "IpDrv.xxx", "OnlineSubsystemDemonware.xxx", "Kynapse.xxx",
    "Fightstyles.xxx", "Demonware.xxx",
]
TFC_ALIASES = ["Textures_P0.tfc", "Textures_P1.tfc", "Textures_P2.tfc", "Textures_P3.tfc"]

# authoritative material parameter -> classification
PARAM_CLASS = {
    "diffusemap": "ALBEDO",
    "normalmap": "NORMAL",
    "specularmap": "SPECULAR",
    "detailmap": "DETAIL",
    "emissivemap": "EMISSIVE",
    "emissivemask": "EMISSIVE",
    "rimmask": "MASK",
    "envmask": "MASK",
    "transmissionmask": "MASK",
    "opacitymap": "MASK",
    "scopemask": "MASK",
    "sphericalmap": "SPHERICAL",
    "reflectionmap": "REFLECTION",
    "envmap": "REFLECTION",
}

# format -> bytes per pixel on disk (packed)
FMT_BPP = {
    "PF_DXT1": 0.5, "PF_DXT3": 1.0, "PF_DXT5": 1.0,
    "PF_BC5": 1.0, "PF_A8R8G8B8": 4.0, "PF_A8": 1.0,
    "PF_G8": 1.0, "PF_R8G8B8A8": 4.0, "PF_BC4": 0.5,
}


# --------------------------------------------------------------------------- #
# small helpers
# --------------------------------------------------------------------------- #
def sha256_file(path: Path) -> str:
    h = hashlib.sha256()
    with path.open("rb") as f:
        for chunk in iter(lambda: f.read(1 << 20), b""):
            h.update(chunk)
    return h.hexdigest()


def log(msg: str) -> None:
    print(f"[textures] {msg}", flush=True)


def run_umodel(umodel: Path, scratch: Path, args: list[str]) -> str:
    cmd = [str(umodel), f"-path={scratch}", *args]
    p = subprocess.run(cmd, capture_output=True, text=True, errors="replace")
    return (p.stdout or "") + (p.stderr or "")


# --------------------------------------------------------------------------- #
# scratch dir with hard-linked packages + tfc aliases
# --------------------------------------------------------------------------- #
def ensure_scratch(game: Path, scratch: Path, umodel: Path) -> None:
    cooked = game / "RvGame" / "CookedPC"
    coalesced = cooked / "coalesced.tfc"
    if not coalesced.exists():
        raise SystemExit(f"coalesced.tfc not found in {cooked}")
    scratch.mkdir(parents=True, exist_ok=True)
    linked = 0
    for name in SCRATCH_PACKAGES:
        src = cooked / name
        dst = scratch / name
        if src.exists() and not dst.exists():
            try:
                os.link(src, dst)
            except OSError:
                shutil.copy2(src, dst)
            linked += 1
    for name in TFC_ALIASES:
        dst = scratch / name
        if not dst.exists():
            try:
                os.link(coalesced, dst)
            except OSError:
                shutil.copy2(coalesced, dst)
    log(f"scratch ready: {scratch} ({linked} packages, {len(TFC_ALIASES)} tfc aliases)")


# --------------------------------------------------------------------------- #
# umodel dump/export drivers
# --------------------------------------------------------------------------- #
def umodel_dump(umodel: Path, scratch: Path, pkg: str, name: str) -> str | None:
    out = run_umodel(umodel, scratch, ["-dump", pkg, name])
    if f"ObjectName: {name}" in out:
        return out
    return None


MIP_READ_RE = re.compile(r"Reading \S+ mip level (\d+) \((\d+)x(\d+)\)")


def umodel_export(umodel: Path, scratch: Path, out_dir: Path, pkg: str, name: str,
                  fmt: str) -> dict:
    """Export one object. UModel exports the highest-resolution mip actually
    present in the TFC, which is not always mip 0 - record which level we got."""
    flag = "-dds" if fmt == "dds" else "-png"
    out = run_umodel(umodel, scratch, ["-export", flag,
                                       f"-out={out_dir}", pkg, name])
    # UModel lays out <out>/<pkg>/Texture2D/<name>.<ext>
    hits = list(out_dir.rglob(f"{name}.{fmt}"))
    m = MIP_READ_RE.search(out)
    return {"ok": bool(hits),
            "mip_level": int(m.group(1)) if m else None,
            "dims": [int(m.group(2)), int(m.group(3))] if m else None}


TEX_FIELD_RE = {
    "size_x": re.compile(r"^\s*SizeX = (\d+)", re.M),
    "size_y": re.compile(r"^\s*SizeY = (\d+)", re.M),
    "format": re.compile(r"^\s*Format = (PF_\w+)", re.M),
    "mip_tail": re.compile(r"^\s*MipTailBaseIdx = (\d+)", re.M),
    "compression": re.compile(r"^\s*CompressionSettings = (TC_\w+)", re.M),
    "cache": re.compile(r"^\s*TextureFileCacheName = (\S+)", re.M),
}


def parse_texture_dump(text: str) -> dict:
    d = {}
    for k, rx in TEX_FIELD_RE.items():
        m = rx.search(text)
        if m:
            d[k] = int(m.group(1)) if k in ("size_x", "size_y", "mip_tail") else m.group(1)
    return d


MATERIAL_RE = re.compile(
    r"Materials\[(\d+)\] = \{?\s*MaterialInstanceConstant'([^']+)'")
PARAM_VALUE_RE = re.compile(r"ParameterValue = Texture2D'[^']*\.Textures\.([^']+)'")
PARAM_VALUE2_RE = re.compile(r"ParameterValue = Texture2D'([^']+)'")
PARAM_NAME_RE = re.compile(r"^\s*ParameterName = (\S+)", re.M)
TEXTURE_LOAD_RE = re.compile(r"Loading Texture2D (\w+) from package (\S+?)(?:\.xxx)?$", re.M)


def parse_mesh_materials(text: str) -> list[dict]:
    mats = []
    for idx, path in MATERIAL_RE.findall(text):
        mats.append({"slot": int(idx), "material_path": path,
                     "material": path.rsplit(".", 1)[-1]})
    mats.sort(key=lambda m: m["slot"])
    return mats


def parse_material_params(text: str) -> dict[str, str]:
    """Return {param_name: texture_name} from a material dump."""
    out: dict[str, str] = {}
    # walk line-pairs: ParameterValue then ParameterName
    lines = text.splitlines()
    last_tex = None
    for ln in lines:
        mv = PARAM_VALUE_RE.search(ln) or PARAM_VALUE2_RE.search(ln)
        if mv:
            last_tex = mv.group(1).rsplit(".", 1)[-1]
            continue
        mn = PARAM_NAME_RE.search(ln)
        if mn and last_tex:
            out[mn.group(1).strip()] = last_tex
            last_tex = None
    return out


# --------------------------------------------------------------------------- #
# manifests
# --------------------------------------------------------------------------- #
def write_json(path: Path, obj) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(json.dumps(obj, indent=2, sort_keys=False) + "\n", encoding="utf-8")
    log(f"wrote {path.relative_to(REPO) if path.is_relative_to(REPO) else path}")


# --------------------------------------------------------------------------- #
# command: build (extract + metadata + material map + classification)
# --------------------------------------------------------------------------- #
def cmd_build(a) -> None:
    game, local, umodel = Path(a.game), Path(a.local_root), Path(a.umodel)
    scratch = local / "cooked"
    logs = local / "logs"
    dds_dir = local / "textures" / "original"
    png_dir = local / "textures" / "original-png"
    for p in (logs, dds_dir, png_dir):
        p.mkdir(parents=True, exist_ok=True)
    ensure_scratch(game, scratch, umodel)

    # 1. dump meshes -> material list
    material_map: dict[str, dict] = {}
    mesh_records = []
    for pkg, mesh in FP_MESHES:
        text = umodel_dump(umodel, scratch, pkg, mesh)
        (logs / f"mesh-{mesh}.txt").write_text(text or "MISSING\n", encoding="utf-8")
        if not text:
            log(f"mesh not found: {pkg}/{mesh}")
            continue
        mats = parse_mesh_materials(text)
        rec = {"mesh": mesh, "package": pkg, "materials": mats}
        mesh_records.append(rec)
        log(f"mesh {mesh}: {len(mats)} material slot(s)")
        for m in mats:
            material_map.setdefault(m["material"], {"material_path": m["material_path"]})

    # 2. dump materials -> param -> texture
    for mat, info in material_map.items():
        pkg = None
        text = None
        for cand in PROBE_PACKAGES:
            text = umodel_dump(umodel, scratch, cand, mat)
            if text:
                pkg = cand
                break
        if not text:
            log(f"material not found: {mat}")
            continue
        (logs / "mat" / f"{mat}.txt").parent.mkdir(parents=True, exist_ok=True)
        (logs / "mat" / f"{mat}.txt").write_text(text, encoding="utf-8")
        info["package"] = pkg
        info["params"] = parse_material_params(text)
        log(f"material {mat}: {len(info['params'])} texture param(s)")

    # 3. collect unique textures; dump metadata + export
    tex_names = sorted({t for info in material_map.values()
                        for t in info.get("params", {}).values()})
    metadata: dict[str, dict] = {}
    export_status: dict[str, dict] = {}
    for t in tex_names:
        pkg = None
        text = None
        for cand in PROBE_PACKAGES:
            text = umodel_dump(umodel, scratch, cand, t)
            if text:
                pkg = cand
                break
        if not text:
            export_status[t] = {"found": False}
            continue
        (logs / "tex" / f"{t}.txt").parent.mkdir(parents=True, exist_ok=True)
        (logs / "tex" / f"{t}.txt").write_text(text, encoding="utf-8")
        meta = parse_texture_dump(text)
        meta["package"] = pkg
        metadata[t] = meta
        dds_info = umodel_export(umodel, scratch, dds_dir, pkg, t, "dds")
        png_info = umodel_export(umodel, scratch, png_dir, pkg, t, "png")
        export_status[t] = {"found": True, **dds_info}
        if dds_info.get("dims"):
            meta["exported_dims"] = dds_info["dims"]
            meta["exported_mip_level"] = dds_info["mip_level"]
        log(f"texture {t}: object {meta.get('size_x')}x{meta.get('size_y')} "
            f"{meta.get('format')} -> exported {dds_info.get('dims')} "
            f"(mip {dds_info.get('mip_level')}) "
            f"dds={dds_info['ok']} png={png_info['ok']}")

    # 4. annotate metadata with roles + hashes
    for mat, info in material_map.items():
        for param, tex in info.get("params", {}).items():
            t = tex.split(".")[-1]
            if t in metadata:
                metadata[t].setdefault("roles", [])
                role = PARAM_CLASS.get(param.lower(), "UNKNOWN")
                metadata[t]["roles"].append({"param": param, "class": role,
                                             "material": mat})
                metadata[t].setdefault("materials", [])
                if mat not in metadata[t]["materials"]:
                    metadata[t]["materials"].append(mat)
    local_root = Path(a.local_root)
    for t, meta in metadata.items():
        src = next(iter(dds_dir.rglob(f"{t}.dds")), None)
        if src:
            meta["source_dds"] = str(src.relative_to(local_root))
            meta["sha256"] = sha256_file(src)
            with Image.open(src) as im:
                meta["decoded_size"] = list(im.size)
                meta["decoded_mode"] = im.mode
            meta["file_size"] = src.stat().st_size

    manifests = {
        "material_map.json": {"meshes": mesh_records, "materials": material_map},
        "texture_metadata.json": {"textures": metadata, "export_status": export_status},
    }
    for name, obj in manifests.items():
        write_json(MANIFEST_DIR / name, obj)


# --------------------------------------------------------------------------- #
# command: classify
# --------------------------------------------------------------------------- #
def cmd_classify(a) -> None:
    meta_path = MANIFEST_DIR / "texture_metadata.json"
    mm_path = MANIFEST_DIR / "material_map.json"
    data = json.loads(meta_path.read_text(encoding="utf-8"))
    textures = data["textures"]
    by_class: dict[str, list[str]] = {}
    records = {}
    for t, m in textures.items():
        classes = sorted({r["class"] for r in m.get("roles", [])})
        primary = classes[0] if classes else guess_class(t, m)
        proven = bool(m.get("roles"))
        by_class.setdefault(primary, []).append(t)
        records[t] = {
            "class": primary,
            "proven": proven,
            "dims": [m.get("size_x"), m.get("size_y")],
            "format": m.get("format"),
            "compression": m.get("compression"),
            "cache": m.get("cache"),
            "package": m.get("package"),
            "materials": m.get("materials", []),
            "roles": m.get("roles", []),
            "sha256": m.get("sha256"),
        }
    out = {"by_class": {k: sorted(v) for k, v in sorted(by_class.items())},
           "textures": records}
    write_json(MANIFEST_DIR / "classification.json", out)
    for k, v in sorted(by_class.items()):
        log(f"{k}: {len(v)}")


def guess_class(name: str, meta: dict) -> str:
    n = name.lower()
    if meta.get("compression") == "TC_Normalmap" or n.endswith("_n"):
        return "NORMAL(likely-name)"
    if n.endswith("_e"):
        return "EMISSIVE(likely-name)"
    if n.endswith("_s"):
        return "SPECULAR(likely-name)"
    if n.endswith("_d"):
        return "ALBEDO(likely-name)"
    return "UNKNOWN"


# --------------------------------------------------------------------------- #
# DDS helpers
# --------------------------------------------------------------------------- #
def dds_fourcc(path: Path) -> str:
    with path.open("rb") as f:
        hdr = f.read(128)
    return hdr[84:88].decode("latin1")


def fmt_from_fourcc(fourcc: str, pf_format: str | None = None) -> str:
    if fourcc == "DXT1":
        return "PF_DXT1"
    if fourcc == "DXT3":
        return "PF_DXT3"
    if fourcc == "DXT5":
        return "PF_DXT5"
    if pf_format:
        return pf_format
    return f"UNKNOWN({fourcc})"


def load_rgba(path: Path) -> Image.Image:
    with Image.open(path) as im:
        return im.convert("RGBA")


def save_png(img: Image.Image, path: Path) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    img.convert("RGB").save(path)


def save_png_rgba(img: Image.Image, path: Path) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    img.save(path)


def encode_level(img: Image.Image, fmt: str) -> bytes:
    buf = io.BytesIO()
    img.convert("RGBA").save(buf, format="DDS", pixel_format=fmt)
    return buf.getvalue()[128:]


def save_dds_mips(path: Path, mips: list[Image.Image], fmt: str) -> None:
    fourcc = fmt.encode("latin1")
    w, h = mips[0].size
    hdr = bytearray(128)
    hdr[0:4] = b"DDS "
    struct.pack_into("<I", hdr, 4, 124)
    flags = 0x1 | 0x2 | 0x4 | 0x1000 | 0x80000   # caps|height|width|pixelformat|mipmapcount
    struct.pack_into("<I", hdr, 8, flags)
    struct.pack_into("<I", hdr, 12, h)
    struct.pack_into("<I", hdr, 16, w)
    struct.pack_into("<I", hdr, 20, 0)
    struct.pack_into("<I", hdr, 24, 0)
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


def read_dds_mipmaps(path: Path) -> int:
    with path.open("rb") as f:
        hdr = f.read(128)
    return struct.unpack_from("<I", hdr, 28)[0]


# --------------------------------------------------------------------------- #
# upscalers (conservative, non-generative)
# --------------------------------------------------------------------------- #
def up_lanczos(img: Image.Image, scale: int) -> Image.Image:
    w, h = img.size
    return img.resize((w * scale, h * scale), Image.LANCZOS)


def up_ibp(img: Image.Image, scale: int, iters: int = 8, down_filter=Image.BOX) -> Image.Image:
    """Iterative back-projection: refine a Lanczos estimate so its down-sample
    matches the source. Recovers edge detail without inventing content."""
    lr = np.asarray(img.convert("RGB"), dtype=np.float32) / 255.0
    hr0 = up_lanczos(img.convert("RGB"), scale)
    hr = np.asarray(hr0, dtype=np.float32) / 255.0
    lr_img = img.convert("RGB")
    for _ in range(iters):
        est = Image.fromarray((np.clip(hr, 0, 1) * 255).astype(np.uint8), "RGB")
        est_lr = np.asarray(est.resize(img.size, down_filter), dtype=np.float32) / 255.0
        err = lr - est_lr
        err_up = np.asarray(
            Image.fromarray((np.clip(err + 0.5, 0, 1) * 255).astype(np.uint8), "RGB")
            .resize(hr0.size, Image.BICUBIC), dtype=np.float32) / 255.0 - 0.5
        hr = np.clip(hr + err_up, 0, 1)
    out = Image.fromarray((hr * 255).astype(np.uint8), "RGB").convert("RGBA")
    # keep original alpha
    out.putalpha(img.convert("RGBA").getchannel("A").resize(out.size, Image.LANCZOS))
    return out


def up_restrained_sharpen(img: Image.Image, scale: int, amount: float = 0.35,
                          radius: float = 1.0, threshold: float = 3.0) -> Image.Image:
    """Lanczos + threshold-limited unsharp. Flat areas are untouched so noise,
    thresholds and specular boundaries do not ring."""
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


# --------------------------------------------------------------------------- #
# normal handling
# --------------------------------------------------------------------------- #
def normal_reconstruct(rgb: np.ndarray) -> np.ndarray:
    v = rgb.astype(np.float32) / 127.5 - 1.0
    n = np.linalg.norm(v, axis=2, keepdims=True)
    n[n == 0] = 1.0
    return np.clip((v / n + 1.0) * 127.5, 0, 255).astype(np.uint8)


def up_normal(img: Image.Image, scale: int, method: str = "lanczos") -> Image.Image:
    if method == "ibp":
        up = up_ibp(img, scale)
    else:
        up = up_lanczos(img, scale)
    arr = normal_reconstruct(np.asarray(up.convert("RGB")))
    return Image.fromarray(arr, "RGB").convert("RGBA")


def make_mips(img: Image.Image, is_normal: bool, min_size: int = 4) -> list[Image.Image]:
    mips = []
    cur = img
    while True:
        rgb = np.asarray(cur.convert("RGB"))
        out = normal_reconstruct(rgb) if is_normal else rgb
        mips.append(Image.fromarray(out, "RGB").convert("RGBA"))
        w, h = cur.size
        if w <= min_size and h <= min_size:
            break
        cur = cur.resize((max(1, w // 2), max(1, h // 2)), Image.LANCZOS)
    return mips


# --------------------------------------------------------------------------- #
# command: upscale + experiment
# --------------------------------------------------------------------------- #
EXPERIMENT_SET = [
    # texture, kind, note
    ("inview_glove_d", "albedo", "first-person glove/hand diffuse (highest priority)"),
    ("AR_E99Rifle_alt_d", "albedo", "E99 assault rifle FP diffuse (most visible weapon)"),
    ("inview_glove_n", "normal", "glove tangent normal - technically-correct path only"),
]


def cmd_upscale(a) -> None:
    local, umodel = Path(a.local_root), Path(a.umodel)
    data = json.loads((MANIFEST_DIR / "texture_metadata.json").read_text(encoding="utf-8"))
    textures = data["textures"]
    out_root = local / "textures" / "upscaled"
    results = {}
    for name, kind, note in EXPERIMENT_SET:
        if name not in textures:
            log(f"skip {name}: no metadata")
            continue
        src = local / textures[name]["source_dds"]
        if not src.exists():
            log(f"skip {name}: source dds missing ({src})")
            continue
        fmt = fmt_from_fourcc(dds_fourcc(src), textures[name].get("format"))
        dds_fmt = "DXT5" if fmt == "PF_DXT5" else "DXT1"
        img = load_rgba(src)
        if kind == "normal":
            cand_a = up_normal(img, 2, "lanczos")
            cand_b = up_normal(img, 2, "ibp")
            fmt_a = fmt_b = dds_fmt
        else:
            cand_a = up_lanczos(img, 2)
            cand_b = up_ibp(img, 2)
            fmt_a = fmt_b = dds_fmt
        tmeta = textures[name]
        entry = {"note": note, "kind": kind, "source": name,
                 "object_dims": [tmeta.get("size_x"), tmeta.get("size_y")],
                 "source_dims": list(img.size),
                 "source_mip_level": tmeta.get("exported_mip_level"),
                 "format": fmt, "dds_format": dds_fmt,
                 "candidates": {}}
        for tag, cand in (("A_lanczos", cand_a), ("B_ibp", cand_b)):
            mips = make_mips(cand, kind == "normal")
            dds_path = out_root / f"{name}__{tag}.dds"
            save_dds_mips(dds_path, mips, fmt_a)
            save_png_rgba(cand, out_root / f"{name}__{tag}.png")
            alpha = np.asarray(cand)[:, :, 3]
            entry["candidates"][tag] = {
                "dims": list(cand.size), "mips": len(mips),
                "dds_bytes": dds_path.stat().st_size,
                "sha256": sha256_file(dds_path),
                "alpha_min": int(alpha.min()), "alpha_max": int(alpha.max()),
            }
        results[name] = entry
        make_comparison(img, cand_a, cand_b, out_root / f"{name}__compare.png", name)
        log(f"upscaled {name}: {entry['source_dims']} -> {list(cand_a.size)}")

    write_json(MANIFEST_DIR / "upscale_results.json", results)


def make_comparison(orig: Image.Image, a: Image.Image, b: Image.Image, path: Path,
                    title: str, crop_frac: float = 0.5) -> None:
    """Native-resolution crop of original vs nearest-neighbour 2x (reference),
    vs candidate A and B - all at the same on-screen pixel scale."""
    w, h = orig.size
    cw, ch = int(w * crop_frac), int(h * crop_frac)
    x, y = (w - cw) // 2, (h - ch) // 2
    o = orig.crop((x, y, x + cw, y + ch)).convert("RGB")
    nn = o.resize((cw * 2, ch * 2), Image.NEAREST)
    ca = a.crop((x * 2, y * 2, (x + cw) * 2, (y + ch) * 2)).convert("RGB")
    cb = b.crop((x * 2, y * 2, (x + cw) * 2, (y + ch) * 2)).convert("RGB")
    from PIL import ImageDraw
    tiles = [(o, "ORIGINAL 1x"), (nn, "2x nearest (ref)"), (ca, "A lanczos"), (cb, "B ibp")]
    tw = max(t[0].width for t in tiles)
    th = sum(t[0].height + 18 for t in tiles)
    canvas = Image.new("RGB", (tw, th), (24, 24, 28))
    d = ImageDraw.Draw(canvas)
    yy = 0
    for im, label in tiles:
        canvas.paste(im, (0, yy))
        d.text((4, yy + im.height + 2), f"{title} - {label}", fill=(220, 220, 220))
        yy += im.height + 18
    path.parent.mkdir(parents=True, exist_ok=True)
    canvas.save(path)


# --------------------------------------------------------------------------- #
# command: validate
# --------------------------------------------------------------------------- #
def mip_chain_bytes(w: int, h: int, bpp: float, levels: int) -> int:
    total = 0
    for i in range(levels):
        total += max(1, w >> i) * max(1, h >> i) * bpp
    return int(round(total))


def cmd_validate(a) -> None:
    local = Path(a.local_root)
    data = json.loads((MANIFEST_DIR / "texture_metadata.json").read_text(encoding="utf-8"))
    up_path = MANIFEST_DIR / "upscale_results.json"
    if not up_path.exists():
        raise SystemExit("run `upscale` first")
    up = json.loads(up_path.read_text(encoding="utf-8"))
    report = {"checks": [], "warnings": []}
    for name, entry in up.items():
        src = local / data["textures"][name]["source_dds"]
        src_dims = tuple(entry["source_dims"])
        fmt = entry["format"]
        bpp = FMT_BPP.get(fmt, 0.5)
        for tag, c in entry["candidates"].items():
            cdims = tuple(c["dims"])
            checks = {
                "texture": name, "candidate": tag,
                "orig_dims": list(src_dims), "new_dims": list(cdims),
                "orig_format": fmt, "new_format": entry["dds_format"],
                "orig_mips": read_dds_mipmaps(src),
                "new_mips": c["mips"],
                "orig_bytes": src.stat().st_size, "new_bytes": c["dds_bytes"],
                "orig_sha256": data["textures"][name].get("sha256"),
                "new_sha256": c["sha256"],
            }
            checks["mip_level_upscaled"] = entry.get("source_mip_level")
            report["checks"].append(checks)
            if cdims != (src_dims[0] * 2, src_dims[1] * 2):
                report["warnings"].append(
                    f"{name}/{tag}: dims {cdims} are not 2x {src_dims}")
            if entry.get("source_mip_level") not in (0, None):
                report["warnings"].append(
                    f"{name}/{tag}: upscaled from mip {entry['source_mip_level']} "
                    f"({src_dims}), NOT object mip0 {entry.get('object_dims')}")
            if entry["dds_format"] != ("DXT5" if fmt == "PF_DXT5" else "DXT1"):
                report["warnings"].append(f"{name}/{tag}: DXT format changed unexpectedly")
            if c["mips"] < 2:
                report["warnings"].append(f"{name}/{tag}: mip chain too short ({c['mips']})")
            expect_mips = _expect_mips(src_dims, tag)
            if abs(c["mips"] - expect_mips) > 1:
                report["warnings"].append(
                    f"{name}/{tag}: mip count {c['mips']} far from expected {expect_mips}")
            # alpha presence: only meaningful for DXT5 sources
            if fmt == "PF_DXT5":
                o = np.asarray(load_rgba(src))[:, :, 3]
                nmin = _alpha_min(up, name, tag)
                if o.min() < 250 and nmin is not None and nmin > 250:
                    report["warnings"].append(
                        f"{name}/{tag}: source had non-opaque alpha, candidate lost it")
    write_json(MANIFEST_DIR / "validation.json", report)
    for w in report["warnings"]:
        log("WARN " + w)
    log(f"validated {len(report['checks'])} candidate(s), {len(report['warnings'])} warning(s)")


def _expect_mips(dims, tag) -> int:
    """Our chains run from the exported top level down to a 4x4 floor."""
    longest = max(dims)
    return int(np.floor(np.log2(longest))) - int(np.floor(np.log2(4))) + 1


def _alpha_min(up: dict, name: str, tag: str):
    entry = up.get(name, {}).get("candidates", {}).get(tag, {})
    return entry.get("alpha_min")


# --------------------------------------------------------------------------- #
# command: vram
# --------------------------------------------------------------------------- #
def _tex_bytes(meta: dict) -> int:
    w, h = meta["size_x"], meta["size_y"]
    bpp = FMT_BPP.get(meta["format"], 0.5)
    mip_tail = meta.get("mip_tail", 0)
    levels = max(1, int(np.log2(max(w, h))) + 1 - min(mip_tail, 0))
    return mip_chain_bytes(w, h, bpp, levels), bpp


def cmd_vram(a) -> None:
    data = json.loads((MANIFEST_DIR / "texture_metadata.json").read_text(encoding="utf-8"))
    textures = data["textures"]
    cls = json.loads((MANIFEST_DIR / "classification.json").read_text(encoding="utf-8"))
    hands = ["inview_glove_d", "inview_glove_n", "inview_glove_s", "DETAILfinepitted02a_n"]
    tmd = ["TMD_new2_d", "TMD_new_n", "TMD_new2_s", "TMD_new_s"]
    weapons = ["AR_E99Rifle_alt_d", "AR_E99Rifle_alt_s", "AR_E99Rifle_n",
               "AR_E99Rifle_alt_e", "AR_shotgun_d", "AR_shotgun_s", "AR_shotgun_n",
               "ar_sniper_rifle_d", "ar_sniper_rifle_s", "ar_sniper_rifle_n"]
    out = {"textures": {}, "groups": {}}

    def group(names):
        o = n = 0
        detail = []
        for t in names:
            m = textures.get(t)
            if not m:
                continue
            ob, bpp = _tex_bytes(m)
            w, h = m["size_x"], m["size_y"]
            nb = mip_chain_bytes(w * 2, h * 2, bpp, int(np.log2(max(w * 2, h * 2))) + 1)
            o += ob
            n += nb
            detail.append({"texture": t, "orig_bytes": ob, "new_bytes_2x": nb,
                           "delta_bytes": nb - ob, "format": m["format"]})
        return {"orig_bytes": o, "new_bytes_2x": n, "delta_bytes": n - o, "detail": detail}

    out["groups"]["hands_only"] = group(hands)
    out["groups"]["hands_plus_tmd"] = group(hands + tmd)
    out["groups"]["hands_tmd_selected_weapons"] = group(hands + tmd + weapons)
    for k, g in out["groups"].items():
        log(f"{k}: {g['orig_bytes']/1048576:.2f} MB -> {g['new_bytes_2x']/1048576:.2f} MB "
            f"(+{g['delta_bytes']/1048576:.2f} MB)")
    write_json(MANIFEST_DIR / "vram_budget.json", out)


# --------------------------------------------------------------------------- #
# cli
# --------------------------------------------------------------------------- #
def main() -> None:
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("--game", default=str(DEFAULT_GAME))
    ap.add_argument("--local-root", default=str(DEFAULT_LOCAL))
    ap.add_argument("--umodel", default=str(DEFAULT_UMODEL))
    sub = ap.add_subparsers(dest="cmd", required=True)
    for name, fn in (("build", cmd_build), ("classify", cmd_classify),
                     ("upscale", cmd_upscale), ("validate", cmd_validate),
                     ("vram", cmd_vram), ("experiment", None)):
        sub.add_parser(name)
    a = ap.parse_args()
    if a.cmd == "experiment":
        cmd_build(a)
        cmd_classify(a)
        cmd_upscale(a)
        cmd_validate(a)
        cmd_vram(a)
        return
    {"build": cmd_build, "classify": cmd_classify, "upscale": cmd_upscale,
     "validate": cmd_validate, "vram": cmd_vram}[a.cmd](a)


if __name__ == "__main__":
    main()
