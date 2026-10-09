#!/usr/bin/env python3
"""Verify a SingularityVR per-eye capture bundle.

The oracle deliberately proves capture identity and image evidence, not the
semantic identity of a hand, shadow, prompt, or VFX. Semantic claims require a
trusted scene baseline and a declared ROI; otherwise the result is
INCONCLUSIVE.
"""
from __future__ import annotations

import argparse
import hashlib
import json
import math
import struct
import sys
import zlib
from pathlib import Path
from typing import Iterable


class OracleError(Exception):
    pass


def fail(message: str) -> None:
    raise OracleError(message)


def sha256(path: Path) -> str:
    h = hashlib.sha256()
    with path.open("rb") as f:
        for block in iter(lambda: f.read(1024 * 1024), b""):
            h.update(block)
    return h.hexdigest().upper()


def paeth(a: int, b: int, c: int) -> int:
    p = a + b - c
    pa, pb, pc = abs(p - a), abs(p - b), abs(p - c)
    return a if pa <= pb and pa <= pc else b if pb <= pc else c


def png_rgb(path: Path) -> tuple[int, int, list[tuple[int, int, int]]]:
    data = path.read_bytes()
    if data[:8] != b"\x89PNG\r\n\x1a\n":
        fail(f"unsupported PNG signature: {path}")
    pos, width, height, bit_depth, color_type, interlace = 8, 0, 0, 0, 0, 0
    raw = bytearray()
    while pos < len(data):
        if pos + 12 > len(data):
            fail(f"truncated PNG: {path}")
        n = struct.unpack(">I", data[pos:pos + 4])[0]
        kind = data[pos + 4:pos + 8]
        chunk = data[pos + 8:pos + 8 + n]
        pos += 12 + n
        if kind == b"IHDR":
            width, height, bit_depth, color_type, _, _, interlace = struct.unpack(
                ">IIBBBBB", chunk
            )
            if bit_depth != 8 or color_type not in (2, 6) or interlace:
                fail("oracle supports only non-interlaced 8-bit RGB/RGBA PNG")
        elif kind == b"IDAT":
            raw.extend(chunk)
        elif kind == b"IEND":
            break
    if not width or not height or not raw:
        fail(f"PNG has no image data: {path}")
    channels = 4 if color_type == 6 else 3
    stride = width * channels
    decoded = zlib.decompress(bytes(raw))
    rows: list[bytes] = []
    offset = 0
    previous = bytearray(stride)
    for _ in range(height):
        if offset >= len(decoded):
            fail(f"truncated PNG scanlines: {path}")
        filt = decoded[offset]
        offset += 1
        current = bytearray(decoded[offset:offset + stride])
        offset += stride
        if len(current) != stride:
            fail(f"truncated PNG row: {path}")
        for i in range(stride):
            left = current[i - channels] if i >= channels else 0
            up = previous[i]
            upper_left = previous[i - channels] if i >= channels else 0
            if filt == 1:
                current[i] = (current[i] + left) & 255
            elif filt == 2:
                current[i] = (current[i] + up) & 255
            elif filt == 3:
                current[i] = (current[i] + ((left + up) // 2)) & 255
            elif filt == 4:
                current[i] = (current[i] + paeth(left, up, upper_left)) & 255
            elif filt != 0:
                fail(f"unsupported PNG filter {filt}: {path}")
        rows.append(bytes(current))
        previous = current
    pixels = []
    for row in rows:
        for i in range(0, stride, channels):
            pixels.append((row[i], row[i + 1], row[i + 2]))
    return width, height, pixels


def bmp_rgb(path: Path) -> tuple[int, int, list[tuple[int, int, int]]]:
    data = path.read_bytes()
    if data[:2] != b"BM" or len(data) < 54:
        fail(f"unsupported BMP: {path}")
    pixel_offset = struct.unpack_from("<I", data, 10)[0]
    dib, width, raw_height, planes, bpp, compression = struct.unpack_from("<IiiHHI", data, 14)
    if dib < 40 or planes != 1 or bpp not in (24, 32) or compression != 0:
        fail("oracle supports only uncompressed 24/32-bit BMP")
    height, top_down = abs(raw_height), raw_height < 0
    stride = ((width * bpp + 31) // 32) * 4
    pixels: list[tuple[int, int, int]] = []
    for y in range(height):
        source_y = y if top_down else height - 1 - y
        row = pixel_offset + source_y * stride
        for x in range(width):
            i = row + x * (bpp // 8)
            b, g, r = data[i:i + 3]
            pixels.append((r, g, b))
    return width, height, pixels


def read_image(path: Path) -> tuple[int, int, list[tuple[int, int, int]]]:
    suffix = path.suffix.lower()
    if suffix == ".png":
        return png_rgb(path)
    if suffix == ".bmp":
        return bmp_rgb(path)
    fail(f"unsupported image type: {path.suffix}; use PNG or BMP")


def luma(pixel: tuple[int, int, int]) -> float:
    return (0.2126 * pixel[0] + 0.7152 * pixel[1] + 0.0722 * pixel[2]) / 255.0


def crop(width: int, height: int, pixels: list[tuple[int, int, int]], roi: tuple[int, int, int, int] | None):
    if roi is None:
        return width, height, pixels
    x, y, w, h = roi
    if w <= 0 or h <= 0 or x < 0 or y < 0 or x + w > width or y + h > height:
        fail(f"ROI {roi} is outside {width}x{height}")
    return w, h, [pixels[(y + yy) * width + x + xx] for yy in range(h) for xx in range(w)]


def image_metrics(a: tuple[int, int, list[tuple[int, int, int]]], roi=None) -> dict:
    width, height, pixels = crop(*a, roi)
    values = [luma(p) for p in pixels]
    nonblack = sum(v > 0.01 for v in values) / len(values) if values else 0.0
    return {"width": width, "height": height, "meanLuma": sum(values) / len(values) if values else 0.0,
            "nonBlackPct": nonblack * 100.0, "pixels": len(values)}


def compare(a, b, roi=None) -> dict:
    wa, ha, pa = crop(*a, roi)
    wb, hb, pb = crop(*b, roi)
    if (wa, ha) != (wb, hb):
        fail(f"image dimensions differ: {(wa, ha)} vs {(wb, hb)}")
    xa, xb = [luma(p) for p in pa], [luma(p) for p in pb]
    n = len(xa)
    mad = sum(abs(x - y) for x, y in zip(xa, xb)) / n if n else 0.0
    mean_a, mean_b = sum(xa) / n if n else 0.0, sum(xb) / n if n else 0.0
    var_a = sum((x - mean_a) ** 2 for x in xa) / n if n else 0.0
    var_b = sum((x - mean_b) ** 2 for x in xb) / n if n else 0.0
    cov = sum((x - mean_a) * (y - mean_b) for x, y in zip(xa, xb)) / n if n else 0.0
    c1, c2 = 0.01 ** 2, 0.03 ** 2
    ssim = ((2 * mean_a * mean_b + c1) * (2 * cov + c2) /
            ((mean_a * mean_a + mean_b * mean_b + c1) * (var_a + var_b + c2))) if n else 0.0
    return {"mad": mad, "ssim": ssim, "pixels": n}


def parse_roi(values: Iterable[str]) -> dict[str, tuple[int, int, int, int]]:
    result = {}
    for item in values:
        try:
            name, numbers = item.split("=", 1)
            result[name] = tuple(int(v) for v in numbers.split(","))
            if len(result[name]) != 4:
                raise ValueError
        except ValueError as exc:
            raise OracleError(f"ROI must be name=x,y,w,h: {item}") from exc
    return result


def load_bundle(path: Path) -> dict:
    try:
        bundle = json.loads(path.read_text(encoding="utf-8"))
    except Exception as exc:
        fail(f"cannot read manifest: {exc}")
    required = ("schema", "runId", "scenario", "cmdSeq", "frameIndex", "captureSeq", "pid", "dllSha256", "source", "eyes")
    missing = [key for key in required if key not in bundle]
    if missing:
        fail("manifest missing: " + ", ".join(missing))
    if bundle["schema"] != "singularityvr.capture.v1":
        fail(f"unsupported capture schema: {bundle['schema']}")
    if bundle["source"] not in ("xr_projection_layer", "xr_compositor", "desktop", "other"):
        fail("unknown capture source")
    if not isinstance(bundle["pid"], int) or bundle["pid"] <= 0:
        fail("pid must be a positive integer")
    if not isinstance(bundle["dllSha256"], str) or len(bundle["dllSha256"]) != 64:
        fail("dllSha256 must be a SHA-256 hex string")
    eyes = bundle["eyes"]
    if not isinstance(eyes, list) or {e.get("eye") for e in eyes} != {"L", "R"} or len(eyes) != 2:
        fail("manifest must contain exactly one L and one R eye")
    for eye in eyes:
        for key in ("eye", "path", "sha256", "cmdSeq", "frameIndex", "captureSeq", "source"):
            if key not in eye:
                fail(f"eye entry missing {key}")
        if eye["source"] != bundle["source"]:
            fail("eye source differs from bundle source")
        for key in ("cmdSeq", "frameIndex", "captureSeq"):
            if eye[key] != bundle[key]:
                fail(f"eye {eye['eye']} {key} differs from bundle")
    return bundle


def run(manifest_path: Path, baseline_path: Path | None, previous_path: Path | None,
        rois: dict, effect_rois: list[str], expected_pid: int | None,
        expected_dll: str | None) -> dict:
    bundle = load_bundle(manifest_path)
    result = {"schema": "singularityvr.oracle.v1", "verdict": "PASS", "manifest": str(manifest_path),
              "identity": {k: bundle[k] for k in ("runId", "scenario", "cmdSeq", "frameIndex", "captureSeq", "pid", "dllSha256", "source")},
              "eyes": {}, "assertions": [], "evidence": []}
    def assertion(name, status, evidence):
        result["assertions"].append({"name": name, "status": status, "evidence": evidence})
        if status == "FAIL": result["verdict"] = "FAIL"
        elif status == "INCONCLUSIVE" and result["verdict"] == "PASS": result["verdict"] = "INCONCLUSIVE"

    if expected_pid is not None and bundle["pid"] != expected_pid:
        assertion("pid", "FAIL", f"expected {expected_pid}, got {bundle['pid']}")
    if expected_dll and bundle["dllSha256"].upper() != expected_dll.upper():
        assertion("dllSha256", "FAIL", f"expected {expected_dll.upper()}, got {bundle['dllSha256'].upper()}")
    if bundle["scenario"] == "":
        assertion("scenario", "FAIL", "scenario is empty")
    paths = {}
    for eye in bundle["eyes"]:
        path = (manifest_path.parent / eye["path"]).resolve()
        if not path.is_file():
            assertion(f"eye-{eye['eye']}-exists", "FAIL", str(path))
            continue
        actual_hash = sha256(path)
        if actual_hash != eye["sha256"].upper():
            assertion(f"eye-{eye['eye']}-hash", "FAIL", f"manifest={eye['sha256']} actual={actual_hash}")
        try:
            image = read_image(path)
            paths[eye["eye"]] = (path, image)
            result["eyes"][eye["eye"]] = image_metrics(image)
            if result["eyes"][eye["eye"]]["nonBlackPct"] <= 0.5:
                assertion(f"eye-{eye['eye']}-nonempty", "FAIL", result["eyes"][eye["eye"]])
        except OracleError as exc:
            assertion(f"eye-{eye['eye']}-decode", "FAIL", str(exc))
    if len(paths) == 2:
        left, right = paths["L"][1], paths["R"][1]
        try:
            lr = compare(left, right)
            result["eyes"]["L_vs_R"] = lr
            if bundle.get("stereoOrderKnown", False) and lr["mad"] < 0.0001:
                assertion("stereo-distinct", "FAIL", "L and R are pixel-identical while order is declared known")
        except OracleError as exc:
            assertion("stereo-dimensions", "FAIL", str(exc))
    if previous_path:
        previous = load_bundle(previous_path)
        same_identity = all(previous[k] == bundle[k] for k in ("runId", "scenario", "cmdSeq", "frameIndex", "captureSeq"))
        same_hashes = sorted(e["sha256"].upper() for e in previous["eyes"]) == sorted(e["sha256"].upper() for e in bundle["eyes"])
        if same_identity or same_hashes:
            assertion("fresh-capture", "FAIL", {"sameIdentity": same_identity, "sameImageHashes": same_hashes})
        else:
            assertion("fresh-capture", "PASS", "identity and image hashes differ from previous capture")
    baseline = None
    if baseline_path:
        try:
            baseline = load_bundle(baseline_path)
        except OracleError as exc:
            assertion("baseline-valid", "FAIL", str(exc))
    if effect_rois:
        if not baseline:
            assertion("effect-baseline", "INCONCLUSIVE", "no trusted baseline supplied")
        elif len(paths) < 2:
            assertion("effect-images", "FAIL", "current L/R image unavailable")
        else:
            base_paths = {e["eye"]: (baseline_path.parent / e["path"]).resolve() for e in baseline["eyes"]}
            for name in effect_rois:
                if name not in rois:
                    assertion(f"effect-{name}", "FAIL", "ROI was not declared")
                    continue
                changed = []
                for eye in ("L", "R"):
                    try:
                        current = read_image(paths[eye][0])
                        base = read_image(base_paths[eye])
                        metrics = compare(current, base, rois[name])
                        changed.append(metrics["mad"] > 0.01 and metrics["ssim"] < 0.99)
                    except (OracleError, OSError) as exc:
                        assertion(f"effect-{name}-{eye}", "FAIL", str(exc))
                if changed and all(changed):
                    assertion(f"effect-{name}", "PASS", "both eyes differ from trusted same-scene baseline")
                elif changed:
                    assertion(f"effect-{name}", "FAIL", "effect change was not present in both eyes")
    if baseline and len(paths) == 2:
        base_paths = {e["eye"]: (baseline_path.parent / e["path"]).resolve() for e in baseline["eyes"]}
        paired = {eye: compare(paths[eye][1], read_image(base_paths[eye])) for eye in ("L", "R")}
        swapped = {eye: compare(paths[eye][1], read_image(base_paths["R" if eye == "L" else "L"])) for eye in ("L", "R")}
        result["baseline"] = {"paired": paired, "swapped": swapped}
        if bundle.get("stereoOrderKnown", False) and all(swapped[e]["ssim"] > paired[e]["ssim"] + 0.05 for e in ("L", "R")):
            assertion("eye-order", "FAIL", "current L/R match opposite baseline eyes better than paired eyes")
        else:
            assertion("eye-order", "PASS", "paired baseline is not contradicted by opposite-eye match")
    elif effect_rois:
        assertion("baseline-available", "INCONCLUSIVE", "effect verdict needs trusted same-scene baseline")
    return result


def main(argv=None) -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("manifest", type=Path)
    ap.add_argument("--baseline", type=Path)
    ap.add_argument("--previous", type=Path)
    ap.add_argument("--roi", action="append", default=[], help="name=x,y,w,h")
    ap.add_argument("--effect-roi", action="append", default=[])
    ap.add_argument("--expected-pid", type=int)
    ap.add_argument("--expected-dll")
    ap.add_argument("--report", type=Path)
    args = ap.parse_args(argv)
    try:
        report = run(args.manifest, args.baseline, args.previous, parse_roi(args.roi), args.effect_roi, args.expected_pid, args.expected_dll)
    except (OracleError, OSError, ValueError) as exc:
        report = {"schema": "singularityvr.oracle.v1", "verdict": "FAIL", "error": str(exc), "manifest": str(args.manifest)}
    text = json.dumps(report, indent=2, sort_keys=True)
    if args.report:
        args.report.parent.mkdir(parents=True, exist_ok=True)
        args.report.write_text(text + "\n", encoding="utf-8")
    print(text)
    print(f"{report['verdict']}: {report.get('error', 'capture oracle complete')}")
    return 0 if report["verdict"] == "PASS" else 2 if report["verdict"] == "FAIL" else 3


if __name__ == "__main__":
    sys.exit(main())
