#!/usr/bin/env python3
"""SingularityVR performance session comparator.

Compares two session logs (baseline vs candidate). Tolerant parser: any field
that is absent is reported as N/A instead of failing.

Understood line families (current and future logs):
  perf: <fps> fps (<ms> ms/frame) | draws/frame peak <n>, split <a> (<b> with
      parallax, <c> flat), offscreen <d>, mono-fallback <e>, user-ptr <f> ...
  frame budget: xrWaitFrame <w> ... + copy <c> + GObjects walk <g> + XR submit
      <s> + reg scan <r> + gameplay hooks <h> + our work <o> = <t> ms
  pad ... : <fps> fps | residual <r> | MISSED <p>% ... | <d> draws/frame |
      <n> frames [wait <w>, submit <s>]
  [1] SLOW frames (...) : <n> fr | wall <w> | CPU <c> (user <u> kernel <k>)
  PerformanceBuild1 GPU XR-copy span: samples=<n> avg_ms=<a> max_ms=<m> ...
  PerformanceBuild1 real D3D9 Present: frames=<n> avg_ms=<a> max_ms=<m>
  hotpath sampler complete: ... fam[calls=<c> samples=<s> avg_us=<a>
      est_ms/frame=<e] ... present-hook residual estimate=<r> ms/frame
  xinput: <n> calls/sec ... / xinput callers (hook total <t> ms/sec)
  generic: <label>[:=] ... avg_ms=<a> ... / frames=<n> avg_ms=<a> max_ms=<m>

Usage:
    python perf_compare.py baseline.log candidate.log [--markdown r.md] [--csv r.csv]

Exit code: 0 always (a regression is a finding, not a tool failure),
2 on usage/IO error.
"""

import argparse
import csv
import math
import os
import re
import sys

# ---------------------------------------------------------------- patterns

RE_PERF = re.compile(
    r"perf:\s*(?P<fps>[\d.]+)\s*fps\s*\((?P<ms>[\d.]+)\s*ms/frame\)"
    r"(?:\s*\|\s*draws/frame\s*peak\s*(?P<draws>\d+))?"
    r"(?:,\s*split\s*(?P<split>\d+))?"
    r"(?:\s*\((?P<par>\d+)\s*with parallax,\s*(?P<flat>\d+)\s*flat\))?"
    r"(?:,\s*offscreen\s*(?P<off>\d+))?"
    r"(?:,\s*mono-fallback\s*(?P<mono>\d+))?"
    r"(?:,\s*user-ptr\s*(?P<up>\d+))?")
RE_BUDGET = re.compile(
    r"frame budget:\s*xrWaitFrame\s*(?P<wait>[\d.]+).*?"
    r"\+\s*copy\s*(?P<copy>[\d.]+).*?"
    r"\+\s*GObjects walk\s*(?P<gobj>[\d.]+).*?"
    r"\+\s*XR submit\s*(?P<submit>[\d.]+).*?"
    r"\+\s*reg scan\s*(?P<reg>[\d.]+).*?"
    r"\+\s*gameplay hooks\s*(?P<gplay>[\d.]+).*?"
    r"\+\s*our work\s*(?P<our>[\d.]+)\s*=\s*(?P<total>[\d.]+)\s*ms")
RE_PAD = re.compile(
    r"^\s*pad\s+(?P<padon>ON|OFF)\s*,\s*feed\s+(?P<feed>ON|OFF)\s*,\s*controllers\s*(?P<ctl>[^:]*):\s*"
    r"(?P<fps>[\d.]+)\s*fps\s*\|\s*residual\s*(?P<resid>[\d.]+).*?"
    r"MISSED\s*(?P<missed>[\d.]+)%.*?\(\s*worst\s*(?P<worst>[\d.]+)\s*ms\).*?"
    r"\|\s*(?P<draws>[\d.]+)\s*draws/frame\s*\|\s*(?P<frames>\d+)\s*frames\s*"
    r"\[wait\s*(?P<wait>[\d.]+),\s*submit\s*(?P<submit>[\d.]+)\]")
RE_SLOW = re.compile(
    r"SLOW frames[^\n:]*:\s*(?P<frames>\d+)\s*fr\s*\|\s*wall\s*(?P<wall>[\d.]+)"
    r"\s*\|\s*CPU\s*(?P<cpu>[\d.]+)\s*\(user\s*(?P<user>[\d.]+)\s*kernel\s*(?P<kern>[\d.]+)\)")
RE_COPYSPAN = re.compile(
    r"GPU XR-copy span:\s*samples=(?P<n>\d+)\s*avg_ms=(?P<avg>[\d.]+)\s*max_ms=(?P<max>[\d.]+)")
RE_PRESENT = re.compile(
    r"real D3D9 Present:\s*frames=(?P<n>\d+)\s*avg_ms=(?P<avg>[\d.]+)\s*max_ms=(?P<max>[\d.]+)")
RE_SAMPLER_FAM = re.compile(
    r"(?P<fam>[A-Za-z_][\w]*)\[calls=(?P<calls>\d+)\s+"
    r"samples=(?P<samples>\d+)\s+avg_us=(?P<avgus>[\d.]+)\s+"
    r"est_ms/frame=(?P<msframe>[\d.]+)\]")
RE_SAMPLER_WIN = re.compile(r"hotpath sampler complete:\s*window=(?P<win>\d+)\s*frames")
RE_RESIDUAL = re.compile(r"present-hook residual estimate=(?P<r>[\d.]+)\s*ms/frame")
RE_XINPUT = re.compile(r"xinput:\s*(?P<cps>\d+)\s*calls/sec")
RE_XIHOOK = re.compile(r"hook total\s*(?P<t>[\d.]+)\s*ms/sec")
RE_GENERIC_AVG = re.compile(
    r"(?P<label>[A-Za-z][\w .:/-]{2,60}?)\s*[:=]\s*[^:\n]*?\bavg_ms=(?P<avg>[\d.]+)"
    r"(?:\s*max_ms=(?P<max>[\d.]+))?")
RE_GENERIC_FRAMES = re.compile(
    r"(?P<label>[A-Za-z][\w .:/-]{2,60}?):\s*frames=(?P<n>\d+)\s*avg_ms=(?P<avg>[\d.]+)"
    r"(?:\s*max_ms=(?P<max>[\d.]+))?")

SAMPLER_FAMILIES = ("draw", "menu", "foreground", "shadow", "ik", "palette",
                    "set_texture", "other_set", "wait_frame", "pose_input",
                    "d3d_copy", "xr_images", "xr_submit", "present_frame")

BUDGET_FIELDS = (("xrWaitFrame", "wait"), ("copy", "copy"),
                 ("GObjects walk", "gobj"), ("XR submit", "submit"),
                 ("reg scan", "reg"), ("gameplay hooks", "gplay"),
                 ("our work", "our"), ("frame total", "total"))


# ---------------------------------------------------------------- parsing

def new_session(name):
    return {
        "name": name, "lines": 0,
        "perf_fps": [], "perf_ms": [], "perf_draws": [], "perf_split": [],
        "perf_mono": [], "perf_off": [], "perf_up": [],
        "budget": {k: [] for _k, k in BUDGET_FIELDS},
        "pad_rows": [],
        "slow_wall": [], "slow_cpu": [], "slow_user": [], "slow_kern": [],
        "copyspan_avg": [], "copyspan_max": [],
        "present_avg": [], "present_max": [],
        "sampler_win": None, "sampler_fam": {}, "residual": [],
        "xinput_cps": [], "xihook_ms": [],
        "generic": {},
    }


def parse_file(path):
    sess = new_session(os.path.basename(path))
    with open(path, encoding="utf-8", errors="replace") as f:
        for raw in f:
            sess["lines"] += 1
            try:
                line = raw.strip()
                if not line:
                    continue
                m = RE_PERF.search(line)
                if m:
                    if m.group("fps"):
                        sess["perf_fps"].append(float(m.group("fps")))
                    if m.group("ms"):
                        sess["perf_ms"].append(float(m.group("ms")))
                    for dst, grp in (("perf_draws", "draws"), ("perf_split", "split"),
                                     ("perf_mono", "mono"), ("perf_off", "off"),
                                     ("perf_up", "up")):
                        if m.group(grp):
                            sess[dst].append(float(m.group(grp)))
                    continue
                m = RE_BUDGET.search(line)
                if m:
                    for _label, key in BUDGET_FIELDS:
                        if m.group(key):
                            sess["budget"][key].append(float(m.group(key)))
                    continue
                m = RE_PAD.search(line)
                if m:
                    sess["pad_rows"].append({
                        "pad": m.group("padon"), "feed": m.group("feed"),
                        "ctl": m.group("ctl").strip(),
                        "fps": float(m.group("fps")), "resid": float(m.group("resid")),
                        "missed": float(m.group("missed")), "worst": float(m.group("worst")),
                        "draws": float(m.group("draws")), "frames": int(m.group("frames")),
                        "wait": float(m.group("wait")), "submit": float(m.group("submit")),
                    })
                    continue
                m = RE_SLOW.search(line)
                if m:
                    sess["slow_wall"].append(float(m.group("wall")))
                    sess["slow_cpu"].append(float(m.group("cpu")))
                    sess["slow_user"].append(float(m.group("user")))
                    sess["slow_kern"].append(float(m.group("kern")))
                    continue
                m = RE_COPYSPAN.search(line)
                if m:
                    sess["copyspan_avg"].append(float(m.group("avg")))
                    sess["copyspan_max"].append(float(m.group("max")))
                    continue
                m = RE_PRESENT.search(line)
                if m:
                    sess["present_avg"].append(float(m.group("avg")))
                    sess["present_max"].append(float(m.group("max")))
                    continue
                if "hotpath sampler complete" in line:
                    w = RE_SAMPLER_WIN.search(line)
                    if w:
                        sess["sampler_win"] = int(w.group("win"))
                    for fm in RE_SAMPLER_FAM.finditer(line):
                        fam = fm.group("fam")
                        if fam not in SAMPLER_FAMILIES and fam != "TrackFpCaster":
                            continue
                        sess["sampler_fam"][fam] = {
                            "calls": int(fm.group("calls")),
                            "samples": int(fm.group("samples")),
                            "avg_us": float(fm.group("avgus")),
                            "ms_frame": float(fm.group("msframe")),
                        }
                    r = RE_RESIDUAL.search(line)
                    if r:
                        sess["residual"].append(float(r.group("r")))
                    continue
                m = RE_XINPUT.search(line)
                if m:
                    sess["xinput_cps"].append(float(m.group("cps")))
                    h = RE_XIHOOK.search(line)
                    if h:
                        sess["xihook_ms"].append(float(h.group("t")))
                    continue
                m = RE_GENERIC_FRAMES.search(line)
                if m and "hotpath sampler" not in line and "GPU XR-copy" not in line \
                        and "D3D9 Present" not in line:
                    sess["generic"].setdefault(m.group("label").strip(), []).append(
                        float(m.group("avg")))
            except (ValueError, IndexError):
                continue  # tolerant: skip malformed values, keep going
    return sess


# ---------------------------------------------------------------- stats

def percentile(sorted_vals, pct):
    if not sorted_vals:
        return None
    if len(sorted_vals) == 1:
        return sorted_vals[0]
    k = (len(sorted_vals) - 1) * (pct / 100.0)
    lo = int(math.floor(k))
    hi = int(math.ceil(k))
    if lo == hi:
        return sorted_vals[lo]
    return sorted_vals[lo] + (sorted_vals[hi] - sorted_vals[lo]) * (k - lo)


def stats(vals):
    if not vals:
        return None
    s = sorted(vals)
    n = len(s)
    mean = sum(s) / n
    med = (s[n // 2] if n % 2 else (s[n // 2 - 1] + s[n // 2]) / 2.0)
    return {"n": n, "avg": mean, "median": med, "p95": percentile(s, 95),
            "p99": percentile(s, 99), "min": s[0], "max": s[-1]}


def fmt(v, unit=""):
    if v is None or (isinstance(v, float) and (math.isnan(v) or math.isinf(v))):
        return "N/A"
    if isinstance(v, float):
        return "%8.3f%s" % (v, unit)
    return "%8s%s" % (v, unit)


def delta_str(b, c, unit="", invert=False):
    """Return (abs_str, pct_str, worse_bool_or_None)."""
    if b is None or c is None:
        return "N/A", "N/A", None
    d = c - b
    pct = (d / b * 100.0) if b != 0 else (float("inf") if d != 0 else 0.0)
    if math.isinf(pct):
        pct_s = "+inf%" if d > 0 else "-inf%"
    else:
        pct_s = "%+.1f%%" % pct
    worse = (d > 0) if not invert else (d < 0)
    # fps: higher is better -> invert
    return "%+.3f%s" % (d, unit), pct_s, worse


# ---------------------------------------------------------------- report

SERIES = [
    ("frame time", "perf_ms", " ms", False),
    ("fps", "perf_fps", "", True),
    ("draws/frame (peak)", "perf_draws", "", False),
    ("split draws", "perf_split", "", False),
    ("mono-fallback draws", "perf_mono", "", False),
    ("offscreen draws", "perf_off", "", False),
    ("user-ptr draws", "perf_up", "", False),
    ("SLOW wall ms", "slow_wall", " ms", False),
    ("SLOW CPU ms", "slow_cpu", " ms", False),
    ("XR-copy span avg_ms", "copyspan_avg", " ms", False),
    ("D3D9 Present avg_ms", "present_avg", " ms", False),
    ("present residual est.", "residual", " ms", False),
    ("xinput calls/sec", "xinput_cps", "/s", False),
]

BUDGET_LABELS = {"wait": "xrWaitFrame", "copy": "copy", "gobj": "GObjects walk",
                 "submit": "XR submit", "reg": "reg scan", "gplay": "gameplay hooks",
                 "our": "our work", "total": "frame total"}


def build_rows(base, cand):
    rows = []  # (metric, b_n, b_avg, c_avg, d_abs, d_pct, worse)
    for label, key, unit, invert in SERIES:
        sb, sc = stats(base[key]), stats(cand[key])
        b_avg = sb["avg"] if sb else None
        c_avg = sc["avg"] if sc else None
        da, dp, w = delta_str(b_avg, c_avg, unit, invert)
        d_num = (c_avg - b_avg) if (b_avg is not None and c_avg is not None) else None
        rows.append({"metric": label, "unit": unit,
                     "b_n": sb["n"] if sb else 0, "c_n": sc["n"] if sc else 0,
                     "b_avg": b_avg, "c_avg": c_avg, "d_num": d_num,
                     "b_med": sb["median"] if sb else None,
                     "c_med": sc["median"] if sc else None,
                     "b_p95": sb["p95"] if sb else None,
                     "c_p95": sc["p95"] if sc else None,
                     "d_abs": da, "d_pct": dp, "worse": w})
    for _label, key in BUDGET_FIELDS:
        sb, sc = stats(base["budget"][key]), stats(cand["budget"][key])
        b_avg = sb["avg"] if sb else None
        c_avg = sc["avg"] if sc else None
        da, dp, w = delta_str(b_avg, c_avg, " ms", False)
        d_num = (c_avg - b_avg) if (b_avg is not None and c_avg is not None) else None
        rows.append({"metric": "budget " + BUDGET_LABELS[key], "unit": " ms",
                     "d_num": d_num,
                     "b_n": sb["n"] if sb else 0, "c_n": sc["n"] if sc else 0,
                     "b_avg": b_avg, "c_avg": c_avg,
                     "b_med": sb["median"] if sb else None,
                     "c_med": sc["median"] if sc else None,
                     "b_p95": sb["p95"] if sb else None,
                     "c_p95": sc["p95"] if sc else None,
                     "d_abs": da, "d_pct": dp, "worse": w})
    return rows


def sampler_table(base, cand):
    """Per-family calls/frame, ms/frame, avg us/call for both sessions."""
    out = []
    bw = base["sampler_win"] or 0
    cw = cand["sampler_win"] or 0
    for fam in SAMPLER_FAMILIES:
        b = base["sampler_fam"].get(fam)
        c = cand["sampler_fam"].get(fam)
        if b is None and c is None:
            continue
        b_cpf = (b["calls"] / bw) if (b and bw) else None
        c_cpf = (c["calls"] / cw) if (c and cw) else None
        b_ms = b["ms_frame"] if b else None
        c_ms = c["ms_frame"] if c else None
        da, dp, w = delta_str(b_ms, c_ms, " ms", False)
        out.append({"family": fam,
                    "b_calls_frame": b_cpf, "c_calls_frame": c_cpf,
                    "b_ms_frame": b_ms, "c_ms_frame": c_ms,
                    "b_avg_us": b["avg_us"] if b else None,
                    "c_avg_us": c["avg_us"] if c else None,
                    "d_abs": da, "d_pct": dp, "worse": w})
    return out


def ranking(sampler_rows):
    ranked = [r for r in sampler_rows
              if r["b_ms_frame"] is not None and r["c_ms_frame"] is not None]
    for r in ranked:
        r["delta"] = r["c_ms_frame"] - r["b_ms_frame"]
    ranked.sort(key=lambda r: r["delta"], reverse=True)
    return ranked


def print_report(base, cand, rows, samp_rows, ranked):
    W = 72
    print("=" * W)
    print("PERF COMPARE: %s  vs  %s" % (base["name"], cand["name"]))
    print("lines: %d vs %d" % (base["lines"], cand["lines"]))
    print("sampler window: %s vs %s frames"
          % (base["sampler_win"] or "N/A", cand["sampler_win"] or "N/A"))
    print("absent fields print as N/A and never fail the comparison.")
    print("=" * W)

    print("--- frame/budget metrics (avg; median/p95 in detail rows) ---")
    print("%-24s %9s %9s %10s %9s  %s"
          % ("metric", "base avg", "cand avg", "delta", "delta%", "n(base/cand)"))
    for r in rows:
        flag = ""
        if r["worse"] is True:
            flag = "  <-- WORSE"
        elif (r["worse"] is False and r["d_num"] not in (None, 0.0)
                and r["b_avg"] is not None):
            flag = "  (better)"
        print("%-24s %9s %9s %10s %9s  %d/%d%s"
              % (r["metric"], fmt(r["b_avg"]), fmt(r["c_avg"]),
                 r["d_abs"], r["d_pct"], r["b_n"], r["c_n"], flag))

    print("--- spread (median / p95) where both sides have data ---")
    for r in rows:
        if r["b_med"] is None or r["c_med"] is None:
            continue
        print("  %-22s med %s -> %s | p95 %s -> %s"
              % (r["metric"], fmt(r["b_med"]), fmt(r["c_med"]),
                 fmt(r["b_p95"]), fmt(r["c_p95"])))

    print("--- hook families: calls/frame, ms/frame, avg us/call ---")
    if not samp_rows:
        print("  N/A - neither log contains a hotpath sampler window")
    else:
        print("%-14s %10s %10s %10s %10s %9s %9s"
              % ("family", "b call/f", "c call/f", "b ms/f", "c ms/f",
                 "b us/c", "c us/c"))
        for r in samp_rows:
            print("%-14s %10s %10s %10s %10s %9s %9s   d=%s (%s)"
                  % (r["family"], fmt(r["b_calls_frame"]), fmt(r["c_calls_frame"]),
                     fmt(r["b_ms_frame"]), fmt(r["c_ms_frame"]),
                     fmt(r["b_avg_us"]), fmt(r["c_avg_us"]),
                     r["d_abs"], r["d_pct"]))

    print("=" * W)
    print("PERFORMANCE REGRESSION RANKING (by delta ms/frame, candidate-base)")
    print("(Only families present in BOTH logs are ranked. Positive = regression.)")
    if not ranked:
        print("  N/A - not enough shared data to rank")
    else:
        for i, r in enumerate(ranked, 1):
            print("  %d. %s %+.4f ms/frame (%s -> %s ms/frame)"
                  % (i, r["family"], r["delta"],
                     fmt(r["b_ms_frame"]), fmt(r["c_ms_frame"])))
    print("=" * W)

    if base["pad_rows"] or cand["pad_rows"]:
        print("--- pad-state rows (context only, not ranked) ---")
        for tag, sess in (("BASE", base), ("CAND", cand)):
            for p in sess["pad_rows"][-4:]:
                print("  [%s] pad=%s feed=%s ctl=%s fps=%s resid=%s missed=%s%% draws=%s frames=%d"
                      % (tag, p["pad"], p["feed"], p["ctl"], fmt(p["fps"]),
                         fmt(p["resid"]), fmt(p["missed"]), fmt(p["draws"]), p["frames"]))
    if base["generic"] or cand["generic"]:
        print("--- generic avg_ms metrics ---")
        for label in sorted(set(base["generic"]) | set(cand["generic"])):
            sb, sc = stats(base["generic"].get(label, [])), stats(cand["generic"].get(label, []))
            b = sb["avg"] if sb else None
            c = sc["avg"] if sc else None
            da, dp, _w = delta_str(b, c, " ms", False)
            print("  %-40s base=%s cand=%s d=%s (%s)"
                  % (label[:40], fmt(b), fmt(c), da, dp))


def write_markdown(path, base, cand, rows, samp_rows, ranked):
    with open(path, "w", encoding="utf-8") as out:
        out.write("# Performance comparison\n\n")
        out.write("baseline `%s` (%d lines) vs candidate `%s` (%d lines).\n\n"
                  % (base["name"], base["lines"], cand["name"], cand["lines"]))
        out.write("Absent fields are N/A.\n\n")
        out.write("| metric | base avg | cand avg | delta | delta% | n base/cand |\n")
        out.write("|---|---|---|---|---|---|\n")
        for r in rows:
            out.write("| %s | %s | %s | %s | %s | %d/%d |\n"
                      % (r["metric"], fmt(r["b_avg"]).strip(), fmt(r["c_avg"]).strip(),
                         r["d_abs"], r["d_pct"], r["b_n"], r["c_n"]))
        out.write("\n## Hook families\n\n")
        if not samp_rows:
            out.write("N/A - no sampler window in either log.\n\n")
        else:
            out.write("| family | base calls/frame | cand calls/frame | base ms/frame | "
                      "cand ms/frame | base us/call | cand us/call | delta | delta% |\n")
            out.write("|---|---|---|---|---|---|---|---|---|\n")
            for r in samp_rows:
                out.write("| %s | %s | %s | %s | %s | %s | %s | %s | %s |\n"
                          % (r["family"], fmt(r["b_calls_frame"]).strip(),
                             fmt(r["c_calls_frame"]).strip(), fmt(r["b_ms_frame"]).strip(),
                             fmt(r["c_ms_frame"]).strip(), fmt(r["b_avg_us"]).strip(),
                             fmt(r["c_avg_us"]).strip(), r["d_abs"], r["d_pct"]))
        out.write("\n## Regression ranking\n\n")
        if not ranked:
            out.write("N/A - not enough shared data.\n")
        else:
            for i, r in enumerate(ranked, 1):
                out.write("%d. **%s** %+.4f ms/frame\n" % (i, r["family"], r["delta"]))
    print("markdown report: %s" % path)


def write_csv(path, rows, samp_rows):
    with open(path, "w", encoding="utf-8", newline="") as f:
        w = csv.writer(f)
        w.writerow(["section", "metric", "base_avg", "cand_avg", "base_med",
                    "cand_med", "base_p95", "cand_p95", "delta_abs", "delta_pct",
                    "n_base", "n_cand"])
        for r in rows:
            w.writerow(["series", r["metric"], r["b_avg"], r["c_avg"], r["b_med"],
                        r["c_med"], r["b_p95"], r["c_p95"], r["d_abs"], r["d_pct"],
                        r["b_n"], r["c_n"]])
        for r in samp_rows:
            w.writerow(["family:" + r["family"], "calls/frame",
                        r["b_calls_frame"], r["c_calls_frame"], "", "", "", "",
                        "", r["d_pct"], "", ""])
            w.writerow(["family:" + r["family"], "ms/frame",
                        r["b_ms_frame"], r["c_ms_frame"], "", "", "", "",
                        r["d_abs"], r["d_pct"], "", ""])
    print("csv report: %s" % path)


def main(argv=None):
    ap = argparse.ArgumentParser(description="Compare two SingularityVR session logs.")
    ap.add_argument("baseline", help="Baseline log file")
    ap.add_argument("candidate", help="Candidate log file")
    ap.add_argument("--markdown", default=None, help="Write Markdown report to PATH")
    ap.add_argument("--csv", default=None, help="Write CSV report to PATH")
    args = ap.parse_args(argv)

    for p in (args.baseline, args.candidate):
        if not os.path.isfile(p):
            print("error: file not found: %s" % p, file=sys.stderr)
            return 2
    base = parse_file(args.baseline)
    cand = parse_file(args.candidate)
    rows = build_rows(base, cand)
    samp_rows = sampler_table(base, cand)
    ranked = ranking(samp_rows)
    print_report(base, cand, rows, samp_rows, ranked)
    if args.markdown:
        write_markdown(args.markdown, base, cand, rows, samp_rows, ranked)
    if args.csv:
        write_csv(args.csv, rows, samp_rows)
    return 0


if __name__ == "__main__":
    sys.exit(main())
