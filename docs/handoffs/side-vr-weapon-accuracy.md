# VR weapon accuracy: controlled Valkyrie/Volk correction

## Observation and falsifier

Physical feedback reports poor Valkyrie and Volk accuracy. The accepted runtime already uses a measured muzzle witness and can match an actual `AddSpread` result to a collision ray. Archived rifle rays have `aimDot` 0.99786–0.99992, but those samples do not distinguish native dispersion from recoil or auxiliary traces. Native `AddSpread` is fingerprinted; the live native spread properties for these particular acquired weapons are not established from shared CDO defaults.

Hypothesis: reducing only the random pitch/yaw delta returned by native `AddSpread`, while leaving its central aim/recoil and the existing measured-axis rotation intact, improves VR shooting feel. This is falsified if grouped wall impacts remain wide at fixed pose, or if impact centers disagree with the visible barrel despite a fresh matching `AddSpread` witness. The test must use impact decals rather than randomized cosmetic tracers.

Implementation is opt-in and makes one combined calculation before the existing single angular-delta write. The profile identity must agree across the currently rendered weapon vertex count, local weapon pointer and a fresh muzzle witness. An absent or mismatched profile retains the pre-existing global `TwoHandSpreadPercent` behavior, with no new weapon-specific scaling.

`[Input] VrValkyrieSpreadPercent=100` and `VrVolkSpreadPercent=100` affect matching one-hand shots. `VrValkyrieTwoHandPercent=100` and `VrVolkTwoHandPercent=100` add a matching-weapon-only support factor. All four are default OFF. `TwoHandSpreadPercent` remains the accepted global two-hand control; do **not** set it to 50 merely to test this candidate, because that also changes other weapons. Suggested controlled A/B: keep global at 100, use Valkyrie 70/50 and Volk 85/50 for one-hand/scoped support. Effective native random-delta percentages are 70/35 and 85/43. These are test settings, not proven optimal game balance. Leave `WeaponMeasuredAim=1`, `WeaponNativeAxis=1`, `WeaponTraceRigidOrigin=1` and `TraceFromGun=1` at MAIN's already accepted values; the new policy does not enable or replace them.

No first-shot detector or recoil-property write is justified by the available evidence; these remain native. The next physical comparison should separate single shots, a burst and two-hand support for both weapons.

## Offline verification

- Fixed-seed 10,000-sample angular RMS: native 1.7985°, Valkyrie one/two hand 1.2593°/0.6297°, Volk one/two hand 1.5289°/0.7735°. Native central aim and roll remain untouched.
- Original barrel-to-ray rotation preserves length and source; the existing fresh shot-sequence match excludes unrelated collision rays.
- Native `AddSpread` method still requires the exact 536-byte fingerprint `90E1F845D48FC976`. The offline source contract test covers the prior `GetWeaponSpread`/`AddSpread` audit, but these tests cannot establish live weapon-property values or visible impact grouping.
- Steam and PSVR2 were not accessed by SIDE. Physical acceptance is pending.
- Earlier build at `eeb76e3` (DLL `314774F7…`) is superseded. Safety-follow-up x86 PE32 `/O2` DLL SHA256: `116941A8BE86871DAF5AADFFF2D5011D2A8E416AA01EDD91F730424609006B18`.
- `test-dual-weapon-tmd.ps1 -SkipOled` PASS, including measured aim, ammo/reload, gloves, TMD and WeaponLocker; `test_rifle_scope_backend.py` 8/8 PASS; input/remap and reload instruction tests PASS. M02 D3D9 and D3D9Ex fixture both PASS (2240/2240 pixels each eye, restored texture/blend state); OLED D3D11 suite PASS. These are offline and fixture results only.
- Safety follow-up repeated the x86 `/O2` build, fixed-seed angular test, dual-TMD/reload/WeaponLocker suite and native-rifle contract suite: PASS. Policy tests cover matched one-hand, matched two-hand, mismatched support, stale witness/weapon switch, global 50 legacy fallback and all defaults at 100. The runtime still calls the original `AddSpread` exactly once and calls `twohandspread::Apply` at most once.

## MAIN physical check

Use the same save and fixed wall marks. With accepted baseline first, then guarded candidate, fire three isolated Valkyrie shots, one short burst, repeat two-handed, and repeat with Volk at closer range. Compare hit decals against the visible barrel, not tracer frequency. Verify reload, weapon switching, Kasimov scope and Centurion were unaffected. If a paired `AddSpread` witness is absent or impacts remain displaced, revert only the candidate DLL/INI changes and inspect those exact receipts before more tuning.
