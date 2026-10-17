# VFX nie odpalają — diagnoza przyczyny (SIDE, L1/L2, bez Steama)

Data: 2026-10-17. Branch: side/hud-script-holster-fixes. Pytanie: czemu generated
VFX (muzzle flash/smoke) "miały być naprawione, ale dalej nie działają".

## Łańcuch triggera (4 bramki, każda z falsyfikatorem w logu)

FILL (wątek gry, Hook_ProcessEvent, tryb celowania 9/10/11/12):
`firedWindow && FnIdx(FireAmmunition)>=0 && fnIdx==... && Rd(g_vfxHost)`
(`d3d9.cpp` ~24240). Mailbox single-slot, overrun liczony (`g_vfxFireOverruns`).
DRAIN, dwa warianty (obie strony sprawdzają `Rd(g_vfxHost)` + `!Rd(g_vrUiMenu)`):
- mid-scene `VfxEyeDraw` (~21339): tylko gdy `g_vfxDraw==1`. Udowodnione martwy
  kierunek (scena docelowa niewidoczna), domyślnie wyłączony.
- Present/backbuffer `DrawVfxOverlay` (~25625): tylko gdy `g_vfxDraw==2` (default).
  Log `VFX overlay: n=%d ...` tylko gdy drained||n>0 (bounded, pierwsze 12).
Bez pluginu `Collect` zwraca 0 (`vfx_host.h:113-115`) i `OnFire` jest no-opem.

## Root cause: konfiguracja + deployment, NIE shader ani hook

1. **Config kill (udowodniony, git blame 9f24fa9):** wysyłany
   `spikes/view_matrix/SingularityVR.ini` ma `VfxHost=0`, `VfxSmoke=0`,
   `VfxScreenFlash=0`. Samo `Rd(g_vfxHost)` w FILL i w obu DRAINach wystarcza,
   żeby żaden strzał nigdy nie wyprodukował quada — niezależnie od hooka,
   pluginu i atlasów. Poprzedni stan (przed 9f24fa9) miał wszystkie trzy =1.
2. **Brak pluginu w ścieżce produkcyjnej (udowodnione):** origin/main nie
   trackuje `SingularityVfx.dll` ani DDS-ów atlasu; `tools/deploy-singularityvr.ps1`
   nie ma żadnej obsługi VFX. Stock deployment = kill-switch z definicji.
   (Natywny flash jest przy tym WŁĄCZONY w shipped INI: `HideMuzzleFx=0` —
   czyli gracz widzi co najwyżej zepsuty natywny flash na szwie stereo, znany
   defekt z native-muzzle-stereo.md.)
3. **ABI host↔plugin ZGODNE (wykluczone jako przyczyna):** `vfx_plugin.h` i
   `vfx_host.h` identyczne origin/main vs feature/vfx-host-abi; stub buduje się
   czysto x86 /O2 w izolowanym katalogu (zero dotykania cudzego worktree),
   `dumpbin /exports` pokazuje wszystkie 7 wymaganych nazw
   (`VfxPluginAbiVersion/Init/Shutdown/OnFire/OnHit/Tick/Collect`).
4. Stub jest prawdziwą implementacją (flash 1–3 klatki + 2 puffy smoke na
   OnFire), nie pustym no-opem.

## Co musi zrobić MAIN (L3, Steam)

1. Sprawdzić zainstalowane: `Binaries\SingularityVfx.dll`, `Binaries\vfx_atlas\`,
   wartości INI `VfxHost/VfxDraw/VfxSmoke/VfxScreenFlash/HideMuzzleFx`.
2. W `%LOCALAPPDATA%\SingularityVR\view_matrix.log` po strzale:
   - brak linii `VFX overlay:` = FILL nigdy nie następuje (hook/nazwy skryptów
     albo `VfxHost=0` albo tryb celowania spoza 9/10/11/12);
   - `VFX overlay: n=0 ...` przy strzale = FILL działa, plugin pusty/nieobecny;
   - `VFX host init: plugin=absent` = brak DLL; `atlas=FAILED` = brak atlasów.
   - `VfxDiag=1` kropka cyjanowa rozdziela "nie rysuje" od "nie dolatuje do HMD".
3. Fix = deployment (plugin + atlasy ścieżką instalera — dziś nie ma jej w
   toolingu) + INI `VfxHost=1` (+ decyzja o `VfxSmoke`/`VfxScreenFlash`) + L3.
   Zmiana samych defaultów w INI bez DLL nic nie da; zmiana `HideMuzzleFx`
   bez VFX zostawia gracza z niczym. Decyzje należą do MAIN + akceptacja usera.
4. Znane ograniczenie niezależne od triggera: sloty cinematic vs dłuższe kwestie
   (patrz batch-3 audio) oraz fakt, że `VfxDraw=1` jest udowodnione martwy —
   nie testować go ponownie bez nowej hipotezy.

## Związek z bramką scripts-readiness (ten branch)

Hold lokomocji NIE blokuje toru VFX: retry hooka route-2, ResolveGameplayOffsets
i skrypty działają w Present niezależnie; hold zeruje tylko sticki/turn.
`ScriptsReady()` wymaga tego samego haka i offsetów, których wymaga FILL —
gdy hold puszcza, VFX-małżonek ma z dużym prawdopodobieństwem to, czego
potrzebuje. Nie jest to jednak dowód, że FnIdx(FireAmmunition) się zresolve'ował.
