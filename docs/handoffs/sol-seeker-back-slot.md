# Seeker: owned back slot, measured support and native launch candidate

SIDE branch `feature/sol-seeker-back-slot`, based exactly on accepted integration
`543bff1242b319673a03d2bc37ff540e00e0bd67` (PR #58).
Target PR base: `integration/main-sol-20261024`. No Steam access or deployment.
L1/L2 results below do **not** establish physical PSVR2 acceptance.

## A. Back storage/retrieval

**Proven native mechanism:** `RvPlayerController::SwitchWeapon` at `1047220`
queries `IsHeavyWeapon` through `+554` twice (return offsets `D2` and `E9`).
A positive result selects the heavy-slot clearing and `104E500` throw path.
The measured 13-byte getter reads native `+2D8` bit `80`; its full fingerprint
and SwitchWeapon's complete 447-byte fingerprint are checked before hooking.

`native_seeker_slot.h` substitutes false **only at those two callers**, on
the engine thread, in gameplay, for the current pawn's actual owned
`ARSteerable` object. It does not clear the real heavy flag or alter native
ammo, chain, archetype, native drop functions or unrelated caller results.
Scripted/explicit drops remain authoritative. An inventory-chain scan is
bounded to 128 objects, rejects cycles, validates reflected fields, live
objects and Instigator ownership; repeated render/XR frames never create items.

Right Grip in the existing lower-back Heavy zone requests native owned selection
through pawn `+B14` (`102F650`, FName by value, `ret8`). The selector searches
the real chain and dispatches through InventoryManager `+42C`. Switching
back uses the last still-owned ordinary weapon. Requests carry pawn identity;
pause/cinematic, changed pawn, deletion, invalid owner and unknown method
contracts refuse. No weapon creation, save edit or fabricated inventory entry.

Zone uses the existing body-equipment layout: 18 cm right, 55 cm below and
32 cm behind HMD, yaw-relative, radius 16 cm. It is a gesture slot, **not a new
visible back mesh**. Hip holster geometry and its update policy are unchanged;
a valid back gesture consumes only its own rising Grip edge. Knife/reload/
support/wheel ownership blocks it. Right-handed mode only in this candidate.
With no ordinary weapon left, store refuses rather than inventing an empty slot.
Persistence across a save/load is NOT proven: the original save system remains
authoritative and the slot reconciles whatever objects it actually restores.

## B. Measured left support

The previous support profile has no Seeker entry and uses its generic fallback.
Original `ch_inview_e99_steerable_mesh.psk` and `e99_steerable.psa` establish
`cog_e99_steerable` bone 53, `l_arm_wrist` bone 9 and `muzzle_flash` bone 56.
The original idle wrist relative to gun root is measured; the independently
authored Fire start differs by only 0.000348 matrix element. No guessed support
offset is added to the global hand or support table.

Body identity: 12444 total wedges; material 0 uses 12436 vertices, first index 0,
14990 triangles, palette `[64,53,54,62,57,59,60,61,55]`, root slot 1.
The 6-face hood section uses a different one-bone palette and cannot publish.
The original 44988-index INDEX16 buffer must match FNV32 `3842523959`.
It is read once per bounded resource identity, not every frame; resources are
released on Reset. No replacement geometry or material is created.

The witness is source `rootBind * skin * local`, published only after **two
successful original body eye draws** and state restoration. It is pawn/weapon
bound and expires after 150 ms. Support solves the complete wrist transform:

`Ctarget = inverse(wristBind * wristSkin * local) * idleWristRelative * rootLocal * BuildGunC`.

The palette delta inverts the same ride that StereoPair actually submits.
No controller transform is applied twice. Matching TMD sections share a
same-frame target; unavailable/mismatched witnesses keep independent tracking.
Release restores the accepted path. The global `GloveLeftShift` setting is
unchanged; its parking nudge is excluded from this measured Seeker support
pose only, as it already is for accepted measured TMD support. No normal-weapon
support profile, free-hand path, sleeve geometry or accepted TMD input is edited.

Expected receipt:
`Seeker pose: source IB matched, body=14990 eyes=2 rootSlot=1; restored, witness published`
and `Seeker support: original idle wrist relative to rendered cog; shared glove/device target`.

## C. Seeker launch and Valkyrie accuracy

**Proven distinction:** Seeker is native `RvWeaponShared_RocketLauncher` with
`mWeaponName=ARSteerable`, not ordinary Volk/Spikeshot or a renamed grenade.
Original main-fire rocket has `mIsDrunk=true`; alternate rocket has
`mSteerable=true` and authored steering/time-dilation parameters. Perfectly
straight flight for both would destroy native behavior.

`native_seeker_projectile.inl` scopes a thread-local launch to the legitimate
local Seeker's native RocketLauncher ProjectileFire. Full native fingerprints
and actual x86 call sites are verified:

- `E87BA0` -> shared ProjectileFire `E86DA0` (558 bytes).
- `+440` / `E6A1A0`: native PhysicalFireStart, caller `E86E28`, output pointer
  and by-value FVector, `ret16`. Native original called first, then measured
  muzzle world origin substituted only for this scoped call.
- `+430` / `E87580`: GetAdjustedAim calls existing hooked AddSpread at return
  `E8764A`. The native randomized cone is rotated onto the measured named muzzle
  axis; roll and cone angle remain, native RNG/recoil/fire remain authoritative.
- `+598` / `E8FCC0`: native SpawnProjectile, archetype/origin/direction pointers,
  `ret12`. Read-only bounded receipt; original call and result are preserved.

The existing SingleLineCheck correction skips only auxiliary traces of this
already-corrected scoped launch. Subsequent projectile flight, native target
assistance, drunk drift, guided mode, damage and ammo are untouched. Invalid or
stale tracking/ownership/camera evidence forwards original behavior. Setup
access violations refuse correction, and SEH unwinding always restores TLS.

**Remaining runtime gate:** confirm both authored fire modes and final native
spawn direction in MAIN. Native post-AddSpread assistance remains authoritative;
the receipt logs final direction beside barrel direction. This is not a claim
that every later point of a drunk/guided trajectory coincides with the barrel.

Valkyrie reuses `weapon_accuracy_policy.h` unchanged: candidate 70% native
spread, with 50% support multiplier (35% combined at legacy global 100%). Native
spread at 100% is source-proven, but this does not prove it was the sole cause
of the user's perceived inaccuracy. Matching fresh weapon witness is required;
aim center, native recoil and other weapon profiles are preserved.

## D. Reload comfort

The existing independent settings already clamp to 16–20 cm. Candidate fragment
sets Valkyrie and Volk to **20 cm**, retaining default 18 in normal builds.
No FSM, socket coordinate, Grip-release, shoulder pickup or ammunition backend
change. Native clip-joint, support exclusion and one-transfer transaction stay
authoritative. Centurion is untouched. If 20 cm fails physically, measure the
socket rather than enlarging the bound further.

## Reproduce and test

Merge only `docs/candidates/seeker-back-slot.ini` keys into MAIN's accepted
profile after review. Do not replace the full INI. All three Seeker flags default
OFF; Valkyrie accuracy defaults 100%, insertion defaults 18 cm.

Private inputs stay outside Git. Reproduce joints/IB with:

```powershell
tools/reload/test-seeker.ps1
python tools/reload/audit_seeker_geometry.py --psk <original-psk> --psa <original-psa> --package <decompressed-weapon-package> --test-exe build/seeker-private/tests/test-seeker.exe --report build/seeker-private/geometry.json --index-output build/seeker-private/seeker.ib
tools/reload/test-seeker.ps1 -IndexAsset build/seeker-private/seeker.ib
python tools/reload/audit_seeker_native.py --exe <owned-exe> --text <archived-text> --dll spikes/view_matrix/d3d9.dll --report build/seeker-private/native-abi.json
python tools/reload/test_locker_compiled_abi.py spikes/view_matrix/d3d9.dll
```

Private source SHA256:
- PSK `2FB038B287EED9D7B7730ACD5E73D6B7913F154B69E1124E89B5110430DD416E`.
- PSA `0FE784A10C05FE32729A4385275D66730C256809F86A72FEDDCAC2134F6BB460`.
- Decompressed package `42CC9DF1622CC971DA506F72560F19E5061D0464D41CEC655DFC1E60EFD901ED`.

Offline results (not game evidence):
- Real slot adapter: 100 synthetic owned-selector ABI calls; full synthetic
  weapon/ammo bytes unchanged; invalidation, owner, cycle, pause, cinema,
  unknown selector, OFF and hook-failure cleanup PASS.
- Geometry: 15 rotation/scale skin-delta cases, both eye transforms, stale and
  wrong-weapon rejection, native cone/roll preservation PASS. Compiled constants
  vs original private joints: max element error `3.625e-6`.
- Real projectile adapter: original calls once, unchanged archetype/result,
  no correction for other weapons, missing camera, invalid/stale owner/pose,
  OFF/pause/cinema; SEH setup refusal and TLS unwind PASS.
- D3D9 **and** D3D9Ex: original Seeker IB identity, corruption rejection,
  cached lookup, unchanged bound IB, Reset/recreation PASS.
- `test-dual-weapon-tmd.ps1`: 18 x86 programs + wiring tests and OLED PASS;
  includes reload/socket boundaries, 30,000 next-weapon transactions,
  accuracy 10,000-sample comparison, holsters, hands and WeaponLocker.
- `test-tmd-short-gauntlet.ps1`: route, palette and 7110-face asset checks PASS.
  Actual standalone D3D9/Ex gauntlet draws: 2x7110, restored IB, 3304/3304 pixels.
- Diegetic HUD: six observed pose replays + 240 actual-atlas D3D9/Ex pixel cases PASS.
- NIS CPU/bytecode/pixel/wiring PASS; OLED D3D11 all presets <=1 LSB vs CPU PASS.
- TMD haptics: native archived-binary seven tests and C++14 baseline groups PASS.
- x86 PE32 `/O2` build and emitted Seeker/WeaponLocker ABI checks: final receipt
  recorded beside this handoff. No Steam or PSVR2 result is claimed.

## One MAIN PSVR2 acceptance session (no chat while wearing headset)

1. With a legitimately acquired Seeker, note ammo. Switch to a normal weapon;
   right Grip at lower-right back draws Seeker again. Repeat draw/store/switch,
   confirm ammo unchanged, then verify ordinary hip holster and reload.
2. Hold left Grip under Seeker, rotate both wrists; glove must contact original
   fore-end. Release must restore free left tracking/TMD without moving right gun.
3. Fire Seeker at a known surface: launch starts at the visible muzzle. Compare
   normal drunk flight with native guided alternate mode; do not treat deliberate
   later drift as a muzzle failure. Check receipt for unchanged native archetype.
4. Compare Valkyrie short bursts one-handed and supported at the same distance;
   check center/recoil/VFX, then insert Valkyrie and Volk magazines at ~19 cm.
   Release farther than 20 cm and support Grip must NOT insert or transfer ammo.
5. Change scene/load a save and verify slot reconciles real ownership; no phantom
   Seeker, duplicates or reload grants. Briefly check accepted HUD/TMD/Settings.

Recommendation: GO for MAIN code review and controlled candidate testing;
NO-GO for replacing the accepted baseline without physical confirmation.
