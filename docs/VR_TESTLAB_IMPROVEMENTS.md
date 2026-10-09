# SingularityVR Test Lab — plan dowodowy bez PSVR2

Cel: powtarzalny pipeline

`Build → Synthetic XR → Replay .vrrec → Sandbox Game → Capture L/R → assertions → PASS/FAIL`.

„PASS” oznacza spełnienie konkretnych kryteriów i komplet dowodów. `runtime
ready`, licznik drawów, log `VFX`, zgodność ABI albo sam fakt uruchomienia gry
nie wystarcza.

## Stan obecny

SingularityVR już ma:

- `dvr_xrsim` jako 32-bitowy OpenXR runtime wybierany przez
  `XR_RUNTIME_JSON`;
- `tools/xrsim_replay_vr_session.py` — odczyt `.vrrec`, monotoniczne batch'e,
  zachowanie event edges, `--stride`, raport i dry-run;
- `tools/vr_session_dump.py` — read-only walidację formatu i zakresu próbek;
- `tools/singularityvr-test-bridge.ps1` — PE32, SHA-256, manifest/O2 oraz
  collection log/state/capture;
- `tools/steam-xrsim-singularity.ps1` — runner bezpiecznie backupujący i
  przywracający instalację, ale przeznaczony wyłącznie dla głównego agenta;
- canonical replay `session_2026-10-09_14-18-07.vrrec`: 5573 próbki,
  80.641 s, replay stride 4 = 1420 batchy, fire 8, melee 2, recenter 1,
  UI click 6, route gameplay/menu/panel;
- per-eye/SBS capture, projection views i state JSON.

Najważniejsza luka: obecne dowody potwierdzają stan runtime i część obrazów,
ale nie łączą jeszcze w jeden automatyczny verdict: event → render pass →
fresh L/R pixels → brak regresji frametime.

## Implemented in this branch

- `tools/vr_capture_oracle.py` implements a dependency-free PNG/BMP oracle.
  It validates the `singularityvr.capture.v1` identity contract, image hashes,
  non-empty eyes, ROI metrics, MAD, SSIM, duplicate/stale captures, baseline
  pairing and explicit `PASS`/`FAIL`/`INCONCLUSIVE` exit codes.
- `tools/singularityvr-test-bridge.ps1` remains the entry point. `Preflight`
  is read-only, `Collect` converts only a coherent xrsim capture plus matching
  acknowledgement/state into a manifest, and `VerifyCapture` invokes the
  oracle. `Collect` refuses a capture if the acknowledged command frame,
  eye-release frame, or source images do not agree.
- `tools/tests/test_vr_capture_oracle.py` covers the five mandatory synthetic
  cases: PASS, stale FAIL, swapped-eye FAIL, missing-effect FAIL against a
  trusted baseline, and unknown-baseline INCONCLUSIVE.

Example verification command:

```powershell
pwsh -NoProfile -File .\tools\singularityvr-test-bridge.ps1 `
  -Action VerifyCapture `
  -Manifest .\run\capture.capture.json `
  -Baseline .\baseline\capture.capture.json `
  -Previous .\run\previous.capture.json `
  -Roi effect=100,100,300,300 `
  -EffectRoi effect `
  -Report .\run\oracle.json
```

The oracle does not infer “hand”, “shadow”, “prompt”, or “VFX” from bright
pixels. An effect assertion means only that a declared ROI changed in both
eyes relative to a trusted same-scene baseline. Semantic identity remains
`INCONCLUSIVE` until a stronger scene/event oracle is supplied.

## TOP 5 ulepszeń Test Lab

### T1 — Frame-identity capture contract (P0)

Rozszerzyć istniejący capture/state contract o `runId`, PID, `cmdSeq`,
`frameIndex`, `captureSeq`, `sessionState`, `projectionViews`, layer list,
left/right path i SHA. Capture jest ważny tylko, jeśli oba obrazy i JSON są
świeże i mają ten sam frame/capture sequence.

PASS:

- state PID == sandbox game PID;
- `sessionState=FOCUSED`, dwa projection views;
- left/right/sbs istnieją, SHA zapisany, JSON wskazuje ten sam frame;
- brak `errors`, out-of-order i stale-command.

FAIL: jakakolwiek niezgodność lub użycie pliku z poprzedniego runu.

Adaptacja z BioShock: `xrsim_frame.cpp` snapshot na `xrWaitFrame` i copy
layerów w `xrEndFrame`; `xrsim_compositor.cpp` capture tylko na żądanej
klatce. Nie tworzyć drugiego runtime.

### T2 — Per-eye image oracle (P0)

Dodać mały, read-only analizator PNG/BMP/ROI do pipeline, bez ingerencji w
runtime. Każdy test deklaruje ROI/kolor lub baseline. Wynik zawiera luma,
non-black, edge/alpha occupancy, L/R MAD/SSIM i delta względem baseline.

PASS przykładowo:

- `Hands`: ROI dłoni ma occupancy > baseline + δ w obu oczach, centroid mieści
  się w tolerancji, a negatywny `DropTracking` usuwa occupancy;
- `Shadows`: shadow ROI różni się od scene-without-shadow baseline w L i R,
  a różnica nie jest tylko czarnym prostokątem;
- `VFX`: muzzle/fire ROI ma wzrost emissive/luma w obu oczach w oknie eventu;
- `Prompt`: prompt ROI > tło po trace i wraca do baseline po odejściu;
- `Stereo`: L/R są świeże, mają sensowną różnicę przy yaw, ale nie są stale
  jedną skopiowaną klatką.

FAIL, jeśli tylko log mówi „drawn” lub licznik drawów rośnie bez widocznego
efektu.

### T3 — Deterministic `.vrrec` replay gate (P0)

Zbudować scenariusz wrappera nad `xrsim_replay_vr_session.py` dla
`session_2026-10-09_14-18-07.vrrec`: preflight dump, clean scratch dir,
launch, FOCUSED wait, replay, capture markers i final release-state.

PASS:

- źródło ma oczekiwany SHA/5573 próbek lub jawny manifest wersji;
- batch count/event count/transitions zgodne z manifestem;
- żadna komenda nie przepadła, `lastCmdError` puste;
- final A/B/X/Y/menu false, trigger/grip 0, stick centered;
- każde capture ma frame identity z T1.

Tryb `step` służy do asercji klatka-po-klatce; tryb `free` tylko do smoke.
Stale command/ack/state pliki są błędem testu, nie „flakiem”.

### T4 — Lifecycle/performance regression gate (P1)

Jedna sesja obejmuje boot → gameplay → menu → gameplay, co najmniej trzy
powtórzenia route i osobne okna `warmup`, `capture`, `menu-return`. Zbierać
frame interval, submit age, dropped/stale eye, CPU/GPU jeśli dostępne oraz
liczyć p50/p95/p99; baseline musi mieć ten sam build, rozdzielczość, runtime i
scenariusz.

PASS przykładowo:

- brak stale eye i out-of-order;
- p95/p99 w każdym odcinku nie przekracza baseline + ustalony budżet;
- powrót do menu nie powoduje trwałego wzrostu mediany/p99 w kolejnym
  gameplay;
- test kończy się świeżym capture i poprawnym cleanup.

FAIL przy braku baseline, nieporównywalnym scenariuszu albo wzroście p99,
nawet gdy średni FPS wygląda dobrze.

### T5 — Isolated agent sandbox and evidence bundle (P1)

Każdy poboczny agent dostaje `xrsim-scratch/<run-id>/` z własnym manifestem,
`command.txt`, `ack.txt`, `state.json`, logiem, capture i raportem. Runner
uruchamia dokładny PID, zapisuje commit/SHA/PE32/O2 i nie dotyka Steam.

PASS:

- katalog nie ma plików z poprzedniego runu;
- proces używa manifestu runtime z tego katalogu;
- bundle ma manifest, log, replay report, L/R/SBS i hash każdego artefaktu;
- żaden plik w `Steam\...\Binaries` nie zmienił SHA.

Tylko główny agent może uruchomić deployment Steam runner. Poboczny agent
nie może podmieniać `d3d9.dll`, `SingularityVfx.dll` ani INI.

## Test matrix i mierzalne kryteria

| Test | Akcja | Automatyczny dowód | PSVR2 |
|---|---|---|---|
| XR boot | launch sandbox + wait | świeży PID, FOCUSED/VISIBLE, 2 views, zero errors | tracking comfort |
| Hands | replay grip/aim + yaw | state pose + palette/render evidence + hand ROI L/R | naturalność pozy/occlusion |
| Shadows | fixed scene + light/head move | shadow pass log + sampled depth/ROI delta L/R | percepcja stabilności cienia |
| Prompt | approach chest/fountain, leave | trace/context state + prompt ROI visible/hidden | readability/placement comfort |
| VFX | fire/melee event | event edge + spawn + L/R ROI + fresh copy | persistence/latency in HMD |
| Menu | gameplay→menu→gameplay ×3 | p50/p95/p99, no stale capture, final route state | UI distance/scale |
| Stereo | yaw + two captures | projection layer, IPD band, L/R difference and freshness | binocular artifacts/smear |
| Replay | canonical `.vrrec` | counts, transitions, releases, frame identity | only subjective route feel |

## Build and gate ordering

1. L1: `vr_session_dump.py`, replay dry-run, math/unit/synthetic tests,
   image oracle fixture tests. No game and no Steam.
2. L2: build x86 PE32 `/O2`, record commit, SHA-256 and manifest; compare
   candidate only against the isolated artifact. Do not modify `d3d9.cpp` in
   this workstream.
3. L3: sandbox game with existing dvr-xrsim, canonical replay, T1/T2/T4/T5,
   then optional Steam smoke through the main agent's approved bridge. L3
   PASS must include artifacts, not just console output.
4. L4: physical PSVR2 only for claims that image/state cannot prove:
   binocular comfort, real tracking loss/recovery, controller occlusion,
   latency, reprojection smear, adaptive hardware and final visual acceptance.

No agent may report `FIXED` without the gate required by the feature. Use
`IMPLEMENTED_NOT_VERIFIED` when code/path exists but the visual or runtime
oracle is absent; use `READY_FOR_PHYSICAL_ACCEPTANCE` only after L3.

## Immediate implementation sequence

1. Add T1 manifest fields and stale-file guard around the existing replay
   scratch directory.
2. Add T2 image analyzer and fixture tests for L/R pairing, non-black, ROI
   occupancy, MAD/SSIM, and stale capture rejection.
3. Add one canonical `.vrrec` scenario manifest with expected counts and
   transitions; use it in dry-run and sandbox replay.
4. Add T4 frame-time CSV/JSON aggregation with p50/p95/p99 and menu-return
   comparison.
5. Add an evidence bundle checker that emits PASS/FAIL and refuses to claim
   visual success when only logs/counters exist.

Separate follow-up task: if existing `dvr-xrsim` cannot expose T1 metadata or
   capture layer identity, specify a small additive runtime instrumentation
   change. Do not implement a second OpenXR emulator in this task.

## Security review performed

- Static source review only for all three repositories; no foreign DLL, EXE,
  installer, PowerShell runner or unknown binary was executed.
- BioShock `bvr_xrsim32.dll` and Dishonored `dvr_xrsim32.dll` were inspected as
  source/artifact references, not loaded into the host.
- No Steam install, production DLL, `d3d9.cpp`, save, registry or OpenXR
  system setting was modified.
- The requested simulated OpenXR is available in SingularityVR through the
  existing Dishonored-derived `dvr-xrsim` path and has local probe/replay
  evidence. Full compatibility with BioShock's xrsim ABI and complete
  visual proof of hands/shadows/VFX is **not confirmed**.
