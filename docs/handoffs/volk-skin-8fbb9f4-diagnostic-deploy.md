# MAIN deploy: Volk impulse five-bone skin probe (`8fbb9f4`)

**Objective:** Capture full Volk S4 skinning pipeline evidence during L2/R2 (not a rendering fix).

| Item | Value |
|------|--------|
| Commit | `8fbb9f46b5e993fa4f3a360f723fa90fe9eceae1` |
| Branch | `fix/sol-volk-impulse-skinning` |
| Base | `067e1b1` → `cac4ce3` |
| SOL-reported DLL SHA256 | `2DC5EF570CB0D8E46FD6F0D65D9EF8B019D1CAB566E5DA4352776F7A0105A754` |
| **MAIN x86 `/O2` installed SHA256** | `6B085CBAC21BDCAAD57A7B72FDC510C9403B166660DE4C366D7BC7ADA15F2CAB` |
| Rollback folder | `deployment/deploy-backups/20261023-pre-volk-skin-8fbb9f4/` (`BEA5A562…` / `cac4ce3`) |

## Diff review (diagnostics only)

- `volk_skin_probe.h`, `test-volk-skin-probe.cpp`, Python audit helpers, handoff doc.
- `d3d9.cpp`: read-only impulse events, bounded draw/upload witnesses, no constant or palette mutation.

## Regressions (MAIN build)

- `tools/tests/test-dual-weapon-tmd.ps1` — PASS (incl. volk-skin-probe unit test, reload, WeaponLocker, OLED)
- `tools/gloves/test-tmd-short-gauntlet.ps1` — PASS
- `test-tmd-short-gauntlet-d3d9` — PASS **D3D9 + D3D9Ex**, 2×7110 draws

## Steam

- **DLL:** diagnostic build installed and hash-verified.
- **INI:** only `[Diagnostic] VolkImpulseSkinProbe=1` added; gameplay/TMD settings unchanged (`DualWeaponTmdReady=1`, etc.).
- **Log:** prior `view_matrix.log` → `view_matrix.log.pre-8fbb9f4-volk-skin.bak`.

## PSVR2 script (single capture)

Volk S4, right weapon, left TMD raised as in prior repro. **No reload, swap, menus, or extra combat.**

1. Idle 2 s (weapon visible, no grip).
2. L2 press, hold 1 s, release.
3. Wait 1 s.
4. R2 once.
5. Wait 1 s.
6. Repeat L2 press/hold/release + R2 once.
7. Hold L2, press R2 while L2 held, then release both.
8. Stop session.

Report whether weapon vanishes on L2 and returns on R2 (same as prior partial).

## Post-session (MAIN)

Archive full `%LOCALAPPDATA%\SingularityVR\view_matrix.log`, correlate all `volk-skin *` lines, classify stages A–E, push `handoff/volk-skin-8fbb9f4-diagnostic-evidence`, restore `cac4ce3`.
