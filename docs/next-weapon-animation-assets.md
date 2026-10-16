# Next weapons — original animation/model preparation, SIDE 2026-10-17

Brief before implementation: extend the existing f519de6 offline prototypes,
not Centurion runtime. Valkyrie, Volk S4 and Kasimov already have original rigid
clip exports and validated PSA Reload tracks. Prepare original albedo and
hierarchical animation data, prove actual cooked section/chunk palettes, and
build a bounded CPU animation consumer. Preserve gun/controller root; do not
retarget the old cinematic hand/root movement onto the VR camera or gun.

Hypothesis: clips and reload-moving gun parts can be driven from original local
tracks relative to a preserved live gun parent. This does NOT prove native ammo
authority, live material overrides, controller grip/socket alignment, or a native
reload guard for any of these weapons. No new runtime caller/INI/hook until each
weapon's identity/ABI/calibration is established. All CPU enable gates stay OFF.

Falsifiers: unknown/mismatched profile or cooked palette accepted; interpolated
skin produces nonfinite/sheared poses; unchanged gun/root registers overwritten;
animated descendant ignores moving ancestor; source UV/texture or vertices fail
roundtrip; a profile/calibration change continues an old transaction. Verify L1
and optimized x86 PE32 L2 offline; no Steam, deployment, saves or other checkouts.

Observed complication: Valkyrie full material export reaches an unrelated TMD
texture with invalid cached TFC chunk. Do NOT use a partial export as success:
export mesh with -notex and the required albedo separately; confirm mesh geometry
against the prior independently exported PSK. Normal/specular/emissive exports
exist where available, but albedo preview is not a native full-material claim.

## Measured original data

- Valkyrie: 11387 wedges; full original INDEX16 buffer 38526 indices, FNV-1a
  `941526542`; palette `[53,56,59,54,58,57]` = gun root, Trigger,
  clip_release, clip, fan, Slide. Main section 11378 vertices/12834 triangles;
  lens section 9/8. Reload 52 frames at 24 Hz. Curved magazine: 547 UV vertices,
  596 triangles. Original `AR_E99Rifle_alt` -> `AR_E99Rifle_alt_d` 2048x1024.
- Volk S4: 8297 wedges; 32505 indices, FNV `754008012`; palette
  `[55,56,53,54,57]` = clip, Slide, gun root, release, Trigger. One section,
  10835 triangles. Reload 108/36 Hz. Magazine 242 vertices/198 triangles.
  Original `AR_Shotgun` -> `AR_shotgun_d` 1024x1024; Slide moves as well as clip.
- Kasimov: 11389 wedges; 39330 indices, FNV `906901468`; palette
  `[53,58,57,59,55,56]` = gun root, wire_2, wire_1, wire_3, Trigger, clip.
  Main section 11292/12942; lens 97/168. Reload 239/60 Hz. Magazine 170/180.
  Original `AR_SniperRifle` -> `ar_sniper_rifle_d` export 1024x512 (mip1,
  NOT full-resolution mip0). wire_2/3 inherit wire_1/2 motion. Authored gun-root
  motion reaches about 32 UU /101 degrees: this is deliberately NOT played in VR.

The named SkeletalMesh export's native material0 and its ReferencedTextures
array establish each original diffuse reference, not just matching filenames.
Each exported `.mat` must also agree on the Diffuse role. Chunk palettes were
parsed inside the unique named mesh export and checked against PSK weight
contributors; they are NOT inferred from bone order. PSK names/parents/local
bind/absolute bind/points/weights/UVs/faces match earlier independent exports.
PSA records and source clip vertices roundtrip exactly/within FLOAT32 tolerance.

## Implementation

`prepare_weapon_animation.py` consumes explicit decompressed UE3 package, PSK,
PSA, original DDS and material export paths. Example (repeat with the three
profile-specific basenames above; no Steam defaults):

```text
python tools/reload/prepare_weapon_animation.py --weapon valkyrie --psk <ch_inview_machinegun_mesh.psk> --psa <MachineGun.psa> --package <RvGame.decompressed.xxx> --diffuse <AR_E99Rifle_alt_d.dds> --material <AR_E99Rifle_alt.mat> --output build/next-weapon-reload/prepared/valkyrie --preview
```

Each profile directory contains its original `_clip.svrk`, albedo `_clip.svrt`,
Reload `_reload.svwa`, independent NumPy matrix `_oracle.svwo`, source/output
hash report and two inspection PNGs. All are private and ignored, NOT in Git.
This machine's preparation root is
`C:/Users/Trilex/vr-mods/SingularityVR-next-weapons/build/next-weapon-reload/prepared`.
No installer or runtime caller loads these files yet.

SVWA v1: 48-byte `<4s10If` header (magic, version, profile, full wedge count,
first index, section0 triangles, full IB FNV, slot count, root slot, tracks,
frames, FPS); 64-byte root bind; each track `<Ii16f` slot/parent/inverse bind;
then `[frame][track][7]` xyz/quaternion FLOAT32 keys in original raw PSK space.
Keep the full original hierarchy, including static children of moving ancestors.
No proprietary records are embedded in the C++ header.

`weapon_reload_animation.h` bounds profiles/headers/tracks/hierarchy, decodes
transactionally at LOAD time, and samples allocation-free with shortest-arc
normalized linear quaternion interpolation. It composes `inverseBind * animated
absolute * liveRootSkin`; parent rotation/scale are retained. Root and unused
registers are copied byte-exact. A disabled/invalid request leaves output intact.
This is a pure helper, NOT a hook. It grants no authority to change a draw.

The imported f519de6 gesture prototypes remain CPU-only/default-OFF. Their
explicit shoulder API uses mid-eye head position + LATCHED physical yaw and
local `(-0.22,-0.12,+0.18)m`: physical LEFT and BACK at every tested yaw,
height-relative for seated play. Caller must use `Supply`, not old belt `Pouch`.
This is math proof, not a physical reach calibration. Profile/calibration
changes cancel, support hand stays reserved, and only shared magazine-gesture
fields are copied to the Centurion recognizer (no cylinder-specific flags).

## Verification

Run from this checkout:

```text
pwsh -NoProfile -File tools/reload/test-centurion.ps1 -PreparedRoot build/next-weapon-reload/prepared -Gpu
```

L1_PASS: ten `/O2 /W4 /WX` CPU executables, 40 Python tests, 30000 conserved
synthetic magazine transactions, 361 physical-yaw shoulder tests, all three
original SVRK decoders, and 7506 fractional original track matrices against an
independent NumPy double-precision oracle. Invalid/truncated/ambiguous assets,
identity/calibration changes, missing native acknowledgement and output mutation
on rejection are covered. Original assets generated twice deterministically.

Own-process GPU PASS (not Steam/L3): existing explicit-asset renderer uploads
SYSTEMMEM -> DEFAULT on D3D9Ex, draws one original textured magazine per eye,
has positive stereo disparity, restores state/RT/DS/viewport/UP buffers even
on injected second-eye failure, releases DEFAULT resources for Reset, and
recreates identical pixels. L/R visible pixels after first draw AND Reset:
Valkyrie 302/302, Volk 307/307, Kasimov 210/210, with non-flat texture colours.
This verifies detached asset consumption, NOT native animated gun GPU output,
world occlusion, scene lighting, headset dimensions or runtime thread safety.
The six CPU inspection views per weapon and all three albedo previews were
visually inspected. L2 build receipt is recorded below after the clean build.

SVWA SHA-256 (profile locked in decoder):

- Valkyrie `97DE1B7B37C733DCB35A8B15EA5AE6FF778A1FB1917305ACF70322C32C30E9DD`
- Volk `CDB998651DEA731CC552CB9C59836C0804A9678FCE2F20C18DAF792551D4D750`
- Kasimov `112CEABEFE13A203094847DCB4FDD441EDAAD774F0BEF28EB6091B09E065E47F`

Original albedo SVRT SHA-256:

- Valkyrie `248760B3A25DD51B02B0BC132DFF711315B44144BEA6A9D07629A0B5BD321155`
- Volk `87C6418A4D491A19BF00FBA2053194EA78D2AFCE3ECB768A8AAB8063D8211708`
- Kasimov `99A9935447D0ECDF47E2A6211F4405CB445F8E768901A522D21F11D04918EF50`

## Next justified integration gate — MAIN

Use current MAIN source, NOT this regression DLL as an installed replacement.
Per weapon, first prove live Pawn.Weapon/mesh/IB bytes + CTAB palette identity,
native ammo producer and reload entry/commit/auto-reload guard. The original
IB hash is a witness, not sufficient live identity alone. No native engine ABI,
socket calibration, draw suppression or guards have been invented here.

Only then install a single default-OFF dispatcher that arbitrates physical left
grip before knife/TMD/support; suppress the original attached clip only during
the matching transaction, render the carried original part, scope the child
palette to BOTH eyes and restore constants. Do not drive the camera/controller
with authored root motion. Native material overrides/normal/specular/scene
lighting are unverified: an albedo fixture must not replace the native material.

L3/L4 UNKNOWN; NOT ready for physical acceptance. Future headset script AFTER
runtime gates (one weapon per test, stay in headset): load a save with the
selected weapon and check normal firing, left knife/right holster; perform one
remove/drop/left-shoulder pickup/insert gesture and verify moving parts + actual
clip/reserve conservation; pause/resume, switch weapon, return to menu and close.
