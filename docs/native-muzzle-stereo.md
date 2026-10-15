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
  and `NativeMuzzleVSBytes`, `NativeMuzzlePSBytes`. Missing/invalid selection
  disables mutation. A failed GPU constant restore latches further repair OFF.

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
L2 receipt follows after a clean committed build. L3/L4 NOT performed by SIDE.

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
