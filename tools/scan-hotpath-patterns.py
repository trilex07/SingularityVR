#!/usr/bin/env python3
"""Static analyzer for expensive patterns inside SingularityVR draw/hook hot paths.

Scans spikes/view_matrix/d3d9.cpp and reports, per hot-path function:
  - D3D9 Get* queries (GetRenderTarget, GetViewport, GetStreamSource, ...)
  - texture / surface desc queries
  - VirtualQuery / Readable / GObjects walks
  - unconditional Log() calls
  - heap allocations (new/malloc/vector)
  - interlocked operations (cheap but worth counting in tight loops)

The goal is to identify work that can be removed from hot paths without changing
behavior: caching, early-outs, or moving diagnostics off the critical path.

Usage:
    python scan-hotpath-patterns.py [path/to/d3d9.cpp]

Defaults to spikes/view_matrix/d3d9.cpp relative to the repo root.
"""

import argparse
import re
import sys
from collections import defaultdict
from dataclasses import dataclass, field
from pathlib import Path
from typing import Dict, List, Set


@dataclass
class FunctionScan:
    name: str
    line_start: int = 0
    d3d_gets: List[str] = field(default_factory=list)
    desc_queries: List[str] = field(default_factory=list)
    readable_walks: List[str] = field(default_factory=list)
    gobjects_walks: List[str] = field(default_factory=list)
    log_calls: List[str] = field(default_factory=list)
    allocations: List[str] = field(default_factory=list)
    interlocked_ops: int = 0
    hotpath_families: Set[str] = field(default_factory=set)


HOTPATH_FUNCS = [
    "Hook_DrawPrim",
    "Hook_DrawIndexedPrim",
    "Hook_DrawPrimUP",
    "Hook_DrawIndexedPrimUP",
    "Hook_SetTexture",
    "Hook_SetVertexShaderConstantF",
    "Hook_SetPixelShaderConstantF",
    "Hook_SetStreamSource",
    "Hook_SetIndices",
    "Hook_SetViewport",
    "Hook_SetScissorRect",
    "Hook_SetRenderState",
    "Hook_Present",
    "StereoPair",
    "HudStereoPair",
    "TrackFpCaster",
    "ApplyBonePalette",
    "SolveIkArms",
    "SuppressCapturedVrMenuDraw",
    "MenuDrawTraceRecord",
    "VidProbeRecord",
    "FireCensusRecord",
    # Frame-hook helpers called from Present
    "ResolveGameplayOffsets",
    "RefreshArmedState",
    "UpdateFgLatch",
    "FindPlayerPawn",
    "FindCamera",
    "SyncVrMenuCaptureState",
    "VidProbeFrameEnd",
    "ReportScan",
    "ApplyMuzzleFxSuppression",
    "ReportHotPathSampler",
]

D3D_GET_RE = re.compile(
    r"\b(GetRenderTarget|GetDepthStencilSurface|GetViewport|GetScissorRect|"
    r"GetVertexShader|GetPixelShader|GetVertexDeclaration|GetStreamSource|"
    r"GetIndices|GetTexture|GetVertexShaderConstantF|GetPixelShaderConstantF|"
    r"GetRenderState|GetSamplerState|GetTransform)\s*\("
)
DESC_QUERY_RE = re.compile(r"\b(GetDesc|GetLevelDesc|LockRect|UnlockRect)\s*\(")
READABLE_RE = re.compile(r"\bReadable\s*\(")
GOBJECTS_RE = re.compile(r"\b(GObjects|FindPlayerPawn|FindProcessInternal|ResolveScriptNames|"
                         r"WalkGObjects|GObjectIter)\b")
LOG_RE = re.compile(r"\bLog\s*\(")
ALLOC_RE = re.compile(r"\b(new\s+\w+|malloc\s*\(|calloc\s*\(|std::vector<|std::string\b)")
INTERLOCKED_RE = re.compile(r"\bInterlocked(?:Increment|Decrement|Exchange|CompareExchange|ExchangeAdd|Or|And)\b")
HOTPATH_SCOPE_RE = re.compile(r"hotpath::scope\s+\w+\s*\(\s*\w+\s*,\s*hotpath::Family::(\w+)\s*\)")


def find_functions(text: str) -> Dict[str, FunctionScan]:
    lines = text.splitlines()
    # Identify function bodies by a conservative heuristic.  Allow the return type
    # and attributes to span pieces; the key is the function name followed by '('.
    func_start_re = re.compile(
        r"^(?:\s*(?:static|inline|constexpr|HRESULT|STDMETHODCALLTYPE)\b\s*)*"
        r"(" + "|".join(re.escape(n) for n in HOTPATH_FUNCS) + r")\s*\("
    )

    funcs: Dict[str, FunctionScan] = {}
    current: List[FunctionScan] = []
    brace_depth = 0

    for i, raw in enumerate(lines, start=1):
        line = raw.strip()

        # Enter new function?
        m = func_start_re.match(line)
        if m and not current:
            name = m.group(1)
            fs = FunctionScan(name=name, line_start=i)
            funcs[name] = fs
            current.append(fs)
            brace_depth = line.count("{") - line.count("}")
            continue

        if not current:
            continue

        # Track braces naively; strings/comments may fool us but are rare enough.
        brace_depth += line.count("{") - line.count("}")

        # Skip comment-only lines.
        code = re.sub(r"//.*", "", line)

        fs = current[-1]

        # hotpath family tag
        hm = HOTPATH_SCOPE_RE.search(code)
        if hm:
            fs.hotpath_families.add(hm.group(1))

        for m in D3D_GET_RE.finditer(code):
            fs.d3d_gets.append(f"L{i}: {m.group(0)}")
        for m in DESC_QUERY_RE.finditer(code):
            fs.desc_queries.append(f"L{i}: {m.group(0)}")
        if READABLE_RE.search(code):
            fs.readable_walks.append(f"L{i}: Readable(...)")
        if GOBJECTS_RE.search(code):
            fs.gobjects_walks.append(f"L{i}: {GOBJECTS_RE.search(code).group(0)}")
        if LOG_RE.search(code):
            fs.log_calls.append(f"L{i}: Log(...)")
        for m in ALLOC_RE.finditer(code):
            fs.allocations.append(f"L{i}: {m.group(0).strip()}")
        fs.interlocked_ops += len(INTERLOCKED_RE.findall(code))

        if brace_depth <= 0:
            current.pop()
            brace_depth = 0

    return funcs


def score_function(fs: FunctionScan) -> int:
    """Higher is more suspect.  Weighted by likely cost, not exact timing."""
    return (
        len(fs.d3d_gets) * 3
        + len(fs.desc_queries) * 2
        + len(fs.readable_walks) * 20
        + len(fs.gobjects_walks) * 25
        + len(fs.log_calls) * 5
        + len(fs.allocations) * 10
        + fs.interlocked_ops
    )


def render(funcs: Dict[str, FunctionScan]) -> str:
    lines: List[str] = []
    lines.append("# Static hot-path scan: spikes/view_matrix/d3d9.cpp")
    lines.append("")
    lines.append("Generated by `tools/scan-hotpath-patterns.py`.  This is a static read-only")
    lines.append("analysis; it does not run Steam or make runtime FPS claims.")
    lines.append("")

    ranked = sorted(funcs.values(), key=score_function, reverse=True)
    lines.append("## Ranked by estimated hot-path risk")
    lines.append("")
    lines.append("| function | line | families | D3D Get | desc | Readable | GObjects | Log | alloc | Interlocked | score |")
    lines.append("|---|---|---|---:|---:|---:|---:|---:|---:|---:|---:|")
    for fs in ranked:
        fam = ", ".join(sorted(fs.hotpath_families)) or "-"
        lines.append(
            f"| {fs.name} | {fs.line_start} | {fam} | {len(fs.d3d_gets)} | "
            f"{len(fs.desc_queries)} | {len(fs.readable_walks)} | {len(fs.gobjects_walks)} | "
            f"{len(fs.log_calls)} | {len(fs.allocations)} | {fs.interlocked_ops} | {score_function(fs)} |"
        )
    lines.append("")

    for fs in ranked:
        if score_function(fs) == 0:
            continue
        lines.append(f"## {fs.name} (line {fs.line_start})")
        lines.append("")
        if fs.hotpath_families:
            lines.append(f"- Hot-path families: {', '.join(sorted(fs.hotpath_families))}")
        if fs.d3d_gets:
            lines.append(f"- D3D9 Get* queries ({len(fs.d3d_gets)}):")
            for item in fs.d3d_gets[:8]:
                lines.append(f"  - `{item}`")
            if len(fs.d3d_gets) > 8:
                lines.append(f"  - ... and {len(fs.d3d_gets) - 8} more")
        if fs.desc_queries:
            lines.append(f"- Desc/lock queries ({len(fs.desc_queries)}):")
            for item in fs.desc_queries[:8]:
                lines.append(f"  - `{item}`")
        if fs.readable_walks:
            lines.append(f"- Readable/VirtualQuery walks ({len(fs.readable_walks)}):")
            for item in fs.readable_walks[:8]:
                lines.append(f"  - `{item}`")
        if fs.gobjects_walks:
            lines.append(f"- GObjects/object-model walks ({len(fs.gobjects_walks)}):")
            for item in fs.gobjects_walks[:8]:
                lines.append(f"  - `{item}`")
        if fs.log_calls:
            lines.append(f"- Unconditional Log() calls in hot path ({len(fs.log_calls)}):")
            for item in fs.log_calls[:8]:
                lines.append(f"  - `{item}`")
        if fs.allocations:
            lines.append(f"- Heap allocations ({len(fs.allocations)}):")
            for item in fs.allocations[:8]:
                lines.append(f"  - `{item}`")
        if fs.interlocked_ops:
            lines.append(f"- Interlocked operations: {fs.interlocked_ops}")
        lines.append("")

    return "\n".join(lines)


def main() -> int:
    parser = argparse.ArgumentParser(description="Scan d3d9.cpp hot paths for expensive patterns")
    default = Path(__file__).resolve().parents[1] / "spikes" / "view_matrix" / "d3d9.cpp"
    parser.add_argument("source", nargs="?", default=str(default), help="path to d3d9.cpp")
    args = parser.parse_args()

    path = Path(args.source)
    if not path.exists():
        print(f"error: file not found: {path}", file=sys.stderr)
        return 1

    text = path.read_text(encoding="utf-8", errors="ignore")
    funcs = find_functions(text)
    print(render(funcs))
    return 0


if __name__ == "__main__":
    sys.exit(main())
