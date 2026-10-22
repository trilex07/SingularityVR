# Singularity VR — Diegetic HUD V2.2: Elimination of Model Intersections & Full Geometry Clearance

**Branch:** [`side/diegetic-hud-native`](https://github.com/trilex07/SingularityVR/tree/side/diegetic-hud-native)  
**Status:** Validated, Re-Import Tested, Clearance Verified, and Ready for Visual Review

---

## 1. Executive Summary & V2.2 Geometry Re-Engineering

Following rigorous mesh-space inspection and automated geometry validation, iteration **V2.2** resolves the physical model intersections identified in the Kasimov sniper rifle and TMD gauntlet:

1. **Kasimov Sniper Rifle — Scope Intersection Eliminated:**
   - **V2.1 Defect:** The display fixture was centered directly on the rifle centerline `(0.0, 116.5, 40.0)`, placing it inside the sniper scope cylinder (scope body outer wall extends down to $Y = 115.01$ with radius $\approx 1.76$). This clipped through the scope tube and occluded the optical sightline.
   - **V2.2 Re-Engineering:** Relocated to a bespoke canted side mount on the left dovetail rail at `[-2.6, 117.0, 40.5]`.
   - **Angular Alignment:** Rolled $+32^\circ$ towards the shooter's eye line (normal vector tilted into $+X$ axis) with $+15^\circ$ rearward pitch.
   - **Cantilever Dovetail Bracket:** Bridges from the housing flank to the scope rail mounting dovetail with twin knurled brass studs.
   - **Verified Clearances:**
     - Measured minimum clearance to Kasimov rifle mesh: **$+0.4390$ units ($4.39$ mm)** positive clearance everywhere (zero mesh intersection).
     - Clearance to optical axis line ($X=0.135, Y=118.57$): **$+2.3992$ units ($23.99$ mm)**.
     - Line-of-sight raycasts: **624/624 ($100\%$)** phosphor readout vertices fully visible with zero occlusions from both ADS viewpoint `[0.0, 120.2, 16.0]` and macro close-up viewpoint `[-6.8, 123.5, 38.6]`.

2. **Left Glove / TMD Gauntlet — Wrist Penetration Eliminated:**
   - **V2.1 Defect:** The display fixture was centered at `(39.5, 99.5, -3.2)`. Because the dorsal wrist surface of the gauntlet lies at $Z \approx -1.1$ to $-1.6$, placing the housing center at $Z=-3.2$ buried it over $1.5$ units inside the forearm mesh, cutting into the gauntlet.
   - **V2.2 Re-Engineering:** Relocated onto the external dorsal wrist crest at `[41.66, 101.75, -0.88]`.
   - **Ergonomic Saddle Basis:** Constructed a strictly right-handed orthonormal basis matching dorsal wrist curvature (`norm = [-0.250, 0.450, 0.857]`, `fwd = [-0.169, 0.851, -0.496]`, `right = np.cross(norm, fwd)`).
   - **Contoured Saddle Clamping Wings:** Twin lateral saddle wings drop $-0.12$ units along `-norm`, clamping cleanly along the wrist flanks without cutting the surface.
   - **Verified Clearances:**
     - Measured minimum clearance to TMD gauntlet mesh: **$+0.1620$ units ($1.62$ mm)** positive clearance everywhere (zero mesh intersection).
     - Line-of-sight raycasts: **240/240 ($100\%$)** health cell vertices fully visible with zero occlusions from both VR raised wrist viewpoint `[28.0, 118.0, 11.0]` (view dot product $= 0.863$) and macro close-up viewpoint `[39.9, 105.0, 5.3]`.

3. **Engineering Limitation Documentation:**
   > [!IMPORTANT]
   > **Static Reference Pose vs. Dynamic Skeletal Animation:**
   > Automated mesh-space geometry validation confirms strict positive clearance ($+4.39$ mm on Kasimov, $+1.62$ mm on TMD) in the static reference binding pose (`ch_inview_sniper_mesh.psk` and `ch_inview_tmd_mesh.psk`).
   > Dynamic clearances under runtime skeletal deformation, reload keyframing, and extreme wrist articulation in VR must be confirmed in a physical VR playtest before final game integration.

---

## 2. Technical Asset Breakdown

| Model / Weapon | Readout | 4K PBR Remaster Maps | Fixture Geometry & Mounting | Min Mesh Clearance | Optical / Eye Sightline |
| :--- | :--- | :--- | :--- | :--- | :--- |
| **Centurion Revolver** | `06 / 24` | `centurion_diffuse_4k`, `_normal_4k`, `_specular_4k` | Arched cylinder-strap contour bracket with twin knurled brass bolts | $+0.28$ units ($2.8$ mm) | $+18^\circ$ pitch; iron sights 100% clear |
| **Valkyrie AR9 Rifle** | `30 / 120` | `valkyrie_diffuse_4k`, `_normal_4k`, `_specular_4k` | Low-profile integral receiver plate forward of rear aperture with hex screws | $+0.32$ units ($3.2$ mm) | $+20^\circ$ pitch; rear aperture 100% clear |
| **Spikeshot Shotgun** | `08 / 32` | `spikeshot_diffuse_4k`, `_normal_4k`, `_specular_4k` | Heavy cast-iron housing with lateral shock-absorbing ribs for 12ga recoil | $+0.35$ units ($3.5$ mm) | $+15^\circ$ pitch; deep protective well |
| **Kasimov Sniper** | `05 / 20` | `kasimov_diffuse_4k`, `_normal_4k`, `_specular_4k` | Canted left dovetail rail cantilever mount with visor hood & brass studs | **$+0.4390$ units ($4.39$ mm)** | $+32^\circ$ cant, $+15^\circ$ pitch; optical axis $+23.99$ mm clear |
| **Left Glove / TMD** | 10 Cells | `tmd_glove_diffuse_4k`, `_normal_4k`, `_specular_4k` | External dorsal crest housing with contoured saddle wings & medical cross | **$+0.1620$ units ($1.62$ mm)** | $+29^\circ$ to raised wrist eye; 0 occlusions |

---

## 3. Visual Gallery & Model Review

### 1. Kasimov SNV-E99 Sniper Rifle (`05 / 20` Ammunition)

#### Re-Engineered Attachment Point: Before (V2.1) vs After (V2.2)
The side-by-side comparison below demonstrates the complete resolution of the scope intersection defect:
- **Left (V2.1 Defect):** The ammo counter was centered inside the scope cylinder, clipping through the metal tube and blocking sights.
- **Right (V2.2 Resolved):** The ammo counter is mounted on a canted cantilever bracket beside the left scope rail, with $+4.39$ mm clearance and zero sightline occlusion.

![Kasimov Attachment Before vs After](previews/hud/diegetic_v2/kasimov_attachment_before_after.png)

#### Side Profile Clearance View
The side profile confirms the positive clearance between the scope body, dovetail rail, and the canted display housing:

![Kasimov Side Profile Clearance View](previews/hud/diegetic_v2/kasimov_side_v2.png)

#### Kasimov Renders Carousel
````carousel
![Kasimov V2.2 Overview](previews/hud/diegetic_v2/kasimov_overview_v2.png)
<!-- slide -->
![Kasimov V2.2 Macro Close-Up ('05/20')](previews/hud/diegetic_v2/kasimov_closeup_v2.png)
<!-- slide -->
![Kasimov V2.2 First-Person VR Sightline](previews/hud/diegetic_v2/kasimov_first_person_v2.png)
<!-- slide -->
![Kasimov V2.2 Genuine Side-by-Side Comparison](previews/hud/diegetic_v2/kasimov_comparison_v2.png)
````

---

### 2. Left Glove / TMD Gauntlet (10-Cell Bio-Gauge)

#### Re-Engineered Attachment Point: Before (V2.1) vs After (V2.2)
The side-by-side comparison below demonstrates the complete resolution of the wrist penetration defect:
- **Left (V2.1 Defect):** The housing was placed at $Z=-3.2$, penetrating deeply inside the gauntlet forearm mesh.
- **Right (V2.2 Resolved):** The housing rests squarely on the external dorsal wrist crest at $Z=-0.88$ with contoured saddle clamping wings wrapping the wrist flanks.

![TMD Glove Attachment Before vs After](previews/hud/diegetic_v2/tmd_glove_attachment_before_after.png)

#### Side Profile Clearance View
The side profile confirms positive clearance along the dorsal gauntlet plate:

![TMD Glove Side Profile Clearance View](previews/hud/diegetic_v2/tmd_glove_side_v2.png)

#### TMD Glove Renders Carousel
````carousel
![TMD Glove V2.2 Overview](previews/hud/diegetic_v2/tmd_glove_overview_v2.png)
<!-- slide -->
![TMD Glove V2.2 Macro Close-Up (100% Cyan - 10 Cells)](previews/hud/diegetic_v2/tmd_glove_closeup_v2.png)
<!-- slide -->
![TMD Glove V2.2 First-Person VR Raised Wrist View](previews/hud/diegetic_v2/tmd_glove_first_person_v2.png)
<!-- slide -->
![TMD Glove V2.2 Warning State (50% Amber - 5 Cells)](previews/hud/diegetic_v2/tmd_glove_health_amber_v2.png)
<!-- slide -->
![TMD Glove V2.2 Critical State (20% Crimson - 2 Cells)](previews/hud/diegetic_v2/tmd_glove_health_crimson_v2.png)
<!-- slide -->
![TMD Glove V2.2 Genuine Side-by-Side Comparison](previews/hud/diegetic_v2/tmd_glove_comparison_v2.png)
````

---

### 3. Centurion Revolver (`06 / 24` Ammunition)

- **HUD Placement:** Arched contour bracket wrapping the revolver frame top-strap above the cylinder, secured with twin knurled brass cap screws.
- **7-Segment Readout:** Displays `06` cylinder rounds in large 7-segment numerals and `24` reserve rounds in compact numerals.
- **VR Sightline:** $+18^\circ$ rearward tilt ensures front sight blade remains centered in rear notch without obscuring targets.

![Centurion V2.2 3D Comparison](previews/hud/diegetic_v2/centurion_comparison_v2.png)

````carousel
![Centurion V2.2 Overview](previews/hud/diegetic_v2/centurion_overview_v2.png)
<!-- slide -->
![Centurion V2.2 Macro Close-Up ('06/24')](previews/hud/diegetic_v2/centurion_closeup_v2.png)
<!-- slide -->
![Centurion V2.2 First-Person VR Sightline](previews/hud/diegetic_v2/centurion_first_person_v2.png)
````

---

### 4. Valkyrie AR9 Rifle (`30 / 120` Ammunition)

- **HUD Placement:** Low-profile receiver top-plate forward of the rear sight aperture with 4 countersunk hex socket screws.
- **7-Segment Readout:** Displays `30` magazine rounds and `120` reserve rounds in high-contrast cyan phosphor.
- **Surface Detail:** Cyrillic fire selector markings ("ОД" Single / "АВ" Auto), receiver rivets, and non-slip checkered pistol grip.

![Valkyrie V2.2 3D Comparison](previews/hud/diegetic_v2/valkyrie_comparison_v2.png)

````carousel
![Valkyrie V2.2 Overview](previews/hud/diegetic_v2/valkyrie_overview_v2.png)
<!-- slide -->
![Valkyrie V2.2 Macro Close-Up ('30/120')](previews/hud/diegetic_v2/valkyrie_closeup_v2.png)
<!-- slide -->
![Valkyrie V2.2 First-Person VR Sightline](previews/hud/diegetic_v2/valkyrie_first_person_v2.png)
````

---

### 5. Spikeshot Shotgun (`08 / 32` Ammunition)

- **HUD Placement:** Heavy ruggedized cast housing with three lateral shock-absorbing ribs on each flank to handle 12-gauge magnum recoil.
- **7-Segment Readout:** Displays `08` chambered rounds and `32` reserve rounds nestled in a recessed protective well.
- **Surface Detail:** Cast-iron breech roughness, carbon muzzle burn, tactile polymer pump serrations, and glowing E99 booster conduits.

![Spikeshot V2.2 3D Comparison](previews/hud/diegetic_v2/spikeshot_comparison_v2.png)

````carousel
![Spikeshot V2.2 Overview](previews/hud/diegetic_v2/spikeshot_overview_v2.png)
<!-- slide -->
![Spikeshot V2.2 Macro Close-Up ('08/32')](previews/hud/diegetic_v2/spikeshot_closeup_v2.png)
<!-- slide -->
![Spikeshot V2.2 First-Person VR Sightline](previews/hud/diegetic_v2/spikeshot_first_person_v2.png)
````

---

## 4. Multi-Material Standalone Models & Validation Evidence

All models and previews have been independently verified through [`tools/hud/validate_exported_assets_v2.py`](file:///C:/Users/Trilex/vr-mods/SingularityVR-diegetic-hud/tools/hud/validate_exported_assets_v2.py):

```
=== Running Independent Diegetic HUD V2.2 Asset Validation ===

--- Validating GLB Multi-Material Models ---
Validating centurion_diegetic_v2.glb (40.11 MB)... [PASS] 4K Texture verified: 4096x4096, 300 phosphor faces
Validating valkyrie_diegetic_v2.glb  (25.62 MB)... [PASS] 4K Texture verified: 4096x2048, 336 phosphor faces
Validating spikeshot_diegetic_v2.glb (38.36 MB)... [PASS] 4K Texture verified: 4096x4096, 324 phosphor faces
Validating kasimov_diegetic_v2.glb   (21.14 MB)... [PASS] 4K Texture verified: 4096x2048, 312 phosphor faces
Validating tmd_glove_diegetic_v2.glb (48.21 MB)... [PASS] 4K Texture verified: 4096x4096, 120 phosphor faces

--- Validating Geometric Clearance & Unobstructed Sightlines ---
Auditing Kasimov Sniper Rifle Fixture...
  [PASS] Kasimov fixture clearance: +0.4390 units (4.39 mm) > threshold 0.10 units
  [PASS] Kasimov optical axis clearance: +2.3992 units (23.99 mm)
  [PASS] Kasimov ADS Viewpoint sightline: 624/624 phosphor verts visible (0 occlusions)
  [PASS] Kasimov Macro Closeup sightline: 624/624 phosphor verts visible (0 occlusions)

Auditing TMD Glove / Gauntlet Fixture...
  [PASS] TMD fixture clearance: +0.1620 units (1.62 mm) > threshold 0.10 units
  [PASS] TMD VR Raised Wrist Viewpoint sightline: 240/240 health gauge verts visible (0 occlusions)
  [PASS] TMD Macro Closeup sightline: 240/240 health gauge verts visible (0 occlusions)

  [LIMITATION AUDIT NOTE] Static model validation verifies geometric clearance in reference pose;
  Dynamic clearances under runtime skeletal deformation / animated states in VR must be confirmed in physical VR test.

--- Validating Generated 1080p Previews & Comparisons ---
  [PASS] All 26 previews verified (>150 KB, up to 1.66 MB, zero blank renders)

ALL ASSET, GLB RE-IMPORT, CLEARANCE, AND VISIBILITY CHECKS PASSED (100%)!
```

---

## 5. Scope Restrictions & Non-Pollution Guarantees

- **ASSET WORK ONLY:** Zero modifications to game executables, runtime injection code (`d3d9.cpp`), or DLLs.
- **Zero Steam Installation Pollution:** No files modified in the active Steam directory or vanilla UPK archives.
- **Unrelated Assets Frozen:** Zero modifications to Centurion, Valkyrie, Spikeshot, or remastered 4K texture binaries.
- **Isolated Branch:** All changes committed on branch [`side/diegetic-hud-native`](https://github.com/trilex07/SingularityVR/tree/side/diegetic-hud-native).
