# Hands-only gloves and pause-menu text

Status: IMPLEMENTED_NOT_VERIFIED. L1 covers the section rule and the clip composition. The Steam session below shows tracking and the existing grip latch. It does not show that pause text moved, and it is not a headset acceptance.

## Why the arm-IK test showed no hands

`ArmIKMode` does not write controller tracking. Tracking comes from `xrLocateSpace`: orientation valid and position valid are separate flags, and both hands were invalid for the whole `ArmIKMode=1` sample (`handsymmetry valid 0/0`). Independent-hand bone updates return before any section work when both hands are invalid, so controller deltas were never applied. The test ini also set `ShowControllerArms=1` and `GloveIsolate=0`, which turns off the production glove hide. The latched mesh was the pistol glove (`3314` verts), not the TMD arm sections, and `mPlayerHandsStatus` stayed "weapon in hands". Missing hands in that run are a tracking and ini result, not an IK result.

## Hands-only

The long forearm is the section at first index `13206`, `7704` triangles. Hands-only drops that draw. It does not rewrite that section's palette. The glove section (first index `0`) stays, and each hand keeps its own controller delta. The short cuff is the existing stump scale toward the wrist on forearm bones that share the glove draw. Elbow IK does not run while `ShowControllerArms=0` and `GloveIsolate>=2`.

Weapon state is the existing support-grip latch and hip holster:

- Holstered: the support latch is reset, so both hands stay on their controllers.
- Equipped: the weapon follows the aim hand. The support hand stays independent until grip acquires.
- Left Grip near the weapon: the off-hand delta is replaced by the weapon-specific support point. Both glove draws stay visible.
- Grip released: the latch clears on that sample and the off hand uses its controller again.
- Holstered again: the weapon gate fails, the latch resets, and both hands are free.

## Pause text

VR Settings is drawn at Present from normalized canvas coordinates on the head plane, once per eye. Pause GFx text is a canvas orthographic matrix in `c5` (`m33=1`, pixel/twip diagonal). That draw was reaching the camera split, which scissors each half and does not remap `c5`. A glyph on the left of the full frame is visible in the left eye only, and a glyph right of the seam is visible in the right eye only.

Class 3 draws with that matrix now take the existing HUD eye copy: scale about the origin by the class-3 inset, then `x' = 0.5x ± 0.5`. The VR Settings row is a different function and is not moved.

## Steam session, 2026-10-12

Candidate `65D2039EF988AE325784751D1C70D73BFDEACF35D9A1DFDD7AF62A22DEAB3714`, x86 PE32, `/O2`. Installed through the deploy tool after a timestamped backup. The installed INI was left as-is (`ShowControllerArms=0`, `GloveIsolate=2`, `IndependentHandsPrototype=1`, `ArmIKMode=0`). OpenXR runtime was SteamVR. The game was launched with `steam.exe -applaunch 42670`. The DLL and INI were restored afterward and both hashes matched the pre-test files.

At startup both hand profiles were unbound and `handsymmetry` was `valid 0/0`. The head pose was already valid. After SteamVR bound `/interaction_profiles/oculus/touch_controller` on both hands, `handsymmetry` was `valid 1/1` with independent yaw and pitch, and the independent-hand audit reported successes with no position blocks. Glove isolation mode 2 ran. The latched arms mesh was the 3314-vert pistol gloves. Grip acquire/release happened three times with no latch error, and holster store/draw logged that the glove draw stayed independent.

Pause opened once. The VR Settings row logged the pause band V 0.60–0.70. Class 3 draws that reached the HUD path were on a non-backbuffer target (`menu mono`), so they did not take the new canvas-ortho eye copy. The sampled pause frames did not contain the `1/12800` font matrix. Pause-text placement still needs a headset look.
