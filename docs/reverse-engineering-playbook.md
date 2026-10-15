# Playbook reverse-engineeringu SingularityVR (UE3 / D3D9 / x86)

**Status:** przewodnik warsztatowy Side. **Stan na:** gałąź `plates-l3-main`
commit `b850ac2` (rundy kabury 11–18, sondy amunicji 1–10).
Numery linii poniżej to punkty orientacyjne na ten commit — szukaj po
**nazwach symboli**, nie po liniach. Kod źródłowy żyje w
`spikes/view_matrix/d3d9.cpp` na gałęzi `plates-l3-main`; ten dokument
powstał na gałęzi `side/reverse-engineering-playbook` i celowo nie duplikuje
kodu, tylko metodę i odkrycia.

Powiązane (też na `plates-l3-main`): `docs/ammo-plates.md` (pełny zapis
rund 1–10 poszukiwań amunicji), `STATUS.md` (handoff sesji, rundy 11–18),
`docs/STEAM_XRSIM_RUNNER_PLAYBOOK.md` (integracja Steam — tylko MAIN).

---

## 1. Metoda dowodowa (obowiązuje zawsze)

```
OBSERVE → HYPOTHESIZE → INSTRUMENT → MEASURE → IMPLEMENT → TEST → REFINE
```

1. **Zaobserwuj** zachowanie gry, nie oczekiwanie. Preferuj istniejące hooki
   i logi zamiast nowych inwazyjnych sond.
2. **Udowodnij ścieżkę wykonania**: funkcja, shader, render target, macierz,
   obiekt UE3, kolejność wywołań. Licznik, nazwa obiektu, udana kompilacja
   albo poprawny wskaźnik to NIE jest dowód poprawnego zachowania.
3. **Hipoteza falsyfikowalna**: co jest zepsute, jaki dowód to potwierdza,
   jaka obserwacja by ją obaliła, jaki najmniejszy test rozstrzyga.
4. **Najmniejszy eksperyment**: sondy read-only, ograniczone logi,
   funkcje domyślnie OFF.
5. **Testuj i iteruj** na dowodach, nie na wrażeniach.
6. **Rozróżniaj poziomy**: offline PASS ≠ build PASS ≠ Steam runtime PASS ≠
   OpenXR PASS ≠ pixel PASS ≠ PSVR2 PASS. Nigdy nie wnioskuj wyżej niż dowód.
7. **Dwie nieudane implementacje z rzędu = stop spekulacji**, powrót do
   reverse-engineeringu (reguła eskalacji z `AGENTS.md`).

### Falsyfikatory, które zadziałały (przykłady)

- Runda 13: „STORED `hand==hip` w centrum udowodniłby rozjazd; kontakty
  krawędziowe dowodzą promienia, nie offsetu" — obaliło „systematyczny bias
  +4 UU" z rundy 12.
- Runda 15: „maska z `edge=1` na ręce przy pasie i brak `PICKED` obaliłaby
  hipotezę zbocza" (patrz §5).
- Runda 15: „rozrzut `hip-hand` prostopadły do celowania" falsyfikował
  hipotezę broń-vs-kontroler (patrz §6).

---

## 2. Warsztat UE3: jak szukać czegokolwiek (amunicja jako wzorzec)

Pełny zapis polowania: `docs/ammo-plates.md` (rundy 1–10). Przebieg w skrócie:

| Runda | Co zrobiono | Wynik |
|---|---|---|
| 1 | 8 pulli całego obiektu broni (`ARMachineGun`/`RvWeaponShared`), diff słów | **Brak** zwykłego int32 magazynka na broni; lotne słowa to flagi/timery |
| 2 | Dump property klasy broni przy equipie | Słownik nazw |
| 3 | Szersze okno, baseline pionka, dyscyplina tapów | Odrzucono fałszywe trafienia |
| 4 | Dump `WeaponFireTypes[0]` po nazwie | Trop firemode |
| 5 | Diffy pionka + dowód strzału przez `mShotCount` | Licznik strzałów znaleziony |
| 6 | Dump klasy HUD (display cache) | Założenie: HUD wie |
| 7 | Trop `InvManager`, relokacja dumpu HUD | Słowa `InvManager` statyczne |
| 8 | Słownik managera, wymuszony find HUD | HUD wymaga wymuszenia |
| 9 | Próbkowanie `mClipAmmo`/`mBinAmmo`/`mClipSize` | **TRAFIONE: żywy cache HUD** |

**Wynik końcowy:** `mClipAmmo`/`mBinAmmo`/`mClipSize` (+0x578/0x57C/0x580
na `RvHUD`): magazynek 4→3→2→1→[przeładowanie]→6→5…, zapas 16→10 przy
przeładowaniu −6. Lekcja: amunicji NIE ma na broni ani na pionku (zegar
+0x114 na pionku tylko liczy strzały) — jest w cache HUD.

### Narzędzia (wszystkie read-only, `spikes/view_matrix/d3d9.cpp`)

- **`Readable(p, n)`** (~l. 1391) — bramka przed każdym odczytem obcej
  pamięci. Nigdy nie czytaj adresu z UE3 bez niej (ryzyko crasha).
- **`FindPlayerPawn()`** (~l. 2176) — cached; **miss = pełny spacer po
  `GObjects`** (112 tys. obiektów, setki ms; na splashu NIE może się udać).
  Patroluj raz na ramkę, nigdy per-draw-call (lekcja run 252: splash
  27→8 fps przez spacery).
- **`WantedIdx(name)`** (~l. 1967) — nazwa → indeks FName (case-insensitive,
  **dokładne**, nie substring). Dlatego `mTMD` ≠ `mTMDArchetype` — to różne
  FName'y i czytnik TMD nie myli ich ze sobą (patrz §5).
- **`PropOffsetUnder` / `PropOffsetInChain`** (~l. 1975/2008) — offset
  property po łańcuchu klas bazowych. Zwraca −1 zamiast śmieci; offset poza
  `[0, 0x8000]` = zły layout, nie „egzotyczne property".
- **`DumpClassProperties`** (~l. 2038) — wypisz WSZYSTKIE property klasy
  zamiast zgadywać nazwy (tak znaleziono blok TMD na `RvPlayerPawn`, run 235).
- **Lekcja run 199** (komentarz przy `PropOffsetInChain`): limit 8 poziomów
  łańcucha ukrył `Pawn.Health` — boundy przechodzą tylko do końca łańcucha
  (`SuperField == null`), guard służy przeciw korupcji, nie jako limit.

### Znane adresy (run 235/236, `RvPlayerPawn`)

```
+0x1054  mTMDArchetype        (zawsze non-null — NIE mylić z posiadaniem TMD)
+0x1058  mTMD                 obiekt TMD — non-null po zdobyciu (nie = uniesione)
+0x105C  mPlayerHandsStatus   0=NONE, 1=TMD, 2=broń (enum HANDS_*, ~l. 2172)
+0x109C  mNoTMDArms           ramiona używane gdy brak TMD (rezerwa na weryfikację)
```

`TmdRaised()` / `WeaponInHands()` (~l. 2173–2174) czytają `g_handsStatus`.
Dekodowanie run 236c: każde `HANDS_TMD` poprzedza pojawienie się 5 sub-meshy
TMD w spisie — korelacja z meshem, nie z pamięcią o grze.

---

## 3. Case study: nóż biodrowy (`hip_knife.h`, rundy 9–18)

Logika w nagłówku (czysta, testowalna CPU): `hipknife::Frame` (wejście),
`State` (stan), `Context()` (bramka), `Update()` (maszyna stanów
`None/Picked/Sheathed/Reset/Swing`).

### 3a. Dwie niezależne bramki: input vs render

Najważniejsze odkrycie rund 14–18: **podniesienie (input) i rysowanie
(render) mają OSOBNE bramki na różnych wątkach.**

- Input: `hipknife::Context(f)` — `enabled && gameplay && focused && alive
  && (nativeKnife || hasTMD) && pawn && ...` (`hip_knife.h` ~l. 62).
- Render: `DrawHipKnife()` (~l. 25282) — dodatkowo `g_stereo`/`dupDraws`,
  `g_haveEyes`, pauza, menu, panel, kółko broni, cutscene, alokacja GPU.

Skutek: `PICKED` w logu NIE znaczy widocznego noża. Diagnostyka też jest
rozdzielona: sondy inputu (`grip held at belt…`, `armed grip at belt…`)
vs maska bramki rendera (`gate blocked (en/ready/vis/…)`).

### 3b. Root cause „modelu noża nie widzę" (runda 18)

Bramka rendera wymagała `g_stereo` — **debugowego trybu alternate-eye
przełączanego F12**, który `SetVrMode()` (~l. 23832) i F1 (~l. 26551)
celowo gaszą. W normalnym VR nóż **nigdy nie mógł się narysować**:
zero linii `hip knife: draw=` w całej historii logów. Fix jednej linii:
bramka na `Rd(g_dupDraws)` (prawdziwe stereo, ten sam sygnał co pierścień
pasa `DrawKnifeBeltMarker`, ~l. 26347). Maska logu też przeszła ze
`stereo=` na `dup=`.

### 3c. TMD: wykrywanie jest poprawne, spór rozstrzygnięty dowodem

- Offset `mTMD +0x1058` (depth 1) vs archetyp `+0x1054` — różne FName'y,
  pomyłka wykluczona (§2).
- Trzy zgodne sygnały: log resolwera, `hands: mTMD is NON-NULL`,
  obliczone `hasTMD` w sondach.
- Fabularnie urządzenie zrasta się z nadgarstkiem w prologu; Centurion to
  broń, nie TMD — brak sprzeczności.
- Mimo to istnieje furtka INI `HipKnifeIgnoreTMD=1` (runda 17): wymusza
  regułę sprzed TMD. Domyślną patrz w kodzie przy odczycie INI.

Reguły po rundzie 14 (prośba użytkownika): **przed TMD obie ręce; po TMD
tylko fizyczna prawa ręka (`h==1`) i tylko bez broni w ręce**
(`noWeapon = !WeaponInHands() || holstered`). Nóż trzymany lewą w chwili
zdobycia TMD resetuje się (`hip_knife.h` ~l. 67–69, 76–81).

### 3d. Zbocze, nie poziom (edge, nie level)

Podniesienie wymaga **narastającego zbocza gripa PRZY pasie**
(`edge[h] = grip && !priorGrip`). Trzymanie gripa z wyprzedzeniem i samo
wejście w promień NIC nie daje — to pułapka UX, nie błąd silnika.
`Update()` kopiuje grip do `priorGrip`, więc sonda rundy 15 łapie zbocze
PRZED wywołaniem (`preEdge`) — po `Update` odczyt zawsze dałby fałsz.

### 3e. Sondy (bounded, jedna linia na unikalną maskę)

- `hip knife: grip held at belt but no pickup (gameplay/focused/alive/
  nativeKnife/hasTMD/noWeapon/pawn/offTMD/offHP/tmd/hp/impIdx/impMask)` —
  `Context == false`: surowe wartości odróżniają post-TMD od nieznanych
  offsetów (`offTMD=-1`), śmierci (`hp<=0`) i przemapowanego impulsu.
- `hip knife: armed grip at belt but no pickup (near/grip/edge/free/trk/
  noWpn/postTMD/preTMD)` (runda 15) — `Context == true`, maski bitowe na rękę
  (bit0=lewa, bit1=prawa). Odszyfrowane w STATUS rund 15–17.
- `hip knife: gate blocked (en/ready/vis/vr/play/dup/eyes/paused/menu/
  panel/wheel/cine)` (runda 17, per-maska — stara jednorazówka wypalała się
  w menu i milczała w grze).
- `hip knife: build rN …` — znacznik buildu + konfiguracja; **dowód, na
  którym DLL gra sesja**. Bez niego linii nie da się przypisać do kodu.
- `hip knife: PICKED/SHEATHED/SWING/RESET held=%d` — zdarzenia (swing wysyła
  natywny atak; trafienie liczy gra od kamery — brak kolizji ostrza 6DoF).

Test CPU: `spikes/view_matrix/tests/test-hip-knife.cpp` (159 checków +
360000 ramek; tryb pre-TMD obu rąk + tryb post-TMD prawej/bez-broni/reset
przybycia TMD).

---

## 4. Case study: kabura (`weapon_holster.h`, rundy 11–18)

- **Trigger == pierścień == render odłożonej broni** (ten sam punkt
  `HolsterHipWorld(H, true)`). Utrzymuj tę równość — to niezmiennik.
- Log `weapon holster: STORED/DRAWN … hand=(…) hip=(…) radius=… aim=(…)`
  (aim od rundy 15).
- **Kontakty krawędziowe ≠ bias kalibracji.** 10× STORED ze średnią ręki
  ~5 UU nad biodrem przy promieniu 12 to wejścia z kierunku podejścia
  (tzw. „z góry z przodu"), nie dowód przesunięcia. Runda 12 odjęła −4 UU
  od Z na tej podstawie — zły kierunek (oddaliła trigger od nadchodzącej
  ręki); runda 13 cofnęła. Lekcja: średnia punktów na sferze o promieniu R
  nigdy nie jest estymatorem środka.
- Hipoteza broń-vs-kontroler (runda 15): użytkownik celuje WIDZIALNĄ bronią,
  trigger testuje punkt KONTROLERA. 3 próbki dały 47°/88°/29° między
  `(hip−hand)` a celowaniem — próbka prostopadła falsyfikuje stały bias
  w osi broni. Nierozstrzygnięte, wymaga większego n.
- **Trim INI** (runda 16, precedens: strojenie kotwicy broni run 217):
  `HolsterTrimFwdCm/RightCm/UpCm`, domyślnie 0 (= trigger w pierścieniu),
  rama ciała, ±30 cm, tylko trigger (pierścień i render nietknięte).
  Ustawione na życzenie użytkownika: `Fwd=-2` (w stronę kamery = do tyłu),
  `Right=-3` (w lewo), `Up=-2` (w dół); backup INI w
  `deployment/install-backups/`. Mapowanie znaków zapisane w STATUS —
  odwrócenie to jedna linijka.

---

## 5. Log: jak czytać `view_matrix.log`

Ścieżka: `%LOCALAPPDATA%\SingularityVR\view_matrix.log` (+ archiwa
`view_matrix_*.log` po rotacji). Czytanie loga to odczyt, nie dotykanie
Steam — dozwolone dla Side.

```powershell
Select-String -Path "$env:LOCALAPPDATA\SingularityVR\view_matrix.log" `
  -Pattern 'hip knife: build r' | Select-Object LineNumber,Line
Select-String -Path "$env:LOCALAPPDATA\SingularityVR\view_matrix.log" `
  -Pattern 'hip knife: (PICKED|SHEATHED|SWING|armed grip at belt|grip held at belt|gate blocked)' |
  Select-Object -Last 15 | Format-Table LineNumber,Line -Wrap -AutoSize
Select-String -Path "$env:LOCALAPPDATA\SingularityVR\view_matrix.log" `
  -Pattern 'weapon holster: (STORED|DRAWN)' | Select-Object -Last 10
Select-String -Path "$env:LOCALAPPDATA\SingularityVR\view_matrix*.log" `
  -Pattern 'hip knife: draw=' | Select-Object -First 3
```

Słownik (skróty): `build rN` = sesja na buildzie N; `draw=` = renderer
doszedł do rysowania (brak w całej historii = nigdy nie renderował);
maski bitowe sond: bit0 = ręka lewa, bit1 = prawa; `edge=1` = świeże
ściśnięcie; `mTMD is NON-NULL` = TMD zdobyte (obiekt, nie uniesienie);
`player pawn: 0x… acquired` = nowy pionek (zmiana poziomu).

Zasady sond: bounded (limity 128/notes, jedna linia na maskę), zero alokacji
per-draw, INI `default OFF`, nigdy stan gry. Jednorazowe logi (`static bool
said…`) wypalają się w menu — diagnostyka gameplayu wymaga masek
per-DISTINCT (lekcja rundy 17).

---

## 6. INI jako panel sterowania (bez rebuildu, restart gry)

Klucze `[Input]` (odczyt przy starcie, `d3d9.cpp` ~l. 30451+):
`HolsterFwd/Right/Up/Radius`, `HolsterTrimFwdCm/RightCm/UpCm`,
`HipKnifeEnabled`, `HipKnifeIgnoreTMD`, `HipKnifeLeftCm/DownCm/ForwardCm`.
MAIN edytuje **zainstalowane** INI wyłącznie z backupem pre-edit do
`deployment/install-backups/<stamp>/` i weryfikuje hash DLL przed/po.
Side nie dotyka instalacji — stroi tylko własny kod/defaulty.

---

## 7. Pułapki (każda kosztowała co najmniej rundę)

1. Krawędź sfery to nie środek (§4). 2. Jednorazowy log w menu (§3e).
3. `g_stereo` (F12 alternate-eye) ≠ stereo VR (§3b). 4. FName dokładny,
   nie substring (§2). 5. Archetyp `mTMDArchetype` ≠ instancja `mTMD` (§3c).
6. `mTMD non-null` (zdobyte) ≠ uniesione (`HANDS_TMD`) (§2). 7. Zbocze gripa,
   nie poziom (§3d). 8. Miss `FindPlayerPawn` = spacer GObjects (§2).
9. Cap łańcucha klas ukrywa property (run 199, §2). 10. `PICKED` (input) ≠
   widoczne (render) — wątki mają osobne bramki (§3a).

---

## 8. Bramki Side (przypomnienie z `AGENTS.md`)

- Własny worktree + własny branch (`git worktree add -b <branch> <path>
  origin/main`), commit własnych plików, push własnego brancha, PR bez
  merge. Zero edycji cudzych checkoutów/branchy, zero reset/force-push.
- L1: testy CPU/syntetyczne. L2: build x86 PE32 `/O2` (`OPENXR_SDK` przez
  `tools/Resolve-OpenXrSdk.ps1`, nigdy sztywna ścieżka), testy, manifest
  `d3d9.build.json` (commit czysty, SHA == zbudowane == zainstalowane —
  to ostatnie weryfikuje tylko MAIN).
- Komendy: `.\spikes\view_matrix\build.ps1`,
  `cl … test-hip-knife.cpp && t.exe`. Zero Steam: brak deployów, launchy,
  runów `steam-xrsim`, backupów/restore instalacji.
- Bez headsetu nie ma akceptacji wizualnej: maksymalnie
  `READY_FOR_PHYSICAL_ACCEPTANCE`, nigdy `PHYSICALLY_VERIFIED`.

## 9. Mapa plików

- `spikes/view_matrix/d3d9.cpp` — wszystko (input `SyncXRInput`, render
  `DrawHipKnife/DrawHolsterMarker/DrawKnifeBeltMarker`, resolver, INI).
- `spikes/view_matrix/hip_knife.h` — maszyna stanów noża (testowalna CPU).
- `spikes/view_matrix/weapon_holster.h` — `HipPoint`/`NearHip`/promienie.
- `spikes/view_matrix/tests/test-hip-knife.cpp` — L1 (159 checków).
- `spikes/view_matrix/build.ps1` + `d3d9.build.json` — L2/proweniencja.
- `tools/deploy-singularityvr.ps1 -DllOnly` — **tylko MAIN**.
- `docs/ammo-plates.md`, `STATUS.md` (oba na `plates-l3-main`) — zapis rund.
