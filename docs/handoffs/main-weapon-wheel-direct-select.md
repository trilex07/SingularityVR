# MAIN — bezpośredni wybór broni w kole (zamiast N× CycleWeapon)

## Stan obecny (recovery)

- `weapon_wheel.h` — FSM: R3 down → open, stick → `selection`, R3 up → **Commit** jeśli `selection > 0`.
- Commit (`d3d9.cpp` ~9988): `g_weaponWheelCommit = selection` → tyle impulsów **`g_stickUpMask` (XI_Y)** = CycleWeapon.
- **Sektor 0** = „obecna broń”; release na 0 = **Cancel**.
- Overlay: `DrawWeaponWheel` — prostokąty `W1`–`Wn`, kotwica `g_handLastPos[1]`.

To **nie** mapuje sektora na konkretną broń — tylko „przewiń listę N razy”.

## Cel

1. Sektor koła = **konkretna broń** (`gunVerts` latch + nazwa `CommonText`).
2. Wektor wyboru: **prawa ręka / aim pose** w płaszczyźnie pierścienia (Alyx), z fallbackiem na **prawy drążek**.
3. Wizual: `ring_weapon_wheel.glb` + atlas Katorga, nie `W1`.

## Katalog broni (dane SIDE)

Plik: `assets/hud/katorga/3d/weapon_wheel_catalog.json`

- `sectors[]`: `index`, `gun_verts`, `class_hint`, `common_text_key`, `label_pl`, `atlas_icon`.
- `cycle_order[]`: **propozycja** kolejności CycleWeapon — **MAIN musi zweryfikować** w grze i nadpisać JSON po logu.

Identyfikator broni w modzie: **`g_gunVerts`** (vertex count latch), zgodnie z `GunAlignSlot` (5799 pistol, 8297 shotgun, 11387 AR, 11389 sniper).

## Algorytm commit (propozycja)

Po `Event::Commit` z FSM (R3 release, `selection > 0`):

```text
target = catalog.sectors[selection].gun_verts
current = Rd(g_gunVerts)
if target == 0 || target == current → done
steps = ShortestCycleSteps(current, target, cycle_order)
if steps < 0 → log "wheel: DIRECT unavailable", fallback legacy N pulses
else → g_weaponWheelCommit = steps  ; legacy pulse loop unchanged
```

**Faza 2 (lepsza):** pętla z limitem `max_steps = cycle_order.length`:

- Emit `stickUp` 8 klatek, gap 3, po każdym pulse odczytaj `g_gunVerts` / log `Pawn.Weapon`.
- Stop gdy `g_gunVerts == target` lub timeout → log failure.

**Faza 3 (RE):** natywny `SwitchWeapon` / inventory index — tylko po znalezieniu w exe; do tego czasu Faza 2.

### `ShortestCycleSteps`

Na liście cyklicznej `cycle_order` (tylko `gun_verts` > 0):

```text
i = index(current), j = index(target)
if i<0 or j<0: return -1
return (j - i + n) % n   ; n = len(cycle_order), 0 = stay on current
```

Jeśli `selection` mapuje **bezpośrednio** na wpis katalogu (nie indeks w cycle_order), użyj `catalog.sectors[selection].gun_verts` jako `target`.

## Wektor sektora (PickSource)

Wspólne API:

```cpp
int SectorForWheel(float x, float y, int slots, float deadzone);
```

| `WeaponWheelPickSource` | x, y |
|-------------------------|------|
| 0 | `tx, ty` — prawy drążek (obecne) |
| 1 | Aim: wektor `(aimForward.x, aimForward.y)` w lokalnej płaszczyźnie koła (normal ≈ `-handUp` lub `-viewUp`) |
| 2 | Jeśli `\|stick\| > deadzone` → stick; else aim |

Kąt zero = **góra ekranu / -handUp** — jak `SectorForStick` w `weapon_wheel.h` (`atan2(x, -y)`).

## Integracja z `weaponwheel::Update`

Minimalna zmiana:

- Nie ruszać edge FSM.
- Po `SelectionChanged`: mapuj `selection` → etykieta z katalogu do logu / UI.
- W `Commit`: wywołaj `ScheduleDirectWeaponSelect(targetGunVerts)` zamiast `g_weaponWheelCommit = selection` (gdy `WeaponWheelDirectSelect=1`).

## Rysowanie koła

Zastąp ciało `DrawWeaponWheel`:

1. Wczytaj GLB `ring_weapon_wheel.glb` (lazy, jednorazowo) — lub proceduralny torus w D3D9.
2. Transform: `T = T_controller * T_offset(attach_schema.nodes.weapon_wheel)`.
3. Billboard / face camera: obróć pierścień tak, by normal była w stronę `-viewForward`.
4. Highlight segmentu `selection`: shader constant / drugi pass z atlas UV z `weapon_wheel_catalog.sectors[i].atlas_icon`.
5. Etykieta `label_pl` — bitmap font istniejący (glyph path) lub atlas text (później).

Paczka SIDE: `hud/ring_weapon_wheel.glb`, promień ~18 cm w JSON.

## Warunki (bez zmian)

`gameplay && WeaponInHands() && !holstered` — jak dziś. TMD / menu / pauza = brak koła.

## Logi obowiązkowe (L4 gate)

```text
weapon wheel: OPEN slots=8 pick=1
weapon wheel: SECTOR sel=2 label=Centurion verts=5799
weapon wheel: DIRECT current=11387 target=5799 steps=3
weapon wheel: DIRECT OK after 3 pulses -> class RvWeaponShared ARPistol
weapon wheel: DIRECT FAIL timeout
```

## Plik nagłówka do skopiowania

`docs/handoffs/integration/weapon_wheel_direct_select.h` — struktury + szkice funkcji (bez linkowania w SIDE build).

## Test jednostkowy (MAIN doda)

`tests/test-weapon-wheel-direct.cpp`:

- `ShortestCycleSteps` na mock `cycle_order`.
- Mapowanie sektor → `gun_verts` z JSON fixture.
