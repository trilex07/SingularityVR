# Jump stall — Steam diagnostic collection runbook (2026-10-12)

**Accepted production DLL (restore after session):** SHA-256 `2FE5011C20DFEAAEDFFFFE58CD39E92A9570867F344649B00C0038AD97B39A47` @ `726e1a7`.

**Candidate branch for diagnostics:** `perf/input-jump-stall-diagnostics` (baseline `726e1a7` + `diagnostic_trace.h` + melee log throttle + analyzer only). **Do not** deploy SIDE `ce74e7d` wholesale — it removes glove/pause paths unrelated to this task.

## INI (game `Binaries\SingularityVR.ini`)

```ini
[Render]
Debug=1

[Diagnostic]
Enabled=1
```

`Debug=1` is required so **F11** / **Shift+F11** reach the diagnostic mode cycler (`DebugKey` gate).

## Mode equivalence (mandatory)

| Mode | Jump in game? |
|---|---|
| NORMAL / HAPTICS_OFF | Yes — valid stall comparison |
| XINPUT_OFF / XINPUT_OFF+HAPTICS_OFF | **No** (`feedGame=false`) — **non-equivalent** |
| BUTTONS_OFF | **No** face buttons to game — **non-equivalent** for Jump |

Check `published=1` on Jump lines in the log before comparing modes. See `docs/audits/input-jump-stall-psvr2-validation-20261012.md`.

## In-headset procedure (MAIN)

1. Deploy diagnostic DLL with `tools/deploy-singularityvr.ps1` (timestamped backup, verify SHA). Candidate: `deployment/candidate-input-jump-baseline-20261012`.
2. Launch **real Steam + SteamVR** (not xrsim).
3. Load gameplay, reproduce the **Jump** stall (same scene/load as prior reports).
4. Cycle modes with **F11**: `NORMAL` → `HAPTICS_OFF` → `XINPUT_OFF` → `XINPUT_OFF+HAPTICS_OFF` → `BUTTONS_OFF`. Spend **≥30 s per mode** with several Jump presses.
4. Optional: **Shift+F11** toggles action polling off (poses stay on) for one isolation pass.
5. Quit cleanly (hold PAUSE / WM_CLOSE path).
6. Collect `%LOCALAPPDATA%\SingularityVR\input_diagnostic_YYYYMMDD_HHMMSS.log` (created at first attach when `Enabled=1`).
7. Roll back DLL to `2FE5011C…` via deploy rollback or backup `20261012-133123` / `20261012-121230`.

## Analysis

```powershell
python tools/analyze-input-diagnostic-log.py "$env:LOCALAPPDATA\SingularityVR\input_diagnostic_*.log" > jump-stall-report.md
```

Interpret per `docs/audits/jump-stall-root-cause.md` (residual vs `presentUs`, `DP_HAPTIC`, `xiGet*`, `DP_SWAPWAIT`).

## Pause menu

Pause GFx / native overlay research is **closed** (NO-GO @ `8b5643a`). In-game pause menu remains **visually broken**; do not mark fixed.
