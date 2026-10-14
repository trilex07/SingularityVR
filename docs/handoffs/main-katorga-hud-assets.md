# Handoff MAIN — assety HUD-u Katorga

SIDE, gałąź `fix/psvr2-hud-design`. Bez merge i bez Steam. To jest paczka grafiki, nie renderer.

## Gotowe

- Płyty SVG z pustymi polami danych: `assets/hud/katorga/plates/`.
- Stany i podglądy, w tym mały rozmiar, jasne i ciemne tło, obie ręce, wersja bez koloru: `states/` i `previews/`.
- Atlas i `atlas.json`: `assets/hud/katorga/atlas/`.
- Opis tokenów i budżet: `docs/hud/KATORGA_ASSETS.md`.

## Do późniejszej integracji, niezweryfikowane

1. Etap 1 z `HUD_STAGE1.md` nadal jest pierwszy: cenzus, potem wcięcie istniejącego zdrowia i amunicji. Te płyty nie zastępują etapu 1.
2. Quad na kontrolerze jest osobno. OBJ `frame/plate-frame.obj` nie ma gniazda. Nie parentować go do siatki broni ani TMD.
3. Podmiana promptu: w zdaniu zostaje polski tekst, a w miejsce `[token]` wchodzi komórka atlasu. Nie dodawać drugiej legendy. Nie używać `prompt_roles.h` (krzyżak i R1) ani tabeli „Pad N”.
4. `[health]` to kółko, prawa ścieżka `b/click`, maska `kCircleHealthMask`. `[use]` i `[reload]` to jeden prawy chwyt. `[age]` to kwadrat na lewym `x/click`. `[cancel]` to kółko tylko w menu (`g_actUiBack` wysyła `XI_B`). W rozgrywce to samo kółko wysyła zdrowie.
5. Lewa ręka celująca odbija ścieżki przy restarcie. Zdrowie wpada wtedy na `left/y`, gdzie stoi menu. Tego nie rysować jako gotowej ikony.

Test gogli należy do MAIN i ma osobny skrypt w `HUD_STAGE1.md`. Ten pakiet go nie zalicza.
