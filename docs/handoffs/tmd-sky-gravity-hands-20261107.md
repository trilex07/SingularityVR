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

## Tracked grip wrist (MAIN continuation)

Default-OFF `[Input] TrackedGripWrist=1` (requires `DualHandActionPose=1`):
removes the native camera rotation from the component mount and drives the
original hand/TMD/gun anatomy by the full live grip quaternion and position,
both hands. Falls back to the existing path on any invalid input.
L1: `test-tracked-grip-wrist.cpp` PASS (head/body/recenter invariance, four mounts).
L2: optimized x86 build PASS, DLL SHA-256 `7F9FD554...E3F91A`.
Physical PSVR2 alignment and both black artifacts: UNPROVEN.

## Physical PSVR2 result 2026-11-07 22:32 (DLL 7F9FD554, SteamVR runtime)

* TrackedGripWrist **FAILED / disproved**: receipts show applied=1 on both sides,
  yet right gun pointed up and left hand was upside-down, fist toward camera.
  The grip-quaternion axis mapping `{y,z,-x}` is wrong for real controllers; do not re-enable.
* Bink/loading cinema showed crossed halves (right eye = left half, left eye = right half).
  Source diff vs accepted 9488a26a touches no cinema code; build used `-Performance`.
  Cause UNPROVEN; check whether accepted 7AD356F8 shows it before blaming this build.
* TmdVisualAudit captured neutral, sky and gravity-input traces + SBS:
  `build/psvr2-trackedgrip-20261107/` (shadow-pass-15292-1..3, capture_*.bmp). Not yet analysed.
* Accepted DLL 7AD356F8 / INI 95D1EE33 restored and hash-verified.

## Candidate 81A2FE6F (aim-orientation tracked wrist)

Evidence (22:32 log, 3 samples x 2 hands): inverse(aim)*grip = rotX(+44.4deg), constant.
TrackedGripWrist now drives orientation from the aim quaternion, position from grip.
Accepted 7AD356F8 was built WITHOUT -Performance; 7F9FD554 was built with it - prime
suspect for the crossed Bink/loading halves. 81A2FE6F is built without it.
Installed with accepted INI + TrackedGripWrist=1 + TmdVisualAudit=1 only.
Left-hand upside-down is NOT explained by 44deg pitch; physical result pending.

## Crossed Bink/loading halves - origin

Not the -Performance flag. The ini has `[Render] CutsceneStereoCinema=1`
(c181f3a7, 2026-11-02, "native stereo cinema candidate") and
`CinemaRightLeftExtensionPermille=20` (e4c2065a, 2026-11-03 trial). The 16:19/16:29
SteamVR sessions ran a DLL that ignores both keys (no "stereo cinema" log line);
7AD356F8 and every later build honour them. Test INI now sets both to 0. Pending user check.

## Crossed Bink halves - actual cause (supersedes the INI-key note above)

Setting CutsceneStereoCinema/CinemaRightLeftExtension to 0 did NOT help (22:48 run).
Evidence: good 16:19 run stays VideoFlat ~940 frames with "video probe: 1 draw";
22:48 run drops VideoFlat -> GameplayStereo at frame 14 with "3 draws": two extra
DrawPrimUP stride 20, tex 2116x1080 fmt21 -> RT 1058x1080 (off-screen).
IsVideoQuad requires g_vidFrameDraws==1, so the Bink quad lost classification.
Fix: count only draws while the scene-sized target is bound (g_rtIsScene).
Candidate 9D2C568D; INI = accepted + TrackedGripWrist=1 + TmdVisualAudit=1. Physical pending.
Producer of the 1058x1080 pass not yet identified.

## PSVR2 22:51 (9D2C568D): video OK, rotation OK, shooting OK

* User: Bink films correct, hand rotation and shooting correct. Left hand flickers.
* Flicker hypothesis: ApplyDualActionC falls back to the legacy pose on frames where
  the tracked source is not current. Candidate 21963140: hold the last tracked
  correction up to 3 scene frames + 5 s stats line `tracked-grip-wrist stats`
  (miss grip/source/math). Physical pending.
* Audit trace PID 17792 seq 3 (gravity): SBS capture 22:53:24 shows two flat black
  rectangles in the room. Not yet proven to be the artifact rather than level geometry.
  Opaque foreground SceneColor group 414 (colorWrite=7, moved=2) is a lead, not proof.
  `tools/shadow-pass-groups.py <trace> <regex>` lists groups by PS CTAB names.

## Gravity black rectangles - distinguishing experiment (E0E08775)

User confirmed the two black rectangles in capture 22:53:24 ARE the artifact.
Trace 17792-3: only the gravity frame contains modulate decal groups 311/312
(srcBlend=DESTCOLOR dstBlend=ZERO, stride 12, PS CTAB DecalLocalBinormal/
DecalLocalTangent, 4 draws per eye). Shadow projection 410/411 is in every frame and
projector reconstruction never fired (ps gate = flashlight shader only), so neither
is specific. Default-OFF `[Diagnostic] DropDecalLocalTangent=1` skips only indexed
draws whose bound PS names DecalLocalTangent. Rectangles gone = producer proven;
still present = decal hypothesis disproved. Cache keys on PS pointer without AddRef
(diagnostic only).
