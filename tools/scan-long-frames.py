#!/usr/bin/env python3
"""Scan input_diagnostic logs for frames with max_us or phase residual > threshold."""
import re, sys, glob, os
from collections import Counter

THRESH = 250000
edge_re = re.compile(
    r"BUTTON EDGE frame=(\d+) mode=(\S+) rising=0x([0-9A-Fa-f]+) falling=0x([0-9A-Fa-f]+).*published=0x([0-9A-Fa-f]+).*max_us=(\d+) max_frame=(\d+)"
)
phase_re = re.compile(
    r"phase frame=(\d+) present=(\d+).*haptic=(\d+).*xiGet=(\d+)/(\d+)/(\d+).*residual=(-?\d+)"
)

def bits(mask):
    names = []
    table = {0x0001:"DPAD_UP",0x0002:"DPAD_DOWN",0x0004:"DPAD_LEFT",0x0008:"DPAD_RIGHT",
             0x0010:"START",0x1000:"A",0x2000:"B",0x4000:"X",0x8000:"Y",
             0x0100:"LSHOULDER",0x0200:"RSHOULDER",0x0040:"LTHUMB",0x0080:"RTHUMB"}
    for b,n in table.items():
        if mask & b: names.append(n)
    return "+".join(names) if names else "none"

def scan(path):
    edges = []
    phases = {}
    modes = []
    with open(path, encoding="utf-8", errors="replace") as f:
        for line in f:
            if line.startswith("DIAG MODE"):
                modes.append(line.strip())
            m = edge_re.search(line)
            if m:
                edges.append(m.groups())
            p = phase_re.search(line)
            if p:
                phases[int(p.group(1))] = p.groups()
    long_edges = []
    for fr, mode, rising, falling, pub, mx, mxf in edges:
        if int(mx) >= THRESH:
            long_edges.append((int(fr), mode, int(rising,16), int(falling,16), int(pub,16), int(mx), int(mxf)))
    return modes, long_edges, phases

def main():
    root = sys.argv[1] if len(sys.argv)>1 else os.path.expandvars(r"%LOCALAPPDATA%\SingularityVR")
    files = sorted(glob.glob(os.path.join(root, "input_diagnostic_*.log")))
    print(f"# Long-frame scan (>={THRESH/1000:.0f} ms) across {len(files)} logs\n")
    corr = Counter()
    for path in files:
        modes, longs, phases = scan(path)
        if not longs:
            continue
        print(f"## {os.path.basename(path)} modes={len(modes)} long_edges={len(longs)}")
        seen = set()
        for fr, mode, rising, falling, pub, mx, mxf in longs:
            key = (mxf, mx)
            if key in seen:
                continue
            seen.add(key)
            ph = phases.get(mxf)
            res = int(ph[6]) if ph else None
            delay = mxf - fr
            label = bits(rising) if rising else ("fall:"+bits(falling))
            print(f"- edge {fr} {mode} rising={bits(rising)} pub=0x{pub:04X} delay={delay} max={mx/1000:.1f}ms frame={mxf} residual_ms={(res or 0)/1000:.1f}")
            if mx >= 400000:
                if rising & 0x1000: corr["A_jump"] += 1
                elif rising & 0x2000: corr["B"] += 1
                elif rising & 0x0200: corr["RSHOULDER"] += 1
                elif rising: corr["other_rising"] += 1
                else: corr["falling_or_none"] += 1
        print()
    print("## >=400ms unique stall attribution (deduped by max frame)")
    for k,v in corr.most_common():
        print(f"- {k}: {v}")

if __name__ == "__main__":
    main()
