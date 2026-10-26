# Final V3: controlled MAIN session

**Historical failed payload; do not launch.** Current safe candidate and grouped
test: `final-v7-safe-session-20261026.md`. It keeps the updated campaign, menu,
textures and seven sound sets while protecting original `RvGame.xxx`. Current
readiness JSON points to V7; the hashes and instructions below describe old V3.

Offline preparation PASS. Two controlled Steam launches FAILED during startup.
Both installations were restored to verified AB348E06. Physical acceptance was
NOT RUN: neither attempt reached the D3D device/OpenXR session. V3 is NO-GO.
Incomplete dubbing coverage is explicit: 707 replacements packed, 551 old takes
retained awaiting SIDE, plus five uncovered identities outside this selection.
SIDE 004e3051 supplied a synchronized plan, not new recordings.

## Sealed candidate and rollback destination

Private directory: build/next-weapon-reload/final-complete-v3-20261026.
Manifest SHA256: 419975c167fcda2519866c5bbf4b45da6da2f9701ac1a65717e2db2cf640d97e.
Renderer: 23A3A768C44F1C6EB5479D899141924C4C44D5C6A73A3175A2827241105370B1.
Bridge: F58CB3A805E46C3D3F7E88137BD605AF21A8A8106C289385132F677A968E95FD.
Both are clean x86 PE32 /O2 builds from source 38ca72f8; subsequent packaging
and guardian changes do not alter the DLL pair.

Rollback DLL is AB348E06, INI 9E68D944, exactly the user's last physical result:
black artifact/L2/hands/support PASS, Age FAIL, Seeker unverified. MAIN compared
the installed hashes, all 30 prior frozen file pins and the six explicit inherited
INI additions against the AB build receipt. No stale 610F4C21 restoration.

The private accepted-profile-ab348e06.json additionally pins every asset preimage,
159 files total. These pins establish rollback identity, not artistic acceptance.
core-change-plan.json permits only the renderer/bridge/texture INI and the single
Render/TmdTrackedAgeParticles=1 semantic INI delta. combined-change-plan.json
adds 127 actual asset changes; three unchanged DDS are verified but need no write
allowance. All 134 candidate sources/preimages passed the real profile guard.
Exact hashes are in final-session-readiness-20261026.json.

## MAIN sequence after user says start

1. Recheck game exit and installed AB/profile/source identities. Repeat production
   deploy-singularityvr.ps1 -RuntimeDllPairOnly -IncludeIni -IncludeTextureIni
   -WhatIf with runtime/ as ArtifactDir and the private profile/core plan. Repeat
   session-assets.ps1 -Action Preflight. The asset tool requires CIM visibility;
   use an approved elevated helper if the sandbox denies it. Do not skip the guard.
2. Install the four runtime files through deploy-singularityvr.ps1 with the same
   arguments, without WhatIf. Record its timestamped four-file backup. Then deploy
   the manifest through its exclusive -SessionAssetsManifestPath mode, using
   candidate/asset-backup as BackupPath. Do not hand-copy into Steam.
3. Run tools/integration/watch-final-session.ps1 -ValidateOnly with CandidateDir,
   RuntimeBackup, AssetBackup, ProfilePath, CorePlan, CombinedPlan and exact
   baseline INI hash from the receipt. Arm the same script in a hidden helper with
   full process visibility, logging stdout/stderr; confirm READY before launch.
   LaunchWindowSeconds=1800 applies BEFORE launch only. There is NO active-session
   timeout, no process termination and no restore while the game is running.
4. MAIN launches through Steam only after successful installation and READY.
   Any failed/partial install means NO LAUNCH; preserve exact receipts and inspect
   the transaction rather than overwriting files blindly.
5. After normal exit and 30 seconds quiet, the guard preflights the ENTIRE runtime
   and asset backup before its first write, restores assets then runtime, and
   independently verifies all accepted hashes. Drift refuses restoration without
   overwriting foreign changes. Review such a refusal explicitly.

## One grouped PSVR2 acceptance session

No chat/tab switching during gameplay. Native menu: main/pause buttons, laser,
readability and Polish text. Load a TMD save: Square effect must start at the
glove in BOTH eyes; repeat L2, rotate head, then support the gun with left Grip.
Confirm no black/transparent silhouette, detached hands or magazine flicker.
Check available weapon VFX, normal manual reload, muzzle aim and the accepted
scope. Compare menu/audio/HD material appearance and ordinary frame times.
Listen to encountered Polish dialogue and weapon/TMD/environment sounds without
calling untouched/unvisited events PASS. Seeker retrieval/support/launch/save-load
are tested ONLY if it is legitimately available; otherwise mark NOT TESTED.
Exit normally and report failures once, after the whole session.

Crash, stereo/tracking/localization regression or substantial repeatable frame-time
regression stops the candidate. No claim of stable 90 Hz without measured data.
No baseline promotion without the user's physical result. The 551 SIDE recordings
and unvisited audio/content remain open even if this partial session passes.

## Offline evidence

21 audio tests, five actual-source codec recoveries, two container round trips,
all 130 payload hashes, full real baseline/preimage preflight PASS. The new final
guard is exercised against the actual production installer on a fake Steam tree:
four runtime files, changed native menu, unchanged DDS, newly absent/present DDS;
active-game refusal, corrupt core/asset backups and foreign live config refusal
all occur before restoration writes; exact baseline and absent file state restore.
Asset scope and texture-configuration install/rollback regressions PASS.

## Actual Steam attempts: 2026-10-26

Full V3: private session `20261026-124237`, game PID 13480, runtime backup
`deployment/install-backups/20261026-124241`. All 130 assets and four runtime
files installed and verified; restore guard READY before Steam launch. The game
crashed before device creation, with read AV at `Singularity.exe+0xACA50`.

Single-variable control: private session `20261026-125629` under
`final-complete-v3-ui-control-20261026`, game PID 31072, runtime backup
`deployment/install-backups/20261026-125632`. The native menu package was retained
from AB while the runtime pair and other 129 V3 assets remained identical. The
same startup fault occurred. The new menu package is therefore not required to
reproduce this crash; this does not establish the package's runtime safety.

Both dumps contain the same unpacked faulting instruction and invalid native
available-slot array: Data=0, Num=-54, Capacity=0. The first writer/caller remains
UNPROVEN. No source fix is justified by that observation alone. Details and the
next orthogonal isolation gate are in
`docs/handoffs/final-v3-startup-failure-20261026.md`.

Both guards restored every asset preimage and the four runtime files, with
accepted-profile postflight PASS (159 pinned baseline files and semantic INI
checks). MAIN independently rehashed the installed runtime and native UI package
afterwards: AB348E06 / 9E68D944 / 99FF4BF5 / 2492C8D0 / 51CDAB5C respectively.
No baseline promotion, headset observation, listening acceptance or frame-rate
measurement occurred. Do not relaunch either sealed failed payload as a test-ready
candidate. The preparation instructions above are history, not launch approval.
