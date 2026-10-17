# Original weapon FX stereo — controlled continuation

SIDE, no Steam. Base origin/main fb73156 (includes current knife/hand integration).
Goal: keep original UE3 muzzle flash, smoke and light/materials, not generated
plugin replacements. Known failure: native muzzle effect appeared at the SBS seam.
Current contributing shader/material/draw is NOT proven. Historical stride72
identification was DISPROVED by its drop test (STATUS runs196/197); do not reuse it.
The original pistol points to FX_MuzzleLockedFX_MuzzleLocked; native flash and
barrel smoke belong to one ParticleSystem. A pointer/name is not pixel proof.

Hypothesis under test: a native effect uses a named ViewProjectionMatrix alias
not tracked by StereoPair after constants/state switch. Smallest safe repair:
ONLY an explicitly identified VS+PS pair, ONLY a named 4-register camera, ONLY
when its current GPU bytes exactly equal an existing valid tracked left-eye
camera, add that alias for this draw and restore tracking/GPU state afterward.
No raw/native camera is guessed, no c5/c26 folklore, no world light matrix edit.

Falsifiers: selected-pair drop does not remove artifact; camera has no exact
valid alias; corrected draw still produces wrong pixels; effect is instead
screen-space/CPU projected or its socket is at the old gun position. In any of
these cases leave alias repair OFF and use captured shaders/constants to resolve
the actual producer. Rejected mixed shadow correction stays OFF.

Before any physical test MAIN needs bounded original-FX baseline/fire captures,
then a selected-pair drop discriminator. Shot novelty alone cannot identify FX:
tracers, shell ejection, impacts and dynamic lights also change when firing.
The initial candidate therefore includes autonomous baseline/shot capture and
an exact-pair discriminator; unproven repair is never enabled by default.

## Original asset evidence (offline)

Read-only source: private `SingularityVR-assets-local/cooked/wp_SP_ARPistol_SF.xxx`,
SHA256 `B06D1516E5F4AE15BD9225A38E52BA706EC32D9C6D0E060320149A2F38DA44AF`.
The existing standalone `unpack_chunks.cpp` decoded all three chunks, each exact
declared size, into a 2,226,227-byte private logical package; input not modified.
`inspect_native_weapon_fx.py` records 98 cue/weapon/particle records.
The new bounded tagged-array reader independently resolves:

- First-person/ironsights particle reference: `FX_MuzzleLockedFX_MuzzleLocked`.
- Shell emitter reference: `FX_handgunShellsEject` (not the flash identity).
- `SP_ARPistol` export928 `mMuzzleFlashLights`: one local reference, export918,
  class/name `RvGameShared_ExplosionLightComponent`, outer928.

Thus native scene light is a separate component, not proof that a flash sprite
shader controls wall illumination. Do not move its `Translation` blindly:
ENGINE_NOTES measured that a component property write alone does not update its
render transform. Capture the real light producer/consumer before intervention.
Private reports/decompressed bytes stay in ignored `build/native-muzzle/`.
No audio is generated, replaced, muted or replayed by this branch.

## Candidate controls and evidence

All keys are under `[Diagnostic]`, restart required:

- `NativeMuzzleProbe=1`: automatic bounded idle + three post-shot full-frame
  driver traces, using the existing shadow pass trace and SBS capture.
  Requires 90 idle focused tracked frames with a weapon; changing the weapon
  aborts comparison. Fire seam compares `self` with the actual player weapon.
  Cross-thread shot/probe mailbox uses interlocked operations, independent of
  the generated VFX host. Probe disables after completion or two-minute timeout.
- `NativeMuzzleStereoMode=0`: unchanged renderer, used for the first capture.
- Mode1: drop all SCENE draws of an explicitly selected VS+PS pair (fingerprint
  AND byte lengths), through all four existing final driver wrappers. This is a
  visual discriminator, NOT a fix. Do not assume one pair is exclusive to FX.
- Mode2: narrowly scoped alias repair using the proven pair, the exact CTAB
  `ViewProjectionMatrix` four-register block and byte-identical valid tracked
  camera. Existing eye separation/remap/gun transform runs once, no reapplication
  of HMD offsets. Already tracked/overlapping registers, unsupported metadata,
  NaNs, raw native matrices or no exact camera alias are refused. This handles
  only the stated alias hypothesis, not every possible particle defect.
- Modes1/2 require `NativeMuzzleVSHash`, `NativeMuzzlePSHash` (16 hex FNV1a64)
  and `NativeMuzzleVSBytes`, `NativeMuzzlePSBytes`. There is no built-in shader
  allowlist: a missing, malformed, or invalid explicit selection disables
  mutation. Mode1 drops only that exact pair and exact byte sizes. A failed GPU
  constant restore latches further repair OFF.
  Mode2 ALSO requires `NativeMuzzleMatrixConvention=0` (ROW) or `1` (COL),
  proven from that shader's actual position instructions, matching the source
  camera's convention. Default-1 refuses repair: equal bytes and CTAB names alone
  do not prove two shaders consume the same matrix orientation.

Metadata is cached at existing shader setters (fixed1024 open-addressed slots,
balanced bounded COM refs, release on Reset/shutdown), never queried per draw.
Camera witnesses read at most128 unique pairs per captured frame at StereoPair
entry; raw draws that bypass it remain in the full driver trace. No new detours,
generated particles, native object writes or original-light suppression.
Shadow/world/hand/pistol alignment code remains unchanged with controls OFF.

`tools/fx/native-muzzle-probe.ini` is a SCOPED KEY SNIPPET, never a replacement
user INI. MAIN backs up/preserves current calibration, accepted shadow settings,
localization, UI and glove/knife data. The snippet temporarily allows native
particles (`HideMuzzleFx=0`) and turns off the generated VFX host/screen flash
for an uncontaminated comparison; this does not change native gun sounds.
Do not run concurrently with another shadow trace owner (shared four-frame cap).
The capture is the next full render frame, not guaranteed the first flash frame;
absence cannot disprove a transient producer. One-off capture hitches are possible.

Offline tools, without touching Steam:

```powershell
tools/fx/test-native-muzzle.ps1
python tools/fx/inspect_native_muzzle_assets.py <offline-decompressed-pistol-package>
python tools/fx/analyze_native_muzzle.py <idle-trace> <shot1> <shot2> <shot3>
# Explicitly select a measured pair; output DROP-only snippet, not an auto-fix:
python tools/fx/analyze_native_muzzle.py <idle-trace> <shot1> --select <VS_FNV64>:<PS_FNV64>
spikes/view_matrix/build.ps1 -GameBinaries <absolute-offline-never-deploy-path>
```

Analyzer uses content hashes, not cross-session COM pointers; refuses overflow
or unavailable shaders. It reports candidates, never marks increased shot-only
draws as the muzzle identity. Inspect CTAB/disassembly/textures plus removal
result before mode2. If `tracked=1` or `alias=-1`, do not enable alias repair
just to try it: that result calls for a different producer-specific experiment.

L1_PASS: x86 /O2 CPU tests cover hash/selection/CTAB bounds, cache positive and
negative queries/refcounts, exact alias/no-double-remap/overlap/refusal, cache
and GPU restoration/fault latch, idle/fire capture bounds; six Python analysis
and original-light-array checks. Existing world + first-person shadow receiver,
VFX eye conversion, native hand anchor, holster and trace analysis regressions PASS.
L2_PASS: clean source `469c99aed64244e95895b42ed7c14ca1ba6a6e23`, x86 PE32
(014C/010B), `/O2`, performance instrumentation OFF; no new native_muzzle-header
compiler/static-analysis warnings. DLL SHA256:
`4A8DB99F20E354CFB6D274BA74F92E6DC84ACE85FE94DD749A6CDBF20314EA23`.
Provenance records an empty working tree. Later receipt-only commit does not
change compiled code. L3/L4 NOT performed by SIDE; contributing draw UNKNOWN.
Local candidate: `C:\Users\Trilex\vr-mods\SingularityVR-native-muzzle-stereo\build\native-muzzle\candidate`.
Contains DLL/provenance, scoped probe snippet, L1/build logs and offline ownership
reports. No game-derived package/model/texture is committed or deployed.

## MAIN-only next gate: one session without chat switching

Integrate onto MAIN's actual current accepted branch and rebuild, rather than
overwriting newer HUD/knife/menu/audio work with this standalone DLL. Verify
backup, candidate==installed SHA and current independent test lease first;
SIDE has neither read nor changed installed files and has not launched Steam.

Use the same pistol, same quiet room, no enemies, wall about2m away: stay still
without firing for5s, then fire three short bursts about3s apart, keeping the
same direction; rotate the gun once, pause/resume and exit normally. Keep the
headset on throughout; MAIN collects logs, four `shadow-pass-PID-N.txt` files
and SBS captures automatically, then verifies restore per the agreed lease.
Observe flash/smoke in BOTH eyes at the muzzle, and wall illumination separately;
also note sound, hand tracking and stereo. Stop early for discomfort.
After selecting and proving the responsible draw, compare mode1 removal, then
only the supported mode2 correction in a separate controlled session. No merge
or claim of completed FX repair until original light + flash + smoke pass L3/L4.

## MAIN continuation — 2026-10-17

MAIN work is on `fix/main-native-muzzle-stereo-test`, based on the current
accepted `origin/main` (`12ab08b`), not SIDE's older `fix/native-muzzle-stereo`
checkout. The SIDE branch's `459c8d2` DLL is stale relative to accepted MAIN;
its stored artifact hashes to `4A8DB99F...`, not the reported `3363DC7B...`.
Its probe/alias concepts were compared with MAIN and the needed runtime path is
already present in MAIN. Do not deploy the SIDE DLL or merge that branch wholesale.

MAIN tightened selection safety: modes 1/2 now require an explicitly configured
nonzero VS+PS FNV64 pair and exact byte lengths; removed historical built-in
shader pairs. Mode 2 still refuses unless the matrix convention is explicit and
valid. Per-eye splitting stays OFF for the first native-only capture.

L1_PASS: `tools/fx/test-native-muzzle.ps1` passed six C++ fixtures and six
Python trace-analysis tests. L2_PASS: MAIN x86 PE32 (`014C/010B`) `/O2`,
performance instrumentation OFF, SHA-256
`4D76F52AE582447E404111C36C732BA5BB1DE50957B24DCB3EAA1F0BF9FED85A`.
This is compile/test evidence only; no runtime or visual success is inferred.

An offline package generator, `tools/fx/prepare_native_muzzle_probe.ps1`, reads
the installed accepted INI and binaries, verifies their profile pins, and stages
either a baseline probe (`NativeMuzzleProbe=0→1`, `NativeMuzzleStereoMode=1→0`)
or, when all four `-DropVSHash/-DropVSBytes/-DropPSHash/-DropPSBytes` parameters
are supplied, a mode-1 exact-pair drop while retaining `NativeMuzzleStereoMode=1`.
The staged DLL hash is the MAIN build above; `dinput8.dll` remains `5B948DDB...`.
Accepted-profile `-WhatIf` has a
PowerShell 7 ShouldProcess interaction, so preflight must use the read-only
`Assert-AcceptedDeployment` function directly or the normal guarded runner after
MAIN confirms the process is closed. The guarded runner installed the package
and its postflight passed. Backup is
`deployment/install-backups/20261017-201056`; report is
`deployment/install-reports/20261017-201057.json`. Candidate hashes: DLL
`4D76F52AE582447E404111C36C732BA5BB1DE50957B24DCB3EAA1F0BF9FED85A`, INI
`7497C897FA20A06C03DEDF5FA042F4FF83118F035191B3EEB342546E1881FB31`, and
unchanged `dinput8.dll` `5B948DDBDE07F16AE779C274CEBA133F7F24A218ABD9FFF2D4133FCADF729D59`.
The game is still closed; no L3 runtime or headset result is claimed.

### L3 round 1 — native-only trace (2026-10-17)

User report: smoke was not visible; a brief flash was ambiguous (possibly a
projectile impact); muzzle sound was not noticed; performance seemed okay.
The report is not a visual/audio pass. Probe did not change audio playback.

Session PID `22704` produced four complete `shadow-pass` traces (idle plus
three post-shot frames), all `overflow=0`, plus SBS captures. The analyzer
returned `CANDIDATES_NOT_IDENTIFIED` with 15 shot-only shader pairs. Captured
frames `20-14-10` and `20-14-14` contain a bright muzzle-adjacent mark and a
larger glow at the SBS seam. This is pixel evidence of shot-correlated content,
not proof of the owning draw. Private local evidence is in
`build/native-muzzle-main/evidence-20261017-l3/`; raw user captures remain in
`%LOCALAPPDATA%\SingularityVR` and are not committed.

### L3 round 2 — exact-pair drop discriminator (2026-10-17)

Selected candidate `314D478A9CFDBE37:C118540CCDBCB241`, VS 3384 bytes and PS
448 bytes. It has 12 draws in each post-shot trace and zero in idle. Three
texture groups include 512x512, 256x256 and 128x256 textures; the last matches
the dimensions of the original `muzzleFlashDir_01`. Its shader metadata shows
particle-cloud parameters and alpha blending. These are supporting clues only;
the pair may be shared by multiple particle types. Mode 1 now drops only this
exact pair; bounded probe remains enabled. No shader matrix alias or generated
per-eye path is active.

#### Round 2 outcome — 314D pair rejected for the large seam glow

The mode-1 exact-pair package used DLL SHA-256
`4D76F52AE582447E404111C36C732BA5BB1DE50957B24DCB3EAA1F0BF9FED85A`, INI
`6265E14D84E67B6A4393A3218278A85264AD1B7F2803B50A10FFECF83D5D0372`, and
unchanged `dinput8.dll` `5B948DDBDE07F16AE779C274CEBA133F7F24A218ABD9FFF2D4133FCADF729D59`.
Its backup was `deployment/install-backups/20261017-202413`; report:
`deployment/install-reports/20261017-202414.json`.

Session PID `28236` completed all four traces with zero overflow. Drop counters
were 24, 264 and 154 over successive reporting windows; no alias, refusal,
restore failure, shader overflow or fault was logged. The user's short report
did not clearly distinguish muzzle flash from impact. The captured gameplay SBS
at 20:27:46 nevertheless shows the broad center/seam glow still present, with
smaller muzzle-adjacent flashes in both eye views. Therefore this pair is not
the producer of the broad glow and is not promoted. The shot-only match remains
useful provenance, not a visual identity.

Rollback completed through the guarded runner and independently verified
against both the backup and accepted profile: live `d3d9.dll`
`789749E5D4DB8FC6C603DF599A38A4FB1EC00DE374E4E2FDD469B09B9C5F361B`, live INI
`8B0B0B4CF2DB78514A44B72CF3E79CC1C48C74E5632CDA34A468CA61702CCFAF`, and live
`dinput8.dll` `5B948DDBDE07F16AE779C274CEBA133F7F24A218ABD9FFF2D4133FCADF729D59`.
The game was closed. No VFX/audio fix or physical acceptance is claimed.

#### Round 3 candidate — scene-color particle quad

The next discriminator is `7EB90CEBB053DA04:4377F314D1C93468` (VS 3904 bytes,
PS 804 bytes). In each of three post-shot traces from round 2 it contributes
two to four four-vertex, alpha-blended draws: one or two in each eye's scissor
rect on the full SBS target. The PS CTAB names `SceneColorTexture` and
`ScreenPositionScaleBias`; the VS has particle-cloud parameters. It is a
plausible broad scene-color effect, not proven to be the visible glow. The VS
witness uses c0, marked tracked, with no distinct matrix alias (`alias=-1`);
this evidence does not justify mode 2. Round 3 tested only the exact mode-1
drop.

Offline staging command:

```powershell
tools/fx/prepare_native_muzzle_probe.ps1 `
  -AcceptedProfilePath <accepted-profile.json> `
  -ArtifactDir <new-candidate-directory> `
  -DropVSHash 7EB90CEBB053DA04 -DropVSBytes 3904 `
  -DropPSHash 4377F314D1C93468 -DropPSBytes 804
```

The prepared candidate INI SHA-256 is
`55881D9963C2AE4A13439D889EF5691D8684142FF6CA221B7972BF99C301E9A9`; candidate
DLL remains `4D76F52AE582447E404111C36C732BA5BB1DE50957B24DCB3EAA1F0BF9FED85A`.
Accepted-profile install preflight passed. The guarded runner installed it and
postflight/independent SHA checks passed. Backup:
`deployment/install-backups/20261017-203710`; report:
`deployment/install-reports/20261017-203710.json`. Live DLL and INI hashes match
the candidate; `dinput8.dll` remains accepted. The L1 native-muzzle test suite
passed after extending the offline generator; no DLL source changed since L2.

#### Round 3 outcome — 7EB90:4377 drop, inconclusive for the glow

Session PID `38304` completed all four read-only driver traces: draws
1785/591/943/957, all `overflow=0`, with no shader-budget overflow or unavailable
shader. The selected-pair counters logged 12, 344 and 170 drops; `alias=0`,
`refused=0`, `restoreFailed=0`, `fault=0`. This proves the exact-pair drop ran,
not that it owned all visible pixels. The user reported a muzzle flash, no
smoke, uncertain absence of the glow, and a small first-shot hitch. The
20:53:22 SBS capture still contains a bright central shot-correlated flare, so
the round does not establish that this pair alone produces/removes the seam
glow. The captured frame is not a pixel-exact headset comparison, and the
user's visual report remains useful but inconclusive. `camera=0 tracked=1`
`alias=-1` again forbids a mode-2 guess.

The bounded trace takes full-frame captures/readbacks, so it can itself cause a
one-off shot hitch. Next run isolates that cost: exact-pair drop for
`7EB90CEBB053DA04:0099C92AE105A584` (VS 3904 bytes, PS 336 bytes), observed as
two four-vertex alpha-blended sprite draws per post-shot frame, one per eye,
with a simple `Texture2D_0` pixel shader. Treat it as a candidate, not an
identified muzzle asset. The staged next visual test keeps `NativeMuzzleProbe=0`
and drops only this pair, so it adds no trace/readback overhead.

The offline helper gained a `-NoProbe` switch for exact-pair visual-only tests;
it omits unchanged INI entries from the exact-change plan so the accepted-profile
guard sees only actual deltas. Staged candidate:
`build/native-muzzle-main/candidate-20261017-drop-0099-no-probe-v3/`.
Candidate DLL SHA-256 remains
`4D76F52AE582447E404111C36C732BA5BB1DE50957B24DCB3EAA1F0BF9FED85A`; candidate
INI SHA-256 is
`B04E3F01635459570BBB1961304BC45499C19A9438A277FE0FFFC2913793EB48` with probe
OFF and the 0099 pair selected. `dinput8.dll` is unchanged. Guarded install
preflight passed with `-WhatIf`; the guarded install and postflight then passed.
Backup is `deployment/install-backups/20261017-210341`; report is
`deployment/install-reports/20261017-210342.json`. Independent live hashes match
candidate DLL `4D76F52AE582447E404111C36C732BA5BB1DE50957B24DCB3EAA1F0BF9FED85A`,
INI `B04E3F01635459570BBB1961304BC45499C19A9438A277FE0FFFC2913793EB48`, and
unchanged dinput8 `5B948DDBDE07F16AE779C274CEBA133F7F24A218ABD9FFF2D4133FCADF729D59`;
the original UI package remains unchanged at SHA-256
`60E7287185300CAA2A8E64FBE14B741475D4D959944B6B0E51F84547841E8DB8`. It is
installed as the temporary visual discriminator only, not a fix.
The next one-session observation should distinguish whether muzzle flash or
smoke changes when this pair is dropped, whether the broad central glow remains,
and whether the first-shot hitch remains without probe capture. The candidate
is only a discriminator; no repair/alias mode is enabled.

Round 3 evidence is archived in
`build/native-muzzle-main/evidence-20261017-l3-drop-7eb9/` (four traces, runtime
log, idle and first-shot SBS frames). The game closed before rollback. The
guarded rollback and independent hashes confirm accepted live DLL
`789749E5D4DB8FC6C603DF599A38A4FB1EC00DE374E4E2FDD469B09B9C5F361B`, INI
`8B0B0B4CF2DB78514A44B72CF3E79CC1C48C74E5632CDA34A468CA61702CCFAF`, and
unchanged `dinput8.dll`
`5B948DDBDE07F16AE779C274CEBA133F7F24A218ABD9FFF2D4133FCADF729D59`.
No VFX/audio fix or physical acceptance is claimed.

#### Round 4 outcome — exact `0099` drop, visual report pending

User confirmed the game was closed. The fresh runtime log records
`NativeMuzzleProbe=0`, mode 1, and the selected drop counters `174`, `188`, and
`38`; all report `alias=0`, `refused=0`, `restoreFailed=0`, `shaderOverflow=0`,
and `fault=0`. The log also confirms a real OpenXR gameplay session (`session=5`,
`shouldRender=1`, `haveEyes=1`, valid captures and submits). This shows that the
candidate draw executed in gameplay without the full-frame capture/readback
probe; it does not identify the draw's visible role or prove a performance
improvement. The user has not yet reported whether muzzle flash, smoke, the
central/seam glow, or first-shot hitch changed. Do not promote or reject the
pair visually until that A/B observation is obtained. No alias repair is
justified by these counters.

The game process was confirmed absent before guarded rollback. Rollback preflight
and accepted-profile postflight passed. Independently verified installed hashes
after rollback: `d3d9.dll`
`789749E5D4DB8FC6C603DF599A38A4FB1EC00DE374E4E2FDD469B09B9C5F361B`,
`SingularityVR.ini`
`8B0B0B4CF2DB78514A44B72CF3E79CC1C48C74E5632CDA34A468CA61702CCFAF`, and
`dinput8.dll`
`5B948DDBDE07F16AE779C274CEBA133F7F24A218ABD9FFF2D4133FCADF729D59`. The UI
package remained unchanged at
`60E7287185300CAA2A8E64FBE14B741475D4D959944B6B0E51F84547841E8DB8`.
Rollback source: `deployment/install-backups/20261017-210341`; deployment
receipt: `deployment/install-reports/20261017-210342.json`. No visual pass or
VFX/audio fix is claimed.

### Round 4 visual result and next discriminator

The physical PSVR2 report arrived after rollback: no FPS drop, broad glow still
perceived, no visible muzzle smoke or flash; the bullet impact on the wall remains
the only clear shot effect. Thus dropping the exact `7EB90CEBB053DA04` /
`0099C92AE105A584` pair does **not** eliminate the glow or restore muzzle FX.
This is a negative result, not a fix. Together with the earlier negative drops
of `314D478A9CFDBE37` / `C118540CCDBCB241` and `7EB90CEBB053DA04` /
`4377F314D1C93468`, it rules out another count-only candidate selection.

Offline shader inspection identifies `4FAEF2389750DBFC` /
`2FEC493CD52D7D78` as a two-triangle, four-vertex foreground draw with
`ViewProjectionMatrix` in c0-c3 and `LocalToWorld` in c5-c8. Its shader uses
row-vector multiplication. The existing trace shows identical c0-c3 in both
eyes, but that is not yet proof of its visible role. A second pass over the raw
trace corrects the earlier API inference: this group has a real VB and IB and
`verts=4`, so it is an indexed-buffer draw, **not** indexed-UP. `type=4` in the
trace is `D3DPT_TRIANGLELIST`, not a draw API code. Consequently the UP-hook
early-return theory was wrong. The accepted profile already drops this exact
pair in mode 1, while the glow persists, making it especially important to
separate this pair's role from the seam glow.

### Next read-only experiment: selected-pair draw-path witness

Hypothesis: the accepted selected pair is submitted through a route that
bypasses `StereoPair`, which would explain why a matrix alias at its entry
cannot affect it. Supporting evidence is that the raw pass trace sees the
pair, but the prior `StereoPair`-entry witness log does not. Falsifier: new
final-driver counts show it inside `StereoPair`; then investigate camera
tracking/geometry instead of bypass. A second falsifier is zero selected-pair
draws despite firing, which means the test did not reach the same producer.

`NativeMuzzlePathWitness=1` is a default-OFF, selected-pair-only diagnostic.
It counts which of the four D3D9 draw APIs reached the final wrapper and
whether that call was inside `StereoPair`. It records **before** the existing
mode-1 drop, changes no D3D9 state or pixels, allocates nothing per draw, and
emits at most 12 aggregate lines. It deliberately leaves full-frame
`NativeMuzzleProbe` OFF; no GPU readback is required. The offline package
generator's `-PathWitness -NoProbe` mode retains all accepted INI values and
adds only this diagnostic key. L3 must prove real selected-pair counts and
OpenXR gameplay; this is not L4 proof of a repaired effect. Do not enable
mode 2 or rewrite matrices based on the shader alone.

### MAIN 2026-10-17: causal emitter control and additive-pair discriminator

The path-witness run used DLL `FC474A18...` with the accepted exact-pair drop
`4FAE:2FEC` and no full-frame readbacks. The user saw smoke and glow split
across the inner edges of the eyes. The selected-pair counter was nonzero, but
every measured selected draw was `DrawIndexedPrimitiveUP` **outside**
`StereoPair`. The drop did not remove the glow. This runtime result supersedes
the earlier inference from trace VB/IB metadata: that metadata was insufficient
to identify the call API. The pair alone does not own the visible seam, and a
`StereoPair`-entry alias cannot correct its observed route.

A causal control then set `HideMuzzleFx=1`, kept generated VFX OFF, and captured
bounded idle/post-shot driver traces. The user reported the seam glow **gone**.
Comparison with emitter-ON traces found three VS:PS pairs present in every ON
shot and absent from both OFF post-shot traces:
`314D478A9CFDBE37:C118540CCDBCB241`,
`4FAEF2389750DBFC:2FEC493CD52D7D78`, and
`488BAEB579E71A73:27DFE7D9C67BCC03`. This is a cross-session difference,
not exact per-draw object tagging. The shot-accumulating `755AEA2A...` pair
remained in the control. The fourth trace slot was consumed by a repeated idle
capture, so only two OFF post-shot frames are available; both are complete.

The remaining additive `488BAE:27DF` pair (VS 2968 bytes, PS 452 bytes) was
then dropped alone, with native FX enabled and readback probe OFF. Its original
trace shows one 20-primitive draw per eye, 32x32 texture, additive blend, CTAB
`ViewProjectionMatrix` at c0-c3 and `LocalToWorld` at c5-c8. Runtime confirmed
selected draws were dropped; all were `DrawIndexedPrimitiveUP` **outside**
`StereoPair`. The user reported a change to a broad whole-eye appearance, with
smoke still in a corner. Therefore this pair contributes to the effect, but
dropping it is not a repair. No shader pair has passed visual acceptance.

Next falsifiable mechanism: one or more selected original-emitter draws take
an `IndexedPrimitiveUP` branch that forwards once, bypassing `StereoPair` and
therefore reuses one eye's placement for the other. The bounded route witness
distinguishes early menu/shadow/inert/off-thread/level-2 returns, stride drop,
HUD copy, sprite-quad, and `StereoPair`. If all selected draws reach `StereoPair`
or a legitimate per-eye HUD path, this hypothesis is false and the correction
must instead follow the emitter's producer/constant writes. Observe-only mode
3 changes no shader constants, D3D9 state, or native emitter references. Do
not enable a new stereo draw path before the route and threading are measured.

Every candidate session was rolled back only after Singularity exited; guarded
accepted-profile postflight passed. Installed accepted hashes after the final
rollback: d3d9 `789749E5...`, INI `8B0B0B4C...`, dinput8 `5B948DDB...`.
Next correction must address eye-specific placement of the original emitter's
draws at their actual bypass path, retaining its three material passes. Do not
promote single-pair drops or infer shader matrix convention from CTAB alone.

### MAIN 2026-10-17: exact IndexedUP route resolved

Observe-only candidate DLL `56F1DDE9...` (source `1987df8`, x86 PE32 `/O2`),
INI `4C58BD31...`, ran in Steam PID 21324 with OpenXR session 5 and both eyes
submitting. In four measurement windows the selected `488BAE:27DF` pair had
`entry=14/44/23/22`, **every entry exited via `HudStereoPair`**, and no selected
entry exited through inert/off-thread, quad, shadow, or ordinary `StereoPair`.
The raw draw count was twice each entry because `HudStereoPair` redrew it for
both eyes. The user still saw asymmetric glow and no clear muzzle smoke. The
guard restored the accepted DLL/INI after normal exit; independent hashes
matched. The installed UI package was observed unchanged in this test.

This is a concrete register-ownership conflict, not just a route guess. The
post-shot trace's additive 20-primitive group 234/235 (VS 742 words/2968 bytes,
PS 113 words/452 bytes) shows world-space `LocalToWorld` at VS c5-c8 (c8
translation `-9425.135,18269.559,-197.927,1`) and `ViewProjectionMatrix` at
c0-c3. `HudStereoPair` reads c5-c8 as an orthographic HUD transform and applies
an eye remap there. An affine world transform passes its w-column check, so
that check does not protect this particle. The smallest falsifiable correction
is an exact-shader, gameplay-scene-only route around `HudStereoPair` into
`StereoPair`, which should remap the tracked camera c0-c3 and leave c5-c8
untouched. Falsifier: exact reroute executes but the effect remains misplaced,
or `StereoPair` reports mono fallback/no valid c0-c3 camera. Other original
emitter passes remain unchanged until separately identified.

Mode 4 implements that isolated falsifier behind the exact VS/PS hashes and
byte lengths. Only gameplay-scene, non-quad, non-cutscene `DrawIndexedPrimitiveUP`
draws with the selected pair bypass `HudStereoPair`; other passes retain their
accepted route. It does not drop original particles, alter the emitter, or
modify unrelated c5-c8 constants. Bounded counters distinguish reroutes from
valid camera slots, tracked c0 and mono fallback. L1/L2 alone cannot establish
correct placement; the next Steam/PSVR2 test must inspect both eyes and smoke
at the actual muzzle.

### MAIN mode-4 PSVR2 result and control correction

Steam PID 37132 ran DLL `8D2B5305...0D23`, OpenXR session 5 rendered both
eyes. The selected `488BAE:27DF` IndexedUP calls took `worldRoute`; bounded
runtime counts reported valid tracked c0 every time and zero mono fallback.
The user saw a flash at the muzzle, but the asymmetric glow persisted and smoke
was weak or absent; stereo/FPS seemed normal. This is **L3 route PASS, L4 VFX
FAIL**, not a fix. After normal exit the guard and independent disk hashes
confirmed restoration of accepted DLL, injector, INI and unchanged UI.

Offline audit found a confound: the accepted INI has mode 1 exact drop of
`4FAE:2FEC`, but mode 4 reused the sole selection slot for `488BAE:27DF`.
Thus the test not only changed the latter's route but also restored draws of
the formerly dropped pair. The next experiment must keep mode 1 and its exact
pair unchanged, while enabling a **separate** default-OFF exact world route
for `488BAE:27DF`. Only then can a persistent glow falsify the route theory.

The separate selector now keeps `NativeMuzzleStereoMode=1` and exact accepted
`4FAE:2FEC` hashes/lengths unchanged. Five new `NativeMuzzleWorld*` INI keys
enable only the `488BAE:27DF` route; the existing drop still runs in the
driver wrapper. L1 tests prove simultaneous accepted drop and world reroute,
including non-overlap and invalid-identity refusal. L2 x86 PE32 `/O2` from
`2d2f892` PASS: DLL SHA-256
`F40CCAE2CB060A62488B4505A2803DD983A1251EA7EB2E55564F858F46B3E9F2`.
Offline candidate `build/native-muzzle-main/candidate-world-route-preserve-drop-488`
has INI SHA-256 `786C43C9B84DEA493A9B58A813D9A2FCF6854C1E0FEA36AE274701DB4F354010`;
read-only accepted-profile guard preflight PASS, no Steam write. Next L3 must
show both nonzero `drop` for the accepted pair and `worldRerouted/cameraC0`
for the new pair. PSVR2 must then check whether the seam glow changes.

Preparation receipt: source commit `e5ef2fc`, L1 native/shadow/hand tests PASS;
L2 x86 PE32 `/O2` DLL SHA-256
`8D2B5305108E9DC402E7543B4C2A2748277C5531D940641CF71B9B73CC1F0D23`.
Offline candidate `build/native-muzzle-main/candidate-world-route-488` has INI
SHA-256 `2A39EAE15A3FFB3788D28FFF85BFE7E899D379D4AC2044F78823624268A443E5`,
mode 4, trace readback OFF, exact pair `488BAEB579E71A73:27DFE7D9C67BCC03`
(2968/452 bytes). Direct accepted-profile guard preflight passed with all three
installed preimages matching. The wrapper's `-WhatIf` currently causes an
unrelated `Get-FileHash` provider-resolution failure, so the read-only guard
function was invoked directly; no Steam file was written. Candidate remains
offline until the next supervised test.
