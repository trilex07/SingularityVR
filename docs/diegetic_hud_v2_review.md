# Singularity VR — Diegetic HUD V2 & Full Texture Remaster

**Branch:** [`side/diegetic-hud-native`](https://github.com/trilex07/SingularityVR/tree/side/diegetic-hud-native)  
**Commit:** [`e46bddf`](https://github.com/trilex07/SingularityVR/commit/e46bddf)

---

## 1. Executive Summary & Quality Targets Achieved

This milestone delivers **Diegetic HUD Visual Polish V2** combined with a **comprehensive 4K PBR visual remaster** of the weapons and gloves for Singularity VR.

Key achievements:
1. **Bespoke HUD Fixtures (Zero 'Same Screen Pasted' Feeling):**
   - Each weapon receives a customized mounting bracket engineered to match its physical geometry, mass distribution, and recoil characteristics.
   - Fixed spacing and improved typography of segmented ammo numerals with zero overlap and clean bloom falloff.
2. **Texture & Surface Remaster (4K PBR):**
   - **4096×4096 / 4096×2048** resolution upgrades with multi-scale frequency detail synthesis.
   - Machined brushed metal grain along mechanical axes, authentic blued steel patina, micro-scratched bare metal edge wear along slide rails and cylinder fluting.
   - Tactile cordura canvas weave with visible individual fibers, conditioned leather palm patches, and raised dual-nylon structural stitching on the gloves.
   - Perturbed unit-length normal maps ($\vec{N}'$) and high dynamic range specular maps matching Direct3D9 DXT1/DXT5 hardware requirements.
3. **VR First-Person Sightline Guarantee:**
   - Mount angles (+15° to +20°) aligned directly with natural VR controller hold angles.
   - 100% clear sightlines: Iron sights, aperture notches, and optical scope apertures remain completely unobstructed.

---

## 2. Technical Asset Breakdown

| Model / Weapon | Remastered 4K Maps (PNG & DDS) | Bespoke Fixture Geometry | Vertices / Tris | Sightline & VR Hold Angle |
| :--- | :--- | :--- | :--- | :--- |
| **Centurion Revolver** | `centurion_diffuse_4k` (DXT5), `_normal_4k` (DXT1), `_specular_4k` | Arched cylinder-strap contour bracket with twin knurled brass bolts | 3,471 / 3,275 | +18° rearward tilt; front blade centered in rear notch |
| **Valkyrie AR9 Rifle** | `valkyrie_diffuse_4k` (DXT5), `_normal_4k` (DXT1), `_specular_4k` | Low-profile integral receiver plate forward of rear aperture with hex screws | 10,829 / 13,010 | +20° rearward tilt; zero obstruction of aperture |
| **Spikeshot Shotgun** | `spikeshot_diffuse_4k` (DXT5), `_normal_4k` (DXT1), `_specular_4k` | Heavy cast-iron housing with lateral shock-absorbing ribs for 12ga recoil | 6,767 / 11,039 | +15° rearward tilt; deep recessed protective well |
| **Kasimov Sniper** | `kasimov_diffuse_4k` (DXT5), `_normal_4k` (DXT1), `_specular_4k` | Cantilever scope mount bracket nestled below optical axis with visor hood | 10,176 / 13,242 | +15° rearward tilt; peripheral glance without scope block |
| **Left Glove / TMD** | `tmd_glove_diffuse_4k` (DXT5), `_normal_4k` (DXT1), `_specular_4k` | Ergonomic curved dorsal cuff with 10 vacuum-tube cells & medical cross | 7,405 / 12,495 | Facing eye line when arm naturally raised in VR |

---

## 3. Visual Gallery & Model Review

### 1. Centurion Revolver (`06 / 24` Ammo)

**Technical Notes:**
- **HUD Placement:** Arched contour plate wrapping the revolver frame top-strap above the cylinder, locked with twin knurled brass cap screws.
- **Texture Remaster Highlights:** 4K blued steel patina, micro-scratched bare gunmetal edge highlights along cylinder fluting chamfers, brushed directional steel grain along barrel machining axes, and crisp cylinder stop notches.
- **VR Readability:** The +18° rearward tilt ensures front sight blade remains perfectly centered in the rear notch with zero obstruction. High-contrast segmented digits (`06 / 24`) in E99 cyan are immediately legible at arm's length.

![Centurion Texture Macro Comparison](/C:/Users/Trilex/.gemini/antigravity/brain/37e373c5-dc1a-4785-b9b3-94b614b04e5d/centurion_texture_comparison.png)

![Centurion V2 3D Comparison](/C:/Users/Trilex/.gemini/antigravity/brain/37e373c5-dc1a-4785-b9b3-94b614b04e5d/centurion_comparison_v2.png)

````carousel
![Centurion V2 Overview](/C:/Users/Trilex/.gemini/antigravity/brain/37e373c5-dc1a-4785-b9b3-94b614b04e5d/centurion_overview_v2.png)
<!-- slide -->
![Centurion V2 Macro Close-Up](/C:/Users/Trilex/.gemini/antigravity/brain/37e373c5-dc1a-4785-b9b3-94b614b04e5d/centurion_closeup_v2.png)
<!-- slide -->
![Centurion V2 First-Person VR Sightline](/C:/Users/Trilex/.gemini/antigravity/brain/37e373c5-dc1a-4785-b9b3-94b614b04e5d/centurion_first_person_v2.png)
````

---

### 2. Valkyrie AR9 Rifle (`30 / 120` Ammo)

**Technical Notes:**
- **HUD Placement:** Integral low-profile receiver top-plate housing forward of the rear sight aperture with countersunk hex socket screws.
- **Texture Remaster Highlights:** 4K stamped sheet steel receiver with crisp Soviet factory proof marks, enhanced Cyrillic fire selector markings ("ОД" Single / "АВ" Auto) with mechanical detent relief, textured bakelite pistol grip with authentic stippled non-slip checkering, and machined Picatinny cuts.
- **VR Readability:** The +20° rearward tilt provides a clear sightline without raising the rifle unnaturally; zero obstruction of the rear aperture or front post.

![Valkyrie Texture Macro Comparison](/C:/Users/Trilex/.gemini/antigravity/brain/37e373c5-dc1a-4785-b9b3-94b614b04e5d/valkyrie_texture_comparison.png)

![Valkyrie V2 3D Comparison](/C:/Users/Trilex/.gemini/antigravity/brain/37e373c5-dc1a-4785-b9b3-94b614b04e5d/valkyrie_comparison_v2.png)

````carousel
![Valkyrie V2 Overview](/C:/Users/Trilex/.gemini/antigravity/brain/37e373c5-dc1a-4785-b9b3-94b614b04e5d/valkyrie_overview_v2.png)
<!-- slide -->
![Valkyrie V2 Macro Close-Up](/C:/Users/Trilex/.gemini/antigravity/brain/37e373c5-dc1a-4785-b9b3-94b614b04e5d/valkyrie_closeup_v2.png)
<!-- slide -->
![Valkyrie V2 First-Person VR Sightline](/C:/Users/Trilex/.gemini/antigravity/brain/37e373c5-dc1a-4785-b9b3-94b614b04e5d/valkyrie_first_person_v2.png)
````

---

### 3. Spikeshot Shotgun (`08 / 32` Ammo)

**Technical Notes:**
- **HUD Placement:** Heavy ruggedized cast-iron housing on receiver top rail with three lateral shock-absorbing ribs designed to withstand 12-gauge magnum kick.
- **Texture Remaster Highlights:** 4K cast-iron heavy breech block with micro-pitted industrial surface roughness, deep carbon heat discoloration gradient around muzzle, tactile ribbed polymer pump slide with sharp serration edges, and vibrant E99 phosphor energy booster conduits.
- **VR Readability:** Deep recessed well (-2.5mm) with thick protective cathode glass protecting phosphor from recoil; high contrast `08 / 32` readout visible even during frantic close-quarters hip firing.

![Spikeshot Texture Macro Comparison](/C:/Users/Trilex/.gemini/antigravity/brain/37e373c5-dc1a-4785-b9b3-94b614b04e5d/spikeshot_texture_comparison.png)

![Spikeshot V2 3D Comparison](/C:/Users/Trilex/.gemini/antigravity/brain/37e373c5-dc1a-4785-b9b3-94b614b04e5d/spikeshot_comparison_v2.png)

````carousel
![Spikeshot V2 Overview](/C:/Users/Trilex/.gemini/antigravity/brain/37e373c5-dc1a-4785-b9b3-94b614b04e5d/spikeshot_overview_v2.png)
<!-- slide -->
![Spikeshot V2 Macro Close-Up](/C:/Users/Trilex/.gemini/antigravity/brain/37e373c5-dc1a-4785-b9b3-94b614b04e5d/spikeshot_closeup_v2.png)
<!-- slide -->
![Spikeshot V2 First-Person VR Sightline](/C:/Users/Trilex/.gemini/antigravity/brain/37e373c5-dc1a-4785-b9b3-94b614b04e5d/spikeshot_first_person_v2.png)
````

---

### 4. Kasimov SNV-E99 Sniper Rifle (`05 / 20` Ammo)

**Technical Notes:**
- **HUD Placement:** Cantilever scope bracket mount nestled flush beneath optical axis with anti-glare visor hood.
- **Texture Remaster Highlights:** 4K anti-glare matte sniper finish eliminating distracting VR specular reflections, knurled brass windage & elevation adjustment turrets on optical scope mount, fluted heavy match-grade barrel with crisp normal-mapped heat dissipation grooves, and machined bolt action handle with realistic contact wear.
- **VR Readability:** Nested right below the scope tube, perfectly visible in peripheral vision when looking down the sightline without blocking the scope eye-relief or optic lens.

![Kasimov Texture Macro Comparison](/C:/Users/Trilex/.gemini/antigravity/brain/37e373c5-dc1a-4785-b9b3-94b614b04e5d/kasimov_texture_comparison.png)

![Kasimov V2 3D Comparison](/C:/Users/Trilex/.gemini/antigravity/brain/37e373c5-dc1a-4785-b9b3-94b614b04e5d/kasimov_comparison_v2.png)

````carousel
![Kasimov V2 Overview](/C:/Users/Trilex/.gemini/antigravity/brain/37e373c5-dc1a-4785-b9b3-94b614b04e5d/kasimov_overview_v2.png)
<!-- slide -->
![Kasimov V2 Macro Close-Up](/C:/Users/Trilex/.gemini/antigravity/brain/37e373c5-dc1a-4785-b9b3-94b614b04e5d/kasimov_closeup_v2.png)
<!-- slide -->
![Kasimov V2 First-Person VR Sightline](/C:/Users/Trilex/.gemini/antigravity/brain/37e373c5-dc1a-4785-b9b3-94b614b04e5d/kasimov_first_person_v2.png)
````

---

### 5. Left Glove / TMD Bio-Monitor (Health Gauge)

**Technical Notes:**
- **HUD Placement:** Curved ergonomic dorsal cuff integrated flush into the TMD gauntlet wrist plate with embossed Soviet medical cross badge and corner bezel screws.
- **Texture Remaster Highlights:** 4K military-grade cordura canvas weave with individual warp and weft fibers, conditioned micro-porous leather palm reinforcement with realistic tactile friction grain, raised dual-nylon structural stitching with millimeter-accurate normal relief, and anodized Soviet milled graphite TMD gauntlet wrist casing.
- **VR Readability:** 10-segment vacuum-tube health gauge with distinct physical divider ribs and glass casing; immediately visible when raising the left arm naturally in VR. 3 distinct dynamic states: Nominal (Cyan 100%), Warning (Amber 50%), and Critical (Crimson 20%).

![TMD Glove Texture Macro Comparison](/C:/Users/Trilex/.gemini/antigravity/brain/37e373c5-dc1a-4785-b9b3-94b614b04e5d/tmd_glove_texture_comparison.png)

![TMD Glove V2 3D Comparison](/C:/Users/Trilex/.gemini/antigravity/brain/37e373c5-dc1a-4785-b9b3-94b614b04e5d/tmd_glove_comparison_v2.png)

````carousel
![TMD Glove V2 Overview](/C:/Users/Trilex/.gemini/antigravity/brain/37e373c5-dc1a-4785-b9b3-94b614b04e5d/tmd_glove_overview_v2.png)
<!-- slide -->
![TMD Glove V2 Macro Close-Up (100% Cyan)](/C:/Users/Trilex/.gemini/antigravity/brain/37e373c5-dc1a-4785-b9b3-94b614b04e5d/tmd_glove_closeup_v2.png)
<!-- slide -->
![TMD Glove V2 First-Person VR Sightline](/C:/Users/Trilex/.gemini/antigravity/brain/37e373c5-dc1a-4785-b9b3-94b614b04e5d/tmd_glove_first_person_v2.png)
<!-- slide -->
![TMD Glove V2 Warning State (50% Amber)](/C:/Users/Trilex/.gemini/antigravity/brain/37e373c5-dc1a-4785-b9b3-94b614b04e5d/tmd_glove_health_amber_v2.png)
<!-- slide -->
![TMD Glove V2 Critical State (20% Crimson)](/C:/Users/Trilex/.gemini/antigravity/brain/37e373c5-dc1a-4785-b9b3-94b614b04e5d/tmd_glove_health_crimson_v2.png)
````

---

## 4. Source Files & Direct GitHub Links

All source models, textures, tools, and renders are committed and available on GitHub:

- **3D Models V2 (`.obj`, `.mtl`, `.glb`):**
  - [Centurion Revolver V2](https://github.com/trilex07/SingularityVR/tree/side/diegetic-hud-native/assets/processed/hud/diegetic/models_v2/centurion_diegetic_v2.glb)
  - [Valkyrie AR9 Rifle V2](https://github.com/trilex07/SingularityVR/tree/side/diegetic-hud-native/assets/processed/hud/diegetic/models_v2/valkyrie_diegetic_v2.glb)
  - [Spikeshot Shotgun V2](https://github.com/trilex07/SingularityVR/tree/side/diegetic-hud-native/assets/processed/hud/diegetic/models_v2/spikeshot_diegetic_v2.glb)
  - [Kasimov Sniper V2](https://github.com/trilex07/SingularityVR/tree/side/diegetic-hud-native/assets/processed/hud/diegetic/models_v2/kasimov_diegetic_v2.glb)
  - [Left Glove / TMD Gauntlet V2](https://github.com/trilex07/SingularityVR/tree/side/diegetic-hud-native/assets/processed/hud/diegetic/models_v2/tmd_glove_diegetic_v2.glb)

- **Remastered 4K PBR Textures (PNG & Direct3D9 DDS):**
  - Directory: [`assets/processed/textures_remastered/`](https://github.com/trilex07/SingularityVR/tree/side/diegetic-hud-native/assets/processed/textures_remastered/)
  - [Manifest JSON](https://github.com/trilex07/SingularityVR/blob/side/diegetic-hud-native/assets/processed/textures_remastered/texture_remaster_manifest.json)

- **Automated Remaster & 3D Render Pipelines:**
  - [`tools/hud/remaster_weapon_glove_textures.py`](https://github.com/trilex07/SingularityVR/blob/side/diegetic-hud-native/tools/hud/remaster_weapon_glove_textures.py)
  - [`tools/hud/prepare_diegetic_models_v2.py`](https://github.com/trilex07/SingularityVR/blob/side/diegetic-hud-native/tools/hud/prepare_diegetic_models_v2.py)
  - [`tools/hud/render_diegetic_previews_v2.py`](https://github.com/trilex07/SingularityVR/blob/side/diegetic-hud-native/tools/hud/render_diegetic_previews_v2.py)

---

## 5. Restrictions Enforced
- **Zero runtime code or DLL modifications:** No changes to `d3d9.cpp` or runtime injectors.
- **Zero Steam installation pollution:** Game directories and vanilla packages untouched.
- **Strictly visual asset work:** Waiting for explicit user approval before proceeding to runtime integration.
