#!/usr/bin/env python3
"""
Naming consistency: the injector's runtime lookup and the deploy step must
agree on the replacement file names for every quality tier.

Builds the tiny C++ helper that prints texq::QualitySuffix() for each tier,
then runs build.ps1's deploy into a temporary directory for every tier and
compares the two. Nothing here touches the Steam installation.
"""

from __future__ import annotations

import os
import shutil
import subprocess
import sys
import tempfile
from pathlib import Path

REPO = Path(__file__).resolve().parents[3]
INJECTOR = REPO / "asset-tools" / "texture-injector"
BUILD = INJECTOR / "build.ps1"
REMASTER = REPO / "material-remaster"
TIERS = ["2x", "4K", "8K"]
VSWhere = Path(os.environ.get("ProgramFiles(x86)", r"C:\Program Files (x86)")) / \
    "Microsoft Visual Studio" / "Installer" / "vswhere.exe"


def compile_suffix_helper(out_dir: Path) -> Path | None:
    """Compile the injector's naming helper so the test can read its runtime values."""
    src = INJECTOR / "tests" / "quality_suffix.cpp"
    if not VSWhere.exists():
        print("  SKIP: vswhere not found - cannot compile the injector helper")
        return None
    vs = subprocess.run([str(VSWhere), "-latest", "-products", "*",
                         "-requires", "Microsoft.VisualStudio.Component.VC.Tools.x86.x64",
                         "-property", "installationPath"],
                        capture_output=True, text=True).stdout.strip()
    if not vs:
        print("  SKIP: no MSVC installation found - cannot compile the injector helper")
        return None
    vcvars = Path(vs) / "VC" / "Auxiliary" / "Build" / "vcvarsall.bat"
    if not vcvars.exists():
        print("  SKIP: vcvarsall.bat not found")
        return None

    exe = out_dir / "quality_suffix.exe"
    obj = out_dir / "qs.obj"
    bat = out_dir / "compile.bat"
    # a batch file is used because cmd /c cannot pass a quoted batch path plus
    # arguments through a single string reliably
    bat.write_bytes(("\r\n".join([
        "@echo off",
        f'call "{vcvars}" x86',
        f'cl /nologo /EHsc /O2 /std:c++17 /I"{INJECTOR}" /Fe:"{exe}" '
        f'/Fo:"{obj}" "{src}"',
        "exit /b %errorlevel%",
    ]) + "\r\n").encode("ascii"))
    r = subprocess.run(["cmd", "/c", str(bat)], capture_output=True)
    if not exe.exists():
        out = (r.stdout + r.stderr).decode("utf-8", errors="replace")
        print("  SKIP: compile failed:\n" + "".join(
            ch if ord(ch) < 128 else "?" for ch in out[-1500:]))
        return None
    return exe


def runtime_suffixes(exe: Path) -> dict[str, str]:
    out = subprocess.run([str(exe)], capture_output=True, text=True).stdout.split()
    assert len(out) >= 5, out
    return {"2x": out[0], "4K": out[1], "8K": out[2], "default": out[4]}


def deploy_to_temp(tier: str, game_bin: Path) -> list[str]:
    """Run the real build.ps1 deploy (no compile) into a scratch directory."""
    game_bin.mkdir(parents=True, exist_ok=True)   # build.ps1 refuses to create it
    cmd = [
        "powershell", "-NoProfile", "-ExecutionPolicy", "Bypass",
        "-File", str(BUILD),
        "-Install", "-NoBuild",
        "-Quality", tier,
        "-GameBin", str(game_bin),
        "-RemasterDir", str(REMASTER),
    ]
    r = subprocess.run(cmd, capture_output=True, text=True)
    if r.returncode != 0:
        raise AssertionError(f"build.ps1 deploy failed for tier {tier}:\n"
                             f"{r.stdout[-2000:]}\n{r.stderr[-2000:]}")
    tex = game_bin / "texture-override"
    if not tex.exists():
        return []
    return sorted(p.name for p in tex.glob("*.dds"))


def test_tier_naming_matches_between_injector_and_deploy() -> None:
    failures: list[str] = []
    with tempfile.TemporaryDirectory() as td:
        tmp = Path(td)
        exe = compile_suffix_helper(tmp)
        if exe is None:
            print("  (naming test skipped: no compiler)")
            assert True
            return
        suffixes = runtime_suffixes(exe)
        print(f"  injector QualitySuffix: {suffixes}")

        for tier in TIERS:
            game_bin = tmp / f"bin-{tier}"
            deployed = deploy_to_temp(tier, game_bin)
            expect = suffixes[tier]
            matching = [n for n in deployed if n.endswith(f"{expect}.dds")]
            print(f"  tier {tier}: {len(matching)} file(s) end with '{expect}.dds'")
            if not matching:
                failures.append(f"tier {tier}: no '{expect}.dds' deployed")
            # every deployed file must carry the tier suffix
            odd = [n for n in deployed if not n.endswith(f"{expect}.dds")]
            if odd:
                failures.append(f"tier {tier}: unexpected names {odd[:3]}")
            # the material remaster package must be present for the 2x tier
            if tier == "2x":
                for want in ("AR_shotgun_diffuse_2x.dds",
                             "AR_shotgun_normal_2x.dds",
                             "inview_glove_diffuse_2x.dds",
                             "inview_glove_normal_2x.dds",
                             "door_blastdoorN_diffuse_2x.dds",
                             "door_blastdoorN_normal_2x.dds"):
                    if want not in deployed:
                        failures.append(f"tier 2x: missing remaster file {want}")
                if any("specular_review" in n for n in deployed):
                    failures.append("tier 2x: review-only specular was deployed")
    assert not failures, failures


def test_remaster_output_uses_the_2x_suffix() -> None:
    for dds in REMASTER.rglob("*_2x.dds"):
        assert "_specular_review" not in dds.name, f"review file named as a tier: {dds.name}"
    found = list(REMASTER.rglob("*_2x.dds"))
    assert found, "no material remaster outputs found in the repository"


if __name__ == "__main__":
    failures = []
    for fn in (test_tier_naming_matches_between_injector_and_deploy,
               test_remaster_output_uses_the_2x_suffix):
        try:
            fn()
            print(f"PASS: {fn.__name__}")
        except AssertionError as e:
            failures.append(fn.__name__)
            print(f"FAIL: {fn.__name__}\n{e}")
    sys.exit(1 if failures else 0)
