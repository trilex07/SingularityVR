#!/usr/bin/env python3
"""Offline tests for the TypekaMix Polish glyph patch.

Synthetic tests never touch game bytes. The package test reads the user's
installed ui_GFx_fonts.xxx and writes only to a temporary directory.
"""
from __future__ import annotations

import struct
import sys
import tempfile
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
sys.path.insert(0, str(Path(__file__).resolve().parents[2] / "title"))
from patch_note_font import (  # noqa: E402
    PINNED_SHA256,
    patch_font_movie,
    patch_package,
    refuse_install_path,
    sha256,
)
from preview_notes import (  # noqa: E402
    FIELD_TWIPS,
    PANGRAM,
    POLISH,
    line_width,
    sample_note,
    typeka_from_package,
    wrap_note,
    write_previews,
)
from swf_shape import decode_shape, encode_shape, shape_bounds  # noqa: E402
from swf_title_patch import split_tags  # noqa: E402
from typeka_polish import (  # noqa: E402
    KEEP_REAL,
    SPECS,
    parse_define_font3,
    patch_typeka_payload,
    rebuild_define_font3,
)
from ue3_layout import inspect  # noqa: E402

EXPECTED_OUTPUT_SHA256 = "60e7287185300caa2a8e64fbe14b741475d4d959944b6b0e51f84547841e8db8"
FONT_PACKAGE = Path(
    r"C:\Program Files (x86)\Steam\steamapps\common\Singularity\RvGame\CookedPC\ui_GFx_fonts.xxx"
)
TOKEN_TEXT = "[use] LT RT A B 0123456789"


def test_shape_roundtrip_synthetic() -> None:
    shape = {
        "nfill": 1,
        "nline": 0,
        "records": [
            ("style", {"flags": 3, "move": (6, 10, -20), "fill0": 1}),
            ("line", (8, 40, 0)),
            ("line", (8, 0, 30)),
            ("curve", (8, -10, 8, -10, -8)),
            ("line", (8, -20, -30)),
            ("end",),
        ],
    }
    encoded = encode_shape(shape)
    assert encode_shape(decode_shape(encoded)) == encoded


def test_refuse_install_output() -> None:
    try:
        refuse_install_path(Path(r"C:\Program Files (x86)\Steam\steamapps\common\Singularity\out.xxx"))
    except ValueError:
        return
    raise SystemExit("install path was accepted")


def test_wrong_hash_rejected() -> None:
    try:
        patch_package(b"not a package")
    except ValueError as exc:
        assert "hash" in str(exc)
        return
    raise SystemExit("bad package was accepted")


def _require_font() -> bytes:
    if not FONT_PACKAGE.is_file():
        raise FileNotFoundError(FONT_PACKAGE)
    data = FONT_PACKAGE.read_bytes()
    assert sha256(data) == PINNED_SHA256
    return data


def _typeka(movie: bytes) -> bytes:
    _, tags = split_tags(movie)
    hits = [
        movie[tag["pay_off"] : tag["pay_off"] + tag["pay_len"]]
        for tag in tags
        if tag["code"] == 75 and b"TypekaMix" in movie[tag["pay_off"] : tag["pay_off"] + 40]
    ]
    assert len(hits) == 1
    return hits[0]


def _movie(package: bytes) -> bytes:
    info = inspect("pkg", package, include_all=True)
    export = info["exports"][0]
    raw = next(prop for prop in export["properties"] if prop["name"] == "RawData")
    value = raw["tag_offset"] + 24
    size = raw["value"]["bytes"]
    return package[value + 4 : value + size]


def test_installed_package_when_present() -> None:
    try:
        original = _require_font()
    except FileNotFoundError:
        print("SKIP: installed ui_GFx_fonts.xxx is not on this machine")
        return
    before = parse_define_font3(_typeka(_movie(original)))
    assert rebuild_define_font3(before, before["shapes"], before["advances"]) == _typeka(_movie(original))
    stubs = {}
    for char in SPECS:
        blob = before["shapes"][before["codes"].index(ord(char))]
        stubs.setdefault(blob, []).append(char)
        assert len(blob) <= 80
    assert len(stubs) <= 3
    for char in KEEP_REAL:
        assert len(before["shapes"][before["codes"].index(ord(char))]) > 80

    patched, report = patch_package(original)
    assert report["status"] == "OFFLINE_PASS_PHYSICAL_PENDING"
    assert sha256(patched) == report["output_sha256"] == EXPECTED_OUTPUT_SHA256
    assert sha256(patched) != PINNED_SHA256
    try:
        patch_package(patched)
    except ValueError:
        pass
    else:
        raise SystemExit("patched package was accepted as an original")

    old_movie = _movie(original)
    new_movie = _movie(patched)
    _, old_tags = split_tags(old_movie)
    _, new_tags = split_tags(new_movie)
    assert len(old_tags) == len(new_tags)
    changed = []
    for old, new in zip(old_tags, new_tags):
        old_payload = old_movie[old["pay_off"] : old["pay_off"] + old["pay_len"]]
        new_payload = new_movie[new["pay_off"] : new["pay_off"] + new["pay_len"]]
        assert old["code"] == new["code"]
        if old_payload != new_payload:
            changed.append(old["code"])
    assert changed == [75]

    old_font = before
    new_font = parse_define_font3(_typeka(new_movie))
    assert old_font["codes"] == new_font["codes"]
    assert old_font["tail"] == new_font["tail"]
    assert old_font["metrics"] == new_font["metrics"]
    assert old_font["code_bytes"] == new_font["code_bytes"]
    replaced = set(SPECS)
    for index, code in enumerate(old_font["codes"]):
        char = chr(code)
        if char in replaced:
            assert new_font["shapes"][index] != old_font["shapes"][index]
            assert len(new_font["shapes"][index]) > 80
            assert new_font["advances"][index] == old_font["advances"][old_font["codes"].index(ord(SPECS[char][0]))]
            bounds = shape_bounds(decode_shape(new_font["shapes"][index]))
            assert bounds[1] >= -18600 and bounds[3] <= 5400
            assert bounds[2] - bounds[0] > 1000 and bounds[3] - bounds[1] > 1000
            assert encode_shape(decode_shape(new_font["shapes"][index])) == new_font["shapes"][index]
        else:
            assert new_font["shapes"][index] == old_font["shapes"][index]
            assert new_font["advances"][index] == old_font["advances"][index]

    for char in POLISH:
        assert ord(char) in new_font["codes"]
        assert len(new_font["shapes"][new_font["codes"].index(ord(char))]) > 80
    for char in TOKEN_TEXT:
        assert new_font["shapes"][new_font["codes"].index(ord(char))] == old_font["shapes"][old_font["codes"].index(ord(char))]

    note = sample_note() + "\n" + TOKEN_TEXT
    lines = wrap_note(note, new_font)
    assert any(not row for row in lines)
    assert any("[" in "".join(row) for row in lines)
    for row in lines:
        if row:
            assert line_width(row, new_font) <= FIELD_TWIPS + 0.01
    for char in PANGRAM:
        if char != " ":
            assert len(new_font["shapes"][new_font["codes"].index(ord(char))]) > 80

    assert struct.unpack_from("<I", new_movie, 4)[0] == len(new_movie)
    assert struct.unpack_from("<I", old_movie, 4)[0] == len(old_movie)

    with tempfile.TemporaryDirectory() as tmp:
        root = Path(tmp)
        previews = write_previews(patched, root / "notes.png")
        for preview in previews:
            blob = Path(preview).read_bytes()
            assert blob.startswith(b"\x89PNG") and len(blob) > 1000


if __name__ == "__main__":
    fns = [value for key, value in sorted(globals().items()) if key.startswith("test_")]
    fails = 0
    for fn in fns:
        try:
            fn()
            print(f"PASS: {fn.__name__}")
        except (AssertionError, SystemExit, ValueError, FileNotFoundError) as exc:
            fails += 1
            print(f"FAIL: {fn.__name__}: {exc}")
    sys.exit(1 if fails else 0)
