# HUD 3D Weapon Mount — Technical Handoff to Astra

- **Repository:** `https://github.com/trilex07/SingularityVR`
- **Branch:** `fix/diegetic-hud-3d-weapon-mount`
- **Last Deployed Candidate Commit:** `8c50d91045e0f997ebe6b8f49bd27ded70bfd15b` (`8c50d91`)
- **Deployed `d3d9.dll` SHA256:** `36C14F258D5E87C626BD390B1520B19C8CB81C15D50BE46573362F8D36B9EAB9`
- **Installed `SingularityVR.ini` SHA256:** `366BEA6E7F5BAF79F5CF2B0F5A1025D7E126201544B10B3A4C3DB215BA27C2` (`DiegeticWeaponHud=1`)
- **Status:** **NOT WORKING IN GAMEPLAY** (3D projection fails `EyeBoundsVisible` check; frame-atomic fallback keeps the legacy 2D floating ammo plate visible).

---

## 1. Current State

- **What Works:**
  - `diegetic_hud_atlas.png` (`1024x512`) loads cleanly at runtime from `Binaries/vr-assets/hud/diegetic/diegetic_hud_atlas.png` (`diegetic hud: atlas ready 1024x512 from vr-assets/hud/diegetic`).
  - Live native ammunition counts (`mClipAmmo`, `mBinAmmo`) are read from the active UE3 `ARvHUD` instance.
  - `RecordLiveTransform` captures `skin * local * controller` in `Hook_DrawIndexedPrim` for Spikeshot (`8297` verts) and Valkyrie (`11387` verts), and `GetLiveTransform` passes the seqlock/tick/frame freshness check in `DrawDiegeticCenturion`.
  - `EyeBoundsVisible` and `g_diegeticDrew[plate]` (`drewEye == 3`) prevent invisible/offscreen 3D draws from suppressing the legacy 2D ammo plate (`DrawAmmoPlate`).
- **What Fails:**
  - In real gameplay (`GameplayStereo`), the 3D fixture vertices built by `BuildFixtureQuads3D` and transformed by `chain = skin * local * controller` project either far outside the eye viewports (e.g. `bboxL=(5400,1287)-(41852,5445) bboxR=(4703,1171)-(5702,1392)` on a `4128x2208` swapchain) or behind the camera near plane (`cl[k][3] <= 0.001f`, yielding empty `1e9..-1e9` bounds).
  - Because `drewEye` stays `0`, `g_diegeticDrew[plate]` is `0` and `DrawAmmoPlate` renders the old floating 2D ammo plate every frame.

---

## 2. Source Code Map

| Component | File & Lines | Responsibility |
| :--- | :--- | :--- |
| **Mount Configs & 3D Math** | `spikes/view_matrix/diegetic_weapon_hud.h:44-70` (`WeaponMount`, `MountForVerts`) | Per-weapon mount dimensions (`cxUU, cyUU, czUU`, `wUU, hUU, dUU`, `pitchDeg`, `rootSlot`, `plate`). |
| **PSK/Native Space & Seqlock** | `spikes/view_matrix/diegetic_weapon_hud.h:192-284` (`ToNativeVertexSpace`, `RecordLiveTransform`, `GetLiveTransform`) | Coordinate conversion, `Mat4`, `Point`, seqlock transform storage. |
| **Viewport Bounds Gate** | `spikes/view_matrix/diegetic_weapon_hud.h:289-307` (`EyeBoundsVisible`) | Verifies projected quad bounding box intersects the eye's half-viewport (`[0, W/2)` or `[W/2, W)`). |
| **3D Fixture Builder** | `spikes/view_matrix/diegetic_weapon_hud.h:315-448` (`BuildFixtureQuads3D`) | Generates 4 housing flanks, 1 bezel top face, 1 screen bed, and dynamic 7-segment digit/slash quads. |
| **Live Transform Capture** | `spikes/view_matrix/d3d9.cpp:20700-20732` (inside `Hook_DrawIndexedPrim`) | Reads `BoneMatrices + rootSlot*3` (`skin`), `LocalToWorld` (`local`), and `BuildGunC` (`controller`), then stores `skin * local * controller`. |
| **Present Overlay Call Order** | `spikes/view_matrix/d3d9.cpp:6088-6089` (inside `CopyBackbufferToD3D11`) | Calls `DrawDiegeticCenturion(dev)` immediately before `DrawAmmoPlate(dev)`. |
| **3D Projection & Render** | `spikes/view_matrix/d3d9.cpp:31328-31597` (`DrawDiegeticCenturion`) | Transforms `Quad3D` by `chain`, projects per eye via `WorldToClip` + `ApplyOffset` + `ApplyEyeRemap`, submits `D3DFVF_XYZRHW` strips, sets `g_diegeticDrew[mo->plate]`. |
| **Legacy 2D Plate Fallback** | `spikes/view_matrix/d3d9.cpp:31096-31315` (`DrawAmmoPlate`) | Checks `(g_diegeticDrew[plate] & 3) == 3` at line `31129-31130`; draws the 2D floating plate if 3D HUD did not draw in both eyes. |
| **Unit Tests** | `spikes/view_matrix/tests/test-diegetic-centurion.cpp` | L1 unit tests built/run via `build/diegetic_fixtures.ps1`. |

---

## 3. Latest Evidence (`hud3d-20261024-204229`)

Autonomous `dvr-xrsim` run `hud3d-20261024-204229` navigated the main menu, clicked `"Kontynuuj grę"`, loaded the save into real gameplay (`SceneMode: ... -> GameplayStereo`), held **Spikeshot (`8297` verts)** across 4 weapon poses, then opened the R3 weapon wheel and switched in-game to **Valkyrie AR9 (`11387` verts)**.

- **Evidence Directory:** `C:\Users\Trilex\vr-mods\SingularityVR\deployment\agent-verify-20261010\evidence\hud3d-20261024-204229`
- **Runtime Log:** `C:\Users\Trilex\vr-mods\SingularityVR\deployment\agent-verify-20261010\evidence\hud3d-20261024-204229\view_matrix.log`
  - Line `7764`: `SceneMode: MonoMenuOrCutscene -> GameplayStereo (frame=3935 ...)`
  - Line `7780`: `diegetic hud: captured live 3D transform for spikeshot (8297) at tick=41050187 frame=3941`
  - Line `7823`: `diegetic hud: atlas ready 1024x512 from vr-assets/hud/diegetic`
  - Line `7824`: `diegetic hud 3D: projection out of eye bounds for spikeshot (8297) drewEye=0 bboxL=(5400,1287)-(41852,5445) bboxR=(4703,1171)-(5702,1392); falling back to 2D plate`
  - Line `8959`: `diegetic hud 3D: projection out of eye bounds for spikeshot (8297) drewEye=0 bboxL=(1000000000,1000000000)-(-1000000000,-1000000000) bboxR=(5851,2644)-(12772,7339); falling back to 2D plate`
  - Line `10190`: `owned inventory: selected existing weapon mesh=11387 via audited pawn selector`
  - Line `10209`: `diegetic hud: captured live 3D transform for valkyrie (11387) at tick=41062640 frame=5052`
  - Line `10212`: `diegetic hud 3D: projection out of eye bounds for valkyrie (11387) drewEye=0 bboxL=(1000000000,1000000000)-(-1000000000,-1000000000) bboxR=(1000000000,1000000000)-(-1000000000,-1000000000); falling back to 2D plate`
- **Verified Gameplay Captures (with visible weapon + legacy 2D plate fallback):**
  - Spikeshot (`8297`) neutral pose: `hud3d-20261024-204229\02-weapon-pose1-neutral_sbs.png` (`_left.png`, `_right.png`)
  - Spikeshot (`8297`) angled pose: `hud3d-20261024-204229\03-weapon-pose2-angled_sbs.png`
  - Spikeshot (`8297`) 45° roll: `hud3d-20261024-204229\04a-weapon-pose3-roll45_sbs.png`
  - Spikeshot (`8297`) 90° roll: `hud3d-20261024-204229\04b-weapon-pose3-roll90_sbs.png`
  - Valkyrie (`11387`) after R3 switch: `hud3d-20261024-204229\05-weapon-after-switch_sbs.png` (`_left.png`, `_right.png`)
- **Automated Test Script (for reference only):** `C:\Users\Trilex\.gemini\antigravity\brain\141acd33-bfff-4b49-805d-23f224b08bce\scratch\verify_3d_hud_xrsim.ps1` (Note: Singularity must NOT be launched with `-ResX=1280 -ResY=720`, otherwise the backbuffer size matches the `1280x720` GFx menu RTT and `CaptureVrMenuTarget` skips menu capture).

---

## 4. Known Failures: Confirmed Causes vs. Hypotheses

### Confirmed Cause #1: `ToNativeVertexSpace` Double-Negated `Y` (`cyUU` is Already in Native GPU Space)
- In commit `8c50d91`, `ToNativeVertexSpace` (`diegetic_weapon_hud.h:195-197`) negated `Y` (`{pskPt.x, -pskPt.y, pskPt.z}`), assuming `WeaponMount::cyUU` (`+115.3`, `+117.2`, `+116.8`) had the sign of raw ActorX `PNTS0000`.
- **Proof from `d3d9.cpp:20688` (`g_weaponMeasuredAim`) and `tools/reload/audit_weapon_mounts.py`:**
  - In raw ActorX PSK (`PNTS0000`), `muzzle_flash` has `Y = -114.6957`, and the weapon receiver sits at `Y ≈ -115` to `-117`.
  - In Singularity's native cooked GPU vertex stream (`audit_next_weapon_space.py:23`, `expected = points * [1, -1, 1]`), `Y` is negated relative to `PNTS0000`, so the native GPU vertex stream has **`Y = +114.6957`** (which is why `d3d9.cpp:20688` uses `const hipknife::V p{.000005440695f, +114.69569557f, 63.52872816f}` directly with `centurionvisual::Point(p, chain)`!).
  - The fixture centers in `WeaponMount` (`cyUU = +115.3f, +117.2f, +116.8f`) **already had `+Y`**. Negating `Y` in `ToNativeVertexSpace` flipped `center.y` to `-115.3 / -117.2 / -116.8` — placing the 3D HUD ~230 UU (~4.4 m) behind the weapon receiver, right near/behind the camera plane (`cl[k][3] ≈ 0`), causing the massive `bboxL=(5400..41852)` projection blowup and behind-camera clipping!

### Confirmed Cause #2: Unfiltered Multi-Section Draw Calls in `Hook_DrawIndexedPrim` (`d3d9.cpp:20702-20732`)
- In `d3d9.cpp:20702-20707`, `RecordLiveTransform` only checks `MountForVerts(numVertices)` and `g_boundShaderLayout.boneRegs >= (mo->rootSlot + 1) * 3`, without checking `baseVertex == 0 && minIndex == 0 && startIndex == 0 && primCount == <bodyPrimCount>`.
- **Proof from `weapon_reload_runtime.h:19-27` (`weaponreload::DrawPart`) and `weapon_reload_animation.h:16-20`:**
  - **Valkyrie (`11387` total wedges)** is a **2-section mesh**:
    1. Section 0 (Body): `base=0, min=0, vertices=11387 (or 11378), first=0, triangles=12834`, 6-bone palette where `slot 0 = cog_machinegun`.
    2. Section 1 (Attachment): `base=0, first=38502 (12834*3), triangles=8, min=0 or 11378, vertices=11387 or 9`, **1-bone palette (`boneRegs = 3`)** where `slot 0` is `[cog_gun]` (an attachment bone, NOT `cog_machinegun`!).
  - Because `Valkyrie` has `mo->rootSlot == 0`, Section 1's 1-bone palette (`boneRegs == 3 >= (0 + 1) * 3`) passes the check in `d3d9.cpp:20707` and **overwrites** `RecordLiveTransform` every frame with the wrong bone palette!
  - Compare with `d3d9.cpp:20677` (`Centurion 5799`), which explicitly gates on `baseVertex==0 && numVertices==5799 && startIndex==0 && primCount==6767`, and `weaponreload::DrawPart` (`weapon_reload_runtime.h:19-27`), which gates on:
    - Centurion (`5799`): `baseVertex == 0 && minIndex == 0 && startIndex == 0 && primCount == 6767`
    - Valkyrie (`11387` or `11378`): `baseVertex == 0 && minIndex == 0 && startIndex == 0 && primCount == 12834`
    - Spikeshot (`8297`): `baseVertex == 0 && minIndex == 0 && startIndex == 0 && primCount == 10835`

### Confirmed Cause #3: Outward Winding of the 4 Housing Flank Quads
- In `BuildFixtureQuads3D` (`diegetic_weapon_hud.h:366-369`), the bezel top (`{p3, p2, p1, p0}`), screen bed (`{s_tl, s_tr, s_br, s_bl}`), and glyphs have outward normal `+norm` (`(p1 - p0) x (p3 - p0)` along `+norm`), which projects to clockwise (CW) winding on screen.
- However, the 4 flank quads (`{p0, p1, p5, p4}`, `{p2, p3, p7, p6}`, `{p3, p0, p4, p7}`, `{p1, p2, p6, p5}`) have inward-pointing normals (opposite to `{p1, p0, p4, p5}`, `{p3, p2, p6, p7}`, `{p0, p3, p7, p4}`, `{p2, p1, p5, p6}`). If back-face culling is enabled (`D3DCULL_CCW` or screen-space signed-area culling), the 4 flanks must use outward CW winding `{p1, p0, p4, p5}`, `{p3, p2, p6, p7}`, `{p0, p3, p7, p4}`, `{p2, p1, p5, p6}`.

---

## 5. Critical Technical Details

### Weapon Catalog & Audited Primary Body Draws
| Weapon | Catalog `verts` (`g_gunVerts`) | Body Draw `numVertices` | Body `primCount` | `rootSlot` (in Body Palette) | Legacy `plate` Index | Fixture Center `(cx, cy, cz)` UU | Box Size `(w, h, d)` UU | Pitch (`pitchDeg`) | Readout Pad (`clip/res`) |
| :--- | :---: | :---: | :---: | :---: | :---: | :---: | :---: | :---: | :---: |
| **Centurion** | `5799` | `5799` | `6767` | `1` (`cog_alt_pistol`) | `0` | `(0.0, 115.3, 43.5)` | `(2.6, 0.45, 1.8)` | `18°` | `2 / 2` (`06/24`) |
| **Valkyrie AR9** | `11387` | `11387` or `11378` | `12834` | `0` (`cog_machinegun`) | `1` | `(0.0, 117.2, 35.0)` | `(2.8, 0.45, 1.9)` | `20°` | `2 / 3` (`30/120`) |
| **Spikeshot** | `8297` | `8297` | `10835` | `2` (`cog_shotgun`) | `2` | `(0.0, 116.8, 42.0)` | `(2.8, 0.45, 1.9)` | `15°` | `2 / 2` (`08/32`) |

### Coordinate Systems & Matrices
- **Native GPU Vertex Stream Space (`v_native`):**
  - Right-handed/reflected weapon model space where `+X` is right across the receiver, `+Y` is up (receiver top-strap sits at `Y ≈ +115..+117 UU`, `Z ≈ +35..+43.5 UU`), and `+Z` is forward toward the muzzle (`d3d9.cpp:20688`).
  - With `pitchDeg` tilt toward the shooter:
    - `right = (1, 0, 0)`
    - `fwd = (0, sin(pitch), cos(pitch))` (points up/forward along the tilted screen surface, from bottom of display to top of display)
    - `norm = (0, cos(pitch), -sin(pitch))` (outward display surface normal, pointing up and rearward `-Z` toward the shooter)
- **World / Translated-World Matrix Chain (`chain`):**
  - Row-vector convention (`v_world = Point(v_native, chain)` where `chain = skin * local * controller`).
  - `skin`: 3x4 GPU bone matrix at `g_boundShaderLayout.bones + rootSlot * 3`, transposed to 4x4 row-vector `armbind::Mat` via `armbind::FromGpu(palette, skin)`.
  - `local`: 4x4 row-vector `LocalToWorld` matrix at `g_boundShaderLayout.local` (already row-vector in UE3 VS constants; read directly via `GetVertexShaderConstantF(local, &local.m[0][0], 4)`).
  - `controller`: 4x4 row-vector VR controller delta matrix from `BuildGunC(controller.m)`.
  - Output of `Point(v_native, chain)` is in **UE3 Translated-World Space (in UU)**, where the camera origin is near `(0, 0, 0)`.
- **Per-Eye Projection (`DrawDiegeticCenturion`, `d3d9.cpp:31480-31568`):**
  - Uses `g_vfxBaseCam` (`g_vfxBaseCamConv`) captured during `StereoPairBody`, or falls back to `g_camMats[i]`.
  - For `eye = 1` (right eye), applies `ApplyOffset(m, conv, g_eyeDeltaUU)` first, then `ApplyEyeRemap(m, conv, eye ? +0.5f : -0.5f)` (which scales clip `X` into the left or right half of the full side-by-side backbuffer).
  - `WorldToClip(m, conv, pt, cl)` produces full-backbuffer clip coordinates `cl[0..3]`, so screen `SX = (cl[0]/cl[3] * 0.5f + 0.5f) * d.Width` and `SY = (0.5f - cl[1]/cl[3] * 0.5f) * d.Height` map directly into `[0, d.Width/2)` for `eye=0` and `[d.Width/2, d.Width)` for `eye=1`.
- **Depth & Occlusion Strategy:**
  - `DrawDiegeticCenturion` runs in `CopyBackbufferToD3D11` (`d3d9.cpp:6088`) on the resolved backbuffer with `D3DRS_ZENABLE = FALSE` and `SetDepthStencilSurface(nullptr)`.
  - Because hardware Z-buffering is unbound at Present time, `DrawDiegeticCenturion` relies on **painter's order** (flanks `0..3` -> bezel `4` -> screen bed `5` -> digits `6..N`) and needs **back-face culling** (either `D3DRS_CULLMODE = D3DCULL_CCW` or screen-space signed-area culling before `DrawPrimitiveUP`) so the display and flanks do not render through the back of the receiver when the weapon is rolled 90°+ away from the eye.

---

## 6. Existing Assets & Reference Measurements

- **Runtime Atlas (Installed & Tracked in Git):**
  - `assets/processed/hud/diegetic/diegetic_hud_atlas.png` (`1024x512`, SHA256 `9ED4A5C365E5521C12EFFC7E3E7C9DF7833EAC2B4B71D45FCAB18ADD4B8D81F9`)
  - `assets/processed/hud/diegetic/diegetic_hud_atlas.json`
  - Installed copy: `C:\Program Files (x86)\Steam\steamapps\common\Singularity\Binaries\vr-assets\hud\diegetic\diegetic_hud_atlas.png`
- **SIDE V2.2 Physical HUD Models, Generator & Review Docs (Commit `ff29efd`):**
  - Generator script with exact mounting coordinates & 3D box math: `git show ff29efd:tools/hud/prepare_diegetic_models_v2.py`
  - Validation & independent GLB render scripts: `tools/hud/validate_exported_assets_v2.py`, `tools/hud/validate_glb_independent_render.py`
  - Review documentation & renders: `docs/diegetic_hud_v2_review.md`, `docs/assets/hud/diegetic_v2/`

---

## 7. What To Do Next (Smallest Path to Working Physical 3D HUD)

1. **Fix `ToNativeVertexSpace` in `spikes/view_matrix/diegetic_weapon_hud.h:195`:**
   - Change `ToNativeVertexSpace` to identity (`return { pskPt.x, pskPt.y, pskPt.z };`) so `center = (0, +115.3/+117.2/+116.8, +43.5/+35.0/+42.0)`, `fwd = (0, sin_p, cos_p)`, and `norm = (0, cos_p, -sin_p)` stay in native GPU vertex space (matching `d3d9.cpp:20688`).
   - Update Test #18 in `spikes/view_matrix/tests/test-diegetic-centurion.cpp` (`+mv->cyUU` instead of `-mv->cyUU`).
2. **Gate `RecordLiveTransform` on the Audited Primary Receiver Body Draw in `spikes/view_matrix/d3d9.cpp:20702-20729`:**
   - Require `baseVertex == 0 && minIndex == 0 && startIndex == 0` and match the primary body section (`(numVertices == 5799 && primCount == 6767)` for Centurion, `((numVertices == 11387 || numVertices == 11378) && primCount == 12834)` for Valkyrie, `(numVertices == 8297 && primCount == 10835)` for Spikeshot).
   - Always record under the canonical catalog vertex ID (`5799`, `11387`, `8297`) so `GetLiveTransform(heldVerts, ...)` matches `g_gunVerts` even if Valkyrie's body chunk is drawn with `numVertices == 11378`.
3. **Enable Back-Face Culling in `DrawDiegeticCenturion`:**
   - Fix the 4 flank quads in `BuildFixtureQuads3D` (`diegetic_weapon_hud.h:366-369`) to outward CW winding (`{p1, p0, p4, p5}`, `{p3, p2, p6, p7}`, `{p0, p3, p7, p4}`, `{p2, p1, p5, p6}`).
   - Check screen-space signed area (or set `D3DRS_CULLMODE, D3DCULL_CCW`) in `DrawDiegeticCenturion` so back-facing quads are culled and the display is hidden when the weapon is rolled away from the viewer.
4. **Log `worldQuads` & `cl` on First Draw / Projection Reject:**
   - Log `worldQuads[4].p[0]` (bezel corner in translated-world UU) and `cl[0][0..3]` once per weapon in `DrawDiegeticCenturion` to immediately verify the 3D coordinates in `view_matrix.log`.
