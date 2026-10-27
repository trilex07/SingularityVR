#!/usr/bin/env python3
"""Offline checks for the Katorga HUD art pack. No game files and no renderer."""
from __future__ import annotations

import json
import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[3]
KIT = ROOT / "assets" / "hud" / "katorga"
sys.path.insert(0, str(ROOT / "tools" / "hud"))

import katorga_kit  # noqa: E402


def _text_nodes(svg: str) -> list[str]:
    return re.findall(r"<text\b[^>]*>(.*?)</text>", svg, flags=re.DOTALL)


def test_pack_builds_and_fields_stay_empty() -> None:
    katorga_kit.write_all()
    for name in ("health.svg", "ammo.svg", "weapon-name.svg"):
        svg = (KIT / "plates" / name).read_text(encoding="utf-8")
        assert 'class="data"' in svg
        for node in _text_nodes(svg):
            assert not re.search(r"\d", node), (name, node)
            assert "Centurion" not in node
    for name in ("normal.svg", "low-health.svg", "empty-mag.svg", "weapon-switch.svg", "sheet.svg"):
        svg = (KIT / "states" / name).read_text(encoding="utf-8")
        for node in _text_nodes(svg):
            assert not re.search(r"\d", node), (name, node)
            assert "Centurion" not in node
    low = (KIT / "states" / "low-health.svg").read_text(encoding="utf-8")
    empty = (KIT / "states" / "empty-mag.svg").read_text(encoding="utf-8")
    assert "MA\u0141O" in low and 'data-fill="worn"' in low and 'data-fill="empty"' in low
    assert "PRZE\u0141ADUJ" in empty and 'id="chamber-open" display="inline"' in empty
    assert 'id="chamber-closed" display="none"' in empty


def test_atlas_rects_and_alpha() -> None:
    from PIL import Image

    katorga_kit.write_all()
    manifest = json.loads((KIT / "atlas" / "atlas.json").read_text(encoding="utf-8"))
    assert manifest["width"] == 512 and manifest["height"] == 256
    rects = manifest["rects"]
    assert len(rects) == 22
    weapon_ids = {r["id"] for r in rects if r["y"] == 128}
    assert "weapon-centurion" in weapon_ids and "wheel-current" in weapon_ids
    for rect in rects:
        assert 0 <= rect["x"] and rect["x"] + rect["w"] <= 512
        assert 0 <= rect["y"] and rect["y"] + rect["h"] <= 256
        assert (KIT / "icons" / f"{rect['id']}.svg").is_file()
    for i, a in enumerate(rects):
        for b in rects[i + 1 :]:
            separated = (
                a["x"] + a["w"] <= b["x"]
                or b["x"] + b["w"] <= a["x"]
                or a["y"] + a["h"] <= b["y"]
                or b["y"] + b["h"] <= a["y"]
            )
            assert separated, (a["id"], b["id"])
    image = Image.open(KIT / "atlas" / "katorga-hud-atlas.png")
    assert image.mode == "RGBA" and image.size == (512, 256)
    assert image.getpixel((1, 1))[3] == 0
    tokens = {row["token"]: row for row in manifest["tokens"]}
    assert tokens["health"]["icon"] == "sense-circle"
    assert tokens["use"]["icon"] == "sense-grip"
    assert tokens["reload"]["icon"] == "sense-grip"
    assert tokens["age"]["icon"] == "sense-square"
    assert tokens["cancel"]["icon"] == "sense-circle"
    assert tokens["cancel"]["context"] == "menu"
    blob = json.dumps(tokens)
    assert "squeeze/value" in blob and "kCircleHealthMask" in tokens["health"]["emits"]
    for row in tokens.values():
        assert "Dpad" not in row["icon"] and "Pad" not in row["icon"] and row["icon"] != "R1"
    assert "Pad 11" in tokens["health"]["not"]


def test_previews_prompts_and_frame() -> None:
    from PIL import Image

    for name in (
        "states-dark.png",
        "states-light.png",
        "states-gray.png",
        "small-dark.png",
        "small-light.png",
        "hands-floor.png",
        "hands-floor-light.png",
        "prompts.png",
        "hands-floor.svg",
        "prompts.svg",
    ):
        assert (KIT / "previews" / name).is_file(), name
    prompts = (KIT / "previews" / "prompts.svg").read_text(encoding="utf-8")
    assert "apteczki" in prompts and "prze\u0142adowa\u0107" in prompts
    assert "Pad" not in prompts
    gray = Image.open(KIT / "previews" / "states-gray.png")
    assert gray.mode == "RGBA"
    # Grayscale proof: the amber channel is gone, so R equals G.
    pixel = gray.getpixel((40, 80))
    assert pixel[0] == pixel[1] == pixel[2]
    obj = (KIT / "frame" / "plate-frame.obj").read_text(encoding="utf-8")
    assert "katorga_plate_frame" in obj
    assert sum(1 for line in obj.splitlines() if line.startswith("v ")) >= 8
    assert not any(line.startswith("b ") or "skeleton" in line.lower() for line in obj.splitlines() if not line.startswith("#"))
    doc = (ROOT / "docs" / "hud" / "KATORGA_ASSETS.md").read_text(encoding="utf-8")
    assert "nie jest zrobione" in doc and "sense-grip" in doc and "Pad N" in doc
    assert "steamapps" not in doc.lower()


if __name__ == "__main__":
    fns = [value for key, value in sorted(globals().items()) if key.startswith("test_")]
    fails = 0
    for fn in fns:
        try:
            fn()
            print(f"PASS: {fn.__name__}")
        except AssertionError as exc:
            fails += 1
            print(f"FAIL: {fn.__name__}: {exc}")
        except Exception as exc:
            fails += 1
            print(f"FAIL: {fn.__name__}: {type(exc).__name__}: {exc}")
    sys.exit(1 if fails else 0)
