# Cinema framing, native scripted shots and noTMD cuff — 2026-11-02

Branch: `fix/cinema-framing-wrist-scripted-aim`, based on `67193c5d`.
Tested installed preimage: BEDA5911FE78F6A67A5F6CA5C885D0B9CE16F8BA62F2E66446976F8340359431.

Physical evidence: user confirms stereo cinema, but reports cropped helicopter image,
unequal visible eye edges, HUD penetrating the noTMD cuff, and inability to aim during
the mutant scripted camera. These are separate acceptance items.

## Implemented corrections

- Proven code defect: successful stereo bypassed mono composition, then the legacy
  Present matte cleared valid source content to 84% width / 70% height. Completed
  stereo now bypasses that matte. The XR quad already sizes the complete source.
  Frame completion, enabled state and menu exclusion share one policy. Bink and
  failed/uncomposed legacy paths remain unchanged. Equal full source rectangles
  are retained; whether this also resolves the perceived asymmetric edge is PENDING.
- Proven ownership conflict: SingleLineCheck rebased a shot to controller aim while
  cinema displayed the native animated weapon. Native-camera scenes now preserve
  original trace arguments and suppress alternate aim modes 6–9 field/result writes.
  Gameplay aim, native spread/recoil, ammunition and damage are unchanged. This is
  restoration of native scripted targeting, NOT an unconditional hit/kill feature.
  Mutant encounter success is PENDING. Four opt-in native-shot receipts are capped.

## HUD contradiction and precise remaining gate

A proposed bind-bound offset clamp was discarded before delivery: installed
WorldScalePct=80 yields 65.625 UU/m; 94 mm gives X=6.16875 UU, while the original
noTMD SVNC cuff max wrist-local X is 5.23952707 UU. A blind increase does not explain
this physical failure. TMD placement and HUD offsets have NOT been changed.

Under existing EquipmentWristTrace=1, exactly one actual-bound noTMD SVNC palette,
IB identity, local matrix, final wrist/world maps and configuration are recorded
per eye. No writes, readback, flush, tracking bypass or per-frame logging is added.
`tools/hud/analyze_wrist_cuff_capture.py` reconstructs original weighted vertices
through those GPU constants relative to the same wrist. It refuses incomplete
receipts/influence mappings. Positive clearance with physical clipping falsifies
mesh penetration and isolates projection/foreground-depth as the next dependency;
negative clearance proves an animated palette/geometry mismatch. No guessed fix.
Private inputs/receipts stay in ignored `build/cinema-framing/`.

## Offline evidence

- x86/x64 gaze, stereo completion/matte/native-aim policies and damage tests PASS.
- Actual D3D9Ex stereo pixels/state restoration PASS; all 8 scene corner pixels
  retained (4 per eye). Native convention lifetime still passes 180 frames.
- HUD policy/projection/source/placement and 12 final-palette integration cases PASS.
- Existing full HUD runner stops at legacy D3D9 HAL CreateDevice (0x8876086C), a host
  limitation, not reported as PASS. Separate unchanged trace/occlusion fixture bodies
  executed D3D9Ex-only: wall, glove/magazine, both eyes, Reset and restoration PASS.
- Dual-TMD/hands/reload/WeaponLocker/OLED regression suite PASS.
- Cuff analyzer: original private weighted SVNC, 2 rigid wrist rotations x 2 eyes,
  unchanged reconstructed source bounds PASS. Python compile PASS.
- Optimized x86 PE32 build PASS. Clean source/DLL identity goes in staged manifest.
- No real gameplay or physical PASS is claimed for these corrections.

## One specific acceptance session

1. New game: helicopter complete scene, compare outer cinema edges in both eyes.
2. Before acquiring TMD, raise/rotate inner left wrist with visible HUD/cuff. This
   automatically captures the one missing final-palette sample; no manual logging.
3. Mutant scripted attack: fire normally. Confirm whether native scene progresses.
4. Close normally. MAIN analyzes cuff capture before changing its placement/depth.

Steam deployment requires closed-game/full-profile backup and exact hashes. No
save edits, unrelated feature updates, auto-hit or physical promotion are included.
