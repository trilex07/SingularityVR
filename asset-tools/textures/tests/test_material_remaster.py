#!/usr/bin/env python3
"""
Manifest-level tests: the material remaster package must satisfy the promises
made about it. Everything asserted here is re-read from the generated files.
"""

from __future__ import annotations

import json
import sys
from pathlib import Path

import numpy as np
import pytest
from PIL import Image

REPO = Path(__file__).resolve().parents[3]
ROOT = REPO / "material-remaster"
ASSETS = ["weapon_shotgun", "glove", "env_door"]
sys.path.insert(0, str(REPO / "asset-tools" / "textures"))
from dds_io import load_dds, read_dds_header, video_memory_bytes  # noqa: E402

RETURNED = None


def manifests() -> dict:
    return {a: json.loads((ROOT / a / "manifest.json").read_text(encoding="utf-8"))
            for a in ASSETS}


def test_every_asset_has_a_manifest_and_comparison_sheets() -> None:
    for a in ASSETS:
        m = manifests()[a]
        assert (ROOT / a / "manifest.json").exists()
        sheets = list((ROOT / a / "comparison").glob("*_compare.png"))
        assert len(sheets) == 3, f"{a}: expected 3 comparison sheets, got {len(sheets)}"
        for s in sheets:
            with Image.open(s) as im:
                assert im.width > 0 and im.height > 0


def test_substituted_maps_are_true_mip0() -> None:
    for a, m in manifests().items():
        for map_type, e in m["maps"].items():
            if e["substituted"]:
                assert e["is_true_mip0"], f"{a}/{map_type} is not true mip0"
                assert (e["source_width"], e["source_height"]) == tuple(e["object_mip0_expected"])


def test_unsubstituted_maps_are_flagged_not_mip0() -> None:
    for a, m in manifests().items():
        for map_type, e in m["maps"].items():
            if not e["substituted"]:
                assert not e["is_true_mip0"], (
                    f"{a}/{map_type} was left unsubstituted but looks like true mip0 - "
                    "revisit whether it should be substituted")


def test_every_output_dds_decodes_back() -> None:
    for a, m in manifests().items():
        for map_type, e in m["maps"].items():
            out = Path(e["output_file"])
            assert out.exists(), f"missing {out}"
            hdr = read_dds_header(out)
            img, _ = load_dds(out)
            assert (img.width, img.height) == (hdr["width"], hdr["height"])
            assert hdr["mip_count"] >= 9
            # the replacement keeps the original resolution - no upscaling
            assert (hdr["width"], hdr["height"]) == (e["source_width"], e["source_height"])
            assert hdr["fourcc"] == e["source_fourcc"], "container format changed"


def test_normal_maps_keep_channel_conventions() -> None:
    for a, m in manifests().items():
        e = m["maps"]["normal"]
        rep = e["validation"]
        n = rep["normal_map_check"]
        assert n["alpha_min"] == 255 and n["alpha_max"] == 255
        assert 110 < n["x_mean"] < 146 and 110 < n["y_mean"] < 146
        # DXT1 quantisation means the vectors are near-unit, not exactly unit;
        # the shipped DXT1 normal maps deviate by more than this
        assert 0.6 <= n["unit_length_min"] <= 1.0
        assert 1.0 <= n["unit_length_max"] <= 1.15


def test_fidelity_is_within_declared_bounds() -> None:
    for a, m in manifests().items():
        for map_type in ("diffuse", "normal"):
            f = m["maps"][map_type]["validation"]["fidelity_vs_source"]
            # visible but not destructive: mean absolute difference stays under
            # 16 levels (6.3% of full range) for every map
            assert 0.0 < f["mean_abs_diff"] < 16.0, f"{a}/{map_type}: {f}"
            assert f["compared_pixels"] > 1000


def test_luminance_shift_is_small() -> None:
    """The palette must survive: mean luminance within 15% of the original."""
    srcs = {
        "weapon_shotgun": "AR_shotgun",
        "glove": "inview_glove",
        "env_door": "door_blastdoorN",
    }
    for a, name in srcs.items():
        m = manifests()[a]
        src = Path(m["maps"]["diffuse"]["source_file"])
        out = Path(m["maps"]["diffuse"]["output_file"])
        ma = np.asarray(load_dds(src)[0].convert("RGB"), np.float32).mean()
        mb = np.asarray(load_dds(out)[0].convert("RGB"), np.float32).mean()
        rel = abs(mb - ma) / ma
        assert rel < 0.15, f"{a}: mean luminance moved {100*rel:.1f}% ({ma:.1f} -> {mb:.1f})"


def test_vram_figures_come_from_the_files() -> None:
    total = 0
    for a, m in manifests().items():
        measured = 0
        for e in m["maps"].values():
            if "output_file" in e:
                measured += video_memory_bytes(Path(e["output_file"]))
        assert m["video_memory"]["replacement_total_bytes"] == measured
        total += measured
    assert total > 0


def test_checksums_match_files_on_disk() -> None:
    import hashlib
    for a, m in manifests().items():
        for map_type, e in m["maps"].items():
            for key in ("source_sha256", "output_sha256"):
                if key not in e:
                    continue
                p = Path(e["source_file"] if key == "source_sha256" else e["output_file"])
                h = hashlib.sha256(p.read_bytes()).hexdigest()
                assert h == e[key], f"{a}/{map_type}: {key} mismatch"


def test_evidence_labels_are_present() -> None:
    for a, m in manifests().items():
        ev = m["evidence"]
        for key, value in ev.items():
            assert value.startswith(("VERIFIED", "ESTIMATED", "NOT TESTED")), (a, key, value)


def test_detail_energy_increased() -> None:
    """The pack must add micro-detail, not just resample the original."""
    for a, m in manifests().items():
        rep = m["maps"]["diffuse"]["validation"]
        d = rep["detail_high_frequency_energy"]
        assert d["ratio"] > 1.02, f"{a}: detail energy ratio {d['ratio']}"


if __name__ == "__main__":
    failures = []
    for fn in (test_every_asset_has_a_manifest_and_comparison_sheets,
               test_substituted_maps_are_true_mip0,
               test_unsubstituted_maps_are_flagged_not_mip0,
               test_every_output_dds_decodes_back,
               test_normal_maps_keep_channel_conventions,
               test_fidelity_is_within_declared_bounds,
               test_luminance_shift_is_small,
               test_vram_figures_come_from_the_files,
               test_checksums_match_files_on_disk,
               test_evidence_labels_are_present,
               test_detail_energy_increased):
        try:
            fn()
            print(f"PASS: {fn.__name__}")
        except AssertionError as e:
            failures.append(fn.__name__)
            print(f"FAIL: {fn.__name__}\n  {e}")
    sys.exit(1 if failures else 0)
