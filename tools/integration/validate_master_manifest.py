"""Read-only consistency check for the master SIDE integration manifest.

Never locates or modifies an installed game. Optional private directories must
be passed explicitly and should be existing offline SIDE workspaces.
"""

import argparse
import hashlib
import json
from pathlib import Path
import struct


REPO = Path(__file__).resolve().parents[2]
MANIFEST = REPO / "docs/integration/MASTER_INTEGRATION_MANIFEST.json"


def sha256(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        for chunk in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest().upper()


def check_hashes(entries: dict[str, str], root: Path, recursive: bool) -> int:
    for name, expected in entries.items():
        matches = list(root.rglob(name)) if recursive else [root / name]
        matches = [item for item in matches if item.is_file()]
        if len(matches) != 1:
            raise ValueError(f"expected one private candidate for {name}, found {len(matches)}")
        actual = sha256(matches[0])
        if actual != expected:
            raise ValueError(f"SHA256 mismatch: {name}: {actual} != {expected}")
    return len(entries)


def check_dds_headers(root: Path) -> int:
    metadata = json.loads((REPO / "processed/enemy-upscale-batch/MANIFEST.json").read_text(encoding="utf-8"))["assets"]
    for name, asset in metadata.items():
        path = root / f"{name}.dds"
        with path.open("rb") as stream:
            header = stream.read(128)
        if len(header) != 128 or header[:4] != b"DDS ":
            raise ValueError(f"invalid DDS header: {path}")
        size = struct.unpack_from("<I", header, 4)[0]
        height, width = struct.unpack_from("<II", header, 12)
        mips = struct.unpack_from("<I", header, 28)[0]
        fourcc = header[84:88].decode("ascii", errors="replace")
        expected_width, expected_height = asset["target_px"]
        if (size, width, height, mips, fourcc) != (
            124, expected_width, expected_height, asset["mip_levels"], asset["format"]
        ):
            raise ValueError(f"DDS geometry/format/mip mismatch: {name}")
    return len(metadata)


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--bink-dir", type=Path, help="explicit offline SIDE Bink directory")
    parser.add_argument("--dds-dir", type=Path, help="explicit offline SIDE DDS directory")
    args = parser.parse_args()
    manifest = json.loads(MANIFEST.read_text(encoding="utf-8"))
    modules = {item["id"]: item for item in manifest["modules"]}
    if len(modules) != len(manifest["modules"]):
        raise ValueError("duplicate module IDs")
    if set(modules) != {f"M{i:02}" for i in range(9)}:
        raise ValueError("module set must be M00..M08")
    if modules["M00"]["features"]["HipKnifeWithTMD"] != 0:
        raise ValueError("TMD knife safety baseline changed")
    if modules["M00"]["features"]["TwoHandSpreadPercent"] != 100:
        raise ValueError("default supported spread no longer native")
    count = check_hashes(modules["M02"]["atlas_sha256"], REPO, False)
    print(f"master manifest: schema PASS; {count} local atlas SHA256 PASS")
    for option, module, key in (
        (args.bink_dir, "M04", "private_candidates"),
        (args.dds_dir, "M06", "candidate_dds_sha256"),
    ):
        if option is not None:
            if not option.is_dir():
                raise ValueError(f"missing offline private directory: {option}")
            count = check_hashes(modules[module][key], option, True)
            print(f"{module}: {count} explicit private candidate SHA256 PASS")
            if module == "M04":
                for name in modules[module][key]:
                    matches = list(option.rglob(name))
                    with matches[0].open("rb") as stream:
                        magic = stream.read(4)
                    if magic != b"BIKi":
                        raise ValueError(f"not Bink 1: {name}")
                print(f"M04: {count} Bink 1 magic PASS; runtime decoder NOT TESTED")
            if module == "M06":
                print(f"M06: {check_dds_headers(option)} DDS headers/mips/formats PASS")
        else:
            print(f"{module}: private files NOT CHECKED (pass explicit --*-dir)")


if __name__ == "__main__":
    main()
