# VR weapon accuracy: controlled Valkyrie/Volk correction

## Observation and falsifier

Physical feedback reports poor Valkyrie and Volk accuracy. The accepted runtime already uses a measured muzzle witness and can match an actual `AddSpread` result to a collision ray. Archived rifle rays have `aimDot` 0.99786–0.99992, but those samples do not distinguish native dispersion from recoil or auxiliary traces. Native `AddSpread` is fingerprinted; the live native spread properties for these particular acquired weapons are not established from shared CDO defaults.

Hypothesis: reducing only the random pitch/yaw delta returned by native `AddSpread`, while leaving its central aim/recoil and the existing measured-axis rotation intact, improves VR shooting feel. This is falsified if grouped wall impacts remain wide at fixed pose, or if impact centers disagree with the visible barrel despite a fresh matching `AddSpread` witness. The test must use impact decals rather than randomized cosmetic tracers.

Implementation is opt-in and uses the existing `TwoHandSpreadPercent` modifier as one combined calculation. The profile identity must agree across the currently rendered weapon vertex count, local weapon pointer and a fresh muzzle witness. An absent or mismatched profile retains the pre-existing `TwoHandSpreadPercent` path; with the default value 100 it is exactly native output.

`[Input] VrValkyrieSpreadPercent=100` and `VrVolkSpreadPercent=100` are independently default OFF. They affect one-handed shots too, only on a fresh matching witness. `TwoHandSpreadPercent` remains the existing, shared two-hand control. Suggested controlled PSVR2 A/B candidate: Valkyrie 70, Volk 85, two-hand 50. The effective native random-delta percentages are thus 70/35 for Valkyrie and 85/43 for Volk. These are test settings, not proven optimal game balance. Leave `WeaponMeasuredAim=1`, `WeaponNativeAxis=1`, `WeaponTraceRigidOrigin=1` and `TraceFromGun=1` at MAIN's already accepted values; the new policy does not enable or replace them.

No first-shot detector or recoil-property write is justified by the available evidence; these remain native. The next physical comparison should separate single shots, a burst and two-hand support for both weapons.

## Offline verification

- Fixed-seed 10,000-sample angular RMS: native 1.7985°, Valkyrie one/two hand 1.2593°/0.6297°, Volk one/two hand 1.5289°/0.7735°. Native central aim and roll remain untouched.
- Original barrel-to-ray rotation preserves length and source; the existing fresh shot-sequence match excludes unrelated collision rays.
- Native `AddSpread` method still requires the exact 536-byte fingerprint `90E1F845D48FC976`. The offline source contract test covers the prior `GetWeaponSpread`/`AddSpread` audit, but these tests cannot establish live weapon-property values or visible impact grouping.
- Steam and PSVR2 were not accessed by SIDE. Physical acceptance is pending.

## MAIN physical check

Use the same save and fixed wall marks. With accepted baseline first, then guarded candidate, fire three isolated Valkyrie shots, one short burst, repeat two-handed, and repeat with Volk at closer range. Compare hit decals against the visible barrel, not tracer frequency. Verify reload, weapon switching, Kasimov scope and Centurion were unaffected. If a paired `AddSpread` witness is absent or impacts remain displaced, revert only the candidate DLL/INI changes and inspect those exact receipts before more tuning.
