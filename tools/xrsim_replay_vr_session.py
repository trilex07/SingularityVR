#!/usr/bin/env python3
"""Replay a recorded SingularityVR .vrrec through the dvr-xrsim control pipe.

The recording is the canonical headset test route.  This tool deliberately
does not touch Steam files or inject Windows input: it only publishes the
recorded OpenXR-like poses and values to xrsim's atomic command channel.
"""
from __future__ import annotations

import argparse
import importlib.util
import json
import math
import os
import struct
import sys
import tempfile
import time
from pathlib import Path


GAMEPLAY = 1 << 0
MENU = 1 << 1
PANEL = 1 << 2
UI_DOWN = 1 << 4
FIRE = 1 << 9
MELEE = 1 << 10
RECENTER = 1 << 11
_next_publish_ns = 0
RAD2DEG = 57.29577951308232


def load_rows(path: Path):
    src = Path(__file__).with_name("vr_session_dump.py")
    spec = importlib.util.spec_from_file_location("vr_session_dump", src)
    if spec is None or spec.loader is None:
        raise RuntimeError(f"cannot load {src}")
    mod = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(mod)
    return list(mod.read(path))


def mode(flags: int) -> str:
    if flags & PANEL:
        return "panel"
    if flags & MENU:
        return "menu"
    if flags & GAMEPLAY:
        return "gameplay"
    return "unknown"


def clamp(v: float, lo: float, hi: float) -> float:
    return max(lo, min(hi, float(v)))


def f(v: float) -> str:
    # Short, deterministic ASCII accepted by the simulator parser.
    return f"{float(v):.6f}"


def state(path: Path) -> dict:
    try:
        return json.loads(path.read_text(encoding="utf-8"))
    except (OSError, ValueError):
        return {}


def atomic_command(path: Path, text: str) -> None:
    # Publish a complete batch in one directory-entry replacement.  This is
    # important for long runs: an in-place truncate can be observed as an
    # empty command by the simulator poller, after which that poll interval is
    # lost.  The temporary file stays on the same NTFS volume.
    global _next_publish_ns
    last = None
    if not _next_publish_ns:
        try:
            _next_publish_ns = max(time.time_ns(), path.stat().st_mtime_ns + 1)
        except FileNotFoundError:
            _next_publish_ns = time.time_ns()
    # Stay at least one millisecond ahead of the real clock.  A fixed +20 ms
    # per batch would eventually fall behind when the simulator is busy and
    # can then look like a stale command stream to diagnostics.
    _next_publish_ns = max(_next_publish_ns + 20_000_000, time.time_ns() + 1_000_000)
    for _ in range(30):
        fd, name = tempfile.mkstemp(prefix="xrsim-replay-", suffix=".tmp", dir=str(path.parent), text=True)
        try:
            with os.fdopen(fd, "w", encoding="ascii", newline="\n") as out:
                out.write(text)
                out.flush()
                os.fsync(out.fileno())
            os.replace(name, path)
            # NTFS timestamps can be coalesced when two writes land in one
            # scheduler tick.  xrsim intentionally keys its poll on this
            # timestamp, so make the sequence strictly monotonic.
            os.utime(path, ns=(_next_publish_ns, _next_publish_ns))
            return
        except OSError as exc:
            last = exc
            try:
                os.unlink(name)
            except FileNotFoundError:
                pass
            time.sleep(0.005)
    raise OSError(f"could not publish {path}: {last}")


def wait_applied(state_path: Path, before: int, count: int, timeout: float) -> dict:
    want = before + count
    deadline = time.monotonic() + timeout
    last = {}
    while time.monotonic() < deadline:
        last = state(state_path)
        if int(last.get("cmdSeq", 0)) >= want:
            err = last.get("lastCmdError")
            if err:
                raise RuntimeError(f"xrsim rejected replay command: {err}")
            return last
        time.sleep(0.001)
    raise TimeoutError(f"xrsim did not apply batch ({before}->{want}); state={last}")


def apply_batch(cmd_path: Path, state_path: Path, lines, before: int, timeout: float) -> dict:
    """Submit one frame, retrying a transient xrsim poll miss.

    The runtime polls its command file on the render cadence.  If Windows
    schedules a write immediately after that poll, the file can remain for
    one cadence without being consumed.  Rewriting the identical batch is
    safe because cmdSeq is the acknowledgement boundary and keeps the
    replay lossless.
    """
    last = None
    for attempt in range(8):
        atomic_command(cmd_path, "\n".join(lines) + "\n")
        try:
            return wait_applied(state_path, before, len(lines), timeout)
        except TimeoutError as exc:
            last = exc
            # It may have been applied just after the timeout; check once
            # before writing again so no batch can be duplicated.
            current = state(state_path)
            if int(current.get("cmdSeq", 0)) >= before + len(lines):
                return current
            time.sleep(0.020 * (attempt + 1))
    raise TimeoutError(str(last))


def row_commands(row, prev_ui: bool, recenter_sent: bool, ui_button: str):
    # vr_session_recorder::Sample order, documented by vr_session_dump.py.
    head = row[3:6]
    left = row[8:11]
    right = row[11:14]
    head_yaw, head_roll = row[6], row[7]
    left_yaw, right_yaw = row[14], row[15]
    left_pitch, right_pitch = row[16], row[17]
    left_roll, right_roll = row[18], row[19]
    lt, rt, lg, rg = (clamp(row[i], 0.0, 1.0) for i in range(20, 24))
    lsx, lsy, rsx, rsy = (clamp(row[i], -1.0, 1.0) for i in range(24, 28))
    flags = int(row[2])
    ui = bool(flags & UI_DOWN)

    lines = [
        "head pose " + " ".join(map(f, (*head, head_yaw * RAD2DEG, 0.0, head_roll * RAD2DEG))),
        "hand l grip pose " + " ".join(map(f, (*left, left_yaw * RAD2DEG, left_pitch * RAD2DEG, left_roll * RAD2DEG))),
        "hand r grip pose " + " ".join(map(f, (*right, right_yaw * RAD2DEG, right_pitch * RAD2DEG, right_roll * RAD2DEG))),
        f"trigger l {f(lt)}",
        f"trigger r {f(rt)}",
        f"grip l {f(lg)}",
        f"grip r {f(rg)}",
        f"stick l {f(lsx)} {f(lsy)}",
        f"stick r {f(rsx)} {f(rsy)}",
    ]
    # UI_DOWN is a semantic hold.  The recording does not store whether it
    # came from R2 or A, but all six recorded UI click windows have zero
    # trigger and therefore came from the A/UI-select path.  Preserve a real
    # down/up edge instead of issuing repeated presses.
    if ui and not prev_ui:
        lines.append(f"btn {ui_button} down")
    elif prev_ui and not ui:
        lines.append(f"btn {ui_button} up")

    # L3+R3 recenter is represented by a semantic command in xrsim.  It is
    # emitted once for the recorded rising event and never repeatedly held.
    rec = bool(flags & RECENTER)
    if rec and not recenter_sent:
        lines.append("recenter")
        recenter_sent = True
    elif not rec:
        recenter_sent = False

    return lines, ui, recenter_sent


def validate_rows(rows):
    transitions = []
    events = {"fire": 0, "melee": 0, "recenter": 0, "ui_click": 0}
    prev_mode = None
    prev_ui = False
    prev_rec = False
    for row in rows:
        flags = int(row[2])
        current = mode(flags)
        if prev_mode is not None and current != prev_mode:
            transitions.append((int(row[0]), prev_mode, current))
        prev_mode = current
        if flags & FIRE:
            events["fire"] += 1
        if flags & MELEE:
            events["melee"] += 1
        ui = bool(flags & UI_DOWN)
        rec = bool(flags & RECENTER)
        if ui and not prev_ui:
            events["ui_click"] += 1
        if rec and not prev_rec:
            events["recenter"] += 1
        prev_ui, prev_rec = ui, rec
    return transitions, events


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("recording", type=Path)
    ap.add_argument("--dir", type=Path, required=False,
                    help="xrsim data directory (defaults to DVR_DATA_DIR or LocalAppData)")
    ap.add_argument("--speed", type=float, default=1.0,
                    help="timing multiplier; 1.0 reproduces recorded elapsed time")
    ap.add_argument("--stride", type=int, default=4,
                    help="regular samples to skip (4 is the tested xrsim-safe route rate; event/state edges are retained)")
    ap.add_argument("--start-delay", type=float, default=0.0,
                    help="seconds to wait after the simulator preflight")
    ap.add_argument("--ack-timeout", type=float, default=0.5)
    ap.add_argument("--min-gap", type=float, default=0.020,
                    help="minimum wall gap between distinct command batches (>= Windows FILETIME tick)")
    ap.add_argument("--ui-button", choices=("a", "x"), default="a")
    ap.add_argument("--dry-run", action="store_true")
    ap.add_argument("--report", type=Path)
    args = ap.parse_args()
    if args.speed <= 0:
        ap.error("--speed must be > 0")
    if args.stride < 1:
        ap.error("--stride must be >= 1")
    if not args.recording.is_file():
        ap.error(f"recording not found: {args.recording}")
    rows = load_rows(args.recording)
    if not rows:
        ap.error("recording has no samples")
    transitions, events = validate_rows(rows)
    source_rows = rows
    if args.stride > 1:
        # Keep every regular stride plus mode/event edges.  This retains the
        # exact user route and all actions while staying below xrsim's finite
        # command-channel budget on long sessions.
        selected = []
        previous_mode = None
        previous_ui = False
        previous_rec = False
        for i, row in enumerate(rows):
            flags = int(row[2])
            current_mode = mode(flags)
            ui = bool(flags & UI_DOWN)
            rec = bool(flags & RECENTER)
            edge = (current_mode != previous_mode or ui != previous_ui or rec != previous_rec or
                    bool(flags & (FIRE | MELEE)))
            if i == 0 or i == len(rows) - 1 or i % args.stride == 0 or edge:
                selected.append(row)
            previous_mode, previous_ui, previous_rec = current_mode, ui, rec
        rows = selected
    duration = max(0.0, (int(source_rows[-1][1]) - int(source_rows[0][1])) / 1000.0 / args.speed)
    report = {
        "recording": str(args.recording),
        "source_samples": len(source_rows),
        "samples": len(rows),
        "stride": args.stride,
        "recorded_duration_s": (int(rows[-1][1]) - int(rows[0][1])) / 1000.0,
        "replay_duration_s": duration,
        "events": events,
        "transitions": [{"frame": a, "from": b, "to": c} for a, b, c in transitions],
        "ui_button": args.ui_button,
        "applied_batches": 0,
        "skipped_duplicate_batches": 0,
        "first_frame": int(rows[0][0]),
        "last_frame": int(rows[-1][0]),
    }
    print(f"recording={args.recording}")
    print(f"samples={len(rows)} source_samples={len(source_rows)} stride={args.stride} recorded_duration={report['recorded_duration_s']:.3f}s replay_duration={duration:.3f}s speed={args.speed:g}")
    print("events=" + ", ".join(f"{k}:{v}" for k, v in events.items()))
    print("transitions=" + ("; ".join(f"{a}:{b}->{c}" for a, b, c in transitions) or "none"))
    if args.dry_run:
        print("PASS: dry-run command stream validated")
        if args.report:
            args.report.parent.mkdir(parents=True, exist_ok=True)
            args.report.write_text(json.dumps(report, indent=2), encoding="utf-8")
        return 0

    data_dir = args.dir or Path(os.environ.get("DVR_DATA_DIR", Path.home() / "AppData/Local/DishonoredVR/xrsim"))
    state_path = data_dir / "state.json"
    cmd_path = data_dir / "command.txt"
    current = state(state_path)
    if not current.get("sessionRunning") or current.get("sessionState") not in ("FOCUSED", "VISIBLE"):
        raise RuntimeError(f"no focused/visible xrsim session at {data_dir}: {current}")
    if args.start_delay:
        time.sleep(args.start_delay)

    prev_ui = False
    recenter_sent = False
    previous_text = None
    last_write_wall = 0.0
    last_recorded_ms = int(rows[0][1])
    start_wall = time.monotonic()
    prev_cmd_seq = int(state(state_path).get("cmdSeq", 0))
    for index, row in enumerate(rows):
        if index:
            delta = max(0.0, (int(row[1]) - last_recorded_ms) / 1000.0 / args.speed)
            # A recording can contain a long pause while the application is
            # loading.  Preserve it, but bound pathological clock jumps.
            time.sleep(min(delta, 2.0))
        last_recorded_ms = int(row[1])
        lines, prev_ui, recenter_sent = row_commands(row, prev_ui, recenter_sent, args.ui_button)
        text = "\n".join(lines) + "\n"
        # xrsim intentionally ignores an identical command file to prevent a
        # stale command from being re-applied forever.  A recorded route can
        # contain adjacent identical samples, so treat that as an already
        # active state rather than waiting for an acknowledgement that cannot
        # arrive.
        if text == previous_text:
            report["skipped_duplicate_batches"] += 1
        else:
            gap = time.monotonic() - last_write_wall
            if gap < args.min_gap:
                time.sleep(args.min_gap - gap)
            st = apply_batch(cmd_path, state_path, lines, prev_cmd_seq, args.ack_timeout)
            prev_cmd_seq = int(st.get("cmdSeq", prev_cmd_seq + len(lines)))
            report["applied_batches"] += 1
            previous_text = text
            last_write_wall = time.monotonic()
        if int(row[2]) & (FIRE | MELEE | RECENTER) or (int(row[2]) & UI_DOWN and not index):
            print(f"event sample={index} frame={row[0]} mode={mode(int(row[2]))} flags=0x{int(row[2]):x}")
    report["elapsed_wall_s"] = time.monotonic() - start_wall
    report["final_cmd_seq"] = prev_cmd_seq
    report["final_state"] = state(state_path)
    print(f"PASS: replay applied {report['applied_batches']} batches in {report['elapsed_wall_s']:.3f}s")
    if args.report:
        args.report.parent.mkdir(parents=True, exist_ok=True)
        args.report.write_text(json.dumps(report, indent=2), encoding="utf-8")
        print(f"report={args.report}")
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except (RuntimeError, TimeoutError, ValueError) as exc:
        print(f"FAIL: {exc}", file=sys.stderr)
        raise SystemExit(1)
