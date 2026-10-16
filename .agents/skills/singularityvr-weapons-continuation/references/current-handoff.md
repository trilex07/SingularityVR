# Current handoff — 2026-10-16, snapshot only

NEWER continuation: user reports native auto-reload guard blocked3 calls and
asks for original visual motion plus SIDE c70b36f review. Read
`docs/centurion-cylinder-animation.md` and `docs/centurion-cylinder-size.md`.
Default-OFF native clip/release palette animation is now implemented with a
private original PSA cache, exact cooked IB-content witness and scoped restore;
runtime/physical validation pending. Cooked clip slot0 is NOT PSK bone57.
16cm insertion accepted; negative-left-sign rationale rejected in favor of
current tracking yaw latched per transaction and magnitude22. Detached spare
scale follows gun/world calibration; no full spare material or individual shell
particles yet. These new receipts supersede older NOT DONE animation/size
paragraphs below, not the Steam ownership boundary. Never infer deployment.

Native continuation now supersedes the older "no ABI/guard implemented" scope:
read `docs/centurion-native-reload.md` first. Offline registration->wrapper->class
vtable->native method and empty BeginFire/fallback path identified. New
`CenturionNativeReloadGuard=0/1/2` OFF/observe/guard experiment, not yet Steam
verified; no actual cylinder opening/ejection animation added. Existing receipt
below identifies the earlier gesture DLL only; read latest STATUS/build manifest
for the new artifact, never assume it is installed.

Latest native source `a2b77ac5f16fc8218799443ce26e17644ea49e71`: L1 13 CPU/31
Python + archived original and own-process x86 detour PASS; clean L2 /O2 PE32.
DLL SHA `89A5209153F194ED858553EAE2C3B99CC1BC2E52C9DD9B1DEBD9C43977458339`.
Private candidate manifest updated/verified, no INI/Steam writes. Native mode1
observation then mode2 guarded test are MAIN's next integration gate; L3/L4 UNKNOWN.

## Location and delivery

- Repository: https://github.com/trilex07/SingularityVR
- Own stopped SIDE checkout: `C:/Users/Trilex/vr-mods/SingularityVR-centurion-grip-shoulder`.
- Pushed branch: `fix/centurion-grip-shoulder`; implementation `80a5e34`,
  test receipt `dfeb397`. Later documentation-only handoff commits may follow.
- Base main `644be25` plus existing Centurion through `e7a0c89`. MAIN integration
  was elsewhere (`SingularityVR-main-plates`); do not edit it or assume it stopped.
- PR creation returned GitHub403; compare form, NOT an existing PR:
  https://github.com/trilex07/SingularityVR/compare/main...fix/centurion-grip-shoulder?expand=1
- No Steam files were changed by this SIDE. Installed/accepted/restore hashes
  were NOT inspected this session. Candidate hashes below are NOT that baseline.

## User's latest decisions

Centurion first; user currently has its save. Preserve hip holsters. Fresh ammo
comes from BEHIND physical LEFT SHOULDER, not the knife zone at the belt.
Physical PSVR2 X/Cross -> health pack; right-stick UP -> jump; Circle -> release
cylinder latch. Desired sequence: Circle, sideways gun motion to open, muzzle
down to eject, take fresh cylinder from shoulder, insert, flick gun to close.
Centurion uses a SIX-CHAMBER CYLINDER, not a rectangular pistol magazine.

User tested the OLD prototype: auto reload still ran, ammo grab selected gun or
knife, mainly white/yellow squares visible. This rejects the old input behavior;
the NEW follow-up has NOT been physically tested.

## Implemented vs unfinished

New follow-up: yaw/head-relative left shoulder, 650ms grip acquisition window,
reload priority over newly acquired support at its exact target, real raw grip
history/no fake release into knife. Separate default-OFF controls and cylinder
gesture FSM. Jump requires neutral rearm; menu select/back, crouch and R3 wheel
retained. Cross hold-laser shortcut disabled under the new layout.

Original private cylinder/albedo preview now appears at shoulder in NeedAmmo,
then in left hand in Carrying. Calibration boxes require explicit marker flag.
XR sync/camera loss cancels pending transactions/taps. Read-only native wrapper
probe captures at most eight distinct128-byte blocks; rejected inputs log max32.

NOT DONE: native auto-reload suppression, actual cylinder opening/ejection mesh
animation, dedicated native begin/refill/end reload ABI. Native cylinder remains
on gun. Preview is unlit/albedo-only, not full native shading or fitted grip.
Optional backend is only shared native Use/reload XInput tap. Do not call this
complete physical reload. Do not merely enable the flags and claim auto fixed.

## Resume implementation here

1. Verify source/branch and read `docs/centurion-grip-shoulder.md`.
2. Trace native auto-reload producer before suppression. Existing current-weapon
   binding probe gives UFunction/Func identity and native wrapper bytes; disassemble
   captured wrapper/callers offline. Declaration alone does not establish ABI.
   MAIN can obtain this bounded probe during its next authorized integration run;
   SIDE does not launch the game or ask user to attach a debugger.
3. Prove intentional reload vs empty-clip automatic request, native ammo commit
   and animation ownership, then add the smallest guarded backend/visual change.
   Preserve measured gesture arbitration and new user-approved control layout.

Read-only HUD observers: named `mClipAmmo/mBinAmmo/mClipSize`, identity-bound,
250ms expiry, sampled at10Hz. No ammo writes; acknowledgement requires clip gain
equal reserve loss. Shared Use tap is not a dedicated reload call.

Offline original Reload animation:53 frames/24Hz, refill notify at1.47052908s
on RvSharedPawn. `RvWeaponShared.AttemptReload` export16381 has bool ReturnValue;
613 is its package FName index, NOT a native opcode. No guessed interpreter calls.

## Navigation and verification

- Runtime: `spikes/view_matrix/d3d9.cpp` (`UpdateCenturionReload`, `SyncXRInput`,
  `ObserveCenturionReloadBindings`, `DrawCenturionReload`).
- CPU policies: `centurion_reload.h`, `reload_controls.h`, `centurion_clip_preview.h`.
- Run inspected `tools/reload/test-centurion.ps1`:11 CPU executables +27 Python
  tests PASS. Extra `tools/knife/test-knife.ps1`:6 CPU +14 Python PASS.
- Build `spikes/view_matrix/build.ps1` WITHOUT `-Install`, commit source first;
  x86 PE32 `/O2`, SDK resolved by `tools/Resolve-OpenXrSdk.ps1`.
- Clean source `80a5e340b79da773912d3ec55d64dbe27e3cf590` built DLL SHA256:
  `B5352D3C9DF60F705904A0AE992EDF478E96650A1E7B6888F4B51F3985384CAA`.
- Private ignored payload: `build/centurion-reload/candidate/`, exact manifest
  `centurion-preview.manifest.json`. Original mesh/albedo exist locally, not Git.
  NO INI staged. L3/runtime pixels/L4 for follow-up UNKNOWN; full reload not ready.
- Candidate flags all default OFF: `CenturionManualReload`, `ReloadControlLayout`,
  `CenturionCylinderGestures`, `CenturionClipPreview`, `CenturionReloadMarkers`,
  `CenturionNativeReloadTap`, `CenturionReloadProbe` (section Input).
  Scoped proposals/test route are in the feature doc; not deployment permission.

Related but separate: `fix/native-glove-cuffs`, own checkout
`C:/Users/Trilex/vr-mods/SingularityVR-native-glove-cuffs`, receipt9e6ab03.
Full original cuffs/closed wrists prepared L1/L2, not headset-verified. Inspect
its `docs/native-glove-cuffs.md` if asked; do not mix it into a reload test.
Other-weapon CPU prototypes exist on `feature/centurion-manual-reload`, not wired
in this follow-up. Do not rewrite them or claim runtime completion.
