# Steam / dvr-xrsim: powtarzalny test i rollback

> Ownership: MAIN only. Side agents never run this runner against the Steam installation; they verify with L1/L2 only. Default L3 is a direct real-Steam test (deploy via `tools/deploy-singularityvr.ps1`, test, rollback); the xrsim variant below is explicitly optional, not a mandatory gate. `tools/singularityvr-test-bridge.ps1` and `vr_capture_oracle.py` do not exist in this tree — marked ⛔ OBSOLETE where named below, never required. `deployment/stable/...` and `deployment/candidate` paths are historical examples, not required directories.

## Dlaczego wcześniejsze uruchomienia były mylące

`XR_RUNTIME_JSON` jest dziedziczone przez proces, który uruchamia grę. Jeżeli
klient Steam już działa, samo ustawienie zmiennej w bieżącym PowerShellu nie
zmienia środowiska Steam. `applaunch` uruchamiał wtedy grę przez
SteamVR/OpenXR, a replay pisał do niepowiązanego katalogu xrsim. Sam wpis
`OpenXR ready` nie jest dowodem użycia `dvr-xrsim`.

`tools/steam-xrsim-singularity.ps1` zawiera teraz obowiązkową kontrolę:

- przy manifeście runtime restartuje Steam, aby odziedziczył
  `XR_RUNTIME_JSON` i `DVR_XRSIM_DIR`;
- czeka na nowy PID `Singularity.exe`, świeży `state.json` z tym samym PID-em,
  stan `FOCUSED`/`VISIBLE`, dwie projekcje i zero błędów;
- odrzuca stary log/state zamiast traktować go jako gotowy test;
- po zakończeniu przywraca DLL, INI i opcjonalny VFX DLL z backupu.

Manifest runtime jest normalizowany do ścieżki absolutnej przed uruchomieniem
Steama. To ważne, bo Steam startuje z własnego katalogu roboczego; względne
`XR_RUNTIME_JSON` może wtedy wskazywać nieistniejący plik i dać płaski fallback
bez OpenXR. Jeżeli runner zgłasza świeży PID bez nowego `state.json`, traktuj to
jako BLOCKED/FAIL, nie jako częściowo udany test.

## Stabilne punkty regresji

Stan przed poprawką artefaktu:

`deployment/stable/20261010-b69d-menu-and-glove-baseline/`

- DLL SHA-256: `B69D6804F364FC9F32555C3A2114A353DB044CC46650557385E18571530FBFAF`
- INI SHA-256: `771641697A66843CEF3F61058E4CBE3FC48D181FD7EDDB77E4C380F4E4AFD699`
- zawiera referencyjny capture menu oraz capture sceny z bronią.

Stan po poprawce filtra odłączonego fragmentu:

`deployment/stable/20261010-c117-finger-artifact-fix/`

- DLL SHA-256: `C117A75F70A4822616E44AF0B8DEA6F33E29DBFC53F196EB2C4B55692FC195DA`
- PE32 x86, `/O2`; pełne dowody replayu są w `proof/`.
- kluczowy dowód runtime w `proof/view_matrix.log`:
  `GloveIsolation: suppressed detached sleeve/finger section verts=390 first=14490 tris=674`.

Katalogi są lokalnymi artefaktami regresji. Przed integracją należy zachować
ich pliki dowodowe albo przenieść je do zatwierdzonego magazynu artefaktów;
nie należy kopiować DLL do Steama przez skrypt automatycznie.

## Standardowy L3 dla głównego agenta (domyślnie bezpośredni test Steam; xrsim opcjonalny)

Poboczny agent wykonuje tylko L1/L2. L3 wykonuje główny agent na głównej
instalacji Steam, domyślnie jako bezpośredni test rzeczywistego Steama:
zbuduj, zweryfikuj SHA/PE32, wykonaj backup i wdróż przez
`tools/deploy-singularityvr.ps1`, przetestuj, a w razie potrzeby wycofaj
przez `-Action Rollback`. Wariant xrsim poniżej jest jawnie opcjonalny
i nie jest warunkiem zaliczenia L3:

```powershell
& .\tools\steam-xrsim-singularity.ps1 `
  -CandidateDirectory .\deployment\candidate `
  -RuntimeManifest .\deployment\candidate\dvr_xrsim32.json `
  -LaunchViaSteam -Windowed -WaitSeconds 60 `
  -PostReadyHoldSeconds 180 `
  -ReportDirectory .\deployment\candidate\reports
```

W czasie `PostReadyHoldSeconds` główny agent uruchamia istniejący replay:

```powershell
python tools\xrsim_replay_vr_session.py `
  "$env:LOCALAPPDATA\SingularityVR\vr_sessions\session_2026-10-09_14-18-07.vrrec" `
  --dir .\deployment\candidate --stride 4 --speed 8 `
  --report .\deployment\candidate\reports\replay.json
```

Uruchamiaj tylko jeden replay dla danego PID. Dwa procesy replay równocześnie
dzielą ten sam kanał komend i mogą celowo wywołać odrzucenie sekwencji; jest to
test integralności mostka, ale nie jest ważnym wynikiem scenariusza gry.

Po replayu zbierz dowody logami/capturami i manifestem; `PASS` wymaga świeżego
PID/state/capture i zgodnego SHA; sam licznik drawów albo pojedynczy
screenshot nie dowodzi poprawnej stereoskopii.

> ⛔ OBSOLETE: `tools/singularityvr-test-bridge.ps1 -Action Collect` nie
> istnieje w tym drzewie — nie używać. ⛔ OBSOLETE: `vr_capture_oracle.py`
> nie jest w repo — nie wymagać; porównanie L/R wykonaj istniejącymi
> capturami z tego samego frame identity.

## Procedura przy regresji rękawic lub artefaktu palca

1. Zatrzymaj grę i zapisz PID, SHA DLL, SHA INI oraz świeży `view_matrix.log`.
2. Sprawdź, że konfiguracja ma `ShowControllerArms=0`,
   `IndependentHandsPrototype=1`, `GloveIsolate=2` i `HideExtraArms=1`.
3. Powtórz scenariusz z bronią w rękach; menu nie uruchamia ścieżki
   `PrepareIndependentLeft`.
4. W logu oczekuj zarówno `GloveCensus ... mode=2`, jak i
   `GloveIsolation: suppressed ... verts=390 first=14490 tris=674`.
5. Wykonaj capture lewego i prawego oka z tego samego frame identity i porównaj
   ROI broni/rękawic z baseline. Jeżeli baseline jest nieznany, wynik to
   `INCONCLUSIVE`, nie `PASS`.
6. Jeżeli dowód nie przejdzie, przywróć snapshot B69D przez świadomą akcję
   głównego agenta i dopiero wtedy rozpocznij kolejną iterację.

## Bramki dowodowe

- **L1:** test `tools/reference-tests/hands/glove_isolate_test.exe` oraz testy
  matematyczne/unit.
- **L2:** build x86 PE32 `/O2`, SHA artefaktu i zgodność z manifestem.
- **L3 (domyślnie):** bezpośredni test głównego agenta na zainstalowanym
  Steam (deploy przez `tools/deploy-singularityvr.ps1`, świeży proces gry,
  log, rollback po teście). Wariant xrsim (`dvr-xrsim`, replay, capture
  L/R, log) jest jawnie opcjonalnym dowodem integracyjnym, nie warunkiem L3.
- **L4:** fizyczny PSVR2, gdy pozostaje kwestia odczucia kontrolera, komfortu
  albo zachowania sprzętowego.

Nie ogłaszaj `FIXED` bez właściwej bramki. Brak dostępnego symulatora nie
blokuje L3, gdy bezpośredni test Steam przeszedł; jako `BLOCKED` traktuj
jedynie brak wymaganego dowodu dla danej bramki.
