# TMD sky, gravity quad and hand alignment

## Frozen starting point

User-confirmed Steam DLL: `7AD356F832ED84E2F599733BE58553BD661381C6C2FD51B4ACDEE25116F38E86`.
Branch starts at `fc6a6591` (binary-producing source `9488a26a`).
The stale `maintenance/workspace-cleanup-20261027` checkout is not a build source.
No Steam files changed during preparation. Popup pointer correction and the
current right-wrist/weapon attachment are preserved.

## Evidence and unresolved causes

The user's accepted-build session log is
`%LOCALAPPDATA%/SingularityVR/view_matrix.log` (2026-11-07 16:50:49).
It confirms original socket CRC matches, short-gauntlet routing and the active
shadow reconstruction configuration. It does not contain a final draw/resource
capture tied to either reported black artifact. Older shadow-pass captures end
on October 25 and cannot establish the November 7 failing draw.

* Sky blackening: material/lighting, projected shadow and screen-texture
  consumption remain separate hypotheses. A successful TMD draw is not pixel proof.
* Gravity square: native FX blend/depth/resource ownership versus a projected
  receiver remain unproven. Do not globally suppress shadow volumes or FX quads.
* Hands: `SyncXRInput` locates separate aim and grip spaces at `handTime`;
  `BuildLiveControllerC` consumes aim-derived offsets and a gun pivot. Some
  no-TMD paths reconstruct the native wrist, while the TMD path preserves its
  existing controller delta. `GloveLeftShift=12` is configured but bypassed for
  the attached TMD assembly; changing it globally cannot repair that assembly.
  `docs/glove-animation.md` explicitly records that Sense grip-to-wrist mounting
  has not been measured. No guessed mounting offset is introduced here.

## Smallest distinguishing experiment (implemented, not a visual fix)

`[Diagnostic] TmdVisualAudit=1` opts into the existing final-driver shadow trace
and completed SBS capture. Default is OFF.

At most three automatic samples/session: neutral background, left aim pitch
above 0.65 radians, and gravity-input held. Thirty stable eligible Present calls
are required; samples are separated by at least four seconds. Tracking/focus,
menu/pause/cinematic and another active/pending trace prevent capture. Existing
four-trace resource ceiling remains authoritative. No power, palette, texture,
shader constant or input routing is changed.

Each trace records final driver state, bound textures/targets/VB/IB, shader
bytecode, clear/copy ordering and both-eye scene pixels through existing tools.
The new log labels the trace sequence and records both raw aim and grip poses.
The existing final-bound TMD wrist probe is sampled only in active audit frames
and flushed on the render thread (max eight records/eye). It reconstructs the
actual wrist from native bind, final skin palette, LocalToWorld and submitted ride.
`gravity-input` explicitly does not establish successful native levitation.

The trace and SBS must actually show the reported defect. If they do not, this
experiment is insufficient; do not claim a cause or apply a speculative fix.

## One physical session

MAIN stages only the audit DLL and a copy of the current INI with
`Diagnostic/TmdVisualAudit=1`, with verified backup/profile guard and restoration.
1. TMD visible against ordinary scenery: hold about two seconds.
2. Point the left controller at the sky until the blackening is visible; hold
   two seconds (allow at least four seconds since the first sample).
3. Successfully levitate an object, keep the black square visible for two seconds
   (again allow four seconds). Keep the right controller visible/tracked.
4. Exit normally. Inspect `tmd-visual-audit` sequence receipts, matching
   `shadow-pass-PID-N.txt`, `capture_*.bmp` and final-bound wrist receipts.

No FPS claim: the opt-in trace includes bounded readbacks and can stall its
sampled frame. It is not enabled in the production profile.

## Verification

Optimized x86 PE32 build passes. `test-dual-weapon-tmd.ps1` passes including the
new real scheduler executable, existing dual pose/grip/reload/aim/holster and
WeaponLocker regressions, Python wiring checks and OLED D3D11 GPU/reference
comparisons. Additional shadow-trace and hand-anchor fixtures are recorded in
the local build evidence. This is L1/L2 evidence only; all three requested
visual/alignment corrections remain UNPROVEN pending the distinguishing capture.
