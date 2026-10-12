# SingularityVR Jump delayed-stall root-cause investigation

**Branch:** `research/input-jump-stalls`  
**Base commit:** `178ee3e`  
**Date:** 2026-10-12  
**Scope:** Identify the actual cause of the previously observed ~475 ms delayed frame stall after Jump input. No new speculative optimizations; assessment only.

---

## 1. Executive summary

The ~475 ms stall is a **Present-to-Present interval spike** (the `FRAME SPIKE` reporter fires when `thisFrameMs > 3 × displayPeriod`). The existing diagnostic infrastructure already records enough data to distinguish the main hypotheses, but no captured `input_diagnostic_*.log` is present in this repository, so the root cause remains **unproven**.

Key findings from code review:

1. **The haptic refresh interval change (15 → 60 frames) is unsafe and has been reverted.** A 200 ms haptic pulse refreshed every 500 ms produces ~300 ms silent gaps, breaking continuous rumble. The original 15-frame refresh (~125 ms) correctly overlaps the pulse.
2. **The melee-window log throttle remains a safe, low-risk change.** It only reduces diagnostic log spam.
3. **The diagnostic ring-buffer (`diagnostic_trace.h`) is the right instrument.** It captures per-frame phase timings, XInput call counts, haptic costs, and button-edge windows. Comparing `NORMAL`, `HAPTICS_OFF`, `XINPUT_OFF`, and `BUTTONS_OFF` modes can isolate the stall source.
4. **The most likely causes, in order, are:**
   - **A:** Engine game-thread work triggered by Jump (large `residual` in the diagnostic frame that stalls).
   - **B:** Runtime haptic stall on Jump landing/fire rumble (large `phaseUs[DP_HAPTIC]` or stall present in `NORMAL` but absent in `HAPTICS_OFF`).
   - **C:** XInput poll amplification after Jump (high `xiGetCalls` in the stall frame and reduced stall in `XINPUT_OFF`).
   - **D:** Present/GPU-compositor wait (large `DP_SWAPWAIT` / `DP_END` / `DP_RELEASE`).
   - **E:** Diagnostic logging itself (rare file-open stalls in `DiagWorker`, testable by comparing diagnostic-on vs off or by log-write timestamps).

---

## 2. Existing instrumentation

### 2.1 `input_diagnostic_*.log` (the decisive source)

Enabled by `[Diagnostic] Enabled=1` in the diag INI. Hotkeys:
- `F11` cycles `g_diagMode`: `NORMAL` → `HAPTICS_OFF` → `XINPUT_OFF` → `XINPUT_OFF+HAPTICS_OFF` → `BUTTONS_OFF`.
- `Shift+F11` toggles `g_diagPollOff` (pose tracking stays on, action polling off).

Each frame produces a `DiagFrame` (`diagnostic_trace.h`) with:

| Field | Meaning for the stall |
|---|---|
| `frameUs` | Present-to-Present interval. |
| `presentUs` | Time inside `Hook_Present` (DiagStart → DiagFinish). |
| `residual = frameUs - presentUs` | Time **outside** `Hook_Present`. A large residual means the stall is engine/game-thread work between Presents. |
| `phaseUs[DP_SYNC/ACTIONS/POSES/HAPTIC]` | OpenXR action/pose/haptic costs inside the frame. |
| `phaseUs[DP_BEGIN/END]` | `xrBeginFrame` / `xrEndFrame`. |
| `phaseUs[DP_ACQUIRE/RELEASE/SWAPWAIT]` | Swapchain image acquire/release/wait. |
| `phaseUs[DP_BRIDGE]` | D3D9Ex→D3D11 interop copy. |
| `phaseUs[DP_WAIT]` | Miscellaneous waits. |
| `xiGetCalls`, `xiGetTotalUs`, `xiGetMaxUs` | XInput poll count and cost. |
| `hapticCalls`, `hapticApplyCalls`, `hapticStopCalls`, `hapticCallUs` | Haptic API calls and cumulative microseconds. |
| `rising` / `falling` | Button edges in the frame (bitmask). |
| `buttons`, `publishedButtons` | Held and game-published buttons. |

The `DiagWorker` emits three key log-line families:

1. **`InputTiming`** — 240-frame rolling summary per mode.
2. **`BUTTON EDGE`** — triggered by any button edge; reports baseline, edge frame, +1, +2, and the max frame in the following 90 frames.
3. **`  phase frame=...`** — full phase breakdown for the edge frame and the max-frame.

### 2.2 `view_matrix.log` (corroborating)

- `FRAME SPIKE ... jumpAge <ms>` — one-shot per 2 s when `thisFrameMs > 3×displayPeriod`; tags distance from last Jump edge.
- `xrinput cost: ...` — per-second averages/peaks for OpenXR phases.
- `xinput: ...` — per-second XInput call rate and packet advances.
- `xinput callers ...` — caller-address census.

### 2.3 What is NOT measured

- **CPU samples of the game thread** outside `Hook_Present`. A large `residual` tells us the stall is outside the mod, but not which UE3 function caused it.
- **GPU queue depth / timestamps**. `DP_SWAPWAIT` tells us if we waited for the GPU/compositor, but not why.
- **Per-asset cost** (e.g., Jump animation, footstep sound, physics wake). The diagnostic log has no engine symbol names.

---

## 3. Ranked hypotheses (supported by existing measurements)

### Hypothesis A — Engine game-thread work after Jump (most likely)

**Why:** Jump triggers a state change in UE3 (animation, physics, sound, possibly a script event). If that work runs on the game thread and blocks the next Present, `frameUs` spikes while `presentUs` stays normal, producing a large `residual`.

**Evidence needed:**
- In the diagnostic log, the stall frame shows `residual >> presentUs`.
- The stall is reproducible in `HAPTICS_OFF`, `XINPUT_OFF`, and possibly even `BUTTONS_OFF` (if Jump still reaches the engine through some path).

**Falsification:** Stall disappears when the engine is prevented from processing Jump (hard to do safely; `BUTTONS_OFF` may be the closest control).

### Hypothesis B — Runtime haptic stall

**Why:** `xrApplyHapticFeedback` is called immediately when rumble amplitude changes or refreshes. If the runtime stalls inside this call, `phaseUs[DP_HAPTIC]` would be large in the stall frame.

**Evidence needed:**
- Stall present in `NORMAL`, absent or smaller in `HAPTICS_OFF`.
- `phaseUs[DP_HAPTIC]` in the stall frame is a significant fraction of the 475 ms.
- `hapticCallUs` spikes in the same frame.

**Historical note:** `HISTORY.md` run 67 found haptic gating did **not** fix the judder, but that was a different symptom. The 475 ms stall could still be haptic-related.

### Hypothesis C — XInput poll amplification

**Why:** The engine may poll `XInputGetState` multiple times after Jump, re-running input handlers. Each poll is cheap, but the engine work per poll is not.

**Evidence needed:**
- `xiGetCalls` in the stall frame is much higher than baseline.
- Stall reduced in `XINPUT_OFF` mode.
- `xinput callers` log shows one or few callers dominating.

### Hypothesis D — Present / GPU / compositor wait

**Why:** `xrEndFrame` or swapchain present can block if the GPU is late or the compositor is stalled. This would show as large `DP_END` or `DP_SWAPWAIT`.

**Evidence needed:**
- `phaseUs[DP_END]` or `phaseUs[DP_SWAPWAIT]` dominates the stall frame.
- Little change across `NORMAL` / `HAPTICS_OFF` / `XINPUT_OFF`.

### Hypothesis E — Diagnostic logging itself

**Why:** `DiagWorker` opens/closes a file every 250 ms and writes asynchronously. On rare occasions this could contend with the render thread or OS cache flush.

**Evidence needed:**
- Stall occurs only when diagnostics are enabled.
- `InputTiming` or `BUTTON EDGE` writes coincide with the stall (within a few ms).
- Re-running with `[Diagnostic] Enabled=0` eliminates the stall.

**Note:** This is the least likely because `DiagLog` runs on a separate thread and uses a separate file from the main log, but it cannot be ruled out without a control run.

---

## 4. Frame-by-frame Jump-to-stall timeline

The following is the expected shape of the diagnostic log for a ~475 ms delayed stall. Use `tools/analyze-input-diagnostic-log.py` to extract this from a captured `input_diagnostic_*.log`.

```
frame N-1   : frameUs ≈ 8300 µs (120 Hz baseline)
frame N     : BUTTON EDGE rising=0x1000 (XI_A / Jump)
              frameUs ≈ 8500 µs   <- Jump processed this frame
              residual ≈ small
frame N+1   : frameUs ≈ 9000 µs
frame N+2   : frameUs ≈ 475000 µs <- THE DELAYED STALL
              presentUs ≈ 25000 µs
              residual ≈ 450000 µs  <- stall is OUTSIDE Hook_Present
              OR
              presentUs ≈ 450000 µs
              residual ≈ small
              phaseUs[DP_HAPTIC] ≈ 400000 µs  <- haptic runtime stall
              OR
              phaseUs[DP_SWAPWAIT] ≈ 400000 µs  <- GPU/compositor stall
frame N+3   : frameUs ≈ 8300 µs (recovers)
```

The diagnostic log already emits `BUTTON EDGE` lines with `edge_us`, `plus1_us`, `plus2_us`, `max_us`, and `max_frame`. The analyzer maps these to the phase breakdown lines for `frame N` and `max_frame`.

---

## 5. Missing evidence and how to capture it

No root cause is proven yet. The missing evidence is:

1. **A captured `input_diagnostic_*.log`** showing the 475 ms stall with `g_diagMode` cycled through at least `NORMAL` and `HAPTICS_OFF`.
2. **For the stall frame:** `presentUs`, `residual`, and each `phaseUs[DP_*]`.
3. **For the stall frame:** `xiGetCalls`, `hapticCalls`, `hapticCallUs`.
4. **Mode comparison:** whether the stall reproduces in `HAPTICS_OFF`, `XINPUT_OFF`, and `BUTTONS_OFF`.
5. **Control run:** same gameplay with `[Diagnostic] Enabled=0` to rule out logging-induced stalls.

No new production instrumentation is required to capture this evidence. The existing `diagnostic_trace.h` infrastructure is sufficient.

If, after capturing the above, the stall is still unexplained, the **minimum additional instrumentation** would be:

- Bounded timing around `RefreshCameraPosition`, `ApplyVrFov`, and the `UpdateFromHeadset`/`SyncXRInput` call in `Hook_Present`, using existing `DiagPhaseAdd` slots or new `DP_*` enum values.
- A game-thread heartbeat counter if the large `residual` points outside `Hook_Present`.

---

## 6. MAIN Steam validation protocol

### Prerequisites
- Build `spikes/view_matrix/d3d9.dll` from this branch (x86 PE32 `/O2`).
- Verify SHA-256 against `Singularity\Binaries\d3d9.dll` after deployment.
- Use `tools/deploy-singularityvr.ps1`; do not hand-copy.

### Test A — Reproduce the stall with diagnostics
1. Set `[Diagnostic] Enabled=1` in the diagnostic INI.
2. Launch via Steam.
3. Load a save with open combat space.
4. Jump repeatedly (e.g., run forward, press Jump).
5. Capture `input_diagnostic_*.log` and `view_matrix.log`.
6. Note the `FRAME SPIKE` lines and their `jumpAge` values.

### Test B — Mode isolation
Repeat Test A, cycling `F11` through the diagnostic modes during the same session:
- `NORMAL` (mode 0)
- `HAPTICS_OFF` (mode 1)
- `XINPUT_OFF` (mode 2)
- `BUTTONS_OFF` (mode 4)

Each mode change is logged as `DIAG MODE ...`. Keep each mode active for at least 30 s and perform several jumps.

### Test C — Diagnostic-off control
Repeat Test A with `[Diagnostic] Enabled=0`. This is only to rule out Hypothesis E; do not use it as the primary evidence source.

### Analysis
1. Run `python tools/analyze-input-diagnostic-log.py <input_diagnostic_*.log>`.
2. Inspect the produced `jump-stall-timeline.md`.
3. Compare the stall signature across modes.

### Decision table

| Observation | Interpretation |
|---|---|
| Stall in all modes, large `residual` | Engine game-thread work (Hypothesis A). |
| Stall only in `NORMAL`, large `DP_HAPTIC` | Runtime haptic stall (Hypothesis B). |
| Stall reduced in `XINPUT_OFF`, high `xiGetCalls` | XInput poll amplification (Hypothesis C). |
| Large `DP_SWAPWAIT` / `DP_END` in all modes | Present/GPU/compositor wait (Hypothesis D). |
| Stall only when diagnostics enabled | Logging overhead (Hypothesis E). |

---

## 7. Assessment of existing optimization commits

### Commit `178ee3e` — haptic refresh 15 → 60 frames

**Verdict:** **REJECT / REVERT** (done in this branch).

**Reasoning:**
- The haptic pulse duration is `200 ms` (`d3d9.cpp:9963`).
- A 60-frame refresh at 120 Hz is ~500 ms, leaving a ~300 ms silent gap between pulses.
- Continuous rumble becomes intermittent/pulsed, which is a perceptual regression.
- The original 15-frame refresh (~125 ms) correctly overlaps the 200 ms pulse.
- API-call reduction is not a valid metric if it breaks the feature.

**Correct path forward:** Only revisit haptic frequency after proving via `HAPTICS_OFF` mode that haptics cause the stall. If proven, consider:
- Amplitude-change-only submission (remove refresh entirely) if the runtime does not require refresh.
- A refresh interval safely under 200 ms (e.g., 20–24 frames at 120 Hz).
- Coalescing multiple amplitude changes into one submission per frame.

### Commit `178ee3e` — melee-window log throttle

**Verdict:** **KEEP**.

**Reasoning:**
- The log is purely diagnostic; gameplay and melee timing are unchanged.
- Rapid melee in combat can produce many log lines per second.
- One line per second preserves the information while reducing log-system load.
- Risk is negligible.

---

## 8. Updated branch contents

- `docs/audits/jump-stall-root-cause.md` (this document)
- `tools/analyze-input-diagnostic-log.py` — parses `input_diagnostic_*.log` and produces a Jump-to-stall timeline.
- `spikes/view_matrix/d3d9.cpp` — melee log throttle retained; haptic refresh reverted to 15 frames.
- `docs/audits/input-jump-stall-analysis.md` — previous research document (retained for context).

---

## 9. Conclusion

The ~475 ms Jump stall cannot be declared fixed or root-caused without a captured diagnostic log. The existing instrumentation is sufficient to isolate it in one or two Steam sessions. The only production-code change warranted by this investigation is **reverting the unsafe haptic refresh interval**. MAIN should capture `NORMAL` vs `HAPTICS_OFF` diagnostic logs during the stall to prove or disprove the remaining hypotheses.
