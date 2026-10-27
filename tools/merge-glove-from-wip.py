#!/usr/bin/env python3
"""Merge glove_isolate integration from d3d9.cpp.wip into d3d9.cpp (keep HEAD elsewhere)."""
from __future__ import annotations

import re
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
HEAD = ROOT / "spikes" / "view_matrix" / "d3d9.cpp"
WIP = ROOT / "spikes" / "view_matrix" / "d3d9.cpp.wip"


def line_range(lines: list[str], start: int, end: int) -> list[str]:
    return lines[start - 1 : end]


def main() -> None:
    head_lines = HEAD.read_text(encoding="utf-8", errors="replace").splitlines(keepends=True)
    wip_lines = WIP.read_text(encoding="utf-8", errors="replace").splitlines(keepends=True)

    text = "".join(head_lines)

    if '#include "glove_isolate.h"' not in text:
        text = text.replace(
            '#include "grip_latch.h"\n',
            '#include "grip_latch.h"\n#include "glove_isolate.h"\n',
            1,
        )

    if "g_gloveIsolate" not in text:
        text = text.replace(
            "int g_armIkMode = 0;                  // ini [Render] ArmIKMode\n",
            "int g_armIkMode = 0;                  // ini [Render] ArmIKMode\n"
            "volatile LONG g_gloveIsolate = 2;     // ini [Render] GloveIsolate\n"
            "volatile LONG g_gloveFaceFilter = 0;  // ini [Render] GloveFaceFilter\n"
            "volatile LONG g_gloveStumpScalePct = 20; // ini [Render] GloveStumpScale\n"
            "volatile LONG g_independentHandsAudit = 0;\n",
            1,
        )

    # Replace PrepareIndependentLeft block + prepend glove helpers from wip.
    glove_helpers = "".join(line_range(wip_lines, 14681, 15104))
    pat_prep = re.compile(
        r"int g_independentHands=0;.*?^static void ApplyWorldOffset",
        re.MULTILINE | re.DOTALL,
    )
    if not pat_prep.search(text):
        raise SystemExit("PrepareIndependentLeft anchor not found in HEAD")
    text = pat_prep.sub(
        glove_helpers + "\nstatic void ApplyWorldOffset",
        text,
        count=1,
    )

    # Insert IsIndependentHandSection helpers before FgHidden.
    helpers_fg = "".join(line_range(wip_lines, 15322, 15343))
    if "IsIndependentHandSection" not in text:
        text = text.replace(
            "static bool FgHidden(UINT verts) {                    // arms, in modes 2 and 3\n",
            helpers_fg + "\nstatic bool FgHidden(UINT verts, UINT first, UINT triangles) {                    // arms, in modes 2 and 3\n",
            1,
        )

    text = text.replace(
        "static bool FgHidden(UINT verts);",
        "static bool FgHidden(UINT verts, UINT first, UINT triangles);",
        1,
    )
    text = text.replace(
        "if (FgHidden(numVertices)) return D3D_OK;",
        "if (FgHidden(numVertices, startIndex, primCount)) return D3D_OK;",
    )

    # Prepend glove-only block inside FgHidden (after opening brace line).
    glove_fg = "".join(line_range(wip_lines, 15345, 15413))
    marker = "static bool FgHidden(UINT verts, UINT first, UINT triangles) {                    // arms, in modes 2 and 3\n"
    if "GloveIsolation: suppressed detached" not in text:
        text = text.replace(
            marker,
            marker + glove_fg,
            1,
        )

    # INI: glove isolate block after ArmIKMode read in LoadIni section.
    if "glove isolation=" not in text:
        ini_block = (
            "    LONG gloveMode = GetPrivateProfileIntA(\"Render\", \"GloveIsolate\", 2, path);\n"
            "    if (gloveMode < 0 || gloveMode > 2) gloveMode = 2;\n"
            "    InterlockedExchange(&g_gloveIsolate, gloveMode);\n"
            "    InterlockedExchange(&g_gloveFaceFilter,\n"
            "        GetPrivateProfileIntA(\"Render\", \"GloveFaceFilter\", 0, path) ? 1 : 0);\n"
            "    {\n"
            "        LONG stump = GetPrivateProfileIntA(\"Render\", \"GloveStumpScale\", 20, path);\n"
            "        if (stump < 0) stump = 0;\n"
            "        if (stump > 100) stump = 100;\n"
            "        InterlockedExchange(&g_gloveStumpScalePct, stump);\n"
            "        Log(\"ini: GloveStumpScale=%ld (forearm collapse keep; 12=0.12 wrist stump, 100=no cut)\", stump);\n"
            "    }\n"
            "    g_independentHandsAudit = GetPrivateProfileIntA(\"Diagnostic\", \"IndependentHandsAudit\", 0, path) ? 1 : 0;\n"
        )
        text = text.replace(
            "    if (g_armIkMode < 0 || g_armIkMode > 2) g_armIkMode = 0;\n"
            "    g_independentHands = GetPrivateProfileIntA(\"Render\", \"IndependentHandsPrototype\", 0, path) ? 1 : 0;\n",
            "    if (g_armIkMode < 0 || g_armIkMode > 2) g_armIkMode = 0;\n"
            + ini_block
            + "    g_independentHands = GetPrivateProfileIntA(\"Render\", \"IndependentHandsPrototype\", 0, path) ? 1 : 0;\n",
            1,
        )
        text = text.replace(
            '    Log("independent hands prototype=%d; arm IK mode=%d (0=native, 1=bounded TMD two-bone, 2=TMD gloves-only); shader CTAB required; verified SP sections only",\n'
            "        g_independentHands, g_armIkMode);\n",
            '    Log("independent hands prototype=%d; arm IK mode=%d (0=native, 1=bounded TMD two-bone, 2=TMD gloves-only); shader CTAB required; verified SP sections only",\n'
            "        g_independentHands, g_armIkMode);\n"
            '    Log("glove isolation=%ld (0=off, 1=census, 2=stump-collapse forearm behind the wrist; GloveStumpScale)",\n'
            "        Rd(g_gloveIsolate));\n",
            1,
        )

    HEAD.write_text(text, encoding="utf-8", newline="\n")
    print(f"Merged glove integration into {HEAD}")


if __name__ == "__main__":
    main()
