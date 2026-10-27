#!/usr/bin/env python3
"""Offline gate for the 3D HUD attach pack (SIDE deliverable)."""
from __future__ import annotations

import json
from pathlib import Path

ROOT = Path(__file__).resolve().parents[3]
PACK = ROOT / "assets" / "hud" / "katorga" / "3d"
HANDOFF = ROOT / "docs" / "handoffs" / "side-hud-3d-attach-pack.md"


def test_handoff_exists() -> None:
    assert HANDOFF.is_file() and HANDOFF.stat().st_size > 500


def test_template_schema() -> None:
    path = PACK / "attachments" / "attach_schema.template.json"
    data = json.loads(path.read_text(encoding="utf-8"))
    assert data["schema"] == "singularityvr-hud-attach-v1"
    assert "plate_health" in data["nodes"]
    assert data["nodes"]["weapon_wheel"]["radius_cm"] > 0


def test_holster_slots_json() -> None:
    path = PACK / "holster_slots.json"
    assert path.is_file()
    data = json.loads(path.read_text(encoding="utf-8"))
    assert data["schema"] == "singularityvr-holster-slots-v1"
    ids = {s["id"] for s in data["slots"]}
    assert "weapon_holster_right" in ids and "medkit_pouch_left" in ids


def test_weapon_wheel_catalog() -> None:
    path = PACK / "weapon_wheel_catalog.json"
    assert path.is_file()
    data = json.loads(path.read_text(encoding="utf-8"))
    assert data["schema"] == "singularityvr-weapon-wheel-catalog-v1"
    assert len(data["sectors"]) >= 4
    assert data["sectors"][1]["label_pl"] == "Centurion"


def test_side_deliverables_when_present() -> None:
    """When SIDE finishes, flip strict mode: require all glb + previews + attach_schema.json."""
    required_glb = (
        "gloves/glove_left.glb",
        "gloves/glove_right.glb",
        "weapons/centurion_inview.glb",
        "hud/plate_health.glb",
        "hud/plate_ammo.glb",
        "hud/ring_weapon_wheel.glb",
        "hud/holster_weapon_right.glb",
        "hud/holster_medkit_left.glb",
    )
    schema = PACK / "attachments" / "attach_schema.json"
    if not schema.is_file():
        return
    json.loads(schema.read_text(encoding="utf-8"))
    for rel in required_glb:
        p = PACK / rel
        assert p.is_file() and p.stat().st_size > 100, rel
    for name in ("spokoj.png", "walka.png", "zmiana_broni.png"):
        prev = PACK / "previews" / name
        assert prev.is_file() and prev.stat().st_size > 10_000, name


if __name__ == "__main__":
    test_handoff_exists()
    test_template_schema()
    test_holster_slots_json()
    test_weapon_wheel_catalog()
    test_side_deliverables_when_present()
    print("PASS: hud 3d pack (handoff + template; full pack if attach_schema.json exists)")
