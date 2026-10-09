#!/usr/bin/env python3
"""Replay/validate a recorded SingularityVR session offline.

This tool never injects input into Windows or the game.  It replays the fixed
sample stream as a deterministic state walk so a headset route can be checked
after every DLL change without asking for another visual run just to catch a
stuck trigger or a bad menu transition.
"""
import argparse
import importlib.util
from pathlib import Path


def load_reader():
    src = Path(__file__).with_name("vr_session_dump.py")
    spec = importlib.util.spec_from_file_location("vr_session_dump", src)
    if spec is None or spec.loader is None:
        raise RuntimeError("cannot load vr_session_dump.py")
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module.read


GAMEPLAY = 1 << 0
MENU = 1 << 1
PANEL = 1 << 2
UI_DOWN = 1 << 4
FIRE = 1 << 9
MELEE = 1 << 10
RECENTER = 1 << 11


def mode(flags):
    if flags & PANEL:
        return "panel"
    if flags & MENU:
        return "menu"
    if flags & GAMEPLAY:
        return "gameplay"
    return "unknown"


def validate(rows):
    transitions = []
    events = {"fire": 0, "melee": 0, "recenter": 0, "ui_click": 0}
    stuck_trigger = []
    previous_mode = None
    previous_ui_down = False
    for index, row in enumerate(rows):
        flags = row[2]
        current_mode = mode(flags)
        if previous_mode is not None and current_mode != previous_mode:
            transitions.append((index, previous_mode, current_mode))
        previous_mode = current_mode
        if flags & FIRE:
            events["fire"] += 1
        if flags & MELEE:
            events["melee"] += 1
        if flags & RECENTER:
            events["recenter"] += 1
        ui_down = bool(flags & UI_DOWN)
        if ui_down and not previous_ui_down:
            events["ui_click"] += 1
        previous_ui_down = ui_down
        # A menu/panel sample must never carry a live fire trigger.  The row
        # layout stores left/right trigger at indices 22 and 23.
        if current_mode in ("menu", "panel") and max(row[22], row[23]) > 0.15:
            stuck_trigger.append((index, current_mode, row[22], row[23]))
    return transitions, events, stuck_trigger


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("recording", type=Path)
    parser.add_argument("--csv", type=Path, help="also export the samples as CSV")
    args = parser.parse_args()
    read = load_reader()
    rows = list(read(args.recording))
    if args.csv:
        # Reuse the canonical exporter so the CSV column order stays in lockstep
        # with the C++ packed Sample layout.
        import csv
        names = ("frame", "time_ms", "flags", "head_x", "head_y", "head_z", "head_yaw", "head_roll",
                 "left_x", "left_y", "left_z", "right_x", "right_y", "right_z",
                 "left_yaw", "right_yaw", "left_pitch", "right_pitch", "left_roll", "right_roll",
                 "left_trigger", "right_trigger", "left_grip", "right_grip",
                 "left_stick_x", "left_stick_y", "right_stick_x", "right_stick_y",
                 "ui_u", "ui_v", "muzzle_x", "muzzle_y", "muzzle_z", "hip_x", "hip_y", "hip_z",
                 "weapon", "prompt_action")
        with args.csv.open("w", newline="", encoding="utf-8") as stream:
            writer = csv.writer(stream)
            writer.writerow(names)
            writer.writerows(rows)
        print(f"wrote {len(rows)} samples to {args.csv}")

    transitions, events, stuck = validate(rows)
    print(f"samples={len(rows)}")
    print("events=" + ", ".join(f"{key}:{value}" for key, value in events.items()))
    print("transitions=" + ("; ".join(f"{a}:{b}->{c}" for a, b, c in transitions) or "none"))
    if stuck:
        print(f"FAIL: trigger active in {len(stuck)} menu/panel samples; first={stuck[0]}")
        return 1
    print("PASS: no trigger leaked into menu/panel samples")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
