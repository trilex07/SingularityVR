# Knife / Valkyrie / supported accuracy follow-up

SIDE only; no Steam access, launch, deployment or merge.

## Evidence and limits

Archived original session: `5c5f79748901821cc92ac0adf1cb489132839687`,
`docs/evidence/steam-4b34ce06-20261023/view_matrix.log`, SHA256
`592023235301C7A57EAFBC1113CAF13C12FD136C31E89E311A8875D866CB7F26`.
Physical report: knife pickup still corrupts presentation; supported TMD assembly accepted.
Frames 3495–3496 already show exclusive native knife route, both eyes, TMD cuff 3994 triangles.
No standalone-held route, no second pickup in this session. Therefore duplicate knife submission
is not a sufficient explanation of this failure. The log has no consumed skin palette or
before/after knife D3D9 state. Knife root cause remains UNPROVEN; no speculative geometry fix.

## Independently reversible changes

- `ef85bf3`: Valkyrie NeedAmmo formerly required a grip rising edge while already inside the
  shoulder volume. A new free squeeze now arms an 800 ms reach window; entering the volume
  while holding completes pickup. Release, occupied hand, support, knife proximity and invalid
  state cancel it. Other weapon profiles, ammunition transfer and FSM remain unchanged.
  This fixes an offline-reproduced rejection; the physical intermittent failure may have
  additional causes.
- `e13498d`: `[Input] TwoHandSpreadPercent=100` preserves native behavior by default. MAIN may
  test 50. Existing fingerprint-verified native AddSpread still runs; only its wrapped random
  pitch/yaw delta is halved for a supported local shot. Native central aim/recoil, roll,
  damage and range are not replaced. No guessed native recoil property. Shot witness consumes
  the corrected result. Single-hand output remains native.
- `9d637a4`: `[Diagnostic] TmdKnifePaletteProbe=1` (default OFF) distinguishes palette deformation
  from private knife draw state leakage. Four source palettes and four consumed palettes name
  all 36 material-0 influencing bones; at most 16 eye draws compare 108 shader registers and
  actual bound IB before/after StateGuard restore. No GPU readback or render-loop allocation.

## Knife falsification and exact missing observation

`tmd-knife-source` / `tmd-knife-incoming` record independent hand input and result;
`tmd-knife-palette` records actual consumed registers; `tmd-knife-state` records
frame/eye/hands/support, IB identity, HRESULTs, changed float count and restoration.
Successful state preservation requires valid HRESULTs, matching IB, changed=0 and restored=1.
Nonzero changed or mismatching IB proves a restore violation; equal state instead directs
analysis to source/final skinning and measured wrist mount. Neither result alone proves visual
acceptance. This candidate intentionally does not claim to fix the knife.

## Offline validation

- Full dual-TMD suite with SkipOled: PASS, including next-weapon reload transactions.
- New test-two-hand-spread.cpp compiled x86 /O2 and executed: PASS (native/default, support/local
  gates, wraparound, central aim and roll preservation).
- tools/knife/test-knife.ps1: PASS, six C++ executables plus 14 Python cases; synthetic renderer
  state/refcounts and compiled shader tests, not physical GPU/content proof.
- x86 PE32 /O2 build at 9d637a4: PASS, clean source tree; existing analyzer warnings remain.
- DLL SHA256: `46B5C4109BC768FA5862D8B169FA2CF7A5BA46103FDAA3D4833BFD9CB13FB4AB`.
- OLED/WeaponLocker code unchanged. No new physical or D3D9 HAL PASS claimed.

## Minimal MAIN validation (only after review)

Preserve the accepted installed configuration/assets. Test Valkyrie after Circle: squeeze just
before reaching behind the left shoulder, insert normally; confirm support/knife cannot steal
the gesture. Compare supported versus single-hand groups at a fixed target with spread=50.
For the unresolved knife, enable the bounded probe before launch, acquire once with TMD,
rotate the wrist, sheath, then close and archive the log before another launch. No full A/B/C
repetition is needed: this specifically supplies the missing palette/restoration observation.
Preserve accepted two-hand sleeve/TMD geometry; do not disable it as a workaround.
