# Seeker PR60 back-slot follow-up (offline, 2026-10-25)

Base: `ca75fb37edb90471a0e363a07ce551864ba5bbdd`, integration/astra-final-side-candidate. Branch: fix/seeker-back-holster-evidence. No Steam access, deployment or game launch.

## Evidence and first failure

MAIN archive: `SingularityVR-astra-final-side/deployment/deploy-backups/20261025-pr60-psvr2-session/` (08:04:55–08:09:23). Log SHA256 `AD0A4DD62445BF6C3B0F287D6A3573D21A2FD1619CD6952F4B4A77622CF11AD5`; deployed DLL `98560D7413CBA7298321DA7EED85AC63ABCC457010C372295615780AAEBE0809`. All three Seeker flags were 1.

- **PROVEN:** lines 21464/21481/21579 etc., while ARSteerable is equipped, report `cameraPawn=00000000 nativePawn=141F5000`. The Seeker Read adapter exclusively required CameraOwnedPawn. It therefore refused before Bind, left back availability zero, and prevented projectile Local from becoming true. The accepted reciprocal native resolver already resolves the correct pawn in these same receipts.
- **PROVEN independent source defect:** Read requires MemberCache for Instigator and mWeaponName. Both were absent from g_wantNames. WantedIdx therefore returns -1 and PropOffsetUnder refuses. Even a successful camera read could never pass these ownership checks. Existing synthetic tests supplied offsets for unregistered names and concealed this defect.
- **NOT proven:** inventory loss on switch. The archive continues reporting ARSteerable in the actual chain, including after the ordinary selection request at line 23637 and as late as line 28996. No snapshot precisely brackets every switch/drop. Do not call disappearance the cause.
- **NOT proven:** an incorrect back zone. No old rising-edge distance/first-gate receipt exists. Zone remains exactly 18 cm right / 55 cm below / 32 cm behind HMD, 16 cm radius, yaw relative.
- **Projectile PARTIAL:** there are zero `Seeker launch` receipts, including zero opt-in-ready receipts. The corrected launch path was not demonstrated. Native normal rocket drift and alternate guided steering remain authored behavior; absence of correction cannot be blamed on either from this evidence.
- Support and save/load remain **NOT TESTED physically** despite pose/support code receipts. Valkyrie accuracy and Valkyrie/Volk 20 cm insertion retain the user's PASS.

## Minimal correction

1. Seeker ownership and hook-controller lookup reuse ReadNativeLocalPlayer's existing typed reciprocal camera/controller/pawn identity. No stale cached-pawn fallback; live class, reflected layout, Instigator, real chain, cycle and method fingerprints remain mandatory.
2. Register Instigator and mWeaponName in the existing name producer. No offset relaxation, ownership fabrication or inventory write.
3. Keep native heavy classification substitution restricted to the same two audited SwitchWeapon return addresses. Preserve native owned selection and all ammo/state handling.
4. Add bounded engine receipts (64 state transitions/request results), XR Grip receipts (48 rising edges), and projectile gate receipts (32 stage/identity changes). No GPU readback, per-frame logging or new hooks.
5. Native selection request is followed by a read-only observation: `selection-equipped`, `selection-timeout` (1.5 seconds), or `selection-context-lost`. The request log is never equated to successful equip.
6. Projectile behavior is unchanged except that the corrected shared ownership resolver can now unblock the existing launch machinery. Final spawn receipt includes actual origin and expected measured muzzle origin, final native direction and barrel axis. Do not judge subsequent guided flight from the launch vector alone.

## Expected MAIN receipts

- `Seeker back gate: stage=ready ... ownedSeeker=... hook=... available=...`.
- `Seeker back slot: retained owned item=...` at the native switch decision.
- `Seeker back grip: gate=accept ... distance=... radius=0.160 ...` or first refusal: context/tracking/busy/unavailable/outside. Busy fields identify each existing owner; hand is physical controller index.
- `selection-requested`, then **separately** `selection-equipped` with current object equal to target; timeout is failure.
- `Seeker launch: ... opt-in ready` and per-shot origin/expectedOrigin/direction/barrel. Gate receipts identify ownership, tracking, fingerprint or hook setup failure if absent.

The old log cannot prove every later gate. If the new read reports pawn-fields/manager-chain-field/item-next-field/weapon-owner-or-name-field, preserve its refusal and investigate that producer. No broad safety check was disabled.

## Offline validation

- Real native adapter: camera resolver deliberately returns zero; reciprocal native identity works. Name lookup now uses a generated header extracted from the **actual production g_wantNames table**, so the missing-name regression fails rather than being mocked away.
- 100 owned draw/store ABI transactions preserve synthetic native weapon/ammo bytes. Also check selection that returns without equipping, delayed confirmation/timeout, dead/wrong owner, pawn change, chain cycle, pause/cinema/OFF, unknown selector and failed hook cleanup.
- Four yaw poses and first-failed Grip reasons; unchanged zone/policy; 15 support/stereo/launch-cone matrix cases.
- Production projectile adapter preserves original fire/start/spawn, native archetype/result, cone/roll and TLS unwind; other weapons/stale/invalid/OFF remain forwarded.
- Seeker D3D9/D3D9Ex original-IB identity and Reset regression; critical dual-TMD/reload/accuracy/holster/WeaponLocker/OLED suite.
- Archived executable/native text plus actual new compiled DLL ABI regression; literal WeaponLocker `[eax+20Ch]` preserved. x86 PE32 /O2. Exact build hashes and results in sibling JSON.

Only L1/L2. No physical or save/load acceptance claimed. No HUD3D/TMD/haptics/texture/localization/accuracy/reload tuning changed.

## One short MAIN retest

With a legitimately acquired Seeker, note ammo; switch to an ordinary gun, then right Grip at the lower-right back to retrieve Seeker and repeat the gesture to return. Require retained ownership, both confirmed selections, no ammo change or duplicate. Fire once in normal mode and once guided, compare launch to muzzle and preserve later native steering. **Only if retrieval passes**, test left support/release, then a normal save/load and retrieval. If unavailable or missing, collect the first gate receipt instead of moving the slot or granting a weapon. Support/save-load stay UNTESTED until performed.

GO for MAIN review/controlled retest; NO-GO for stable promotion before physical acceptance.
