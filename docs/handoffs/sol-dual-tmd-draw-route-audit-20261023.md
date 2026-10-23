# Dual-TMD draw-route audit after three PSVR2 failures

Status: **investigation only**. No renderer, geometry, IK, input, Steam, or
installation changes on this branch. `d771c40`, `a2244f4`, and `3f1399f`
failed physical acceptance; `7e7d8208` is the standalone short-gauntlet
checkpoint, not proof that its dual-weapon mode works.

## Evidence identity and limits

- MAIN's failed-session handoff is
  `docs/handoffs/sol-geometry-3f1399f-psvr2-fail-evidence.md` in the MAIN
  checkout. Its archived `view_matrix_full.log` has SHA-256
  `B4023AAFE259CF5E85404A7EDD79EDEF1E661912EB597CA8C20E07DE7BFAD3A9`.
  Failed DLL: `96375E307A64368A2E8CE765CC6DD16A38BE8C54E800B755DE777F7CA3192C43`.
  MAIN reports verified rollback to `7e7d8208` / DLL `C075677A...33BC8`.
- Original private `ch_inview_tmd_mesh.psk` SHA-256:
  `F64F5EE7ABD6A2DFDA0A686B3FF16C4EAD13325D4CEB352D987C8895EF269A5B`.
  The private SVTG SHA-256 is
  `452EA0C0DDFB4721A7F9B9718629975CF021F9E079E1CD92235C076E9E376192`.
  `prepare_tmd_short_gauntlet.generate()` rechecked the unique cooked index
  sequence against `RvGame.decompressed.xxx`; proprietary assets remain outside Git.
- No physical screenshots or final GPU pixel/vertex readback were in MAIN's
  archived evidence. The route log is one-shot per state/material/outcome, not
  a continuous census. It records source IB/VB **after** `IndexGuard.Restore()`,
  not the index buffer that was bound during the draw. These limits matter.

## Source-to-stereo pipeline

`Hook_DrawIndexedPrim` sees the original TMD shared VB/IB, calls `FgHidden`,
selects `RideTargetFor`, reads and changes the 5-bone shader palette in
`PrepareIndependentLeft`, then constructs an optional SVTG/SVNC `IndexGuard`.
Inside `StereoPair`, each eye dispatches the bound SVTG (7110 faces) first,
otherwise a bound native-cuff SVNC, otherwise the 49-run hardware cut (6527
faces) for material 1, otherwise the remaining `nSeg` source range. The guard
then restores the source IB and the original palette. See
`spikes/view_matrix/d3d9.cpp:19971-20025,20253-20365,20396-20480`.
The source TMD VB/IB in the failed session was `vb=2A3EE0E0`, `ib=0D020F80`,
stream `0/32` for all three signatures; this is stronger than vertex count
alone. Runtime pointer identity is session-local.

| Original material / source signature | Observed hands / grip and route | Applied palette / final index range | Eyes and long-geometry risk |
|---|---|---|---|
| 0, `first=0`, 4402 faces, `min=0`, 7444 verts | `hands=1,support=0` at log 11180: `native-full`; `hands=1/2,support=1` at 18854/19026: `native-full`. BUT log 11222/11226 and code show the native-cuff SVNC path can bind here. | Independent palette `108` registers at support (36 slots); `kTmdGloveMap` includes left wrist bone 9. In the proven full-cuff draws the bound range is SVNC `first=0,3994` faces, not native 4402. At support the bounded SVNC success log is exhausted, so exact bound range is **not recorded**. | `StereoPair` normally calls each eye; route `stereoEyes=0` does not count this branch. Offline SVNC preserves a compact left glove (5.29 UU past wrist) and right cuff (10.76 UU), removing a 436-face separate right strip. No original long left forearm in this bound selection; runtime skin deformation remains unmeasured. |
| 1, `first=13206`, 7704 faces, `min=2626`, 4818 verts | `hands=1,support=0` at 11224: `svtg`, following exact `FgHidden` admission. `hardware` fallback at 11150 and 18490. `native-full` at 9749/11181 before full eligibility. During support at 18855/19027: `zero`, `segments=0` (intentional hide). | `45` registers (15 slots) when independently prepared. SVTG bound range `first=0,7110` faces, vertex indices 2743..7322; 6527 hardware + 560 short-sleeve + 23 cap. Hardware fallback: 49 original-index runs, 6527 faces. Native-full: original `first=13206,7704`, including 1177 arm/sleeve faces. Zero: no draw. | Log 11224 is one-eye success (`stereoEyes=1`), while later 11260 separately proves a **two-eye** SVTG draw with IB/palette restoration OK. Neither proves every subsequent frame. Full 7704 can contain the 280-face long island; its one-shot observations are outside the clearly focused raised/support route, but later occurrences cannot be ruled out by the one-shot log. SVTG excludes that island, yet its short sleeve can still stretch through mixed skin matrices (below). |
| 2, `first=36318`, 17 faces, `min=7407`, 37 verts | Native-full initially at 9750/11182; hidden at 9771,11225,18856,19028. | Original 17-face right-forearm tail when native. In SVTG gameplay it is hidden after successful base receipt or by hands-only selection. | Eye count is not logged. This 17-face right tail cannot account for a long **left** gauntlet, though its transient visibility is not pixel-verified. |
| Weapon mesh (separate signature, `gunVerts=8297`) | At support, census counts gun draws; `gunRide` is nonzero. | Gun uses its own foreground/BuildGunC path, not the TMD shared VB/IB. | `reachedStereo` is a **pre-submission counter**, not proof of a visible supported gun. |

`route=native-full` is a diagnostic label for `nSeg>0` when neither SVTG nor
material-1 hardware fallback was selected. It **does not** distinguish a
material-0 SVNC draw from the source 4402-face draw. Likewise `stereoEyes`
counts only successful SVTG submissions. `dual-tmd census: reachedStereo`
is incremented before `StereoPair`; it even counts material-1 at support when
the later route is `zero`. Thus neither counter is a pixel receipt.

## First confirmed violated contract

The 7110-face SVTG is not a rigidly retargeted object. Re-running the asset
generator and inspecting PSK skin weights shows its 560 original short-sleeve
faces plus 23 cap faces contain **414/583 triangles weighted both to TMD
hardware and to arm/sleeve bones** (241/560 among the source sleeve faces).
Examples: `tmd_base` appears on 406 source sleeve faces,
`l_arm_forearm2` on 287, `l_arm_elbow` on 85, and `sleeve_fix_*` on many.

On this exact material, `kTmdBaseMap` has `tmd_base=27`, `l_arm_elbow=8`,
`l_arm_forearm2=46` and sleeve-fix bones 41-45, **but no `l_arm_wrist=9`**
(`d3d9.cpp:17278`). `PrepareIndependentLeft` finds `wristSlot[0]<0`. With the
failed INI's `ShowControllerArms=0`, `GloveIsolate=2`, it rigidly transforms
the `tmd_base`/hardware skin matrices through `IsTmdDevice` but skips the
arm/sleeve skin matrices because that branch explicitly requires
`wristSlot[physicalSide]>=0` (`d3d9.cpp:17769-17849`). Mode 2 does not repair
them through stump collapse (`d3d9.cpp:17434-17437`). **Adjacent vertices of
one submitted SVTG sleeve therefore consume different coordinate deltas.**
This is a proven palette/skin contract violation, irrespective of what the
final pixels look like.

`a2244f4` removed the exact TMD section's inherited gun/TMD camera ride,
selecting independent pose ownership (`RideTargetFor`,
`d3d9.cpp:17152-17168`). In the standalone `7e7d8208` source, that early
no-ride rule does not exist. The integration thus changed the camera/world
composition while leaving the material-1 split palette unchanged. This
explains why the mismatch can become conspicuous in the dual build even
though the standalone asset and its index cut were visually acceptable.
The **visual** attribution of the user's elongated pixels specifically to
these 414 mixed faces remains an inference until a per-draw GPU capture or
before/after PSVR2 check; the palette violation itself is proven offline.

At Left Grip, log 18851-18855 proves `hands=1`, `support=1`, material 0 still
submits while material 1 is zero. Log 18853 confirms the new fore-end-target
branch *did* fire for `gunVerts=8297` in `HANDS_TMD`; the older authored-pair
receipt at 19025 belongs to the later `hands=2` state. The weapon's native
gun draw and the TMD-pose glove are therefore sourced from different pose
owners during the first support state. This is a second verified presentation
contract mismatch; whether it alone explains the photographed grip failure
is not provable without synchronized pixels/palettes. `3f1399f` fixed a
target choice, not this source-pose mismatch.

## One minimal, falsifiable correction to review — **not implemented**

For the exact material-1 SVTG path only, apply the same left-controller wrist
delta already given to TMD hardware to its *selected short-sleeve and cap
influencing arm bones* (`l_arm_elbow`, `l_arm_forearm2`, `sleeve_fix_1..5`).
Do not alter the source indices, 49-run fallback, native animation, right
weapon path, or general IK. Recompute the actual GPU palette on a raised TMD
draw at two wrist rotations; all skin contributors to each of the 414 mixed
faces must share the same rigid controller delta, with the hardware's native
relative animation preserved. A bounded per-eye capture of the final bound
IB and palette (or a one-variable PSVR2 A/B) then tests whether the elongated
shape disappears while support remains separate. **Falsifier:** if the GPU
palette already gives those arm bones the same delta, or if the stretched
pixels persist after equality is verified, this hypothesis is insufficient;
identify the contributing draw from a pixel/frame capture instead of adding
another offset or hide rule.

Do **not** deploy this report as a fix. The support-pose mismatch also remains
open; no UI/weapon/IK changes are authorized by this investigation.
