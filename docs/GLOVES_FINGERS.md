# Gloves + finger posing (FP hands)

Branch `feature/gloves-fingers` (worktree `SingularityVR-hands`), based on
production `7275802`. Staged, tested, NOT wired into `d3d9.cpp` yet.

## What it does

1. **Glove isolation** (`spikes/view_matrix/glove_isolate.h`): hides the stiff
   forearm/upper-arm flesh and sleeve by collapsing their palette matrices onto
   the live wrist center (`S' = S * D`, `D = T(-w)*Scale(0)*T(w)`).
   PSK weight census: tmd mat0 has 611 ARM-weighted verts (991 all-arm tris),
   noTMD mat0 has 992 (1598 tris) — those collapse to the wrist point, wrist
   and finger vertices are untouched, boundary triangles degenerate.
   The **TMD device bones stay native** (user decision): `tmd_base`, `hinge*`,
   `rail*`, `disc*`, `nixie*`, `faceplate`, `wheel*`, `nub_*`.
2. **Finger posing** (`spikes/view_matrix/finger_pose.h` + generated
   `spikes/view_matrix/finger_table.h`): per-finger 0..1 curl, smoothed
   (~14/s), driving verified flexion axes baked from the bind skeleton (all 10
   fingers passed a numeric tip-toward-palm closure test). Applied as
   `S' = S * D` with `D = T(-c)*R(axis,+angle)*T(c)` in live joint space, to the
   joint and its descendants. Positive angle closes (verified offline);
   `FLEX_SIGN` flips globally if a headset check ever disagrees.
3. **Input** (`fingerpose::BuildInput`): index ← trigger value; middle/ring/
   pinky ← grip (squeeze force, else squeeze value); thumb rests when the stick
   (or thumbrest/buttons) is touched, relaxes otherwise. Every source optional.

## Files

| file | contents |
| --- | --- |
| `spikes/view_matrix/glove_isolate.h` | bone classification, collapse math |
| `spikes/view_matrix/finger_pose.h` | curl model, deltas, input builder |
| `spikes/view_matrix/finger_table.h` | generated axes/angles (finger_axes.py) |
| `tools/reference-tests/hands/glove_finger_test.cpp` | test (PASS) |

Test: `PASS: glove isolation + finger posing` (x86 `/O2 /std:c++17`).

## Integration (main agent)

1. At the existing arm-draw seam (the `FgMatchesSignature` + palette path that
   feeds `ApplyArmChain`), after computing wrist centers: for each palette slot,
   resolve the bone name via the section bone map + reference bones, run
   `gloveisolate::Classify`, and for `Collapse` apply `ApplyDelta(S, D)` with
   `CollapseDelta(wristCenter[side])`. Behind `GloveIsolate=2` (`1` = census
   log of would-collapse slots, `0` = off). `Review` bones are never collapsed.
2. Finger chains: `FindSlot` each `kFingers` joint bone by name; collect
   descendants with `IsDescendantOrSelf`. Per frame: sample OpenXR
   (`trigger/value`, `squeeze/force`→`squeeze/value` fallback,
   `thumbstick/touch`, `thumbrest/touch`, `x/y/a/b/touch`), `BuildInput` per
   hand, `SmoothCurls`, then for each joint `FlexDelta(center, Rot(S)*axis,
   curl*maxDeg)` and `ApplyDelta(S, D)` on the joint + descendants.
   Compose AFTER the IK wrist delta (fingers ride the posed wrist).
3. Draw signatures for the seam: tmd mat0 ≈ 2285 verts, noTMD mat0 ≈ 2586 verts
   (confirm via the existing census before enabling).
4. The game may already animate fingers; deltas compose on top (`S' = S * D`),
   so this works either way. If a headset check shows mirrored motion, flip
   the flex sign (single constant).

## Non-goals

No OpenXR hand-tracking dependency (Sense via SteamVR doesn't expose it);
procedural curl is the VR-standard fallback. No IB surgery. No mesh edits.
