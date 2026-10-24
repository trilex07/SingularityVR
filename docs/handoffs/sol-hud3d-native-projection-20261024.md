# Diegetic weapon HUD: body-owned stereo projection

Base: `d3dde91b6f4df6a22778b9546418c709449229d5`.
Branch: `fix/sol-diegetic-hud-native-projection`.
SIDE work only: no Steam deployment, launch, installed configuration change or merge.
Historical MAIN evidence was read from the repository's saved deployment evidence.

## Evidence and correction of the previous handoff

The old gameplay log (`hud3d-20261024-204229/view_matrix.log`, lines
7779–7824 and 10208–10212) proves that the body draw and HUD capture ran, but
the HUD projection rejected both eyes. It does **not** contain the captured
matrix values and cannot establish which camera/palette produced those bounds.

Two source contracts were violated:

1. Capture accepted all draws with matching vertex count. Valkyrie's body is
   `base=0,min=0,first=0,tris=12834,verts=11387|11378`, root slot 0. Its later
   attachment is `first=38502,tris=8` and can also use verts=11387, but slot 0
   is a different bone. The attachment could overwrite the body snapshot.
   The new `MountForBodyDraw` accepts only the audited primary body, with the
   existing Centurion 6767/root1 and Volk 10835/root2 signatures as well.
2. Present projected a saved weapon transform with `g_vfxBaseCam` from a
   different, non-moved world draw, or the first remaining camera slot. This
   is not the body shader's named ViewProjectionMatrix and is not captured
   at the same time. Now the exact named, valid, remappable body camera is
   stored with the chain and both existing eye remaps. Missing camera refuses
   capture; it never substitutes an unrelated camera. `GunC` remains applied
   once in `skin * LocalToWorld * GunC`, not again in projection.

These are proven source contract defects. Attribution of every historical
offscreen frame to a specific one remains INFERRED, not runtime-replayed proof.

**The proposed Y-sign change in HUD3D_ASTRA_HANDOFF.md is wrong.** We independently
searched the named original cooked SkeletalMesh export and matched every native
32-byte-stride position against both signs of the source PSK:

- Centurion: all 5799 positions match reflected Y; mount-to-nearest-vertex
  distance 0.48165 UU with reflection versus 214.01867 UU without it.
- Valkyrie: all 11387 positions match reflected Y; 1.53977 versus 209.41658 UU.
- Volk: all 8297 positions match reflected Y; 0.67688 versus 214.38932 UU.

The original fixture PSKs have positive Y, including the independently exported
`ch_inview_alt_pistol_mesh.psk`. Native cooked streams have negative Y. Preserve
`ToNativeVertexSpace({x,y,z}) = {x,-y,z}`. The positive hardcoded muzzle point
elsewhere is not proof of GPU vertex coordinates and was not changed here.
Full metrics, mesh identity and input hashes are in `evidence/hud3d-native-space/`.
No original geometry/package data is committed.

The handoff's winding recommendation is also not applied. With the existing
reflection the source winding reverses for ALL faces (including the bezel),
not only four flanks. This patch preserves the existing double-sided draw and
does not introduce unmeasured backface or depth behavior.

## Rendering changes

- `diegetic_weapon_hud.h`: exact body selector; coherent pose + two eye camera
  snapshot; canonical ROW/COL register conversion; existing age/frame gates.
- `d3d9.cpp`: capture from body CTAB camera, no global Present camera lookup.
- `diegetic_hud_draw.h`: production projection and D3D9 submission shared with
  the offline GPU tests. Both eyes are preflighted before drawing; an invalid
  eye cannot leave a partial HUD underneath the legacy plate.
- Explicit fixed-function texture stage, blend, color-write, alpha-test,
  fog/stencil and scissor setup. Restore RT/depth before applying saved state,
  because SetRenderTarget resets the viewport. Stream/shaders/textures are
  restored through the existing D3DSBT_ALL block.
- Draw failures, missing atlas, stale pose or rejected projection leave the
  legacy 2D counter enabled. Feature remains default OFF.
- No input, ammunition, firearm, TMD, inventory, localization, NIS/OLED or
  WeaponLocker behavior changed. At most the existing 22/24 HUD quad draws
  per frame; no per-frame allocations or readbacks added.

## Verification

`powershell -File tools/hud/test-diegetic-hud.ps1`:

- Existing HUD layout/atlas/freshness L1 checks PASS.
- 24 stereo projection cases: 3 guns, 4 rotations, 4128x2208 and 5264x2684;
  exact body/attachment rejection, 11378 alias, ROW/COL conversion, near-plane,
  nonfinite and stale/weapon-mismatched pose rejection PASS.
- Actual atlas + production Render helper on D3D9 **and D3D9Ex**: 108 pixel
  cases each (216 total). Three weapons, three rotations, three resolutions,
  before/after Reset. Pixels present in both halves; hostile inherited
  draw states produce the exact same pixels; RT/viewport/scissor/color-write/
  texture-stage restoration, draw failure and missing-art/invalid-plan paths PASS.
- Offline PNG: `build/fixtures/diegetic-d3d9-stereo.png`. Synthetic cameras,
  not a screenshot of a working game or physical VR acceptance.
- Existing ammo-layout/probe, HUD reseat (203), next-weapon reload (30000
  synthetic transactions), measured aim, WeaponLocker preview/policy/layout,
  NIS (178), OLED CPU/D3D11 (all presets, <=1 LSB) PASS.
- Build: `powershell -File spikes/view_matrix/build.ps1`, x86 PE32 `/O2`.
  Repository-wide pre-existing compiler/analyzer warnings remain.
  DLL SHA256: `8D8D532CA67E66C4F898691EEE2A895AA5F14C29B592299B268022E7713B9B7B`.

Reproduce the original-byte audit with explicit private inputs:

```powershell
py -3 tools/hud/audit_native_hud_space.py --psk <original.psk> --package <decompressed.xxx> --mesh ch_inview_shotgun_mesh --mount 0 116.8 42
```

Valkyrie uses `ch_inview_machinegun_mesh`, mount `0 117.2 35`, original RvGame
package. Centurion uses `ch_inview_alt_pistol_mesh`, mount `0 115.3 43.5`,
original wp_SP_ARPistol_SF package. Private source hashes are in the JSON evidence.

## MAIN acceptance / remaining limits

GO for review and a controlled runtime candidate, **not stable/physical PASS**.
Keep the existing atlas (SHA256
`9ED4A5C365E5521C12EFFC7E3E7C9DF7833EAC2B4B71D45FCAB18ADD4B8D81F9`).
Enable only the existing `[Render] DiegeticWeaponHud=1` on MAIN's combined base.

One session, no chat/headset switching: hold Volk/shotgun, Valkyrie and Centurion
in turn; inspect both eyes, rotate/roll each gun, fire/reload to check counts,
then pause/resume and switch weapons. Expect a counter fixed to the receiver,
not the old floating plate; no single-eye copy, stale pose or attachment jump.

Receipts:

- `diegetic hud: body pose ... first=0 tris=... root=... vp=c... conv=...`
- `diegetic hud 3D: ... state=1 handledEyes=3 submitted=11/11` (12/12 Valkyrie),
  per-eye bounds and backbuffer size. This is API submission, not pixel proof.
- On a failure: `body camera unavailable`, or `state=0` projection rejection,
  or `state=2` D3D failure. Six state-change receipts per weapon include all
  chain and per-eye matrix rows so the exact next failure can be replayed
  offline. No repeating full matrix dumps or GPU synchronization.

Original receiver placement/size, scene occlusion and readability are still
physical gates. Present-time rendering retains the baseline no-depth overlay;
it is not a new depth-occluded mesh renderer. The session must also confirm that
the actual named camera is available for each native body shader.
