# SingularityVR — Project Rules for All Agents

**Version:** 1.0
**Effective:** 2026-10-09
**Applies to:** Every agent, every branch, every session.

---

## 1. PARALLEL AGENT ISOLATION (MANDATORY)

| Resource | Rule |
|---|---|
| **Git worktree** | Each agent MUST use its own `git worktree add` checkout. No shared working directories. |
| **Game sandbox** | Each agent MUST use a separate copy of the game installation (`C:\Games\SingularityVR_<agent-id>\`). No shared Binaries folder. |
| **Configuration** | Each agent MUST use its own `SingularityVR.ini` (copied to its sandbox). No shared ini. |
| **Logs / output** | Each agent writes to its own `%LOCALAPPDATA%\SingularityVR_<agent-id>\` or sandbox-local `logs/`. No shared log files. |
| **Steam production** | **Exclusively owned by the primary agent.** No other agent may copy, move, or overwrite anything in `C:\Program Files (x86)\Steam\steamapps\common\Singularity\Binaries\`. |

> **Violation = immediate branch freeze.** The offending agent's worktree is discarded and the session ends.

---

## 2. VERIFICATION LADDER (PROMOTION GATE)

A feature is **not FIXED** until it passes **all lower levels**. Compilation alone is **level 0** — it proves nothing about correctness.

| Level | Name | Must demonstrate | Who runs it |
|---|---|---|---|
| **0** | Compile | `cl /O2` succeeds, PE32 x86, no `/analyze` fatal warnings | Agent CI |
| **1** | Unit / math tests | All deterministic assertions pass (exit 0) | Agent |
| **2** | Synthetic XR tests | Headless `openxr_loader` + fake runtime passes frame submit, timing, resource lifecycle | Agent |
| **3** | Recorded XR replay | Replay captured frames against recorded headset poses; image diff < threshold | Agent |
| **4** | Agent sandbox runtime | Full game launch in agent's isolated sandbox; 60 s stable 90 Hz, no crashes, feature toggles work | Agent |
| **5** | Shared Steam runtime test | Primary agent deploys to `Steam\...\Binaries\` and runs 5 min session; no regressions vs baseline | Primary only |
| **6** | Physical PSVR2 acceptance | Real headset on PS5/PC; 10 min play; comfort, clarity, tracking, 90 Hz lock | Primary + QA |

**No level may be skipped.** A feature reported "FIXED" at level N without passing N−1 is **rejected**.

---

## 3. BRANCH & COMMIT DISCIPLINE

| Rule | Detail |
|---|---|
| **Feature freeze** | Once a branch is marked `FROZEN` (in its latest commit message), no code changes allowed — docs only. |
| **Integration gate** | `integration/<feature>-on-latest-runtime` is created **only** from an explicitly approved stable production commit. Never from `main` or a moving branch. |
| **Cherry-pick only** | Integration uses selective cherry-pick of the feature commits. Never blind merge of research history. |
| **Production wins** | During conflict resolution: production behavior wins for menu, GFx, cursor, laser, hands, IK, grip, shadows, prompts, Triangle, recenter, camera. Research wins only for its declared domain (e.g. upscaling, render resolution, NIS resources, NIS frame processing, NIS GPU timing). |
| **No force push** | `git push --force` is forbidden on shared remotes. |
| **Clean tree** | Every commit must leave `git status --short` empty except for intentional untracked deployment artefacts. |

---

## 4. DEPLOYMENT POLICY

| Target | Who may write | Notes |
|---|---|---|
| Agent worktree / sandbox | Owning agent | Full read/write. |
| `deployment/<feature>-experiment/` | Owning agent | Staging only. DLLs, ini, shaders, README. Git-ignored binaries. |
| `C:\Program Files (x86)\Steam\steamapps\common\Singularity\Binaries\` | **Primary agent only** | All other agents: **read-only**. Violation = branch freeze. |

---

## 5. LOGGING & TELEMETRY

- **No per-frame disk I/O** in hot paths (Present, Submit, Copy). Accumulate in memory; flush at ≤ 1 Hz or shutdown.
- **GPU timestamps** are mandatory for any compute/dispatch work. CPU `Dispatch` time is never reported as GPU time.
- **Per-second perf line** format is the single source of truth for frame-budget decisions.
- **Lifetime counters** (shader inits, stage/output creates) must appear on the perf line — any per-frame recreation is a bug.

---

## 6. TEST REQUIREMENTS

| Category | Minimum |
|---|---|
| Deterministic unit tests | ≥ 1 assertion per public function; exit 0 = pass |
| Resource lifetime | Counters for every persistent allocation; assert no per-frame growth |
| Seam / bounds | High-contrast SBS seam test (offline image or math) must pass |
| Fallback | Every init path has a sticky `failedOnce` → native path; game must launch |
| Off-mode regression | `Mode=Off` build behaves bit-identical to production native path |

---

## 7. DOCUMENTATION

- Every feature branch **must** have an integration manifest (`docs/<FEATURE>_PRODUCTION_INTEGRATION.md`) before integration is attempted.
- The manifest lists: exact call surface, new files, conflict map (LOW/MEDIUM/HIGH), resolution rules, gated presets.
- **No integration without a manifest.**

---

## 8. ACKNOWLEDGMENT

By starting work on this repository, every agent acknowledges these rules. Ignorance is not a defense.

**Primary agent** (Steam production owner): ________________________
**Research agent(s):** ________________________
**Date:** 2026-10-09

---

*End of Project Rules.*