# M03 — Live HUD data integration; physical mount still gated

Date: 2026-10-23. Scope: this integration branch only. Steam untouched.

## Observed producers and first defects

- The current renderer obtains `Pawn.Health` and `HealthMax` through cached
  reflected offsets. They are IEEE-754 floats. `DrawHealthPlate` decoded them
  correctly; the older `DrawWristHealth` divided their **integer bit patterns**.
  At 72/144 HP, the old ratio is near full rather than 0.5. Both paths could
  also draw in one frame when their INI switches were enabled.
- The current ammo renderer reads the HUD's named `mClipAmmo`/`mBinAmmo`
  properties, with the current first-person weapon vertex identity. It does
  **not** use the ambiguous `mCounts` array. Its left-eye width could leak into
  the right-eye fallback when the latter's metric projection failed.
- The SIDE V2 GLBs are static preview meshes with example numerals. Its
  committed runtime atlas is 512x256 while its current generator describes
  1024x512. Neither preview numbers nor the documented static mount points
  establish the final animated weapon/TMD bone basis in this runtime. The
  21–48 MB multi-material GLBs therefore remain outside the DLL/package.

## Controlled correction

`healthbar::DecodeLiveHealth` is now the common read-only health decoding step
for both current displays and supplies a 10-cell count for the future wrist
fixture. Invalid/non-finite health hides the readout; no synthetic full HP.
The optional Katorga plate takes precedence over the old strip, with the old
strip retained if the optional plate atlas fails to load and `WristHealth=1`.
Ammo plate sizing
now starts from a fresh fallback in each eye, rather than sharing a mutable
width across eyes. The existing health/ammo art, hand tracking, scene gate,
OpenXR projection, D3D9 state block and game inventory are unchanged.

This is a live-data/stereo-correctness step, **not a claim that the SIDE 3D
readouts are physically mounted**. No new overlay is drawn, so there is no
second set of example numbers or hidden dependency on GLBs.

## Falsifiers and next gate

- If a real health change still leaves the wrist strip nearly full, inspect
  the current pawn identity/property offset; the float-bit defect is fixed.
- If only one eye has an incorrectly sized ammo plate, capture each eye's
  projected pixels-per-metre and viewport; the shared-width defect is fixed.
- Before enabling 3D, capture the **same-frame final** native weapon and TMD
  skin palettes, LocalToWorld and controller pose for one known draw, then map
  the proposed fixture pivot through that exact basis. Reject any candidate
  that intersects a weapon/scope, shows static digits, duplicates the current
  plates, loses native materials, or lacks both-eye depth/occlusion proof.

Offline: `tools/tests/test-hud-live.ps1`, full x86 `/O2` build and dual-TMD
regressions. Physical PSVR2 result: **NOT TESTED**. MAIN may verify the current
plate correction separately, but the 3D asset remains **NO-GO**.
