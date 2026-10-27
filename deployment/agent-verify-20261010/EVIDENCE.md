# Agent verify 2026-10-10 (Steam + dvr-xrsim)

## Build under test

- DLL SHA-256: `DB689BFB0DCFAC39A19B2C021BFB342A9EB82D15AA06F8F99DDBB093487DDD74`
- INI: `ShadowMode=3`, `GloveIsolate=2`, `GloveFaceFilter=0`

## Dishonored-style Steam harness (recommended)

```powershell
cd C:\Users\Trilex\vr-mods\SingularityVR-production
.\tools\run-singularity-dish-xrsim.ps1
```

- Fresh `%LOCALAPPDATA%\SingularityVR\xrsim-steam-<stamp>\` (no stale `state.json`)
- Restart Steam → `steam -applaunch 42670` so `XR_RUNTIME_JSON` reaches the game (jak `xrsim-launch.ps1 -ViaSteam` w Dishonored)
- Canonical `session_2026-10-09_14-18-07.vrrec` przez `xrsim_replay_vr_session.py` (bez `dinput8` / bez starego `steam-xrsim-singularity` scratch dir)
- Klatry: `xrsim-shot.ps1` (compositor L/R), nie backbuffer BMP

Latest good run: `evidence/dish-style-20261010-174242/` — vrrec **PASS** 1420 batches, gameplay PNGs, log `ShadowMode=3` + `shadow-sample … mono`, `GloveIsolation: suppressed … verts=390`.

## Older automated runs

| Run | Report |
|-----|--------|
| 17:22–17:25 | `reports/steam-xrsim-run-20261010-172251.json` — xrsim ready, log collected |
| 17:33–17:36 | `reports/steam-xrsim-run-20261010-173343.json` — replay failed (stale xrsim `cmdSeq`) |
| 17:43–17:44 | `evidence/dish-style-20261010-174242/` — Dish harness + vrrec (see above) |

Log signals (`deployment/agent-test-bridge/20261010-172554/view_matrix.log`):

- `shadow sampling: ShadowMode=3`
- `shadow-sample: … mono` (sampling pass not stereo-split)
- `GloveIsolation: suppressed detached sleeve/finger section verts=390 first=14490 tris=674`
- **No** `ShadowMode3 census: light-space slots mask=0x…` in this session → register protection likely inactive
- Warning: windowed 1280×720 → `TRUE STEREO is ON but NOTHING was split` (xrsim route ≠ headset backbuffer)

## Visual verdict (headset still required)

- Stare FAIL: `evidence/FAIL-gameplay-sbs.png` (stary scratch xrsim, replay bez PID guard).
- Nowe Steam+dish: `evidence/dish-style-20261010-174242/steam-gameplay-hands-shadows_*.png` — prawdziwy gameplay po vrrec; na klatce widać scenę i broń, ale **nie** da się z compositora 1:1 potwierdzić „naprawione rękawiczki / brak kwadratowych cieni” jak na PSVR2.

**Result: integration PASS, visual headset PASS pending** — logi pod build `DB689…` wyglądają sensownie; Ty na hełmie musisz ocenić dłoni i cienie.

## What you can try on the headset (isolation)

1. `dinput8.dll` to tylko injector tekstur — **nie** steruje cieniami ani mesh rąk; wyłączenie rzadko zmienia te artefakty.
2. Confirm `Binaries\SingularityVR.ini`: `ShadowMode=3`, `GloveFaceFilter=0`, `GloveIsolate=2`.
3. Confirm DLL hash matches `DB689…` above.

Physical PSVR2 acceptance is still required; xrsim proves integration only, not headset comfort.
