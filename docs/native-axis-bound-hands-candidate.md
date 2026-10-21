# Native shot axis, cinematic hands and scoped gore

Offline candidate; no physical verification yet. Preserve accepted scope alignment,
R1 precision and all installed assets. New features default OFF.

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
