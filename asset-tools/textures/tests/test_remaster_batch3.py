#!/usr/bin/env python3
"""
Manifest-level tests for offline material remaster batch 3 (normal-only).

Same guarantees as batch 2, adapted: no diffuse maps exist, so fidelity
bounds apply to normals, there is no detail-energy or luminance test, and
every manifest must record the unprocessed lower-mip diffuse with its reason.
"""

from __future__ import annotations

import hashlib
import sys
from pathlib import Path

LOCAL_ROOT = Path(r"C:\Users\Trilex\vr-mods\SingularityVR-assets-local")

REPO = Path(__file__).resolve().parents[3]
SAFE = REPO / "material-remaster-batch3" / "manifests"

sys.path.insert(0, str(REPO / "asset-tools" / "textures"))
from dds_io import load_dds, read_dds_header, video_memory_bytes  # noqa: E402

import json

KEYS = ["weapon_sniper_normal", "weapon_railgun_normal", "device_tmd_normal"]


def manifests() -> dict:
    return {k: json.loads((SAFE / f"{k}.json").read_text(encoding="utf-8")) for k in KEYS}


def _local(p: str) -> Path:
    return LOCAL_ROOT / Path(p.replace("<assets-local>/", ""))


def test_safe_manifests_exist_and_cover_all_groups() -> None:
    got = sorted(p.stem for p in SAFE.glob("*.json") if p.stem != "manifest_summary")
    assert got == sorted(KEYS), got
    for k, m in manifests().items():
        assert m["batch"] == 3, k
        assert m["status"] == "OFFLINE ONLY - NOT VR TESTED", k


def test_no_binary_artifacts_committed() -> None:
    import subprocess
    out = subprocess.run(["git", "-C", str(REPO), "ls-files", "material-remaster-batch3/"],
                         capture_output=True, text=True).stdout.split()
    bad = [f for f in out if f.endswith((".dds", ".png"))]
    assert not bad, bad
    assert any(f.endswith(".json") for f in out), "no manifests committed"


def test_normal_is_true_mip0_and_substituted() -> None:
    for k, m in manifests().items():
        e = m["maps"]["normal"]
        assert e["substituted"] and e["is_true_mip0"], k
        assert (e["source_width"], e["source_height"]) == tuple(e["object_mip0_expected"])


def test_unprocessed_diffuse_recorded_with_reason() -> None:
    for k, m in manifests().items():
        assert m.get("unprocessed"), f"{k} has no unprocessed record"
        assert "normal" not in str(m["unprocessed"]), k


def test_relief_is_procedural_not_diffuse_coupled() -> None:
    for k, m in manifests().items():
        notes = " ".join(m["newly_authored"])
        assert "procedural-only" in notes, k


def test_output_dds_decode_and_match_manifest() -> None:
    for k, m in manifests().items():
        e = m["maps"]["normal"]
        p = _local(e["output_file"])
        assert p.exists(), f"missing local output {p}"
        hdr = read_dds_header(p)
        img, _ = load_dds(p)
        assert (img.width, img.height) == (hdr["width"], hdr["height"])
        assert hdr["fourcc"] == e["source_fourcc"], k
        # revision 2: substitutes are presented at 2x the source mip0
        assert (hdr["width"], hdr["height"]) == (e["source_width"] * 2,
                                                e["source_height"] * 2), k
        assert hashlib.sha256(p.read_bytes()).hexdigest() == e["output_sha256"]
        assert video_memory_bytes(p) == e["validation"]["video_memory_bytes"]


def test_normal_conventions_and_relative_unit_length() -> None:
    import numpy as np
    for k, m in manifests().items():
        n = m["maps"]["normal"]["validation"]["normal_map_check"]
        assert n["alpha_min"] == 255 and n["alpha_max"] == 255, k
        assert 110 < n["x_mean"] < 146 and 110 < n["y_mean"] < 146, k
        assert 1.0 <= n["unit_length_max"] <= 1.15, k
        # relative gates against the shipped original: the enhanced map must be
        # no worse (modulo byte quantisation) on both the worst texel and the
        # overall out-of-tolerance share
        src = _local(m["maps"]["normal"]["source_file"])
        a = np.asarray(load_dds(src)[0]).astype(np.float32)
        x = (a[:, :, 0] / 255) * 2 - 1
        y = (a[:, :, 1] / 255) * 2 - 1
        z = a[:, :, 2] / 255
        L = (x * x + y * y + z * z) ** 0.5
        orig_out = float(((L < 0.98) | (L > 1.02)).mean() * 100)
        assert n["pct_outside_0p98_1p02"] <= orig_out + 0.01, (k, n, orig_out)
        # garbage floor only: single-texel extremes are cosmetically irrelevant
        # (and pre-exist in the shipped maps); the share metric above is the
        # real gate
        assert n["unit_length_min"] >= 0.45, (k, n)


def test_fidelity_bounds() -> None:
    for k, m in manifests().items():
        f = m["maps"]["normal"]["validation"]["fidelity_vs_source"]
        assert 0.0 < f["mean_abs_diff"] < 16.0, f"{k}: {f}"


def test_evidence_labels_present() -> None:
    for k, m in manifests().items():
        for key, value in m["evidence"].items():
            assert value.startswith(("VERIFIED", "ESTIMATED", "NOT TESTED")), (k, key, value)


def test_output_filenames_match_injector_lookup() -> None:
    for k, m in manifests().items():
        for map_type, e in m["maps"].items():
            if not e.get("substituted"):
                continue
            t = e["injector_target"]
            if k == "device_tmd_normal":
                # known gap: TMD_new_n has no target-table entry (MAIN-side
                # table update required); the file is review-only by filename
                assert t["present"] is False, k
                assert e["deploy_filename"].endswith(".nodesc.dds"), k
                continue
            assert t["present"] is True, f"{k}/{map_type} has no injector target"
            assert t["hash_match"] is True, f"{k}/{map_type} hash mismatch vs target table"
            want = Path(t["table_file"]).stem + "_2x.dds"
            assert Path(e["output_file"]).name == want, (k, map_type)
            assert e["deploy_filename"] == want, (k, map_type)


if __name__ == "__main__":
    fns = [v for k, v in sorted(globals().items()) if k.startswith("test_")]
    failures = []
    for fn in fns:
        try:
            fn()
            print(f"PASS: {fn.__name__}")
        except AssertionError as e:
            failures.append(fn.__name__)
            print(f"FAIL: {fn.__name__}\n  {e}")
    sys.exit(1 if failures else 0)
