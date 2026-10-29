# MAIN — Kasimov / Centurion HD and shared-source candidate

Combined branch: `integration/kasimov-centurion-hd-shared-assets`.
Supersedes the untested Kasimov-only candidate. Exact build and staging identities
are recorded in the adjacent candidate JSON after the final optimized build.

## Scope and preservation

- Includes accepted Valkyrie source `c9d67044` and its physical follow-up,
  Kasimov runtime `7f795270`, Centurion asset handoff `218e7652`, and SIDE
  performance handoff `cc80e72b`. No main merge or Steam modification yet.
- Existing controller roots, manual reload FSMs, cylinder/socket scaling,
  native ammo authority, muzzle/aim, separate scope, hands/TMD, UI, PL,
  OLED/NIS and WeaponLocker factory ABI remain authoritative.
- New `[Render] CenturionHdNativeModel=1` is default OFF in code. Existing
  Valkyrie/Kasimov HD flags are also default OFF; only the private candidate
  enables them together. Missing or incompatible caches retain native draws.

## Centurion native rendering

`prepare_centurion_runtime.py` verifies all 41 handoff files, the original
5799-vertex/6767-face GPU prefix, five native palette entries and HD source
detail geometry. Centurion GLB coordinates are ActorX axes, unlike Kasimov;
the complete authored hierarchy matches the source bind within 0.000097 UU.
The final cache has 8051 vertices / 7927 faces, adding 1160 HD faces.
Cylinder details remain on `clip`; body details remain on `cog_alt_pistol`.
No reload root, guessed hinge, cylinder size or socket offsets were introduced.

The exact original body/index signature gates replacement. A bounded, reset-safe
index witness hashes the original IB without waiting for a reload. The existing
native shader, packed declaration, palette, texture binding, depth and stereo
submission are reused. Carried-cylinder replay uses that same five-slot palette.
Original 53-frame/24 FPS mechanical actions and existing native reload acknowledgment
are retained. Extra PBR maps are not treated as an invented UE3 material.

Private output: `build/centurion-hd/runtime/centurion_hd.svct` plus
`handgun_d.dds` / `handgun_n.dds` (4096², 13 mips, DXT5/DXT1).
Kasimov retains all 80 bones, 143 dual-weighted points and the original lens draw.

## Lossless shared images: actual integration boundary

The SIDE private source packages total 113,253,954 bytes instead of 218,399,604
bytes, saving 105,145,650 bytes (48.1%). `shared_hd_gltf.py` now resolves them
for all four native converters through `--shared-root`. It verifies original
package provenance, every geometry/animation view and image byte, accessor/node
structure and safe relative URIs. Missing, corrupt or escaping assets fail closed.
The eight animated/detached pairs remain byte-equivalent to their original GLBs.

All four resulting native caches and DDS pairs match the original-input outputs
byte-for-byte. GLBs and their duplicate images are not loaded or shipped to the
game: existing native material replay already shares runtime texture bindings.
This is a source-packaging/conversion optimization, **not evidence of less VRAM,
faster loading, lower GPU time, higher FPS or 90 Hz**. No D3D9/OpenXR optimization
was invented from offline evidence.

Reproducibility repair: the supplied Valkyrie `normal_native` PNG predates HD
detail. Comparing it with the inverse-green full-HD glTF normal was invalid.
Conversion now retains the full-HD normal and reproduces the physically accepted
`7b88b292...` DDS exactly; it does not change the accepted runtime texture.

## User-requested weapons for the test save

`[Testing] GrantKasimovCenturion=1` is an explicit, default-OFF test option.
On the existing engine input thread, after a stable reciprocal native pawn and
2.5 seconds outside menus/cinematics, it requests only missing Centurion/Kasimov
ownership bits (0x9) through the existing audited native factory/pickup.
PreviewOnly still blocks acquisition. At most three native attempts, two seconds
apart; no ammo edits, synthetic ownership or progression writes. Native retention
must be enabled. Once both weapons exist, it stops for that process.

The original `Player0.ue3profile` was backed up privately before any test.
It has **not** been edited. Acquisition and persistence through a native
checkpoint/save-load require real-game confirmation; do not call them PASS.
Disable this test option after successful acquisition/checkpoint verification.

## Validation and gates

- Actual source geometry/palette and shared-input byte-equivalence: PASS.
- Actual D3D9/D3D9Ex Centurion pixel fixture: 6109/6109 nonblack pixels per API;
  native state/IB/VB restoration, reset, invalid source/cache and fallback PASS.
- Actual Volk/Valkyrie/Kasimov asset, animation and D3D9/D3D9Ex suites: PASS.
- Focused x86 tests cover native reload/ABI, test grant refusal/ownership,
  tracked cylinder, grip, TMD, measured aim/spread, scope, wheel, settings,
  Locker preview/layout and deployment asset scope. OLED D3D11 and NIS pixel
  tests PASS. See candidate JSON for final completion and compiled ABI evidence.
- Historical test-runner issues were harness-only: MinHook linking and the
  gun-scale fixture's required fixed image base. Its existing dedicated runner
  is reused; no accepted gameplay logic was changed to make tests pass.
- Steam/OpenXR session, real native acquisition/save, HD content in both HMD
  eyes, mechanical interaction and performance: PENDING, not physical PASS.

Reproduce with `tools/tests/test-centurion-hd-runtime.ps1`, the three existing
HD runners, `test-shared-hd-loader.py`, OLED/NIS runners and
`tools/reload/test_locker_compiled_abi.py` against the final DLL and map.
All proprietary packages/caches/DDS and save backups remain private/ignored.

## One grouped PSVR2 acceptance session

MAIN deploys only after full accepted-profile checks, exact asset/DLL/INI hashes,
verified backups and the active-game guard. The private lease restores the
previous accepted files after exit; it never times out an active game.

1. Load the existing save; wait for the scoped native grant. Use R3 to confirm
   Centurion and Kasimov alongside existing weapons, without dropping them.
2. Kasimov: inspect HD and lens in both eyes, aim/fire, R1 stabilization,
   support Grip; Circle → fresh magazine behind physical-left shoulder → insert
   → fire. No mandatory manual bolt action.
3. Centurion: inspect HD/cylinder, aim/fire; existing Circle/unlock → remove/dump
   → shoulder cylinder → insert → close/flick sequence. Confirm single ammo
   transfer, no floating details or size changes.
4. Briefly check accepted Valkyrie/Volk, left TMD and UI. Reach a normal native
   checkpoint, close normally; MAIN verifies receipts/ammo and profile hashes.
   Save-load persistence and frame timing remain unverified until observed.

On regression, restore and verify all baseline hashes. Do not restore the user's
new save automatically: retain the explicit pre-test backup for recovery.
