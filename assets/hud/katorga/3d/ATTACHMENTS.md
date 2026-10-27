# Montaż HUD 3D — paczka SIDE

Jednostki: **UE3 cm**, Z up. Parent runtime: **OpenXR grip** + kości **`l_arm_wrist` / `r_arm_wrist`** z draw noTMD.

## Pliki

| Plik | Zawartość |
|------|-----------|
| `gloves/glove_left.glb` | `ch_inview_noTMD` mat0, lewa, bez ramion |
| `gloves/glove_right.glb` | j.w. prawa |
| `weapons/centurion_inview.glb` | tris broni z `ch_inview_pistol_mesh` (Slide, clip, …); pivot w chwycie; node **`muzzle_forward`** (czerwona sfera) |
| `hud/plate_health.glb` | quad 6.4×3.8 cm, UV 0–1 pod atlas Katorga |
| `hud/plate_ammo.glb` | j.w. |
| `hud/ring_weapon_wheel.glb` | pierścień r0≈20.2 r1≈24.8 cm, 72 segmenty, pełne 360° |
| `attachments/attach_schema.json` | offsety względem kości |
| `textures/` | kopie `inview_glove_d.png`, `handgun_d.png` |

Generator: `tools/hud/export_katorga_3d_pack.py`.

## Bind (PSK, absolutne cm)

W `attach_schema.json` → `bind_reference`:

- `l_arm_wrist`, `r_arm_wrist`, `cog_pistol` — pozycje z REFSKELT noTMD.

Offsety w `nodes.*.offset_pos` z **`source": "art_psvr2"`** to korekta czytelności (siedzenie na podłodze), nie pomiar z gry:

| Node | Parent | Offset (cm) | Uzasadnienie |
|------|--------|-------------|--------------|
| `plate_health` | `l_arm_wrist` | (0, 6.5, 4) | nad mankietem, twarz do kamery |
| `plate_ammo` | `r_arm_wrist` | (-5, 2, 5) | bok chwytu / slide, nie parent do mesh broni |
| `weapon_wheel` | `r_arm_wrist` | (0, 0, 0) | środek dłoni; promień **18 cm** w JSON |
| `holster_weapon` | `body_yaw_hip` | (8, 12, -30) UU | prawe biodro — kabura (sync z `Holster*`) |
| `holster_medkit` | `body_yaw_hip` | (8, -12, -30) UU | lewe biodro — apteczka |

Szczegóły slotów: `HOLSTER_SLOTS.md`, `holster_slots.json`.

## MAIN — mapowanie

1. **Rękawice:** istniejący pipeline noTMD + `GloveIsolate=2` — te GLB służą jako referencja skali/UV, nie zastępują draw silnika.
2. **Broń in-view:** align do **`cog_pistol`** (lub controller aim), nie transformu z `render_real_glove_hud.py`.
3. **Płyty / kółko:** osobne quady w **controller/view space**; `face_camera: true` w JSON = billboard w runtime.
4. **Anty:** nie parentować płyt do `Slide` / siatki broni UE3 bez osobnej decyzji.

## Preview

`previews/*.png` (1920×1080) — **zatwierdzone concept art** (`katorga-target-*`), dopasowane do briefu wizualnego. Nie są zrzutem GLB z Blendera; GLB weryfikuje geometrię gry.
