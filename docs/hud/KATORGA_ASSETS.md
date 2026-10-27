# Assety HUD-u Katorga

Grafika jest gotowa jako pliki. Podpięcie pod broń, nadgarstek i zdania gry nie jest zrobione i nie jest sprawdzone w goglach.

## Pochodzenie i licencja

Wszystkie płyty, ikony, podglądy i ramka OBJ są narysowane na nowo do tego moda. Objęte są licencją MIT repozytorium. Nie pochodzą z plików Singularity, z fontów gry, z atlasu Kenney ani z arkusza znaków konsoli. Żaden font nie jest osadzony. Napisy w SVG używają nazwy kroju tylko jako podpowiedzi dla podglądu. Raster podglądu powstał czcionką Segoe UI obecną w Windows w chwili generowania i ta czcionka nie wchodzi do gry.

Kolory są z projektu HUD-u: płyta `#101214`, stal `#5a6f8d`, papier `#e6e1d6`, bursztyn `#c4a46a` wyłącznie jako drugi kanał obok słowa albo ubytku kształtu.

## Pliki

- `assets/hud/katorga/plates/` — edytowalne płyty. Pola `health-value`, `ammo-mag`, `ammo-reserve`, `weapon-name-text` są puste.
- `assets/hud/katorga/states/` — spokój, mało zdrowia, pusty magazynek, zmiana broni. Nadal bez przykładowych liczb.
- `assets/hud/katorga/icons/` — pojedyncze komórki.
- `assets/hud/katorga/atlas/katorga-hud-atlas.svg`, `.png` i `atlas.json`.
- `assets/hud/katorga/previews/` — jasne i ciemne tło, mały rozmiar, obie ręce, zdania promptów, wersja bez koloru.
- `assets/hud/katorga/frame/plate-frame.obj` — płaska ramka 64×20×1,6 mm. Nie ma kości ani gniazda na broń albo TMD.
- `assets/hud/katorga/previews/weapon-wheel.png` — koło broni (atlas rząd 3, nie concept art).
- `assets/hud/katorga/3d/previews/weapon_wheel_generated.png` — ten sam renderer co wyżej, dla paczki MAIN.

Źródłem odtwarzalnym jest `tools/hud/katorga_kit.py`. Sylwetki broni w atlasie (`weapon-*`, `wheel-*`) są oryginalnym line-art moda, nie wycinkami z Singularity.

## Koło broni (R3)

Komórki atlasu y=128: `wheel-current`, `weapon-centurion`, `weapon-valkyrie`, `weapon-spikeshot`, `weapon-kasimov`, `wheel-slot-empty`. Mapowanie sektorów: `assets/hud/katorga/3d/weapon_wheel_catalog.json`. Podgląd: `python tools/hud/render_weapon_wheel_preview.py`.

## Stany

Spokój: dziesięć pełnych kresek, zamknięty prostokąt magazynka. Mało zdrowia: zostają dwie krótsze kreski i słowo MAŁO. Pusty magazynek: przerwany obrys komory i słowo PRZEŁADUJ. Zmiana broni: pusta linia nazwy nad nabojami. Bursztynowa ramka dochodzi przy MAŁO i przy pustym magazynku, ale po zdjęciu koloru zostają liczba kresek, przerwa w obrysie i słowo.

Liczby na podglądach PNG (`10/10`, `2/10`, `0/20`, `Centurion`) są tylko próbką czytelności. W płytach ich nie ma.

## Tokeny promptów

Sprawdzone w `spikes/view_matrix/d3d9.cpp` na `72c8e1c`, tabela `g_padButtons`, oraz `input_remap.h`. To nie jest mapa z `prompt_roles.h` (tam zdrowie jest krzyżakiem, a użycie R1) i nie jest tabelą „Pad N” z `prompt_label_rewrite.h`.

| Token | Ikona | Ścieżka, prawa ręka celuje | Co dostaje gra | Lewa ręka celuje |
| --- | --- | --- | --- | --- |
| `[health]` | `sense-circle` | `/user/hand/right/input/b/click` | maska D-pada w górę `0x0001` | `mirrorHand` przenosi na `left/y`. Ta sama ścieżka jest przyciskiem menu (trójkąt) i menu się nie odbija. Kolizja. Ikony lewej ręki nie wdrażać przed testem. |
| `[use]` | `sense-grip` | prawy `squeeze/value` | `XI_X` `0x4000` | lewy chwyt |
| `[reload]` | `sense-grip`, ten sam prostokąt | ten sam wiersz „Use / reload” | `XI_X` `0x4000` | lewy chwyt |
| `[age]` | `sense-square` | `/user/hand/left/input/x/click` | `XI_LSHOULDER` `0x0100` | `right/a`, czyli krzyżyk. Skok idzie w drugą stronę. |
| `[cancel]` | `sense-circle`, tylko menu | `g_actUiBack`, prawy `b/click`, bez odbicia | `XI_B` `0x2000`, gdy menu jest otwarte | zostaje prawe kółko |

Zdania, w które ikona ma wejść zamiast tokenu:

- `[health]` — „Naciśnij [health], aby użyć znalezionych w świecie apteczek.” Klucz `GameDialogs.HealthPackDesc`.
- `[use]` — „[use] Apteczka”. Klucz `Pickup.TakeHealthPack`.
- `[age]` — „Naciśnij [age], aby odnowić apteczki…”. Klucz `Tutorial.Medkit`.
- `[reload]` — „Naciśnij [reload], aby przeładować”. Klucz `Tutorial.Reload`.
- `[cancel]` — w tym przejściu nie ma polskiego zdania z tym tokenem. W kodzie anulowanie menu jest natywnym B, a kółko je wysyła tylko przy otwartym menu. W grze kółko wysyła zdrowie, a natywne B idzie z prawego drążka w dół (kucanie). Anulowanie koła broni to L3 w `weapon_wheel.h` i nie jest tym tokenem.

Podmiana dotyczy samego glifu w zdaniu. Nie dokłada się drugiego rzędu podpisów.

Siedzenie na podłodze: podgląd `hands-floor` stawia płyty w dolnej części koła. Nie ma tu wysokości stojącej postaci ani punktu na siatce.

## Budżet renderowania

Jedna tekstura atlasu 512×256 RGBA8, 524288 bajtów nieskompresowanych, SHA-256 `c8287e418147b999…`. Komórka 64×64, początek w lewym górnym rogu. Prostokąty są w `atlas.json`.

Na oko: dwa quady (zdrowie i amunicja). Nazwa broni jest drugą linią quadu amunicji, nie trzecim rysowaniem. Ikona promptu nie jest quadem: zastępuje glif w już rysowanym zdaniu. Jedno mieszanie alfa, bez drugiego przejścia pełnoekranowego, bez cienia i bez poświaty. Puls poniżej 0,5 Hz tylko na kreskach niskiego zdrowia, bez skalowania płyty. Atlas nie przebudowuje się co klatkę. Jedna mipmapa wystarcza, bo płyta jest blisko dłoni.

Ramka OBJ nie wchodzi do tego budżetu, dopóki nie ma miejsca montażu.

## Czego ten pakiet nie robi

Nie czyta amunicji, nie kotwiczy płyty, nie zmienia `HudStereoPair`, kina, dłoni, cieni, dubbingu, logo, kursora, dźwięku, projektora ani instalacji Steam. Wynik jest offline. Nie jest przejściem na PSVR2.
