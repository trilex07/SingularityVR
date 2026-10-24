"""Export four reviewed true-mip0 DDS maps to a private, non-Git directory.

This is an offline asset preparation tool. It never writes to a game install.
"""
from __future__ import annotations

import argparse
import hashlib
import json
import subprocess
from pathlib import Path

REF = "0dcdcd5c6dc677573df24ea5616b69ef71ebb8ff"
MAPS = {
    "AR_shotgun_d": ("material-remaster/weapon_shotgun/AR_shotgun_diffuse_2x.dds", "6340E3245854C8CFA9D32DC9DF3B82E7BA41DCF035B0C37219C08FE9A9983C65", b"DXT5"),
    "AR_shotgun_n": ("material-remaster/weapon_shotgun/AR_shotgun_normal_2x.dds", "76A7D6EE4F44682880F28B7E1ED5C76130FE6015162D1AF2CF0872401B770834", b"DXT1"),
    "inview_glove_d": ("material-remaster/glove/inview_glove_diffuse_2x.dds", "E702E96CBAE6AE73B24BF8E01969D0AF41F15BCF1E04CCAB2B8508435FAFF026", b"DXT1"),
    "inview_glove_n": ("material-remaster/glove/inview_glove_normal_2x.dds", "754C1B0617DB838421C6E4B85FE0A9FBDFCCFD14B7DD886A8CF516497C2CF3CA", b"DXT1"),
}


def inspect_dds(data: bytes, fourcc: bytes) -> dict[str, int | str]:
    if len(data) < 128 or data[:4] != b"DDS " or data[84:88] != fourcc:
        raise ValueError("DDS magic/format mismatch")
    height = int.from_bytes(data[12:16], "little")
    width = int.from_bytes(data[16:20], "little")
    levels = int.from_bytes(data[28:32], "little")
    if (width, height, levels) != (1024, 1024, 11):
        raise ValueError("Expected verified 1024x1024 true mip0 plus 11 levels")
    blocks = 8 if fourcc == b"DXT1" else 16
    size = 128 + sum(max(1, ((width >> i) + 3) >> 2) * max(1, ((height >> i) + 3) >> 2) * blocks for i in range(levels))
    if size != len(data):
        raise ValueError("DDS mip chain is truncated or has trailing bytes")
    return {"width": width, "height": height, "levels": levels, "format": fourcc.decode()}


def export(repo: Path, output: Path) -> dict:
    repo = repo.resolve()
    output = output.resolve()
    if output == repo or repo in output.parents:
        raise ValueError("Private DDS output must be outside this Git checkout")
    if any((parent / ".git").exists() for parent in (output, *output.parents)):
        raise ValueError("Private DDS output must be outside every Git checkout")
    if any(part.lower() in {"steam", "steamapps", "binaries"} for part in output.parts):
        raise ValueError("Refusing a game/Steam destination")
    payloads = {}
    report = {"source_commit": REF, "maps": {}}
    for name, (path, expected, fmt) in MAPS.items():
        data = subprocess.check_output(["git", "show", f"{REF}:{path}"], cwd=repo)
        sha = hashlib.sha256(data).hexdigest().upper()
        if sha != expected:
            raise ValueError(f"Source SHA mismatch for {name}")
        report["maps"][name] = {"sha256": sha, **inspect_dds(data, fmt)}
        payloads[name] = data
    for name, data in payloads.items():
        dest = output / f"{name}.dds"
        if dest.exists() and dest.read_bytes() != data:
            raise ValueError(f"Refusing to overwrite different private file: {dest}")
    output.mkdir(parents=True, exist_ok=True)
    for name, data in payloads.items():
        dest = output / f"{name}.dds"
        if not dest.exists():
            dest.write_bytes(data)
    return report


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--repo", type=Path, default=Path(__file__).resolve().parents[2])
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    print(json.dumps(export(args.repo, args.output), indent=2))


if __name__ == "__main__":
    main()
