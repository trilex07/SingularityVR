# Input Diagnostic Log Analysis

Source: `C:\Users\Trilex\AppData\Local\SingularityVR\input_diagnostic_20261006_175604.log`

This report is generated from an existing `input_diagnostic_*.log`. It does not run Steam or claim a root cause without measured evidence.

## Diagnostic mode changes

| line | mode | action polling |
|---:|---|---|
| 1 | NORMAL | ON |
| 90 | HAPTICS_OFF | ON |
| 112 | XINPUT_OFF | ON |
| 113 | XINPUT_OFF+HAPTICS_OFF | ON |
| 114 | BUTTONS_OFF | ON |
| 255 | NORMAL | ON |
| 256 | HAPTICS_OFF | ON |
| 257 | XINPUT_OFF | ON |
| 462 | XINPUT_OFF+HAPTICS_OFF | ON |
| 526 | BUTTONS_OFF | ON |
| 528 | NORMAL | ON |
| 529 | HAPTICS_OFF | ON |

## Rolling InputTiming summaries

| mode | frames | avg ms | p95 ms | max ms | present avg ms | XR avg ms | bridge avg ms | XInput avg ms | lifecycle violations |
|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|
| NORMAL | 240 | 37.81 | 41.63 | 698.63 | 7.29 | 1.30 | 0.12 | 0.00 | 0 |
| NORMAL | 240 | 14.04 | 14.81 | 475.45 | 11.10 | 7.18 | 0.14 | 0.00 | 0 |
| NORMAL | 240 | 22.90 | 62.31 | 314.56 | 11.02 | 4.47 | 0.13 | 0.00 | 0 |
| NORMAL | 240 | 15.82 | 42.11 | 167.44 | 7.94 | 4.11 | 0.37 | 0.00 | 0 |
| NORMAL | 240 | 12.04 | 14.76 | 101.70 | 7.06 | 4.82 | 0.11 | 0.00 | 0 |
| NORMAL | 240 | 12.28 | 15.05 | 142.43 | 7.33 | 4.96 | 0.10 | 0.00 | 0 |
| NORMAL | 240 | 11.98 | 15.02 | 98.99 | 7.19 | 4.86 | 0.12 | 0.00 | 0 |
| NORMAL | 240 | 13.01 | 15.66 | 98.40 | 4.70 | 2.50 | 0.11 | 0.00 | 0 |
| NORMAL | 240 | 18.18 | 16.26 | 505.06 | 6.55 | 4.05 | 0.09 | 0.00 | 0 |
| NORMAL | 240 | 18.09 | 15.28 | 474.94 | 6.48 | 4.16 | 0.10 | 0.00 | 0 |
| NORMAL | 240 | 12.13 | 15.07 | 134.16 | 9.24 | 6.76 | 0.11 | 0.00 | 0 |
| NORMAL | 240 | 11.95 | 12.09 | 96.95 | 7.77 | 5.75 | 0.10 | 0.00 | 0 |
| NORMAL | 240 | 11.98 | 12.05 | 91.92 | 7.77 | 5.79 | 0.12 | 0.00 | 0 |
| NORMAL | 240 | 11.96 | 12.39 | 90.58 | 7.66 | 5.72 | 0.10 | 0.00 | 0 |
| NORMAL | 240 | 12.08 | 12.34 | 124.32 | 7.76 | 5.63 | 0.12 | 0.00 | 0 |
| NORMAL | 240 | 11.90 | 12.40 | 91.72 | 7.57 | 5.63 | 0.12 | 0.00 | 0 |
| NORMAL | 240 | 11.91 | 12.94 | 92.73 | 7.57 | 5.62 | 0.11 | 0.00 | 0 |
| NORMAL | 240 | 11.86 | 12.25 | 86.98 | 7.64 | 5.71 | 0.12 | 0.00 | 0 |
| NORMAL | 240 | 11.91 | 12.10 | 99.13 | 7.75 | 5.78 | 0.12 | 0.00 | 0 |
| NORMAL | 240 | 12.10 | 12.54 | 121.39 | 7.84 | 5.67 | 0.12 | 0.00 | 0 |
| HAPTICS_OFF | 240 | 11.91 | 12.59 | 90.90 | 7.61 | 5.66 | 0.10 | 0.00 | 0 |
| HAPTICS_OFF | 240 | 23.99 | 21.49 | 487.25 | 5.44 | 3.07 | 0.10 | 0.00 | 0 |
| HAPTICS_OFF | 240 | 11.92 | 12.04 | 90.15 | 10.19 | 8.28 | 0.14 | 0.00 | 0 |
| HAPTICS_OFF | 240 | 12.02 | 14.94 | 91.95 | 7.46 | 5.37 | 0.11 | 0.00 | 0 |
| BUTTONS_OFF | 240 | 12.03 | 12.31 | 128.62 | 7.59 | 5.55 | 0.11 | 0.00 | 0 |
| BUTTONS_OFF | 240 | 12.01 | 12.62 | 92.89 | 7.47 | 5.54 | 0.11 | 0.00 | 0 |
| BUTTONS_OFF | 240 | 22.08 | 14.82 | 931.22 | 4.90 | 2.88 | 0.11 | 0.00 | 0 |
| BUTTONS_OFF | 240 | 11.91 | 12.58 | 91.81 | 8.29 | 6.38 | 0.11 | 0.00 | 0 |
| XINPUT_OFF | 240 | 11.87 | 12.37 | 85.93 | 7.87 | 5.94 | 0.11 | 0.00 | 0 |
| XINPUT_OFF | 240 | 12.05 | 12.83 | 124.81 | 7.46 | 5.36 | 0.11 | 0.00 | 0 |
| XINPUT_OFF | 240 | 11.95 | 13.03 | 94.21 | 7.33 | 5.39 | 0.11 | 0.00 | 0 |
| XINPUT_OFF | 240 | 11.96 | 14.94 | 90.67 | 7.39 | 5.44 | 0.11 | 0.00 | 0 |
| XINPUT_OFF | 240 | 11.90 | 12.08 | 88.29 | 7.44 | 5.55 | 0.12 | 0.00 | 0 |
| XINPUT_OFF+HAPTICS_OFF | 240 | 12.00 | 12.05 | 120.28 | 7.68 | 5.64 | 0.11 | 0.00 | 0 |
| XINPUT_OFF+HAPTICS_OFF | 240 | 11.96 | 12.87 | 95.68 | 7.35 | 5.42 | 0.11 | 0.00 | 0 |
| BUTTONS_OFF | 240 | 11.90 | 12.58 | 89.74 | 7.34 | 5.40 | 0.12 | 0.00 | 0 |
| HAPTICS_OFF | 240 | 17.76 | 14.20 | 482.38 | 6.37 | 4.32 | 0.12 | 0.00 | 0 |
| HAPTICS_OFF | 240 | 18.62 | 26.83 | 476.60 | 6.31 | 3.68 | 0.46 | 0.00 | 0 |
| HAPTICS_OFF | 240 | 12.04 | 12.53 | 123.19 | 7.46 | 5.37 | 0.11 | 0.00 | 0 |

## Jump-edge events

| edge frame | mode | edge ms | +1 ms | +2 ms | max ms | max frame | delay to max | verdict |
|---:|---|---:|---:|---:|---:|---:|---:|---|
| 161 | NORMAL | 37.94 | 33.23 | 34.80 | 698.63 | 211 | 50 frames | STALL |
| 186 | NORMAL | 36.48 | 37.45 | 36.18 | 698.63 | 211 | 25 frames | STALL |
| 206 | NORMAL | 35.30 | 67.40 | 25.34 | 698.63 | 211 | 5 frames | STALL |
| 475 | NORMAL | 10.78 | 8.24 | 475.45 | 475.45 | 477 | 2 frames | STALL |
| 657 | NORMAL | 253.32 | 2.37 | 9.37 | 314.56 | 672 | 15 frames | STALL |
| 2224 | NORMAL | 5.98 | 10.93 | 463.45 | 505.06 | 2227 | 3 frames | STALL |
| 2285 | NORMAL | 8.44 | 10.39 | 470.69 | 474.94 | 2288 | 3 frames | STALL |
| 5292 | HAPTICS_OFF | 11.61 | 10.35 | 478.99 | 487.25 | 5358 | 66 frames | STALL |
| 5355 | HAPTICS_OFF | 11.08 | 10.56 | 472.30 | 487.25 | 5358 | 3 frames | STALL |
| 6308 | BUTTONS_OFF | 11.54 | 10.71 | 11.25 | 92.89 | 6361 | 53 frames | STALL |
| 6344 | BUTTONS_OFF | 10.62 | 11.42 | 11.08 | 92.89 | 6361 | 17 frames | STALL |
| 6427 | BUTTONS_OFF | 11.01 | 10.83 | 12.14 | 490.38 | 6513 | 86 frames | STALL |
| 6450 | BUTTONS_OFF | 12.62 | 9.02 | 12.20 | 490.38 | 6513 | 63 frames | STALL |
| 6480 | BUTTONS_OFF | 11.28 | 91.50 | 5.35 | 931.22 | 6552 | 72 frames | STALL |
| 6488 | BUTTONS_OFF | 12.30 | 10.76 | 9.69 | 931.22 | 6552 | 64 frames | STALL |
| 6580 | BUTTONS_OFF | 13.34 | 13.39 | 12.71 | 90.56 | 6601 | 21 frames | ok |
| 6598 | BUTTONS_OFF | 12.32 | 12.42 | 12.35 | 90.56 | 6601 | 3 frames | STALL |
| 6606 | BUTTONS_OFF | 11.45 | 11.77 | 12.03 | 26.32 | 6663 | 57 frames | elevated |
| 6622 | BUTTONS_OFF | 12.10 | 12.23 | 11.55 | 26.32 | 6663 | 41 frames | elevated |
| 6642 | BUTTONS_OFF | 11.74 | 11.01 | 10.84 | 87.72 | 6721 | 79 frames | STALL |
| 6659 | BUTTONS_OFF | 10.00 | 10.90 | 10.79 | 87.72 | 6721 | 62 frames | STALL |
| 6672 | BUTTONS_OFF | 11.25 | 11.67 | 11.07 | 87.72 | 6721 | 49 frames | STALL |
| 6690 | BUTTONS_OFF | 11.31 | 11.28 | 10.79 | 87.72 | 6721 | 31 frames | STALL |
| 7680 | XINPUT_OFF | 10.96 | 88.15 | 5.96 | 88.15 | 7681 | 1 frames | STALL |
| 7709 | XINPUT_OFF | 11.27 | 10.85 | 11.03 | 27.24 | 7743 | 34 frames | elevated |
| 7726 | XINPUT_OFF | 11.47 | 11.62 | 11.17 | 89.50 | 7801 | 75 frames | STALL |
| 7963 | XINPUT_OFF | 11.46 | 11.64 | 10.92 | 97.79 | 8041 | 78 frames | STALL |
| 8784 | XINPUT_OFF+HAPTICS_OFF | 11.19 | 11.37 | 11.38 | 27.14 | 8823 | 39 frames | elevated |
| 9185 | HAPTICS_OFF | 11.51 | 10.56 | 462.15 | 482.38 | 9189 | 4 frames | STALL |

## Phase breakdown for worst Jump-associated stall per mode

### Mode: BUTTONS_OFF, edge frame 6480, max frame 6552

- Delay: **72 frames** after Jump
- Max frame time: **931.22 ms**
- presentUs: 4.72 ms
- residual: 926.50 ms
- Largest phase: **end** = 0.27 ms

| phase | ┬Ás | ms |
|---|---:|---:|
| wait | 20 | 0.02 |
| begin | 18 | 0.02 |
| sync | 112 | 0.11 |
| actions | 21 | 0.02 |
| poses | 24 | 0.02 |
| end | 266 | 0.27 |
| bridge | 80 | 0.08 |
| acquire | 5 | 0.01 |
| swapwait | 2 | 0.00 |
| release | 66 | 0.07 |
| haptic | 0 | 0.00 |

- XInput: get calls=2, get total ┬Ás=1, get max ┬Ás=1
- Haptics: calls=0, apply=0, stop=0, api ┬Ás=0, ampMilli=0

- **Attribution hint:** large residual Ôćĺ stall is mostly **outside Hook_Present** (engine/game thread).

### Mode: NORMAL, edge frame 161, max frame 211

- Delay: **50 frames** after Jump
- Max frame time: **698.63 ms**
- presentUs: 31.57 ms
- residual: 667.06 ms
- Largest phase: **bridge** = 0.46 ms

| phase | ┬Ás | ms |
|---|---:|---:|
| wait | 23 | 0.02 |
| begin | 23 | 0.02 |
| sync | 127 | 0.13 |
| actions | 26 | 0.03 |
| poses | 27 | 0.03 |
| end | 257 | 0.26 |
| bridge | 460 | 0.46 |
| acquire | 6 | 0.01 |
| swapwait | 2 | 0.00 |
| release | 80 | 0.08 |
| haptic | 0 | 0.00 |

- XInput: get calls=14, get total ┬Ás=1, get max ┬Ás=1
- Haptics: calls=0, apply=0, stop=0, api ┬Ás=0, ampMilli=0

- **Attribution hint:** large residual Ôćĺ stall is mostly **outside Hook_Present** (engine/game thread).

### Mode: HAPTICS_OFF, edge frame 5292, max frame 5358

- Delay: **66 frames** after Jump
- Max frame time: **487.25 ms**
- presentUs: 1.63 ms
- residual: 485.62 ms
- Largest phase: **end** = 0.32 ms

| phase | ┬Ás | ms |
|---|---:|---:|
| wait | 22 | 0.02 |
| begin | 19 | 0.02 |
| sync | 299 | 0.30 |
| actions | 33 | 0.03 |
| poses | 92 | 0.09 |
| end | 315 | 0.32 |
| bridge | 107 | 0.11 |
| acquire | 2 | 0.00 |
| swapwait | 2 | 0.00 |
| release | 73 | 0.07 |
| haptic | 0 | 0.00 |

- XInput: get calls=1, get total ┬Ás=1, get max ┬Ás=1
- Haptics: calls=0, apply=0, stop=0, api ┬Ás=0, ampMilli=0

- **Attribution hint:** large residual Ôćĺ stall is mostly **outside Hook_Present** (engine/game thread).

### Mode: XINPUT_OFF, edge frame 7963, max frame 8041

- Delay: **78 frames** after Jump
- Max frame time: **97.79 ms**
- presentUs: 1.16 ms
- residual: 96.63 ms
- Largest phase: **end** = 0.24 ms

| phase | ┬Ás | ms |
|---|---:|---:|
| wait | 17 | 0.02 |
| begin | 14 | 0.01 |
| sync | 120 | 0.12 |
| actions | 22 | 0.02 |
| poses | 22 | 0.02 |
| end | 237 | 0.24 |
| bridge | 69 | 0.07 |
| acquire | 1 | 0.00 |
| swapwait | 1 | 0.00 |
| release | 51 | 0.05 |
| haptic | 0 | 0.00 |

- XInput: get calls=1, get total ┬Ás=0, get max ┬Ás=0
- Haptics: calls=0, apply=0, stop=0, api ┬Ás=0, ampMilli=0

- **Attribution hint:** large residual Ôćĺ stall is mostly **outside Hook_Present** (engine/game thread).

### Mode: XINPUT_OFF+HAPTICS_OFF, edge frame 8784, max frame 8823

- Delay: **39 frames** after Jump
- Max frame time: **27.14 ms**
- presentUs: 1.79 ms
- residual: 25.35 ms
- Largest phase: **wait** = 0.69 ms

| phase | ┬Ás | ms |
|---|---:|---:|
| wait | 693 | 0.69 |
| begin | 11 | 0.01 |
| sync | 106 | 0.11 |
| actions | 21 | 0.02 |
| poses | 21 | 0.02 |
| end | 189 | 0.19 |
| bridge | 113 | 0.11 |
| acquire | 3 | 0.00 |
| swapwait | 2 | 0.00 |
| release | 50 | 0.05 |
| haptic | 0 | 0.00 |

- XInput: get calls=1, get total ┬Ás=0, get max ┬Ás=0
- Haptics: calls=0, apply=0, stop=0, api ┬Ás=0, ampMilli=0

- **Attribution hint:** large residual Ôćĺ stall is mostly **outside Hook_Present** (engine/game thread).

## Mode comparison summary

| mode | Jump edges | stalls (>25 ms) | worst stall ms |
|---|---:|---:|---:|
| BUTTONS_OFF | 14 | 14 | 931.22 |
| HAPTICS_OFF | 3 | 3 | 487.25 |
| NORMAL | 7 | 7 | 698.63 |
| XINPUT_OFF | 4 | 4 | 97.79 |
| XINPUT_OFF+HAPTICS_OFF | 1 | 1 | 27.14 |

- If the worst stall is similar across `NORMAL`, `HAPTICS_OFF`, and `XINPUT_OFF`, the cause is likely engine-side (large residual).
- If the stall disappears only in `HAPTICS_OFF`, suspect runtime haptics.
- If the stall disappears only in `XINPUT_OFF`, suspect XInput poll amplification.
- If the stall is present even in `BUTTONS_OFF`, it is not Jump button processing per se.

## How to interpret this report

1. Look for a Jump edge whose `max_us` is ~475 ms.
2. Read the `delay to max` column: how many frames after Jump did the stall land?
3. In the phase breakdown, compare `presentUs` vs `residual`:
   - `residual` dominates Ôćĺ stall is outside the mod's Present hook (engine/game thread).
   - `presentUs` dominates Ôćĺ stall is inside Present; inspect the largest phase.
4. Compare the same Jump scenario across diagnostic modes to isolate input/haptic causality.

