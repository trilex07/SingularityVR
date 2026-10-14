"""Patch the user's own ui_GFx_fonts.xxx so TypekaMix can draw Polish notes.

The pinned package is an uncompressed UE3 v584 file with one fonts_en export.
The tool refuses any other hash. It never writes into a Steam or Program Files
path. Output stays an uncompressed package: the TypekaMix DefineFont3 grows,
and the RawData count, export size and file slack are updated to match.
"""
from __future__ import annotations

import argparse
import hashlib
import json
import struct
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "title"))
sys.path.insert(0, str(Path(__file__).resolve().parent))
from swf_title_patch import encode_tag_header, split_tags  # noqa: E402
from typeka_polish import patch_typeka_payload  # noqa: E402
from ue3_layout import inspect  # noqa: E402

PINNED_SHA256 = "3f7bf0bef08d73aed3b1ba249730b4bbb16ec427dd49992a6f4c4a7ce6c8b51b"
PAGE = 4096


def sha256(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


def refuse_install_path(path: Path) -> None:
    text = str(path).replace("/", "\\").lower()
    if "steamapps" in text or "program files" in text:
        raise ValueError(f"refusing to write into an installed game path: {path}")


def _typeka_tag(movie: bytes) -> dict:
    _, tags = split_tags(movie)
    hits = []
    for tag in tags:
        payload = movie[tag["pay_off"] : tag["pay_off"] + tag["pay_len"]]
        if tag["code"] == 75 and b"TypekaMix" in payload[:40]:
            hits.append((tag, payload))
    if len(hits) != 1:
        raise ValueError(f"expected one TypekaMix font, found {len(hits)}")
    return {"tag": hits[0][0], "payload": hits[0][1], "tags": tags}


def patch_font_movie(movie: bytes) -> tuple[bytes, dict]:
    found = _typeka_tag(movie)
    patched_payload, report = patch_typeka_payload(found["payload"])
    tag = found["tag"]
    replacement = encode_tag_header(75, len(patched_payload)) + patched_payload
    out = bytearray(movie[: tag["tag_off"]] + replacement + movie[tag["pay_off"] + tag["pay_len"] :])
    struct.pack_into("<I", out, 4, len(out))
    split_tags(bytes(out))
    report["tag_off"] = tag["tag_off"]
    report["old_tag_bytes"] = tag["hlen"] + tag["pay_len"]
    report["new_tag_bytes"] = len(replacement)
    return bytes(out), report


def patch_package(package: bytes) -> tuple[bytes, dict]:
    digest = sha256(package)
    if digest != PINNED_SHA256:
        raise ValueError(
            "ui_GFx_fonts.xxx hash does not match the supported Singularity package "
            f"(got {digest})"
        )
    try:
        info = inspect("ui_GFx_fonts.xxx", package, include_all=True)
    except (AssertionError, IndexError, UnicodeDecodeError, struct.error) as exc:
        raise ValueError(f"unsupported font package layout: {exc}") from exc
    if info["counts"]["exports"] != 1 or len(info["exports"]) != 1:
        raise ValueError("supported package has exactly one export")
    export = info["exports"][0]
    if export.get("parse_error") or export["name"] != "fonts_en" or export["class_name"] != "GFxMovieInfo":
        raise ValueError("fonts_en GFxMovieInfo was not found")
    raw = next((prop for prop in export["properties"] if prop["name"] == "RawData"), None)
    if raw is None or raw["type"] != "ArrayProperty":
        raise ValueError("fonts_en RawData is missing")
    value = raw["tag_offset"] + 24
    size = raw["value"]["bytes"]
    count = struct.unpack_from("<I", package, value)[0]
    if count != size - 4 or value + size != export["offset"] + export["size"] - 8:
        raise ValueError("RawData does not occupy the fonts_en export in the expected way")
    movie = package[value + 4 : value + size]
    patched_movie, report = patch_font_movie(movie)
    delta = len(patched_movie) - len(movie)
    body = bytearray(package[export["offset"] : export["offset"] + export["size"]])
    local_value = value - export["offset"]
    local_size = raw["tag_offset"] + 16 - export["offset"]
    new_body = body[: local_value + 4] + patched_movie + body[local_value + size :]
    struct.pack_into("<I", new_body, local_value, len(patched_movie))
    struct.pack_into("<i", new_body, local_size, size + delta)
    out = bytearray(package[: export["offset"]] + new_body + package[export["offset"] + export["size"] :])
    struct.pack_into("<I", out, export["record_offset"] + 32, len(new_body))
    slack = len(out) % PAGE
    if slack:
        out += b"\x00" * (PAGE - slack)
    try:
        verify = inspect("patched", bytes(out), include_all=True)
    except (AssertionError, IndexError, UnicodeDecodeError, struct.error) as exc:
        raise ValueError(f"patched package does not parse: {exc}") from exc
    verified = verify["exports"][0]
    if verified.get("parse_error") or verified["size"] != len(new_body):
        raise ValueError("patched export size does not parse back")
    movie_off = verified["offset"] + local_value + 4
    split_tags(bytes(out[movie_off : movie_off + len(patched_movie)]))
    report.update(
        input_sha256=digest,
        output_sha256=sha256(bytes(out)),
        movie_delta=delta,
        export_size=len(new_body),
        package_bytes=len(out),
        compression="uncompressed-v584",
        status="OFFLINE_PASS_PHYSICAL_PENDING",
    )
    return bytes(out), report


def main() -> None:
    parser = argparse.ArgumentParser(description="Add Polish TypekaMix glyphs to a user's font package")
    parser.add_argument("--input", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--report", type=Path, required=True)
    parser.add_argument("--preview", type=Path)
    args = parser.parse_args()
    refuse_install_path(args.output)
    if args.preview:
        refuse_install_path(args.preview)
    if args.output.exists() or args.report.exists():
        raise SystemExit("refusing to overwrite an existing output or report")
    patched, report = patch_package(args.input.read_bytes())
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_bytes(patched)
    if args.preview:
        from preview_notes import write_previews

        report["previews"] = write_previews(patched, args.preview)
    args.report.parent.mkdir(parents=True, exist_ok=True)
    args.report.write_text(json.dumps(report, indent=2, ensure_ascii=False), encoding="utf-8")
    print(json.dumps({"output_sha256": report["output_sha256"], "status": report["status"]}, indent=2))


if __name__ == "__main__":
    try:
        main()
    except ValueError as exc:
        raise SystemExit(str(exc)) from exc
