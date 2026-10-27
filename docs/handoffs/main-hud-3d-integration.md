# MAIN — integracja HUD 3D Katorga + koło broni (jeden brief)

**Źródło SIDE:** gałąź `side/hud-3d-attach-pack`, worktree `SingularityVR-hud-design`.  
**Cel docelowy:** merge do `SingularityVR-recovery` (lub MAIN branch), **bez** dubbingu/cieni/kina.

## 1. Skopiuj assety do instalacji gry

Z repozytorium SIDE:

```text
assets/hud/katorga/3d/          →  <Steam>/Singularity/Binaries/vr-assets/hud/katorga/3d/
assets/hud/katorga/atlas/       →  …/vr-assets/hud/katorga/atlas/   (już jest w design pack 2D)
```

Skrypt (MAIN może uruchomić po ustawieniu `$GameRoot`):

```powershell
tools/hud/install-katorga-3d-assets.ps1 -GameRoot "D:\Steam\steamapps\common\Singularity\Binaries"
```

Loader w DLL **nie istnieje jeszcze** — poniżej punkty zaczepienia.

## 2. Pliki danych (runtime)

| Plik | Rola |
|------|------|
| `vr-assets/hud/katorga/3d/attachments/attach_schema.json` | Offsety płyt + promień koła |
| `vr-assets/hud/katorga/3d/weapon_wheel_catalog.json` | Sektor → `gunVerts` + nazwa PL |
| `vr-assets/hud/katorga/3d/holster_slots.json` | Kabura prawo + apteczka lewo (biodro) |
| `vr-assets/hud/katorga/3d/textures/*.png` | Opcjonalnie obok GLB |
| `vr-assets/hud/katorga/atlas/atlas.json` + PNG | Ikony segmentów koła |

Regeneracja GLB (dev): `python tools/hud/export_katorga_3d_pack.py`.

## 3. Hooki w `d3d9.cpp` (recovery — już są)

| Hook | Linia (orientacyjnie) | Co zrobić |
|------|------------------------|-----------|
| Input R3 | ~9793–9817 | Po `weaponwheel::Update`: **nowy commit** — patrz `main-weapon-wheel-direct-select.md` |
| Cycle pulses | ~9988–9999 | Zastąpić / uzupełnić gdy `WeaponWheelDirectSelect=1` |
| Draw overlay | `DrawWeaponWheel` ~24371 | Zamienić `ClearRects`+`W1` na mesh koła + atlas; parent z `attach_schema` |
| Present chain | ~5290 | Po HUD stereo: opcjonalnie `DrawKatorgaHudPlates()` |
| INI load | ~28153 | Dodać `WeaponWheelDirectSelect`, `KatorgaHudEnabled` |

Nagłówek propozycji logiki (skopiuj do recovery):  
`docs/handoffs/integration/weapon_wheel_direct_select.h`

## 4. Kolejność implementacji MAIN

1. **Etap 1** (`HUD_STAGE1.md`) jeśli nie done — nie blokuje merge assetów, ale L4 HUD czytelności tak.
2. Wczytaj JSON z `vr-assets/.../attach_schema.json` + `weapon_wheel_catalog.json` przy starcie (jak INI).
3. **Koło — logika:** sektor z ręki/drążka → `targetGunVerts` → commit (direct select).
4. **Koło — rys:** panele radialne (styl holo — `wheel/holo_wheel_style.json`, `wheel_weapon_draw.py`), cyan / pomarańcz na sektorze; hub „WYBIERZ BROŃ”. `ring_weapon_wheel.glb` = footprint promienia; wireframe z atlasu + `draw_wireframe` lub ten sam shader co preview PNG.
5. **Płyty:** `plate_health` / `plate_ammo` — billboard, dane z istniejącego `Health`/`HealthMax` + rysunek amunicji (etap 1) lub puste sloty.
6. **Kabura / apteczka:** mesh biodrowe — `holster_weapon_right.glb` (ten sam punkt co `HolsterHipWorld`), `holster_medkit_left.glb` (lewo, mirror). Patrz `main-holster-medkit-hip-slots.md`.
7. Logi: `weapon wheel: DIRECT target=5799 sector=2 steps=3` — wymagane przed L4.

## 5. INI proponowane (`SingularityVR.ini`)

```ini
[Input]
WeaponWheelSlots=8
WeaponWheelDirectSelect=1
WeaponWheelPickSource=1
; 0 = right stick (obecne), 1 = controller aim w płaszczyźnie koła, 2 = oba

[Render]
KatorgaHudEnabled=0
; 0 = tylko koło po DirectSelect; 1 = płyty + koło
KatorgaHudAssetRoot=vr-assets/hud/katorga/3d

; Apteczka na lewym biodrze (logika MAIN — domyślnie off)
MedkitHolsterEnabled=0
MedkitHolsterFwd=8
MedkitHolsterRight=-12
MedkitHolsterUp=-30
MedkitHolsterRadius=10
```

## 6. Testy przed L4

| Test | Oczekiwane |
|------|------------|
| `python tools/hud/tests/test_hud_3d_pack.py` | PASS |
| `spikes/view_matrix/tests/test-weapon-wheel.cpp` | PASS (stary FSM) |
| Nowy `test-weapon-wheel-direct.cpp` (MAIN) | sector→verts, cycle cap |
| Log w grze | OPEN → sector change → DIRECT commit → `Pawn.Weapon` class zmiana |
| PSVR2 | Broń w ręku, R3, ruch w kole, puść R3, **właściwa broń** (nie losowy cycle) |

## 7. Dokumenty powiązane

- `main-weapon-wheel-direct-select.md` — algorytm wyboru
- `main-holster-medkit-hip-slots.md` — kabura + apteczka biodro
- `assets/hud/katorga/3d/INTEGRATION.md` — mesh/UV
- `main-hud-3d-attach-pack.md` — lista plików SIDE
- `docs/hud/HUD_DESIGN.md`, `KATORGA_ASSETS.md`

## 8. Antycele

- Nie parentować płyt do `Slide` / mesh broni UE3.
- Nie psuć L3+R3 recenter, holsteru, `GloveIsolate`.
- Nie włączać `WeaponWheelDirectSelect` domyślnie bez jednego logu `DIRECT` w testach desktop automation.
