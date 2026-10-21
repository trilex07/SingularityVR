"""Offline, identity-checked effects staging. Never writes to the game installation."""
import argparse
import hashlib
import io
import json
import re
import struct
from pathlib import Path

from PIL import Image

ROOT = Path(__file__).resolve().parents[1]
# Existing runtime identities plus three independently checked blood originals.
BLOOD = {
    "bsplat_decal_d": (128, 128, "DXT1", 4958202339012386159),
    "t_blood_splat02_d": (256, 256, "DXT5", 8762027923008784863),
}
NAMES = (
    "muzzleflashCenter", "muzzleFlashDir_01", "brightFlash_01", "spark_hit01",
    "sparks1_subuv", "explosion", "fireball_base_intense_02",
    "bsplat3_d", "bsplat_decal_d", "t_blood_splat02_d", "t_decal_metal_d_v4",
)


def require(ok, why):
    if not ok:
        raise ValueError(why)


def mip_size(w, h, fmt):
    return max(1, (w + 3) // 4) * max(1, (h + 3) // 4) * (8 if fmt == "DXT1" else 16)


def dds_validate(raw, complete=True):
    require(len(raw) >= 128 and raw[:4] == b"DDS ", "DDS magic/header")
    require(struct.unpack_from("<I", raw, 4)[0] == 124, "DDS header size")
    flags, h, w, linear = struct.unpack_from("<4I", raw, 8)
    mips = struct.unpack_from("<I", raw, 28)[0] or 1
    fmt = raw[84:88].decode("ascii")
    require(w > 0 and h > 0 and max(w, h) <= 8192 and fmt in ("DXT1", "DXT5"), "DDS dimensions/format")
    require(struct.unpack_from("<I", raw, 76)[0] == 32 and struct.unpack_from("<I", raw, 80)[0] & 4, "DDS FourCC flags")
    require(1 <= mips <= max(w, h).bit_length(), "DDS mip count")
    if complete:
        require(mips == max(w, h).bit_length(), "DDS complete mip chain")
        require(flags & 0x80000 and not flags & 8, "compressed DDS requires LINEARSIZE, not PITCH")
        require(linear == mip_size(w, h, fmt), "DDS top-level linear size")
        require(flags & 0x21007 == 0x21007, "DDS dimensions/pixel-format/mipmap flags")
        require(struct.unpack_from("<I", raw, 108)[0] & 0x401008 == 0x401008, "DDS texture/mipmap/complex caps")
    size = 128 + sum(mip_size(max(1, w >> i), max(1, h >> i), fmt) for i in range(mips))
    require(len(raw) == size, "DDS mip byte budget")
    return dict(w=w, h=h, mips=mips, fourcc=fmt, bytes=len(raw))


def build_dds(image, fmt):
    require(fmt in ("DXT1", "DXT5"), "unsupported compression")
    w, h = image.size
    levels, level = [], image.convert("RGBA")
    while True:
        encoded = io.BytesIO()
        level.save(encoded, format="DDS", pixel_format=fmt)
        raw = encoded.getvalue()
        dds_validate(raw, complete=False)
        levels.append(raw[128:])
        if level.size == (1, 1):
            break
        level = level.resize((max(1, level.width // 2), max(1, level.height // 2)), Image.Resampling.LANCZOS)
    header = bytearray(128)
    header[:4] = b"DDS "
    struct.pack_into("<7I", header, 4, 124, 0xA1007, h, w, mip_size(w, h, fmt), 0, len(levels))
    struct.pack_into("<2I", header, 76, 32, 4)
    header[84:88] = fmt.encode("ascii")
    struct.pack_into("<I", header, 108, 0x401008)
    raw = bytes(header) + b"".join(levels)
    dds_validate(raw)
    # Independent decode of the whole DDS, including its advertised mip layout.
    with Image.open(io.BytesIO(raw)) as decoded:
        decoded.load()
        require(decoded.size == (w, h), "DDS decoder dimensions")
    return raw


def fnv64(data):
    h = 0xCBF29CE484222325
    for byte in data:
        h = ((h ^ byte) * 0x100000001B3) & 0xFFFFFFFFFFFFFFFF
    return h


def targets():
    text = (ROOT / "spikes/texture_cache/targets_generated.h").read_text()
    result = {m[0]: (int(m[1]), int(m[2]), m[3], int(m[4])) for m in re.findall(
        r'\{\s*"([^"]+)",\s*(\d+)u,\s*(\d+)u,\s*D3DFMT_(DXT[15]),\s*(\d+)ULL', text)}
    for name, identity in BLOOD.items():
        require(name not in result or result[name] == identity, f"blood identity registry drift: {name}")
        result[name] = identity
    return result


def prepare(originals, enhanced, output, names=NAMES):
    # New directory only, under this checkout/build. No live directories, symlink
    # escapes, overwrites or partially valid payload advertised as complete.
    output = output.resolve()
    require(output.is_relative_to((ROOT / "build").resolve()) and output != (ROOT / "build").resolve(), "output must be under checkout/build")
    require(not output.exists(), "output already exists; use a new staging directory")
    require(len(names) > 0 and len(set(names)) == len(names) and all(n in NAMES for n in names), "invalid effects selection")
    identities, inputs = targets(), []
    for name in names:
        w, h, fmt, expected = identities[name]
        matches = []
        for path in originals.rglob(name + ".dds"):
            raw = path.read_bytes()
            desc = dds_validate(raw, complete=False)
            if (desc["w"], desc["h"], desc["fourcc"]) == (w, h, fmt) and fnv64(raw[128:128 + mip_size(w, h, fmt)]) == expected:
                matches.append(path)
        require(matches, f"no identity-matching original for {name}")
        original = sorted(matches)[0]
        png = enhanced / (name + ".dds.8K.png")
        # Blood/decal fallback is explicitly a resample, not new artistic detail.
        source = png if png.is_file() else original
        require(png.is_file() or name in (*BLOOD, "bsplat3_d", "t_decal_metal_d_v4"), f"enhanced source missing: {png}")
        with Image.open(source) as image:
            require(image.width * h == image.height * w, f"aspect ratio changed: {name}")
        inputs.append((name, original, source, w, h, fmt, expected))
    output.mkdir(parents=True)
    records = []
    for name, original, source, w, h, fmt, expected in inputs:
        size = (max(1, w * 2048 // max(w, h)), max(1, h * 2048 // max(w, h)))
        with Image.open(source) as image:
            image = image.convert("RGBA").resize(size, Image.Resampling.LANCZOS)
        raw = build_dds(image, fmt)
        destination = output / (name + ".dds")
        destination.write_bytes(raw)
        records.append(dict(name=name, original=str(original), source=str(source),
            sourceSha256=hashlib.sha256(source.read_bytes()).hexdigest(),
            originalSha256=hashlib.sha256(original.read_bytes()).hexdigest(), originalMip0Fnv64=expected,
            method="enhanced-source-downsample" if source.suffix.lower() == ".png" else "original-resample-no-new-detail",
            **dds_validate(raw), sha256=hashlib.sha256(raw).hexdigest()))
    manifest = dict(schema="svr-effects-staging-v1", status="OFFLINE_ONLY_NOT_VISUALLY_VERIFIED",
        excludedUnregistered=["smoketrail_01", "tracer2", "bulletImpactFlesh_d"], files=records)
    (output / "manifest.json").write_text(json.dumps(manifest, indent=2), encoding="utf-8")
    return manifest


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--originals", type=Path, required=True)
    parser.add_argument("--enhanced", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    result = prepare(args.originals, args.enhanced, args.output)
    print(f"Staged {len(result['files'])} validated DDS files; no Steam writes")


if __name__ == "__main__":
    main()
