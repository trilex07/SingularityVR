# SIDE — produkcyjne assety HUD-u Katorga

Kontynuuj projekt z `fix/psvr2-hud-design`, commit `cd76bac`. Jesteś mistrzem
art designu, game designu i level designu. Przeczytaj AGENTS.md, STATUS.md,
projektowe skille oraz HUD_DESIGN/INVENTORY/STAGE1. Pracuj we własnym worktree
i na własnej gałęzi; commit/push bez merge. MAIN nadal pracuje na
`fix/post-opening-followups`, PR19. Nie modyfikuj jego plików ani Steam.

Cel: oryginalne, lekkie assety wariantu B — odczyt amunicji na broni i zdrowia
na nadgarstku TMD. Styl: czarna płyta, stalowa ramka, papierowe cyfry i czytelne
oznaczenia zużycia. Bez generycznego neonu i bez zasłaniania środka kadru.
Gracz siedzi także na podłodze; nie zakładaj wysokości ani zasięgu ramion.

## Dostawa

1. Edytowalne płyty SVG: amunicja, zdrowie i krótka nazwa broni. Oddzielne
   pola dla danych; przykładowe liczby wyłącznie w makietach, nie wypalone
   w assetach produkcyjnych. Nie zakładaj nieznanej pojemności magazynku.
2. Atlas przezroczystych ikon/elementów: kreski zdrowia, magazynek, otwarta
   komora, przeładowanie, ostrzeżenia. Źródła SVG, podgląd PNG, jawny manifest
   prostokątów atlasu i paddingu. Rozdzielczość prototypowa, nie obietnica VR.
3. Stany normalny, niski poziom zdrowia, pusty magazynek, zmiana broni.
   Czytelność musi przetrwać brak rozróżniania kolorów: liczby/słowa/kształty.
4. Ikony PSVR2 Sense dla promptów. Zweryfikuj aktualne mapowanie akcji w kodzie
   MAIN; nie kopiuj mapy Xboxa/Pad N. Osobno opisz `[health]`, `[use]`, `[age]`,
   `[reload]`, `[cancel]`. Cel integracji: zastąpić natywny token/glif wewnątrz
   polskiego zdania, nie dokleić drugą legendę. Sam wygląd napisu Pad N nie
   dowodzi producenta glifu ani tekstu — to wciąż zadanie pomiarowe dla MAIN.
5. Podglądy na jasnym/ciemnym tle, mała wielkość kątowa, obie ręce, polskie
   teksty. Wymiary/proponowany dystans jawnie jako projekt, nie zmierzone dane.
6. Opcjonalny lekki prototyp ramki 3D dopiero po2D. Nie zgaduj mocowania na
   siatce broni/TMD; dostarcz jednostki, pivot, skalę i puste punkty mocowania
   do późniejszej kalibracji. Sam model nie daje działającego kotwiczenia.

## Granice integracji

Etap1 nadal najpierw mierzy native zdrowie/amunicję i poprawia czytelność
tylko udowodnionych klastrów. Nowe płyty nie zastępują tego pomiaru. Nie czytaj
amunicji z mCounts bez indeksu; nie implementuj kotwiczenia, hooków, rendererów
czy producentów danych. Energia TMD, cele, napisy i kierunek obrażeń czekają
na osobne udowodnienie producenta. Prototyp grafiki nie jest L3/L4.

Nie ruszaj kina, dłoni, cieni, dubbingu, fontów notatek, logo, kursora/lasera,
SFX, projektora ani gameplay stereo. MAIN zajmuje się tymi tematami. Nie
publikuj fontów/assetów wyciągniętych z gry. Oznacz pochodzenie i licencje
własnych elementów; font zastępczy w makiecie nie oznacza finalnej typografii.

Handoff: branch/SHA, dokładne pliki, source assets, podglądy, manifest atlasu,
budżet pamięci/rysowania, testy i niewiadome. Gotowa grafika oraz niezweryfikowana
integracja mają być osobno, bez twierdzeń o działaniu w goglach.
