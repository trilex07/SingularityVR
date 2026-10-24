# MAIN-only deployment plan

1. Confirm latest physically accepted HUD commit. This branch now contains f307ad32 including orientation correction and final-bound wrist diagnostics. Physical acceptance is still pending; reconcile only if MAIN advances again. Review JSON source/binary hashes and keep the accepted install/profile backup.
2. Build/reverify if reconciliation changes source. Install only through existing MAIN deployment tooling, with game closed. Preserve installed INI, Polish assets, texture quality and accepted features. No full-INI replacement. First session gameplay integration only; remaster_apply=0 and priority_material_mode=0. Verify four reloads, HUD, TMD, wheel, NIS/OLED and locker.
3. Accuracy independent A/B: keep TwoHandSpreadPercent=100; compare defaults (100) with VrValkyrieSpreadPercent=70, VrValkyrieTwoHandPercent=50, VrVolkSpreadPercent=85, VrVolkTwoHandPercent=50 under [Input]. Not accepted balance yet.
4. Texture gate 1: verified dinput8 texture bridge + first four staged DDS only. In texture_override.ini [general], remaster_apply=0, priority_material_mode=1, remasterdir=<MAIN-reviewed staged path>. Log-only leaves existing base override behavior intact; it does not disable unrelated accepted base overrides. Verify exact four target hashes and no UI/RT/shadow matches. Next separate session priority_material_mode=2; test materials, alpha, stereo, reset and pacing. Return mode0 to accepted base behavior.
5. Do NOT set broad remaster_apply=1 for subsequent gates. Valkyrie pair and Kasimov normal are staged with hashes but need independent scoped gate extension before deployment. Blocked mip sources stay blocked.
6. Audio is separate: inspect tools/audio/prepare_xtts_native_banks.py and stage_xtts_native_delta.py inputs. Require accepted profile SHA, bank pins, selected manifest and exact package list. Generate outside Steam, review retained/replaced waves and cue linkage. No blanket WAV copy or mixed audio/gameplay deployment. Preserve original Polish Bink tracks.
7. Font gate is BLOCKED on current package identity. Old patcher expects original package; never replace accepted fonts just to make that test pass. Notes manifest synthetic tests pass independently.

Private first-gate staging is reproducible:
`python tools/textures/stage_first_hires_gate.py --assets-root C:/Users/Trilex/vr-mods/SingularityVR-assets-local --name <new-unique-directory>`

The script verifies seven sources before writing four reviewed maps, refuses existing destination and traversal, and records hashes. Review the candidate JSON for exact paths/SHA. SIDE has not changed Steam or launched gameplay. Physical verification remains MAIN-owned.
