# Three-weapon runtime integration (SIDE)

2026-10-17 newer follow-up: `docs/next-weapon-refill-only.md` adds separate
default-OFF ReloadRefillOnly: native conserved refill without starting the full
Reload state,180ms render-only seating tail, physical-left shoulder and block-only
holstered ownership. Older native-animation timing/receipts below describe flag0.
The new experiment has its own source/L2 receipt; neither is physical acceptance.

Active scope: Valkyrie, Volk S4 and Kasimov. Preserve the separate Centurion
cylinder implementation and every accepted non-reload system. No Steam access.

Delivery: draft PR https://github.com/trilex07/SingularityVR/pull/45 against main,
created via the authenticated GitHub browser after connector create returned403.
Branch `feature/next-weapon-reload-assets` pushed; NOT merged. Initial creation
was conflict-free; that is not a current mergeability assertion. MAIN integrates/
reviews and owns L3/L4. Native-material follow-up below supersedes the original
albedo-only presentation when its separate default-OFF opt-in is enabled.

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
AttemptReload hook, inside native `StereoPair`, at legacy overlay submission, and on
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
Legacy presentation (`ReloadNativeMagazine=0`) uses original albedo/private
overlay depth, NOT full native lighting/occlusion. See the independent opt-in
below; neither path has physical acceptance for these three weapons yet.

## Native-material magazine follow-up (user-requested PR45 update)

Mechanism: the reported Centurion spare problems reveal the same limitation in
these three weapons: the overlay has a separate unlit albedo and clears PRIVATE
depth. An arbitrary colour multiplier cannot fix that ownership/occlusion bug.
Hypothesis: replay the original native gun DIP inside EACH existing stereo-eye
callback, keeping only its measured rigid magazine bone alive. Retarget that
bone to the already-tested stock/carry/drop affine map; collapse every other
bone to the SAME point so no body/trigger/bolt triangle remains. Exact original
IB/CTAB/current-pawn/weapon/freshness/scene/focus checks remain mandatory.

`[Input] ReloadNativeMagazine=1` (default0) selects this new presentation for
whichever ONE weapon's manual reload flag is enabled. It does not enable or
change the independent Centurion experiment/sequence. All three still use
Circle eject -> physical-left shoulder grip -> insert/release, NO closing flick.
Native ammo permit/ACK/control logic and original child/root/scope animation
are unchanged. Existing original caches are reused; no new art/audio required.

The production route now retains native VB/IB, UV, normals, material shaders,
textures, pixel constants, render targets and depth/stencil state. ONLY the
measured bone registers change, restore after EACH replay even on partial
upload/draw failure, then the entire saved palette restores after BOTH eyes.
Stock, carried and dropped magazines share this path; old private-depth overlay
returns immediately under the opt-in. Extra DIP cost is bounded to at mostTWO
copies per eligible pass/eye (new stock/carried plus old falling magazine), no
draw-time allocation/file I/O/material or depth-target switch. Attachment/scope
chunks never replay as magazines; ambiguous segmented/hand-palette draws refuse.

Falsifiers: cache-metres -> bind -> native-skin/local/controller -> XR does not
roundtrip for each actual clip slot; other bone triangles remain; material/
texture/depth state changes; palette survives a failed second-eye draw; weapon
or context swap reuses a stale witness. OWN D3D9Ex synthetic material/depth
fixture checks all THREE clip slots, BOTH eyes and Reset; it is NOT a real
game shader or scene. Same bound material does not prove all position-dependent
lighting, shadows, hand/depth ordering or scene occlusion is correct. Native
lighting appearance, cast shadows, alignment and performance remain MAIN L3/L4.

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
For the native-material round add ONLY `ReloadNativeMagazine=1`; legacy control
is0. Keep every accepted Centurion/INI/asset key untouched.

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

Current native-material receipt (2026-10-17): implementation8a3f603 plus candidate
provenance correction, clean source `8896ff9f6a640bfde3246d04c4efdca014da4eff`.
Seventeen CPU +TWO own-process D3D9Ex GPU executables PASS; all19 independently
checked x86 PE32. All57 Python tests PASS. Adds12960 stock/carry/drop native-skin
roundtrips across the THREE original magazine palettes, transactional failures
and palette restoration, while retaining7506 original fractional child poses,
4320 native socket checks and30000 own-process native transfer transactions.
Synthetic shader/triangles shared-depth test gives10609/10609 visible pixels
per profile/round, zero wrong-colour/occlusion/body pixels, both eyes and two
resource/Reset rounds. It does NOT prove game lighting, shadows or hand ordering.

Clean x86 PE32 `/O2` DLL3193856 bytes, no performance instrumentation, SHA-256
`F5533704CD997DDB45F9EB5C1FC8514C824CD6CF0DC20D331101A1D5FAD892C6`.
Build passed the project's existing static-analysis gate; unrelated pre-existing
warnings remain in its log, so this is NOT a warning-free analysis claim.
Local/private handoff:
`C:/Users/Trilex/vr-mods/SingularityVR-next-weapons/build/next-weapon-reload/candidate-8896ff9/`.
All11 payload sizes/SHA-256 independently verified; manifest SHA-256
`C1862892766255AF9E75762AC161EA000A86C8945951B21198F2E7A6860527EF`,
settings proposal SHA-256
`11A19C0181A8408C56AE4CFE0FAE6EF98C3A6E4A7F1E333030A532069A4CFECC`.
Manifest distinguishes actual main ancestor513013ab from the moving origin/main
snapshot. Older folders (including candidate-8a3f603) are superseded and are NOT
restore baselines. Logs: `build/next-weapon-reload/native-material-tests.log`,
`native-material-final-build.log`; the final Python provenance test rerun has7
tests (the earlier full log has6). Documentation-only later commits do not
change binary provenance. No Steam/config/installation inspection or writes;
L1_PASS/L2_PASS only, L3/L4 UNKNOWN. MAIN still integrates SOURCE, never blindly
deploys this SIDE DLL over newer accepted work.

Previous albedo-only offline receipt (2026-10-17), superseded for the new
native-material opt-in by the later follow-up receipt: production source
`4c965dd65e765b8a77aba701bfe4ac35774ca84a`, clean tree, x86 PE32 `/O2`, no
performance instrumentation, DLL3190272 bytes, SHA-256
`EDBE3F35CCE9869C166AE08B4F22B3E99DBF1B5AD79363B986791065CAF2649E`.
Seventeen CPU executables, one own-process D3D9Ex fixture and55 Python tests
PASS; all18 executables independently verified PE32/x86. Includes30000 real
MinHook detours of OWN fake inventory for three objects,7506 original fractional
track poses and4320 socket/affine roundtrips; none invokes the game.
Actual original albedo pixels in the GPU fixture's left/right images:
Valkyrie302/302, Volk307/307, Kasimov210/210, with state/failure/Reset checks.

Private handoff directory (absolute, local only):
`C:/Users/Trilex/vr-mods/SingularityVR-next-weapons/build/next-weapon-reload/candidate-4c965dd/`.
All11 payload SHA-256 values independently checked after packaging; manifest
SHA-256 `8796250D6C844B0F179A56B87E2A0BF3C60EDD5C72B37370678711823FD0D4C2`.
Logs: `build/next-weapon-reload/runtime-tests.log`, `runtime-build.log`;
native static proof: `build/next-weapon-reload/native-three.json`.
Earlier candidate folders are superseded, not accepted restore destinations.
Documentation-only receipt commits after the production source do not change
this binary's provenance. Installed/accepted/restore identities were NOT read;
ZERO Steam/config/assets deployment. L1_PASS/L2_PASS only; L3/L4 UNKNOWN.

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
For native-material presentation pass the carried magazine in front/behind the
gun and glove in both eyes; check matching colour, correct occlusion, unchanged
native gun/scope/hand appearance, and no persistent/floating full gun copies.
Cancel once by pausing or switching weapon, check pause/Options/stereo return,
close normally and let MAIN verify restoration; report visual/fit/lighting or
performance failures explicitly, without calling this physically verified yet.

# MAIN PSVR2 follow-up, 2026-10-18

The first Valkyrie Steam trial loaded OpenXR and validated the original clip
assets (`CPU assets=READY`), but Circle did not begin manual reload and native
`AttemptReload` forwarded twice for `ARMachineGun/RvWeaponShared`. The only
`kind=none` sample preceded rifle equip; it is **not** evidence of a class
identity failure. The next build logs the individual identity, context,
ammo-freshness, render-witness and native gates on bounded Circle edges. Do
not bypass any gate based on this trial. Physical reload remains unverified.

The same trial exposed a separate weapon-wheel bug: `g_wwSeen` called
`InterlockedCompareExchange` with the exchange/comparand reversed, so the
Valkyrie was logged as newly seen every frame but never stored. This has a
direct source-level fix and a regression contract. The wheel also now begins
with no orange selection until actual hand/stick movement; sector zero remains
selectable. `ValkyrieSingleGrip` is a default-OFF, one-hand A/B reusing the
accepted pistol's weapon-delta target for the right glove; its visual effect
still requires PSVR2.

## MAIN physical follow-up, 2026-10-18 (VFX/holster candidate)

The guarded Steam test of DLL `56CFBCE1…DFFA8F` confirmed with the tester:
native muzzle VFX returned, Valkyrie one-hand grip improved, accidental holster
placement was acceptable, and Circle weapon-wheel selection worked. The
installed accepted DLL/INI and all nine reload assets were restored and their
hashes checked after the game closed. These observations do **not** accept
Valkyrie manual reload or the prompt rewrite.

On Circle with Valkyrie equipped, the runtime reported a valid weapon context,
identity, fresh ammo state, native refill method, and assets, but
`next reload witness` remained `valid=0`. The render-socket witness is a safety
gate, so bypassing it to force ammo changes would be unjustified. The next
default-OFF diagnostic records the first rejection at each stage of
`PrepareNextReload` (draw part, gameplay context, visual snapshot, shader
layout, index buffer, palette and socket reconstruction). **Hypothesis:** the
Valkyrie mesh draw or its transform fails before socket publication.
**Falsifier:** stage 10 (witness published) appears during gameplay while the
Circle input still cannot begin reload; then investigate the input/FSM consumer
instead. This diagnostic has not yet been run in Steam.

The tester still saw `Pad 3` instead of `O`. An earlier `[reload]` producer log
showed `(O)`, so it does not prove ownership of that visible `Pad 3` prompt.
The diagnostic now also logs exact `Pad 3` text at the localization producer;
do not globally remap this token because other actions share numbered pad
glyphs. The VR Settings laser was not reported in this trial.

### 2026-10-19 Valkyrie headset trial

The installed test DLL was `FE79F27D...`, with original reload assets temporarily
installed. Steam/OpenXR ran at 90 FPS before gameplay. The tester confirmed that
the new bilingual instruction appears after Circle on Valkyrie but **does not
replace** the native `Pad 3` prompt; Centurion has no equivalent transient
instruction. Valkyrie's Circle did not eject a magazine, while native button
reload remained available. The Steam screenshot looked correct to the tester,
including its eye/HUD composition; this does not validate the magazine path.

Runtime evidence: exact Valkyrie object, fresh 26/144/30 ammo, native method,
tracked hands and Circle edges all passed. `PrepareNextReload` identified the
original Valkyrie body draw, valid palette and reconstructible socket, then
rejected it at **stage 9** because the reconstructed socket was over 0.75 m
from the right controller. The read-only input consumer therefore correctly
reported `witness=0`, `valid=0`, event none. The observed 1.44-1.58 m
`socketDist` in that line is the *left hand to an unset socket*, not the stage-9
right-hand distance. Do not loosen the 0.75 m gate or block native reload yet.
Next diagnostic logs four bounded samples of the two XR points and root/local/
controller translation to identify the mismatched coordinate space. An offline
roundtrip proves the algebra for constructed matrices, but not the live UE3
matrix producer.

The native text producer was identified as `Keyboard/Button2` -> `Pad 3`; it
appeared both with Centurion and Valkyrie. That generic label alone does not
prove a reload-only semantic, so a global substitution to O remains unsafe.
After the test, exact accepted DLL/INI/UI hashes were verified and all nine
temporary assets were removed. Both automatic restore helpers had failed on
this run (a relaunch race and a PowerShell array wrapper respectively), so MAIN
performed a guarded manual rollback and fixed the asset receipt parser. The
restore leases now require 30 seconds of stable process absence before acting.

### Offline continuation after the tester signed off

- Centurion now uses its own four-line bilingual tutorial recipe after Circle,
  in the same bounded per-eye hint renderer. It retains the original reload
  mechanics. This is additive guidance, NOT proof that native `Pad 3` is gone.
- Complete `test-centurion.ps1` passed with `-PreparedRoot` pointing to the
  original `SingularityVR-next-weapons/build/next-weapon-reload/prepared` assets.
  This includes all three weapon animation/socket/replay profiles, refill-only
  first-shot palettes, own-process x86 ABI and synthetic GPU replay. Initial
  local fixture run lacked `.svwo` oracles; that was not a runtime fix failure.
- Eleven mirror/hint source-contract tests pass. Optimized x86 PE32 DLL builds
  successfully (existing warnings remain), SHA-256:
  `19BE9208CC076A37BC23AAE38996382C05675B4D82220D1899F583A6B9328B82`.
- Stage-9 logging now includes all four affine rows of bind/root/local/controller,
  at most four samples per weapon, two seconds apart. These are sufficient to
  reproduce the failed composition instead of guessing from translations.
- The last session has no such full matrix snapshot. The actual native-space
  mismatch and semantic native prompt replacement remain open. Do not remove
  safety gates or claim the three weapons work from CPU roundtrips alone.
- No Steam launch/deploy: the user ended physical testing for the night.
