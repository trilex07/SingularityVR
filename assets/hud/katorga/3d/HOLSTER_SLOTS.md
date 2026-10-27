# Sloty biodrowe — broń (prawo) i apteczka (lewo)

Ramka jak w runtime holsterze broni: **`body_yaw`**, punkt biodra względem HMD / origin śledzenia, bez pitchu głowy (patrz `HolsterHipWorld` w recovery).

## Układ (widok z góry)

```text
              +X (forward)
                 ↑
    apteczka     |     kabura broni
    (lewe biodro)|     (prawe biodro)
  ← -Y           |           +Y →
  medkit         |      weapon holster
```

Wysokość: **`HolsterUp` / `MedkitHolsterUp`** (domyślnie **-30 UU** — poziom biodra pod oczami).

## Pliki

| Plik | Opis |
|------|------|
| `holster_slots.json` | Offsety UU, promienie, powiązanie INI / hooków |
| `hud/holster_weapon_right.glb` | Kieszeń kaburowa (poziomo na prawym boku) |
| `hud/holster_medkit_left.glb` | Sakiewka apteczki + UV pod `sense-circle` |
| `attachments/attach_schema.json` | Nodes `holster_weapon`, `holster_medkit` |

## Broń (prawo)

- **Logika:** już w modzie — `weaponholster::Update`, `HolsterEnabled`, grip prawy przy biodrze.
- **Wizual:** MAIN rysuje `holster_weapon_right.glb` w **`HolsterHipWorld`** (ten sam punkt co pivot odłożonej broni).
- Opcjonalnie: ghost broni w slocie gdy `g_weaponHolstered`.

## Apteczka (lewo)

- **Pozycja:** lustrzane **`right`** (domyślnie **-12 UU**), ten sam **`fwd`** i **`up`** co kabura.
- **Interakcja:** propozycja w `holster_slots.json` — **domyślnie wyłączone** (`MedkitHolsterEnabled=0`) do czasu implementacji MAIN.
- **Ikona:** atlas `sense-circle` (token `[health]` w promptach — osobna ścieżka od fizycznego slotu).

## Orientacja mesh

- Oba GLB: lokalne **+Y = na zewnątrz ciała** (prawy slot: +Y w body frame; lewy slot: mesh obrócony o 180° wokół Z, żeby +Y wskazywało w lewo).
- **+X** w mesh ≈ kierunek pasa (do przodu tułowia), **+Z** ≈ pion na biodrze.

Regeneracja: `python tools/hud/export_holster_hip_meshes.py`.

## MAIN

Brief: `docs/handoffs/main-holster-medkit-hip-slots.md`.
