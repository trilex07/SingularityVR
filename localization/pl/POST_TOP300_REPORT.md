# Polski draft — raport po TOP-300

- Plik lokalny: `C:\Users\Trilex\vr-mods\SingularityVR-localization-local\polish\full_draft.jsonl`
- TOP-300 pominięte: **299** wpisów (**299 unikalnych identyfikatorów**; jeden identyfikator występuje dwa razy w tabeli recenzji).
- Wpisów przetworzonych: **5575**.
- Statusy poza TOP-300: `{'skip': 5574, 'translated': 1183}` — brak `machine_draft` i `needs_review`.
- Statusy pozostawione w TOP-300: `{'empty': 7, 'needs_review': 74, 'translated': 90, 'machine_draft': 128}`; te wpisy są własnością użytkownika.

## Wyniki według kategorii

- `hud`: skip=409, translated=72; TOP-300 pominięte=0
- `item`: skip=5, translated=73; TOP-300 pominięte=0
- `objective`: skip=50, translated=221; TOP-300 pominięte=0
- `other`: skip=4625, translated=361; TOP-300 pominięte=0
- `subtitle`: ; TOP-300 pominięte=61
- `tutorial`: skip=1; TOP-300 pominięte=223
- `ui`: skip=484, translated=456; TOP-300 pominięte=15

## Słownik decyzji terminologicznych

- `Reload` -> `Przeładuj` / `PRZEŁADUJ`.
- `Checkpoint` -> `punkt kontrolny`.
- `Weapon Tech` -> `Technologia broni`.
- `Weapon Locker` -> `Szafa z bronią`.
- `Objective` -> `Cel`.
- `Collectibles` -> `Znajdźki`.
- `Difficulty` -> `Poziom trudności`.
- `Save Slot` -> `Miejsce zapisu`.
- `Ammo` -> `Amunicja`.
- `Score` -> `Wynik`.
- `Kills` -> `Zabójstwa`.
- `Assists` -> `Asysty`.
- `Damage` -> `Obrażenia`.
- `Accuracy` -> `Celność`.
- `Clip Size` -> `Pojemność magazynka`.
- `Firing Speed` -> `Szybkostrzelność`.
- `Fire Rate` -> `Szybkostrzelność`.
- `Loadout` -> `Wyposażenie`.
- `Leaderboard` -> `Tablica wyników`.
- `TMD` and weapon names remain unchanged.
- `[use]`, `[aim]` and other control tokens remain unchanged.
- Engine/editor/debug-only records are marked `skip` when no safe player-facing translation exists.

## Kontrola integralności

- Identyfikatory są odczytywane i zapisywane bez zmian.
- Tokeny, znaczniki i placeholdery są chronione podczas zamiany.
- Draft pozostaje plikiem lokalnym; repozytorium zawiera wyłącznie narzędzie i ten raport.
