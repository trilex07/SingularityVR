# Weapon holster candidate — 2026-10-08

This candidate adds a body-relative motion-controller holster without changing
the existing two-hand support-grip latch.

## Runtime behavior

- The primary hand (right by default, or the configured aim hand) is sampled
  through the existing OpenXR grip action.
- A **grip rising edge** is accepted only when the hand is inside the configured
  hip radius. The first edge stores the equipped weapon; a later edge at the
  hip draws it again. Holding grip does not repeat the toggle.
- While stored, the gun transform and `GunPivotWorld` use the fixed body-yaw hip
  point. The weapon remains rendered there while the controller hand is free.
- The gameplay fire trigger is suppressed while the weapon is stored. The
  support-hand two-hand latch is also disabled until the weapon is drawn.
- The state is reset on tracking/camera loss and when the equipped weapon
  identity changes. Menus do not move the weapon or create a second input path.

The hip defaults are `fwd=4`, `right=18`, `up=-24`, radius `12` UU. They are
configurable in `[Input]`:

```ini
HolsterEnabled=1
HolsterFwd=4
HolsterRight=18
HolsterUp=-24
HolsterRadius=12
```

## Validation

Pure state and geometry tests are in
`spikes/view_matrix/tests/test-weapon-holster.cpp`. They cover radius edges,
body-yaw rotation, rising-edge semantics, hold behavior, re-entry, weapon
change, and tracking reset. The executable prints:

```text
weapon holster: radius, rising-edge toggle, hold, re-entry, weapon/reset gates
```

The existing runtime, interaction, motion, transition, reference-integration,
VFX eye-scale, and foreground-boundary tests also pass after the integration.

## Candidate build

- DLL: `C:\Users\Trilex\vr-mods\deployment\weapon-holster-candidate-20261008\d3d9.dll`
- INI: `C:\Users\Trilex\vr-mods\deployment\weapon-holster-candidate-20261008\SingularityVR.ini`
- SHA-256: `AFDC7AF015EA1C1D20AA621D1A743915AFED8A3DA88A8C8EEB5BB8BBBA495B44`
- PE32: machine `0x014C`, optional header `0x010B`

This is a candidate only; the currently deployed Steam DLL was not replaced.
