# Offline build and regression evidence — 2026-10-23

Scope is this branch in the isolated `SingularityVR-sol-master-integration`
worktree. No game launch, Steam deployment or PSVR2 verification. The final
source code state at the build was integration code commit `881e18c7`; later
documentation/validator files were untracked at build time. They do not enter
the DLL. MAIN must rebuild from the reviewed branch HEAD and record its own
SHA before any installation.

## Native build

- Command: `spikes/view_matrix/build.ps1` (local offline OpenXR SDK).
- Result: **PASS**, MSVC x86 `/O2`, PE machine `0x014C`, PE32 optional-header
  magic `0x010B`, built `spikes/view_matrix/d3d9.dll`.
- Exact local DLL SHA256:
  `E0CE640A99E30D80774916876514586FF2C8DA224FD2BF56DD62688557E35271`.
- `d3d9.build.json` records branch, commit, architecture and optimization;
  it is generated/ignored, not a distributable binary. Existing `/analyze`
  warnings remain (including stack-size, nullable handle and deprecated
  `codecvt` warnings); no compile or link error. This build does **not** prove
  that native game hooks, D3D9 state or visible pixels work in Steam.

## Regression results

| Check | Result | Limit |
| --- | --- | --- |
| `tools/tests/test-dual-weapon-tmd.ps1 -SkipOled` | **PASS**: 16 x86 C++ programs plus Python wiring, including next-weapon reload 30,000 synthetic transactions, two-hand spread, glove/TMD, aim and WeaponLocker preview | Synthetic/source-level and CPU only. No UE3 inventory or PSVR2 pickup proof. |
| `spikes/view_matrix/tests/run-test-oled.ps1` | **PASS**: CPU/D3D11 compute comparison within 1 LSB and device lifetime/OFF identity | Own-process D3D11, not game stereo submission. |
| `tools/tests/test-right-eye-mirror.ps1` | **PASS**: geometry, own-process D3D9Ex pixel/RT0/viewport/reset, 11 Python tests | Steam F12 and headset remain unknown. |
| `tools/gloves/test-tmd-runtime-regressions.ps1 -Asset <private SVTG>` | **PARTIAL**: earlier x86 CPU programs including Centurion, 30,000 next-weapon transactions, TMD, cuff and locker passed. **BLOCKED** at `test-tmd-short-gauntlet-d3d9`: own-process `CreateDevice(D3DDEVTYPE_HAL)` assertion failed before draw. | Do not claim D3D9 or D3D9Ex draw PASS from this suite on this host. No game render result. |
| `asset-tools/test_ui_atlas_qa.py` | **PASS**: 163 rects bit-exact, 91 neutral + 36 PL + 36 EN, alpha, UV/gutters, 40 MiB active atlas footprint | Tests atlas source pixels, not runtime menu hookup or perceived VR readability. |
| Three x86 prompt programs (`prompt_roles_test`, `prompt_label_rewrite_test`, `test-prompt-placement`) | **PASS** | These test current main's mapping/placement. They do not validate SIDE old bindings or every live GFx producer. |
| `tools/integration/validate_master_manifest.py --bink-dir <offline> --dds-dir <offline>` | **PASS**: JSON/module safety, 3 atlas SHA; 3 Bink SHA and Bink 1 magic; 8 DDS SHA, 128-byte headers, dimensions, FourCC, mip counts | Bink decode in UE3, DDS alpha/normal perceptual quality, texture-cache matching and Reset are untested. |
| `tools/audio/validate_private_candidates.py --root <offline>` | **PASS** for WAV/PCM/channel/rate/frames/hash/peak/RMS of 8 target sets; 7 referenced cue identities, 1 heuristic ocean/storm | No true-peak/phase/listening/loop playback or UE3 sound-package proof. |
| `tools/tests/test-note-manifest.py` | **PASS**: exact note scope, source hash and duplicate guards | Staged Polish JSON is not active in the game. |
| `python -m py_compile` on imported audio tools | **PASS** | Syntax only. |
| `tools/fonts/tests/test_note_font.py` | **FAIL/BLOCKED** optional installed-package test after three synthetic subtests passed; the source reads an installed Steam font package and its expected hash was not met. | Running this test accidentally crossed the no-Steam-access boundary, though it was read-only and wrote only temporary output. It was not repeated; no installed file was modified. Do not claim font compatibility. |

`git diff --check` PASS. All proprietary BIK/DDS and private audio files remain
outside Git. Private asset hashes are in the manifest; no asset was copied to
the Steam game directory. The generated local DLL is not a physical candidate
until Cursor MAIN reviews, rebuilds and snapshots its own deployment.

## M03 follow-up — live HUD correctness

The subsequent HUD change has its own evidence in
`M03_HUD_LIVE_DATA_AND_MOUNT_GATE.md`; it does not import the SIDE static GLBs.
`tools/tests/test-hud-live.ps1` passed all three optimized x86 programs
(health decoding/gesture, HUD gates, stereo ammo layout). The full
`tools/tests/test-dual-weapon-tmd.ps1 -SkipOled` suite passed, including
Centurion/next-weapon reload, TMD, two-hand, aim and WeaponLocker programs.
The production DLL rebuilt successfully as x86 PE32 `/O2`, SHA256
`79ED957374B0C63857561FB2FE96B9CB8D76115BA50504C79CAE3486AF2B5599`.
Existing static-analysis warnings remain; no new fatal warning or compiler
error was reported. The build receipt was generated before the documentation
commit and must be rebuilt by MAIN from the reviewed HEAD. Steam and PSVR2:
**NOT TESTED**. The physical 3D diegetic asset path remains **NO-GO**.

## Priority unresolved gates

1. **P0:** physical Valkyrie shoulder pickup with TMD, left-palm magazine,
   insertion/native loaded rounds and no support/knife false pickup.
2. **P1:** UI atlas adapter into current D3D9 menu/laser/GFx ownership and
   both-eye hit/pixel test. Assets alone are not live graphics.
3. **P2:** dynamic diegetic ammo/health producers and final attached basis;
   current plates remain active fallback.
4. **P3:** proven UE3 sound package route plus true-peak/phase/listening and
   source identity for ocean/storm. Keep audio a separate package.
5. **P4:** Bink decoder/runtime timing in game; conservative 720p first,
   4K candidate remains experimental.
6. **P5:** exact DDS original hash→override mapping, alpha/normal review,
   D3D9/D3D9Ex resource lifetime, Reset and GPU budget; two agemap sources
   missing.
7. **P6:** actual note producer/font package and Polish glyph/wrapping proof;
   preserve installed working voicepack.
