"""Analyze the opt-in cpu_microstutter CSV. Correlation is not causation.

Usage: python tools/perf/analyze_microstutter.py capture.csv [--json report.json]
The frame interval includes game, driver, compositor and scheduler work; only
render_thread_cpu_us is CPU time measured for the Present-calling thread.
"""
from __future__ import annotations

import argparse
import csv
import json
from pathlib import Path

EVENTS = {1: "fire", 2: "damage", 4: "weapon_switch", 8: "use_grip_attempt"}
THRESHOLDS_US = (11111, 16667, 22222, 33333)


def percentile(values: list[int], pct: int) -> int:
    if not values:
        return 0
    ordered = sorted(values)
    return ordered[(len(ordered) * pct + 99) // 100 - 1]


def read_capture(path: Path) -> tuple[list[dict[str, int]], int]:
    rows: list[dict[str, int]] = []
    lost = -1  # no footer: process exited before the bounded 9000-frame stop
    with path.open(newline="", encoding="utf-8") as stream:
        for row in csv.DictReader(line for line in stream if not line.startswith("#")):
            rows.append({key: int(value) for key, value in row.items()})
    for line in path.read_text(encoding="utf-8").splitlines():
        if line.startswith("# lost_frames="):
            lost = int(line.split("=", 1)[1])
    return rows, lost


def analyze(rows: list[dict[str, int]], lost: int = 0) -> dict:
    # The first frame has no preceding Present boundary and cannot have a wall interval.
    frames = [r for r in rows if r["frame_wall_us"] > 0]
    by_id = {r["frame"]: r for r in frames}
    wall = [r["frame_wall_us"] for r in frames]
    result = {
        "frames": len(frames), "lost_frames": lost,
        "capture_complete": lost >= 0,
        "wall_us": {"p50": percentile(wall, 50), "p95": percentile(wall, 95),
                    "p99": percentile(wall, 99), "max": max(wall, default=0)},
        "over_budget": {str(t): sum(v > t for v in wall) for t in THRESHOLDS_US},
        "events": {},
        "interpretation": "wall interval includes CPU, driver, waits and scheduling; event proximity alone is not a cause",
    }
    for bit, name in EVENTS.items():
        event_rows = [r for r in frames if r["event_mask"] & bit]
        # Three frames before/after capture first-shot initialization and delayed impacts.
        near = {f for r in event_rows for f in range(r["frame"] - 3, r["frame"] + 4)}
        window = [by_id[f] for f in near if f in by_id]
        result["events"][name] = {
            "count": len(event_rows),
            "window_frames": len(window),
            "window_p95_us": percentile([r["frame_wall_us"] for r in window], 95),
            "window_over_16_7ms": sum(r["frame_wall_us"] > 16667 for r in window),
            "native_max_us": max((r["fire_native_max_us"] if bit == 1 else r["damage_native_max_us"]
                                  for r in event_rows), default=0) if bit in (1, 2) else None,
        }
    # A true gameplay pickup receipt is not present in this shim. Do not rename
    # raw Use/Grip edges as successful pickups.
    result["pickup_producer_confirmed"] = False
    return result


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("capture", type=Path)
    parser.add_argument("--json", type=Path)
    args = parser.parse_args()
    rows, lost = read_capture(args.capture)
    result = analyze(rows, lost)
    out = json.dumps(result, indent=2)
    if args.json:
        args.json.write_text(out + "\n", encoding="utf-8")
    print(out)


if __name__ == "__main__":
    main()
