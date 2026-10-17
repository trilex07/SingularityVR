# Three-weapon runtime integration (SIDE)

Active scope: Valkyrie, Volk S4 and Kasimov. Preserve the separate Centurion
cylinder implementation and every accepted non-reload system. No Steam access.

## Mechanism and falsifiers

The offline native mapper resolves the registered no-parameter AttemptReload
wrapper, derives its virtual displacement, then follows the constructors of
RvWeaponShared, RvWeaponShared_Shotgun and RvWeaponShared_SniperRifle. All three
vtable entries resolve to the same previously audited native implementation.
Use ONE existing hook, separate identity-bound permits, and normal XInput
Use/reload input. Never call ProcessInternal as ProcessEvent or write HUD ammo.
Different wrapper/method bytes, object/class, IB bytes or shader layout MUST
disable this experiment rather than guess an override.

Circle eject -> left-shoulder grip -> release at the actual magazine joint.
The native material remains. Freeze the observed pre-reload root during the
transaction/native animation tail (including the original separate scope/lens
palette), drive original child tracks, and collapse the rigid attached magazine
while absent. Restore ALL touched constants after both eyes. Derive the
interaction socket from original clip bind, observed root skin,
LocalToWorld and the SAME controller C used by StereoPair. Invert the existing
XR-to-translated-world map, with centre position, not camera actor location.
Falsifier: nonfinite/unreasonable socket or missing fresh render witness means
no gesture/native suppression. Reproducible L3 evidence still belongs to MAIN.

On insert, arm a bounded single-use native permit before the normal reload tap.
Only an observed clip gain with equal reserve debit AND consumption of this
transaction's native permit acknowledges completion.
No auto-refill, repeated permit or inventory edits. Partial/tactical reloads
retain UE3's original remaining-round accounting. Full clip/no reserve remains
ineligible. Lost context/weapon/tracking cancels and restores native visuals.

Evidence levels: CPU/ABI/GPU fixture and x86 PE32 /O2 build do NOT establish
Steam or physical PSVR2 correctness. New per-weapon flags default OFF.

## Implemented runtime path

`weapon_reload_dispatch.inl` is included by production `d3d9.cpp`, not a detached
prototype. It is called before knife/support routing, from the existing native
AttemptReload hook, inside native `StereoPair`, at overlay submission, and on
XR init/camera loss/focus loss/Reset. The THREE flags automatically enable their
own native guard and single normal XInput reload tap; Centurion flags/FSM stay
separate. No extra MinHook owner or device slot65 hook is introduced.
The bounded Present bootstrap independently resolves reload names, votes the
existing UFunction::Func layout and finds the cached HUD (at most12 attempts,
one per240 frames with a camera). It does not depend on Centurion probes,
CutsceneAuto or HUD overlays, and does not install an interpreter hook. These
read-only discovery walks are outside input/draw hooks; no guessed offsets.

Shader constant registers come from existing CTAB reflection. The full INDEX16
buffer must match the original FNV and exact count before any palette changes.
READONLY locking is enabled by removing WRITEONLY only on the three measured
buffer sizes, for both D3D9 and D3D9Ex; all COM witnesses release before Reset.
The body and second palette/index ranges come from actual cooked chunk tables.
Cache hash/shape/material/animation witnesses must agree before enabling input.

The socket is the actual original seated clip joint, not the old Centurion
authored offset. The carried/drop affine map retains original geometry, UV,
handedness and the current native gun/world scale. The cache's measured centre
is mounted 6cm ahead of the left aim pose; the shoulder stock centre is EXACTLY
the acquisition centre. Drops last1.2s and do not write inventory or collisions.
This is an authored interaction mount, NOT proof of a correct physical grip.

Original child movement extrema identify out/return frames: Valkyrie15/29,
Volk28/43, Kasimov79/101. Kasimov's original cog_sniper local Reload translation
range is 23.369/46.512/32.481UU, so forwarding the native reload without a root
hold would move the whole rifle. Root hold ends at the observed native permit
time + original animation duration +150ms; native execution/ammo remain UE3's.
Ack timeout is at least animation duration +1.5s, rather than a4s universal cap.

Cancellation restores native visuals and forwarding. A missing witness, stale
HUD observation, lock contention, unsupported version or lost context intentionally
leaves native behavior available; no claim of suppressing auto-reload outside a
verified context. Full-clip/no-reserve ejection is intentionally not supported.
The carried original albedo uses private overlay depth, not the game's complete
normal/specular/light material or hand/world occlusion; those are NOT accepted
visual claims. Native attached weapon materials remain untouched.

## MAIN integration / offline candidate

Integrate SOURCE from `feature/next-weapon-reload-assets` into CURRENT MAIN.
Do not overwrite newer accepted work with this SIDE DLL. Required private files
(never publish original game assets) for each `valkyrie`, `volk`, `kasimov`:
`Binaries/vr-assets/reload/<name>/<name>_reload.svwa`, `<name>_clip.svrk`,
`<name>_clip.svrt`. Prepared originals are in this SIDE checkout's ignored
`build/next-weapon-reload/prepared/<name>/`; see asset research doc for producers
and exact input/output SHA-256. Existing accepted dependencies must be preserved.

INI surgical edits only: `[Input] ReloadControlLayout=1`, then exactly ONE of
`ValkyrieManualReload=1`, `VolkManualReload=1`, `KasimovManualReload=1`; others0.
All three default0. Do not replace user INI or change accepted Centurion keys.
Right-handed/unswapped first slice: Circle eject; physical LEFT grip for ammo;
Cross health/right-stick-up jump come from the already existing layout.

Offline handoff folder generator refuses dirty source/build receipts, wrong
PE/architecture/optimization/DLL SHA, wrong private caches and output outside
this checkout's ignored lab; it NEVER discovers or touches Steam:

```powershell
python tools/reload/prepare_runtime_candidate.py --prepared-root build/next-weapon-reload/prepared --output build/next-weapon-reload/candidate-<source>
```

It copies the SIDE candidate +9 private files, SHA manifest and a JSON settings
proposal with all three opt-ins OFF, NOT an installed INI. Installed/accepted
identities are UNKNOWN to SIDE. MAIN owns preflight, backup/hash, test lease and
restore, with no file replacement while Singularity.exe is running.

## Verification and physical route

Runner: `tools/reload/test-centurion.ps1 -NativeEvidence <private-original-evidence.json>
-PreparedRoot <this-checkout>/build/next-weapon-reload/prepared -Gpu`.
It runs existing Centurion/knife/input regressions and the three-weapon policy,
native detour, original hierarchy, socket/affine and native-root scope fixtures;
the GPU process loads SYSTEM d3d9.dll and never the game/proxy/OpenXR.
Offline native inheritance evidence is produced by `inspect_next_weapon_native.py`.
The package also proves Shotgun/Sniper inherit Shared and do not override the
relevant script states/functions. Final clean build/SHA receipt follows in STATUS.

MAIN first requires `next reload` readiness with real object/native/render
witnesses, then BLOCK without permit, one PERMIT after insert, and
`native-ammo-ack` with `consumed=1` plus matching clip/reserve debit. A counter or
draw count alone is not proof. If readiness fails, investigate the missing live
producer via existing bounded traces instead of loosening identity checks.

Three-sentence test, ONE weapon/session, no mid-headset chat:
Load a save with that weapon, shoot partially and empty, confirm no automatic
reload while ready and check the existing left knife/right holster still work.
Press Circle once, see its original magazine eject, take ammo with a NEW left
squeeze behind the left shoulder and release it at the gun; verify a single
refill, correct reserve debit and stable weapon/controller alignment without a
closing flick, then repeat a partial refill if possible.
Cancel once by pausing or switching weapon, check pause/Options/stereo return,
close normally and let MAIN verify restoration; report visual/fit/lighting or
performance failures explicitly, without calling this physically verified yet.
