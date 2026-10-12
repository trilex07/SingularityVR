#!/usr/bin/env python3
"""Analyzer for SingularityVR input_diagnostic_*.log files.

Extracts button edges, frame timings, and per-phase breakdowns to help root-cause
Jump-related delayed stalls. Produces a Markdown timeline report.

Usage:
    python analyze-input-diagnostic-log.py input_diagnostic_YYYYMMDD_HHMMSS.log > report.md

No Steam or runtime required; this is a read-only parser.
"""

import argparse
import re
import sys
from dataclasses import dataclass, field
from statistics import median
from typing import Dict, List, Optional


@dataclass
class PhaseBreakdown:
    frame: int = 0
    present_us: int = 0
    wait_us: int = 0
    begin_us: int = 0
    sync_us: int = 0
    actions_us: int = 0
    poses_us: int = 0
    end_us: int = 0
    bridge_us: int = 0
    acquire_us: int = 0
    swapwait_us: int = 0
    release_us: int = 0
    haptic_us: int = 0
    xi_get_calls: int = 0
    xi_get_total_us: int = 0
    xi_get_max_us: int = 0
    xi_set_calls: int = 0
    xi_set_total_us: int = 0
    xi_set_max_us: int = 0
    trigger_l: int = 0
    trigger_r: int = 0
    rumble_l: int = 0
    rumble_r: int = 0
    haptic_calls: int = 0
    haptic_apply_calls: int = 0
    haptic_stop_calls: int = 0
    haptic_call_us: int = 0
    haptic_amplitude_milli: int = 0
    reset_count: int = 0
    swapchain_count: int = 0
    residual: int = 0

    def largest_phase(self) -> tuple:
        """Return (phase_name, microseconds) for the largest measured phase."""
        phases = [
            ("wait", self.wait_us),
            ("begin", self.begin_us),
            ("sync", self.sync_us),
            ("actions", self.actions_us),
            ("poses", self.poses_us),
            ("end", self.end_us),
            ("bridge", self.bridge_us),
            ("acquire", self.acquire_us),
            ("swapwait", self.swapwait_us),
            ("release", self.release_us),
            ("haptic", self.haptic_us),
        ]
        return max(phases, key=lambda x: x[1])


@dataclass
class ButtonEdge:
    frame: int = 0
    mode: str = ""
    rising: int = 0
    falling: int = 0
    held: int = 0
    published: int = 0
    baseline_avg_us: int = 0
    edge_us: int = 0
    plus1_us: int = 0
    plus2_us: int = 0
    max_us: int = 0
    max_frame: int = 0
    eye: int = 0
    phase_edge: Optional[PhaseBreakdown] = None
    phase_max: Optional[PhaseBreakdown] = None


@dataclass
class LongStall:
    frame: int = 0
    mode: str = ""
    thread: int = 0
    frame_us: int = 0
    present_us: int = 0
    residual_us: int = 0
    thread_cpu_us: int = 0
    wait_us: int = 0
    end_us: int = 0
    swapwait_us: int = 0
    haptic_us: int = 0
    xi_get_calls: int = 0
    xi_get_total_us: int = 0
    trigger_l: int = 0
    trigger_r: int = 0
    edges90: str = ""

    def cpu_vs_wait_hint(self) -> str:
        """Interpret Present-thread CPU vs residual (4e58bdb+ logs only)."""
        if self.thread_cpu_us <= 0 and self.residual_us > 250000:
            return "no thread_cpu_us (pre-4e58bdb log or field missing)"
        if self.residual_us <= 0:
            return "n/a"
        ratio = self.thread_cpu_us / float(self.residual_us)
        if ratio < 0.25:
            return "Present thread mostly idle/waiting during residual (CPU ≪ residual)"
        if ratio > 0.75:
            return "Present thread CPU ≈ residual (busy on Present thread; use ETW for call stacks)"
        return "mixed (partial Present-thread CPU during residual)"


@dataclass
class InputTiming:
    mode: str = ""
    frames: int = 0
    avg_us: int = 0
    p95_us: int = 0
    max_us: int = 0
    present_avg_us: int = 0
    xr_avg_us: int = 0
    bridge_avg_us: int = 0
    xinput_avg_us: int = 0
    lifecycle_violations: int = 0


def parse_phase(line: str) -> Optional[PhaseBreakdown]:
    m = re.search(
        r"phase\s+frame=(\d+)\s+present=(\d+)\s+wait=(\d+)\s+begin=(\d+)\s+sync=(\d+)\s+actions=(\d+)\s+poses=(\d+)\s+end=(\d+)\s+bridge=(\d+)\s+acquire=(\d+)\s+swapwait=(\d+)\s+release=(\d+)\s+haptic=(\d+)\s+xiGet=(\d+)/(\d+)/(\d+)\s+xiSet=(\d+)/(\d+)/(\d+)\s+trig=(\d+)/(\d+)\s+rumble=(\d+)/(\d+)\s+hapticCalls=(\d+)\s+apply=(\d+)\s+stop=(\d+)\s+apiUs=(\d+)\s+ampMilli=(\d+)\s+reset=(\d+)\s+swapchain=(\d+)\s+residual=(-?\d+)",
        line,
    )
    if not m:
        return None
    g = m.groups()
    p = PhaseBreakdown()
    p.frame = int(g[0])
    p.present_us = int(g[1])
    p.wait_us = int(g[2])
    p.begin_us = int(g[3])
    p.sync_us = int(g[4])
    p.actions_us = int(g[5])
    p.poses_us = int(g[6])
    p.end_us = int(g[7])
    p.bridge_us = int(g[8])
    p.acquire_us = int(g[9])
    p.swapwait_us = int(g[10])
    p.release_us = int(g[11])
    p.haptic_us = int(g[12])
    p.xi_get_calls = int(g[13])
    p.xi_get_total_us = int(g[14])
    p.xi_get_max_us = int(g[15])
    p.xi_set_calls = int(g[16])
    p.xi_set_total_us = int(g[17])
    p.xi_set_max_us = int(g[18])
    p.trigger_l = int(g[19])
    p.trigger_r = int(g[20])
    p.rumble_l = int(g[21])
    p.rumble_r = int(g[22])
    p.haptic_calls = int(g[23])
    p.haptic_apply_calls = int(g[24])
    p.haptic_stop_calls = int(g[25])
    p.haptic_call_us = int(g[26])
    p.haptic_amplitude_milli = int(g[27])
    p.reset_count = int(g[28])
    p.swapchain_count = int(g[29])
    p.residual = int(g[30])
    return p


def parse_button_edge(line: str) -> Optional[ButtonEdge]:
    m = re.search(
        r"BUTTON EDGE\s+frame=(\d+)\s+mode=(\S+)\s+rising=0x([0-9A-Fa-f]+)\s+falling=0x([0-9A-Fa-f]+)\s+held=0x([0-9A-Fa-f]+)\s+published=0x([0-9A-Fa-f]+)\s+baseline_avg_us=(\d+)\s+edge_us=(\d+)\s+plus1_us=(\d+)\s+plus2_us=(\d+)\s+max_us=(\d+)\s+max_frame=(\d+)\s+eye=(\d+)",
        line,
    )
    if not m:
        return None
    g = m.groups()
    return ButtonEdge(
        frame=int(g[0]),
        mode=g[1],
        rising=int(g[2], 16),
        falling=int(g[3], 16),
        held=int(g[4], 16),
        published=int(g[5], 16),
        baseline_avg_us=int(g[6]),
        edge_us=int(g[7]),
        plus1_us=int(g[8]),
        plus2_us=int(g[9]),
        max_us=int(g[10]),
        max_frame=int(g[11]),
        eye=int(g[12]),
    )


def parse_longstall(line: str) -> Optional[LongStall]:
    m = re.search(
        r"LONGSTALL\s+frame=(\d+)\s+mode=(\S+)\s+thread=(\d+)\s+frame_us=(\d+)\s+present_us=(\d+)\s+residual_us=(-?\d+)\s+thread_cpu_us=(\d+)\s+wait=(\d+)\s+end=(\d+)\s+swapwait=(\d+)\s+haptic=(\d+)\s+xiGet=(\d+)/(\d+)\s+trig=(\d+)/(\d+)\s+edges90=(.*)",
        line,
    )
    if not m:
        return None
    g = m.groups()
    return LongStall(
        frame=int(g[0]),
        mode=g[1],
        thread=int(g[2]),
        frame_us=int(g[3]),
        present_us=int(g[4]),
        residual_us=int(g[5]),
        thread_cpu_us=int(g[6]),
        wait_us=int(g[7]),
        end_us=int(g[8]),
        swapwait_us=int(g[9]),
        haptic_us=int(g[10]),
        xi_get_calls=int(g[11]),
        xi_get_total_us=int(g[12]),
        trigger_l=int(g[13]),
        trigger_r=int(g[14]),
        edges90=g[15].strip(),
    )


def parse_input_timing(line: str) -> Optional[InputTiming]:
    m = re.search(
        r"InputTiming\s+mode=(\S+)\s+frames=(\d+)\s+avg_us=(\d+)\s+p95_us=(\d+)\s+max_us=(\d+)\s+present_avg_us=(\d+)\s+xr_avg_us=(\d+)\s+bridge_avg_us=(\d+)\s+xinput_avg_us=(\d+)\s+lifecycleViolations=(\d+)",
        line,
    )
    if not m:
        return None
    g = m.groups()
    return InputTiming(
        mode=g[0],
        frames=int(g[1]),
        avg_us=int(g[2]),
        p95_us=int(g[3]),
        max_us=int(g[4]),
        present_avg_us=int(g[5]),
        xr_avg_us=int(g[6]),
        bridge_avg_us=int(g[7]),
        xinput_avg_us=int(g[8]),
        lifecycle_violations=int(g[9]),
    )


def button_name(bit: int) -> str:
    names = {
        0x0001: "DPAD_UP",
        0x0002: "DPAD_DOWN",
        0x0004: "DPAD_LEFT",
        0x0008: "DPAD_RIGHT",
        0x0010: "START",
        0x0020: "BACK",
        0x0040: "LTHUMB",
        0x0080: "RTHUMB",
        0x0100: "LSHOULDER",
        0x0200: "RSHOULDER",
        0x1000: "A (Jump)",
        0x2000: "B",
        0x4000: "X",
        0x8000: "Y",
    }
    return names.get(bit, f"OTHER(0x{bit:04X})")


@dataclass
class DiagnosticLog:
    mode_changes: List[tuple] = field(default_factory=list)
    timings: List[InputTiming] = field(default_factory=list)
    edges: List[ButtonEdge] = field(default_factory=list)
    phases: Dict[int, PhaseBreakdown] = field(default_factory=dict)
    long_stalls: List[LongStall] = field(default_factory=list)


def parse_log(text: str) -> DiagnosticLog:
    log = DiagnosticLog()
    for line_no, line in enumerate(text.splitlines(), start=1):
        if line.startswith("DIAG MODE"):
            m = re.search(r"DIAG MODE\s+(\S+)\s+actionPolling=(\S+)", line)
            if m:
                log.mode_changes.append((line_no, m.group(1), m.group(2)))
            continue
        t = parse_input_timing(line)
        if t:
            log.timings.append(t)
            continue
        e = parse_button_edge(line)
        if e:
            log.edges.append(e)
            continue
        p = parse_phase(line)
        if p:
            log.phases[p.frame] = p
            continue
        ls = parse_longstall(line)
        if ls:
            log.long_stalls.append(ls)
            continue
    return log


def correlate(log: DiagnosticLog) -> None:
    """Attach phase breakdowns to button edges where available."""
    for e in log.edges:
        e.phase_edge = log.phases.get(e.frame)
        e.phase_max = log.phases.get(e.max_frame)


def render(log: DiagnosticLog, path: str) -> str:
    lines: List[str] = []
    lines.append("# Input Diagnostic Log Analysis")
    lines.append("")
    lines.append(f"Source: `{path}`")
    lines.append("")
    lines.append("This report is generated from an existing `input_diagnostic_*.log`. It does not run Steam or claim a root cause without measured evidence.")
    lines.append("")

    if log.mode_changes:
        lines.append("## Diagnostic mode changes")
        lines.append("")
        lines.append("| line | mode | action polling |")
        lines.append("|---:|---|---|")
        for line_no, mode, poll in log.mode_changes:
            lines.append(f"| {line_no} | {mode} | {poll} |")
        lines.append("")

    if log.long_stalls:
        lines.append("## LONGSTALL events (residual > 250 ms, 4e58bdb+ diagnostic builds)")
        lines.append("")
        lines.append("| frame | mode | frame ms | present ms | residual ms | thread CPU ms | hint | edges90 |")
        lines.append("|---:|---|---:|---:|---:|---:|---|---|")
        for ls in sorted(log.long_stalls, key=lambda x: x.residual_us, reverse=True):
            lines.append(
                f"| {ls.frame} | {ls.mode} | {ls.frame_us / 1000.0:.2f} | {ls.present_us / 1000.0:.2f} | "
                f"{ls.residual_us / 1000.0:.2f} | {ls.thread_cpu_us / 1000.0:.2f} | {ls.cpu_vs_wait_hint()} | {ls.edges90} |"
            )
        lines.append("")
        lines.append("- Compare **thread CPU ms** to **residual ms**: low ratio → wait/scheduling; high ratio → Present thread was executing (ETW still needed for UE3 attribution).")
        lines.append("")

    if log.timings:
        lines.append("## Rolling InputTiming summaries")
        lines.append("")
        lines.append("| mode | frames | avg ms | p95 ms | max ms | present avg ms | XR avg ms | bridge avg ms | XInput avg ms | lifecycle violations |")
        lines.append("|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|")
        for t in log.timings:
            lines.append(
                f"| {t.mode} | {t.frames} | {t.avg_us / 1000.0:.2f} | {t.p95_us / 1000.0:.2f} | {t.max_us / 1000.0:.2f} | "
                f"{t.present_avg_us / 1000.0:.2f} | {t.xr_avg_us / 1000.0:.2f} | {t.bridge_avg_us / 1000.0:.2f} | "
                f"{t.xinput_avg_us / 1000.0:.2f} | {t.lifecycle_violations} |"
            )
        lines.append("")

    # Find Jump edges and any large max_us following them.
    jump_edges = [e for e in log.edges if e.rising & 0x1000]
    if jump_edges:
        lines.append("## Jump-edge events")
        lines.append("")
        lines.append("| edge frame | mode | edge ms | +1 ms | +2 ms | max ms | max frame | delay to max | verdict |")
        lines.append("|---:|---|---:|---:|---:|---:|---:|---:|---|")
        for e in jump_edges:
            delay_frames = e.max_frame - e.frame
            spike_ms = e.max_us / 1000.0
            baseline_ms = e.baseline_avg_us / 1000.0
            if spike_ms > baseline_ms * 3 and spike_ms > 25.0:
                verdict = "STALL"
            elif spike_ms > baseline_ms * 1.5:
                verdict = "elevated"
            else:
                verdict = "ok"
            lines.append(
                f"| {e.frame} | {e.mode} | {e.edge_us / 1000.0:.2f} | {e.plus1_us / 1000.0:.2f} | "
                f"{e.plus2_us / 1000.0:.2f} | {spike_ms:.2f} | {e.max_frame} | {delay_frames} frames | {verdict} |"
            )
        lines.append("")

        # Detailed phase breakdown for the worst Jump stall per mode.
        lines.append("## Phase breakdown for worst Jump-associated stall per mode")
        lines.append("")
        worst_by_mode: Dict[str, ButtonEdge] = {}
        for e in jump_edges:
            if e.max_us > 25000:  # > 25 ms
                prev = worst_by_mode.get(e.mode)
                if prev is None or e.max_us > prev.max_us:
                    worst_by_mode[e.mode] = e

        if worst_by_mode:
            for mode, e in sorted(worst_by_mode.items(), key=lambda kv: kv[1].max_us, reverse=True):
                lines.append(f"### Mode: {mode}, edge frame {e.frame}, max frame {e.max_frame}")
                lines.append("")
                lines.append(f"- Delay: **{e.max_frame - e.frame} frames** after Jump")
                lines.append(f"- Max frame time: **{e.max_us / 1000.0:.2f} ms**")
                if e.phase_max:
                    p = e.phase_max
                    lines.append(f"- presentUs: {p.present_us / 1000.0:.2f} ms")
                    lines.append(f"- residual: {p.residual / 1000.0:.2f} ms")
                    phase_name, phase_us = p.largest_phase()
                    lines.append(f"- Largest phase: **{phase_name}** = {phase_us / 1000.0:.2f} ms")
                    lines.append("")
                    lines.append("| phase | µs | ms |")
                    lines.append("|---|---:|---:|")
                    lines.append(f"| wait | {p.wait_us} | {p.wait_us / 1000.0:.2f} |")
                    lines.append(f"| begin | {p.begin_us} | {p.begin_us / 1000.0:.2f} |")
                    lines.append(f"| sync | {p.sync_us} | {p.sync_us / 1000.0:.2f} |")
                    lines.append(f"| actions | {p.actions_us} | {p.actions_us / 1000.0:.2f} |")
                    lines.append(f"| poses | {p.poses_us} | {p.poses_us / 1000.0:.2f} |")
                    lines.append(f"| end | {p.end_us} | {p.end_us / 1000.0:.2f} |")
                    lines.append(f"| bridge | {p.bridge_us} | {p.bridge_us / 1000.0:.2f} |")
                    lines.append(f"| acquire | {p.acquire_us} | {p.acquire_us / 1000.0:.2f} |")
                    lines.append(f"| swapwait | {p.swapwait_us} | {p.swapwait_us / 1000.0:.2f} |")
                    lines.append(f"| release | {p.release_us} | {p.release_us / 1000.0:.2f} |")
                    lines.append(f"| haptic | {p.haptic_us} | {p.haptic_us / 1000.0:.2f} |")
                    lines.append("")
                    lines.append(f"- XInput: get calls={p.xi_get_calls}, get total µs={p.xi_get_total_us}, get max µs={p.xi_get_max_us}")
                    lines.append(f"- Haptics: calls={p.haptic_calls}, apply={p.haptic_apply_calls}, stop={p.haptic_stop_calls}, api µs={p.haptic_call_us}, ampMilli={p.haptic_amplitude_milli}")
                    lines.append("")
                    # Attributions
                    if p.residual > p.present_us * 2 and p.residual > 10000:
                        lines.append("- **Attribution hint:** large residual → stall is mostly **outside Hook_Present** (engine/game thread).")
                    elif p.haptic_us > p.present_us * 0.3 and p.haptic_us > 1000:
                        lines.append("- **Attribution hint:** large haptic phase → **runtime haptic stall**.")
                    elif p.swapwait_us > p.present_us * 0.3 and p.swapwait_us > 1000:
                        lines.append("- **Attribution hint:** large swapwait → **GPU/compositor wait**.")
                    elif p.xi_get_calls > 5:
                        lines.append("- **Attribution hint:** many XInput get calls → **XInput poll amplification**.")
                    else:
                        lines.append("- **Attribution hint:** inconclusive from phases alone.")
                    lines.append("")
                else:
                    lines.append("- No phase breakdown available for max frame.")
                    lines.append("")
        else:
            lines.append("No Jump-associated frames exceeded 25 ms.")
            lines.append("")

    # Mode comparison summary
    if jump_edges:
        lines.append("## Mode comparison summary")
        lines.append("")
        lines.append("| mode | Jump edges | stalls (>25 ms) | worst stall ms |")
        lines.append("|---|---:|---:|---:|")
        mode_summary: Dict[str, Dict] = {}
        for e in jump_edges:
            d = mode_summary.setdefault(e.mode, {"edges": 0, "stalls": 0, "worst": 0})
            d["edges"] += 1
            if e.max_us > 25000:
                d["stalls"] += 1
                if e.max_us > d["worst"]:
                    d["worst"] = e.max_us
        for mode, d in sorted(mode_summary.items()):
            lines.append(f"| {mode} | {d['edges']} | {d['stalls']} | {d['worst'] / 1000.0:.2f} |")
        lines.append("")
        lines.append("- If the worst stall is similar across `NORMAL`, `HAPTICS_OFF`, and `XINPUT_OFF`, the cause is likely engine-side (large residual).")
        lines.append("- If the stall disappears only in `HAPTICS_OFF`, suspect runtime haptics.")
        lines.append("- If the stall disappears only in `XINPUT_OFF`, suspect XInput poll amplification.")
        lines.append("- If the stall is present even in `BUTTONS_OFF`, it is not Jump button processing per se.")
        lines.append("")

    lines.append("## How to interpret this report")
    lines.append("")
    lines.append("1. Look for a Jump edge whose `max_us` is ~475 ms.")
    lines.append("2. Read the `delay to max` column: how many frames after Jump did the stall land?")
    lines.append("3. In the phase breakdown, compare `presentUs` vs `residual`:")
    lines.append("   - `residual` dominates → stall is outside the mod's Present hook (engine/game thread).")
    lines.append("   - `presentUs` dominates → stall is inside Present; inspect the largest phase.")
    lines.append("4. Compare the same Jump scenario across diagnostic modes to isolate input/haptic causality.")
    lines.append("")
    return "\n".join(lines)


def main() -> int:
    # Ensure UTF-8 output whether piped to a file or printed to a capable terminal.
    if hasattr(sys.stdout, "reconfigure"):
        sys.stdout.reconfigure(encoding="utf-8", errors="replace")
    parser = argparse.ArgumentParser(description="Analyze SingularityVR input_diagnostic log")
    parser.add_argument("log", help="path to input_diagnostic_*.log")
    args = parser.parse_args()

    try:
        with open(args.log, "r", encoding="utf-8", errors="ignore") as f:
            text = f.read()
    except FileNotFoundError:
        print(f"error: file not found: {args.log}", file=sys.stderr)
        return 1

    log = parse_log(text)
    correlate(log)
    print(render(log, args.log))
    return 0


if __name__ == "__main__":
    sys.exit(main())
