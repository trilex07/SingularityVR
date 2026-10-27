# SIDE — paczka 3D HUD na rękawicach (attach + glTF + preview)

**Dla agenta SIDE.** Skopiuj cały ten plik jako brief albo pracuj bezpośrednio z repozytorium.

## Rola

Agent **SIDE (assety 3D / grafika)** dla **Singularity VR** (UE3, D3D9).

**Nie MAIN.** Zakaz dotykania: `SingularityVR-recovery`, `d3d9.cpp`, Steam, INI użytkownika, dubbing, cienia, kina, TV mirror, fontów notatek, merge do `main` bez zgody.

**Cel:** paczka **modeli + punkty montażu + rendery Blender** (nie stary skrypt Python), żeby MAIN mógł podpiąć HUD w runtime.

Wygląd: **wariant B** (`docs/hud/HUD_DESIGN.md`) + kółko zmiany broni jak **Half-Life: Alyx** (pełne, półprzezroczyste, wokół dłoni). **Tylko rękawice** — bez przedramion (`GloveIsolate`, gloves-only).

## Gałąź i worktree

- Worktree: `SingularityVR-hud-design`
- Gałąź bazowa: `fix/psvr2-hud-design` → preferowana nowa: **`side/hud-3d-attach-pack`**
- Commituj sensowne kroki; **bez** deploy Steam.

## Co już jest (użyj)

| Obszar | Ścieżka |
|--------|---------|
| Design | `docs/hud/HUD_DESIGN.md`, `HUD_STAGE1.md`, `KATORGA_ASSETS.md` |
| Handoff MAIN (2D) | `docs/handoffs/main-katorga-hud-assets.md`, `main-psvr2-hud.md` |
| SVG / atlas | `assets/hud/katorga/plates/`, `atlas/`, `states/` |
| PSK źródło rękawic | `research/native-hands-20261007/.../ch_inview_noTMD.psk` |
| PSK pistolet in-view | `.../RvGameMPLoader_SF/.../ch_inview_pistol_mesh.psk` |
| Tekstury | `SingularityVR-assets-local/textures/original-png/.../inview_glove_d.png`, `textures/upscaled/handgun_d__A_lanczos.png` |
| OBJ wycinek (bind space) | `assets/hud/katorga/real/glove_*.obj`, `pistol.obj`, `README.md` |
| Debug render (NIE standard jakości) | `tools/hud/render_real_glove_hud.py`, `assets/hud/katorga/real/0*.jpg` |

**Referencje wizualne (docelowy look):**

- `C:\Users\Trilex\.cursor\projects\c-Users-Trilex-vr-mods\assets\katorga-hud-walka.jpg`
- `katorga-target-walka.jpg`, `katorga-target-spokoj.jpg`, `katorga-target-zmiana.jpg`

**Nie używaj** `katorga-real-*.jpg` jako akceptacji wizualnej.

## Runtime (tylko czytaj)

- Kość broni in-view: **`cog_pistol`** (`spikes/view_matrix/singularity_bind_reference.h` w recovery).
- Sockety noTMD (props): `Journal` → `l_arm_wrist`, `GunSocket` → `cog_gun`, `TMD_Muzzle` → `l_arm_wrist` — wskazówki, **nie** parentuj płyt HUD do TMD/siatki broni UE3 bez decyzji MAIN.
- Etap 1 MAIN (`HUD_STAGE1.md`, `HudStereoPair`, `HUD SAFE`) **osobny** — ta paczka to etap 2+ (quady na kontrolerze).

---

## Deliverables (obowiązkowe)

### 1. Katalog `assets/hud/katorga/3d/`

```
assets/hud/katorga/3d/
  gloves/glove_left.glb
  gloves/glove_right.glb
  weapons/centurion_inview.glb      # ch_inview_pistol_mesh, tris broni
  hud/plate_health.glb
  hud/plate_ammo.glb
  hud/ring_weapon_wheel.glb         # pełny pierścień 360°, UV pod segmenty
  attachments/attach_schema.json      # wypełniony, nie tylko template
  previews/spokoj.png
  previews/walka.png
  previews/zmiana_broni.png         # ≥1920×1080, Blender
  ATTACHMENTS.md
  CHANGELOG.md
```

Opcjonalnie: `.blend` źródłowy w `assets/hud/katorga/3d/source/`.

### 2. Wymagania modeli

- **Rękawice:** mesh = noTMD mat0, **bez ramion** (logika jak `glove_isolate.h` / `glove_cut.py`).
- **Broń:** lufa w osi celowania (jak `katorga-hud-walka`), pivot na chwycie; empty **`muzzle_forward`**.
- **Płyty:** cienkie quady, UV pod atlas Katorga, **puste pola** na cyfry (MAIN wypełnia).
- **Kółko:** pełna obwódka w local space, promień ~15–20 cm od środka dłoni, 8–12 segmentów, wyróżniony slot **CENTURION**; przezroczystość materiału, nie obcięty mesh.

### 3. `attach_schema.json`

Schema: `singularityvr-hud-attach-v1`. Pola: `aim_hand`, `coordinate` (UE3 cm), `nodes` (`grip`, `plate_health`, `plate_ammo`, `weapon_wheel`) z `offset_pos`, `offset_rot_quat`, `radius_cm` dla koła.

Offsety mierzone od **PSK bind** (`r_arm_wrist`, `cog_pistol`), **nie** od pozy preview w `render_real_glove_hud.py`.

Szablon startowy: `assets/hud/katorga/3d/attachments/attach_schema.template.json`.

### 4. `ATTACHMENTS.md`

- Skąd każdy offset (bind vs korekta artystyczna PSVR2 / siedzenie na podłodze).
- Jak MAIN ma mapować `controller_grip` (OpenXR) vs kość `cog_pistol`.
- Jawne **antycele:** nie parentować płyt do slide/mesh broni bez osobnej spec.

### 5. Handoff MAIN

Po zakończeniu uzupełnij `docs/handoffs/main-hud-3d-attach-pack.md` (lista plików, kolejność integracji, linki do preview).

### 6. Test offline

`tools/hud/tests/test_hud_3d_pack.py` — PASS gdy paczka kompletna (rozszerz testy po dodaniu glTF).

---

## Kryteria akceptacji SIDE

- [ ] Wszystkie `.glb` ładują się w Blenderze (normals OK).
- [ ] Preview: broń widoczna, celuje do przodu; kółko **nieucięte** w kadrze FP.
- [ ] `attach_schema.json` + `ATTACHMENTS.md` bez niewyjaśnionych liczb.
- [ ] Zero zmian w recovery / `d3d9.cpp`.
- [ ] Preview bliskie `katorga-target-*` / `katorga-hud-walka` (**glove-only**).

## Antycele

- Brak implementacji renderera w grze.
- Brak czytania `mCounts` / pól amunicji silnika.
- Brak drugiej legendy PSVR2 na płytach (tokeny tylko w atlasie 2D).
- Brak merge PR19 / recovery bez autoryzacji.

## Workflow sugerowany

1. Import PSK → Blender (wycinek ramion jak `build/glove_cut.py`).
2. Export glTF + empties attach.
3. Modele płyt / pierścienia vs SVG w `assets/hud/katorga/plates/`.
4. Render Cycles/Eevee → `previews/`.
5. Push gałęzi SIDE; MAIN integruje osobno.

## Powiązane

- `assets/hud/katorga/real/README.md` — co jest dowodem mesh vs co jest mockupem.
- Test 2D pack: `tools/hud/tests/test_hud_design.py`.
