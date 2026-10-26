"""Verify private first-person DDS payloads against a public hash manifest.

Offline only. No game files are read or written. Run before any MAIN A/B.
"""
from __future__ import annotations

import argparse
import hashlib
import json
import re
from pathlib import Path

from dds_io import fnv1a64_mip0


def dds_header(path: Path) -> dict:
    data = path.read_bytes()
    if len(data) < 128 or data[:4] != b"DDS ":
        raise ValueError(f"invalid DDS: {path}")
    height = int.from_bytes(data[12:16], "little")
    width = int.from_bytes(data[16:20], "little")
    mip_count = int.from_bytes(data[28:32], "little")
    fmt = data[84:88].decode("ascii")
    if fmt not in {"DXT1", "DXT5"} or not width or not height:
        raise ValueError(f"unsupported DDS layout: {path}")
    full_count = max(width, height).bit_length()
    if mip_count != full_count:
        raise ValueError(f"incomplete mip chain: {path}")
    block_bytes = 8 if fmt == "DXT1" else 16
    expected = 128
    w, h = width, height
    for _ in range(mip_count):
        expected += ((w + 3) // 4) * ((h + 3) // 4) * block_bytes
        w, h = max(1, w // 2), max(1, h // 2)
    if expected != len(data):
        raise ValueError(f"truncated/trailing DDS bytes: {path}")
    return {"width": width, "height": height, "fourcc": fmt, "mip_count": mip_count}


def verify(manifest: dict, stage: Path, targets: str, originals: Path | None = None) -> dict:
    seen = set()
    receipts = {}
    for name, spec in manifest["maps"].items():
        if name in seen:
            raise ValueError(f"duplicate identity: {name}")
        seen.add(name)
        if spec["status"] != "staged":
            continue
        if not re.fullmatch(r"[A-Za-z0-9_]+", name):
            raise ValueError(f"unsafe name: {name}")
        source_layout = spec["source_layout"]
        expected_target = (f'{{ "{name}", {source_layout[0]}u, {source_layout[1]}u, '
                           f'D3DFMT_{spec["fourcc"]}, {spec["source_fnv64"]}ULL, "{name}.dds"')
        if targets.count(expected_target) != 1:
            raise ValueError(f"target identity/layout mismatch: {name}")
        if originals is not None:
            source_name = spec.get("source_name", name)
            if not re.fullmatch(r"[A-Za-z0-9_]+", source_name):
                raise ValueError(f"unsafe source identity: {name}")
            original = originals / spec["source_package"] / "Texture2D" / f"{source_name}.dds"
            if not original.is_file():
                raise FileNotFoundError(original)
            if hashlib.sha256(original.read_bytes()).hexdigest().lower() != spec["source_sha256"].lower():
                raise ValueError(f"original source hash mismatch: {name}")
            source_dds = dds_header_source(original)
            if (source_dds["width"], source_dds["height"], source_dds["fourcc"]) != (
                *source_layout, spec["source_fourcc"]
            ):
                raise ValueError(f"original source layout mismatch: {name}")
            if fnv1a64_mip0(original) != spec["source_fnv64"]:
                raise ValueError(f"original source FNV mismatch: {name}")
        path = stage / f"{name}.dds"
        if not path.is_file():
            raise FileNotFoundError(path)
        digest = hashlib.sha256(path.read_bytes()).hexdigest()
        if digest.lower() != spec["sha256"].lower():
            raise ValueError(f"hash mismatch: {name}")
        layout = dds_header(path)
        if (layout["width"], layout["height"], layout["fourcc"]) != (
            spec["width"], spec["height"], spec["fourcc"]
        ):
            raise ValueError(f"replacement layout mismatch: {name}")
        receipts[name] = {"sha256": digest, **layout}
    if {p.stem for p in stage.glob("*.dds")} != set(receipts):
        raise ValueError("unregistered or missing DDS in stage")
    return receipts


def dds_header_source(path: Path) -> dict:
    """Extracted UModel sources may contain only mip0, unlike replacements."""
    data = path.read_bytes()[:128]
    if len(data) != 128 or data[:4] != b"DDS ":
        raise ValueError(f"invalid original DDS: {path}")
    return {"width": int.from_bytes(data[16:20], "little"),
            "height": int.from_bytes(data[12:16], "little"),
            "fourcc": data[84:88].decode("ascii")}


if __name__ == "__main__":
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument("--manifest", type=Path, default=Path(__file__).resolve().parents[2] / "docs" / "textures" / "hires-first-person-manifest.json")
    p.add_argument("--stage", type=Path, required=True)
    p.add_argument("--targets", type=Path, default=Path(__file__).resolve().parents[2] / "spikes" / "texture_cache" / "targets_generated.h")
    p.add_argument("--originals", type=Path, help="optional private extracted original/ directory for SHA/FNV verification")
    a = p.parse_args()
    print(json.dumps(verify(json.loads(a.manifest.read_text(encoding="utf-8")), a.stage,
                            a.targets.read_text(encoding="utf-8"), a.originals), indent=2))
