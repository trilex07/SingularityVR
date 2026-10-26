# Final completion tracker

Owner: MAIN. Updated 2026-10-26. This document owns release status; historical
STATUS.md entries and agent handoffs do not override the scoped human result.

## Verified starting point

- Source: `71728b5d7e2712d1640fa18fa58e48bd6dc708ee`.
- Physically tested DLL: `AB348E06080E479988E810375CC50D92EEA1936D94313A02360517269DC4F964`.
- Tested INI: `9E68D94401ADD0986CF7ACB8E922F805BD3DCFBCAD967BE59388FAA13DF8831C`.
- Both installed hashes independently read on 2026-10-26; no Steam writes/launch.
- Human scope: black artifact PASS; L2 PASS; both hands/two-hand grip PASS;
  Age/Square effect FAIL. Seeker was absent in the last save, NOT VERIFIED.
- Candidate build provenance: prior worktree
  `build/main-seeker-evidence/candidate/AB348E06/candidate-manifest.json`.
- Older accepted/restore receipt identifies DLL `610F4C21...` / INI `C5D24E16...`.
  Do not silently use it to overwrite the physically improved starting point.
- Integration branch: `integration/final-completion-20261026`.

## Work and acceptance

### P1-A â€” Age/Square visual producer

- Owner MAIN; status IMPLEMENTEDâ€”AWAITING HMD.
- Default-OFF `TmdTrackedAgeParticles` corrects exact owned ageOld/ageNew
  particle attachment parents through the existing native update callback.
  Original x86 contracts and production callback tests PASS; runtime visual
  attribution remains falsifiable, not claimed proven from source.
- Evidence: `docs/handoffs/final-age-particle-attachment-20261026.md`.
- Sources: `native_tmd_socket.inl`, `native_tmd_beam.inl`,
  `tmd_muzzle_socket.h`, original RvEffectObject/particle route.
- Evidence: `docs/handoffs/main-l2-square-native-audit-20261025.md`;
  previous private native disassemblies and current physical FAIL.
- Dependency: prove actual particle attachment producer and matrix ownership;
  corrected curved beam origin alone did not correct the visible effect.
- Acceptance: effect originates at TMD in both eyes, no clipping/mismatch;
  existing L2/black-artifact/hands PASS preserved, comparable frame times.

### P1-B â€” Seeker

- Owner SIDE seeker_finish, MAIN integration; status IMPLEMENTEDâ€”AWAITING HMD.
- Existing source retained after native ABI, geometry and archived-session audit;
  no new unsupported pose/zone/accuracy adjustment. Integrated `f14e7995`.
- Branch `side/final-seeker-20261026`; base `71728b5d`.
- Sources: native ownership/slot/support/projectile code and archived physical logs.
- Dependency: legitimate acquired Seeker for physical test, native lifecycle
  test. Completion directive selects optional `SeekerBackSlot=1` for the next
  controlled candidate; prior native-drop preference is superseded for this test.
  Retention must use native owned weapons, with no injection or duplication.
- Acceptance: verified ownership and supported native draw/switch, reliable Grip,
  muzzle-aligned launch preserving guidance/spread/recoil; save/load separately tested.

### P2-A — XTTS Polish dubbing

- SIDE selection 693f5599 integrated; all 1258 chosen WAV hashes, mono PCM16
  24 kHz formats and exact native identities independently verified.
- MAIN built all 116 installed-source banks offline. 111 banks have safe
  replacements: 707 takes replaced, 551 retained unchanged (see exact
  timing/codec reasons in the MAIN receipt). Five additional
  uncovered native identities are unchanged. NOT complete Polish coverage.
- Source lookup preserved from installed installation, not older repo text.
- Actual installed vignette RT99_0105_SC1 is stereo 44.1 kHz. Preserve its
  channel/frame contract when encoding mono source; no metadata rewrite.
- Exact retained list: `docs/handoffs/final-dubbing-main-build-20261026.json`.
- SIDE artifact claims listening approval; MAIN has not independently verified
  that claim and performed no listening. Technical PASS is not listening PASS.
- Remaining: SIDE must supply shorter approved takes for timing refusals;
  investigate codec frame parity separately. In-game listening is PENDING.
- Reconciled SIDE plan: 551 timing takes, including nine with codec refusals;
  zero additional codec-only takes after five MAIN codec recoveries. The receipt's guarded duration already
  excludes outer silence. No new recordings arrived with SIDE d38b2407.

### P2-B — Sound remaster

- User delegated selection. Conservative native-content variant selected for
  all eight sets; all eight native banks built and staged. No listening claim.
- Exact SoundCue/ambient/music consumers verified; storm is Freighter's
  windStormHigh01_R layer, NOT a global weather replacement.
- Fixed Vorbis allocation, native properties and non-target bytes preserved;
  real encode/decode/container tests PASS. Physical listening PENDING.
- Evidence: `docs/handoffs/final-native-remaster-candidate-20261026.json`.

### P3-A — Menus and legacy prompts

- Native main/pause Button and ButtonLeft symbols now use neutral V2.1 plates
  for up/over/down/disabled; original native text/actions/layout retained.
- All 120 frame-state bounds and real embedded bitmap pixels checked offline;
  no duplicate text or extra render hook. New exact GFx cursor identity admitted.
- Existing Settings/wheel/backdrop bindings preserved. PSVR2 interaction and
  appearance remain PENDING.
- Evidence: `docs/handoffs/final-native-menu-remaster-20261026.md`.

### P3-B — HD assets incl. Valkyrie 4K

- Ten private DDS files staged, including accepted Volk/glove files and exact
  source-derived Valkyrie, Centurion and Kasimov pairs. New gates default OFF;
  next controlled candidate explicitly enables the three reviewed weapon pairs.
- Kasimov diffuse uses highest real mip1 (1024x512); 4K is interpolation, not
  recovered native detail. Original declared mip0 is absent from cooked data.
- Clean bridge x86 /O2 build and real D3D9/Ex pixel/reset/lifetime tests PASS.
- Physical material/alpha/normal appearance and memory impact remain PENDING.

## Milestones and stop rules

1. Gameplay candidate: independently reviewed P1 changes plus preserved baseline;
   x86 PE32 /O2, actual native ABI and relevant CPU/GPU regressions before one
   grouped PSVR2 session. Additional measurement only for a specific blocker.
2. Integrated candidate: approved audio/visual payload and manifest; grouped
   listening/UI/material validation. Unapproved assets remain staged and OFF.
3. Before any MAIN deploy: active-game check, current installed identities,
   guards/lease ownership, backups of every changed file, scoped plan and exact SHA.
4. Crash/stereo/tracking/black artifact/localization/performance regression:
   stop candidate, preserve evidence, restore verified lease baseline after exit.
5. Builds/logs/simulator/counters never imply physical PASS or stable 90 Hz.

## Delivery ledger

Integrated visual7548b119/22117405, Seekerf14e7995, audio5d56338f/6d5d9125.
No Steam deployment, launch, audio replacement or promotion performed by this
branch. Age callback/native-machine-code tests, dual-hand regression suite,
TMD runtime suite including D3D9/D3D9Ex7110-face submission, WeaponLocker preview
and OLED D3D11 pixel/reference tests PASS. All physical gates remain open.

## Current combined offline payload

130 hash-verified private files: 111 campaign banks, eight sound-remaster banks,
one menu package and ten DDS files. Manifest under
`build/next-weapon-reload/final-complete-v3-20261026/manifest.json`.
No Steam deployment or gameplay launch. Safe partial dubbing is explicit;
551 retained takes must not be represented as installed new XTTS audio.

V3 codec addendum: five short single-page Vorbis failures recovered without
changing compressed speech packets or native metadata. Two banks changed; all
previous 702 replacements preserved. SIDE 004e3051 supplied no new recordings.
See docs/handoffs/final-dubbing-codec-recovery-20261026.md. Steam untouched.
