#!/usr/bin/env python3
"""Validate weapon_wheel_catalog.json and cycle step helper."""
from __future__ import annotations

import json
from pathlib import Path

ROOT = Path(__file__).resolve().parents[3]
CATALOG = ROOT / "assets" / "hud" / "katorga" / "3d" / "weapon_wheel_catalog.json"


def shortest_cycle_steps(current: int, target: int, order: list[int]) -> int:
    if not order or current == target:
        return 0
    try:
        i = order.index(current)
        j = order.index(target)
    except ValueError:
        return -1
    n = len(order)
    return (j - i) % n


def test_catalog_schema() -> None:
    data = json.loads(CATALOG.read_text(encoding="utf-8"))
    assert data["schema"] == "singularityvr-weapon-wheel-catalog-v1"
    sectors = data["sectors"]
    assert len(sectors) >= 4
    assert sectors[0]["index"] == 0
    assert sectors[1]["gun_verts"] == 5799
    assert "Centurion" in sectors[1]["label_pl"]
    assert sectors[1]["atlas_icon"] == "weapon-centurion"
    assert sectors[0]["atlas_icon"] == "wheel-current"
    order = data["cycle_order"]
    assert 5799 in order and 11387 in order


def test_shortest_cycle() -> None:
    order = [5799, 11387, 8297, 11389]
    assert shortest_cycle_steps(5799, 5799, order) == 0
    assert shortest_cycle_steps(5799, 11387, order) == 1
    assert shortest_cycle_steps(11389, 5799, order) == 1
    assert shortest_cycle_steps(9999, 5799, order) == -1


if __name__ == "__main__":
    test_catalog_schema()
    test_shortest_cycle()
    print("PASS: weapon wheel catalog")
