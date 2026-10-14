# Etap 1 — zdrowie, amunicja, aktualna broń

Cel: te trzy informacje da się przeczytać w PSVR2 siedząc na podłodze, bez zasłaniania środka i bez zgadywania offsetu. Wygląd docelowy jest w `HUD_DESIGN.md` (wariant B). Ten etap nie buduje płyt na siatce broni.

## Krok 1 — pomiar, bez zmiany obrazu

MAIN włącza istniejący cenzus UI na krótką chwilę w już zaplanowanym teście gogli. Z logu `UI element:` spisać clip `c8`, stride, liczbę prymitywów i skalę dla:

- klastra zdrowia w narożniku,
- klastra amunicji w drugim narożniku,
- cyfr po strzale i po przeładowaniu, żeby oddzielić magazynek od zapasu,
- promptu `PRZEŁADUJ` i promptu `[use]`,
- podkładu pełnoekranowego, który ma zostać wyjęty (`|x| >= 1`).

Falsyfikacja: jeśli zdrowie i amunicja dzielą jeden rysunek z podkładem albo z komunikatem, wcięcia nie ma. Wtedy etap 1 rysuje tylko własny stempel zdrowia z `Health/HealthMax` i nazwę z `Pawn.Weapon`, a amunicja zostaje w narożniku do następnego pomiaru. Tę decyzję zapisuje log, nie założenie.

## Krok 2 — wcięcie tylko zmierzonych klastrów

Nowy wiersz DISPLAY `HUD SAFE`, obok NOTE SIZE. Skala około zera na `c5–c8`, przed `ApplyEyeRemap`, ten sam porządek co notatki. Podkład z `|clip.x| >= 1` bez skali. Celownik stride 8 bez zmian i dalej chowany przy celowaniu kontrolerem. Domyślna skala wychodzi z pomiaru tak, aby klaster wszedł w bezpieczne koło, a nie z okrągłej liczby wziętej z notatek.

Nazwa broni: gdy klasa obiektu `Pawn.Weapon` się zmienia, jedna linia z `CommonText` przez 1,2 s przy klastrze amunicji, potem znika. Resolver nazw, który już jest w modzie. Brak dopasowanego klucza zostawia nazwę klasy, nie wymyślony polski opis.

Amunicja na tym etapie jest nadal rysunkiem gry. Nie czytamy `mCounts`.

## Kryteria akceptacji

- Siedząc na podłodze, w obu oczach widać zdrowie i amunicję bez odwracania głowy w narożnik gogli.
- Środek kadru nie dostaje paska, celownika ani nazwy broni.
- Pusty magazynek nadal pokazuje dotychczasowy komunikat przeładowania. Jeśli gra rysuje `0`, ta cyfra jest w kole, a nie obcięta.
- Niskie zdrowie widać po ubytku grafiki gry albo po `n/max`, nie po samym kolorze.
- Zmiana broni pokazuje jedną nazwę z listy `CommonText` i gaśnie.
- Notatki, kino, dłonie, cienie, dubbing i stereo rozgrywki wyglądają jak w ostatnim przyjętym teście.
- Log nie zawiera `HUD remap SKIPPED` dla geometrii świata i cenzus celownika zostaje `8 seen, 8 dropped` przy chowaniu.

## Test w goglach, około czterech minut

1. Usiąść na podłodze, broń w ręce, spojrzeć przed siebie. Odczytać zdrowie i naboje na głos.
2. Oddać serię do pustego magazynka. Odczytać przeładowanie. Przeładować.
3. Zmienić broń. Odczytać nazwę i sprawdzić, że zgasła.
4. Otworzyć notatkę i menu pauzy. Zamknąć. Środek walki z powrotem pusty.
5. Jeśli da się dostać obrażenia bez ryzyka zapisu: sprawdzić, że spadek zdrowia widać kształtem albo liczbą.

Po teście MAIN przywraca kopię, jeśli którykolwiek punkt padnie. Wynik fizyczny zapisuje MAIN. Ten dokument go nie przyznaje.
