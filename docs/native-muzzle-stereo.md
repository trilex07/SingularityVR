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
a candidate changing only `Diagnostic/NativeMuzzleProbe=0→1` and
`Diagnostic/NativeMuzzleStereoMode=1→0`. The staged DLL hash is the MAIN build
above; `dinput8.dll` remains `5B948DDB...`. Accepted-profile `-WhatIf` has a
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

The mode-1 package is installed for the next user test under the guarded runner.
Candidate DLL SHA-256 is `4D76F52AE582447E404111C36C732BA5BB1DE50957B24DCB3EAA1F0BF9FED85A`,
INI SHA-256 is `6265E14D84E67B6A4393A3218278A85264AD1B7F2803B50A10FFECF83D5D0372`,
and unchanged `dinput8.dll` remains `5B948DDBDE07F16AE779C274CEBA133F7F24A218ABD9FFF2D4133FCADF729D59`.
The test backup is `deployment/install-backups/20261017-202413`; install report
is `deployment/install-reports/20261017-202414.json`. After the test, restore
the accepted profile and verify the restored hashes. If this pair's removal
does not remove the seam glow, do not promote this selection; test another
measured candidate instead of jumping to mode 2. Mode 2 still requires exact
camera-alias and matrix-convention evidence for the identified draw. Smoke, audio, original
light, and both-eye visual correctness remain unverified.
