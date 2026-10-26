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

### P1-A — Age/Square visual producer

- Owner MAIN; status NOT STARTED (root-cause verification in progress).
- Sources: `native_tmd_socket.inl`, `native_tmd_beam.inl`,
  `tmd_muzzle_socket.h`, original RvEffectObject/particle route.
- Evidence: `docs/handoffs/main-l2-square-native-audit-20261025.md`;
  previous private native disassemblies and current physical FAIL.
- Dependency: prove actual particle attachment producer and matrix ownership;
  corrected curved beam origin alone did not correct the visible effect.
- Acceptance: effect originates at TMD in both eyes, no clipping/mismatch;
  existing L2/black-artifact/hands PASS preserved, comparable frame times.

### P1-B — Seeker

- Owner SIDE seeker_finish, MAIN integration; status NOT STARTED (audit running).
- Branch `side/final-seeker-20261026`; base `71728b5d`.
- Sources: native ownership/slot/support/projectile code and archived physical logs.
- Dependency: legitimate acquired Seeker for physical test, native lifecycle
  decision (earlier user accepted original drop lifecycle; no silent fake retention).
- Acceptance: verified ownership and supported native draw/switch, reliable Grip,
  muzzle-aligned launch preserving guidance/spread/recoil; save/load separately tested.

### P2-A — XTTS Polish dubbing

- Owner SIDE audio_finish; status AWAITING HUMAN APPROVAL.
- Branch `side/final-audio-20261026`; inspect `side/dubbing-corrections`,
  local delivery manifests and actual native-bank consumer.
- Acceptance: complete hash-pinned scope, correct line mapping, duration/format/
  loudness checks, English preserved, documented rights and listening approval.
- No generated voice activation merely because files exist or ASR passes.

### P2-B — Sound remaster

- Owner SIDE audio_finish; status AWAITING HUMAN APPROVAL.
- Existing branch `side/audio-music-sfx-remaster`; 8 sets/24 variants are claims
  to verify against actual assets/consumers.
- Acceptance: approved variant per event, native SoundCue/timing/spatial/concurrency
  preserved, no duplicate playback; actual listening required.

### P3-A — Menus and legacy prompts

- Owner SIDE visual_finish; status NOT STARTED (deployment/binding audit running).
- Branch `side/final-visual-20261026`; reuse V2.1/M02 assets and existing GFx.
- Acceptance: declared screen scope consistent, PL/EN glyphs, laser/selection/
  scroll/transitions, both eyes; no duplicate overlay or HUD3D redesign.

### P3-B — HD assets incl. Valkyrie 4K

- Owner SIDE visual_finish; status NOT STARTED (identity/approval audit running).
- Acceptance: exact material identity, alpha/mips/format, correct appearance,
  bounded memory/load cost, no unapproved automatic activation.

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

No changes integrated yet. No Steam deployment, launch, audio replacement or
promotion performed by this branch. Commit/build/test receipts will be appended
as work lands. Every unresolved human or runtime dependency stays explicit.
