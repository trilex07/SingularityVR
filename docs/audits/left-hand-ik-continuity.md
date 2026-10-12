# Left-hand IK continuity

Status: `IMPLEMENTED_NOT_VERIFIED`. Branch `feature/left-hand-ik-continuity`, cut from `origin/main` (`e07367a`). The pause-menu experiment is not part of this branch. No Steam deploy until a headset pass is approved.

`ArmIKMode` still defaults to 0. `IndependentHandsPrototype` still defaults to 0. This change does not alter those defaults.

## What changed

`armnatural::Build` takes the caller's `armik::State` and writes it only after the solve and every bone delta succeed. The bend seed is the authored shoulder–wrist plane (`PreferredBend`), not the world-Z fallback.

Physical left and right each keep their own state. Section 0 (gloves) and section 1 (arms) can both enter `PrepareIndependentLeft` in one frame. The second call with the same `g_armPaletteFrameToken` and the same mesh delta reuses the first solve and does not advance the elbow. A different mesh delta in that frame is rejected and the first solve is kept.

A hand with `handValid[side] == false` does not run IK. Only that hand's state is cleared. The other hand keeps solving. Both states are cleared when the pawn pointer is lost, on `ResetVrAfterCameraLoss`, on every stage rebuild (`g_haveCentre = false`: accepted recenter, camera jump, VR-mode toggle, F1, F10), and when `ArmIKMode` changes.

## Visual test configuration

Set these in the game ini. Do not treat any other combination as this test.

```
[Render]
IndependentHandsPrototype=1
ArmIKMode=1
ShowControllerArms=1
GloveIsolate=0
```

`GloveIsolate` defaults to 2 and, with `ShowControllerArms=0`, hides the native forearm section. `ArmIKMode=2` is the gloves-only suppress path, not this IK. `ArmIKMode=0` must look like today's native arms.

## Headset procedure

One question: with the ini above, does each elbow stay on its authored side while that controller moves, including when the arm passes near straight, and does the other arm stay still when one controller loses tracking?

False pass: the elbows move but pop to the opposite side at full extension, or both arms freeze when only one controller is occluded.

## Automatic evidence

L1 `test-arm-ik-continuity` and `test-reference-integration` both print `PASS`. The continuity file covers authored left/right poles, fresh-versus-retained elbow disagreement at the pole, a smooth pole crossing with no consecutive bend flip, one state advance for two sections in a frame, per-hand tracking loss and recovery, a failed solve that keeps the previous chain, a camera-style reset, and `ArmIKMode=0`.

L2 `spikes/view_matrix/build.ps1` produced an x86 PE32 `/O2` DLL (`machine=0x014C`, optional header `0x010B`). SHA-256 `9C5CF444480566D2983256E0A77082C59B433582634B60A471A3460391A90EF7`. Not deployed. Not headset-verified.
