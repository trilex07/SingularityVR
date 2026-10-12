# Performance stall investigation — conclusion (2026-10-12)

**Branch:** `perf/input-jump-stall-diagnostics` @ `4e58bdb`  
**Production baseline (unchanged on Steam):** `726e1a7` / DLL `2FE5011C20DFEAAEDFFFFE58CD39E92A9570867F344649B00C0038AD97B39A47`  
**Scope:** ~450–500 ms Present-to-Present spikes only. Hands, grip, pause renderer, shadows, cutscenes — **out of scope and frozen.**

## What was implemented on `4e58bdb` (vs `726e1a7`)

| Change | Purpose |
|---|---|
| `diagnostic_trace.h`: `threadCpuUs` via `GetThreadTimes` on Present thread | Compare CPU time to `residual` on long frames |
| `diagnostic_trace.h`: rate-capped `LONGSTALL` lines (`residual > 250 ms`, max 48) | One log line per stall with phase buckets + up to 8 prior edges (no hot-path I/O) |
| `d3d9.cpp`: melee-window log throttle (1 s) | Reduce combat log spam only; **no** haptic 15→60 |
| `tools/analyze-input-diagnostic-log.py`, `tools/scan-long-frames.py` | Offline analysis of existing logs |
| Audits + runbooks under `docs/audits/` | Evidence and capture procedure |

No rendering, input, holster, grip, IK, localization, or pause code paths were modified beyond the melee log throttle.

## Can current diagnostics separate CPU work from waits?

| Instrument | What it measures | Limit |
|---|---|---|
| `frameUs` | QPC gap between `DiagStart` calls (Present-to-Present) | Includes everything between hooks |
| `presentUs` | Time inside current `Hook_Present` | Mod + XR phases inside hook |
| `residual = frameUs - presentUs` | **Unattributed gap** | Not game-thread CPU; not GPU; not a profile |
| Phase buckets (`DP_*`, haptic, xiGet) | Work **inside** Present hook | On ~495 ms stalls these are ~0–4 ms |
| **`thread_cpu_us` (`4e58bdb` only)** | User+kernel CPU on **Present thread** between frames | If ≪ `residual_us` → thread was waiting/sleeping/not scheduled; if ≈ `residual` → Present thread was executing. **Does not** attribute UE3 game thread if stall runs elsewhere |
| Existing logs (46 files) | Full button-edge + phase data | **No** `LONGSTALL` / `thread_cpu_us` — captured before `4e58bdb` |

**Answer:** Pre-`4e58bdb` logs **cannot** distinguish execution vs wait. **`4e58bdb` can distinguish Present-thread CPU vs wait only**, after one diagnostic deploy and a captured stall. **UE3 game-thread work outside Present** still needs a separate probe (e.g. ETW/WPR on `Singularity.exe`, or a future bounded game-thread timestamp if a safe hook exists).

## Measured findings (evidence-backed)

1. **~495 ms class is real** on baseline lineage (`input_diagnostic_20261012_143903.log`: frame 13517, `residual≈492 ms`, `present≈3.6 ms`, haptic/swapwait negligible).
2. **Not explained by mod haptics or compositor swap wait** on captured stall frames (`haptic=0`, `swapwait≈0`).
3. **Not uniquely Jump (`XI_A`)** in the latest capture; that session’s largest stall followed **`0x2001`** (Health `DPAD_UP` + Crouch `XI_B` per `input_remap.h`), triggers 0. Jump edges in the same log peaked ~25–90 ms.
4. **Cross-log scan (`scan-long-frames.py`):** ≥400 ms stalls appear after Jump, Health, Crouch, X, and with no rising edge; the 90-frame edge reporter **over-attributes** one stall to many buttons.
5. **`HAPTICS_OFF`** did not remove ~475 ms stalls in Oct-6 matrix log → haptic refresh changes are **not** a valid fix target (15→60 correctly rejected).
6. **No mod-side root cause proven** → **no performance fix shipped** to production.

## Runtime testing blocker (2026-10-12)

Hands-candidate smoke launch: process attached, **`xr-heartbeat: session=0`** — OpenXR/SteamVR session did not start. **No valid in-headset or stereo performance capture** from that run. This does not invalidate offline log analysis; it blocks **new** `LONGSTALL` evidence until a normal Steam+SteamVR gameplay session with `[Diagnostic] Enabled=1`.

## Verdict

| Item | Status |
|---|---|
| Root cause of ~495 ms stall | **Unresolved** — bucket is **outside measured `Hook_Present` (`residual`)**, subsystem inside UE3/wait path **not identified** |
| Mod fix | **None** (no demonstrated mod-side defect) |
| Production DLL | **Unchanged** — `2FE5011C…` |

## Single next measurement (required before any fix)

1. Deploy diagnostic DLL built from **`4e58bdb`** only (`deployment/candidate-longstall-20261012/`, SHA in `sha256.txt`), INI `[Diagnostic] Enabled=1`, `[Render] Debug=1`, **rollback after one session**.
2. Reproduce **one** ≥250 ms stall in gameplay (any input; note what you did).
3. In `input_diagnostic_*.log`, find **`LONGSTALL`** and compare **`thread_cpu_us`** vs **`residual_us`**:
   - **CPU ≪ residual** → prioritize wait/sleep/sync/scheduling (Present thread idle during gap).
   - **CPU ≈ residual** → Present thread was busy; still need ETW to see *what* (UE3 vs mod outside phased buckets).
4. Optional parallel: **WPR CPU sample** on `Singularity.exe` for the same window (no repo code change).

Do **not** implement speculative optimizations (especially haptic 15→60) until step 3 (or ETW) implicates a subsystem.

## Frozen / inactive (do not touch during performance work)

- `feature/hands-support-grip` @ `a18ba1e` — **inactive, do not deploy or merge**
- `feature/left-hand-ik-continuity` — **do not activate**
- `perf/input-jump-stall-diagnostics` — investigation branch; **not merged to production**
