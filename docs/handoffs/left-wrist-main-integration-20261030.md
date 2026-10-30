# Left-wrist HUD — MAIN integration on retained HD candidate

## Scope and identities

MAIN branch: `integration/left-wrist-hud-retained-hd`.
Parent retained candidate: `dcbf7813`, code `e7e7b5f`, installed DLL
`0AAEAFAC7B230C80F922BD2EC533FDF711258524BD2BF35CAC4013F11E56DE0C`.
PR73 imported at pinned `ce163a15493f76061aea8a8ab9c60bd34a50c3c3`.
PR70 hand/shot-haptics, all HD follow-up repairs and PR71 trace-default-OFF are retained.
No new assets are needed; panel art is existing PR73 code-generated glyph geometry.

## Proven integration defects and minimal fixes

1. SIDE `DrawEquipmentWrist` required `VrModeOn()` and simultaneously rejected
   `g_dupDraws`, although `VrModeOn()` reads that exact flag. Preview was
   unreachable in stereo. A shared tested runtime gate removes this contradiction.
   Falsifier: runtime receipt still refuses mode while all actual gate inputs allow it.
2. The wrist producer omitted `moved=3`'s `BuildTmdAssemblyC`, used by the actual
   accepted dual-TMD renderer. It consequently published a wrist in a different
   space from the submitted glove. `FinalWrist` now composes native bind, final
   skin, section-local, **actual** ride, and inverse tracking map exactly once.
   Existing reload/grip transforms are not modified. Falsifier: final published
   wrist matrix differs from the submitted bind/skin/local/ride chain.
3. HUD pose publication now requires successful submissions in **each** eye,
   plus index/palette restoration and visible TMD glove segments. A successful
   left-eye HRESULT alone cannot authorize it. Reset invalidates published poses.
4. Missing optional HUD fields no longer prevent publication of independently
   resolved count fields. Unavailable values still remain hidden, never synthetic.
5. Joined both ProcessEvent name gates and both post-return observers, retaining
   shot-count witnessing and the natural TMD maximum observer independently.

## World-depth dependency — NOT YET PRODUCTION

`HISTORY.md` foreground depth-priority evidence and
`docs/shadow-clear-gate-experiment.md` establish clears before late first-person
rendering, including unrelated R32F shadow clears. Neither identifies a safe
present-time world-depth attachment for this new combined candidate.

No depth-copy hack, clear suppression, made-up Z constant or stale-pose scene
draw is introduced. The unchanged fail-closed `DepthEvidence{}` prevents
production mode and health replacement. Preview explicitly has no world occlusion.
**A live focused stereo frame is a specific unresolved technical dependency.**

`[Diagnostic] EquipmentWristTrace=1` arms once after a valid, both-eye visible
preview and tracking remain stable for 18 frames. The next frame records:

- Actual RT and DS identities, dimensions, depth format, multisampling.
- Before-depth-clear ordering, native Clear flags/Z/HRESULT.
- First per-eye world/foreground submissions after each clear, viewport/scissor,
  selected camera register/convention and actual final bound matrix.
- Successful wrist publication timing, controller sample and final tracking matrix.
- Present boundary; maximum 64 retained records, overflow reported.

No resource mutations, GetRenderTargetData, locks, copies, shader queries or GPU
waits. Records are fixed storage; COM references are released next Present/Reset.
The trace automatically ends and does not rearm on Reset. Logging occurs at
Present, never inside a draw. Diagnostic capture frame must be excluded from
steady-state timing comparisons. Identity/order evidence alone is not pixel proof.

## Live values

Reuse PR73 local controller→Pawn/sharedHUD and Pawn→TMD reflected producers.
`EquipmentWristTrace` additionally emits at most 32 changed value snapshots,
sampled no faster than 250 ms; owner IDs and availability flags accompany them.
At most 12 natural `GetTMDEnergyMax` return receipts report value and time.
There is no direct native getter invocation. A missing/stale maximum hides energy.
Collection/spend, depletion/recharge and item-consumption semantics remain live QA.

## Configuration for first consolidated check

```ini
[Render]
EquipmentWristHud=1
EquipmentWristPreviewNoDepth=1
EquipmentWristSuppressHealth=0
EquipmentWristPolish=1
EquipmentWristEnergy=1
EquipmentWristTech=1
EquipmentWristSupplies=1
[Diagnostic]
EquipmentWristTrace=1
```

Existing health and ammo displays remain authoritative. Unavailable rows stay
hidden. Optional production defaults in code remain OFF. Existing NIS/OLED,
reload, localization, grants and haptics configuration must remain unchanged.

## Offline verification

- `tools/tests/test-hud-live.ps1`: runtime gates, 12 final-palette/ride/rotation/unit
  cases, freshness/source lifecycle, field validation, projection/visibility,
  preview cannot suppress health.
- Its actual D3D9/D3D9Ex trace fixture: disabled/null-device early-outs, native
  depth identity/format, both scissor halves, clear sequence, unchanged RT/DS/
  viewport/scissor/matrix, overflow limit, missing depth, reference release/Reset.
- `tools/hud/test-diegetic-hud.ps1`: existing four-weapon mounted-HUD actual pixel
  and state-restoration regression. This is **not** new wrist-panel pixel proof.
- `tools/tests/test-independent-hands-shot-haptics.ps1` and
  `tools/tests/test-dual-weapon-tmd.ps1`: hands, haptics, OLED/WeaponLocker regressions.
- Optimized x86 PE32 `/O2` build and exact receipt follow in candidate JSON.

## One physical dependency session

After a fresh user `start`, use the hash-verified retained candidate; no chat
switches in goggles. In gameplay, raise the physical-left forearm inward and hold
for one second to arm the one-frame capture; rotate wrist near a wall and weapon.
Observe both eyes/readability. Use TMD to change energy; use a pack/vial or collect/
spend E99 if naturally available. Grip/release and perform a reload. Close normally.
Do not claim preview is occluded; through-wall visibility is expected at this gate.
The existing Kasimov/Centurion consolidated acceptance can share this session.

MAIN then analyzes the captured actual world-depth source and pose ordering,
implements the smallest evidence-supported occluded route, tests wall/weapon/
forearm overlap and Reset, and requests one targeted final occlusion check.
Only that result can authorize production mode and health suppression.

Current closure: integrated preview/evidence route; **production depth and live
PSVR2/data/performance acceptance remain PENDING**. No FPS/90 Hz claim.
