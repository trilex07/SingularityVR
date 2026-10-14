# Inwentarz HUD-u Singularity VR

Odczyt źródeł z commita `72c8e1c` (`fix/post-opening-followups`), bez uruchamiania gry.
Offsety poniżej są cytatem wcześniejszego pomiaru. Etap 1 ma je rozwiązywać po nazwie
właściwości, tak jak robi to obecny solver, a nie wpisywać na sztywno.

## Potwierdzone

| Element | Co jest pewne | Gdzie |
| --- | --- | --- |
| Zdrowie | `Pawn.Health` i `Pawn.HealthMax` czyta istniejący solver po nazwie. Zapisany pomiar: `+0x2E4` i `+0x2E8`, głębokość 10. | `ENGINE_NOTES.md`, `d3d9.cpp` przy `g_offHealth` |
| Apteczki | `RvGameSharedHUD.mHealthPackCount` na żywej klasie `RvHUD`. Zapisany pomiar `+0x588`. Wybrane celowo zamiast tablicy `mCounts`, bo indeks enum jest nieznany. | `d3d9.cpp` około komentarza run 200 |
| Broń w rękach | `Pawn.Weapon` to wskaźnik, nie TMD. Zapisany pomiar `+0x3D4`. Klasa pistoletu w notatce silnika: `RvWeaponShared ARPistol`. | `STATUS.md` punkt 10, `ENGINE_NOTES.md` |
| TMD podniesione | `mPlayerHandsStatus`: 0 nic, 1 TMD, 2 broń. Zgodne z cenzusem meshy w sześciu wdrożeniach, także przy zmianie broni. TMD nie podmienia `Pawn.Weapon`. | `d3d9.cpp` `HANDS_NONE/TMD/WEAPON` |
| Celownik | Cztery kreski, stride 8, zmierzone przy `(±0.025, 0)` i `(0, ±0.045)`. `HideCrosshair=1` chowa je, gdy celowanie jest kontrolerem. | `LooksLikeCrosshair` |
| Zdrowie i amunicja na ekranie | Od runu 151 są w obu oczach. Siedzą w narożnikach klatki 16:9, poza wygodnym kołem gogli. Mechanizm wcięcia (`skala c5–c8` przed przesunięciem oka) jest zrobiony dla notatek, nie dla tych narożników. | `STATUS.md` punkt 2, `HudStereoPair` |
| Komunikat środkowy | Tekst komunikatu doszedł do obu oczu w runie 147. „RELOAD” i „R Open” leżą blisko środka, stride 20. | komentarz przy zrzucie UI, run 153 |
| Interakcja | `LooksLikeContextPrompt`: stride 20, 16–40 prymitywów, `\|x\|<0.24`, `y` od −0.25 do −0.05. Stabilne dla skrzyni i podniesienia. Ikona PSVR2 dokłada się obok i nie gasi oryginalnego tekstu. | `d3d9.cpp`, `DrawVrContextPrompt` |
| Notatka | Marker notatki zmierzony: podkład stride 8 i panel stride 12. `NoteInsetPct` domyślnie 40, strona DISPLAY, wiersz NOTE SIZE. | `NoteMarker` |
| Tokeny przycisków | W tekstach gry: `use fire impulse age crouch grab flashlight aim dash switchwpn jump reload cancel health`. Rozwiązanie tokenu jest w exe, nie w skrypcie. | `docs/PSVR2_PROMPT_MAP.md` |
| Nazwy broni | Klucze `CommonText`: Centurion, AR9 Valkyrie, Autocannon, Spikeshot, Wyrzutnia Dethex, Volk S4, Kasimov SNV-E99, Seeker, RLS-7. | szkic lokalizacji, nie zrzut z gry |
| Moce TMD | Klucze `TMDHUD`: IMPULSE, ANTYGRAWITACJA, BLOKADA CZASU. Osobny popup „TMD ENERGY” widziano jako rysunek UI (run 253). | lokalizacja i komentarz run 253 |
| Ścieżka rysowania UI | Ortograficzne rysunki user-pointer po zapisie `c5–c8`, tylko do celu sceny. Pełnoekranowy podkład (`\|x\|>=1`) nie może się kurczyć. Skala po przesunięciu oka rozjeżdża oczy. | `HudStereoPair` |
| Ustawienia | Strona DISPLAY już istnieje i jest miejscem przewidzianym na wcięcie HUD-u. | `kPageName`, komentarz run 165 |
| Wzrost postaci | `BaseEyeHeight` jest liczone od środka cylindra kolizji, nie od podłogi. Panel na stałej wysokości świata pomyli siedzenie na podłodze ze staniem. | `ENGINE_NOTES.md` |

## Klucze tekstów, bez potwierdzonego miejsca na ekranie

Te napisy są w szkicu polskim. Nie wiadomo, który rysunek GFx je pokazuje.

- `RvUI.Reload` = „PRZEŁADUJ”, `Tutorial.Reload` = „Naciśnij [reload], aby przeładować”.
- `RvUI.NoAmmo` = „Brak amunicji”, `RvUI.FullAmmo` i warianty per broń.
- `Pickup.HealthPack` = „Apteczka”, `Pickup.TakeHealthPack` = „[use] Apteczka”.
- `RvUI3D.Objectives` = „Cele”. Strona o tej nazwie występuje w przechwytywaniu menu; pozycja celu w trakcie gry nie jest zmierzona.
- Podniesienia amunicji: `[use] [ARPistol] Amunicja` i odpowiedniki pozostałych broni.

## Nieznane — nie zgadywać

- Pole liczby naboi w magazynku i w zapasie. Komentarz wspomina „ammo readout” jako rysunek, nie jako właściwość. `mCounts` wymaga indeksu `ERvCountItem`, którego nikt nie rozkodował.
- Pole energii TMD. Jest popup, nie ma nazwy właściwości.
- Clip-space narożników zdrowia i amunicji. Wiadomo, że są w narożnikach. Nie ma z tego odczytu współrzędnych `c8`.
- Napisy dialogowe: brak producenta w przejrzanym kodzie.
- Kierunkowy sygnał obrażeń i winieta: brak producenta.
- Który stride i tekstura należą do cyfr zdrowia, a które do amunicji. Bez cenzusu UI (`UiElementDump`) wcięcie całego HUD-u ściągnęłoby też celownik, komunikaty i podkłady.
- Czy każda broń ma ten sam czterokreskowy celownik. Komentarz każe zmierzyć ponownie, gdy inna broń zostawi własną siatkę.
- Ścieżka GFx konkretnego pola amunicji. Film notatek jest opisany osobno. Tego HUD-u nie rozbierano na tagi.

## Czego ten projekt nie rusza

Kino świata, chwyt dłoni i pistoletu, polskie głosy pierwszego obszaru, glify notatek, logo, wskaźnik menu, oryginalne SFX broni, projektor, cienie i stereo rozgrywki zostają przy MAIN.
