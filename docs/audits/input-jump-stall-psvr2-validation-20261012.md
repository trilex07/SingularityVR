# PSVR2 — Jump stall baseline validation (2026-10-12)

**Build:** `726e1a7` lineage on `perf/input-jump-stall-diagnostics` (diagnostic DLL + `[Diagnostic] Enabled=1`, `[Render] Debug=1`).  
**Production restore after session:** DLL SHA **`2FE5011C20DFEAAEDFFFFE58CD39E92A9570867F344649B00C0038AD97B39A47`**.

## Mode equivalence (read before comparing)

| F11 mode | Jump reaches game? | Use for stall A/B |
|---|---|---|
| **NORMAL** | Yes (`published=1` on Jump in log) | Yes |
| **HAPTICS_OFF** | Yes | Yes — isolates haptics only |
| **XINPUT_OFF** | **No** — mod clears pad (`feedGame=false`) | **Non-equivalent** — do **not** treat as “XInput caused stall” |
| **XINPUT_OFF+HAPTICS_OFF** | **No** | Non-equivalent |
| **BUTTONS_OFF** | **No** — face buttons stripped (`published=0` on Jump) | Non-equivalent for Jump execution |

Only **NORMAL** and **HAPTICS_OFF** prove Jump gameplay while varying haptics.

## False-pass modes

- **You jumped but log shows `published=0`** → mode blocked Jump; that segment is invalid for Jump stall comparison.
- **No `DIAG MODE` line after F11** → `Debug=1` missing or desktop keyboard not reaching game; fix INI and retry.
- **Attributing stall to XINPUT_OFF** when character never left the ground — that mode is intentionally input-starved.

## Headset script (~12–15 min)

Same combat-safe spot you used before (flat ground, no cutscene). **≥30 s per mode**, **≥5 Jump presses** per mode.

1. Put on PSVR2; confirm SteamVR tracking.
2. Launch **Singularity** from Steam (not xrsim). Load gameplay save.
3. **NORMAL** — press **Right A (Jump)** five times with ~2 s between. Note any ~0.5 s hitch after Jump.
4. Press **F11** once → HUD/log should show **HAPTICS_OFF** (keyboard at desk, or pass-through if available).
5. **HAPTICS_OFF** — five Jumps again; confirm character still jumps.
6. **F11** → **XINPUT_OFF** — press Jump five times. **Expected: no in-game Jump.** Still note any hitch (optional; non-equivalent).
7. **F11** through to **BUTTONS_OFF** — press Jump; **expected: no Jump action** (`published=0` in log).
8. **F11** back to **NORMAL**; one more Jump to confirm restore.
9. Quit to menu / exit cleanly (hold pause path you normally use).

## After quit

Tell MAIN (or confirm yourself): new file  
`%LOCALAPPDATA%\SingularityVR\input_diagnostic_YYYYMMDD_HHMMSS.log`  
timestamp **after** this deploy.

MAIN runs:

```powershell
python tools/analyze-input-diagnostic-log.py "$env:LOCALAPPDATA\SingularityVR\input_diagnostic_<stamp>.log"
```

Copy the log into `deployment/evidence/jump-baseline-20261012/` for preservation (optional MAIN step).
