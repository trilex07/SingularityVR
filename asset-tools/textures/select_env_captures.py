#!/usr/bin/env python3
"""
Select environment captures for upscaling.

Reads the injector's runtime captures (Binaries\\capture\\*.dds), keeps the
surface-like albedo textures at or above a minimum size, skips anything that is
already an injector target, copies the chosen ones into the local captured
sources and appends them to captured_sources.json (kind=albedo). The normal-map
look-alikes are detected by blue dominance and skipped (their lighting is left to
the original e_n maps for now).

Nothing is committed but the script and the resulting manifest; pixels stay local.
"""
from __future__ import annotations

import argparse
import json
import re
import shutil
from pathlib import Path

from PIL import Image
import numpy as np


def parse_target_hashes(header: Path) -> set[int]:
    if not header.exists():
        return set()
    txt = header.read_text(encoding="utf-8", errors="replace")
    return {int(m.group(1)) for m in re.finditer(r"(\d+)ULL", txt)}


def classify(img: Image.Image) -> str:
    a = np.asarray(img.convert("RGB"), dtype=np.float32)
    r, g, b = a[:, :, 0].mean(), a[:, :, 1].mean(), a[:, :, 2].mean()
    if b > r + 18 and b > g + 18 and b > 110:
        return "normal"
    return "albedo"


def main() -> None:
    ap = argparse.ArgumentParser()
    ap.add_argument("--capture", default=r"C:\Program Files (x86)\Steam\steamapps\common\Singularity\Binaries\capture")
    ap.add_argument("--local-root", default=r"C:\Users\Trilex\vr-mods\SingularityVR-assets-local")
    ap.add_argument("--manifest", default=r"C:\Users\Trilex\vr-mods\SingularityVR-assets\research\texture-upscale\manifests\captured_sources.json")
    ap.add_argument("--header", default=r"C:\Users\Trilex\vr-mods\SingularityVR-assets\asset-tools\texture-injector\targets_generated.h")
    ap.add_argument("--max", type=int, default=30)
    ap.add_argument("--min", type=int, default=512)
    a = ap.parse_args()

    capture = Path(a.capture)
    local = Path(a.local_root)
    dst = local / "textures" / "original" / "captured"
    dst.mkdir(parents=True, exist_ok=True)

    manifest_path = Path(a.manifest)
    entries = json.loads(manifest_path.read_text(encoding="utf-8")) if manifest_path.exists() else []
    existing = {e["name"] for e in entries}
    targets = parse_target_hashes(Path(a.header))

    # group capture files by content hash (the same hash may appear at several sizes)
    by_hash: dict[str, Path] = {}
    for f in sorted(capture.glob("*.dds")):
        m = re.match(r".*_([0-9A-F]{16})\.dds$", f.name)
        if not m:
            continue
        by_hash.setdefault(m.group(1), f)

    candidates = []
    for h, f in by_hash.items():
        full = f.stem.split("_")
        dims = next((p for p in full if "x" in p and p[0].isdigit()), "")
        try:
            img = Image.open(f)
            w, hh = img.size
        except Exception:
            continue
        if min(w, hh) < a.min:
            continue
        if int(h, 16) in targets:
            continue
        if classify(img) != "albedo":
            continue
        candidates.append((w * hh, h, f, dims, (w, hh)))

    candidates.sort(reverse=True)
    added = 0
    for area, h, f, dims, (w, hh) in candidates:
        name = f"env_{h[:8].lower()}"
        if name in existing:
            continue
        if added >= a.max:
            break
        shutil.copy2(f, dst / f"{name}.dds")
        entries.append({
            "name": name,
            "dds": f"textures/original/captured/{name}.dds",
            "kind": "albedo",
            "note": f"Environment surface, runtime capture {w}x{hh}.",
        })
        existing.add(name)
        added += 1
        print(f"+ {name}  {w}x{hh}  ({f.name})")

    manifest_path.write_text(json.dumps(entries, indent=2) + "\n", encoding="utf-8")
    print(f"added {added} environment captures; manifest now {len(entries)} entries")


if __name__ == "__main__":
    main()
