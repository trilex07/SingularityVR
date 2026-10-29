# Volk S4 native HD integration — 2026-10-29

Branch: `integration/volk-s4-hd-physical-reload`. Baseline: accepted first-release
`6991ef1f`; runtime before integration `38ca72f8`, DLL `23A3A768...`.
PR66's three asset-tool commits are included without merging an unrelated branch.
Private game-derived assets remain outside Git.

## Implemented contract

`prepare_volk_runtime.py` checks all 22 handoff hashes and identifies the complete
original cooked vertex stream inside the named shotgun export. Its original 8297
vertices (positions, UVs, weights, tangents) and 10835 triangles are byte exact.
Six source-bone detail groups add 1892 triangles; final 12085 vertices / 12727
triangles use the original five-slot palette [clip, Slide, cog_shotgun, release,
Trigger]. All 1892 added faces match their pinned source OBJ after ONE native Y
reflection (max error 4.92105468e-6 UU). No guessed attachment offsets.

The native draw swaps only stream 0 and IB, then restores both even after failed
binding/draw. Exact full-body signature, verified original IB identity, native
shader palette, foreground scene, current Volk identity and GameplayStereo gate
prevent substitution into another weapon, attachment or shadow pass. Missing or
invalid cache/declaration/GPU resources retain original geometry. GPU resources
are allocated once, released on Reset, and recreated lazily. No GPU readback in
runtime; CPU private cache is read at initialization only.

Native shader/material, textures, animation constants, depth, stereo transforms,
root pose and existing muzzle path remain owners. Native shadows retain their
original geometry: no extra HD shadow submission. PBR metallic/roughness/height
are NOT UE3 shader inputs and are intentionally not passed as if they were.
4K BC3 diffuse preserves original alpha; BC1 normal reverses the GLB green flip.
Constant-color new details use a verified unused native UV island; base UVs stay
unchanged. The existing priority-material bridge consumes the two exact
`texture-override-remaster/AR_shotgun_[dn].dds` files. Global 8K tier is unchanged.

Existing manual reload FSM/native ammo acknowledgment, cancellation, shoulder
acquisition, observed wrist witness, authored palm grip/finger curl and magazine
fall are reused. The same HD draw also replays the carried/dropped clip palette;
no second magazine renderer or controller-space shortcut is introduced. Original
mag_remove_reference/mag_insert_reference remain references, never a baked root
animation. mag_release_state remains static. Slide move/hold/return and mag_seat
sample the original timeline relative to the COMPLETE source endpoint transform:
D(f)*inverse(D(end)), followed by existing parent/root. This resolves the 0.56 UU
seat-to-bind discrepancy without an arbitrary translation. Root, trigger and
release remain byte exact. Default `[Render] VolkHdNativeModel=0` is immediate
geometry/mechanics fallback (restore the two DDS separately for full rollback).

## Evidence and limitations

- x86 /O2 mechanics: 3002 time/controller samples; endpoint matrix max-abs error
  7.67642065e-19; absent magazine collapse, unchanged root/static parts and tail,
  other weapon refusal PASS.
- Real D3D9 AND D3D9Ex: SVVH submits 12727 triangles into each test viewport;
  nonblack pixels 5401/5401, VB/IB, shaders, depth/blend/cull/viewport restoration,
  draw failure, GPU release/recreate, foreign IB, invalid stride, missing cache and
  corrupt cache transactional refusal PASS. This uses a synthetic test shader:
  NOT native UE3 material, OpenXR or physical PSVR2 proof.
- Existing dual-TMD/input/support/reload/aim/WeaponLocker tests and OLED D3D11
  tests PASS. Accepted-profile guard: 38 synthetic fail-closed cases PASS.
- Steam, PSVR2 visual/depth/muzzle/reload and frame-time comparison: PENDING.
  No claim of zero performance regression: extra vertices/4K map cost is unmeasured.

Fresh Steam install was inspected: the three original Volk reload dependencies
were absent. Copies in the preserved pre-clean-install backup match the existing
runtime FNV constants 2782548863 / 789788386 / 2003765126. Include only these three
Volk files, the new SVVH and two DDS in the exact asset delta. Do not silently
copy other weapon assets or change their input/FSM behavior.

## Reproduce

1. Build `tools/uedecompress/unpack_chunks.cpp` x86 /O2, unpack the owned native
   `wp_SP_ARShotgun_SF.xxx` to private `build/volk-hd/shotgun.u`.
2. `python tools/assets/prepare_volk_runtime.py --package ../volk_s4/final_handoff
   --cooked build/volk-hd/shotgun.u --output build/volk-hd/runtime`
3. `tools/tests/test-volk-hd-runtime.ps1` (real D3D device access required).
4. `tools/tests/test-dual-weapon-tmd.ps1`, `tools/tests/test-accepted-profile-guard.ps1`.
5. `spikes/view_matrix/build.ps1` builds PE32 /O2 and records exact source/DLL identity.

MAIN uses `deploy-singularityvr.ps1` exclusive session-assets manifest (six exact
files) then DllOnly + IncludeIni accepted-profile plan. Back up all changed files;
compare the live baseline to the preserved accepted copy and document any INI
changes. Roll back DLL/INI first, then assets. No launch over an active game.
Bounded runtime receipts: `volk-hd: CPU cache=READY` and paired
`volk-hd native: eye=... replaced=1 vertices=12085 triangles=12727 ... gate=HD-submitted`.
CPU readiness alone is not a visible draw. Receipt failure retains original model.

One PSVR2 session: inspect Volk surface/depth in both eyes and wrist rotations;
fire; support/release Grip; Circle eject/drop; left shoulder Grip; rotate carried
magazine (stable, fingers grip); insert; inspect seat/Slide; confirm native ammo
counts, fire again, cancel one reload. Compare usual frame times to accepted
baseline and check TMD/other weapon briefly. Stop for stereo/depth/tracking failure
or repeatable performance regression; restore and hash-check. User requested
Steam installation at the end of preparation; launch remains user-controlled.

## Final installed candidate

- Source `7fbbe97a`, clean x86 PE32 /O2; DLL SHA256
  `485FE0AF26925C0BD339DC0ACFBC9340742522FB4DC27C385401C4A603CAC537`.
- Exact cache/DDS identities and evidence levels: `docs/release/volk-hd-candidate-20261029.json`.
- MAIN installed on explicit user request at 2026-10-29 22:44; retained, NOT launched.
  Full accepted-profile Post PASS, six asset SHA matches, original bridge and
  texture INI unchanged. Runtime backup `deployment/install-backups/20261029-224430`;
  six-file backup `build/next-weapon-reload/volk-hd-20261029/asset-backup`.
- Existing reload assets were absent and are restored from verified original
  backup; no other weapon assets were copied. Only INI change: Render/VolkHdNativeModel=1.
- Initial post-install asset-only verifier expected the pre-install DLL and
  refused (without writes) after the runtime install succeeded. The complete
  accepted-profile Post check and six explicit hashes subsequently passed.
  Asset-only ValidateLive belongs BEFORE runtime installation; preserve original
  manifest hash for later rollback. No weakening of either production guard.
- Private `build/volk-hd/restore-session.ps1` performs production DLL/INI rollback
  FIRST, then six-file rollback and exact baseline hash confirmation. Not run
  because no gameplay failure occurred and user requested candidate installation.
- Physical PSVR2/material/depth/muzzle/reload/frame-time acceptance remains PENDING.

## User acceptance and main promotion — 2026-10-29

After the installed candidate was offered for PSVR2 testing, Trilex replied
"gites wrzuc na maina" and explicitly authorized main promotion. This records
general user acceptance of the installed Volk integration, not independently
measured frame timing or an invented per-feature test checklist. Keep the exact
installed DLL and asset hashes above; documentation-only acceptance changes
require no rebuild or Steam replacement. Candidate remains installed; backup
remains available. Earlier PENDING entries describe pre-acceptance evidence.
