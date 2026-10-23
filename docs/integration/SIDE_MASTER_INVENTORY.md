# SIDE master inventory — 2026-10-23

This is an offline integration inventory, not a PSVR2 acceptance record. Baseline:
`origin/main` `702f0c32adaa7e9d65df45da3f11ffafec9d89f6`.
The last explicitly restored accepted Steam build recorded in the handoffs is
`514bb03`; the later `05a94ab` / DLL `4B34CE06…` session gave partial physical
feedback and failed for the knife. Current `702f0c3` source has no complete
physical acceptance receipt in this repository. MAIN must verify the installed
baseline independently before deployment.

Status vocabulary: **INTEGRATE** = present in this branch and offline tested;
**PREPARE** = staged but not wired into live runtime; **REWORK** = needs current
runtime ownership/producer work; **REJECT** = unsafe, duplicate or superseded;
**BLOCKED** = private asset or runtime proof missing. `INTEGRATE` never means
Steam or PSVR2 PASS.

| SIDE branch / exact tip | Purpose | Decision and path | Conflict / missing evidence |
| --- | --- | --- | --- |
| `side/ui-utility-graphics-remaster` `316e60ce0b24a0949eecb84994a2077cc231d59e` | Industrial bilingual UI atlas, source sprites, QA | **PREPARE** M02: imported processed assets/generators/tests; atlas QA PASS | Current HUD, menu and laser already own D3D9 draw/hit geometry. No opt-in runtime binding or stereo visual test; old button roles must not be copied. |
| `fix/psvr2-hud-design` `5f4364c851934cb675b386b2199aa5acb7ae38f7` | HUD layout/design | **REWORK**: reference for restrained industrial style | Current plates already render dynamic data; do not add a second health/ammo overlay. |
| `side/diegetic-hud-native` `ff29efdefc5982eef5a1c8ba100d3003c5c0bd56` | GLB weapon/TMD physical readouts | **REWORK** M03: retain concept, do not import private GLBs | Preview values are static; final native ammo/health producer, skin attachment and occlusion are not proven. Large textures/material count. |
| `feature/ps-prompt-icons` `91d49d392b6127827d507647bbc0f05bc4a3ec68` | Sense icons/text | **REJECT** wholesale source merge; use M02 art only after producer mapping | Old Cross/Jump role conflicts with current bindings. Current main already owns prompt atlas and rewritten verified producers. |
| `side/hud-script-holster-fixes` `addab020f52df31f15264c65f2e715954ff72c67` | HUD/holster fixes | **REWORK** | Stale native ownership versus current glove/reload/wheel; no isolated regression proof. |
| `side/menu-zoom-diagnosis` `dc725ab47a68bc6e1b64671d5f7c94ef6a9c8f4d` | Menu diagnosis | **REJECT** old behavior patch; preserve documentation only | Current accepted menu geometry and laser must not be replaced by old offsets. |
| `side/audio-music-sfx-remaster` `ba8774c7b9f2c838b3138d42f676a0c692a9b99b` | Eight conservative/enhanced cue pairs | **PREPARE/BLOCKED** M05: tooling/docs and read-only private PCM validator imported | Seven cue identities referenced; ocean/storm is heuristic. No validated UE3 sound package installer. Separate MAIN test package only. |
| `side/bink-stills-vr` `4367040521b9bf74cd6d93d8c260078fb3d7bda7` | VR logo/loading Bink | **PREPARE** M04: hash-verified private candidates and manifest | No decoder/runtime PSVR2 proof; 4K loading is experimental. Proprietary BIK not committed. |
| `side/enemy-upscale-batch` `393eb16b73e7421fde8a3d852eadd2f5aed7a739` | Eight enemy/decal DDS | **PREPARE/BLOCKED** M06: manifest and generator imported, private DDS hash checked | Current `spikes/texture_cache` target mapping/ownership still needs controlled proof; two source textures missing. No DDS binaries committed. |
| `feature/material-remaster-v2` `0dcdcd5c6dc677573df24ea5616b69ef71ebb8ff` | Broad texture injector | **REJECT** wholesale merge | Separate D3D9 hook/cache competes with current texture-cache ownership; D3D9Ex, managed pool, LockRect and Reset risks. |
| `side/enemy-detail-proposal` `7d3a762b28a81bb90d67823141415e82936cf929` | Gore/dismemberment | **REJECT** duplicate | Current main already contains native gore work and user reported physical PASS. |
| `review/tmd-vfx-side-safety` `8fb7e73b209b66ec01a27faabe39f5457dec6a17` | TMD aim, NIS and effects | **REJECT** old render/aim merge | Current main supersedes these routes. Never redirect right weapon aim to left TMD. |
| `side/vfx-trigger-diagnosis` `6d130cdee5bd634431ecd94e28a707688e4312c6` | VFX trigger diagnostic | **REWORK** | Old diagnostic, no current effect-object ownership proof. |
| `side/dubbing-corrections` `c3c911d8496e63bb56561f85e40674a22726d8f7` | Polish corrections | **PREPARE** M07: 64 unique note IDs staged as text data | Active localization/package mapping, dialogue timing and installed glyphs still need validation. Do not overwrite working audio. |
| `fix/polish-note-glyphs` `219c6fa4aabf2b5aaac26a7774307d9610a8cc6a` | GFx note font patch | **REJECT** duplicate code | Patcher already exists on main. Installed package hash is private/out-of-scope to SIDE. |
| `side/mirror-screenshot-fix` `93cbc02488847f5b9899fd73ba34f5624b5a5f69` | Right-eye mirror/screenshots | **REJECT** superseded | Current main has newer mirror path; synthetic D3D9Ex regression passes. |

Current-main source integrations (not SIDE cherry-picks here): `ef85bf3` Grip-before-reach,
`e13498d` optional supported spread (default 100% = native), `9d637a4` bounded
knife palette probe, `702f0c3` TMD disables knife by default. M01 commit
`d897dc4` is the only new runtime behavior in this master branch.

The 8-DDS and 3-BIK hashes are indexed in
`MASTER_INTEGRATION_MANIFEST.json`. The private files stay in their SIDE
workspaces. A source-ready asset is not a deploy-ready game override.
