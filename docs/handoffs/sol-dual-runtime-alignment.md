# Dual runtime alignment: critical visibility evidence gate

Base cac4ce3. Branch fix/sol-dual-tmd-runtime-alignment. No Steam modifications.

## Verified / not verified

Latest view_matrix.log records raw=846, reachedStereo=846 and gunRide=846 for
HANDS_TMD with shotgun identity8297 during census window4950. It also records
HANDS_TMD with foreground11387 at line131107, far after the5400-frame census
expiry. Neither proves pixels, and the early census cannot explain the later
physical disappearance. No new suppression cause is proven.

Do not add a broad visibility exemption: FgHidden already preserves gun draws
in TMD state, and the recorded gun reached stereo. Remaining distinctions:
native animation/local transform places the gun outside view, our camera ride
places it incorrectly, a later segment/draw fails, or another pass occludes it.

## Minimal bounded diagnostic

With existing DualWeaponTmdProbe=1, record at most24 hands/support transitions
on foreground draws matching current equipped weapon vertex identity. At entry,
record pawn weapon identity, hands/support/context/nativeHands and incoming CTAB
LocalToWorld / first four bone constant registers. Inside StereoPair record those
same matrices per eye before submission, then HRESULT, eye count and segment count.
This path does not expire with startup census; matrices are device-state reads,
not render-target readback. No control, pose, material or geometry changes.

No `submitting` receipt means an early return; incoming versus submitting
matrices distinguish inherited native movement from a changed local palette.
Successful draw alone does not rule out clipping or occlusion. First four bone
rows are a witness, NOT the complete skin palette. If necessary select the
proven weapon bone slot next, not arbitrary registers.

## Other issues / dependencies

- Pad12 maps to R2 in prompt_label_rewrite.h, but the header itself describes
  Controls-menu labels and best-effort defaults. The reported gameplay token
  is unknown. No mapping or controls changed.
- Glove/FX/knife origin cannot be inferred from the wrist-only log. No offset
  or mesh changes are justified by this evidence. Preserve cac4ce3/685e42dc.
- Reviewed 9bb0e47 diff: extends Volk carry witness to original TMD material0,
  uses TmdBind wrist9 and publishes after successful drawing/restoration.
  It touches PrepareIndependentLeft but not the independent short sleeve staging.
  Integration deliberately deferred: requested prerequisite (validated rendering
  correction) is not met. Do not present review as integration or PASS.

Build x86 PE32 /O2 PASS, DLL A93D60D00D5317AA5DE58D4F2E50B861472A4E2B0E4BBE55CC3AC54A84755628.
This is a diagnostic intermediate candidate, NOT a visibility fix or acceptance
build. MAIN owns any gameplay run; collect idle -> TMD -> Impulse -> R2 -> Grip
and release in one session. No repeated unchanged baseline test required.
