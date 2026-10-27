#!/usr/bin/env python3
"""Validate holster_slots.json and attach_schema hip nodes."""
from __future__ import annotations

import json
from pathlib import Path

ROOT = Path(__file__).resolve().parents[3]
PACK = ROOT / "assets" / "hud" / "katorga" / "3d"
CATALOG = PACK / "holster_slots.json"
SCHEMA = PACK / "attachments" / "attach_schema.json"


def test_holster_catalog() -> None:
    data = json.loads(CATALOG.read_text(encoding="utf-8"))
    assert data["schema"] == "singularityvr-holster-slots-v1"
    slots = {s["id"]: s for s in data["slots"]}
    w = slots["weapon_holster_right"]
    m = slots["medkit_pouch_left"]
    assert w["side"] == "right" and w["hip_offset_uu"]["right"] > 0
    assert m["side"] == "left" and m["hip_offset_uu"]["right"] < 0
    assert w["hip_offset_uu"]["up"] == m["hip_offset_uu"]["up"]


def test_attach_schema_holster_nodes() -> None:
    data = json.loads(SCHEMA.read_text(encoding="utf-8"))
    hw = data["nodes"]["holster_weapon"]
    hm = data["nodes"]["holster_medkit"]
    assert hw["offset_pos_uu"][1] > 0
    assert hm["offset_pos_uu"][1] < 0
    assert hw["slot_id"] == "weapon_holster_right"
    assert hm["atlas_icon"] == "sense-circle"


def test_holster_meshes_when_present() -> None:
    for rel in ("hud/holster_weapon_right.glb", "hud/holster_medkit_left.glb"):
        p = PACK / rel
        if not p.is_file():
            return
        assert p.stat().st_size > 100, rel


if __name__ == "__main__":
    test_holster_catalog()
    test_attach_schema_holster_nodes()
    test_holster_meshes_when_present()
    print("PASS: holster slots")
