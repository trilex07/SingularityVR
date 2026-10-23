# TMD SVTG mixed-palette correction (offline)

Base: `9137dc5`, draw-route audit in `sol-dual-tmd-draw-route-audit-20261023.md`.

## Matrix-level cause

The original material-1 palette has 15 slots (`kTmdBaseMap`). It contains
`tmd_base=27`, `l_arm_elbow=8`, `l_arm_forearm2=46` and `sleeve_fix_1..5=43,44,45,42,41`,
but not `l_arm_wrist=9`. In `PrepareIndependentLeft`, hardware bones receive the
physical-left controller delta while the arm/sleeve branch requires a wrist
slot and therefore leaves those seven matrices in native space. The private
PSK/SVTG verification counts 414 of 583 short-sleeve/cap triangles with
weights on both groups. This is a split skinning transform, not a rigid object
offset.

`skeletal::LocalDeltaPair` produces row-vector rigid deltas. BoneMatrices is
stored as three output-component GPU rows; `skeletal::TransformBone` is the
existing postmultiplication used for native TMD hardware. Applying it to each
*original* sleeve skin matrix, once, preserves each bone's bind/animation
transform and makes the weighted surface consume the same delta as hardware.

## Scope and failure behavior

The staged 45-register palette is built only for the exact material-1 TMD
section with valid physical-left tracking and the audited 15-slot map. It is
uploaded only after the 7110-triangle SVTG index buffer binds successfully.
Invalid tracking at commit or a failed palette upload aborts that draw; index
and original palette restoration still run. Missing/corrupt SVTG or failed
index binding keeps the existing 49-run, 6527-face hardware-only fallback with
its original palette. No material-0/2, gun, IK, two-hand or shadow path changes.

## Validation and remaining gate

`test-tmd-sleeve-palette.cpp` checks two wrist rotations, all seven slots,
native animation preservation, no double transform and fail-closed commit
gates. `test_tmd_sleeve_palette.py` uses private original PSK/SVTG to check all
414 mixed-weight faces at two rotations. These are L1 math/asset tests, not
headset proof. The one-shot runtime receipt now includes `sleevePalette=7`.

The short-gauntlet suite, dual-weapon suite and OLED regression suite passed.
The D3D9/D3D9Ex pixel fixture was attempted but could not create its first
D3D9 HAL device on this host, before any SVTG code ran; its pixel gate is
NOT RUN here and remains for a GPU-capable environment. This is not a claim of
physical visual correctness.

MAIN PSVR2 acceptance: compare the same raised-left-wrist scene with the
short-gauntlet flag OFF/ON, rotate the wrist twice, then hold/release left Grip
on a right-hand weapon. ON must show a closed short cuff with no stretched long
forearm in either eye; native moving TMD parts and right-hand gun must remain
intact. Inspect the `sleevePalette=7` receipt and both restoration statuses.
