# SIDE handoff: independent free hand and shot vibration

Base: `origin/main` at `a1b506b9`. This branch does not install or run the game.

## Observed producers and correction

- The native glove finger path was reachable when both hands were free, or when the off hand owned a knife/magazine. `NativeGlovesEligible()` deliberately rejects a held right weapon, leaving the free left fingers in UE3's old two-hand pose. The exact noTMD glove section now poses **only** the off-hand finger bones while a weapon is held and support/knife/magazine ownership is absent. The right weapon fingers, wrist transforms, support latch and TMD hardware are unchanged.
- The analog squeeze value was reset at the start of each input frame, preventing a smooth opening. Valid tracked input now blends toward the real squeeze. Missing/inactive input or tracking loss clears the prior fist immediately. Pause/cinematic/menu paths do not admit free-hand posing.
- Archived M09 evidence shows zero nonzero XInput rumble while firing, although adaptive trigger feedback worked. The new optional shot overlay watches `FireAmmunition` on the current weapon and requires its reflected `mShotCount` to increase by exactly one **after the native call**. R2 alone, dry fire, unknown weapon and unresolved witness do not arm it. Pistol/rifle/shotgun/sniper have separate bounded profiles; native rumble cancels the fallback. The existing single OpenXR haptic submission and 15-frame refresh are retained.
- Native TMD `StartFirePower` success, local ownership, left-hand pulse profiles and the M09 physical PASS path are preserved. Enable its existing `TmdPulses=1` for the combined physical trial; a disabled flag is not an OpenXR failure.

## Controlled MAIN configuration

```ini
[Haptics]
Enabled=1
Level=1
TmdPulses=1
WeaponShotPulses=1
```

`WeaponShotPulses` defaults to 0 and is independent of TMD and adaptive triggers. No extra texture, asset or INI key is required for the hand correction. If `mShotCount` is not readable/incrementing for a weapon, fallback vibration fails closed; the first 12 matching calls log `weapon haptics witness` with before/after and armed status.

## Offline validation / limits

- `tools/tests/test-independent-hands-shot-haptics.ps1`: policy, native witness, bounded pulse, native priority, analog release and ownership gates.
- `tools/gloves/test-gloves.ps1`: native glove pose/index/palette suite.
- `tools/tests/test-dual-weapon-tmd.ps1`: independent gun/TMD/support suite.
- `spikes/view_matrix/build.ps1` without `-Install`: optimized x86 PE32.
- `tools/reload/test-combined-followups.ps1` and `tools/gloves/test-tmd-runtime-regressions.ps1` need private `OriginalPackage` / `Asset` inputs; they cannot be run from the public checkout alone.

This is L1/L2 evidence. It does not prove headset appearance, perceived vibration, or actual `mShotCount` progression. MAIN should verify those in one PSVR2 session, watching the bounded witness receipts.

## One MAIN headset test

With a weapon in the right hand, hold the left hand free, then press/release Grip near the fore-end and away from it; check that only valid support couples the hands and fingers reopen after release. Fire one shot, an automatic burst and an empty trigger pull, distinguishing actual right-controller vibration from adaptive trigger resistance; test one other weapon if available. Activate Impulse and Age, grab/release a magazine, pause/resume and change weapons; check left TMD pulses, grip ownership, no stuck haptics and no displaced hands. Preserve the accepted installed pair for rollback and verify it after the trial.
