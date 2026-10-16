# Centurion: test feedback, shoulder source and cylinder controls

2026-10-16, SIDE `fix/centurion-grip-shoulder`. ZERO Steam operations.
Based on `origin/main` 644be25, with existing Centurion source through e7a0c89
merged into this OWN branch. MAIN's current integration checkout is separate;
integrate/rebuild there, never deploy this standalone older integration over it.

## Observation, mechanism, falsifier

Physical feedback: game still auto-reloads at empty, left grip grabs gun
instead of ammunition, front-centre spare overlaps knife, white/yellow squares.
Source confirms: previous implementation intentionally retained UE3 auto-reload,
refused any existing support latch, used a belt source and calibration boxes.
The support latch acquires while squeezing near gun; original reload required
the initial rising edge at a smaller target. Moving toward that target while
holding grip could therefore permanently exclude reload.

Hypothesis: short, bounded acquisition priority at the precise reload target
and a physically separate shoulder source resolve those INPUT conflicts.
Falsifiers: same squeeze still owns knife/support, long-held support is stolen,
or source trigger and rendered stock centre differ. CPU integrated replay
exercises actual reload -> knife -> support order; physical alignment UNKNOWN.

## Implemented, not physically verified

- Physical-left source: head minus 22cm left, 12cm down, PLUS 18cm backward,
  yaw-relative only. Radius 13cm. Same anchor for grab, marker and stock mesh.
  Seated/standing math tested at 1440 yaw/height poses. Existing hip anchors and
  radii unchanged. New `ReloadShoulderLeftCm/DownCm/BackCm`; obsolete belt keys
  intentionally ignored so an old INI cannot move ammo back onto the knife.
- A fresh squeeze has 650ms to reach a reload target. Valid reload acquisition
  resets support BEFORE support/knife routing. Expired support/held knife are
  never stolen. Knife receives the real grip history with `free=false`, not a
  fabricated button release. Cancellation retains ownership until release.
- Optional `ReloadControlLayout=1`: gameplay Cross (PSVR2 X, right A action)
  -> native health; Circle -> cylinder release, NOT health; physical right stick
  up -> 120ms jump pulse, 0.75 threshold / neutral <0.35 rearm / dominant-up cone.
  Menu select/back unchanged, crouch-down and R3 weapon wheel unchanged. Direct
  stick-up weapon cycling is replaced; wheel still uses its original cycle bit.
  Right-handed, SwapSticks=0 only; other layouts fall back to legacy routing.
  Cross long-hold laser toggle is disabled in this layout; use VR Settings.
  Existing native tutorial glyphs do NOT automatically describe these remaps.
- Optional `CenturionCylinderGestures=1` (also requires control layout): Circle
  starts Unlocked; lateral motion >=7cm OR roll ~28deg opens; muzzle downward
  >~33deg for 100ms dumps; left grip takes fresh cylinder behind shoulder;
  release at gun socket loads; after >=100ms, lateral/roll flick with muzzle
  no longer down requests ONE native tap. Firing is gated during transaction
  and must be released to resume. No ammo written and no engine call guessed.
- `CenturionReloadMarkers` now explicitly default OFF. White/yellow/cyan squares
  are diagnostics, not final magazine art. `CenturionClipPreview=1` uses the
  existing private original cylinder/albedo asset, in stock at NeedAmmo and
  carried at Carrying. No procedural replacement asset or new texture generated.
  Source placement has no hand-mount offset; carried mount still needs L4 tuning.
- XR reinit, failed action sync and camera loss cancel pending taps/gestures.
  Probe adds at most 32 refused-input rows and eight distinct 128-byte native
  AttemptReload wrapper captures, read-only, outside draw hooks, for offline RE.

## IMPORTANT: native authority still missing

This is NOT completed physical reload. Existing UE3 auto-reload and full reload
animation still run, native cylinder remains on gun, and gesture state does not
actually swing the cylinder mesh or emit real ejected cartridges. Display is
albedo-only, not native lighting/hand occlusion. The opt-in backend remains a
shared Use/reload XInput tap, not a dedicated reload function.

Offline package `RvGame.decompressed.xxx` SHA256
09DE450D9F620952C3701D874BEB90A8BD9235263975157A641B9E3441336C92:
export16381 is RvWeaponShared.AttemptReload; child16380 is bool ReturnValue.
613 is its package FName index, NOT a native opcode number. The tiny serialized
stub is not the native implementation. This identifies a declaration, NOT the
actual auto-reload producer or its call ABI. Existing ProcessInternal hook cannot
be assumed to observe native execution. A native-code capture is read-only;
never return early from the interpreter or write ammo based merely on this name.
The original Reload animation's refill notify at1.47052908s remains offline
structural evidence only. No auto-reload suppression patch is included/claimed.

Next native step for MAIN: use existing `CenturionReloadProbe=1` on its CURRENT
rebuilt integration, archive identity/binding/code rows and native ammo changes.
Disassemble captured native wrapper + callers, distinguish empty-clip automatic
request from deliberate native reload, prove begin/refill/end before gating it.
No requirement for user to attach debugger or switch out of headset mid-test.

## Narrow test configuration (MAIN only)

Do not replace the whole INI or any Polish/knife/glove/menu assets. All new
features default OFF; this file is a scoped proposal, NOT authorization to deploy.

```ini
[Input]
CenturionManualReload=1
ReloadControlLayout=1
CenturionCylinderGestures=1
CenturionClipPreview=1
CenturionReloadMarkers=0
CenturionNativeReloadTap=0
CenturionReloadProbe=1
```

First run is dry gesture recognition: native tap OFF; fire TWO shots, do not
empty cylinder (auto reload is not fixed), follow Circle -> sideways flick ->
muzzle down -> take from left shoulder -> insert/release -> close flick.
MAIN reads bounded state logs, user stays in headset. An insertion log or draw
submission does NOT prove real ammo changed or native cylinder was animated.
Only later, separate explicit NativeTap=1 trial away from interactables.

Headset script (MAIN after L2/integration):
1. Verify Cross heals when injured, right-stick-up jumps once per neutral return,
   crouch/R3 wheel and pause/options/back still work; Circle must not heal.
2. With partially spent Centurion, perform complete gesture, check shoulder
   grab never picks knife and handling ammo does not attach support/fire/TMD.
3. End normally and report geometry/alignment/sequence; MAIN verifies exit and
   safe restore/provenance. No claim auto-reload is fixed by this candidate.

## Verification

L1: `tools/reload/test-centurion.ps1`: eleven /O2 /W4 /WX CPU executables
(including controls routing, cylinder sequence, actual grip arbitration order,
360000 lifecycle frames, presentation restore faults) +27 Python tests.
L2 receipt is appended after clean-source x86 PE32 /O2 build.
L3 Steam / pixel content / L4 PSVR2 for this follow-up: UNKNOWN.

L1_PASS: all eleven CPU executables +27 Python tests; additional knife suite
six CPU executables +14 Python tests PASS (synthetic only, private rig/shader
replay not requested). SM3 vertex/pixel shader compilation PASS, not GPU proof.
L2_PASS: clean source `80a5e340b79da773912d3ec55d64dbe27e3cf590`, x86 PE32,
`/O2`, empty `workingTreeStatus` in build manifest. DLL SHA256:
`B5352D3C9DF60F705904A0AE992EDF478E96650A1E7B6888F4B51F3985384CAA`.
Existing static-analysis/compiler warnings remain; not claimed warning-free.
Private candidate in this checkout's ignored `build/centurion-reload/candidate`,
including verified original cylinder/albedo; NO INI and NO Steam deployment.
Not READY_FOR_PHYSICAL_ACCEPTANCE as a full manual reload: native authority
gate above remains open. Control/gesture-only integration can be evaluated
separately by MAIN with its current accepted build and exact scoped INI plan.
