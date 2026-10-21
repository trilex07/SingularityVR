# MAIN handoff — audited SIDE TMD / effects proposal

2026-10-21. SIDE-only review of `a437c8c` and `7d3a762` on
`side/enemy-detail-proposal`, based on `origin/main` `8ffa2d1`.
No Steam process access, launches, deployment, restore, saves or installed writes.

## Follow-up: replacement native TMD candidate

User requested implementing TMD after the review. The unsafe original changes
remain rejected; this branch now adds a different default-OFF `TmdNativeAim`
getter-only candidate. It changes renderer source only to publish its existing
TMD matrix, not to select shared gun/controller aim. See
[`tmd-native-aim.md`](../tmd-native-aim.md) for original-binary ABI evidence,
failure gates, offline tests and MAIN's physical route. Existing effects scope
and its default-OFF gate are unchanged; test these two features separately.

## Decision

Do **not** merge the original SIDE branch wholesale. It also contains `9c75c68`
(the global gore/CDO experiment), and its base predates accepted renderer work.
The initial review commit7a525a7 left `spikes/view_matrix/d3d9.cpp` and its
INI byte-for-byte equal to the base. The follow-up only adds the getter candidate
described above; shipped INI remains unchanged. It does not supersede MAIN's scoped gore,
aiming, cutscene or audio candidates.

### TMD aiming: original implementation rejected; replacement awaiting L3/L4

1. In `UpdateFromHeadset`, SIDE selects OffHand for shared `g_aimYawUU`,
   `g_aimPitchUU` and engine targets whenever `TmdRaised()`. These feed ordinary
   weapon aim too. A pose state is not an action/trace ownership witness.
2. Its `Hook_SingleLineCheck` edit switches the existing **weapon-fire window**
   to the TMD pivot. The fire-function list is FireAmmunition/BeginFire/StartFire;
   no demonstrated native TMD recipient/trace is added. Holding TMD while firing
   the right-hand gun can therefore redirect a gun trace to the other hand.
3. The modified VFX block constructs `singularity_vfx::VfxFireEvent`, the mod's
   muzzle mailbox. It does not relocate native Chronolight/Deadlock/impulse
   emitters. The report's claim that these are all repaired is unsupported.
4. `TmdAimDir` uses TraceSign and wantYaw/wantPitch, whereas `BuildTmdC` uses
   GunSigns, TmdYaw/Pitch/Roll trims, GunViewFrame and BuildGunTransform. It does
   not derive an axis from the mesh transform; alignment is not established.
5. `TmdOnOffHand=1` already defaults ON on main for mesh ownership. It is not a
   new default-OFF gate for the proposed aiming experiment.

Required next step for MAIN: retain existing gun aim. Reuse the existing
`tools/fx/audit_deadlock_backend.py` contracts (HandleTMDInputClientside,
StartFirePower/StopFirePower and RvTMDPower_Deadlock); identify the actual live
power object, owning pawn, input edge and resulting trace/emitter in a bounded
read-only capture. Classify **action**, not raised-hand state. Reject stale pose,
wrong recipient, AI, menu, cinema and gun-fire nesting. Only then add an opt-in
scoped TMD transform, shared by its actual emitter and trace, and test both hands
pointing in opposite directions while the gun fires. Falsifier: gun or camera
follows the off hand, or TMD beam and hit disagree. No unlocked save/runtime
evidence was manufactured in this SIDE task.

## Corrections delivered: effects staging and opt-in injector

- Replaced the machine-specific generator with an explicit offline CLI. New
  output directory must be under this checkout/build; no overwrite or Steam path.
- All selected original textures must match dimensions, DXT format and exact
  compact mip-0 FNV64 from the runtime registry. Source SHA256 and output SHA256
  recorded. Source absence, identity drift and aspect changes are fatal before
  output creation, not silently skipped.
- Correct compressed DDS `DDSD_LINEARSIZE`, full top-mip byte size (SIDE used a
  row pitch), complete mip chain down to 1x1 and exact byte budget. Independent
  Pillow decode; DXT5 alpha regression test. No conversion to uncompressed DDS.
- 11 explicit exact targets. Added/verified bsplat_decal_d and
  t_blood_splat02_d hashes using local owned originals. No guessed hashes.
- Three SIDE outputs have no target registration on this base: smoketrail_01,
  tracer2, bulletImpactFlesh_d. They are excluded, not falsely labelled integrated.
- Blood/metal original-resampling fallback is labelled **no new detail** in the
  manifest. Not every output is square; aspect is preserved with max edge 2048.
- New startup-only `[general] effects_remaster=0` in texture_override.ini.
  Both `effects_remaster=1` and existing `remaster_apply=1` route these 11 names
  to `remasterdir`; existing group switch still applies. New blood matches fail
  closed when effects flag is OFF. Legacy already-supported FX route unchanged.
- Applied this routing in both the bridge callback and standalone SetTexture
  path, preserving hook ownership, return values and cache/lifetime semantics.
  Standalone additionally rejects replacement format mismatch like the bridge.
- Existing glyph/unknown/exact-name tests remain; no SIDE headless skip imported.

## Reproduction and artifacts (private, local)

```powershell
python tools/fx/test_effects_staging.py
python tools/build-2k-effects.py --originals C:/Users/Trilex/vr-mods/SingularityVR-assets-local/textures/original --enhanced C:/Users/Trilex/vr-mods/tex_tiers/tier_8K --output build/effects-review/candidate-v2
./spikes/texture_cache/build.ps1 -OutputDirectory build/effects-review/injector
```

Prepared assets: `build/effects-review/candidate-v1/manifest.json` plus 11 DDS.
Game-derived assets remain ignored/local; Git contains tools/tests/identities only.
Build receipt: `build/effects-review/injector/dinput8.build.json`.
The first build receipt records a dirty source snapshot with per-input hashes;
final clean-commit rebuild receipt is authoritative for MAIN.

## Gates and test route

L1: DDS validation/alpha/negative-input tests and exact-scope CPU test.
L2: x86 PE32 `/O2` dinput8, static format analysis, resource-native-cache CPU15,
GPU8, timing7/disabled, actual-hook forwarding9 and bridge GPU plain/Ex checks.
These are own-process synthetic GPU tests, not Steam or game-pixel validation.
No new d3d9 renderer is built because this branch does not change it.

MAIN must stage DDS only into a separate remasterdir candidate, preserve the
accepted renderer/INI and other remaster assets, and use normal verified backup
and lease rules. Test effects separately from TMD: same pistol/rifle, 3 shots and
one impact/blood scene; compare both eyes, alpha edges, smoke, exposure and FPS.
Collect exact `BRIDGE MATCH` / `SUBSTITUTE` names; missing matches falsify runtime
integration even if files exist. Effects quality, TMD aim, TMD emitter placement
and TMD gore remain **NOT PHYSICALLY VERIFIED** by this branch.
