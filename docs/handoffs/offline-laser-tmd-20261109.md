# Offline laser controls and captured TMD decal filter

Scope: offline implementation; installed accepted DLL/profile preserved.

## Laser

VR Settings -> Controller -> LASER OPTIONS (minimal UI), or Display -> LASER
OPTIONS (full UI). OFF / DOT / BEAM + DOT stays available. Subpage returns to
its originating page. New Render keys, saved by the existing panel Save path:

| Key | Default | Range |
| --- | --- | --- |
| LaserWidthPct | 100 | 25..200 |
| LaserDotSizePct | 100 | 25..250 |
| LaserBrightnessPct | 100 | 10..100 |
| LaserShimmer | 1 | 0/1 |

Width/dot adjustments use steps of 10 percentage points with endpoint clamps.
Brightness scales alpha; shimmer toggles the animated dot halo. Both eyes use
the same phase. Existing thin appearance remains the 100% reference. Dot is
still a fixed-distance direction marker, not a surface collision hit. No actual
environmental scattering, volumetric lighting or occlusion is claimed.

## TMD hypothesis and falsification

Private trace build/psvr2-9d2c-20261107/shadow-pass-17792-3.txt groups 311/312
associates the black rectangles with a modulate decal pass. Broad accepted
Diagnostic/DropDecalLocalTangent skips a shader family by CTAB name. New
Render/TmdGravityDecalFix=0 (default OFF) matches bytecode length 2132 and
FNV1a32 9CE7CA1D, triangle list, stream0 stride12/offset0, verts5161,
primitives6..64, colourWrite15, zWrite0, blend1/src9/dst1, stencil0, op1,
zEnable1/zFunc4, separateAlpha1/src1/dst2/op1. Failed state queries fail closed.
Context requires VR gameplay, TMD possession/pose, scene target, no pause,
menu, panel or cinematic. Reset/ResetEx clear bound classifications. First four
suppressed draws produce `tmd-gravity-decal: exact captured pass suppressed`.

This identifies a captured draw class, not unique runtime actor ownership or
the engine root cause. Same shader/draw class elsewhere can still match.
Shader GetFunction/hash is read at each successful bind when the new flag is
enabled; no per-draw allocation or retained COM reference is introduced.
Performance of the optional filter has NOT been measured.

Future controlled A/B: leave accepted baseline recoverable; disable broad
Diagnostic/DropDecalLocalTangent and enable only Render/TmdGravityDecalFix.
Reproduce lift and sky aim; require suppression receipts, then compare neutral
decals and both-eye rendering. Missing receipts or surviving rectangles falsify
candidate coverage. No physical acceptance or replacement of accepted fix yet.

## Evidence

- L1_PASS: test-laser-style.cpp modes/cycle, widths, shimmer, alpha scaling/clamps.
- L1_PASS: test-tmd-gravity-decal.cpp positive captured shader; 2132 single-byte
  mutations rejected; each draw-state gate independently rejected when changed.
  Private shader was extracted from group311, never committed.
- L1_PASS: source-extracted actual save/load cases compiled into
  build/test-laser-settings.cpp; Windows INI roundtrip, out-of-range clamps and
  missing-key defaults tested using a disposable temp file.
- Trigger-shot and independent-hands-shot-haptics CPU regressions PASS.
- L2_PASS: build.ps1 x86 PE32 /O2, build/laser-tmd-offline-build.log.
  DLL SHA256 EED37A279675CB99C287E08DAE3649D93CE52700342E2D3AA138F1209EAB4A31.
  Build includes existing analysis/compiler warnings; not warning-free.
- Installed DLL read-only hash check unchanged:
  C6C1A5041F544CCFCA23DD5D3F475E989603698067504D4D65A813AFC4552B3B.
- L3 and physical laser/menu/filter checks NOT RUN. No Steam launch/deploy,
  profile edits, save edits or performance improvement claims this iteration.
