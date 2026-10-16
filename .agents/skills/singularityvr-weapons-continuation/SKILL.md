---
name: singularityvr-weapons-continuation
description: Resume SingularityVR weapon work after a chat or model handoff, especially Centurion manual reload, controller bindings, native gloves, knife and holsters. Separates prototypes from accepted Steam/PSVR2 behavior and routes to the current task receipt.
---

# SingularityVR — weapons continuation

## Project and role

SingularityVR converts the user's Singularity (Unreal Engine 3, Windows x86,
D3D9) into OpenXR VR with stereo, 6DoF controllers, tracked weapons/hands,
holsters, VR menus and Polish localization. The main runtime is the native C++
proxy in `spikes/view_matrix/d3d9.cpp`, not a Unity/UE5 project or GLB scene.

Act as a C++/UE3 reverse-engineering engineer: trace the actual producer and
consumer, then implement a narrow reversible change. Reuse original game
meshes, UVs, textures and rigs where the user requested native visual identity;
do not replace them with approximate procedural models or restart asset research.

The current user request and repository `AGENTS.md` govern scope/permissions.
Default to SIDE unless the human assigns MAIN. SIDE owns an isolated worktree
and branch, L1 CPU tests and L2 optimized x86 build; ZERO Steam launches,
deployment, restore, installed-file edits or live memory probes. Only MAIN owns
Steam/L3 and physical PSVR2/L4. This skill grants no additional authority.

## Resume cheaply

1. Read [current handoff](references/current-handoff.md), then the latest user
   request, `AGENTS.md`, latest relevant `STATUS.md` block. Verify branch, HEAD,
   worktree ownership and diff before edits. Dates/SHAs in the handoff are a
   snapshot, NOT proof of today's installed or accepted build.
2. For the active Centurion task read `docs/centurion-grip-shoulder.md`; use
   `docs/centurion-manual-reload.md` only for deeper native/animation evidence.
   Do not reread the full chat or entire history to begin.
3. Apply existing project skills: `vr-mod-workflow` and
   `singularityvr-agent-test-bridge`; for hard RE/MAIN takeover use
   `singularityvr-main-engineering`. Prefer their established procedures to
   duplicating or inventing deployment tooling.

## Non-obvious invariants

- Understand before patching. After two failed guesses, deepen RE. Never guess
  UE3 offsets, shader registers, UObject call ABIs or inventory writes.
- `Hook_ProcessEvent` here is actually **ProcessInternal(FFrame&,Result)**.
  Never invoke its trampoline as `ProcessEvent(UFunction*,Parms)`/Reload.
- Gesture acceptance, a draw counter or a successful build is NOT a native ammo
  transfer, rendered-content proof or headset acceptance. Label L1/L2/L3/L4.
- Preserve left-hip knife/right-hip gun, accepted input cache, shadows, cinema,
  menus, Polish banks/fonts and calibration unless explicitly in scope.
  A renderer DLL alone does not preserve its INI and asset dependencies.
- Do not edit another checkout, replace an INI wholesale, reset/stash away work,
  force-push, merge main, or deploy an old SIDE DLL over newer MAIN integration.
  Commit/push completed changes; a GitHub 403 means no PR was created.
- MAIN prepares the whole short headset route first. User stays in PSVR2 until
  normal exit; no mid-test chat/alt-tab. Prepare first, await the requested fresh
  `start` gate before launch. Verify actual exit, backups and hashes before any
  replacement; obey the accepted-profile guard and bounded restore lease.

## Deliver

Continue safe implementation/RE, not repeated speculative fixes or report-only
handoffs. Provide a compact outcome with source branch/commit, exact evidence,
candidate vs installed identities, remaining gates and the next justified step.
If handed this file outside the repository, locate the checkout from the
reference and verify it first; do not assume the attachment installs the skill.
