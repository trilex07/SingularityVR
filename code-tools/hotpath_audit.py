#!/usr/bin/env python3
"""SingularityVR static render hot-path auditor.

READ-ONLY: parses C/C++ sources, never edits them. Flags operations that are
potentially expensive when executed per draw call or per frame, so a human can
verify whether they actually run hot.

Usage:
    python hotpath_audit.py [--root DIR]... [--markdown out.md] [--min-severity LOW|MEDIUM|HIGH]

Exit code: 0 always (findings are leads, not failures), 2 on usage/IO error.
"""

import argparse
import os
import re
import sys

# Functions that run per draw call (or wrap every draw). Anything costly here
# multiplies by thousands of draws x2 eyes.
PER_DRAW = {
    "Hook_DrawPrim", "Hook_DrawIndexedPrim", "Hook_DrawPrimUP",
    "Hook_DrawIndexedPrimUP", "Hook_DrawPrimitive", "Hook_DrawIndexedPrimitive",
    "Hook_DrawPrimitiveUP", "Hook_DrawIndexedPrimitiveUP",
    "StereoPair", "StereoPairBody", "HudStereoPair",
}

# Functions that run per frame / per Present. Costly ops here multiply by ~90-120/s.
PER_FRAME = {
    "Hook_Present", "UpdateFromHeadset", "SyncXRInput",
    "Hook_SetTexture", "Hook_SetRenderTarget",
    "Hook_SetVS", "Hook_SetVSConstF",
    "Hook_SetVertexShader", "Hook_SetPixelShader",
    "Hook_SetVertexShaderConstantF",
    "Hook_SetViewport", "Hook_Clear",
    "CopyBackbufferToD3D11",
    "UpdateFgLatch", "RefreshArmedState",
    "ApplyMuzzleFxSuppression", "PrepareIndependentLeft",
}

# Helpers that must never run on a frame path (object-graph walks, syscalls).
NEVER_HOT = {
    "FindPlayerPawn", "FindCamera", "FindHudObject", "FindClassByName",
    "SolvePropertyLayout", "ResolveGameplayOffsets",
    "NameOf", "NameFromIndex", "Readable",
}

# (category, compiled regex, severity, why-template)
PATTERNS = [
    ("com-get-state",
     re.compile(r"->\s*(GetRenderTarget|GetDepthStencilSurface|GetBackBuffer)\s*\("),
     "HIGH",
     "D3D9 Get* call on the device: vtable hop plus driver validation, and on "
     "some drivers it stalls the pipeline. Per draw it also AddRefs the "
     "returned surface, doubling the cost with the matching Release."),
    ("com-get-resource",
     re.compile(r"->\s*(GetTexture|GetSurfaceLevel|GetVolumeLevel|GetStreamSource|GetIndices|GetVertexShader|GetPixelShader)\s*\("),
     "MEDIUM",
     "COM getter: cheap in isolation, but each call is an interlocked AddRef "
     "on the returned object plus validation. Per draw x2 eyes it adds up; "
     "cache the pointer instead of re-querying."),
    ("com-getdesc",
     re.compile(r"->\s*GetDesc\s*\("),
     "MEDIUM",
     "GetDesc copies a descriptor struct through a COM call. Harmless once, "
     "wasteful per draw; hoist the size/format out of the hot path."),
    ("com-get-constants",
     re.compile(r"->\s*Get(Vertex|Pixel)ShaderConstantF\s*\("),
     "HIGH",
     "Reading shader constants back from the device per draw forces a "
     "CPU-side shadow copy lookup at best and a driver round-trip at worst. "
     "The shadow-pass dump in StereoPairBody is correctly one-shot gated; "
     "any UNGATED occurrence is a bug."),
    ("com-refcount",
     re.compile(r"->\s*(AddRef|Release)\s*\("),
     "MEDIUM",
     "Interlocked refcount: takes the cache line exclusively. One pair per "
     "draw at thousands of draws/frame is cross-thread line bouncing against "
     "the render thread. Usually paired with a com-get above - fix the getter "
     "and this goes away too."),
    ("com-qi",
     re.compile(r"->\s*QueryInterface\s*\("),
     "HIGH",
     "QueryInterface does GUID comparison plus validation plus AddRef. Never "
     "belongs on a per-draw path; resolve once and cache the interface."),
    ("shader-ctab",
     re.compile(r"(GetFunctionDesc|GetConstantTable|D3DXGetShaderConstantTable|GetConstantDesc|CTAB|ctab)",
                        ),
     "HIGH",
     "Shader reflection parses bytecode tables. Milliseconds, not "
     "microseconds - per draw it would dominate the frame. Must run at "
     "shader-bind or load time only."),
    ("lock",
     re.compile(r"(EnterCriticalSection|WaitForSingleObject|WaitForMultipleObjects|std::mutex|std::lock_guard|AcquireSRWLock|EnterCritical)"),
     "MEDIUM",
     "Blocking lock on a render path risks priority inversion and serialises "
     "the two eye draws. Prefer the file's Rd()/plain-word pattern for "
     "reads; keep interlocked ops for writes only."),
    ("interlocked-hot",
     re.compile(r"Interlocked(Increment|Decrement|Exchange|Add|CompareExchange)"),
     "LOW",
     "A single interlocked op is ~10-20ns, NOT a bug by itself. It matters "
     "only in aggregate: per draw x2 eyes x thousands of draws against a "
     "word Present also writes = cache-line fight (see the run-203 note on "
     "Rd()). Listed so the aggregate can be eyeballed, not as a defect."),
    ("heap-alloc",
     re.compile(r"(?<![A-Za-z0-9_:=])\b(new\s+\w|new\s*\[|delete\s*\[?(?!\s*;)|malloc\s*\(|calloc\s*\(|free\s*\(|HeapAlloc)"),
     "HIGH",
     "Heap allocation takes the process heap lock and can page. Per draw it "
     "fragments and serialises; use stack/fixed storage (the hotpath_sampler "
     "pattern) instead. (`= delete` declarations are excluded.)"),
    ("string-alloc",
     re.compile(r"std::string\s+\w+\s*[=(]|std::wstring\s+\w+\s*[=(]|\+\s*std::string|append\s*\(\s*\""),
     "MEDIUM",
     "std::string construction typically heap-allocates. Per draw this is a "
     "hidden malloc per call; prefer fixed char buffers (the file already "
     "does this for log lines)."),
    ("container-insert",
     re.compile(r"\.(push_back|push_front|emplace_back|emplace|insert)\s*\("),
     "MEDIUM",
     "Container insertion can reallocate (vector) or allocate per node "
     "(map). Per draw prefer pre-sized/fixed tables (the census hash "
     "tables are the good example)."),
    ("file-io",
     re.compile(r"\b(fopen|fwrite|fprintf|vfprintf|fputc|fclose|fopen_s|std::ofstream|CreateFileA?|WriteFile)\s*\("),
     "HIGH",
     "File I/O is milliseconds (open/write/close syscalls). The run-202 "
     "incident (120 fps -> 11 fps from a per-frame Log) proves this class. "
     "Log() here is async-batched, but the enqueue still costs per draw."),
    ("log-call",
     re.compile(r"(?<![A-Za-z0-9_:])Log\s*\("),
     "HIGH",
     "Log() formats AND enqueues under contention. Rate-limited/one-shot "
     "calls are fine; any Log reachable per draw is a repeat of run-202. "
     "Verify the call site is gated before dismissing."),
    ("format",
     re.compile(r"(snprintf|sprintf|_snprintf|_snwprintf|vsnprintf|swprintf)\s*\("),
     "MEDIUM",
     "printf-style formatting is hundreds of ns with locale/float handling. "
     "Fine for one-shot diagnostics; per draw it rivals the draw itself. "
     "Pre-format outside the hook."),
    ("timing-qpc",
     re.compile(r"(QueryPerformanceCounter|QueryPerformanceFrequency|GetTickCount64?\s*\(\s*\)|Now\s*\(\s*\)|MsSince\s*\()"),
     "LOW",
     "QPC is ~20-50ns - free for per-frame use, and the file deliberately "
     "times StereoPair with it. Listed only because per-DRAW pairs (start+end "
     "x4000 draws) reach ~0.5ms/frame; keep at most one pair per draw."),
    ("virtualquery",
     re.compile(r"\b(VirtualQuery|Readable\s*\()"),
     "HIGH",
     "VirtualQuery is a kernel transition (~microseconds). The run-35/256 "
     "incidents (hundreds of thousands of syscalls freezing menus) are this "
     "class. Must never run per draw; the 8-slot region cache only bounds "
     "walks, it does not make them hot-path safe."),
    ("object-walk",
     re.compile(r"\b(FindPlayerPawn|FindCamera|FindHudObject|FindController|FindClassByName|NameOf\s*\(|ScanAllowed|GObjects)"),
     "HIGH",
     "GObjects walks are tens of thousands of objects with syscalls per "
     "object. The file already gates these behind backoffs and once-a-frame "
     "resolves; any NEW call from a draw hook reintroduces run-35."),
    ("openxr-call",
     re.compile(r"\b(xrWaitFrame|xrBeginFrame|xrEndFrame|xrLocateViews|xrLocateSpace|xrSyncActions|xrGetActionState\w*|xrEnumerate\w*|xrPollEvent)\s*\("),
     "MEDIUM",
     "OpenXR runtime calls cross into the runtime and may block (WaitFrame) "
     "or cost microseconds (locate/sync). Correct per FRAME from "
     "UpdateFromHeadset; HIGH severity if found inside a per-DRAW hook - "
     "check the containing function."),
    ("xr-submit",
     re.compile(r"\b(xrEndFrame|SubmitFrame|CopyBackbufferToD3D11|GetRenderTargetData)\s*\("),
     "MEDIUM",
     "Submission/copy path: the measured black box (run-56). Timing it is "
     "good; duplicating the copy or reading back the target per draw is not."),
    ("ik-solve",
     re.compile(r"\b(Solve\w*IK|TwoBoneIK|ArmIK|SolveArm|ClampChain|IterateJoints|acosf?\s*\(|atan2f?\s*\()"),
     "MEDIUM",
     "IK/trig per draw is unusual - normally once per frame per limb. "
     "atan2/acos are tens of ns each; fine per frame, suspect per draw. "
     "Verify call frequency before acting."),
    ("menu-classify",
     re.compile(r"\b(Menu\w*Classif\w*|ClassifyMenu|IsMenu\w*|MenuInset|GFx\w*|HudStereo|HudArm\w*)\b"),
     "MEDIUM",
     "Menu/UI classification runs per user-pointer draw. String compares or "
     "texture queries here multiply by the UI draw count; keep to integer "
     "tag compares on the hot path."),
    ("shadow-classify",
     re.compile(r"\b(Shadow\w*Classif\w*|IsShadow\w*|shadow\s*pass|ShadowMap\w*|g_boundShaderLayout\.shadow)\b"),
     "LOW",
     "Shadow classification itself is usually a flag check (cheap). Flagged "
     "so reviewers can confirm no surface/texture queries hide behind it "
     "per draw."),
    ("sleep",
     re.compile(r"\b(Sleep\s*\(|SleepEx\s*\()"),
     "HIGH",
     "Sleep on a render/input thread surrenders the quantum (ms). The file "
     "hooks Sleep precisely because UE3's smoother sleeps the frame; any "
     "ADDITIONAL sleep on this path compounds it."),
    ("device-state",
     re.compile(r"->\s*(SetScissorRect|SetViewport|SetRenderState|SetVertexShaderConstantF|SetPixelShaderConstantF|SetTexture|SetStreamSource|SetIndices)\s*\("),
     "LOW",
     "Device state uploads are the MECHANISM of StereoPair (per-eye "
     "re-upload is intended). Listed only for completeness: an EXTRA "
     "redundant upload per draw (same values twice) is the only actionable "
     "sub-case."),
]

FUNC_DEF = re.compile(
    r"^(?:template\s*<[^;{}]*>\s*)?(?:[\w:\*&<>\s]+?)\b([A-Za-z_]\w*(?:::[A-Za-z_]\w*)?)\s*\($"
    r"|^([A-Za-z_]\w*(?:::[A-Za-z_]\w*)?)\s*\([^;{}]*\)\s*(?:\{)?\s*$"
)
SKIP_PREFIX = ("if ", "if(", "for ", "for(", "while ", "while(", "switch ",
               "switch(", "catch ", "catch(", "return ", "return(",
               "else", "typedef ", "using ", "#", "*", "/", ":", "}", "{",
               "case ", "default:", "public:", "private:", "protected:")
SKIP_FUNC_NAMES = {"if", "for", "while", "switch", "catch", "return", "else",
                   "sizeof", "decltype", "static_assert", "TIME_HOOK"}


def _strip_comment(line):
    idx = line.find("//")
    if idx >= 0:
        return line[:idx]
    return line


def collect_functions(lines):
    """Return list of (name, start_1based, end_1based) via brace tracking.

    Handles multi-line signatures (STDMETHODCALLTYPE hooks, wrapped arg
    lists) by balancing parens first, then requiring the opening brace
    before any semicolon.
    """
    funcs = []
    n = len(lines)
    i = 0
    while i < n:
        code = _strip_comment(lines[i]).strip()
        if (not code or code.startswith(SKIP_PREFIX)
                or "(" not in code or ";" in code.split("(", 1)[0]):
            i += 1
            continue
        m = re.match(
            r"^(?:template\s*<[^;{}]*>\s*)?(?:[\w:\*&<>\s]+?)?"
            r"\b([A-Za-z_]\w*(?:::[A-Za-z_]\w*)?)\s*\(", code)
        if not m:
            i += 1
            continue
        name = m.group(1).split(":")[-1]
        if name in SKIP_FUNC_NAMES:
            i += 1
            continue
        # Balance parens across lines starting at the match.
        depth = 0
        j = i
        closed_at = None
        closed_col = 0
        stop = min(n, i + 25)
        while j < stop:
            seg = _strip_comment(lines[j])
            start_col = seg.find("(", seg.find(name) if j == i else 0) if j == i else 0
            for col in range(start_col if j == i else 0, len(seg)):
                ch = seg[col]
                if ch == "(":
                    depth += 1
                elif ch == ")":
                    depth -= 1
                    if depth == 0 and closed_at is None:
                        closed_at = j
                        closed_col = col
                        break
                elif ch == ";" and depth == 0 and j == i and col < seg.find("("):
                    break
            if closed_at is not None:
                break
            j += 1
        if closed_at is None:
            i += 1
            continue
        # After the closing paren: optional qualifiers, then '{' (definition)
        # or ';' (declaration/prototype/call). Search boundedly.
        tail = _strip_comment(lines[closed_at])[closed_col + 1:]
        k = closed_at
        is_def = False
        brace_line = None
        while k < min(n, closed_at + 8):
            probe = tail if k == closed_at else _strip_comment(lines[k])
            probe_s = probe.strip()
            if probe_s.startswith(";"):
                break
            b = probe.find("{")
            s = probe.find(";")
            if b >= 0 and (s < 0 or b < s):
                is_def = True
                brace_line = k
                break
            if s >= 0:
                break
            if re.search(r"\b(class|struct|enum)\b", probe_s):
                break
            k += 1
        if not is_def:
            i += 1
            continue
        # Brace-match to the end of the body.
        depth = 0
        started = False
        k = brace_line
        while k < n:
            seg = _strip_comment(lines[k])
            depth += seg.count("{") - seg.count("}")
            if "{" in seg:
                started = True
            if started and depth <= 0:
                funcs.append((name, i + 1, k + 1))
                break
            k += 1
            if k - brace_line > 30000:
                break
        i = (k + 1) if k < n else (brace_line + 1)
    return funcs


def function_at(funcs, lineno):
    best = None
    for name, s, e in funcs:
        if s <= lineno <= e:
            if best is None or (s >= best[1]):
                best = (name, s, e)
    return best[0] if best else "<global>"


def classify_frequency(func):
    if func in PER_DRAW:
        return "per-draw (x1000s/frame)"
    if func in PER_FRAME:
        return "per-frame (~90-120/s)"
    if func in NEVER_HOT:
        return "MUST STAY COLD (walk/syscall helper)"
    return "helper (verify call path)"


def severity_for(category, sev, func):
    # OpenXR calls inside a per-draw hook escalate: they belong per frame.
    if category == "openxr-call" and func in PER_DRAW:
        return "HIGH"
    # QPC inside per-draw stays LOW (deliberate instrumentation).
    return sev


def audit_file(path):
    with open(path, encoding="utf-8", errors="replace") as f:
        lines = f.readlines()
    funcs = collect_functions(lines)
    findings = []
    for idx, line in enumerate(lines, start=1):
        stripped = line.strip()
        if stripped.startswith("//") or stripped.startswith("*"):
            continue
        code = _strip_comment(line)
        if "= delete" in code or "= default" in code:
            continue
        for category, rx, sev, why in PATTERNS:
            m = rx.search(code)
            if m:
                func = function_at(funcs, idx)
                findings.append({
                    "file": path,
                    "line": idx,
                    "func": func,
                    "category": category,
                    "match": m.group(0).strip()[:80],
                    "frequency": classify_frequency(func),
                    "severity": severity_for(category, sev, func),
                    "why": why,
                })
    return findings


def main(argv=None):
    ap = argparse.ArgumentParser(description="Static render hot-path auditor (read-only).")
    ap.add_argument("--root", action="append", default=[],
                    help="Source root or file to scan (repeatable). Default: spikes/view_matrix.")
    ap.add_argument("--include", default="*.cpp,*.h",
                    help="Comma-separated globs for --root dirs (default: *.cpp,*.h).")
    ap.add_argument("--markdown", default=None,
                    help="Also write a Markdown report to this path.")
    ap.add_argument("--min-severity", default="LOW", choices=["LOW", "MEDIUM", "HIGH"],
                    help="Minimum severity to display (default: LOW).")
    ap.add_argument("--repo", default=None,
                    help="Repo/worktree root used to resolve the default scan dir.")
    args = ap.parse_args(argv)

    import fnmatch

    if not args.root:
        base = args.repo or os.path.join(os.path.dirname(os.path.abspath(__file__)), "..")
        default_dir = os.path.join(base, "spikes", "view_matrix")
        args.root = [default_dir] if os.path.isdir(default_dir) else [base]

    globs = [g.strip() for g in args.include.split(",") if g.strip()]
    files = []
    for root in args.root:
        if os.path.isfile(root):
            files.append(root)
        elif os.path.isdir(root):
            for dirpath, _dirs, names in os.walk(root):
                if "tests" in dirpath.split(os.sep):
                    continue
                for nm in names:
                    if any(fnmatch.fnmatch(nm, g) for g in globs):
                        files.append(os.path.join(dirpath, nm))
        else:
            print("warning: root not found: %s" % root, file=sys.stderr)

    if not files:
        print("error: no files to scan", file=sys.stderr)
        return 2

    findings = []
    for path in sorted(files):
        try:
            findings.extend(audit_file(path))
        except OSError as e:
            print("warning: cannot read %s: %s" % (path, e), file=sys.stderr)

    rank = {"HIGH": 0, "MEDIUM": 1, "LOW": 2}
    min_rank = rank[args.min_severity]
    shown = [f for f in findings if rank[f["severity"]] <= min_rank]
    shown.sort(key=lambda f: (rank[f["severity"]], f["file"], f["line"]))

    by_sev = {"HIGH": 0, "MEDIUM": 0, "LOW": 0}
    for f in findings:
        by_sev[f["severity"]] += 1
    by_cat = {}
    for f in findings:
        by_cat[f["category"]] = by_cat.get(f["category"], 0) + 1

    print("=" * 70)
    print("HOT-PATH AUDIT (static, read-only) - %d file(s), %d finding(s)"
          % (len(files), len(findings)))
    print("HIGH=%d  MEDIUM=%d  LOW=%d" % (by_sev["HIGH"], by_sev["MEDIUM"], by_sev["LOW"]))
    print("=" * 70)
    print("Findings are LEADS, not proven bugs. A COM call outside a draw hook")
    print("is normal; the same call inside StereoPair/Hook_DrawPrim is not.")
    print("-" * 70)
    print("by category:")
    for cat in sorted(by_cat, key=lambda c: -by_cat[c]):
        print("  %-18s %d" % (cat, by_cat[cat]))
    print("-" * 70)
    cur = None
    for f in shown:
        if f["severity"] != cur:
            cur = f["severity"]
            print("== %s ==" % cur)
        rel = os.path.basename(f["file"])
        print("%s:%d  [%s] %s :: %s (%s) -- %s"
              % (rel, f["line"], f["func"], f["match"], f["category"],
                 f["frequency"], f["severity"]))
    print("-" * 70)
    print("WHY (per category):")
    seen = set()
    for _cat, _rx, _sev, why in PATTERNS:
        if _cat in by_cat and _cat not in seen:
            seen.add(_cat)
            print("  [%s] %s" % (_cat, why))

    if args.markdown:
        with open(args.markdown, "w", encoding="utf-8") as out:
            out.write("# Hot-path audit\n\n");
            out.write("Static, read-only. %d file(s), %d finding(s): " % (len(files), len(findings)))
            out.write("HIGH=%d MEDIUM=%d LOW=%d.\n\n" % (by_sev["HIGH"], by_sev["MEDIUM"], by_sev["LOW"]))
            out.write("> Findings are leads, not proven bugs. Verify the call path and "
                      "frequency before acting.\n\n")
            out.write("| file | line | function | match | category | frequency | severity |\n")
            out.write("|---|---|---|---|---|---|---|\n")
            for f in shown:
                out.write("| %s | %d | %s | `%s` | %s | %s | %s |\n" % (
                    os.path.basename(f["file"]), f["line"], f["func"],
                    f["match"].replace("|", "\\|"), f["category"],
                    f["frequency"], f["severity"]))
            out.write("\n## Rationale per category\n\n")
            done = set()
            for cat, _rx, _sev, why in PATTERNS:
                if cat in by_cat and cat not in done:
                    done.add(cat)
                    out.write("### %s\n\n%s\n\n" % (cat, why))
        print("markdown report: %s" % args.markdown)
    return 0


if __name__ == "__main__":
    sys.exit(main())
