"""Summarize existing bounded Seeker receipts without promoting them to HMD PASS.

No process/game access, inventory writes, test launches or user-save access.
The report distinguishes an accepted grip, a requested selection, actual equip,
and the post-initialization projectile from their earlier intermediate receipts.
"""
from __future__ import annotations

import argparse
from collections import Counter
import hashlib
import json
import math
from pathlib import Path
import re


NUMBER = r"[-+]?(?:\d+(?:\.\d*)?|\.\d+)(?:[eE][-+]?\d+)?"
VECTOR = rf"\(({NUMBER}) ({NUMBER}) ({NUMBER})\)"


def vector(line: str, name: str):
    match = re.search(rf"\b{re.escape(name)}={VECTOR}", line)
    return tuple(map(float, match.groups())) if match else None


def distance(a, b):
    return math.sqrt(sum((x - y) ** 2 for x, y in zip(a, b)))


def angle(a, b):
    aa, bb = sum(x*x for x in a), sum(x*x for x in b)
    if not aa or not bb:
        return None
    return math.degrees(math.acos(max(-1.0, min(1.0,
        sum(x*y for x, y in zip(a, b)) / math.sqrt(aa*bb)))))


def analyze(text: str) -> dict:
    grips, stages = Counter(), Counter()
    distances, launches, results, gloves = [], [], [], []
    retention, pending, unpaired = [], None, []
    for number, line in enumerate(text.splitlines(), 1):
        if "Seeker back grip: " in line:
            gate = re.search(r"gate=([\w-]+)", line)
            if gate:
                grips[gate[1]] += 1
                available = re.search(r"availablePawn=([0-9A-Fa-f]+)", line)
                d = re.search(rf"distance=({NUMBER}) radius=({NUMBER})", line)
                if d:
                    distances.append({"line": number, "gate": gate[1],
                        "available_pawn": available[1] if available else None,
                        "distance_m": float(d[1]), "radius_m": float(d[2])})
        if "Seeker back gate: " in line:
            stage = re.search(r"stage=([\w-]+)", line)
            if stage:
                stages[stage[1]] += 1
        if "Seeker back slot: retained owned item=" in line:
            retention.append(number)
        if "Seeker launch: native " in line:
            if pending is not None:
                unpaired.append(pending["line"])
            origin, expected = vector(line, "origin"), vector(line, "expectedOrigin")
            direction, barrel = vector(line, "direction"), vector(line, "barrel")
            pending = {"line": number, "origin": origin, "direction": direction,
                "origin_expected_error_uu": distance(origin, expected)
                    if origin and expected else None,
                "native_spread_from_barrel_deg": angle(direction, barrel)
                    if direction and barrel else None}
            launches.append(pending)
        if "Seeker projectile result: " in line:
            location, velocity = vector(line, "location"), vector(line, "velocity")
            origin, direction = vector(line, "inputOrigin"), vector(line, "inputDirection")
            valid = all(f"{name}Valid=1" in line for name in ("location", "velocity", "rotation"))
            # Pair only consecutive, matching argument receipts. A later result,
            # null return or a different shot must not inherit earlier evidence.
            paired = bool(pending and origin and direction and pending["origin"]
                and pending["direction"] and distance(origin, pending["origin"]) <= .002
                and distance(direction, pending["direction"]) <= .00002)
            results.append({"line": number, "launch_line": pending["line"] if paired else None,
                "readable_native_result": valid,
                "spawn_location_error_uu": distance(location, origin)
                    if valid and location and origin else None,
                "initialized_velocity_vs_input_deg": angle(velocity, direction)
                    if valid and velocity and direction else None,
                "initialized_speed_uu_per_s": math.sqrt(sum(x*x for x in velocity))
                    if valid and velocity else None})
            if pending and not paired:
                unpaired.append(pending["line"])
            pending = None
        if "Seeker glove completed: " in line:
            match = re.search(r"frame=(\d+) eyes=(\d+) draw=([0-9A-Fa-f]+) "
                r"ibRestore=([0-9A-Fa-f]+) paletteRestore=([0-9A-Fa-f]+)", line)
            if match:
                frame, eyes, draw, ib, palette = match.groups()
                gloves.append({"line": number, "frame": int(frame), "eyes": int(eyes),
                    "draw_hresult": int(draw, 16), "ib_restore_hresult": int(ib, 16),
                    "palette_restore_hresult": int(palette, 16)})
    if pending:
        unpaired.append(pending["line"])
    initialized = [r for r in results if r["readable_native_result"] and r["launch_line"]]
    successful_gloves = [g for g in gloves if g["eyes"] == 2 and
        not (g["draw_hresult"] or g["ib_restore_hresult"] or g["palette_restore_hresult"])]
    coverage = {
        "owned_and_hook_ready_observed": stages["ready"] > 0,
        "accepted_back_zone_edge_observed": grips["accept"] > 0,
        "native_retention_hook_observed": bool(retention),
        "native_selection_requested_observed": stages["selection-requested"] > 0,
        "native_selection_equipped_observed": stages["selection-equipped"] > 0,
        "post_native_initialized_launch_observed": bool(initialized),
        "two_eye_glove_submission_observed": bool(successful_gloves),
        "save_load_persistence": "NOT ESTABLISHED by bounded receipts",
        "physical_visibility_and_impact": "PENDING HUMAN PSVR2 VERIFICATION",
    }
    return {"schema": 1, "evidence_level": "ARCHIVED RUNTIME RECEIPTS ONLY",
        "coverage": coverage, "back_grip_gates": dict(grips), "native_stages": dict(stages),
        "grip_distances": distances, "retention_lines": retention,
        "launches": launches, "native_results": results, "unpaired_launch_lines": unpaired,
        "glove_submissions": gloves,
        "limitations": ["Bounded logs may omit later events after their receipt budget is exhausted.",
            "Numerical values reflect log rounding; no spread/velocity threshold is invented.",
            "Native projectile steering and eventual impact are not measured here.",
            "A two-eye successful draw is not proof of visible or correctly positioned glove pixels.",
            "No artifact/source identity is inferred from a log filename or old agent report."]}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("log", type=Path)
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--source-commit", help="Externally verified source identity, never inferred")
    parser.add_argument("--dll-sha256", help="Externally verified deployed artifact identity")
    args = parser.parse_args()
    data = args.log.read_bytes()
    report = analyze(data.decode("utf-8-sig", errors="replace"))
    report["evidence"] = {"log": str(args.log.resolve()), "sha256": hashlib.sha256(data).hexdigest(),
        "source_commit": args.source_commit, "dll_sha256": args.dll_sha256,
        "identity_note": "Caller-supplied identities require the deployment manifest; absent values remain unknown."}
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
    print(json.dumps(report["coverage"], indent=2))


if __name__ == "__main__":
    main()
