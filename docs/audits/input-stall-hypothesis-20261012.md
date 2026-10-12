# Input stall hypotheses — 2026-10-12 (measured)

## What `residual` is

In `diagnostic_trace.h`:

- `frameUs` = QPC from the **previous** `DiagStart` (start of `Hook_Present`) to **this** `DiagStart`.
- `presentUs` = QPC from **this** `DiagStart` to `DiagFinish` (time inside the current Present hook).
- Logged `residual` = `frameUs - presentUs`.

That is the Present-to-Present gap **minus the current hook body**. It is **not** a CPU profile. It includes UE3 game-thread work, sleeps, locks, scheduler gaps, and anything else between Present calls. OpenXR phase buckets, haptics, XInput, and swapchain waits are measured **inside** the hook and were negligible on the ~495 ms frame.

`threadCpuUs` (added this session) is `GetThreadTimes` user+kernel delta on the Present thread between `DiagStart` calls. A future `LONGSTALL` line compares `thread_cpu_us` to `residual_us`:

- CPU ≈ residual → the Present thread was executing.
- CPU ≪ residual → the thread was waiting, sleeping, or not scheduled.

## Button mapping (do not call 0x2000 a gunshot)

| Mask | Mod meaning |
|---|---|
| `0x1000` XI_A | Jump (`right a/click`) |
| `0x0001` DPAD_UP | Health (`right b/click`, `kCircleHealthMask`) |
| `0x2000` XI_B | Crouch (`kStickDownCrouchMask`, stick down) |
| triggers | Separate `trig=` bytes, not the button mask |

`input_diagnostic_20261012_143903.log` frame 13515 rising `0x2001` is **Health + Crouch bits together**, `trig=0/0`. It is not evidence of a trigger shot.

## All logs ≥250 ms (46 files)

Tight delays (2–4 frames) occur after **Jump (A)**, **Health (DPAD_UP)**, **Crouch (B)**, **X**, and sometimes **no rising edge**. The button-edge reporter also takes the **max frame in the next 90 frames**, so a stall is attributed to every earlier edge in that window. Counts of “Jump stalls” are therefore inflated.

`BUTTONS_OFF` still shows ~490 ms residual stalls while Jump is `published=0` (2026-10-06). That comparison is **non-equivalent** for gameplay, but it shows the hitch can occur without the game receiving the button.

## Ranking

1. **Inter-Present gap outside `Hook_Present`** — measured (`residual` ≈ frame time). Not haptics, not `DP_SWAPWAIT`.
2. **Not a single button handler** — same ~450–500 ms class appears after several different edges; 90-frame window contaminates attribution.
3. **CPU vs wait — unknown on existing logs.** `LONGSTALL` + `thread_cpu_us` is the next measurement. No code fix until that split is observed.
4. **Haptics / compositor** — rejected on the stall frames already captured (`haptic=0`, `swapwait≈0`).
