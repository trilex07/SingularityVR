# M01 shoulder-magazine follow-up (SIDE)

Baseline: `1c358e2fd74d22af4c7fdfad0f4fb4382d83020d`. Evidence: archived MAIN log `docs/evidence/m01-1c358e2-20261023/view_matrix.log` on branch `handoff/evidence-m01-1c358e2-psvr2-fail`, SHA256 `8D9BC6F80E9C7A786DD0767CC429BB38DFDD960082E52CA9BA92C94C655326E1`.

## Observed failure before changing code

- Valkyrie reached `NeedAmmo` and logged 29 `grab-fresh` / `Carrying` transitions, but every one ended in `discarded`; no Valkyrie `inserted/request` or native ammo permit appeared. The first accepted shoulder grip was 0.052 m from the anchor. This disproves a universal missing Circle, Grip, TMD ownership, or shoulder-radius gate.
- Eleven bounded Valkyrie grip receipts were rejected at 0.184–0.343 m against the 0.170 m sphere. This is a separate, real geometric miss; the archived receipts contain scalar distance, not the left/right local displacement needed to calibrate an exact replacement volume.
- Volk reached `inserted/request` and `native-ammo-ack` in the same session. Preserve its carry rendering, insertion and ammo-transfer routes.
- In the compiled-source path, both `weaponcarry::Source` and the earlier `nextCarry` section gate admitted the TMD material-0 wrist only for Volk. Valkyrie with TMD and `ReloadPalmGrip=1` therefore could not publish a current wrist for its carried magazine, even when the FSM accepted pickup. The runtime `next reload draw gate: stage=11 kind=1` is consistent with that exclusion. This is a source-proven missing producer, not yet physical proof of the visual outcome.

## Falsifiable correction

1. Admit the same already-measured TMD wrist source and the earlier carry-section eligibility for Valkyrie only while a matching Valkyrie transaction is carrying; require the existing successful stereo draw, index/palette restoration, identity and 150 ms freshness gates. A next physical trace must show a current Valkyrie TMD wrist and a real per-eye carried-magazine draw; otherwise this hypothesis is rejected.
2. Replace the point-only shoulder test for Valkyrie/Volk with the union of the original sphere and a short capsule extending toward physical left in the latched head-yaw frame. It must accept all old valid points and reject the chest/right/weapon areas. The archived scalar distances cannot prove that all 11 misses lay to the left; a bounded local-coordinate receipt in the next MAIN session will distinguish this.
3. Permit an unbroken, newly pressed left Grip to reach the zone within a bounded per-weapon window, while resetting on release, support ownership, knife ownership, invalid tracking or identity change. A support/menu-held Grip must not be inherited.

No Steam/game files are touched by this SIDE branch. Offline checks will be labelled L1/L2; headset acceptance remains MAIN-owned.

## Geometry and settings

The original rear-left supply centre remains 22 cm left, 12 cm down and 18 cm back relative to the head centre. For Valkyrie and Volk only, the new pickup volume is a radius-18-cm capsule from that original centre to a point 18 cm farther toward physical left, rotated by the *latched physical head yaw*. This preserves every point of the old 17-cm sphere but does not expand into the chest/right-hand direction. The 18-cm extension is a deliberately bounded candidate, **not** a measurement established by the scalar-only M01 log.

The template INI documents six independent keys in `[Input]`: `ValkyrieShoulderExtendLeftCm` and `VolkShoulderExtendLeftCm` (0–25, default 18), `ValkyrieShoulderRadiusCm` and `VolkShoulderRadiusCm` (12–22, default 18), plus `ValkyrieGripReachMs` and `VolkGripReachMs` (0–3000, default 1800). Invalid values fail back to their defaults with a receipt. `ReloadShoulderLeftCm` remains owned by Centurion and does **not** control these profiles. Kasimov remains on its old point/edge pickup. No installed INI is rewritten by this branch.

The Grip window starts only on a new unowned left-Grip edge, survives a continuous hold while reaching, and ends on release, support/knife ownership, tracking or identity loss, timeout, or pickup. Existing insertion, native acknowledgement and ammo conservation are unchanged. A pre-held support Grip cannot become a pickup after its owner releases without a new Grip edge.

## Offline verification

- L1: `tools/tests/test-dual-weapon-tmd.ps1 -SkipOled` PASS, including the reload FSM, 30,000 synthetic conserved transactions, both TMD/no-TMD wrist-source cases, weapon identity, two-hand, glove, Centurion and WeaponLocker tests.
- L1: `tools/gloves/test-tmd-short-gauntlet.ps1` PASS (SVTG draw dispatch, 7110 triangles, 49-run fallback, index/palette restoration, sleeve bone consistency).
- L1: `python tools/reload/test_carry_ray_wiring.py` PASS; `spikes/view_matrix/tests/run-test-oled.ps1` PASS (CPU and D3D11/GPU reference).
- The expanded `tools/gloves/test-tmd-runtime-regressions.ps1` passed its CPU tests through WeaponLocker layout but is **BLOCKED** at `test-tmd-short-gauntlet-d3d9.cpp` because this SIDE desktop cannot create a D3D9 HAL device. No D3D9/D3D9Ex PASS is claimed from that fixture.
- L2: x86 PE32 `/O2` build with static analysis PASS. Build hash and final commit are recorded in the delivery message after the final build.

## MAIN-only controlled PSVR2 acceptance

1. After backing up and hashing the accepted DLL/INI, install **only** this DLL plus the six `[Input]` settings above; keep the accepted Polish, Volk/TMD, knife and other settings unchanged. For the test set `[Diagnostic] ShoulderPickupProbe=1`. Do not stage UI/3D HUD/audio/Bink/DDS changes.
2. With TMD owned and Valkyrie equipped: Circle to eject; press left Grip before reaching farther left of the shoulder, carry a **visible** magazine to the gun, release at the socket and fire. Repeat with Grip pressed after the hand has entered the zone; briefly check that an ordinary two-hand support Grip away from the shoulder creates no magazine.
3. Repeat the two pickup methods with Volk; confirm its previously working carried magazine, insertion, native ammo transfer and firing. Check both eyes, TMD/left glove, and that no second magazine appears from a held Grip.

Archive the complete `view_matrix.log` before another launch. Acceptance requires `shoulder pickup gate` local coordinates/distance for the attempts, `next carry wrist: kind=1 ... segments>0` for Valkyrie, `next reload draw gate: stage=12 kind=1`, both-eye magazine draws, `inserted/request`, native ammo acknowledgement and a successful shot. The same native completion and no false pickup must hold for Volk. A `GrabFresh` alone is **not** PASS. If the magazine is still invisible, inspect the first missing receipt rather than enlarging the zone again.

## Exact rollback boundary (MAIN only)

Use the timestamped backup and accepted-profile change plan created for the controlled install; never copy files over a running game. After normal game exit, MAIN first checks `Get-Process Singularity -ErrorAction SilentlyContinue` is empty, then runs `tools/deploy-singularityvr.ps1 -Action Rollback -BackupPath <exact timestamped backup> -AcceptedProfilePath <accepted-profile.json> -ChangePlanPath <approved rollback plan> -WhatIf`, followed by the same command without `-WhatIf`. Verify the restored installed DLL and INI SHA256 against the recorded pretest hashes. Do not roll back unrelated separately accepted assets.
