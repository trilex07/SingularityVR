# HUD PSVR2 — projekt

Tożsamość: płaski, przemysłowy odczyt stacji Katorga-12. Ministry dla krótkich napisów interfejsu, TypekaMix zostaje przy notatkach. Kolory wzięte z już obejrzanych pól GFx: papier `#e6e1d6`, stal `#5a6f8d`, czerń płyty `#101214`. Ostrzeżenie jest bursztynem `#c4a46a` dopiero jako drugi kanał, obok liczby albo słowa. Bez neonu, bez zaokrąglonego „wizjera przyszłości” i bez celownika doklejonego na środek, bo przy celowaniu kontrolerem kula nie idzie w środek kadru.

Gracz siada także na podłodze. Nic nie jest kotwiczone do wysokości stojącej postaci. Płyty jadą z głową albo z kontrolerem.

## Trzy warianty

### A — wciąć istniejący HUD

Skala około środka na `c5–c8`, ta sama co NOTE SIZE, tylko dla zmierzonych klastrów zdrowia i amunicji. Oryginalna grafika zostaje. Da się to zrobić na obecnej ścieżce, gdy cenzus oddzieli te klastry od podkładu i od celownika. Słabo znosi siedzenie wyłącznie wtedy, gdy narożnik po wcięciu nadal wpada w dolną część koła. Nie dokłada nazwy broni, jeśli gra jej tam nie rysuje.

### B — odczyt na broni i na TMD (rekomendowany)

Spokojna gra jest prawie pusta. Naboje są stemplem na broni, po stronie ręki celującej. Zdrowie jest rzędem kresek na nadgarstku drugiej ręki, tam gdzie i tak jest TMD. Nazwa broni zapala się na broni tylko przy zmianie i gaśnie. Środek kadru zostaje dla wroga. Interakcja zostaje pod środkiem, w pasie już zmierzonym dla promptów. Siedzenie na podłodze przesuwa płyty razem z rękami, więc nie ma stałego zasięgu ramion.

### C — wąska tablica na wzroku

Zdrowie, nazwa i naboje w jednym pasie u dołu bezpiecznego pola, przyklejone do głowy. Czytelne, gdy ręce leżą na kolanach. Zasłania dolny świat i wygląda jak obcy HUD, jeśli nie jest awaryjnym trybem. W ustawieniach jako wyłączony zamiennik, nie jako domyślny wygląd.

Rekomendacja: wygląd B. Pierwszy wdrożony krok jest węższy i jest opisany w `HUD_STAGE1.md`: najpierw uczynić istniejące zdrowie, amunicję i nazwę broni czytelnymi w kole gogli. Płyty na kontrolerach są etapem późniejszym, bo to nowy renderer świata, a liczby naboi nie mają jeszcze nazwanego pola.

## Stany

Makiety: `docs/hud/mockups/`.

- Spokój. Dziesięć kresek zdrowia, magazynek pełny, brak celownika, środek wolny. Oryginalne narożniki narysowane jako martwe pola poza kołem.
- Walka i stres. Zostają dwie kreski i napis `2/10`. Magazynek `0`, obok otwarty prostokąt komory i słowo `PRZEŁADUJ`. Uderzenie z lewej jest kreseczką na lewym brzegu koła, nie czerwoną poświatą. Środek wolny.
- Zmiana broni. Na płycie naboi przez krótką chwilę widać `Centurion`, czyli `CommonText.ARPistol`.
- Interakcja. Pod środkiem: karb chwytu i `Notatka`. Oryginalny token `[use]` może zostać, dopóki ikona nie jest pewna. Napis nie wchodzi na przedmiot ani na wroga.

Pusty magazynek i niskie zdrowie są na makiecie walki, żeby było widać, że nie polegają na samym kolorze: zostaje liczba, słowo i zmiana kształtu.

## Miary

- Bezpieczne koło: treść wewnątrz około 80% widocznego pola gogli, nie w narożniku 16:9. Do wyliczenia służy istniejące `SafeAreaFromFov()` w `prompt_placement.h`. Legenda promptów nadal stoi na `y = 0.72` wysokości i ten dokument jej nie podpina.
- Cyfra główna, zdrowie albo naboje: 1,4–1,6 stopnia. Na broni trzymanej 40 cm od oka to około 10 mm. Nazwa broni i podpisy: 0,8–1,0 stopnia. Mniejsze nie przechodzą rzutu oka.
- Kontrast: papier na czarnej płycie, ramka stalowa. Przezroczystość płyty najwyżej taka, by znaki zostały czytelne na śniegu i na ciemnym korytarzu. Test akceptacji: oba tła, nie jeden zrzut.
- Ruch: pojawienie 150–200 ms, zniknięcie 300 ms. Puls niskiego zdrowia wolniejszy niż 0,5 Hz, tylko na kreskach, bez ruszania całego kadru. Brak sprężyn i braku skali, które dają wrażenie ruchu świata.
- Priorytet, od góry: puste przeładowanie, prompt interakcji, niskie zdrowie, naboje, nazwa broni, cel wyprawy, moce TMD tylko gdy `mPlayerHandsStatus==1`, napisy na dolnej krawędzi koła. Dwa napisy nie zajmują tego samego miejsca. Środek zostaje pusty.
- Obie ręce: płyta naboi jedzie z ręką celującą, zdrowie z drugą. Odbicie lustrzane, gdy ustawienie rąk jest odwrotne. Domyślnie prawa ręka celuje, zgodnie z obecną mapą PSVR2.
- Po polsku: „PRZEŁADUJ”, „Apteczka”, „Notatka”, nazwy z `CommonText`. Liczba jest wspólna dla języków. Nie zastępować polskich znaków ASCII.
- Ustawienia, strona DISPLAY, obok NOTE SIZE:
  - `HUD SAFE` — wcięcie klastrów, domyślnie po zmierzeniu, nie 100%.
  - `HUD MIEJSCE` — `broń` (B) albo `wzrok` (C). Domyślnie `broń`, a dopóki etap 1 nie rysuje płyt, wiersz jest nieaktywny.
  - `HUD RĘKA` — zgodne z ręką celującą.

## Wykonalność, osobno od wyglądu

| Wygląd | Da się teraz | Brakuje |
| --- | --- | --- |
| Wciąć zdrowie i amunicję | Ścieżka `HudStereoPair` i wyjątek podkładu | Współrzędne klastrów z cenzusu. Bez nich nie wiadomo, co skalować. |
| Liczba zdrowia | `Health` / `HealthMax` po nazwie | Tylko decyzja, czy rysować własny stempel, czy ruszyć rysunek gry. |
| Liczba naboi | Rysunek gry, jeśli cenzus go wyizoluje | Nazwa właściwości magazynku i zapasu. |
| Nazwa broni | Wskaźnik `Pawn.Weapon` i klucze `CommonText` | Potwierdzenie, że nazwa klasy żywego obiektu zgadza się z kluczem. Istniejący resolver nazw, bez nowego offsetu. |
| Płyta na siatce broni | — | Nowy quad na kontrolerze. Poza etapem 1. |
| Energia TMD, cele w świecie, napisy, kierunek obrażeń | Część ma klucze albo sam popup | Producent albo zmierzona pozycja. Zostają w grze tak, jak są, aż do własnego etapu. |
