# Diagnoza dłoni i palety kości — 2026-10-10

## Objaw

W gameplayu praworęcznym pistolet/rig dłoni wygląda jak przypięty do złej ręki,
a po drugiej stronie widać nienaturalnie długie palce.

## Co zostało wykluczone

- `SingularityVR.ini` ma `LeftHanded=0`.
- Kod wybiera prawą dłoń jako `AimHand()` i lewą jako `OffHand()` przy tym ustawieniu.
- Log sesji potwierdza prawidłowe wejście: `handOff aim (-15.1, +23.0, -19.7)` oraz
  `off (+11.1, +22.9, -19.7)`.
- To nie jest przede wszystkim zamiana kontrolerów w SteamVR.

## Znaleziona przyczyna

`PrepareIndependentLeft()` używa tabel `kNoTmdGloveMap`, `kNoTmdSleeveMap`,
`kTmdGloveMap`, `kTmdBaseMap` i `kTmdExtraMap` jako mapy **slot palety GPU → numer
kości**. Tabele zawierają podobny zestaw numerów, ale w innej kolejności niż
rzeczywisty `bone_map` wyeksportowany dla sekcji.

Przykład bez TMD:

- kod w `d3d9.cpp`: pierwszy slot `kNoTmdGloveMap` to kość `9`;
- zweryfikowany `noTMD-section-palette.json`: pierwszy slot to `85`, a kość `9`
  występuje dopiero na slocie 18.

Przykład fragmentu 390:

- kod: `[7, 8, 27, 49, 50, 85]`;
- rzeczywista paleta: `[50, 85, 49, 27, 8, 7]`.

To oznacza, że transformacja kontrolera dla nadgarstka/palców jest zapisywana do
innych kości niż te, które czyta shader. Wynikiem są skrzyżowane ręce, broń
wyglądająca na przypiętą do lewej strony i „długie palce”.

## Dowód runtime

Sesja `deployment/agent-test-bridge/20261010-150008/view_matrix.log` stale
raportuje `GloveCensus ... left=0/0 right=2/2`, mimo że oba kontrolery są ważne.
To pasuje do błędnej kolejności/mapowania slotów, a nie do braku trackingu.

## Następny krok

Nie zmieniać jeszcze handedness ani pozycji kontrolerów. Należy zastąpić te pięć
tabel dokładną kolejnością `bone_map` z plików `noTMD-section-palette.json` i
`tmd-section-palette.json`, zbudować x86 `/O2`, a potem wykonać osobny test obrazu
lewej i prawej dłoni oraz test odłączonego fragmentu 390.

## Wdrożenie i test — 2026-10-10

Tabele zostały zastąpione kolejnością `bone_map` dla wszystkich sekcji:

- noTMD glove: 35 slotów;
- noTMD sleeve/finger: 6 slotów;
- TMD glove: 36 slotów;
- TMD base: 15 slotów;
- TMD extra: 2 sloty.

Wynik testów:

- L1: `glove_isolate_test.exe` — PASS;
- L1: `glove_finger_test.exe` — PASS;
- build: x86 PE32 `/O2`, DLL SHA-256 `3A1DE39E67172DBDFA60759D725FD145944450473C6973979210B371A24C949A`;
- Steam+xrsim: świeży PID, `FOCUSED`, 2 projekcje, 0 błędów;
- gameplay audit: `IndependentHandsAudit ... success=2`;
- fragment długich palców: `GloveIsolation: suppressed detached sleeve/finger section verts=390 first=14490 tris=674`.

Steam został po teście automatycznie przywrócony do baseline. Test xrsim potwierdza
wykonanie ścieżki runtime i supresji, ale ostateczne potwierdzenie poprawnego obrazu
lewej/prawej dłoni wymaga jeszcze krótkiego testu na PSVR2.
