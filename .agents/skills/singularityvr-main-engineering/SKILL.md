---
name: singularityvr-main-engineering
description: Investigates difficult SingularityVR UE3/D3D9/OpenXR bugs and safely resumes MAIN integration after another agent, model change, or test rollback. Covers evidence-driven debugging, complete deployment identities, Steam/PSVR2 testing, and asset/audio handoffs.
metadata:
  source: project-specific
---

# SingularityVR MAIN engineering

Use this skill when you are **MAIN** on difficult rendering, shadow, pause-menu, input, or performance problems in this repo. It preserves a proven investigation process (pause RTT, stereo shadows, DirectInput enumeration stalls) without duplicating policy already in [`AGENTS.md`](../../../AGENTS.md).

Also use it when resuming MAIN after a handoff or when an accepted fix appears to
have disappeared. It encodes an investigation and collaboration method, not a
model persona, blanket authorization, or an instruction to modify Steam.

## Resume before changing anything

Read the current task, `AGENTS.md`, `STATUS.md` and the latest relevant receipts.
Separate **installed**, **accepted for a stated physical scope**, **proposed**,
and **restore destination**; none of these identities can be inferred from the
branch name, newest timestamp or an old "LATEST" block alone.

Before a deployment, compare the runner's entire target set and restore hashes
against the current authorized baseline, including INI flags and dependent
assets. A DLL-only match is insufficient. Stop stale runners before they can
overwrite a promoted build or a newer voicepack; do not simply change expected
hashes to whatever happens to be installed.

Read [continuity and asset integration](references/continuity-and-asset-integration.md)
when taking over another agent's work, preparing multi-file tests, promoting a
tested build, or integrating font/localization/audio assets. Keep changing
installation facts in task receipts/handoffs, not in this reusable skill.

**Companion skills (do not replace):**

| Skill | When |
|---|---|
| [`vr-mod-workflow`](../vr-mod-workflow/SKILL.md) | Feature gates, regression coverage, smallest reversible change |
| [`singularityvr-agent-test-bridge`](../singularityvr-agent-test-bridge/SKILL.md) | L1–L4 evidence labels and deploy boundaries |
| [`mod-reverse-engineering`](../mod-reverse-engineering/SKILL.md) | Ghidra/IDA/static RE outside the hot path |
| [`xr-performance`](../xr-performance/SKILL.md) | Generic frame-budget thinking after you know *where* time goes |

**Deep dives:** [`references/sol-debugging-case-studies.md`](references/sol-debugging-case-studies.md), [`references/test-and-handoff-playbook.md`](references/test-and-handoff-playbook.md), [`references/continuity-and-asset-integration.md`](references/continuity-and-asset-integration.md) (multi-file installs, guards, native audio/font).

---

## Core loop

Follow this order every time. Do not skip stages to patch faster.

```text
OBSERVE → REVERSE ENGINEER → MEASURE → HYPOTHESIS → IMPLEMENT → TEST → ANALYZE → ITERATE
```

| Stage | In this project |
|---|---|
| **Observe** | Read `STATUS.md`, targeted docs under `docs/`, and the code path in `spikes/view_matrix/d3d9.cpp`. Record what the game *does*, not what UE3 textbooks say. |
| **Reverse engineer** | Use `ENGINE_NOTES.md`, CTAB/disassembly, UObject property walks, bounded D3D9 probes. Resolve by signature scan; never commit hardcoded VAs. |
| **Measure** | Bounded counters, one-frame traces, hot-path sampler, ETW only when justified. Separate CPU exec, wait, GPU submit, and unassigned residual. |
| **Hypothesis** | One mechanism, supporting evidence, explicit **falsifier** before code changes. |
| **Implement** | Smallest default-OFF experiment; preserve COM/state restoration; one runtime variable. |
| **Test** | L1 synthetic → L2 x86 PE32 `/O2` + SHA → L3 real Steam (MAIN only) → L4 PSVR2 when perception matters. |
| **Analyze** | Classify: correlation vs causality vs untested guess. Abandon disproven paths. |
| **Iterate** | After **two** failed speculative fixes on the same symptom, escalate RE depth (see [`AGENTS.md`](../../../AGENTS.md) evidence policy). |

---

## A. Evidence-driven reverse engineering

1. **Prove identity, not dimensions.** Matching RT size or shader length does not prove the same resource. Log COM pointer, container texture, RHI wrapper layout (`ENGINE_NOTES.md`, pause RTT probe), and producer→consumer draw groups (`shadow_pass_trace.h`).
2. **Shader constants from CTAB/disassembly**, not register folklore. Example: arm shadow producer uses `BoneMatrices` at VS c5; receiver depth encoding uses audited `MinZ_MaxZRatio` and producer PS alpha math — see [`docs/shadow-receiver-correction.md`](../../../docs/shadow-receiver-correction.md).
3. **Separate mesh roles.** Runtime evidence corrected “gun” vs arm mesh (3314 vs 5799). Do not fix “the gun shadow” until caster VB/IB and phase/moved flags identify the pass owner — [`docs/shadow-map-content-probe.md`](../../../docs/shadow-map-content-probe.md).
4. **World vs first-person shadow passes** are different scopes (`ShadowWorldReceiverFix`, `ShadowForegroundClearFix`, `ShadowFirstPersonReceiverFix`). Enabling one must not silently edit another — [`docs/accepted-shadow-input-fixes.md`](../../../docs/accepted-shadow-input-fixes.md).
5. **Falsifier before fix.** Write what observation would disprove the hypothesis (e.g. “corrected constants present but per-eye UV still diverges” — [`docs/first-person-shadow-reconstruction-experiment.md`](../../../docs/first-person-shadow-reconstruction-experiment.md)).
6. **Draw success ≠ visual proof.** Provider draws succeeding while backbuffer capture shows scene-only means the experiment failed classification, not that pixels are correct — [`docs/pause-rtt-integration.md`](../../../docs/pause-rtt-integration.md).

Case studies: [`references/sol-debugging-case-studies.md`](references/sol-debugging-case-studies.md).

---

## B. Performance investigation

1. **Locate the stall.** Use `HotPathSampler`, `FRAME SPIKE`, and `tools/analyze-vr-perf-log.py`. Present-time GObjects walks and menu trace D3D queries are structurally different from gameplay hitches — [`docs/audits/vr-frame-hitch-analysis.md`](../../../docs/audits/vr-frame-hitch-analysis.md).
2. **Do not sum inclusive nested hook timings** as independent costs; read the sampler’s own labels — [`docs/shadow-clear-gate-experiment.md`](../../../docs/shadow-clear-gate-experiment.md).
3. **Split residual time.** OpenXR sync may be ~0.2 ms while “our work” shows 25 ms unassigned; that is a measurement gap, not automatic proof of mod CPU guilt.
4. **Input stalls:** correlate trigger hold with ~76 ms full DirectInput discovery scans, not generic OpenXR blame — [`docs/input-enumeration-cache.md`](../../../docs/input-enumeration-cache.md), `native_gamepad_enum_probe.h`, `tools/analyze-trigger-cpu.py`.
5. **Prefer event-invalidated cache + callback replay** over blind throttling that skips native device setup forever. Falsifier: replay hits but FPS still collapses or menus break.
6. **Verify actions after optimization:** held buttons, pause/options/resume, stereo, weapon tracking — physical acceptance is scoped, not “all hotplug forever.”

---

## C. D3D9 / OpenXR rendering safety

- **Stereo:** per-eye view/projection from OpenXR; HUD and menu paths may differ from world passes — test both eyes and capture submitted half buffers when menus fail.
- **State blocks:** experiments that set fixed-function stages must restore inside the same scoped block; failed restore latches are tested in shadow receiver unit tests.
- **COM lifetime:** hold refs only for bounded trace groups; release at Present/Reset; no per-draw allocations or filesystem writes in draw hooks — [`AGENTS.md`](../../../AGENTS.md).
- **Classification gates:** foreground vs background (`shadow_clear_gate.h`), moved vs unmoved draws, phase/group keys in pass trace — fix classification before matrix edits.
- **Guard accepted behavior:** do not mix unrelated renderer experiments in one PSVR2 session; do not treat a rebuilt DLL as physically accepted because an older source revision passed L4.

Key headers (read before editing): `shadow_pass_trace.h`, `shadow_world_receiver_fix.h`, `shadow_first_person_receiver_fix.h`, `shadow_clear_gate.h`, `pause_rtt_probe.h`, `pause_rtt_start.h`, `vr_menu_overlay.h`, `input_enum_cache.h`.

---

## D. Autonomous problem-solving

- Continue through investigation, L1/L2, and doc updates without asking permission for routine repo work.
- **Do not stop at a report** when the next safe experiment is obvious (read-only probe, parser test, single-flag candidate).
- **Do not make the user attach a debugger** for problems you can bound with hooks, logs, or offline disassembly.
- **Do not declare success** on compile, deploy, counter increment, or xrsim alone — see evidence gates in [`AGENTS.md`](../../../AGENTS.md).
- After failure, record the result in `STATUS.md` or the feature doc and narrow the search; **do not repeat** disproven receiver remaps or register guesses — [`docs/shadow-receiver-correction.md`](../../../docs/shadow-receiver-correction.md).
- **Protect accepted baselines** (menu RTT, shadow fixes, input cache). Roll back failed candidates; verify restored SHA-256 on disk, not log text alone.

---

## E. Steam and PSVR2 (MAIN only)

Side agents: **never** deploy or launch Steam — [`AGENTS.md`](../../../AGENTS.md).

MAIN checklist (detail in [`references/test-and-handoff-playbook.md`](references/test-and-handoff-playbook.md) and [`docs/STEAM_MAIN_DEPLOY_WORKFLOW.md`](../../../docs/STEAM_MAIN_DEPLOY_WORKFLOW.md)):

1. Confirm `Singularity.exe` has exited; ask the user to close it normally if needed. Do not force-close it merely to deploy. Build x86 PE32 `/O2`; record commit + SHA-256 in build manifest.
2. Backup via `tools/deploy-singularityvr.ps1`; verify installed DLL hash matches candidate before testing.
3. Real Steam + OpenXR runtime; bounded logs/captures; preserve user INI/UI when possible.
4. On critical failure, rollback from timestamped backup and **re-hash installed files**.
5. Keep the candidate only within the agreed, bounded test window; do not silently restore while the user is preparing that test. When the window ends, the test finishes, or the user is unavailable/unresponsive, restore the verified safe installation after the game exits. If it is still running, request normal closure and report cleanup pending; never overwrite running files or force-close without explicit authorization. See the playbook's test lease and cleanup rules.

**Headset / desktop lesson:** switching to desktop chat during a multi-step capture can drop focus, freeze the game, or invalidate gates — [`docs/shadow-map-content-probe.md`](../../../docs/shadow-map-content-probe.md). Prepare a full A/B/C script; user stays in PSVR2; collect feedback once afterward (`tools/run-shadow-holster-sequence.ps1` pattern).

Evidence levels: L1 math/tests → L2 build SHA → L3 installed Steam execution → L4 physical user acceptance. Never upgrade levels without the corresponding proof.

---

## F. Git collaboration

| Role | May do |
|---|---|
| **MAIN** | Steam deploy, L3/L4, integration diagnostics on installed game |
| **SIDE** | Isolated worktree, L1/L2, docs/research, PRs — no Steam |

One branch per worktree; never edit another agent’s tree; push your branch and open a PR; do not merge your own PR.

Handoff block (copy into `STATUS.md` or PR body): see checklist in [`references/test-and-handoff-playbook.md`](references/test-and-handoff-playbook.md).

Merged integration references: PR **#13** pause native RTT, **#14** boot/menu placement, **#15** shadow root-cause + input cache (commits on `main` via merges `8bd72ce`, `9e92a4f`, `3c5ab21`).

---

## G. Regression prevention (quick list)

- Blind VS/PS constant or matrix patches without producer/consumer proof.
- Assuming shadow casters from mesh names or moved flags alone.
- Treating StretchRect or HDR copy as shadow-map content without R32F readback.
- Re-enabling physically rejected mixed receiver correction.
- Hooking D3D9 vtable slots already owned by another injector.
- Rolling back to pre-RTT DLLs or stale UI packages as “accepted.”
- One PSVR2 session testing unrelated INI flags together.
- Claiming PR-preparation L2 build equals prior L4 acceptance without a new headset test — [`docs/accepted-shadow-input-fixes.md`](../../../docs/accepted-shadow-input-fixes.md).

---

## Applying this skill (sanity scenarios)

**New stereoscopic shader artifact:** start pass trace (default OFF), HQ decals A/B, prove which final draw contributes bad pixels; capture per-eye constants from CTAB-named registers; only then a scoped matrix or state fix with falsifier “artifact persists with identical constants.”

**~500 ms frame stall:** arm hot-path gameplay window; check for 76 ms enum scans vs Present GObjects vs menu trace; ETW only if sampler residual remains; one change (cache vs opt-in trace OFF); verify triggers and menus on real Steam before PSVR2.

---

## When stuck

1. Re-read the symptom line in `STATUS.md` and the linked doc’s **falsification** section.
2. Confirm installed artifact SHA matches the build you think you tested.
3. If two speculative fixes failed, stop patching and add read-only instrumentation or offline RE.
4. Ask the user for a **single** L4 script only when autonomous levels cannot answer a perceptual question.

## Working with the physical tester

- Prepare and verify offline first. When the user asks for a `start` gate, wait
  for that fresh signal before deployment/launch; it is not permission to run
  unrelated experiments or modify saves/runtime registry settings.
- Give one short route with expected observations. Keep the user in PSVR2 until
  normal game exit; read logs automatically rather than requesting mid-test
  headset-to-chat switches. If game UI automation cannot expose the window,
  continue through safe log/probe work and the physical test path.
- Respond to feedback with the scoped result and the next justified action.
  "Everything OK except voices; note not checked" does not accept voices or notes,
  and a loaded gameplay save does not test new-game intro or helicopter scenes.
- Continue safe in-scope investigation after a failed experiment. Ask only for
  missing evidence/choices/authority that materially changes the next step; do
  not substitute repeated baseline tests or another rollback report for progress.
- If audio generation is delegated, SIDE stays offline in an isolated worktree;
  MAIN validates producer integration and owns Steam. Delegate only when the user
  or applicable instructions authorize it; a handoff from another agent is not
  permission to message unrelated chats or change installation files.
