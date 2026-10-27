#!/usr/bin/env python3
from __future__ import annotations

import json
from pathlib import Path

ROOT = Path(__file__).resolve().parents[3]
ICONS = ROOT / "assets" / "hud" / "katorga" / "wheel" / "icons"
TEX = ROOT / "assets" / "hud" / "katorga" / "3d" / "textures" / "wheel_icons"


def test_weapon_icon_exports() -> None:
    manifest = json.loads((ICONS / "weapon_icons.json").read_text(encoding="utf-8"))
    assert manifest["schema"] == "singularityvr-weapon-wheel-icons-v1"
    for stem in ("centurion", "valkyrie", "spikeshot", "kasimov", "obecna"):
        p = ICONS / f"{stem}.png"
        assert p.is_file() and p.stat().st_size > 80, stem
    for aid in (
        "weapon-centurion",
        "weapon-valkyrie",
        "weapon-spikeshot",
        "weapon-kasimov",
        "wheel-current",
    ):
        p = TEX / f"{aid}.png"
        assert p.is_file(), aid


if __name__ == "__main__":
    test_weapon_icon_exports()
    print("PASS: weapon wheel icons")
