# MAIN â€” Seeker HD / rear reload acceptance, 2026-10-30

## Scope and identities

Integrated PR #79 source `8151b341eda4b2f21f6312356f7b94e892d81734` onto accepted MAIN `2e9d5f32` on `integration/main-seeker-hd-rear-acceptance`. No merge to main. Initial optimized x86 PE32 DLLs built from `c0d5b8f6f5aa16b6c27b615bd11e27cc8b5f66b0`.

- d3d9.dll: `14194A4A58A5DF2C59909083EEAFF1432E1D45313FB2766297123A12E171787C`
- dinput8.dll: `64C0CFE840E51B9B1D5D5A4730CE351EE8135229124A028DFECFC65D2F43E5B3`
- Private candidate root: `build/next-weapon-reload/seeker-main-20261030/`; exact per-stage INI, plans, asset hashes and build manifests are pinned there. Private game-derived outputs are not committed.
- All 68 upstream private-manifest file hashes verified. Runtime takes only five exact files from `SingularityVR-seeker-hd-rear-reload/build/seeker-private/final_handoff/`: `seeker_reload.svwa`, `seeker_clip.svrk`, `seeker_clip.svrt`, `AR_Steerable_d.dds`, `AR_Steerable_n.dds`.
- Diffuse DDS: `70a5be45ddabccd14c6a2d47c23b222257ecaf4ab55e46d03ee7960bec5a479f`; normal DDS: `ec0e2a8b0ad9e5d030bf4f024660c6eccc1ca7f67c898a86970746f52647801f`.

## Integration changes

Reused SIDE's original-skeleton animation and 410-triangle detachable magazine, existing manual reload FSM, existing native refill adapter and original-IB/material replay. GLBs remain offline sources; they are not parsed in the render loop. Production-owned D3D9 texture bridge preloads a complete diffuse/normal pair outside bind, releases/recreates it through existing Reset/ResetEx lifecycle, and does not install another SetTexture detour.

MAIN added one independent, default-OFF `[Diagnostic] SeekerNativeAmmoProbe=1` observer. It samples only an actually owned/current Seeker using the existing audited clip/reserve/capacity getters. Natural forwarded AttemptReload is invoked exactly once by the existing path; the observer never invokes reload/refill or writes ammo. An accepted request alone is insufficient: status 1 requires the exact capacity-limited transfer, equal total ammunition and unchanged pawn/weapon identity. Invalid context, unexpected changes or timeout cannot report conservation. Sampling is bounded to eight transactions/thread, 100 ms intervals and an eight-second transaction, on natural same-thread interpreter calls. Flag OFF/no pending transaction returns before polling the clock.

MAIN also exposed separate diffuse and normal matched/replaced counters on the existing bounded bridge receipt. An aggregate match count cannot prove that both identities were seen. Invalid scalar identity forwards the original resource. Expanded the existing session-asset allowlist by exactly the five Seeker files and exercised real backup/deploy/restore on fake Steam trees.

## Offline verification

PASS: private package/source rig/socket/material validation; original texture identity/hash and preload policy; four-weapon reload/carry/native-permit fixtures; Seeker rear-only corridor, orientation, cancellation and conservation policy; new native transaction observer; native ownership/retention/selector x86 fixtures; D3D9 and D3D9Ex native upload, pixel/resource binding, failed-pair fallback, Reset and COM lifetime; 10,000 cached binds with no DDS reads/uploads; MAIN texture bridge/shadow lifecycle; wrist HUD wall/glove/magazine pixel occlusion; independent hands/confirmed-shot haptics and startup regressions; compiled WeaponLocker literal `[eax+20Ch]` / FName ABI; guarded runtime-plus-assets restore, running refusal, corrupt backup/config refusal and absent-file restoration.

Both optimized PE32 builds PASS. These fixtures do not establish physical stereo/material quality, guided firing, real Seeker ammo support or FPS improvement. In particular, the cached 10,000-bind fixture excludes first resource classification/readback; no loading/VRAM/frame-time gain is claimed.

Logs: `build/seeker-{cpu-tests,main-bridge-tests,hud-tests,haptics-tests,startup-tests,asset-tests,final-lease-tests}.log`, `build/seeker-{main,bridge}-build.log` and private candidate manifests. The preparation helper uses private inputs; it does not generate replacement models.

## Accepted restore destination

- d3d9: `147C3297211CFA6CA6B964E29FBBD79538B7EE69D7BC155861B66F67B24BDC96`
- dinput8: `F58CB3A805E46C3D3F7E88137BD605AF21A8A8106C289385132F677A968E95FD`
- SingularityVR.ini: `81E9F33BE4D6A5A968DE0D96A45C59A262F2FDD3DB8ECF7DED89D40CF12A87E2`
- texture_override.ini: `6BD63F5291EFEDB8F7419067EEC909541982EBD6FF38F8E386765213244AB6C5`

Profile was reconstructed from the accepted promotion plan and every pinned installed preimage plus INI semantic setting reverified before deployment. Existing NIS/OLED, localization, menu, HUD, hands, weapon VFX and other reload gates are preserved. New files have absent preimages, backed/restored through the scoped asset tool.

## Controlled runtime gates

1. Stage 1: `TextureBridgeOwned=1`, `bridge_owned=1`, `seeker_material_mode=1`, `SeekerManualReload=0`, `SeekerNativeAmmoProbe=1`. Require **both** diffuse/normal match counts >0, replacement counts exactly zero, genuine owned/current Seeker and `Seeker native ammo: END status=1 accepted=1` with explicit conserved counts. If no native transaction completes, do not enable physical reload; use only the specific missing receipt to resolve the dependency.
2. Restore accepted baseline after exit, then stage 2 from that same baseline: only Seeker material mode changes to 2. Require both replacement counts >0, user material/stereo confirmation, wall/depth/reset/switch/level behavior and comparable frame-time evidence. Do not silently count a first classification stall as a cached-bind regression or vice versa.
3. Only after native transfer proof, prepare a separate plan enabling `SeekerManualReload=1`. One grouped PSVR2 check: Circle ejects rearward; shoulder Grip carries a visible stable magazine; rear insertion works while front/side fail; release confirms exactly one legitimate refill; standard/guided shots, support, holster, cancel and switching remain correct. Log/fixture results alone cannot mark physical PASS.

## Initial runtime sessions (historical)

User explicitly confirmed readiness and an owned Seeker. Stage 1 installed with exact built/deployed hashes, runtime backup `deployment/install-backups/20261030-170706`, per-stage asset backup and validated combined guard. Guard PID 30100, Steam launch PID 34728, actual game PID 27144. Before-launch window 1800 seconds, **no timeout during active gameplay**; after normal exit, guard restores runtime and five-file asset baseline with verified hashes. No gameplay input injection. Initial OpenXR heartbeat reported shouldRender=0 awaiting headset wake.

Native ammo, HD physical appearance, physical rear reload and frame-time comparison: **PENDING**. Do not distribute/merge/declare acceptance on this intermediate state.

### Stage 1 observed result

Actual owned Seeker: pawn `1C1A6400`, weapon `39056000`, native ownership chain count 3 and heavy hook `00E48760` ready. OpenXR `shouldRender=1`, valid captured frames and true-stereo receipts recorded. Final mode-1 texture receipt: **diffuse 4201 matches / 0 replacements; normal 4201 / 0; total 8402 / 0**. This proves both source identities and log-only forwarding in the real game.

Three native launch receipts preserved the physical origin and native guided projectile mechanism. No `Seeker native ammo BEGIN/END` appeared; the user reported no ammunition. Therefore the ammo contract is **NOT ESTABLISHED**, not a reload failure or permission to synthesize reserve. Physical reload remains OFF. Existing shared native code requires nonempty reserve; shared vtable identity alone still does not prove Seeker refill support.

After normal exit, combined guard restored all five asset preimages and four runtime files; four exact baseline hashes re-read and matched. Stage 2 then installed the same binary pair with only material mode=2. Backup `deployment/install-backups/20261030-171157`, guardian PID 30164, Steam launcher PID 32752. Both stages are independently bound to the accepted baseline, not to an unaccepted prior candidate. Mode-2 results and the subsequent correction are recorded below.

Mode-1 late gameplay sample: `perf: 89.9 fps (11.12 ms/frame)`; cumulative controllers-in-hand bucket 89.2 fps, missed 0.6%, worst 80 ms. Startup/standby/menu samples are separate and lower. This is descriptive evidence, **not** a 90 Hz guarantee or matched-scene performance non-regression result. No per-resource classification latency was measured.

### Stage 2 actual failure and bounded correction

Stage 2 is closed and the exact accepted four-file runtime and five asset preimages were restored, with verified hashes. Archived logs are in `build/next-weapon-reload/seeker-main-20261030/mode2/`. Mode 2 matched diffuse **8382/0** and normal **8382/0** (matches/substitutions). CPU and GPU pairs were READY, but **no HD substitutions executed**. HD runtime acceptance FAIL; no physical appearance claim.

The preload receipt identifies upload thread **38092 (94CC)**, while the real Present CPU receipt identifies render thread **38540 (968C)**. `Ready()` required the upload thread, rejecting the resources on the actual rendering path. This is a confirmed first violated eligibility contract, not a texture identity or asset-generation problem.

Correction `18f7dfc0`, fixture refinement `14433cdf`: MAIN Present explicitly registers its thread through lifecycle event 3. Registration does no D3D/file work. Binding requires that registered thread, exact owning device and the complete immutable GPU pair. Creation/reset remain in existing lifecycle callbacks; release/reset clears registration. No SetTexture loading/upload, global bridge ownership changes or reload enablement.

New real D3D9 and D3D9Ex fixtures create resources on one thread, register/bind both textures on another, inspect actual bound GPU resources, and reject unregistered/foreign-device/foreign-thread access. Calls are serialized; native device creation flags unchanged. Existing pixel, partial-allocation, missing/corrupt asset, Reset/ResetEx and cached-bind fixtures PASS. MAIN bridge fixtures and compiled Locker factory ABI PASS. Existing fixture intentionally forwards `E_ACCESSDENIED`; the two-thread test verifies preservation of this sentinel rather than inventing a successful original result.

Both x86 PE32 `/O2` DLLs built from `14433cdf`:

- d3d9.dll: `800FBA6499C0D225F701415245615DE436FBCA78A99FA6670BD3C65D8DDE21D0`
- dinput8.dll: `393E6EABB426375336BE939AA699E7B6ECAF2541E89412019596874FCEC7E0B3`
- Prepared private stage: `build/next-weapon-reload/seeker-main-20261030/mode2-render-thread-fix/`, material mode 2, physical reload OFF, accepted baseline unchanged. Source/DLL identities and exact five asset hashes pinned in that stage; earlier failed sessions preserved.

**Next specific gates:** real mode-2 receipt must show both replacements >0, followed by headset confirmation of original material effects/stereo. Native reload remains BLOCKED by missing legitimate reserve ammunition: no observed AttemptReload transaction/conserved transfer, no direct ammo changes permitted. A legitimately stocked Seeker/save is needed before enabling physical rear reload. Support, rear reload, transition/reset and matched-scene performance acceptance remain PENDING; no main merge or release promotion.
