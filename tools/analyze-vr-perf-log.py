#!/usr/bin/env python3
"""Standalone analyzer for SingularityVR view_matrix performance logs.

Parses the log emitted by spikes/view_matrix/d3d9.cpp and produces:
  - frame-budget summary (median fps, "our work", XR wait/submit)
  - hot-path family ranking from the bounded sampler
  - frame-spike inventory with contextual tags (jump, melee, weapon swap, no centre)
  - GObjects walk / register-scan events
  - state buckets (wait vs submit per activity state)
  - frame-rate governor activity

Usage:
    python analyze-vr-perf-log.py path/to/view_matrix.log > report.md

No Steam, runtime, or synthetic FPS claims.  This is a research aid only.
"""

import argparse
import re
import sys
from collections import defaultdict
from dataclasses import dataclass, field
from statistics import median
from typing import Dict, List, Optional, Tuple


@dataclass
class FrameBudget:
    median_fps: float = 0.0
    our_work_ms: float = 0.0
    xr_wait_ms: float = 0.0
    xr_wait_state: str = ""
    xr_submit_ms: float = 0.0


@dataclass
class Spike:
    line: int
    frame_ms: float
    period_ms: float
    draws_total: int
    draws_dup: int
    draws_mono: int
    melee: bool
    heal: bool
    weapon_swapped: bool
    no_centre: bool
    melee_age_ms: int
    jump_age_ms: int


@dataclass
class HotpathFamily:
    calls: int = 0
    samples: int = 0
    avg_us: float = 0.0
    est_ms_per_frame: float = 0.0


@dataclass
class WalkEvent:
    kind: str
    objects: int = 0
    ms: float = 0.0


@dataclass
class MenuTraceEvent:
    line: int
    state: int
    groups: int
    dropped: int


@dataclass
class PerfReport:
    budgets: List[FrameBudget] = field(default_factory=list)
    spikes: List[Spike] = field(default_factory=list)
    hotpath: Dict[str, HotpathFamily] = field(default_factory=dict)
    gobjects_walks: List[WalkEvent] = field(default_factory=list)
    scans: List[WalkEvent] = field(default_factory=list)
    menu_traces: List[MenuTraceEvent] = field(default_factory=list)
    sleep_calls: int = 0
    sleep_ms: int = 0
    sleep_neutralised: bool = False


def parse_budget_line(line: str) -> Optional[FrameBudget]:
    """fps median 119.9   our work 3.02 ms   xrWaitFrame 4.35 ms IDLE   XR submit 0.94 ms"""
    m = re.search(
        r"fps median\s+([0-9.]+)\s+our work\s+([0-9.]+)\s+ms\s+xrWaitFrame\s+([0-9.]+)\s+ms\s+(\w+)\s+XR submit\s+([0-9.]+)\s+ms",
        line,
    )
    if not m:
        return None
    return FrameBudget(
        median_fps=float(m.group(1)),
        our_work_ms=float(m.group(2)),
        xr_wait_ms=float(m.group(3)),
        xr_wait_state=m.group(4),
        xr_submit_ms=float(m.group(5)),
    )


def parse_spike_line(line: str, line_no: int) -> Optional[Spike]:
    m = re.search(
        r"FRAME SPIKE\s+([0-9.]+)\s+ms\s+\(period\s+([0-9.]+)\):\s+draws\s+(\d+)\s+\(dup\s+(\d+)\s+mono\s+(\d+)\)\s+melee\s+(\d+)\s+heal\s+(\d+)\s+swapped\s+(\d+)\s+nocentre\s+(\d+)\s+meleeAge\s+(-?\d+)\s+jumpAge\s+(-?\d+)",
        line,
    )
    if not m:
        return None
    return Spike(
        line=line_no,
        frame_ms=float(m.group(1)),
        period_ms=float(m.group(2)),
        draws_total=int(m.group(3)),
        draws_dup=int(m.group(4)),
        draws_mono=int(m.group(5)),
        melee=m.group(6) == "1",
        heal=m.group(7) == "1",
        weapon_swapped=m.group(8) == "1",
        no_centre=m.group(9) == "1",
        melee_age_ms=int(m.group(10)),
        jump_age_ms=int(m.group(11)),
    )


def parse_hotpath_line(line: str) -> Dict[str, HotpathFamily]:
    out: Dict[str, HotpathFamily] = {}
    # family[calls=N samples=N avg_us=X est_ms/frame=Y]
    for m in re.finditer(
        r"(\w+)\[calls=(\d+)\s+samples=(\d+)\s+avg_us=([0-9.eE+-]+)\s+est_ms/frame=([0-9.eE+-]+)\]",
        line,
    ):
        out[m.group(1)] = HotpathFamily(
            calls=int(m.group(2)),
            samples=int(m.group(3)),
            avg_us=float(m.group(4)),
            est_ms_per_frame=float(m.group(5)),
        )
    return out


def parse_walk_line(line: str) -> Optional[WalkEvent]:
    # player pawn: ... after walking N object(s)
    m = re.search(r"after walking\s+(\d+)\s+object\(s\)", line)
    if m:
        # No per-call timing in the log; rely on the frame-budget impact.
        return WalkEvent(kind="FindPlayerPawn", objects=int(m.group(1)))
    return None


def parse_scan_line(line: str) -> Optional[WalkEvent]:
    # --- scan complete: 0 candidate window(s) ...
    m = re.search(r"scan complete:\s+(\d+)\s+candidate window\(s\)", line)
    if m:
        return WalkEvent(kind="RegisterScan", objects=int(m.group(1)))
    return None


def parse_menu_trace_line(line: str, line_no: int) -> Optional[MenuTraceEvent]:
    # MenuDrawTrace state=123456789 class=2 groups=47 dropped=0 sample=frames20..27 path=1:stereo,2:hud,4:provider
    m = re.search(
        r"MenuDrawTrace state=(\d+)\s+class=(\d+)\s+groups=(\d+)\s+dropped=(\d+)",
        line,
    )
    if not m:
        return None
    return MenuTraceEvent(line=line_no, state=int(m.group(1)), groups=int(m.group(3)), dropped=int(m.group(4)))


def parse_sleep_line(line: str) -> Tuple[int, int, bool]:
    m = re.search(
        r"frame-rate governor:\s+(\d+)\s+render-thread sleeps totalling\s+(\d+)\s+ms",
        line,
    )
    if not m:
        return 0, 0, False
    neutralised = "NEUTRALISED" in line
    return int(m.group(1)), int(m.group(2)), neutralised


def parse_log(text: str) -> PerfReport:
    report = PerfReport()
    for line_no, line in enumerate(text.splitlines(), start=1):
        b = parse_budget_line(line)
        if b:
            report.budgets.append(b)
            continue
        s = parse_spike_line(line, line_no)
        if s:
            report.spikes.append(s)
            continue
        hp = parse_hotpath_line(line)
        if hp:
            report.hotpath = hp
            continue
        w = parse_walk_line(line)
        if w:
            report.gobjects_walks.append(w)
            continue
        sc = parse_scan_line(line)
        if sc:
            report.scans.append(sc)
            continue
        mt = parse_menu_trace_line(line, line_no)
        if mt:
            report.menu_traces.append(mt)
            continue
        scalls, sms, sneu = parse_sleep_line(line)
        if scalls:
            report.sleep_calls += scalls
            report.sleep_ms += sms
            report.sleep_neutralised = report.sleep_neutralised or sneu
    return report


def classify_spike(s: Spike) -> List[str]:
    tags = []
    if s.jump_age_ms >= 0 and s.jump_age_ms < 500:
        tags.append("jump-recent")
    if s.melee_age_ms >= 0 and s.melee_age_ms < 500:
        tags.append("melee-recent")
    if s.melee:
        tags.append("melee-window")
    if s.heal:
        tags.append("heal-window")
    if s.weapon_swapped:
        tags.append("weapon-swap")
    if s.no_centre:
        tags.append("no-centre")
    if s.draws_mono > s.draws_dup:
        tags.append("mono-fallback-heavy")
    return tags


def spikes_near_menu_traces(spikes: List[Spike], traces: List[MenuTraceEvent], window_lines: int = 200) -> int:
    """Count spikes whose line number is within window_lines of any MenuDrawTrace flush."""
    if not spikes or not traces:
        return 0
    trace_lines = sorted(t.line for t in traces)
    count = 0
    for s in spikes:
        # binary search for nearest trace line
        lo, hi = 0, len(trace_lines)
        while lo < hi:
            mid = (lo + hi) // 2
            if trace_lines[mid] < s.line:
                lo = mid + 1
            else:
                hi = mid
        nearest = None
        if lo < len(trace_lines):
            nearest = trace_lines[lo]
        if lo > 0 and (nearest is None or abs(trace_lines[lo - 1] - s.line) < abs(nearest - s.line)):
            nearest = trace_lines[lo - 1]
        if nearest is not None and abs(nearest - s.line) <= window_lines:
            count += 1
    return count


def render_report(r: PerfReport) -> str:
    lines: List[str] = []
    lines.append("# SingularityVR frame-hitch analysis (from log)")
    lines.append("")
    lines.append("This report is produced by `tools/analyze-vr-perf-log.py` from the bounded counters")
    lines.append("and one-shot spike lines already emitted by the mod.  It does not run Steam or")
    lines.append("claim synthetic FPS improvements.")
    lines.append("")

    if r.budgets:
        fps_vals = [b.median_fps for b in r.budgets]
        our_vals = [b.our_work_ms for b in r.budgets]
        wait_vals = [b.xr_wait_ms for b in r.budgets]
        sub_vals = [b.xr_submit_ms for b in r.budgets]
        lines.append("## Frame budget summary")
        lines.append("")
        lines.append(f"- Budget windows parsed: {len(r.budgets)}")
        lines.append(f"- Median FPS: {median(fps_vals):.1f} (range {min(fps_vals):.1f}–{max(fps_vals):.1f})")
        lines.append(f"- 'Our work' median: {median(our_vals):.2f} ms (range {min(our_vals):.2f}–{max(our_vals):.2f})")
        lines.append(f"- XR wait-frame median: {median(wait_vals):.2f} ms (range {min(wait_vals):.2f}–{max(wait_vals):.2f})")
        lines.append(f"- XR submit median: {median(sub_vals):.2f} ms (range {min(sub_vals):.2f}–{max(sub_vals):.2f})")
        lines.append("")

    if r.hotpath:
        lines.append("## Hot-path family ranking (est. ms/frame)")
        lines.append("")
        ranked = sorted(r.hotpath.items(), key=lambda kv: kv[1].est_ms_per_frame, reverse=True)
        lines.append("| family | calls | samples | avg µs/call | est ms/frame |")
        lines.append("|---|---:|---:|---:|---:|")
        for name, f in ranked:
            lines.append(
                f"| {name} | {f.calls} | {f.samples} | {f.avg_us:.3f} | {f.est_ms_per_frame:.4f} |"
            )
        lines.append("")
        known = sum(
            f.est_ms_per_frame
            for n, f in r.hotpath.items()
            if n in {"wait_frame", "pose_input", "d3d_copy", "xr_images", "xr_submit"}
        )
        present_total = r.hotpath.get("present_frame", HotpathFamily()).est_ms_per_frame
        lines.append(f"- Measured XR + copy total: {known:.4f} ms/frame")
        lines.append(f"- Present-hook total (inclusive): {present_total:.4f} ms/frame")
        lines.append(f"- Unattributed residual in Present hook: {max(0.0, present_total - known):.4f} ms/frame")
        lines.append("")

    if r.spikes:
        lines.append("## Frame spikes")
        lines.append("")
        lines.append(f"- Spike count: {len(r.spikes)}")
        spikes_by_tag: Dict[str, int] = defaultdict(int)
        for s in r.spikes:
            for tag in classify_spike(s):
                spikes_by_tag[tag] += 1
        if spikes_by_tag:
            lines.append("- Spike context tags:")
            for tag, count in sorted(spikes_by_tag.items(), key=lambda kv: kv[1], reverse=True):
                lines.append(f"  - `{tag}`: {count}")
        lines.append("")
        lines.append("| frame ms | period ms | draws | dup | mono | tags |")
        lines.append("|---:|---:|---:|---:|---:|---|")
        for s in r.spikes:
            tags = ", ".join(classify_spike(s))
            lines.append(
                f"| {s.frame_ms:.1f} | {s.period_ms:.2f} | {s.draws_total} | {s.draws_dup} | {s.draws_mono} | {tags} |"
            )
        lines.append("")

    if r.gobjects_walks:
        total_objects = sum(w.objects for w in r.gobjects_walks)
        lines.append("## GObjects walks")
        lines.append("")
        lines.append(f"- Walk events: {len(r.gobjects_walks)}")
        lines.append(f"- Total objects walked: {total_objects}")
        if r.gobjects_walks:
            lines.append(f"- Max objects in one walk: {max(w.objects for w in r.gobjects_walks)}")
        lines.append("")

    if r.scans:
        zero_candidate = sum(1 for s in r.scans if s.objects == 0)
        lines.append("## Register scans")
        lines.append("")
        lines.append(f"- Scan events: {len(r.scans)} ({zero_candidate} with zero candidates)")
        lines.append("")

    if r.menu_traces:
        total_groups = sum(t.groups for t in r.menu_traces)
        total_dropped = sum(t.dropped for t in r.menu_traces)
        lines.append("## Menu draw trace activity")
        lines.append("")
        lines.append(f"- Trace flush events: {len(r.menu_traces)}")
        lines.append(f"- Total trace groups emitted: {total_groups}")
        lines.append(f"- Total dropped groups: {total_dropped}")
        if r.spikes:
            near = spikes_near_menu_traces(r.spikes, r.menu_traces)
            lines.append(f"- Spikes within 200 lines of a trace flush: {near} of {len(r.spikes)}")
            if near:
                lines.append("  - *If these coincide temporally, the 8-frame trace sampling window may be contributing to transition hitches.*")
        lines.append("")

    if r.sleep_calls:
        lines.append("## Frame-rate governor")
        lines.append("")
        lines.append(f"- Render-thread sleeps: {r.sleep_calls} totalling {r.sleep_ms} ms")
        lines.append(f"- Neutralised by NUMPAD+: {r.sleep_neutralised}")
        lines.append("")

    lines.append("## Interpretation notes")
    lines.append("")
    lines.append("- XR `wait_frame` is mostly runtime idle time, not mod work.")
    lines.append("- `our work` is a residual bucket: it absorbs any unmeasured latency, so a")
    lines.append("  spike there needs the hot-path ranking below it to locate the cause.")
    lines.append("- **Splash/transition stalls** are identified by `player pawn: ... after walking N")
    lines.append("  object(s)` and `scan complete: 0 candidate window(s)`. These are structurally")
    lines.append("  different from jump/combat hitches and should not be treated as the same root cause.")
    lines.append("- **Jump/combat hitches** are identified by `FRAME SPIKE` lines carrying")
    lines.append("  `jump-recent`, `melee-recent`, `melee-window` or `weapon-swap` tags.")
    lines.append("- **Menu transition hitches** may correlate with `MenuDrawTrace` flush lines; the")
    lines.append("  trace samples frames 20-27 after every menu state change and issues many D3D")
    lines.append("  queries per draw during that window.")
    lines.append("- Tags are heuristic: `jump-recent` / `melee-recent` use the 500 ms age window")
    lines.append("  from the spike reporter.")
    lines.append("")
    return "\n".join(lines)


def main() -> int:
    parser = argparse.ArgumentParser(description="Analyze SingularityVR view_matrix.log")
    parser.add_argument("log", help="path to view_matrix.log")
    args = parser.parse_args()

    try:
        with open(args.log, "r", encoding="utf-8", errors="ignore") as f:
            text = f.read()
    except FileNotFoundError:
        print(f"error: file not found: {args.log}", file=sys.stderr)
        return 1

    report = parse_log(text)
    print(render_report(report))
    return 0


if __name__ == "__main__":
    sys.exit(main())
