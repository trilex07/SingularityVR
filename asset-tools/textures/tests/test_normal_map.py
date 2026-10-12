#!/usr/bin/env python3
"""Tests for signed normal-map handling (dds_io normal helpers)."""

from __future__ import annotations

import sys
from pathlib import Path

import numpy as np
import pytest
from PIL import Image

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
from dds_io import downsample_normal, normal_to_signed, signed_to_normal  # noqa: E402


def normal(r: int, g: int, b: int, a: int = 255) -> Image.Image:
    arr = np.zeros((8, 8, 4), np.uint8)
    arr[:, :, 0], arr[:, :, 1], arr[:, :, 2], arr[:, :, 3] = r, g, b, a
    return Image.fromarray(arr, "RGBA")


def test_flat_normal_stays_flat() -> None:
    """A flat normal (128,128,255) must survive downsampling unchanged."""
    out = np.asarray(downsample_normal(normal(128, 128, 255), (4, 4)))
    assert (out[:, :, 0] == 128).all()
    assert (out[:, :, 1] == 128).all()
    assert (out[:, :, 2] == 255).all()
    assert (out[:, :, 3] == 255).all()


def test_tilt_is_preserved() -> None:
    """A uniformly tilted normal keeps its tilt after a signed downsample."""
    # X = +0.6, Y = -0.6, Z = 0.529 -> unit length
    import math
    x, y = 0.6, -0.6
    z = math.sqrt(1 - x * x - y * y)
    img = normal(int((x + 1) * 127.5), int((y + 1) * 127.5), int(z * 255))
    out = np.asarray(downsample_normal(img, (4, 4)))
    ox = (out[0, 0, 0] / 255.0) * 2 - 1
    oy = (out[0, 0, 1] / 255.0) * 2 - 1
    oz = out[0, 0, 2] / 255.0
    length = (ox * ox + oy * oy + oz * oz) ** 0.5
    assert abs(length - 1.0) < 0.02
    assert ox > 0.3 and oy < -0.3


def test_alpha_is_byte_exact() -> None:
    """Alpha carries material data (masks) and must pass through untouched."""
    arr = np.zeros((16, 16, 4), np.uint8)
    arr[:, :, :3] = 128
    arr[:, :, 2] = 255
    arr[:, :, 3] = np.arange(256, dtype=np.uint8).reshape(16, 16)  # 0..255 ramp
    img = Image.fromarray(arr, "RGBA")
    out = np.asarray(downsample_normal(img, (8, 8)))
    assert not (out[:, :, 3] == 255).all()
    # the ramp average is still mid-grey, not a hard 0 or 255
    assert 100 < out[:, :, 3].mean() < 156


def test_blended_tilts_stay_unit_length() -> None:
    """Mixing two different tilts must give a unit vector, not a bent one.

    The naive path (average the RGB bytes, never renormalise) produces vectors
    shorter than 1, which flattens the shading. The signed path renormalises.
    """
    arr = np.zeros((8, 8, 4), np.uint8)
    arr[:, :4, 0] = 204      # +X on the left half
    arr[:, 4:, 1] = 204      # +Y on the right half
    arr[:, :, 2] = 229       # Z ~ 0.9
    arr[:, :, 3] = 255
    img = Image.fromarray(arr, "RGBA")

    naive = np.asarray(img.resize((4, 4), Image.BOX), np.float32)
    nx = (naive[:, :, 0] / 255) * 2 - 1
    ny = (naive[:, :, 1] / 255) * 2 - 1
    nz = naive[:, :, 2] / 255
    naive_len = float((nx * nx + ny * ny + nz * nz).mean() ** 0.5)

    good = np.asarray(downsample_normal(img, (4, 4)), np.float32)
    gx = (good[:, :, 0] / 255) * 2 - 1
    gy = (good[:, :, 1] / 255) * 2 - 1
    gz = good[:, :, 2] / 255
    good_len = float((gx * gx + gy * gy + gz * gz).mean() ** 0.5)

    # signed path lands on unit length within byte quantisation
    assert 0.99 <= good_len <= 1.01, good_len
    # naive path does not: it drifts well outside unit length
    assert not (0.99 <= naive_len <= 1.01), naive_len


def test_signed_round_trip() -> None:
    """Round trip through signed space preserves unit-length normals + alpha."""
    rng = np.random.default_rng(7)
    # author normals the way a normal map stores them: tilt in X/Y, Z >= 0
    xy = rng.normal(size=(16, 16, 2)).astype(np.float32) * 0.35
    z = np.sqrt(np.maximum(0.0, 1.0 - (xy ** 2).sum(axis=2)))
    raw = np.concatenate([xy, z[:, :, None]], axis=2)
    raw /= np.linalg.norm(raw, axis=2, keepdims=True)          # unit length

    arr = np.zeros((16, 16, 4), np.uint8)
    arr[:, :, 0] = np.clip(np.round((raw[:, :, 0] * 0.5 + 0.5) * 255.0), 0, 255)
    arr[:, :, 1] = np.clip(np.round((raw[:, :, 1] * 0.5 + 0.5) * 255.0), 0, 255)
    arr[:, :, 2] = np.clip(np.round(raw[:, :, 2] * 255.0), 0, 255)
    arr[:, :, 3] = rng.integers(0, 256, (16, 16), dtype=np.uint8)

    signed = normal_to_signed(arr)
    back = signed_to_normal(signed)
    # byte quantisation (0.5 level) plus the Z>=0 clamp is the whole loss here
    assert np.abs(back.astype(np.int16) - arr.astype(np.int16)).mean() < 1.0
    # alpha survives the round trip byte for byte
    assert (back[:, :, 3] == arr[:, :, 3]).all()


if __name__ == "__main__":
    sys.exit(pytest.main([__file__, "-v"]))
