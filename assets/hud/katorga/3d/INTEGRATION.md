# Integracja techniczna — mesh / tekstury

## GLB

| Plik | Tris (approx) | Tekstura | Parent runtime |
|------|---------------|----------|----------------|
| `gloves/glove_left.glb` | ~1607 | `textures/inview_glove_d.png` | Istniejący draw noTMD |
| `gloves/glove_right.glb` | ~1625 | j.w. | j.w. |
| `weapons/centurion_inview.glb` | gun subset | `textures/handgun_d.png` | `cog_pistol` |
| `hud/plate_health.glb` | 12 | atlas Katorga | `l_arm_wrist` + offset JSON |
| `hud/plate_ammo.glb` | 12 | atlas | `r_arm_wrist` + offset |
| `hud/ring_weapon_wheel.glb` | 144 | atlas segmenty | `r_arm_wrist` / grip |
| `hud/holster_weapon_right.glb` | 12 | opcjonalnie atlas / stal | `body_yaw_hip` + offset UU |
| `hud/holster_medkit_left.glb` | 12 | `sense-circle` | `body_yaw_hip`, lewe biodro |

Loader: trimesh-compatible GLB 2.0; MAIN może użyć tinygltf / własnego minimalnego parsera / konwersji offline do custom `.mesh`.

## UV

- Płyty: pełny kwadrat 0–1 — MAIN mapuje regiony atlasu (`plates/*.svg` → `atlas.json`).
- Pierścień: UV kątowe w meshu (0–1 w obwodzie, 0–1 radialnie).

## Koło broni — segmenty

Liczba segmentów runtime: `min(WeaponWheelSlots, catalog.sectors.length)`.

Mapowanie: `weapon_wheel_catalog.json` → `sectors[i].atlas_icon` → rect w `../atlas/atlas.json`.

Ikony broni (sam wireframe, bez amunicji i bez pierścienia „obecna”):  
`../wheel/icons/*.png` (256 px), paczka MAIN: `textures/wheel_icons/<atlas_icon>.png`.  
Regeneracja: `python tools/hud/export_weapon_wheel_icons.py`.

## Ścieżka w grze

Domyślnie (INI `KatorgaHudAssetRoot`):

```text
<Binaries>/vr-assets/hud/katorga/3d/
```

Skrypt kopii: `tools/hud/install-katorga-3d-assets.ps1`.
