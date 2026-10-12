#!/usr/bin/env python3
"""Standalone analyzer for SingularityVR input / Jump / haptic stall signatures.

Parses view_matrix.log and reports:
  - XInput poll rate and polls-per-frame spikes
  - XInput caller distribution
  - XR input phase peaks (sync, action states, poses, haptics)
  - Jump / melee edge-tagged FRAME SPIKE inventory
  - Haptic call frequency and cumulative cost (when diagnostics are enabled)

Usage:
    python analyze-input-perf-log.py path/to/view_matrix.log > report.md

No Steam, runtime, or synthetic FPS claims.
"""

import argparse
import re
import sys
from collections import defaultdict
from dataclasses import dataclass, field
from statistics import median
from typing import Dict, List, Optional, Tuple


@dataclass
class Budget:
    fps: float = 0.0
    our_work_ms: float = 0.0


@dataclass
class XInputStats:
    calls_per_sec: int = 0
    set_state_per_sec: int = 0
    packet_advances_per_sec: int = 0
    emulation: str = ""


@dataclass
class XrInputCost:
    sync_avg: float = 0.0
    sync_peak: float = 0.0
    action_avg: float = 0.0
    action_peak: float = 0.0
    pose_avg: float = 0.0
    pose_peak: float = 0.0
    haptic_avg: float = 0.0
    haptic_peak: float = 0.0
    samples: int = 0


@dataclass
class Spike:
    line: int
    frame_ms: float
    jump_age_ms: int
    melee_age_ms: int
    tags: List[str] = field(default_factory=list)


@dataclass
class HapticDiag:
    calls: int = 0
    apply_calls: int = 0
    stop_calls: int = 0
    total_us: int = 0


def parse_budget(line: str) -> Optional[Budget]:
    m = re.search(
        r"fps median\s+([0-9.]+)\s+our work\s+([0-9.]+)\s+ms", line
    )
    if not m:
        return None
    return Budget(fps=float(m.group(1)), our_work_ms=float(m.group(2)))


def parse_xinput(line: str) -> Optional[XInputStats]:
    m = re.search(
        r"xinput:\s+(\d+)\s+calls/sec.*?packet advances/sec\s+(\d+).*?SetState/sec\s+(\d+).*?emulation\s+(\w+)",
        line,
    )
    if not m:
        return None
    return XInputStats(
        calls_per_sec=int(m.group(1)),
        packet_advances_per_sec=int(m.group(2)),
        set_state_per_sec=int(m.group(3)),
        emulation=m.group(4),
    )


def parse_xinput_callers(line: str) -> List[Tuple[str, int]]:
    out: List[Tuple[str, int]] = []
    for m in re.finditer(r"0x([0-9A-Fa-f]+)\s+x(\d+)", line):
        out.append((m.group(1), int(m.group(2))))
    return out


def parse_xrinput_cost(line: str) -> Optional[XrInputCost]:
    m = re.search(
        r"xrinput cost:\s+xrSyncActions\s+([0-9.]+)\s+ms\s+\(peak\s+([0-9.]+)\)\s+\|\s+action states\s+([0-9.]+)\s+\(peak\s+([0-9.]+)\)\s+\|\s+hand poses\s+([0-9.]+)\s+\(peak\s+([0-9.]+)\)\s+\|\s+haptics\s+([0-9.]+)\s+\(peak\s+([0-9.]+)\).*?(\d+)\s*frames",
        line,
    )
    if not m:
        return None
    return XrInputCost(
        sync_avg=float(m.group(1)),
        sync_peak=float(m.group(2)),
        action_avg=float(m.group(3)),
        action_peak=float(m.group(4)),
        pose_avg=float(m.group(5)),
        pose_peak=float(m.group(6)),
        haptic_avg=float(m.group(7)),
        haptic_peak=float(m.group(8)),
        samples=int(m.group(9)),
    )


def parse_spike(line: str, line_no: int) -> Optional[Spike]:
    m = re.search(
        r"FRAME SPIKE\s+([0-9.]+)\s+ms.*?meleeAge\s+(-?\d+)\s+jumpAge\s+(-?\d+)", line
    )
    if not m:
        return None
    s = Spike(
        line=line_no,
        frame_ms=float(m.group(1)),
        jump_age_ms=int(m.group(3)),
        melee_age_ms=int(m.group(2)),
    )
    if s.jump_age_ms >= 0 and s.jump_age_ms < 500:
        s.tags.append("jump-recent")
    if s.melee_age_ms >= 0 and s.melee_age_ms < 500:
        s.tags.append("melee-recent")
    return s


def parse_haptic_diag(line: str) -> Optional[HapticDiag]:
    # hapticCalls= N apply= N stop= N us= N
    m = re.search(
        r"hapticCalls[=:]\s*(\d+).*?apply[=:]\s*(\d+).*?stop[=:]\s*(\d+).*?(\d+)\s*us",
        line,
        re.IGNORECASE,
    )
    if not m:
        return None
    return HapticDiag(
        calls=int(m.group(1)),
        apply_calls=int(m.group(2)),
        stop_calls=int(m.group(3)),
        total_us=int(m.group(4)),
    )


@dataclass
class InputReport:
    budgets: List[Budget] = field(default_factory=list)
    xinputs: List[XInputStats] = field(default_factory=list)
    callers: List[Tuple[str, int]] = field(default_factory=list)
    xr_costs: List[XrInputCost] = field(default_factory=list)
    spikes: List[Spike] = field(default_factory=list)
    haptic_diags: List[HapticDiag] = field(default_factory=list)


def parse_log(text: str) -> InputReport:
    r = InputReport()
    for line_no, line in enumerate(text.splitlines(), start=1):
        b = parse_budget(line)
        if b:
            r.budgets.append(b)
            continue
        xi = parse_xinput(line)
        if xi:
            r.xinputs.append(xi)
            continue
        if "xinput callers" in line:
            r.callers.extend(parse_xinput_callers(line))
            continue
        xr = parse_xrinput_cost(line)
        if xr:
            r.xr_costs.append(xr)
            continue
        sp = parse_spike(line, line_no)
        if sp:
            r.spikes.append(sp)
            continue
        hd = parse_haptic_diag(line)
        if hd:
            r.haptic_diags.append(hd)
    return r


def polls_per_frame(xi: XInputStats, budgets: List[Budget]) -> float:
    if not budgets:
        return 0.0
    fps = median([b.fps for b in budgets]) if budgets else 120.0
    return xi.calls_per_sec / fps if fps > 0 else 0.0


def render(r: InputReport) -> str:
    lines: List[str] = []
    lines.append("# SingularityVR input / Jump / haptic stall analysis")
    lines.append("")
    lines.append("Generated by `tools/analyze-input-perf-log.py` from existing `view_matrix.log` lines.")
    lines.append("This is a research aid; it does not run Steam or claim synthetic FPS improvements.")
    lines.append("")

    if r.budgets:
        fps_vals = [b.fps for b in r.budgets]
        our_vals = [b.our_work_ms for b in r.budgets]
        lines.append("## Frame budget summary")
        lines.append("")
        lines.append(f"- Budget windows: {len(r.budgets)}")
        lines.append(f"- Median FPS: {median(fps_vals):.1f}")
        lines.append(f"- Median 'our work': {median(our_vals):.2f} ms")
        lines.append("")

    if r.xinputs:
        lines.append("## XInput polling")
        lines.append("")
        lines.append("| calls/sec | polls/frame | packet adv/sec | SetState/sec | emulation |")
        lines.append("|---:|---:|---:|---:|---|")
        for xi in r.xinputs:
            ppf = polls_per_frame(xi, r.budgets)
            lines.append(
                f"| {xi.calls_per_sec} | {ppf:.2f} | {xi.packet_advances_per_sec} | {xi.set_state_per_sec} | {xi.emulation} |"
            )
        lines.append("")
        max_ppf = max((polls_per_frame(xi, r.budgets) for xi in r.xinputs), default=0.0)
        if max_ppf > 1.5:
            lines.append(f"- **WARNING**: max polls/frame = {max_ppf:.2f}. Engine input path is running multiple times per frame.")
        else:
            lines.append(f"- Max polls/frame = {max_ppf:.2f}; XInput poll count is in normal range.")
        lines.append("")

    if r.callers:
        lines.append("## XInput caller distribution")
        lines.append("")
        lines.append("| caller | count |")
        lines.append("|---|---:|")
        total = sum(c for _, c in r.callers)
        for addr, count in sorted(r.callers, key=lambda kv: kv[1], reverse=True):
            pct = 100.0 * count / total if total else 0.0
            lines.append(f"| 0x{addr} | {count} ({pct:.1f}%) |")
        lines.append("")
        if len(r.callers) == 1:
            lines.append("- Single caller dominating: likely one engine site polling in a loop.")
        elif len(r.callers) > 1:
            lines.append("- Multiple callers: engine has several independent XInput consumers.")
        lines.append("")

    if r.xr_costs:
        lines.append("## OpenXR input phase peaks")
        lines.append("")
        lines.append("| sync peak | action peak | pose peak | haptic peak | samples |")
        lines.append("|---|---:|---:|---:|---:|")
        for xr in r.xr_costs:
            lines.append(
                f"| {xr.sync_peak:.3f} | {xr.action_peak:.3f} | {xr.pose_peak:.3f} | {xr.haptic_peak:.3f} | {xr.samples} |"
            )
        lines.append("")
        max_haptic = max(xr.haptic_peak for xr in r.xr_costs)
        if max_haptic > 0.5:
            lines.append(f"- **WARNING**: haptic peak = {max_haptic:.3f} ms. Runtime haptic calls may be stalling.")
        else:
            lines.append(f"- Haptic peak = {max_haptic:.3f} ms; within normal range.")
        lines.append("")

    if r.spikes:
        lines.append("## Jump / melee edge-tagged spikes")
        lines.append("")
        lines.append(f"- Spike count: {len(r.spikes)}")
        tag_counts: Dict[str, int] = defaultdict(int)
        for s in r.spikes:
            for t in s.tags:
                tag_counts[t] += 1
        if tag_counts:
            lines.append("- Tags:")
            for tag, count in sorted(tag_counts.items(), key=lambda kv: kv[1], reverse=True):
                lines.append(f"  - `{tag}`: {count}")
        lines.append("")
        lines.append("| frame ms | jump age | melee age | tags |")
        lines.append("|---:|---:|---:|---|")
        for s in r.spikes:
            tags = ", ".join(s.tags) or "-"
            lines.append(f"| {s.frame_ms:.1f} | {s.jump_age_ms} | {s.melee_age_ms} | {tags} |")
        lines.append("")

    if r.haptic_diags:
        total_calls = sum(h.calls for h in r.haptic_diags)
        total_apply = sum(h.apply_calls for h in r.haptic_diags)
        total_stop = sum(h.stop_calls for h in r.haptic_diags)
        total_us = sum(h.total_us for h in r.haptic_diags)
        lines.append("## Haptic diagnostics")
        lines.append("")
        lines.append(f"- Total haptic calls: {total_calls}")
        lines.append(f"- Apply calls: {total_apply}")
        lines.append(f"- Stop calls: {total_stop}")
        if total_calls:
            lines.append(f"- Average call cost: {total_us / total_calls:.1f} µs")
        lines.append("")

    lines.append("## Interpretation")
    lines.append("")
    lines.append("- **XInput polls/frame >> 1** means the engine input path is looping; the caller table names the suspect.")
    lines.append("- **Haptic peak >> 0.5 ms** or high µs/frame suggests runtime haptic submission is the stall.")
    lines.append("- **`jump-recent` / `melee-recent` spikes** point to input-edge triggers; check logs and haptic bursts at those moments.")
    lines.append("- If both XInput poll spikes and haptic peaks are low, the stall is likely outside input code.")
    lines.append("")
    return "\n".join(lines)


def main() -> int:
    parser = argparse.ArgumentParser(description="Analyze SingularityVR input/Jump/haptic log")
    parser.add_argument("log", help="path to view_matrix.log")
    args = parser.parse_args()

    try:
        with open(args.log, "r", encoding="utf-8", errors="ignore") as f:
            text = f.read()
    except FileNotFoundError:
        print(f"error: file not found: {args.log}", file=sys.stderr)
        return 1

    print(render(parse_log(text)))
    return 0


if __name__ == "__main__":
    sys.exit(main())
