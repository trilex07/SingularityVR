# Centurion aim and TMD — SIDE evidence receipt

Baseline: `9aa6070df3fca86b25b9ee216474f4a2818c3e1c`. Steam and installed
configuration were not changed. This branch is **not a physical aim fix**.

## Centurion

The real outgoing collision ray runs through `Hook_SingleLineCheck` during a
native fire window. It is translated to the hand/gun pivot and rotated by the
controller deviation; the original native collision function remains in charge.
The existing measured-muzzle witness is available only for Valkyrie, Volk and
Kasimov (`weapon_muzzle_geometry.h::RootPoint`). Centurion (mesh 5799) is not
represented. `BuildGunC` additionally applies a Centurion yaw trim to the
visible mesh only. Thus enabling `WeaponMeasuredAim` cannot prove or correct
the Centurion's barrel/collision agreement.

The archived 2026-10-22 `view_matrix_2026-10-22_23-13-09.log` has measured
long-gun ray receipts (`aimDot` near 1), but no corresponding Centurion
measured-barrel receipt. It cannot establish whether the pistol mismatch is
mesh axis, controller-to-native mapping, parallax, spread or recoil.

`[Diagnostic] CenturionAimProbe=1` now records at most 16 local Centurion
collision rays, controller direction and transformed mesh +X. It changes no
ray, damage, spread, recoil, mesh or inventory state. Mesh +X is explicitly a
comparison basis, not an asserted muzzle socket. The falsifier for a
controller-ray mismatch is a tight nativeRay/controller angle while visible
impact remains away from the barrel. If the angle is large during still,
single shots, investigate the native trace rotation; if small, resolve the
original Centurion muzzle bone/socket before modifying collision rays.

## TMD

Current Sense mapping in `g_padButtons`: left L2 trigger -> native `Impulse`;
left Square -> native `AgeYoung`. Native power selection and energy gates stay
unchanged. The TMD page now displays these verified controls as read-only
help. It does not claim other powers have one-button Sense bindings.

`BuildTmdC` follows the tracked off-hand pose with independent
`TmdFwd/Right/Up/Yaw/Pitch/Roll` trims and the common gun mesh scale.
`TmdNativeAim` is already available but default OFF and requires separate
physical proof; turning it on is not a visual repair. This branch does not
change the TMD mesh transform: the supplied feedback does not localize the
visible offset to a specific bone, material, pivot or scale, and a guessed
trim would make the glove worse in another wrist orientation.

## Review disposition

The TMD help and bounded aim probe can be reviewed independently, but this
branch is **NO-GO as the requested Centurion/TMD visual fix**. The earliest
missing evidence is Centurion muzzle/bore identity plus a same-shot native-ray
receipt, and a reproducible TMD wrist-versus-device offset measured at two
controller rotations. No Steam or PSVR2 validation is claimed.
