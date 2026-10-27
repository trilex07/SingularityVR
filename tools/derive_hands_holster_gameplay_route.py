#!/usr/bin/env python3
"""Create an isolated hands/holster xrsim route with a menu-select probe.

The input is the already-sanitized derived route, never an original headset
recording. This adds one semantic A/UI-select tap after startup, inserts a
stationary load window, and preserves the existing two right-grip pulses and
one-hand-at-a-time pose phases. The menu-select is only a probe; it does not
guarantee that the game enters a saved scene.
"""
from __future__ import annotations

import argparse
import hashlib
import json
import statistics
import struct
from pathlib import Path

HEADER = struct.Struct("<IIII")
SAMPLE = struct.Struct("<QII3f2f6f6f4f4f2f3f3fII")
MAGIC = 0x31524356
UI_DOWN = 1 << 4
FIRE = 1 << 9
MELEE = 1 << 10


def read_recording(path: Path):
    data = path.read_bytes()
    if len(data) < HEADER.size:
        raise ValueError("recording is shorter than its header")
    magic, version, sample_size, reserved = HEADER.unpack_from(data)
    if magic != MAGIC or version != 1 or sample_size != SAMPLE.size:
        raise ValueError("unsupported .vrrec header")
    if (len(data) - HEADER.size) % sample_size:
        raise ValueError("truncated sample at end of recording")
    rows = [SAMPLE.unpack_from(data, offset)
            for offset in range(HEADER.size, len(data), sample_size)]
    return (magic, version, sample_size, reserved), rows


def write_recording(path: Path, header, rows):
    path.parent.mkdir(parents=True, exist_ok=True)
    if path.exists():
        raise FileExistsError(f"refusing to overwrite {path}")
    with path.open("xb") as stream:
        stream.write(HEADER.pack(*header))
        for row in rows:
            stream.write(SAMPLE.pack(*row))


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("source", type=Path)
    parser.add_argument("output", type=Path)
    parser.add_argument("--audit", type=Path, required=True)
    args = parser.parse_args()

    header, source = read_recording(args.source)
    if len(source) != 2800:
        raise ValueError(f"expected the 2,800-sample isolated route, got {len(source)}")

    # This is a route derivation only. It must remain free of combat, movement,
    # interaction, UI, and recenter events before the deliberate test edges.
    for i, row in enumerate(source):
        if row[2] & (UI_DOWN | FIRE | MELEE):
            raise ValueError(f"source has an unexpected event flag at sample {i}")
        if max(row[20], row[21], row[22]) > 0.001:
            raise ValueError(f"source has a trigger or left-grip value at sample {i}")
        if max(abs(row[24]), abs(row[25]), abs(row[26]), abs(row[27])) > 0.001:
            raise ValueError(f"source has stick input at sample {i}")
    source_grip_edges = sum(row[23] >= 0.5 and
                            (i == 0 or source[i - 1][23] < 0.5)
                            for i, row in enumerate(source))
    if source_grip_edges != 2 or sum(row[23] >= 0.5 for row in source) != 48:
        raise ValueError("source does not contain the audited two 24-sample right-grip pulses")

    pulse_first, pulse_end = 800, 820
    load_window = 1000
    dt_ms = int(round(statistics.median(
        source[i + 1][1] - source[i][1] for i in range(len(source) - 1)
    )))
    if dt_ms <= 0:
        raise ValueError("non-positive sample period")

    rows = []
    for i, original in enumerate(source[:pulse_end]):
        row = list(original)
        if pulse_first <= i < pulse_end:
            row[2] |= UI_DOWN
        rows.append(tuple(row))

    hold_template = list(rows[-1])
    hold_template[2] &= ~(UI_DOWN | FIRE | MELEE)
    hold_start_frame = rows[-1][0]
    hold_start_time = rows[-1][1]
    for offset in range(1, load_window + 1):
        row = list(hold_template)
        row[0] = hold_start_frame + offset
        row[1] = hold_start_time + offset * dt_ms
        rows.append(tuple(row))

    time_shift = load_window * dt_ms
    for original in source[pulse_end:]:
        row = list(original)
        row[0] += load_window
        row[1] += time_shift
        if pulse_first <= row[0] - load_window < pulse_end:
            row[2] |= UI_DOWN
        rows.append(tuple(row))

    ui_edges = sum(bool(row[2] & UI_DOWN) and
                   (i == 0 or not (rows[i - 1][2] & UI_DOWN))
                   for i, row in enumerate(rows))
    right_grip_edges = sum(row[23] >= 0.5 and
                           (i == 0 or rows[i - 1][23] < 0.5)
                           for i, row in enumerate(rows))
    if (ui_edges, right_grip_edges) != (1, 2):
        raise ValueError("route edge audit failed")
    if sum(row[23] >= 0.5 for row in rows) != 48:
        raise ValueError("right-grip pulse duration differs from the audited source route")
    if any(max(row[20], row[21]) > 0.001 for row in rows):
        raise ValueError("trigger input leaked into route")
    if any(max(abs(row[24]), abs(row[25]), abs(row[26]), abs(row[27])) > 0.001
           for row in rows):
        raise ValueError("stick input leaked into route")

    write_recording(args.output, header, rows)
    audit = {
        "source": str(args.source.resolve()),
        "derived": str(args.output.resolve()),
        "samples": len(rows),
        "duration_s": (rows[-1][1] - rows[0][1]) / 1000.0,
        "ui_select": {"samples": [pulse_first, pulse_end - 1], "rising_edges": ui_edges},
        "stationary_load_window_samples": load_window,
        "right_grip_rising_edges": right_grip_edges,
        "left_grip_active_samples": sum(row[22] >= 0.5 for row in rows),
        "fire_trigger_samples": sum(max(row[20], row[21]) > 0.001 for row in rows),
        "stick_samples": sum(max(abs(row[24]), abs(row[25]), abs(row[26]), abs(row[27])) > 0.001
                              for row in rows),
        "source_sha256": hashlib.sha256(args.source.read_bytes()).hexdigest().upper(),
        "derived_sha256": hashlib.sha256(args.output.read_bytes()).hexdigest().upper(),
        "note": "The sole A/UI-select edge probes the documented main-menu Return path, but prior runtime evidence shows it may not leave the menu. Do not claim gameplay or holster coverage unless the live log confirms an equipped-hand draw.",
    }
    args.audit.parent.mkdir(parents=True, exist_ok=True)
    args.audit.write_text(json.dumps(audit, indent=2), encoding="utf-8")
    print(json.dumps(audit, indent=2))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
