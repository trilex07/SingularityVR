# Test and handoff playbook (MAIN)

Authoritative policy remains [`AGENTS.md`](../../../../AGENTS.md). This file is operational detail for Steam/PSVR2 and agent handoffs.

---

## MAIN pre-deploy

1. **Worktree sanity:** `git worktree list`, correct branch, clean scope (no edits in other agents’ paths).
2. **Confirm game exited:** if `Singularity.exe` is running, ask the user to close it normally and recheck. Do not automatically use `-StopGame` or terminate a process just to enable deployment; forced termination requires explicit user authorization for that action and an exact verified PID.
3. **Build:** x86 PE32, `/O2`, OpenXR via `OPENXR_SDK` + `tools/Resolve-OpenXrSdk.ps1`.
4. **Provenance:** branch, full commit SHA, SHA-256 of `d3d9.dll` in manifest (`d3d9.build.json` or project equivalent).
5. **Backup:** timestamped DLL + INI (+ UI package if replaced) — [`docs/STEAM_MAIN_DEPLOY_WORKFLOW.md`](../../../../docs/STEAM_MAIN_DEPLOY_WORKFLOW.md).
6. **Install candidate;** verify **installed** hash equals manifest (not “built elsewhere”).
7. **Experimental flags:** use checked-in `tools/set-*-experiment.ps1` with their bounded leases; remove owned flags before control tests. Persistent accepted INI switches are not leases: removing a flag does not disable a fix whose INI/default remains ON. Declare the actual INI/flag state and follow the accepted-fixes doc for an OFF control.

---

## MAIN runtime test

- Launch via real Steam + OpenXR runtime unless xrsim is explicitly scoped — [`docs/STEAM_XRSIM_RUNNER_PLAYBOOK.md`](../../../../docs/STEAM_XRSIM_RUNNER_PLAYBOOK.md).
- **One variable** per session when possible (e.g. ClearGate ON vs OFF without also changing receiver scope).
- **Bounded evidence:** targeted log excerpts, trace files, capture counts — no whole-log exfil, no per-draw spam.
- **Windowed desktop inspection** does not prove stereo or hand rendering — [`AGENTS.md`](../../../../AGENTS.md).
- Keep the candidate through the agreed active L4 window; do not silently roll back while the user is preparing it. End-of-test and unattended cleanup follow the rules below, not an indefinite wait for L4.
- On crash / no session / broken input: rollback, re-hash restored files, document failure.

---

## Test lease and cleanup

Before launch, agree the test window and arm the reviewed restore guard with a
verified backup, the exact candidate SHA and cleanup switches for this test's
owned flags/requests. `tools/watch-pause-test-restore.ps1` reports READY; its
default `LaunchWindowSeconds=600` bounds waiting for a game launch, not the
duration of a running game. It restores after observed game exit, or after a
no-launch timeout when the game is not running. It does not force-close a game.

- If the user is unavailable or does not reply before the agreed window ends,
  remove only this test's owned diagnostic requests/flags and restore the safe
  installation as soon as the game is confirmed closed. Do not leave an
  unattended candidate waiting indefinitely for another test window.
- If the game is still running, do not replace installed files or silently kill
  it. Request normal closure and report cleanup pending; retain the safe
  post-exit guard. A timeout or lack of reply does not authorize termination.
- After the test finishes, restore the declared safe installation and verify
  actual file hashes. Keeping/promoting a candidate beyond that test requires
  explicit user direction; L2 counters or a prior L4 build are not acceptance of
  an untested rebuild. Preserve intentional user INI changes and report which
  files were restored versus retained.

---

## PSVR2 physical test script (template)

Give the user **one block**; they stay in headset until finished.

```text
Setup: [branch @ commit], DLL SHA-256 [hash], INI/UI unchanged except [list].
Baseline: accepted features ON/OFF as listed in docs/accepted-shadow-input-fixes.md (or state deltas).
Test window: [agreed deadline/duration]; verified restore guard [backup/READY].

A — [action / scene / duration]
B — [action / scene / duration]
C — [optional control / return to menu]

Do NOT alt-tab to desktop during A→C.
After closing the game, report: stereo OK?, shadows?, gun tracking?, menus?, FPS feel?, any crash?

I will read: [log paths / trace requests used].
```

For holster/matrix captures, prefer `tools/run-shadow-holster-sequence.ps1` instructions in [`docs/shadow-map-content-probe.md`](../../../../docs/shadow-map-content-probe.md).

**Critical:** desktop chat focus during multi-step captures has caused freeze/focus loss — do not ask mid-sequence questions.

---

## Evidence level definitions (short)

| Level | Means | Does not mean |
|---|---|---|
| **L1** | Unit tests, synthetic math, parsers | Steam behavior |
| **L2** | PE32 `/O2` build, tests pass, SHA recorded | Visual correctness |
| **L3** | Verified DLL on installed game, OpenXR session, bounded runtime logs | Headset comfort / pixel-perfect |
| **L4** | User PSVR2 acceptance for stated scope | All levels/weapons/hotplug |

Update [`STATUS.md`](../../../../STATUS.md) with exact labels; never mark `PHYSICALLY_VERIFIED` without L4.

---

## Handoff checklist (MAIN ↔ SIDE ↔ user)

Copy and fill:

```markdown
## Handoff — [feature / incident]

- **Branch / HEAD:** `name` @ `full-sha`
- **Agent role:** MAIN | SIDE
- **Installed Steam state:** DLL SHA … | INI hash … | UI package hash … (or N/A for SIDE)
- **Candidate artifact:** DLL SHA … from commit …
- **Accepted baseline:** (menu/shadow/input flags + known good SHAs from STATUS or accepted doc)
- **Experimental switches ON:** …
- **Evidence achieved:** L1 / L2 / L3 / L4 (which tests, PIDs if L3+)
- **Proven facts:** bullet list tied to logs/docs
- **Open hypotheses:** bullet list
- **Failed experiments (do not repeat):** with falsification outcome
- **Autonomous test results:** commands + pass/fail
- **Next action:** single concrete step + owner
- **User-facing script needed?:** yes/no + draft if yes
```

SIDE handoffs stop at L2 unless explicitly given MAIN-only artifacts for review.

---

## Restore verification

Never claim rollback success without:

1. Game exited.
2. Rollback command run from known backup path.
3. `Get-FileHash` (or project script) on **installed** DLL, INI, UI package.
4. Experimental INI keys absent or documented.
5. `STATUS.md` updated with restored hashes.

Watchdog scripts must log READY/restored manifests; spurious LASTEXITCODE after success still requires hash proof — [`docs/pause-rtt-integration.md`](../../../../docs/pause-rtt-integration.md).

---

## SIDE agent boundaries

- No `tools/deploy-singularityvr.ps1`, no Steam launch, no installed-game mutation.
- L1/L2 only; document what MAIN should run for L3/L4.
- Branch from `origin/main`; PR for review; no self-merge.

---

## Useful tools (non-exhaustive)

| Task | Tool |
|---|---|
| Deploy / rollback | `tools/deploy-singularityvr.ps1` |
| Shadow trace request | `tools/request-shadow-pass-trace.ps1` |
| Shadow trace analysis | `tools/analyze-shadow-pass-trace.py` |
| Holster A/B/C schedule | `tools/run-shadow-holster-sequence.ps1` |
| Input enum experiment | `tools/set-input-enum-experiment.ps1` |
| VR perf log | `tools/analyze-vr-perf-log.py` |
| Trigger CPU | `tools/analyze-trigger-cpu.py`, `tools/record-trigger-cpu.ps1` |
| Pause RTT package test | `tools/test-pause-rtt-package.ps1` |
| Install state audit | `tools/shadow-install-state.ps1` |
