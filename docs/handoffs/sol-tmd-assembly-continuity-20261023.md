# TMD assembly continuity — SIDE handoff

Base: `685e42dc2175e2d1e652327678079f617d1c6724`.
Branch: `fix/sol-tmd-assembly-continuity`. No Steam writes or game launch.

## Evidence and scope

Read-only failed-session log: `%LOCALAPPDATA%/SingularityVR/view_matrix.log`,
SHA256 `380BC19D4B14ABDF26112A475D04322068DBE17AB6B15566626190F32F701DB9`.
Line 137 records `GloveLeftShift=12`. Lines 12207–12279 report successful
TMD SVNC draws (3994 faces); line 12458 reports SVTG stereo (7110 faces,
seven sleeve bones, index/palette restoration OK). Frame 11165 has material
0 and material 1; frame 11428 has support active and material 1 suppressed.
The old route census reports the restored source IB, NOT the bound SVNC IB.
It cannot prove the support wrist's submitted topology.

Confirmed source-level violation: `PrepareIndependentLeft` applies a 12-UU
glove-only component-X parking shift when a weapon is held. Device bones are
explicitly excluded. SVTG sleeve bones are staged from original matrices and
the controller delta, so they do not inherit that parking shift either.
This breaks the attachment relationship even with the corrected sleeve palette.
Correction: suppress that parking shift only for material-0 TMD gloves with
short-gauntlet enabled, isolation mode 2 and support inactive. Support targets,
right weapon, animation, original materials and sleeve staging are unchanged.
This removes one proven divergence; it is NOT a claim of final headset alignment.

Private original-asset measurement: the SVNC left component has 1786 faces and
no open boundary; closest original glove/sleeve vertices are 0.192703 UU apart.
At -70 and +55 degree synthetic wrist rotations a common rigid transform
preserves that relationship; a glove-only shift adds exactly 12 UU error.
These are original-topology and mathematical tests, not captured GPU palettes.

## Support wrist remains an evidence gate

No extra cap or artificial offset is added to a mesh already topologically
closed. The observed opening may be a different submitted route or deformation;
current logs cannot distinguish these. Material-1 hardware remains hidden on Grip.

Bounded `tmd-assembly bound` receipts now read the actual bound IB and final
VS wrist/device skin matrix inside StereoPair, before restoration. Original
matrix rows are logged as `tmd-assembly source`. One frame is selected from
material 0 for each hands/support combination, with separate eye/material bits;
at most 32 bound receipts per process, no surface readback or GPU synchronization.
These are pre-submission state receipts, not proof of successful draw/pixels.
Missing material-1 receipts in support are expected. Missing paired receipts
outside support are evidence of routing/order divergence, not inferred success.

MAIN should compare same-frame/eye source-to-final rigid deltas, inspect whether
support material 0 binds SVNC (3994), and correlate with the existing post-draw
SVTG receipt and physical image. If SVNC binds and the surface still appears
open, capture that material's final palette and cull/material state before
changing topology. Do not re-enable hardware to conceal it.

## Validation

- Private `tools/gloves/test_tmd_assembly.py`: PASS, topology and two rotations.
- `test-tmd-short-gauntlet.ps1` with original PSK/cooked/SVTG: PASS, actual dispatch,
  fallback, restoration, five rotations, 414 mixed-influence triangles.
- `test-dual-weapon-tmd.ps1`: PASS, policy, input, glove, reload, aim, WeaponLocker,
  Python wiring and OLED CPU/D3D11 GPU tests.
- `test-tmd-runtime-regressions.ps1`: CPU tests passed; sandbox HAL creation
  failed before rendering. Same built D3D9 fixture run outside sandbox: PASS
  D3D9 and D3D9Ex, 2x7110 faces, IB restored, 3304 pixels in each eye fixture.
- Build: x86 PE32 `/O2`, machine 014C, optional header 010B; existing warnings.
- DLL SHA256: `4329FE7CE7F2A9A6A5801297FC5A905507F7BFE3EBD6857B2B28A21453C4BE82`.
- Original SVTG unchanged: `452EA0C0DDFB4721A7F9B9718629975CF021F9E079E1CD92235C076E9E376192`.

## MAIN physical gate

Raise TMD and rotate the wrist: glove/device/sleeve should stay connected without
the former parking displacement or long arm. Hold and release left Grip with a
weapon: inspect the wrist opening and support, then raise TMD again. MAIN collects
the bounded receipts; no physical PASS is claimed and support opening is not
declared fixed. This is a scoped attachment correction plus a discriminating
support diagnostic, not authorization for promotion to accepted baseline.
