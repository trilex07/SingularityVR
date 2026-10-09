# VR reference implementation gaps

Analiza kodu, nie tylko README. Dokument jest referencją projektową; nie
oznacza, że kod z obcego repozytorium został skopiowany lub że jego runtime
jest ABI-kompatybilny z SingularityVR.

## Revisions and licensing

| Projekt | Commit analizowany | Licencja / użycie |
|---|---|---|
| [BioShock Trilogy VR](https://github.com/VR-Stereo-Hub/bioshock-trilogy-vr) | `41f004bc6a1e4fa8be0d0e0749f9ac5a0453e75` (`main`) | MIT; tylko koncepcje i ścieżki źródłowe |
| [Dishonored-VR](https://github.com/VR-Stereo-Hub/Dishonored-VR) | `7c1cb8a323e889220d33e62dbb7ed2b64ec765a5` (`VR-Main`, v1.0.3) | zlib/libpng / DXVK-derived notices; clean-room adaptacja |
| [universal-modder](https://github.com/rehan-remade/universal-modder) | `8370faa8e114baf33acdb23079aff552a7728c4b` | MIT; wykorzystano wzorce oracle/evidence, nie vendoring |

Checkouty referencyjne pozostają poza repozytorium SingularityVR w
`C:\Users\Trilex\vr-mods\references\` oraz
`agent-skills-sources\universal-modder`. Nie wykonano ich binariów,
instalatorów ani obcych skryptów.

## TOP 10 rzeczy do adaptacji

### 1. Atomowy punkt commitowania sterowania xrsim

BioShock: `src/tools/xrsim/xrsim_control.cpp` — osobny poller komend, parser
parse-then-commit i publikowanie całego batcha dopiero na granicy
`xrWaitFrame`; `;`/multiline nie może zostawić połowy pozy.
`src/tools/xrsim/xrsim_frame.cpp` gwarantuje, że `xrEndFrame` kopiuje warstwy
przed wygaśnięciem pamięci wywołującego.

Singularity ma atomiczne zapisy w `tools/xrsim_replay_vr_session.py`, ale
nie ma jeszcze jawnego testu, że state/capture odpowiada dokładnie tej samej
granicy klatki. Adaptacja: dodać `frameIndex`/`cmdSeq` do raportu replay i
odrzucać capture z innym indeksem.

### 2. Deterministyczny step mode i ruch ciągły

BioShock `xrsim_control.cpp`: `pace step`, timeout/starve i hold liczony w
klatkach (`12f`), a `head move`/`hand smooth` są potrzebne do testów prędkości
zamachu, nie tylko do teleportu. Stale command files są odrzucane po czasie
startu procesu.

Singularity `xrsim_replay_vr_session.py` zachowuje event edges i monotoniczne
timestampy, lecz plan testów powinien używać `step` dla asercji per-frame, a
`free` tylko dla smoke/performance. Minimalna adaptacja to wrapper replay,
który zapisuje PID/scope katalogu i czyści stare `command.txt` przed startem.

### 3. Capture warstwy OpenXR, nie tylko okna

BioShock `src/tools/xrsim/xrsim_compositor.cpp`: składa rzeczywiście
zgłoszony layer stack — projection per eye i quad layers — z osobnymi RT,
staging texture i workerem encode. Zapisuje `left.png`, `right.png`, `sbs.png`
oraz JSON. `SavedState` przywraca RTV/DSV, blend/depth/raster, viewport,
shadery, constanty i SRV.

Singularity test bridge zbiera bitmapy/log/state, a canonical replay ma
`left/right/sbs`, ale obecny bridge nie robi jeszcze walidacji layer-to-image.
Adaptacja: uznać capture za ważny tylko, gdy JSON zawiera świeży projection
layer, dwa views i ten sam `frameIndex` co obrazy.

### 4. Automatyczny test stereo zamiast samego licznika drawów

Dishonored `tools/eye-check.ps1` wykonuje pięć nóg: parowanie L/R, projection
layer/IPD, różnica obrazu, kontrolowany yaw i powrót do stabilnego pair.
`tools/xrsim-shot.ps1` sprawdza `captureSeq`, `sessionState`, luma,
non-black, quad/projection counts i aim-ray metrics.

Singularity ma `canonical-replay` i per-eye capture, lecz brak wspólnego
automatycznego verdictu obrazu. Adaptacja: L/R freshness + non-black + SSIM/
mean-absolute-difference względem baseline; progi kalibrować na tej samej
rozdzielczości, bez uznawania identyczności oczu za poprawność.

### 5. Dane kompozytora jako oracle widoczności

BioShock `xrsim_compositor.cpp` oprócz warstw zapisuje `meanLumaL/R`,
`nonBlackPctL/R`, eye separation, `ClaimRatioH`, aim-ray dots i maksymalne
odchylenie. To pozwala odrzucić przypadek „draw był wykonany, ale efekt
zniknął przed copy do OpenXR”.

W Singularity logi `shadow_path`/`vfx_path` w
`tools/steam-xrsim-singularity.ps1` są tylko dowodem ścieżki kodu. Trzeba je
połączyć z maską obrazu ROI i per-eye capture.

### 6. Rozdzielenie HUD sink/provider i anchorów

Dishonored `src/core/gfx/hud_class.*`, `hud_capture.*`, `hud_layout.*`:
Scaleform jest klasyfikowany po backbuffer/full viewport/depth/blend i probe
regionu vertex-shadera; draw trafia do prywatnego sinka, a później jako
world/head/hand-relative `XrCompositionLayerQuad`. `src/core/vr/hud_anchor.h`
ma czystą matematykę pozycji, billboardu i guardów „too near/behind face”.

Singularity ma `HudStereoPair`, `ui_pointer.h` i remap menu. Bezpieczna
adaptacja to testowalny podział classify → capture → place → submit, nie
przeniesienie adresów UE3 ani całego HUD-u Dishonored.

### 7. Jedna autorytatywna ścieżka ray → trace → prompt/click

Dishonored `src/game/dishonored/aim_ray.cpp` publikuje jeden `FireFrame`, a
`fire_aim_math.h` zamienia ray na kamerę; gra nadal wykonuje trace i sama
decyduje o highlight/click. To jest bezpieczniejsze niż osobne, rozjeżdżone
prompty i input.

Singularity ma `ui_pointer.h` oraz test interakcji. Należy dodać oracle:
context event + trace hit + prompt ROI > tło, a po odejściu prompt ROI wraca
do baseline. Sam `ui_click` nie dowodzi widocznego promptu.

### 8. Ręce: model renderowany, nie tylko poprawna poza

BioShock `src/game/bioshock1r/hands.cpp`, `aim.cpp`, `frame_context.h` oraz
`bioshockinf/hands.cpp` rozdzielają grip/aim, uczą mapowania obiektu do ręki,
utrzymują model trim i używają tej samej transformacji dla ręki, broni i
muzzle ray. Dishonored `src/game/dishonored/hands/hand_mesh.cpp` wykonuje
bone-matrix rewrite, split arm/hand i mirror open-hand, bez polegania na
nieudowodnionym two-bone IK.

Singularity ma `arm_ik.h`, `support_grip.h` i synthetic hand tests. Brakuje
połączenia pose → wrist/palette → draw → widoczny piksel. Minimalny test:
pose z dużym, kontrolowanym przesunięciem, log palette, per-eye hand ROI i
negative test dla `DropTracking`.

### 9. Stabilizacja i lifecycle zamiast stałego „smooth everything”

Dishonored `src/game/dishonored/head_track.cpp`, `camera.cpp`, `cam_shake.cpp`
grupuje próbki chain stamp, używa yaw-only basis pod projection i usuwa
konkretne wpływy shake/bob/reaction przez hold table. Nie maskuje problemu
globalnym smoothingiem.

Singularity ma `camera_stabilization.h` i lifecycle route w canonical replay.
Adaptacja: mierzyć frametime i pose age oddzielnie w gameplay → menu →
gameplay; nie uznawać braku crashu za poprawny lifecycle.

### 10. Agentowe oracle i izolacja testu

universal-modder `skills/game-automation/SKILL.md`,
`skills/mod-any-game/SKILL.md` oraz
`knowledge/techniques/oracles-how-agents-know-a-mod-works.md` wymagają
powtarzalnej sceny, screenshotu/trace/logu oraz jawnego oracle. Repo ostrzega,
że uruchomienie, licznik i zielony test nie są dowodem widoczności.

Singularity ma `tools/singularityvr-test-bridge.ps1`, który preflightuje
PE32/SHA/O2 i zbiera dowody, lecz `steam-xrsim-singularity.ps1` nadal jest
narzędziem deploymentowym i nie może być używany przez pobocznego agenta.
Adaptacja: każdy agent dostaje własny `xrsim-scratch/<run-id>` i raport;
tylko główny agent może wykonać instalację DLL do Steam.

## Porównanie problemów SingularityVR

| Problem | Co pokazują referencje | Minimalna bezpieczna adaptacja | Dowód |
|---|---|---|---|
| Stutter/FPS | xrsim capture tylko na żądanej klatce; Dishonored rozróżnia sync/deferred/shared capture | oddzielić diagnostyczny capture od zwykłego frame path; profilować p50/p95/p99 | L1 synthetic + L3 replay/perf |
| Cienie | Dishonored nie ma stereo shadow-map oracle; wyklucza shadow targets z jitter/capture | testować politykę/sampling i ROI, nie budować nowego shadow renderer | L1/L3; PSVR2 dla comfort/real depth |
| Niewidoczne ręce | wspólny transform hand/weapon/aim i jawny palette/bone rewrite | dodać render-evidence do istniejącego arm_ik/support_grip | L1 pose, L3 per-eye ROI; L4 przy artefaktach percepcyjnych |
| Prompt skrzynia/fontanna | HUD classify/capture/layout + game-owned trace | test event→trace→ROI visible→hidden, osobno gameplay/menu | L3 image + state |
| Stutter po menu | menu hold i lifecycle-aware capture; nie czytać starej klatki | round-trip 3× z frame-time baseline i fresh capture | L3 p95/p99, brak wzrostu |
| VFX/muzzle | ten sam hand/muzzle ray, layer composition dowód | fire event + spawn log + ROI L/R + copy freshness | L3; L4 dla latency/comfort |

## Czego nie przenosić

- `bvr_xrsim32.dll` i `dvr_xrsim32.dll` są runtime'ami referencyjnymi z
  własnym ABI/manifestem. Ich załadowanie lub zgodność eksportów nie dowodzi
  poprawnego działania SingularityVR.
- Nie kopiować engine-specific offsets, hooków ani obcych binariów do repo.
- Nie budować drugiego emulatora. Najpierw wzbogacić istniejący
  `dvr_xrsim`/Test Bridge o frame identity, layer-aware capture i verdicty.
- `bvr_xrsim32.dll` istnieje w bieżącym checkout BioShock jako artefakt/source
  workflow, a `dvr_xrsim32.dll` w checkout Dishonored jako zbudowany artefakt;
  nie zostały uruchomione ani zweryfikowane jako kompatybilne z Singularity.

