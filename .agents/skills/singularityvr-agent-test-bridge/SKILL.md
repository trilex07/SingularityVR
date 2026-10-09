---
name: singularityvr-agent-test-bridge
description: Mandatory evidence workflow for SingularityVR runtime, OpenXR, rendering, input, hands, VFX, shadows, menus, performance, and deployment changes.
---

# SingularityVR agent test bridge

Use this skill for every change that can affect the running mod. The ownership
boundary is strict: sub-agents produce L1/L2 evidence; only the main agent may
perform L3 on the main Steam installation.

## Sub-agent route: L1/L2 handoff

1. Read `AGENTS.md`, `docs/VR_STATUS.md`, and the target evidence.
2. Run `tools/agent-l1-l2-handoff.ps1 -Action L1` with the recording and
   isolated xrsim directory when a replay dry-run is relevant.
3. Inspect the candidate artifact with
   `tools/agent-l1-l2-handoff.ps1 -Action L2 -Artifact <dll> -BuildManifest <manifest>`.
   Require x86 PE32, `/O2`, matching build SHA, and a recorded commit.
4. Package the reports with `-Action Package`. Send the handoff manifest,
   commit, artifact path, SHA, and report directory to the main agent.
5. Do not launch Singularity, write Steam, deploy a DLL, stop another agent,
   or invoke a main-install runtime from a sub-agent.

## Main-agent route: explicit L3

The main agent reviews the handoff, then consciously performs the following:

1. Verify the artifact without deployment:
   `tools/l3-steam-guard.ps1 -Action Verify -Artifact <dll> -ExpectedSha256 <sha>`.
2. Back up the current main-install DLL/INI/VFX:
   `tools/l3-steam-guard.ps1 -Action Backup -SteamRoot <steam-root> -BackupDirectory <new-backup>`.
3. Explicitly deploy the reviewed artifact using the existing project deployment
   procedure. This skill and the handoff tools do not perform that action.
4. Launch the main Steam installation with the existing `dvr-xrsim` manifest,
   wait for the exact game PID to report `FOCUSED` and two projection views,
   then replay the canonical `.vrrec`.
5. Collect only fresh L/R/SBS capture, xrsim state, ack, log, PID and DLL SHA;
   run `Collect` and `VerifyCapture` from `tools/singularityvr-test-bridge.ps1`.
6. On failure or after the bounded test, stop the exact game PID and use the
   explicit guard rollback:
   `tools/l3-steam-guard.ps1 -Action Rollback ... -ConfirmRollback`.

The L3 verdict is PASS/FAIL/INCONCLUSIVE from fresh evidence, not from a
build, ABI match, draw counter, or simulator initialization line. L3 proves
main-install integration; L4 still requires the physical PSVR2.
