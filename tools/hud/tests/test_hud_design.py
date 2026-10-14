#!/usr/bin/env python3
"""Offline checks for the HUD design pack. No game assets and no runtime."""
from __future__ import annotations

import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[3]
DOCS = ROOT / "docs" / "hud"
MOCKS = DOCS / "mockups"


def test_required_files() -> None:
    for name in (
        "HUD_INVENTORY.md",
        "HUD_DESIGN.md",
        "HUD_STAGE1.md",
        "mockups/01-spokoj.svg",
        "mockups/02-walka.svg",
        "mockups/03-zmiana-broni.svg",
        "mockups/04-interakcja.svg",
    ):
        path = DOCS / name
        assert path.is_file() and path.stat().st_size > 200, name
    assert (ROOT / "docs" / "handoffs" / "main-psvr2-hud.md").is_file()


def test_states_and_polish() -> None:
    calm = (MOCKS / "01-spokoj.svg").read_text(encoding="utf-8")
    fight = (MOCKS / "02-walka.svg").read_text(encoding="utf-8")
    switch = (MOCKS / "03-zmiana-broni.svg").read_text(encoding="utf-8")
    interact = (MOCKS / "04-interakcja.svg").read_text(encoding="utf-8")
    assert "10/10" in calm and "12" in calm
    assert "2/10" in fight and "PRZEŁADUJ" in fight and "0" in fight
    assert "Centurion" in switch
    assert "Notatka" in interact
    for text in (calm, fight, switch, interact):
        assert "środek wolny" in text or "wróg" in text or "notatka w świecie" in text
        assert "#c4a46a" not in text or "2/10" in text or "PRZEŁADUJ" in text or "0" in text


def test_unknowns_stay_unknown() -> None:
    inventory = (DOCS / "HUD_INVENTORY.md").read_text(encoding="utf-8")
    stage = (DOCS / "HUD_STAGE1.md").read_text(encoding="utf-8")
    assert "mCounts" in inventory and "Nieznane" in inventory
    assert "Health" in stage and "Pawn.Weapon" in stage
    assert "nie czytamy" in stage.lower() or "Nie czytamy" in stage


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
    sys.exit(1 if fails else 0)
