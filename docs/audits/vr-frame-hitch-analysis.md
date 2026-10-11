# SingularityVR intermittent frame-hitch analysis

**Branch:** `research/vr-frame-hitch-analysis`  
**Base:** `origin/main` (`ce65fb5 Merge pull request #8`)  
**Date:** 2026-10-11  
**Mandate:** Research only. No runtime C++ edits, no Steam, no deployment, no synthetic FPS claims.

---

## 1. Summary

This audit uses the existing bounded hot-path sampler and one-shot spike reporter in `spikes/view_matrix/d3d9.cpp` plus a new static analyzer to separate the sources of intermittent long frames around **jump/input, combat and scene transitions**. The dominant measured hazards are already known; this work ranks them, quantifies their frequency, and identifies the smallest behavioral-preserving optimization that can be justified from the source alone.

**Key finding:** the largest *measured* mod-side stalls are **splash/transition stalls** in `Hook_Present` (`FindPlayerPawn` GObjects walks and camera register scans). These are structurally different from the **jump/combat hitches** reported by the `FRAME SPIKE` reporter, which carry `meleeAge`/`jumpAge` tags and occur during gameplay.

The most expensive per-draw path is `MenuDrawTraceRecord`, which issues roughly **20 D3D9 state queries per draw** during an 8-frame sampling window after every menu state change. With ~1,000 draws/frame, that is a bounded **~160,000 D3D9 queries per menu transition**.

The **primary optimization** (safe only after MAIN finishes pause-menu diagnosis) is to make `MenuDrawTrace` **opt-in/default-off**. The **secondary optimization** is a carefully split menu-suppression early-out that preserves `SyncVrMenuCaptureState()` for capture cleanup on menu close and focus transitions.

---

## 2. Scope and method

### Files read

- `spikes/view_matrix/d3d9.cpp` — hot-path sampler, Present frame hook, draw hooks, spike reporter.
- `spikes/view_matrix/hotpath_sampler.h` — bounded CPU accounting implementation.
- `spikes/view_matrix/vr_menu_overlay.h` — captured menu provider and `SyncVrMenuCaptureState`.
- `docs/PERFORMANCE_TRANSITION_STATUS_20261008.md` — prior transition-specific fixes.
- `docs/PERF_KILL_LIST.md` — manual checklist for engine/runtime settings.
- `STATUS.md` — run 252-257 splash-regression post-mortem and other measured fixes.
- `ENGINE_NOTES.md` — `Readable()` / `VirtualQuery` cost and GObjects walk cost.

### New standalone tooling

- `tools/analyze-vr-perf-log.py` — parses `view_matrix.log` and ranks hot-path families, lists frame spikes, and counts GObjects/register-scan events.
- `tools/scan-hotpath-patterns.py` — static read-only scan of `d3d9.cpp` that flags D3D `Get*` queries, `Readable()` / GObjects walks, `Log()` calls, allocations and interlocked operations inside hook functions.
- `spikes/view_matrix/tests/test-hotpath-sampler.cpp` — deterministic unit tests for the sampler math (sampling stride, counter aggregation, window boundaries, scaling).
- `spikes/view_matrix/tests/test-menu-suppress-policy.cpp` — policy helper for the proposed menu-suppression early-out, including menu-close and stale-capture transition cases.
- `spikes/view_matrix/tests/test-menu-trace-policy.cpp` — regression tests for the opt-in/default-off MenuDrawTrace policy.

No production C++ file was modified.

---

## 3. Existing diagnostics

The production code already contains the instruments this analysis relies on:

| Instrument | Location | What it measures |
|---|---|---|
| `g_hotPathSampler` | `d3d9.cpp:94`, `hotpath_sampler.h` | Per-family CPU cost sampled every 32nd call; reports `avg_us` and `est_ms/frame`. |
| `FRAME SPIKE` log | `d3d9.cpp:24440` | One-shot per 2 s when `thisFrameMs > 3×displayPeriod`; tags melee/jump/weapon-swap/no-centre ages. |
| State buckets | `d3d9.cpp:24547-24559` | Per-frame bucket by activity state; links `waitMs` and `xrSubmitMs` to each bucket. |
| Frame-rate governor log | `d3d9.cpp:24866` | Counts render-thread sleeps per second (UE3 `bSmoothFrameRate`). |
| GObjects walk log | `d3d9.cpp` / `STATUS.md` | Reports objects walked and cost. |

These are all **off or bounded by default** (`HotPathSampler=0`, `TimeOurWork=0`, `[Diagnostic] Enabled=0`) so the analysis is reading existing instrumentation, not adding new runtime overhead.

---

## 4. Static hot-path risk scan

`tools/scan-hotpath-patterns.py` scores hook functions by D3D9 `Get*` queries, `Readable()` / GObjects walks, unconditional `Log()` calls, allocations and interlocked operations. The ranking from `origin/main` is:

| function | line | D3D Get | desc | Readable | GObjects | Log | alloc | Interlocked | score |
|---|---|---:|---:|---:|---:|---:|---:|---:|---:|
| `Hook_Present` | 23331 | 0 | 0 | 7 | 9 | 154 | 1 | 231 | 1376 |
| `HudStereoPair` | 17373 | 4 | 0 | 0 | 0 | 4 | 0 | 31 | 63 |
| `Hook_DrawPrim` | 13103 | 4 | 0 | 0 | 0 | 1 | 0 | 13 | 30 |
| `Hook_DrawPrimUP` | 17914 | 0 | 0 | 0 | 0 | 1 | 0 | 12 | 17 |
| `Hook_SetTexture` | 19088 | 0 | 0 | 0 | 0 | 0 | 0 | 6 | 6 |

The absolute score is a heuristic, but the gap is decisive: **Present is ~22× more suspect than any per-draw hook** because it contains the GObjects / `Readable()` work and the report block.

### 4.1 Present-time hazards

`Hook_Present` contains:

- **7 `Readable(...)` calls** — `Readable()` is a `VirtualQuery` syscall. Six of them are inside a single-frame gameplay-offsets block (`d3d9.cpp:24367-24385`); one is in `FindCamera` fallback.
- **9 GObjects / object-model references** — `ResolveScriptNames`, `FindProcessInternal` and direct `GObjects` accesses. The heavy `FindPlayerPawn(force=true)` path lives inside `ResolveGameplayOffsets`.
- **154 `Log()` calls** — almost all are gated by one-shot flags or counters, but the report block is large and executes once per second unconditionally.
- **231 interlocked operations** — many are cheap counters, but the density is high in a function that also does long walks.

### 4.2 Per-draw hazards

- **`HudStereoPair`** reads shader constants via `GetVertexShaderConstantF` (4 times) and does 31 interlocked operations. This runs for every HUD draw.
- **`Hook_DrawPrim`** does 4 D3D `Get*` queries (likely `GetRenderTarget` / `GetViewport` for the scope-shape test) and 13 interlocked ops.
- **`Hook_SetTexture`** does 6 interlocked ops and the shadow-map cache check (cached after warmup).
- **`SuppressCapturedVrMenuDraw`** / **`MenuDrawTraceRecord`** show low static scores because the expensive `SyncVrMenuCaptureState` and texture compares are hidden behind inline helpers, but the hot-path family `menu_classification` is sampled every time these functions are entered.

---

## 5. Root-cause ranking

### Category A — Splash / transition stalls (distinct from jump/combat hitches)

These show up in `Hook_Present` and are tied to level loads, splash screens and death/reloads. They are **not** the same phenomenon as the jump/combat hitches flagged by `FRAME SPIKE`.

#### A.1 #1 — `FindPlayerPawn` GObjects walks in Present (game-thread, mod-side)

**Evidence:**
- `STATUS.md` run 252-257: `RefreshArmedState` → `FindPlayerPawn(force=true)` cost **1,365 ms in a single call** when no pawn existed (splash screen).
- Each walk iterates **112,565 objects** and calls `Readable()` per object — hundreds of thousands of kernel transitions.
- Mitigation already applied: time-based backoff (2 s → 15 s) and an 8-slot `Readable` region cache reduced the splash cost to **351 ms** every 15 s.
- Log marker: `player pawn: … after walking N object(s)`.

**Why it hitches:** a 350 ms stall every 15 s at 120 fps is a **42-frame freeze**. It is most visible at level loads / splash screens because the pawn does not exist, so the walk can never succeed.

**Correlated timing evidence required:** a `FRAME SPIKE` line with `nocentre=1` or `weapon-swap=1` and a `player pawn: … after walking N object(s)` line within the same 1–2 s window would link the walk to a gameplay hitch. Without that correlation, this remains a **splash/transition stall**, not a jump/combat stall.

**Removable without changing behavior:** make the walk event-driven instead of polling. The pawn pointer is already observed to change only on weapon swap / level load (`RefreshArmedState` reads `Pawn.Weapon` every frame). A safe, minimal change is to:
- Cache the last successful pawn pointer.
- Only re-walk when the cached pointer fails `IsLive()` **or** when `Pawn.Weapon` changes (which is already detected every frame).
- Reset the backoff on success, not on miss, so a transient miss does not escalate to 15 s of UNKNOWN state.

This preserves the current fallback behavior while eliminating the periodic failing walks.

#### A.2 #2 — Camera register re-scan at splash / no-lock (game-thread, mod-side)

**Evidence:**
- `d3d9.cpp:25507-25518`: `g_scanArmed` fires when `!g_haveLock && g_camPosValid && dupDraws`.
- At a splash screen all three conditions are true: VR is on, there is a camera position, but **there is no view matrix to find**.
- Log marker: `--- scan complete: 0 candidate window(s) ... no window even had a plausible forward vector`.
- `STATUS.md` run 252 measured the scan at **0.00 ms** and exonerated it for the splash staircase, but it remains a periodic expense when the lock is lost.

**Removable without changing behavior:** gate the scan on a scene-target validity flag. If the current render target is not the scene target (splash / menu / video), do not arm the scan. The matrix cannot be usefully located unless real scene geometry is being uploaded.

### Category B — Gameplay hitches around jump / input / combat

These are captured by the one-shot `FRAME SPIKE` reporter and tagged with `meleeAge`, `jumpAge`, `weaponSwapped` and `nocentre`.

#### B.1 #1 — Menu draw trace sampling window (the actual expensive path)

**Evidence:**
- `menu_draw_trace.h:55-91`: `MenuDrawTraceRecord` runs on every draw hook when `g_menuDrawTrace` is set (default **1**).
- During its 8-frame sampling window (frames 20–27 after a menu state change) it issues **~20 D3D9 state queries per draw**: `GetRenderTarget`, `GetDepthStencilSurface`, `GetVertexShader`, `GetPixelShader`, `GetVertexDeclaration`, `GetTexture`, `GetViewport`, `GetScissorRect`, `GetStreamSource`, `GetIndices`, `GetLevelDesc`, `GetSurfaceLevel`, seven `GetRenderState` calls, and `GetVertexShaderConstantF`.
- Log marker: `MenuDrawTrace state=... class=... groups=... dropped=...`.

**Bounded overhead estimate:**

```
per transition ≈ 8 frames × 1,000 draws/frame × 20 queries/draw ≈ 160,000 D3D9 queries
```

This is a **static upper bound**, not a measured runtime value. The actual cost depends on how many draws fall inside the window and how the D3D9 runtime caches state. It is, however, the most expensive per-draw diagnostic path in the mod.

**Why it is the priority:** D3D9 `Get*` calls serialize state and can stall the command buffer. A pause/main-menu/station open or close happens exactly when the user is sensitive to a hitch.

**Causation cannot be claimed from log line distance:** `FRAME SPIKE` lines and `MenuDrawTrace` flush lines carry no shared frame ID or timestamp, and flushes are emitted **after** the 8-frame sampling window ends. The analyzer therefore reports counts separately and explicitly marks correlation as unavailable.

#### B.2 #2 — Per-draw menu suppression checks (mod-hook overhead, lower priority)

**Evidence:**
- `SuppressCapturedVrMenuDraw` is called from the four draw hooks.
- It enters the `menu_classification` hot-path family and calls `SyncVrMenuCaptureState()` before its `!g_vrUiMenu` early-out.
- `Hook_SetTexture` checks `stage < 4` and `g_vrUiMenu` for every texture bind.

**Why it hitches:** each call is cheap, but with **1,000–1,500 draws/frame** the aggregate is non-zero. The risk spikes during menu transitions when `SyncVrMenuCaptureState` actually invalidates and releases textures.

**Safety caveat:** `SuppressCapturedVrMenuDraw` calls `SyncVrMenuCaptureState()` **before** its early return. Skipping the whole call delays token invalidation and captured-texture cleanup until the next `CaptureVrMenuTarget` attempt. This affects:
- **Menu close:** the captured texture from the closed menu may persist for frames after the token changes.
- **Focus transitions:** switching from pause to main menu changes the token; stale capture cleanup must happen promptly.
- **Stale capture ownership:** `g_vrCaptureOwner.CanProvide(VrMenuStateToken())` returns false after invalidation; delaying `SyncVrMenuCaptureState` can keep the owner claiming it provides the old token.

A safe implementation must therefore either (a) still call `SyncVrMenuCaptureState` on every draw and only skip the suppression logic, or (b) explicitly invalidate on the frame the token changes. Option (a) is the smaller change.

**Removable without changing behavior:** add a cheap guard that skips only the body of `SuppressCapturedVrMenuDraw` after `SyncVrMenuCaptureState()` has run. `MenuDrawTraceRecord` must remain callable because `g_menuDrawTrace` defaults to 1 and MAIN needs the trace.

### 5.4 #4 — Foreground / shadow classification per draw (mod-hook overhead)

**Evidence:**
- `TrackFpCaster` runs inside `Hook_DrawIndexedPrim` and selects `foreground_classification` or `shadow_classification`.
- It compares buffer/range identities against a 32-entry cache.
- The cache is reset on weapon swap.

**Why it hitches:** per-indexed-draw identity comparison. Cost is proportional to foreground/shadow draw count.

**Removable without changing behavior:** when the mesh latch is stable and no weapon swap has occurred, skip the caster learning pass. The cache is already retained; a `g_fpCasterCount > 0 && weapon unchanged && !shadowPass` fast-path could return early for known casters.

### 5.5 #5 — Frame spike reporter unconditional bookkeeping (mod-hook overhead)

**Evidence:**
- `d3d9.cpp:24405-24445`: computes `thisFrameMs`, peak counters, deltas, and calls `MeleeWindowActive()`, `HealWindowActive()`, `GetTickCount64()` on every Present even when `g_frameSpikeReport == 0`.
- The actual log is gated, but the surrounding work is not.

**Why it hitches:** tiny per-frame cost, but it runs in the same critical section as the Present hook and adds jitter.

**Removable without changing behavior:** move `thisFrameMs` computation and spike bookkeeping under the `g_frameSpikeReport || TimeOurWork` gate. The frame budget report already has its own separate path.

### 5.6 #6 — `HudStereoPair` D3D constant queries (mod-hook overhead)

**Evidence:**
- `HudStereoPair` calls `GetVertexShaderConstantF` 4 times (static scan).
- It rewrites `c5-c8` per eye for HUD insetting.

**Why it hitches:** per-HUD-draw driver queries. These are likely cached by the runtime but still enter kernel mode.

**Removable without changing behavior:** cache the last shader constant block and skip the read if the HUD transform has not changed. The current code already tracks `g_hudConstsWritten`; extending that to skip `GetVertexShaderConstantF` when the block is unchanged is a small cache addition.

### 5.7 #7 — XR `xrWaitFrame` and runtime waits (runtime, not mod)

**Evidence:**
- Frame budget line: `xrWaitFrame 4.35 ms IDLE`.
- `hotpath::Family::xr_wait_frame` is sampled around the OpenXR call.

**Why it hitches:** this is the compositor / runtime pacing the application. It is **not mod overhead** and should not be "optimized" by removing work (that would just increase idle time).

**Action:** distinguish it in reports; the analyzer tool already separates it.

### 5.8 #8 — D3D9-to-D3D11 copy (GPU/CPU synchronization, architectural)

**Evidence:**
- `hotpath::Family::d3d9_to_d3d11_copy`.
- `ZeroCopy=1` and `FrameCopyMode=1` defaults already minimize this.

**Why it hitches:** copying the backbuffer to the OpenXR swapchain is required by the current architecture.

**Action:** not removable without replacing the rendering bridge; out of scope for a small fix.

---

## 6. Proposed optimizations

### 6.1 Primary — opt-in/default-off menu trace policy (suitable after MAIN finishes pause diagnosis)

Once MAIN's pause-menu investigation is complete, make `g_menuDrawTrace` **default to 0** and expose an opt-in toggle (e.g., INI key `MenuDrawTrace=1` or a panel row). This is the highest-impact change because it removes the ~160,000 D3D9 queries per menu transition entirely when diagnostics are not needed.

**Preservation of behavior:**
- `MenuDrawTraceRecord` returns immediately when `g_menuDrawTrace == 0` (`menu_draw_trace.h:55`).
- Capture cleanup, suppression, main menu, pause menu and all native UI rendering are unaffected; the trace is a pure diagnostic consumer.
- The one-line return does not change any render state or skip any production draw path.

**Regression test added:** `spikes/view_matrix/tests/test-menu-trace-policy.cpp` models the trace enable/disable gate and verifies that disabled traces produce zero recorded groups.

### 6.2 Secondary — safe menu-suppression early-out (with SyncVrMenuCaptureState preserved)

After the trace policy, the next small win is to avoid the `MenuSourceBoundAny()` and suppression-body work during gameplay. Because `SyncVrMenuCaptureState()` must still run to keep capture ownership correct, the safe shape is to split the existing function rather than skip it entirely:

```cpp
// Called from every draw hook, before SuppressCapturedVrMenuDraw.
SyncVrMenuCaptureState();

if (!ShouldSkipMenuSuppression(Rd(g_vrUiMenu), Rd(g_vrMenuClass),
                               g_vrCaptureOwner.CanProvide(VrMenuStateToken()))) {
    if (SuppressCapturedVrMenuDrawBody()) return D3D_OK;
}
```

Where `SuppressCapturedVrMenuDrawBody` is the current function minus the leading `SyncVrMenuCaptureState()` call. This preserves:
- **Menu close / focus transitions:** token invalidation and texture release happen on the first draw after the token changes.
- **Stale capture ownership:** `g_vrCaptureOwner` is updated immediately, so provider draws do not use stale sources.
- **Main menu / pause / station UI:** suppression still fires whenever a menu is focused or a provider is active.

The helper in `spikes/view_matrix/tests/test-menu-suppress-policy.cpp` is unchanged; it now documents that the predicate must be evaluated **after** `SyncVrMenuCaptureState` has run.

---

## 7. Verification plan

### L1 — deterministic / synthetic

1. Run `test-hotpath-sampler.cpp`, `test-hotpath-sampler-scaling.cpp`, `test-menu-suppress-policy.cpp` and `test-menu-trace-policy.cpp` with `/O2`.
2. Run `scan-hotpath-patterns.py` against `d3d9.cpp` and confirm the ranked table.

### L2 — build / test

1. Build the x86 `/O2` DLL from the branch (no source changes, so this is a no-op baseline check).
2. Verify PE32/x86 provenance of the unchanged artifact.

### L3 — MAIN-owned runtime (optional, not required for this research PR)

1. Capture a `view_matrix.log` through a jump/combat/transition session.
2. Run `tools/analyze-vr-perf-log.py` and confirm:
   - `Hook_Present` residual is the largest bucket.
   - GObjects walk events coincide with **transition** moments, not `FRAME SPIKE` jump/combat events.
   - `FRAME SPIKE` lines near `MenuDrawTrace` flushes indicate trace-sampling transition hitches.
   - After applying the menu-suppression early-out, `menu_classification` `est_ms/frame` drops during gameplay while `MenuDrawTrace` output remains intact.

---

## 8. Falsification tests

| Hypothesis | What would prove it wrong |
|---|---|
| GObjects walks are the #1 mod-side hitch. | Disable `FindPlayerPawn` walks; if hitches persist at transitions, the cause is elsewhere (e.g., register scans). |
| Menu early-out is safe. | Run with `g_vrUiMenu` forced false; verify `g_vrMenuSuppressed` is zero and no menu-source draws leak. |
| `HudStereoPair` constant queries matter. | Cache the queries; if HUD draw time does not drop, the driver already caches them. |
| Frame spike reporter adds jitter. | Gate the reporter; if `our work` residual variance does not shrink, the cost is elsewhere. |

---

## 9. Deliverables

| File | Purpose |
|---|---|
| `docs/audits/vr-frame-hitch-analysis.md` | This audit. |
| `tools/analyze-vr-perf-log.py` | Parses existing logs into a ranked report. |
| `tools/scan-hotpath-patterns.py` | Static risk scan of hook functions. |
| `spikes/view_matrix/tests/test-hotpath-sampler.cpp` | Sampler math unit tests. |
| `spikes/view_matrix/tests/test-hotpath-sampler-scaling.cpp` | Deterministic scaling tests for sampler math. |
| `spikes/view_matrix/tests/test-menu-suppress-policy.cpp` | Policy helper for proposed suppression early-out, including transition cases. |
| `spikes/view_matrix/tests/test-menu-trace-policy.cpp` | Regression tests for opt-in/default-off MenuDrawTrace policy. |

No production source file was changed.

---

## 10. Recommended next steps

1. **After MAIN finishes pause-menu diagnosis, change `g_menuDrawTrace` default from 1 to 0** and add an opt-in toggle (INI key or panel row). This removes the bounded ~160,000 D3D9 queries per menu transition when diagnostics are not needed.
2. **If applying the menu-suppression early-out, split `SuppressCapturedVrMenuDraw`** so that `SyncVrMenuCaptureState()` still runs on every draw and only the suppression body is skipped. This preserves capture cleanup on menu close and focus transitions.
3. **Event-drive `FindPlayerPawn`** off the `Pawn.Weapon` change signal instead of the 2 s / 15 s backoff, but only for splash/transition stalls, not jump/combat hitches.
4. **Gate register scans** on scene-target validity so splash screens do not arm them.
5. **Cache `HudStereoPair` constant reads** when `g_hudConstsWritten` has not changed.
6. Keep the analyzer scripts checked in so future regressions can be diagnosed from logs without new instrumentation.
