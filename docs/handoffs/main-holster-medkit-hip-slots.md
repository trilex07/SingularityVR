# MAIN — kabura (prawo) + apteczka (lewo), poziom biodra

**Assety SIDE:** `assets/hud/katorga/3d/holster_slots.json`, `hud/holster_*.glb`, nodes w `attach_schema.json`.

## 1. Broń — prawy bok (już jest logika)

Nie zmieniaj semantyki `weaponholster::Update` bez testu holster sequence.

| Element | Gdzie |
|---------|--------|
| Hip point | `HolsterHipWorld` → `weaponholster::HipPoint(g_holsterConfig, bodyYaw)` |
| INI | `HolsterEnabled`, `HolsterFwd`, `HolsterRight`, `HolsterUp`, `HolsterRadius` |
| Stan | `g_weaponHolstered` |

**Nowe (wizual):** gdy `KatorgaHudEnabled=1` (lub osobny `KatorgaHolsterVisual=1`):

1. Wczytaj offset z `holster_slots.json` → slot `weapon_holster_right` (musi być zgodny z INI holster — domyślnie fwd=8, right=12, up=-30).
2. Rysuj `holster_weapon_right.glb` w world space w tym samym punkcie co `HolsterHipWorld(..., includeHmdOffset=true)`.
3. Opcjonalnie: gdy broń odłożona, ten sam pivot co `GunPivotWorld` holster path.

## 2. Apteczka — lewy bok (nowe)

**Cel UX:** fizyczne miejsce na biodrze po lewej, poziomo (jak pas), spójne z kaburą po prawej.

Propozycja geometrii (mirror `HolsterRight`):

```ini
[Input]
MedkitHolsterEnabled=0
MedkitHolsterFwd=8
MedkitHolsterRight=-12
MedkitHolsterUp=-30
MedkitHolsterRadius=10
```

Helper (skopiuj obok `weapon_holster.h`):

```cpp
inline weaponholster::V3 MedkitHipPoint(const weaponholster::Config& weaponCfg,
                                          float bodyYawRad) {
    weaponholster::Config c = weaponCfg;
    c.right = -weaponCfg.right; // or read MedkitHolsterRight from INI
    return weaponholster::HipPoint(c, bodyYawRad);
}
```

**FSM (propozycja — implementacja MAIN):**

- Lewy grip **rising edge** w promieniu `MedkitHolsterRadius` od `MedkitHipPoint`.
- Fazy: pusty slot / niesiona apteczka ze świata / użycie (emit istniejącego `CB_HEALTH` tylko gdy gameplay + nie menu).
- **Nie** mapować tego na prawe kółko B (`[health]`) bez osobnego testu kolizji menu (patrz `KATORGA_ASSETS.md`).

**Wizual:** `holster_medkit_left.glb` + ikona `sense-circle` z atlasu na froncie sakiewki.

## 3. Loader JSON

Przy starcie (obok `weapon_wheel_catalog.json`):

- `vr-assets/hud/katorga/3d/holster_slots.json`
- Waliduj `schema == singularityvr-holster-slots-v1`
- Mapuj `attach_node` → transform z `attach_schema.json` nodes

## 4. Test L4

| Scenariusz | Oczekiwane |
|------------|------------|
| Prawe biodro + grip | Broń odkłada / wyciąga jak dziś; opcjonalnie widać kaburę Katorga |
| Lewe biodro + grip | Tylko po `MedkitHolsterEnabled=1`; log `medkit holster: STORED/DRAWN` |
| Koło R3 + holster | Koło nadal wymaga `!g_weaponHolstered` (bez zmian) |

## 5. Testy offline

```text
python tools/hud/tests/test_holster_slots.py
python tools/hud/tests/test_hud_3d_pack.py
```

## Antycele

- Nie przesuwać domyślnego `HolsterRight` bez aktualizacji JSON + logu.
- Nie włączać `MedkitHolsterEnabled` domyślnie w Steam do czasu L4 lewej ręki.
