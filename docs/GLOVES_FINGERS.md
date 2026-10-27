# Glove isolation (FP hands, bez proceduralnej animacji palców)

Aktualny zakres celowo obejmuje wyłącznie izolację rękawic. Proceduralne
sterowanie palcami zostało usunięte, ponieważ zwiększało powierzchnię błędów
wejścia i renderingu. Pozostaje istniejący tor `glove_isolate.h` oraz
`GloveIsolate=2` w konfiguracji.

W trybie gloves-only odrzucany jest także wąsko zidentyfikowany, odłączony
fragment sekcji `verts=390, first=14490, tris=674` (alternatywnie `verts=3314`)
zaobserwowany w przechwyceniu gameplayu. Nie jest to filtr po samym rozmiarze:
główna sekcja rękawicy i sekcje TMD pozostają dozwolone.

## Działanie

Przedramię, ramię i rękaw są składane na żywy środek nadgarstka przez
`S' = S * D`, gdzie `D = T(-w) * Scale(0) * T(w)` w konwencji macierzy
modułu. Kości nadgarstka i palców pozostają nietknięte, podobnie jak
urządzenie TMD. Nieznane kości są tylko raportowane jako `Review` i nigdy nie
są składane.

Tryby:

- `0` — wyłączone,
- `1` — census bez modyfikacji palety,
- `2` — rzeczywiste składanie kości arm-flesh na nadgarstek.

## Pliki

| plik | zawartość |
| --- | --- |
| `spikes/view_matrix/glove_isolate.h` | klasyfikacja kości i matematyka kolapsu |
| `tools/reference-tests/hands/glove_isolate_test.cpp` | test L1 izolacji |

Test L1: `PASS: glove isolation (keep/collapse/review + collapse math)`.

## Ograniczenia dowodu

L1 dowodzi klasyfikacji i matematyki. L2 dowodzi builda PE32 oraz SHA DLL.
Dopiero L3 może dowieść, że w konkretnej scenie rękawice są widoczne, ramiona
znikają i oba oczy otrzymują obraz. Sam licznik drawów nie jest dowodem
widoczności. L4 pozostaje testem fizycznego PSVR2.
