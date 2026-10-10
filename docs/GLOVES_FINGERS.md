# Glove isolation (FP hands, no finger animation)

Branch `feature/gloves-fingers` (worktree `SingularityVR-hands`). Staged,
tested, NOT wired into `d3d9.cpp` here — production already carries this
module (`GloveIsolate=2`, see `docs/VR_STATUS.md`).

Scope change: finger posing was removed from this branch at the user's request
(the animation kept breaking). What remains is ONLY the forearm cut. Deleted:
`finger_pose.h`, `finger_table.h`, the finger test. No history was rewritten.

## What it does

Hides the stiff forearm/upper-arm flesh and sleeve by shrinking their palette
matrices toward the live wrist center (`S' = S * D`, `D = T(-w)*Scale(s)*T(w)`,
default `s = 0.12`). PSK weight census: tmd mat0 has 611 ARM-weighted verts
(991 all-arm tris), noTMD mat0 has 992 (1598 tris).

`s = 0` (old behavior) is BANNED by simulation: it kills 730/4402 triangles
around the wrist — over-cut rim + see-through palm. `s = 0.12` keeps a small
stump that plugs the opening with 0 dead tris (verified on the real PSK
weights). Tune via ini (`GloveStumpScale`); `s = 1` disables the cut.
No index-buffer surgery, so no dependence on runtime triangle order.

Kept by user decision: the gloves (wrist + finger bones, both hands) and the
TMD device bones (`tmd_base`, `hinge*`, `rail*`, `disc*`, `nixie*`,
`faceplate`, `wheel*`, `nub_*`). Unknown bones are never collapsed
(`Review` → census-logged, fail-safe).

## Files

| file | contents |
| --- | --- |
| `spikes/view_matrix/glove_isolate.h` | bone classification, collapse math |
| `tools/reference-tests/hands/glove_isolate_test.cpp` | test (PASS) |

Test (x86 `/O2 /std:c++17`): `PASS: glove isolation
(keep/collapse/review + collapse math)`.

## Integration (main agent)

At the existing arm-draw seam, after computing wrist centers: for each palette
slot, resolve the bone name via the section bone map + reference bones, run
`gloveisolate::Classify`, and for `Collapse` apply `ApplyDelta(S, D)` with
`CollapseDelta(wristCenter[side])`. Behind `GloveIsolate=2` (`1` = census log
of would-collapse slots, `0` = off). `Review` bones are never collapsed.
Draw signatures for the seam: tmd mat0 ≈ 2285 verts, noTMD mat0 ≈ 2586 verts
(confirm via the existing census before enabling).

L1 only (logic + math). Physical behavior (gloves visible, arms gone, TMD
intact, both eyes) needs a headset session — never claimed here.
