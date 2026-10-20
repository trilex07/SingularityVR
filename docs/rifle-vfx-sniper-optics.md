# Rifle VFX / Kasimov grip and eye proximity (2026-10-20)

Accepted installation remains the user's promoted663a49a artifact7A1DE585…9495,
INI60224CA0…4ADC. Work is on `fix/rifle-vfx-sniper-grip-scope`, not main.
No deployment or Steam launch in this preparation.

## Observations, mechanisms, falsifiers

- User: no rifle VFX; sniper grip wrong with one/two hands; scope should activate
  near an eye. This is NOT acceptance of either new implementation below.
- Source proves only Centurion/Valkyrie/Volk select the weapon delta for the
  single right glove. Kasimov used the uncorrected controller delta. Its left
  support target was an unmeasured38/-5/-2UU offset. Opt-in
  `Input/KasimovAuthoredGrip=1` uses the existing original pose relationship and
  the same proven weapon-delta route, without changing calibration, scale,
  source geometry or other guns. Falsifier: both paths execute for11389 but
  the user's original gloves remain detached from the gun.
- The saved accepted session has stage9 for Valkyrie and `muzzleWitness=0`.
  `PrepareNextReload` previously rejected its insertion socket before publishing
  the independent muzzle point. Move muzzle sampling before that unrelated
  distance guard; keep all mesh/IB/layout/identity checks. This fixes the
  demonstrated witness dependency, NOT proof that native rifle particles draw.
- The old native-FX capture finishes on the first gun; family counts from that
  session do not identify a later rifle shot. New optional
  `Diagnostic/NativeMuzzleProbeGunVerts=11387` waits for Valkyrie before the
  existing bounded idle+three-shot capture. The filter affects diagnostics only;
  accepted particle rendering and exact three shader identities stay unchanged.
  No new fake flash, guessed shader register or scene-light edit. Rifle VFX
  remains BROKEN pending producer-specific trace/pixel evidence and correction.

## Original optics measurement and implementation

Read-only `tools/reload/audit_sniper_optics.py <original sniper PSK>` verifies:
source SHA25639316F6A2FDBEF91292CBA42F83B62D5A9D19C2255785D441681F559BDC852EF;
material1 has97 unique points/168triangles, entirely weighted to `cog_sniper`.
Native cog-space centre=(-.00000279783,-11.1806386,-3.49776761)UU;
extent=(1.643694,1.643692,.154182)UU. Muzzle is forward along root+Z.
No game-derived asset is added to Git.

`Input/SniperNearEyeScope=1` is default OFF. An audited body draw maps that
centre through bind/skin/local/BuildGunC and the inverse XR world map. Capture
the lens position/axis relative to the tracked aim controller. The native scope
hides the gun, so retain that rigid mount, not a150ms mesh-visibility lease.
Invalidate on pawn/weapon identity/context loss. Require either tracked eye
within relief1.5–24cm, radial4.5cm, forward dot>.96 and100ms dwell. Exit bounds
32cm/7cm/.90 reduce threshold flicker. These are interaction tolerances, not
claims of physical calibration. The current sampler reuses the enabled Kasimov
native reload asset path; the candidate keeps that accepted prerequisite ON.

The request enters the existing native AltFire input before knife/reload/menu
ownership gates. Native per-eye scoped view/reticle and ScopeActive continue
to own zoom/shot behavior. This is a proximity-triggered **native scoped view**,
not a newly implemented magnified picture-in-picture lens renderer. Other
weapons keep their existing L2 behavior. Falsifiers: wrong/lost mount, no native
scope despite active request, oscillation when the mesh hides, or no exit after
lowering the weapon. Require runtime and physical checks for all four.

## Verification / next MAIN test

L1: optics oracle; scope dwell/hysteresis/alignment/context/identity/hidden-mesh,
360 yaw/translated-eye poses and2000 XR↔world round trips; sniper-only grip
truth table; rifle-only capture wait without disabling other particles; existing
reload/hand/shadow regressions. L2 build must be clean x86 PE32 `/O2` with SHA.
Neither is L3/L4. Native rifle VFX is diagnostic work, not declared fixed.

Offline candidate prep uses `prepare_combined_followups.ps1
-RifleSniperFollowups` with current accepted profile, clean DLL/manifest and
current accepted INI. It changes only four listed keys and the DLL. Do not use
old pre-promotion change plans. Wait for a fresh `start` before guarded install.

One headset route, no chat: equip Valkyrie, stay still5s, fire three short
bursts into open space3s apart; equip Kasimov, inspect one/two-hand holds,
raise the actual scope to either eye, lower it, repeat, then pause/resume and
close normally. Read trace completion/overflow and lens/request receipts before
interpreting feedback. Retain the current145-pin accepted baseline for restore.

Delivery: clean source `c81d01c251ef84a01d8eb0dd3164d6b81c62490f`, x86 PE32
`/O2` DLL SHA256 `6ACB6194577254A7678C05542CE5B24146D41E6C284676B46C10BBF4F865EFE0`.
Candidate INI SHA256 `BC1848B9B317781FE10801368E995EC8F330C8B1D7ABC7890F7B625ED06E4F4C`.
Actual delta is3 INI additions (Probe was already1). Offline preparation fixture
27checks PASS; read-only145-file accepted-profile/candidate preflight PASS.
Artifact at `build/combined-followups/rifle-sniper-c81d01c` only; installed
DLL/INI independently hash-match promoted baseline. Native muzzle/hand/shadow
suite, full reload regression with original private caches, optics oracle and
updated scope test PASS. Build warnings inherited; no compile/link errors.
