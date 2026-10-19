# Rifle shot, native FX and spare-magazine follow-up

## Observed, not inferred

User confirms Valkyrie reload succeeds, but shots do not appear to originate at
the barrel, rifle VFX is missing, and the spare intersects the glove. Archived
last session: ignored `build/next-weapon-reload/valkyrie-shot-feedback-20261019.log`.
Two native ammo acknowledgements succeeded. No further Steam writes this pass.

The installation was restored by the previous test lease. Independently checked
DLL SHA-256 `789749E5D4DB8FC6C603DF599A38A4FB1EC00DE374E4E2FDD469B09B9C5F361B`,
INI `8B0B0B4CF2DB78514A44B72CF3E79CC1C48C74E5632CDA34A468CA61702CCFAF`.

## Falsifiable fixes (default OFF)

`[Input] ReloadPalmGrip=1`, with `ReloadNativeMagazine=1`: the old path placed
the magazine bounding centre 6 cm ahead of the OpenXR AIM pose, without using
the rendered wrist or closing the fingers. Now the original noTMD glove is
posed, its actual wrist reconstructed through bind/skin/local/ride/inverse-XR
matrices, and published only after a successful draw AND palette restoration.
The native material replay consumes a same-weapon, same-phase, <=150 ms witness.
No depth clear, translucent substitute, changed size or unlit replacement.
An unavailable witness omits the spare copy, not the attached weapon.

`fit_magazine_palm.py` fits the original native-space clip to the original
left glove at explicit curl values. All fits: 1,482 hand triangles; Valkyrie
596, Volk 198, Kasimov 180 magazine triangles. Zero triangle intersections and
no sampled containment in either direction. Nearest surface distances are
0.210 / 0.111 / 0.261 UU. These are geometry checks, not rendered pixels.
`audit_weapon_mounts.py --hands <original-noTMD.psk> --originals <originals-root>`
reproduces the mounts and checks the three named muzzle bones against originals.

`[Input] WeaponTraceRigidOrigin=1`: the existing SingleLineCheck hook moved
Start by the controller pivot before computing `atan2(End-Start)`, leaving End
unchanged. This alters the original ray direction, particularly near targets.
The experiment translates BOTH endpoints, retaining the original direction,
spread and length before existing controller rotation. When available it uses
the actual named `muzzle_flash` position through the verified original weapon
palette/local/BuildGunC chain, published after successful draw/restore and
checked against current pawn/weapon and 150 ms freshness. Otherwise it uses
the previous pivot offset with direction-preserving translation. The flag is
also the gate for collecting these muzzle witnesses; Centurion is not opted in
to this new geometry path. Scope/head-aim behavior is unchanged.

Falsifiers: a fresh witness with wrong visible muzzle means the modeled chain
or origin space is still wrong; a rigidly carried magazine with intersections
means actual glove scale/pose differs from the measured offline fit. Log the
observed identity/pose before changing offsets. Do not broaden native draw IDs.

## Native VFX is still an open investigation

Offline package SHA `6F949F292CD8151965F73BDF34EBD2E77EAC02ECA244B4A5776AFAB659137BE7`
proves Valkyrie's `mLoopingMuzzleFlash1stPerson` references
`fx_muzzleLockedPlayer_Loop_Player2`. The accepted exact shader family was
identified from the pistol. Last-session log lacks a contributing-draw witness
for this rifle effect. Neither an asset name nor a successful build proves a
shader or identifies visible pixels. No unproven alias or broad draw heuristic
has been added. The candidate preserves `NativeMuzzleParticleStereo=1` and the
accepted exact drop. `-NativeFxCapture` enables only the existing read-only,
four-frame idle/three-shot trace with its two-minute timeout. No BMP readback
is introduced. Analyse complete traces before choosing a removal/repair pair.

## Gates and next physical script

L1: existing C++ suite (including original fractional animation oracles),
`test-weapon-carry-ray` (4,000 translated rays and identity/matrix cases),
110 Python tests, original-PSK oracle PASS. L2: initial x86 PE32 `/O2` build
PASS; the packaged clean-source build receipt remains the deployment authority.
No L3/L4 claim for these fixes. No main merge.

Prepare a NEW candidate using `prepare_runtime_candidate.py`, then
`prepare_vr_settings_rifle_test.ps1 -Weapon Valkyrie -CarryRayFix -NativeFxCapture`.
Do not edit/install over a running game; use the existing deployment and
restore lease, retaining accepted localization, knife, VFX, mirror and UI.

One headset run, no chat/alt-tab: load Valkyrie save, stand idle 5 seconds,
fire three short bursts separated by about 3 seconds while aiming into open
space, then Circle, collect spare behind left shoulder, inspect it near the
face and rotated, insert and fire again. Close normally; report shot/barrel,
native flash/smoke and spare visibility/intersection. MAIN reads traces after
exit and verifies restore. Do not ask the user to type during the run.

Earlier settings/tutorial toggle, shotgun hand grip, wheel mapping and NPC
name-size requests are not solved by this focused follow-up.
