# Sol-style debugging case studies (SingularityVR)

Factual summaries backed by repository docs and merged work on `main`. Not a timeline dump; use with [`../SKILL.md`](../SKILL.md).

Evidence levels: **L1** synthetic/unit, **L2** x86 build+SHA, **L3** real Steam/OpenXR execution, **L4** physical PSVR2 user acceptance.

---

## 1. Native pause menu (Scaleform / RTT) in VR

**Symptom:** Pause and Objectives clipped or outside the visible VR region; stereo menu misaligned; provider draws “succeed” but headset shows wrong content.

**Initial hypothesis:** Wrong asset or missing dedicated `RTT_VRPause` render target.

**Evidence**
- Read-only probe: `SPPause` references `RTT_VRPause`, 1280×720, mode raw 2; `SPMain` uses `RTT_UI1` — [`docs/pause-rtt-integration.md`](../../../../docs/pause-rtt-integration.md).
- Resource association required RHI wrapper layout correction (opaque handle at base+0xC, COM at base+8), not guessed +0x1C — same doc.
- Complete provider RGB at 1280×720; Objectives body had RGB with **zero alpha**; blend mode mattered.
- Post-provider capture via wrong surface showed **scene only** → draw success was not menu visibility proof.

**Successful path (PR #13, merge `8bd72ce`)**
- Native movie registration honors dedicated RTT; full-UV presentation; fixed-function sampling contract isolated in state block; compact centred panel with matching laser geometry; restore watchdog with verified hashes — commits `d35008c` … `eac307b` on integration branch.

**Physical validation:** L4 “looks perfect” for compact panel layout; options/return-to-main needed separate confirmation — [`docs/pause-rtt-integration.md`](../../../../docs/pause-rtt-integration.md).

**Lesson:** Prove UObject → RHI → bound surface identity before changing draw policy; capture **submitted XR backbuffer**, not an intermediate RT that native code no longer owns.

**Code / tools:** `spikes/view_matrix/pause_rtt_probe.h`, `pause_rtt_start.h`, `native_ui_input.h`, `vr_menu_overlay.h`, `tools/test-pause-rtt-package.ps1`, `tools/read-pause-rtt-state.ps1`.

---

## 2. Stereo shadow squares, frozen gun, enemy dynamics

**Symptom:** Square shadow artifacts; controller-following “background negative”; black/frozen gun; dynamic enemy shadows missing or wrong; per-eye gun/glove shadow mismatch.

**Initial hypotheses (several disproven)**
- Global receiver matrix remap using scene-depth camera → **physically rejected** (shadows followed controller) — [`docs/shadow-receiver-correction.md`](../../../../docs/shadow-receiver-correction.md).
- Map-pass drops or blind c1/c13 remaps → failed visually — [`docs/shadow-rootcause.md`](../../../../docs/shadow-rootcause.md).

**Evidence that stuck**
- Bounded pass trace: shader/RT/texture/scissor identity, overflow reported, bytecode at Present not in draw hot path — `shadow_pass_trace.h`, [`docs/shadow-rootcause.md`](../../../../docs/shadow-rootcause.md).
- R32F readback: encoded/filtered shadow data, not raw Z — [`docs/shadow-map-content-probe.md`](../../../../docs/shadow-map-content-probe.md).
- **Role correction:** mesh 3314 = arm caster; 5799 = gun scene draws (moved flags differ) — `STATUS.md`, map-probe doc.
- Premature foreground window: gun late depth/color groups `fgCount` 90+ with ClearGate OFF vs ~1–4 ON — [`docs/shadow-clear-gate-experiment.md`](../../../../docs/shadow-clear-gate-experiment.md).
- World receiver: mathematical caster-origin error vs corrected per-eye inverse; live enemy map producer identified — [`docs/world-shadow-reconstruction-experiment.md`](../../../../docs/world-shadow-reconstruction-experiment.md).
- Near1 receiver: identical native inverse despite different eye VS; algebraic UV witness for per-eye reconstruction — [`docs/first-person-shadow-reconstruction-experiment.md`](../../../../docs/first-person-shadow-reconstruction-experiment.md).

**Successful combination (PR #15 + follow-up, L4 accepted)**
- `ShadowWorldReceiverFix` + `ShadowForegroundClearFix` + `ShadowFirstPersonReceiverFix` + input cache defaults documented — [`docs/accepted-shadow-input-fixes.md`](../../../../docs/accepted-shadow-input-fixes.md).
- Persistent INI promotion is L2; algorithms unchanged from physically tested commits — do not conflate.

**Physical validation:** L4 user acceptance PID39308 trace 1196 draws / 304 groups; scoped scene, not all weapons/levels/hotplug.

**Lesson:** Fix **pass classification and scoped receiver math** in order; one counter or matrix upload is not proof; CTAB names beat register guessing.

**Code:** `shadow_world_receiver_fix.h`, `shadow_clear_gate.h`, `shadow_first_person_receiver_fix.h`, `shadow_pass_trace.h`, `tools/analyze-shadow-pass-trace.py`, `tools/run-shadow-holster-sequence.ps1`.

---

## 3. Held-input FPS collapse (~76 ms stalls)

**Symptom:** Holding L2/R2 (and similar) drove FPS from ~90 toward ~40; release recovered; not explained by OpenXR sync alone.

**Initial hypothesis:** OpenXR input or mod draw hooks dominate frame time.

**Evidence**
- Same-session logs: OpenXR sync/actions ~0.18 ms in slow interval; large “our work” residual unassigned — [`docs/shadow-clear-gate-experiment.md`](../../../../docs/shadow-clear-gate-experiment.md).
- Native callback audit: ~76 ms full DirectInput device discovery; throttling 500 ms / 2000 ms improved FPS partially but is incomplete — [`docs/input-enumeration-cache.md`](../../../../docs/input-enumeration-cache.md).
- Callback `011D2180` (image-relative; use `.text` + base) compares GUIDs and may create/configure devices — not a poll loop.

**Successful approach**
- Event-invalidated cache of up to 64 `DIDEVICEINSTANCEW` records; on hit **replay original callback** on game thread with stored instance — `input_enum_cache.h`.
- Falsifiers: replay without scan reduction; menu/action regression; hotplug (explicitly out of scope for first L4).

**Physical validation:** L4 PID19520 — zero full scans in gameplay intervals, ~90 FPS, user confirmed controls + pause/options/resume — [`docs/input-enumeration-cache.md`](../../../../docs/input-enumeration-cache.md).

**Lesson:** Measure the native callback that actually burns 76 ms; optimize discovery frequency without breaking device creation paths.

**Tools:** `tools/analyze-trigger-cpu.py`, `tools/record-trigger-cpu.ps1`, `tools/set-input-enum-experiment.ps1`, `native_gamepad_enum_probe.h`.

---

## 4. Investigation hygiene: tab switch and multi-step captures

**Symptom:** Scheduled A/B shadow holster capture aborted when user switched to desktop chat; game lost focus/froze.

**Evidence:** [`docs/shadow-map-content-probe.md`](../../../../docs/shadow-map-content-probe.md) § No-tab-switch holster protocol.

**Successful mitigation:** External runner schedules requests; user remains in PSVR2; single feedback round after sequence — `tools/run-shadow-holster-sequence.ps1`.

**Lesson:** MAIN owns capture choreography; never interleave chat prompts mid-sequence.

---

## 5. Boot / main menu placement (PR #14)

**Symptom:** Boot labels and VR Settings row misaligned for PSVR2 readability.

**Successful path:** Centred boot labels, lowered main menu, raised settings row with hit-band match — merge `9e92a4f`, doc [`docs/boot-main-menu-placement.md`](../../../../docs/boot-main-menu-placement.md).

**Lesson:** Menu fixes are policy + hit testing; verify L4 separately from pause RTT acceptance.
