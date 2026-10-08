# Left-hand pose fix — 2026-10-08

The active Steam configuration had `AimMethod=1` but did not contain
`IndependentHandsPrototype=1`. The native independent-hand path is explicitly gated by that
setting, so the left-hand mesh stayed on the game's authored animation even though the left
controller button actions were being read.

This iteration keeps the tested native-hand path and adds two runtime-side safeguards:

* the `handpose` action now accepts both `aim/pose` and `grip/pose` for each hand. SteamVR's PSVR2
  compatibility layer has versions where the left Sense controller exposes grip pose while aim pose
  is inactive;
* a pose-only `khr/generic_controller` profile is suggested as a fallback. It does not suggest
  generic face-button paths, so it cannot invalidate the normal Touch/Index input tables.

The frame diagnostic now reports action-active, orientation-valid, and position-valid separately for
each hand. This distinguishes an unbound controller from a valid pose whose positional tracking was
lost.

The deployment INI used for this candidate has:

```ini
IndependentHandsPrototype=1
ShadowMode=2
VrShadows=0
VfxHost=0
VfxDiag=0
HideMuzzleFx=1
```

The shadow/SFX experiments remain in their existing candidate directory and are not enabled by this
build. The old active Steam DLL and INI are copied to a rollback directory before deployment.
