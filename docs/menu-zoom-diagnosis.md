# Diagnoza: za duże menu przed grą + zoom pauzy w cutscence

Status: **diagnoza Side, bez zmiany kodu** (brak loga z reprodukcją —
poprawka bez dowodu byłaby spekulacją). Stan kodu na
`plates-l3-main @ b850ac2` (numery linii orientacyjne, szukać po symbolach).

## 1. Mechanizm wspólny: treść mono wysłana jako połówki stereo

Submit do gogli ma 4 rodzaje (`MenuSubmitKind`, `menu_submit_policy.h`):
`1 = crop` (ta sama połowa do obu oczu — poprawne dla mono),
`2 = half-rect` (każde oko dostaje swoją połowę SBS — poprawne TYLKO dla
prawdziwego stereo). Treść mono raz narysowana na całą szerokość, wysłana
rodzajem 2, wygląda w goglach na ~2× za dużą (zoom): lewe oko widzi lewą
połowę rozciągniętą na całe oko.

`cropEyes` (`d3d9.cpp` ~l. 12027) wymaga `frame->menuFlat`, a flaga
`menuFlat` (`SnapshotMenuOnSlot`, ~l. 6050) to:

```
class == 1  (IIS boot)
OR (CutsceneMenuPairFix=1 AND (
      class == 2 + focus            (frontend/Main)
   OR class == 3 + focus + cinematic + paused   (pauza w kinie) ))
```

Klasy (`native_ui_input.h` ~l. 111, 123): 0 brak, 1 IIS, 2 Main,
3 Pause, **4 inne natywne UI** (Options, Dialog, Slots/SlotsBase, LoadGame,
Review, Credits, Notepad, Stats, UseStation, BarisovStation, WeaponLocker).

## 2. Podejrzane ścieżki (do potwierdzenia logiem)

- **„Drugie menu przed grą"**: jeśli to film spoza tabeli → w logu pojawi się
  `VR UI focus UNCLASSIFIED` (nazwie winowajcę; `native_ui_input.h` ~l. 150:
  „prime suspect for the nested-views second menu — draws miss the menu path
  and fall through to StereoPair"). Jeśli to klasa 4 → dostaje zawsze
  rodzaj 2 (nigdy `menuFlat`), więc mono = zoom. Jeśli to klasa 2 z rodzajem
  1, a mimo to za duże → winny jest placement/skala (`menu_hud_scale.h`:
  `ClassBackbufferScale`, `MenuPlacementClipY`), nie submit.
- **Pauza w cutscence**: działa tylko pełna piątka
  fix + klasa 3 + focus + cinematic + paused → `SceneMode: … MenuFlat
  reason=32`. Wypadnięcie któregokolwiek (typowo `uiFocus`, albo fix OFF)
  daje rodzaj 2 na mono klatce kina = zoom. Histereza kontrolera
  (2 ramki wejścia / 4 wyjścia, `vr_scene_mode.h`) może rozjeżdżać przejścia.

## 3. Co mówią logi (stan dowodowy)

- Bieżący log: klasa 2 → `flat=1 crop=1 kind=1` (poprawnie, fix=1);
  pauza w grze (klasa 3, `cinematic=0`) → `kind=2` (**poprawnie BY DESIGN**,
  pauza w grze rysuje stereo); zero linii `UNCLASSIFIED`; zero sekwencji
  cinematic+paused. **Żaden z dwóch bugów nie występuje w dostępnych logach.**
- Archiwa 11–12.10: klasa 2 → `kind=2` (era sprzed fixa — wygląd zgodny
  z opisem, ale fix już wylądował).
- Wniosek: potrzebna celowana reprodukcja, nie zgadywanie.

## 4. Protokół reprodukcji (dla MAIN / użytkownika, jeden przebieg na bug)

1. Otwórz „drugie menu przed grą". 2. W grze, w trakcie cutscenki, daj pauzę.
Po każdej akcji spisz z `%LOCALAPPDATA%\SingularityVR\view_matrix.log`:

```powershell
Select-String -Path "$env:LOCALAPPDATA\SingularityVR\view_matrix.log" `
 -Pattern 'VR UI focus:|UNCLASSIFIED|menu-submit-edge|SceneMode:' |
 Select-Object -Last 12 | Format-Table Line -Wrap -AutoSize
```

Rozstrzygnięcia:
- `UNCLASSIFIED` z nazwą filmu → dopisać film do tabeli (`MenuClass()`).
- `class=4 … flat=0 crop=0 kind=2` → kandydat-fix: objąć klasę 4
  identical-crop (ta sama co klasa 2).
- `class=3 … kind=2` przy pauzie w kinie → sprawdzić `focus=` i linię
  `SceneMode:` (czy jest `reason=32` + `cinemaPause=1`); `focus=0` = luka
  focus-gatingu.
- `class=2/3 … kind=1`, a mimo to zoom → geometria crop/compose albo
  `menu_hud_scale.h`, nie submit.

Dopiero linia z reprodukcji autoryzuje kandydata-fix (L1/L2 Side, runtime MAIN).
