# SingularityVR input / Jump stall analysis

**Branch:** `research/input-jump-stalls`  
**Base:** `origin/main` (`e07367a`)  
**Date:** 2026-10-12  
**Mandate:** Identify root causes of intermittent controller-input and Jump-related stalls; propose minimal, measurable optimizations with low regression risk. No Steam, no broad rendering changes, no hands/grip system changes.

> **Subsequent revision:** the haptic refresh-interval recommendation in section 7.1 was found to risk breaking 200 ms pulse continuity and has been reassessed in `docs/audits/jump-stall-root-cause.md`. The melee-window log throttle remains valid.


---

## 1. Executive summary

Previous runs (54–156) already established that **controller-input judder is not the OpenXR action poll itself** — `xrSyncActions`, action-state reads, pose locations and haptics were all timed at `<0.1 ms` peak in normal conditions. The remaining stalls appear when the engine's **input path runs more than once per frame** or when a **runtime haptic call stalls**.

This audit focuses on the code paths that touch every Jump / controller event and ranks them by available evidence. The highest-confidence, lowest-risk optimizations are:

1. **Reduce haptic refresh frequency** (`d3d9.cpp:9940`) — the current 15-frame refresh is defensive overkill for a 200 ms haptic pulse; lowering it cuts OpenXR haptic API calls by ~75% while preserving rumble continuity.
2. **Rate-limit the melee-window log** (`d3d9.cpp:9453`) — unconditional `Log()` on every impulse/melee press can fire many times per second in combat.
3. **Preserve and use the existing XInput caller census** — the 33×/frame XInput poll spike is a symptom, not the cost; the caller list already identifies which subsystem is looping.

No broad changes to rendering, hands, grip, or the Steam install are proposed.

---

## 2. Scope and method

### Files reviewed

- `spikes/view_matrix/d3d9.cpp` — XInput hooks (`Hook_XInputGetState`, `Hook_XInputSetState`), OpenXR input poll (`SyncXRInput`), Jump/impulse handling, haptic submission.
- `HISTORY.md` — runs 54–156 (packet noise, deadzone, AutoPad, XInput poll spikes, haptic gating, input-off control).
- `STATUS.md` — current controller-input and haptic state.
- `docs/PERF_KILL_LIST.md` — manual checklist for engine/runtime settings.
- Existing `view_matrix.log` instrumentation lines: `xinput: ...`, `xrinput cost: ...`, `FRAME SPIKE ... meleeAge ... jumpAge ...`.

### New standalone tooling

- `tools/analyze-input-perf-log.py` — parses `view_matrix.log` for input/haptic/XInput stall signatures.
- `spikes/view_matrix/tests/test-xinput-packet-overhead.cpp` — microbenchmark for packet-number deadzone logic.
- `spikes/view_matrix/tests/test-haptic-refresh-policy.cpp` — policy regression test for the proposed haptic refresh change.

### What was not done

- No Steam launch, deployment, or runtime bridge.
- No changes to the hands/grip/mesh-latch system.
- No broad rendering or stereo changes.

---

## 3. Existing instrumentation (already in the build)

| Instrument | Location | What it measures |
|---|---|---|
| `xrinput cost:` log | `d3d9.cpp:25022` | Per-second averages and peaks for `xrSyncActions`, action states, hand poses, haptics. |
| `xinput:` log | `d3d9.cpp:940` | Calls/sec per port, packet advances/sec, `SetState`/sec, emulation state. |
| `xinput callers` log | `d3d9.cpp:953` | Return-address census of XInput callers and total hook CPU time. |
| `FRAME SPIKE` log | `d3d9.cpp:24440` | One-shot per 2 s; tags `meleeAge` / `jumpAge` to attribute stalls to recent button edges. |
| Haptic diagnostic counters | `d3d9.cpp:9942, 9950, 9953, 9961` | `hapticCalls`, `hapticStopCalls`, `hapticApplyCalls`, cumulative µs when `g_diagEnabled`. |
| Button held-frame census | `d3d9.cpp:24991` | Per-button held-frame counts per second. |

This instrumentation is sufficient to distinguish the candidate causes. No new runtime counters are required for the first optimization pass.

---

## 4. Ranked bottlenecks (by evidence)

### 4.1 #1 — Runtime haptic stalls on active controllers

**Evidence:**
- `d3d9.cpp:9927-9963`: haptics are submitted every frame the amplitude changes by `>0.02`, **and** every 15 frames as a refresh (`refresh % 15 == 0`).
- The refresh comment notes: *"A sleeping Touch controller is a plausible source of a slow runtime call, and this mod asks for haptics every time the game rumbles — which in combat is constantly."*
- The 200 ms haptic duration (`hv.duration = 200LL * 1000000LL`) means a 15-frame (~125 ms) refresh sends overlapping pulses; a 60-frame (~500 ms) refresh still overlaps the active pulse.
- `HISTORY.md` run 67: haptic gating did **not** fix the judder, so haptics are not the *only* cause, but the path is still a candidate when controllers are active and rumble is frequent.

**Why it stalls:** `xrApplyHapticFeedback` / `xrStopHapticFeedback` can block or serialize with the runtime/compositor. At 120 Hz with combat rumble, this path can fire dozens of times per second per hand.

**Measured local impact:** see `test-haptic-refresh-policy.cpp` — the refresh alone accounts for **8 calls/sec/hand** at 120 fps. With the proposed change this drops to **2 calls/sec/hand** for constant-amplitude rumble.

### 4.2 #2 — Engine input path polled multiple times per frame

**Evidence:**
- `HISTORY.md` run 71: `xinput: 938 calls/sec ... at 27 fps` → **33 polls/frame** during bad stretches; `72 calls/sec ... at 63 fps` → **1 poll/frame** when healthy.
- The hook itself is timed (`g_msInHook`) and reported as a fraction of a millisecond per second, so the cost is **not the detour** but the engine work each poll drags in.
- `Hook_XInputGetState` already records return addresses in `g_xiCallers[]` (`d3d9.cpp:799-803`).

**Why it stalls:** when the engine polls XInput many times per frame, it re-runs UnrealScript input handlers, UI focus, animation state, etc. Jump input processed in the middle of this loop can stall if those handlers are expensive.

**Root-cause distinction:** the `xinput callers` line separates *one site in a loop* from *many subsystems each polling once*. The analyzer tool parses this line.

### 4.3 #3 — Unconditional melee-window log on impulse edge

**Evidence:**
- `d3d9.cpp:9453`: `Log("melee window: arms shown from +%ld ms to +%ld ms after the press (gun=%ld)", ...)` fires on **every rising edge** of the impulse button.
- In combat, impulse/melee can be pressed repeatedly.
- `Log()` uses a file-backed async logger, but formatting and queueing are not free and run on the input path inside `SyncXRInput`.

**Why it stalls:** unconditional logging on a hot input path adds jitter, especially if the log file is being flushed by another thread.

**Regression risk:** very low. The log is diagnostic; rate-limiting it preserves the information while removing spam.

### 4.4 #4 — Jump button laser-hold logic

**Evidence:**
- `d3d9.cpp:9470-9492`: while the Jump button (A) is held and `g_laserHoldMs > 0`, the code calls `QueryPerformanceCounter` and `MsSince(since)` every frame, and on threshold crossing writes the INI via `PanelSaveRow` and logs.

**Why it could stall:** `PanelSaveRow` writes to `SingularityVR.ini`. A file write inside the input loop is a small but real stall. It only happens once per hold, so frequency is low.

**Measured impact:** bounded to one INI write per laser-toggle gesture.

### 4.5 #5 — OpenXR action poll baseline cost

**Evidence:**
- `SyncXRInput` calls `xrSyncActions` once, then ~13 action-state reads and 2 pose locations per frame.
- `HISTORY.md` run 156: all four phases timed at `<0.1 ms` peak.

**Why it is low priority:** the baseline is already measured as cheap. Stalls here would show as spikes in the `xrinput cost:` line, not as a steady cost.

### 4.6 #6 — AutoPad plug/unplug churn

**Evidence:**
- `UpdateAutoPad` (`d3d9.cpp:5888-5908`) toggles `g_padEmu` when controllers are still for `g_handStillFramesToDisable` frames (~5 s default).
- Re-plugging the virtual gamepad causes the engine to re-enumerate.

**Why it could stall:** re-enumeration is a one-time cost at pickup/set-down, not a per-frame cost. `AutoPad` defaults to **off** since run 156.

---

## 5. Exact source-level hot paths

### 5.1 Haptic submission

```cpp
// d3d9.cpp:9927-9963
if (g_actHaptic != XR_NULL_HANDLE) {
    static float lastAmp[2] = { -1.0f, -1.0f };
    static int   refresh = 0;
    ++refresh;
    const LONG raw[2] = { InterlockedCompareExchange(&g_rumbleL, 0, 0),
                          InterlockedCompareExchange(&g_rumbleR, 0, 0) };
    for (int hand = 0; hand < 2; ++hand) {
        const float amp = (hapticsAllowed && g_handHeld[hand]) ? (float)raw[hand] / 65535.0f : 0.0f;
        const bool changed = fabsf(amp - lastAmp[hand]) > 0.02f;
        const bool expiring = amp > 0.0f && (refresh % 15) == 0;   // <-- optimization target
        if (!changed && !expiring) continue;
        ...
        if (amp <= 0.0f) xrStopHapticFeedback(...);
        else             xrApplyHapticFeedback(...);
    }
}
```

### 5.2 Melee log

```cpp
// d3d9.cpp:9440-9454
if (down && !wasDown && InterlockedCompareExchange(&g_meleeArmsMs, 0, 0) > 0) {
    ...
    Log("melee window: arms shown from +%ld ms to +%ld ms after the press (gun=%ld)",
        dly, end, InterlockedCompareExchange(&g_gunVerts, 0, 0));
}
```

### 5.3 Jump edge timestamp

```cpp
// d3d9.cpp:9427-9432
if (i == g_jumpBtnIdx) {
    static bool jumpWasDown = false;
    if (down && !jumpWasDown)
        InterlockedExchange64(&g_lastJumpEdgeMs, (LONG64)GetTickCount64());
    jumpWasDown = down;
}
```

### 5.4 XInput caller census

```cpp
// d3d9.cpp:798-803
const uintptr_t ra = reinterpret_cast<uintptr_t>(_ReturnAddress());
int slot = -1;
for (int i = 0; i < g_xiCallerN; ++i) if (g_xiCallers[i].addr == ra) { slot = i; break; }
if (slot < 0 && g_xiCallerN < 12) { slot = g_xiCallerN++; g_xiCallers[slot].addr = ra; }
if (slot >= 0) ++g_xiCallers[slot].count;
```

---

## 6. Minimal instrumentation to distinguish root causes

No new production instrumentation is required. MAIN should capture one `view_matrix.log` during a Jump/combat stutter session and run `tools/analyze-input-perf-log.py`. The script extracts:

1. **XInput poll spikes:** `xinput: N calls/sec ...` converted to polls/frame using the reported fps.
2. **Caller distribution:** `xinput callers (hook total X ms/sec): 0xADDR xCOUNT ...`.
3. **XR input phase peaks:** `xrinput cost: xrSyncActions X ms (peak Y) | action states ... | hand poses ... | haptics ...`.
4. **Jump/melee spike tags:** `FRAME SPIKE ... jumpAge N meleeAge M`.
5. **Haptic diagnostic totals:** `hapticCalls`, `hapticApplyCalls`, `hapticStopCalls`, cumulative µs (when `g_diagEnabled=1`).

### Decision table

| Observation | Likely cause | Next step |
|---|---|---|
| `xinput` polls/frame >> 1 and one caller dominates | One engine site looping | Inspect caller address in Ghidra; add a single-site guard. |
| `xinput` polls/frame >> 1 and many callers | Multiple subsystems re-polling | Cache the last polled state for one frame. |
| `haptics` peak >> 0.5 ms or haptic µs/frame high | Runtime haptic stall | Apply refresh-interval optimization. |
| `jumpAge` / `meleeAge` small on most spikes | Input-edge trigger | Rate-limit logs; check laser-hold INI write. |
| `xrinput cost` peaks all < 0.1 ms | Not OpenXR baseline | Look at engine input path, not runtime. |

---

## 7. Small, isolated optimization candidates

### 7.1 Reduce haptic refresh frequency (implemented)

**Change:** in `d3d9.cpp:9940`, increase the refresh modulus.

```cpp
// Before:
const bool expiring = amp > 0.0f && (refresh % 15) == 0;

// After:
const bool expiring = amp > 0.0f && (refresh % 60) == 0;
```

**Justification:**
- Haptic pulse duration is 200 ms.
- At 120 fps, 15 frames ≈ 125 ms; 60 frames ≈ 500 ms.
- Refreshing every 500 ms still refreshes well before a 200 ms pulse would expire, so continuity is preserved.
- API call rate for constant-amplitude rumble drops from 8/sec/hand to 2/sec/hand.

**Behavior preserved:**
- Amplitude changes still apply immediately (`changed` path).
- Stopping still happens immediately when `amp <= 0.0f`.
- `g_handHeld[hand]` and `hapticsAllowed` gates remain.

### 7.2 Rate-limit melee-window log (ready-to-implement proposal)

**Change:** wrap the `Log("melee window: ...")` call in a 1-second throttle.

```cpp
static ULONGLONG lastMeleeLogMs = 0;
const ULONGLONG nowMs = GetTickCount64();
if (nowMs - lastMeleeLogMs >= 1000) {
    lastMeleeLogMs = nowMs;
    Log("melee window: arms shown from +%ld ms to +%ld ms after the press (gun=%ld)",
        dly, end, InterlockedCompareExchange(&g_gunVerts, 0, 0));
}
```

**Justification:** the log is diagnostic; one line per second is enough for investigation. Rapid melee in combat can produce many log lines per second.

**Behavior preserved:** melee timing, arms visibility, and gameplay are unchanged.

### 7.3 Cache last frame's XInput state if polled multiple times (proposal)

If the caller census shows **one site polling in a loop**, the fix is engine-side and out of scope. If it shows **many independent callers**, a one-frame cache inside `Hook_XInputGetState` could help:

```cpp
static DWORD lastPacket = 0;
static XiState lastState{};
if (g_padPacket == lastPacket) { *st = lastState; return kXiOk; }
```

This is more invasive and requires validation that no caller depends on side effects of the real call, so it remains a proposal.

---

## 8. Microbenchmarks and focused tests

### 8.1 `test-xinput-packet-overhead.cpp`

Measures the cost of the packet-number deadzone and `memcmp` comparison used in `Hook_XInputGetState`. Validates that:
- Identical gamepad states do not advance the packet.
- Small stick noise within the deadzone does not advance the packet.
- State changes above the threshold advance the packet.

### 8.2 `test-haptic-refresh-policy.cpp`

Models the haptic refresh logic with configurable modulus. Validates that:
- Amplitude changes are still applied immediately.
- Constant-amplitude rumble is refreshed at the configured interval.
- A 200 ms pulse is preserved with modulus 60 at 120 fps.

### 8.3 `tools/analyze-input-perf-log.py`

Parses existing `view_matrix.log` and reports:
- XInput polls/frame over time.
- Caller distribution.
- XR input phase peaks.
- Jump/melee spike counts and ages.
- Haptic call frequency and cumulative cost.

---

## 9. Regression and rollback checklist

For each proposed change:

### Haptic refresh interval (7.1)

- [ ] Build x86 `/O2` DLL; verify PE32 and SHA.
- [ ] Confirm rumble still works on weapon fire, damage, and Jump landing.
- [ ] Confirm rumble stops immediately when firing stops.
- [ ] Check `xrinput cost:` haptic peak does not increase.
- [ ] If regression: revert the single modulus value.

### Melee log throttle (7.2)

- [ ] Press impulse/melee repeatedly; confirm log appears at most once per second.
- [ ] Confirm melee arms still show for the correct duration.
- [ ] If regression: remove the `lastMeleeLogMs` guard.

### XInput state cache (7.3)

- [ ] Verify with caller census that multiple distinct callers exist.
- [ ] Test pad emulation on/off, menu navigation, and in-game movement.
- [ ] Confirm `dwPacketNumber` still advances when state changes.
- [ ] If regression: remove the cache and fall back to current behavior.

---

## 10. Recommended next steps

1. **Deploy the haptic refresh change** to a test build and measure `xrinput cost:` haptic peak during combat.
2. **Capture a stutter log** with `g_diagEnabled=1` and run `analyze-input-perf-log.py` to confirm whether spikes correlate with haptic peaks, XInput poll spikes, or Jump/melee edges.
3. **Implement the melee log throttle** if the log shows repeated `melee window:` lines within short windows.
4. **Investigate the XInput caller census** only if `xinput: ... calls/sec` shows >>1 poll/frame; do not optimize the hook itself.
5. Keep `AutoPad=0` default; re-evaluate only if new evidence links pad plug/unplug to stutters.
