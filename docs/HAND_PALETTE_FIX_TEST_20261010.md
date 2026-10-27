# Naprawa stron dłoni — test 2026-10-10

## Zmiana

W `spikes/view_matrix/d3d9.cpp` poprawiono kolejność map palety GPU. Wcześniejsze
tabele były posortowanymi listami numerów kości, ale silnik oczekuje kolejności
slotów `bone_map` zapisanej w sekcjach meshów.

## Walidacja lokalna

- `tools/reference-tests/hands/glove_isolate_test.exe` — PASS;
- `tools/reference-tests/hands/glove_finger_test.exe` — PASS;
- build x86 PE32 `/O2` — PASS;
- SHA-256 DLL: `3A1DE39E67172DBDFA60759D725FD145944450473C6973979210B371A24C949A`.

## Walidacja runtime

Raport:

`deployment/hand-palette-fix-20261010/reports/steam-xrsim-run-20261010-151237.json`

Log:

`deployment/agent-test-bridge/20261010-151611/view_matrix.log`

Istotne wpisy:

- `IndependentHandsAudit ... success=2`;
- `GloveIsolation: suppressed detached sleeve/finger section verts=390 first=14490 tris=674`;
- `physical XR inputs`: obie dłonie `action 1 pose 1 position 1`;
- xrsim: `FOCUSED`, `projectionViews=2`, `errors=0`.

Po teście Steam wrócił do baseline: DLL `B69D6804...`, INI `CCA85A49...`.

## Status

Naprawa jest potwierdzona na poziomie kompilacji, testów logicznych i runtime
simulatora. Wizualna akceptacja w headsetcie pozostaje osobnym krokiem.
