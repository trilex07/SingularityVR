# Handoff MAIN — HUD PSVR2

SIDE, gałąź `fix/psvr2-hud-design`. Bez merge, bez Steam, bez podmiany plików gry.

## Co przeczytać

- `docs/hud/HUD_INVENTORY.md` — co jest zmierzone, a czego nie wolno zgadywać.
- `docs/hud/HUD_DESIGN.md` — warianty A/B/C, rekomendacja B, miary i ustawienia.
- `docs/hud/HUD_STAGE1.md` — pierwszy krok: pomiar, potem wcięcie zdrowia i amunicji oraz nazwa broni.
- `docs/hud/mockups/` — spokój, walka (niskie zdrowie i pusty magazynek), zmiana broni, interakcja.

## Decyzja

Domyślny wygląd to odczyt na broni i na nadgarstku TMD, bo gracz siada na podłodze i narożniki 16:9 wypadają z koła gogli. Etap 1 tego jeszcze nie rysuje. Etap 1 używa `HudStereoPair` i solvera właściwości, które już są.

Nie implementować energii TMD, celów, napisów ani kierunku obrażeń, dopóki nie ma producenta. Nie czytać amunicji z `mCounts`. Nie scalać wcięcia HUD-u z NOTE SIZE: notatka ma własny procent.

## Granice

Nie ruszać kina, chwytu, cieni, dubbingu, glifów notatek, logo, lasera menu, SFX broni, projektora ani stereo rozgrywki. Offsety z `ENGINE_NOTES.md` są pomiarem jednej budowy. W kodzie zostaje rozwiązywanie po nazwie.

Test gogli z `HUD_STAGE1.md` jest krótki i należy do MAIN. Wynik tego brancha jest projektem offline, nie przejściem fizycznym.
