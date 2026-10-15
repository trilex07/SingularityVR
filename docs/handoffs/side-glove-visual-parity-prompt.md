# SIDE prompt: glove visual + size parity with native weapon-hold gloves

Copy-paste to a SIDE agent. MAIN owns Steam; SIDE verifies L1/L2 only.

## Goal (user-verified context)

Custom free-hand gloves render and animate (PR #30, default OFF,
`GloveRendererEnabled`). The user confirms they work, and asks: make them
look exactly like the in-game gloves seen while holding a weapon, at exactly
the same size.

## Reference: what "native" means here

- Held-weapon hands keep the authored pistol grip with `GloveIsolate=2`
  (stump-collapse forearm behind the wrist, `GloveStumpScale`), driven by the
  native UE3 skinned palette (`PrepareIndependentLeft` / `BuildLiveControllerC`).
  Do NOT change native gun or hand-palette transforms.
- Current baked pack: 817/826-vertex rigid HUD GLBs + authored open/fist
  morph, rendered at 52.5 mesh units/m with embedded native diffuse texture
  (`build/gloves/candidate`, `docs/glove-animation.md`,
  `spikes/view_matrix/glove_animation.h`, `glove_renderer.h`).
- The size gap is unmeasured: 52.5 u/m was an explicit offline argument, and
  no Sense grip-to-wrist mounting offset was ever measured.

## Tasks

1. Measure the size delta offline: baked `bounds_metres` (see
   `build/gloves/candidate/assets-report.json`) vs the native held-glove
   world size derived from code (wrist geometry, `MetresToUU`, glove isolate
   path) and existing captures under `references/` / `review-screens/`.
   State the falsifier first (e.g. which capture + which landmark would
   disprove your scale factor).
2. Close the appearance gap (diffuse/material response, cuff transition,
   finger proportions) behind the existing default-OFF flag only. No new INI
   keys without MAIN approval; no held-weapon/TMD path changes.
3. Keep it bounded: same 4-draws/frame budget, no per-frame allocation or file
   access, preserve the fail-safe (pack load failure keeps native hands).

## Constraints (authoritative, from AGENTS.md)

- Own isolated worktree/branch from `origin/main`
  (`git worktree add -b <branch> <path> origin/main`). Never touch
  `plates-l3-main` or any other worktree; never commit/push to another
  agent's branch.
- L1 (unit/math/geometry checks) + L2 (x86 PE32 `/O2` build, tests,
  provenance/SHA) only. No Steam launches/deploys, no headset claims.
- Push your branch, open a PR, do not merge it. Record evidence + limits in
  STATUS.md and the PR body.

## Handoff back to MAIN

Branch, HEAD, DLL SHA-256, L1/L2 logs, measured scale factor + the capture
that proves it, what still differs from native gloves, and the exact PSVR2
script for MAIN (three sentences max).
