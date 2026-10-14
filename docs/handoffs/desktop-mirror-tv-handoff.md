# Desktop / TV mirror — prawe oko (handoff MAIN)

## Problem

Przy grze w HMD okno gry na telewizorze/pulpicie pokazuje **surowy backbuffer SBS**
(lewa i prawa połowa obok siebie). To nie jest błąd podglądu pliku — brakuje
**lustra jednego oka**, jak w SteamVR.

## Rozwiązanie (w `d3d9.cpp`)

Po `UpdateFromHeadset()` (OpenXR już dostał pełny SBS), a **przed** `g_origPresent()`:

- `ApplyDesktopMirror()` robi `StretchRect` wybranej połówki → pełny ekran okna.
- HMD **nie jest dotykany** (capture do XR jest wcześniej).

### INI

W `[Render]`:

```ini
DesktopMirrorEye=2
```

| Wartość | Efekt |
|--------|--------|
| `0` | Domyślnie — stare zachowanie (SBS na pulpicie) |
| `1` | Lustro lewego oka, pełne okno |
| `2` | Lustro prawego oka, pełne okno |

Respektuje `g_swapEyes` (DELETE) — ta sama połówka co w `subImage.imageRect` dla oka 1/0.

Przy `g_scCropped` (menu flat / cutscene mono) oba oczy dostają tę samą połówkę;
lustro bierze **tę samą** połówkę co most XR (nie pustą prawą).

### Warunki

Działa tylko gdy: `VrModeOn()`, `g_dupDraws`, aktywna sesja XR (`g_xrRunning`).

## Użycie na TV

1. Zbuduj / wgraj DLL z gałęzi `fix/desktop-mirror-tv`.
2. W `SingularityVR.ini`: `DesktopMirrorEye=2`.
3. Okno gry na telewizorze (extended display), najlepiej **borderless / pełny ekran na tym monitorze**.
4. HMD bez zmian.

## Weryfikacja L3

- W HMD: normalne stereo.
- Na TV: **jeden** obraz, bez pionowej linii w środku; zgodny z prawym okiem (obrót głowy, UI).
- Log startu: `ini: DesktopMirrorEye=2 (...)`.

## Koszt

Dwa `StretchRect`/klatka na GPU — zwykle pomijalne vs readback XR.
