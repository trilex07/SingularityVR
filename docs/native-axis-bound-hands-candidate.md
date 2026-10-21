# Native shot axis, cinematic hands and scoped gore

Offline candidate; no physical verification yet. Preserve accepted scope alignment,
R1 precision and all installed assets. New features default OFF.

## Build receipt

Source `5370fb859a7f1bb0722562228aca36294be9cdfb`, clean x86 PE32 `/O2`.
DLL SHA256 `F2387D10C6916B94D34BD27D8A4E271DA67AEA113F391B594FA099623979CE33`.
Private prepared directory `build/combined-followups/native-axis-5370fb8`.
Native muzzle/math suite, full Centurion/three-weapon regression suite and native
action-hands checks PASS. Own-process GPU replay is not game-pixel evidence.
Accepted-profile deployment `WhatIf` preflight PASS; writes/launches none.
MSVC analysis emitted existing warnings; this is not a warning-free claim.
Stable installed DLL EF3A1F3B9F0641A291F9285DD7283EC7D3B4433A8F87BC210DBC98B721B7B753
and INI 55A3ED4DBC6E158F01A2219177D825C066E7306DABCE6ACFB1C7118EC9E3F659
were verified unchanged before preparation.

## Evidence and falsifiers

- Valkyrie logs contain rays starting at the measured muzzle but not pointing along
  it. Some may be auxiliary traces: only a ray matching the actual AddSpread result
  may be corrected. The common RvWeaponShared vtable+0x428 body is 536 bytes,
  FNV64 90E1F845D48FC976, SHA256
  55d56047815eac5f1e8432398e36f32568388e101c3ba4f32da8821bb708f154.
  Capture native pre/post-spread directions, then rigidly rotate that cone to the
  measured gun axis. Centurion explicitly uses its existing controller axis,
  not a nonexistent next-weapon mesh witness. No spread or inventory writes.
  Reject stale/wrong weapon, sequence, unmatched ray and cinematic contexts.
  Falsifier: matching receipts occur but impacts still disagree with the sights.
- Gore previously refused `limb-property-type`; the physical impression of gore
  was native behavior, not proof of our writes. Owned RvSharedPawn export chain
  puts mLimbSettings at index 477, beyond the old 400-node bound. Raise to 4096
  and retain typed-property checks. Settings are assets, not PersistentLevel
  actors; accept readable derived RvLimbSettings and restore after TakeDamage.
  No CDO, censorship, packed bool or global permanent mutations.
- Native hand ownership previously required the cinema camera path. An opt-in
  additionally honors actual g_inCinematic while VR is active, allowing original
  animated/bound hands without controller bone surgery. Falsifier: bound scene
  still shows independently tracked gloves, or tracking fails after its exit.

## Flags

Input/WeaponNativeAxis=1; Render/CinematicHandsOwnership=1;
Render/CutsceneNativeHands=1; Gameplay/GoreDismemberment=1;
Gameplay/GoreChancePct=100; Input/WeaponSteadyPrecision=2.

## One uninterrupted PSVR2 test (MAIN only, after user start)

Centurion then Valkyrie: aim at fixed marks at medium distance, single shots then
a short burst; compare sights, muzzle FX and impacts, retaining native spread.
Kasimov: compare reticle/impact and hold R1 with both hands; confirm scope and FPS.
Visit the bound-hands scene, then leave it: original hands during the scene,
controller tracking and stereo afterwards; also observe enemy gore and FPS.
Close normally before reporting. Never ask the user to type while in the headset.
