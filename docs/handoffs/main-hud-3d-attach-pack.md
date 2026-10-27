# Handoff MAIN — paczka 3D HUD Katorga (skrót)

**Pełny brief integracji:** [`main-hud-3d-integration.md`](main-hud-3d-integration.md)  
**Logika koła broni:** [`main-weapon-wheel-direct-select.md`](main-weapon-wheel-direct-select.md)  
**Kabura + apteczka (biodro):** [`main-holster-medkit-hip-slots.md`](main-holster-medkit-hip-slots.md)

**Status:** SIDE na gałęzi `side/hud-3d-attach-pack`.

## Instalacja assetów

```powershell
tools/hud/install-katorga-3d-assets.ps1 -GameRoot "<Binaries>"
```

## Pliki

`assets/hud/katorga/3d/` — GLB, tekstury, `attachments/attach_schema.json`, **`weapon_wheel_catalog.json`**, preview PNG.

Atlas 2D: `assets/hud/katorga/atlas/` (kopiowany razem ze skryptem).

## Kod propozycji (skopiuj do recovery)

- `docs/handoffs/integration/weapon_wheel_direct_select.h`
- Hooki: `d3d9.cpp` — `DrawWeaponWheel`, ~9793, ~9988 (patrz main-hud-3d-integration.md)

## Testy

```text
python tools/hud/tests/test_hud_3d_pack.py
python tools/hud/tests/test_weapon_wheel_catalog.py
python tools/hud/tests/test_holster_slots.py
```

## MAIN — kolejność

1. Etap 1 HUD (`HUD_STAGE1.md`) jeśli potrzebny.
2. Wczytaj JSON + `WeaponWheelDirectSelect=1`.
3. Podmień rysowanie koła + direct select commit.
4. Opcjonalnie płyty Katorga (`KatorgaHudEnabled=1`).
5. L4 PSVR2 + logi `weapon wheel: DIRECT`.
