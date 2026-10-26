# V7: integrated candidate for one grouped PSVR2 test

**SUPERSEDED AFTER ACCEPTANCE, 26 October 2026.** Trilex accepted the grouped
session and requested persistent promotion. V7 is now the installed baseline;
see [first-release acceptance](first-release-baseline-20261026.md). The old
instructions and runner below target AB348E06 for restore. Do not rerun them
against the promoted installation. The following text is historical test scope.

**READY_FOR_PHYSICAL_ACCEPTANCE**; startup PASS with identical bytes.
Not project completion/new accepted baseline. Steam holds verified AB348E06;
game and restore guards stopped. Wait for user **start**.

## Identity and scope

- Branch: `integration/final-completion-20261026`.
- Clean runtime source: `38ca72f88112bfcbd26addb0d6aa4f76095aa15e`, x86 PE32 `/O2`.
- Renderer: `23A3A768C44F1C6EB5479D899141924C4C44D5C6A73A3175A2827241105370B1`.
- Bridge: `F58CB3A805E46C3D3F7E88137BD605AF21A8A8106C289385132F677A968E95FD`.
- Private stage: `build/next-weapon-reload/final-complete-v7-safe-20261026`.
- Manifest: `df6a7a3ff4ae8e012f4c9b2837537333977a25e79376b852000f73030541fa82`.
- Receipt: `final-v7-safe-candidate-20261026.json`. 134 assets, four runtime
  files, 164 rollback pins. Runtime/payload equal startup control PID 32108.

Included: Age particle parent fix, existing TMD/L2/hands/Seeker integration,
116 campaign banks (1248 replacements), seven native sound remasters, native
main/pause buttons and ten DDS including Valkyrie/Centurion/Kasimov pairs.
Existing Polish localization, HUD 3D, OLED/NIS and WeaponLocker remain in source.

Withheld: **one TMD impulse sound inside RvGame.xxx**, whose replacement causes
startup AV. Original core/audio is protected. Ten exact campaign content refusals
retain base audio; five additional identities are uncovered. Actual SIDE generator
is Pocket-TTS Polish 6L: no validated XTTS/full-game Polish coverage claim.
Listening, materials and physical gameplay acceptance remain PENDING.

## MAIN deployment

1. Recheck real game exit, no stale guards, 164 baseline pins and 138 sources.
   Production `deploy-singularityvr.ps1 -RuntimeDllPairOnly -IncludeIni
   -IncludeTextureIni -WhatIf` uses staged runtime/profile/core plan; asset
   `session-assets.ps1 -Action Preflight` uses the manifest.
2. Review private `run.ps1` against receipt SHA. It uses production installers,
   records four-file/asset backups and arms `watch-final-session.ps1` in hidden
   PowerShell 7. Failure means NO LAUNCH; reconcile installation/rollback first.
3. After user start, run with full process visibility. Confirm guard READY before
   Steam AppID 42670 launch. 1800 seconds is BEFORE launch only. No active-game
   timeout, forced closure, save/registry change or write during gameplay.
4. After normal exit +30 seconds quiet, guard restores all assets/runtime and
   verifies 164 pins. Independently rehash runtime/UI/core/font/localization.
   Foreign edits refuse restore; inspect rather than overwrite. No promotion
   without the user. Private failed V3/V5/V6 helpers are not current launch routes.

## One session, no typing or tab switching in the headset

1. Main menu then pause/Options: buttons, laser, Polish labels, legibility.
2. Load TMD save; Square on ageable target: effect starts at left glove in both
   eyes. Repeat L2, turn head with right controller still, then left Grip support;
   compare after R2. No black/transparent silhouette or detached hand.
3. Available weapons: aim/VFX, manual reload, visible stable carried magazine,
   wrist tracking and scope. Observe HD materials and ordinary frame times.
4. Listen to encountered Polish dialogue and weapon/environment sounds. Original
   TMD impulse sound is intentional. Unvisited dialogue/events stay NOT TESTED.
5. Only if legitimately acquired: Seeker switch/retrieve, left support, native
   guided launch; save/load separately if practical. Otherwise NOT TESTED.
6. Exit normally and report failures once. Stop early on crash, broken stereo,
   tracking or substantial repeatable performance regression. No stable 90 Hz
   claim from asleep-headset startup logs; not full-game validation in one test.
