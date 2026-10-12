#!/usr/bin/env python3
"""Tests for dds_io.py - real DDS writing, decoding, hashing and VRAM math."""

from __future__ import annotations

import struct
import sys
import tempfile
from pathlib import Path

import numpy as np
import pytest
from PIL import Image

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
from dds_io import (  # noqa: E402
    fnv1a64_mip0,
    load_dds,
    read_dds_header,
    save_dds,
    save_normal_dds,
    validate_dds,
    video_memory_bytes,
)


def synthetic(w: int, h: int, mode: str = "rgba") -> Image.Image:
    rng = np.random.default_rng(w * 1000 + h)
    if mode == "rgba":
        arr = np.zeros((h, w, 4), np.uint8)
        arr[:, :, 0] = np.linspace(0, 255, w, dtype=np.uint8)[None, :]
        arr[:, :, 1] = np.linspace(0, 255, h, dtype=np.uint8)[:, None]
        arr[:, :, 2] = 128
        arr[:, :, 3] = rng.integers(0, 256, (h, w), dtype=np.uint8)
        return Image.fromarray(arr, "RGBA")
    arr = np.zeros((h, w, 3), np.uint8)
    arr[:, :, 0] = np.linspace(0, 255, w, dtype=np.uint8)[None, :]
    arr[:, :, 1] = np.linspace(0, 255, h, dtype=np.uint8)[:, None]
    arr[:, :, 2] = 200
    return Image.fromarray(arr, "RGB")


def test_dxt5_round_trip(tmp_path: Path) -> None:
    img = synthetic(256, 256, "rgba")
    p = tmp_path / "t.dds"
    hdr = save_dds(p, img, "DXT5")
    assert hdr["width"] == 256 and hdr["height"] == 256
    assert hdr["fourcc"] == "DXT5"
    # 256 -> 1x1 is 9 levels
    assert hdr["mip_count"] == 9

    back, hdr2 = load_dds(p)
    assert (back.width, back.height) == (256, 256)
    assert hdr2["fourcc"] == "DXT5"
    assert hdr2["mip_count"] == 9

    a = np.asarray(img.convert("RGBA"), np.int16)
    b = np.asarray(back.convert("RGBA"), np.int16)
    # DXT colour error must stay small for a smooth gradient
    assert np.abs(a - b).mean() < 12.0


def test_dxt1_round_trip(tmp_path: Path) -> None:
    img = synthetic(128, 128, "rgb")
    p = tmp_path / "t.dds"
    hdr = save_dds(p, img, "DXT1")
    assert hdr["fourcc"] == "DXT1"
    assert hdr["mip_count"] == 8
    back, _ = load_dds(p)
    assert (back.width, back.height) == (128, 128)


def test_header_layout_matches_dds_spec(tmp_path: Path) -> None:
    p = tmp_path / "t.dds"
    save_dds(p, synthetic(64, 64, "rgb"), "DXT1")
    raw = p.read_bytes()
    assert raw[:4] == b"DDS "
    assert struct.unpack_from("<I", raw, 12)[0] == 64     # height
    assert struct.unpack_from("<I", raw, 16)[0] == 64     # width
    assert struct.unpack_from("<I", raw, 28)[0] == 7      # mip count
    assert struct.unpack_from("<I", raw, 76)[0] == 32     # pixelformat size
    assert struct.unpack_from("<I", raw, 80)[0] == 0x4    # FOURCC flag
    assert raw[84:88] == b"DXT1"


def test_video_memory_math(tmp_path: Path) -> None:
    # 4x4 DXT1 with a single level is exactly one 8-byte block
    p = tmp_path / "t.dds"
    save_dds(p, synthetic(4, 4, "rgb"), "DXT1", full_chain=False)
    assert video_memory_bytes(p) == 8
    # 8x8 DXT5, one level: 4 blocks of 16 bytes
    p2 = tmp_path / "t2.dds"
    save_dds(p2, synthetic(8, 8, "rgba"), "DXT5", full_chain=False)
    assert video_memory_bytes(p2) == 64


def test_fnv_matches_reference_implementation(tmp_path: Path) -> None:
    """The injector hashes the game's locked mip0; our helper must agree."""
    p = tmp_path / "t.dds"
    save_dds(p, synthetic(64, 64, "rgb"), "DXT1", full_chain=False)
    raw = p.read_bytes()
    fourcc = raw[84:88].decode("latin1")
    w = struct.unpack_from("<I", raw, 16)[0]
    h = struct.unpack_from("<I", raw, 12)[0]
    blk = 8 if fourcc == "DXT1" else 16
    need = (w // 4) * blk * (h // 4)
    hv = 0xCBF29CE484222325
    for x in raw[128:128 + need]:
        hv ^= x
        hv = (hv * 0x100000001B3) & 0xFFFFFFFFFFFFFFFF
    assert fnv1a64_mip0(p) == hv


def test_validate_reports_decoded_state(tmp_path: Path) -> None:
    p = tmp_path / "t.dds"
    save_dds(p, synthetic(64, 64, "rgb"), "DXT1")
    v = validate_dds(p)
    assert v["width"] == 64 and v["height"] == 64
    assert v["fourcc"] == "DXT1"
    assert v["mip_count"] == 7      # 64,32,16,8,4,2,1
    assert v["decoded_size"] == [64, 64]
    assert v["decoded_channels"] == 4
    assert v["pixel_data_bytes"] == v["declared_mip_bytes"]
    assert 0.0 <= v["seam_row_mean_abs"] <= 255.0


def test_normal_dds_keeps_alpha_and_unit_length(tmp_path: Path) -> None:
    arr = np.full((64, 64, 4), 128, np.uint8)
    arr[:, :, 2] = 255
    arr[:, :, 3] = 255
    p = tmp_path / "n.dds"
    save_normal_dds(p, Image.fromarray(arr, "RGBA"))
    v = validate_dds(p)
    assert v["fourcc"] == "DXT1"
    back, _ = load_dds(p)
    b = np.asarray(back)
    assert b[:, :, 3].min() == 255
    x = (b[:, :, 0] / 255.0) * 2 - 1
    y = (b[:, :, 1] / 255.0) * 2 - 1
    z = b[:, :, 2] / 255.0
    length = np.sqrt(x * x + y * y + z * z)
    assert np.nanmax(np.abs(length - 1.0)) < 0.2


def test_rejects_non_dds(tmp_path: Path) -> None:
    p = tmp_path / "bad.dds"
    p.write_bytes(b"not a dds file at all")
    with pytest.raises(ValueError):
        read_dds_header(p)


def test_rejects_odd_dimensions(tmp_path: Path) -> None:
    p = tmp_path / "odd.dds"
    hdr = bytearray(128)
    hdr[0:4] = b"DDS "
    struct.pack_into("<I", hdr, 12, 66)
    struct.pack_into("<I", hdr, 16, 130)
    struct.pack_into("<I", hdr, 80, 0x4)
    hdr[84:88] = b"DXT1"
    p.write_bytes(bytes(hdr) + b"\0" * 4096)
    with pytest.raises(ValueError):
        read_dds_header(p)


if __name__ == "__main__":
    sys.exit(pytest.main([__file__, "-v"]))
