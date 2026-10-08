# R3 weapon wheel — 2026-10-08

The motion build now reserves the right stick click (R3) for a weapon wheel
while the player is in gameplay and holding a weapon.

- Hold R3 to open a small wheel projected above the tracked right controller.
- Move the right stick to highlight a sector; sector 0 is the currently held
  weapon and later sectors mean successive entries in Singularity's existing
  cycle-weapon action.
- The first pass renders stable numbered cards (`W1`–`W4`) rather than guessing
  inventory names. The native cycle remains authoritative; class-specific
  artwork can be added once one runtime log establishes the inventory order.
- Release R3 to commit. The mod emits the required held native cycle pulses,
  with release gaps between pulses. It does not scan GObjects or invent a new
  inventory API.
- L3+R3 remains the existing recenter/screenshot chord.
- R3 is not forwarded to the old anti-gravity row while the wheel is open or
  on its press/release edge.

The initial wheel has four sectors and can be changed in `[Input]`:

```ini
WeaponWheelSlots=4
```

The pure mapping test is
`spikes/view_matrix/tests/test-weapon-wheel.cpp`; it covers the dead zone,
up/right/down/left sectors and the 2..8 slot clamp. The overlay is rendered
only while the wheel is held, from the existing Present-time state-block path.

The candidate DLL and INI are in
`C:\Users\Trilex\vr-mods\deployment\weapon-holster-candidate-20261008\`.

## Steam smoke test

The Steam-installed build was rebuilt as PE32 `/O2` and launched from the
installed `Binaries` directory.  The startup log reached D3D9Ex creation and
settled at the desktop XRSim frame loop without an exception.  The signed INI
parser now reports `hip fwd/right/up=(4,18,-24)`; the old `GetPrivateProfileIntA`
path incorrectly printed a wrapped unsigned value for `HolsterUp=-24`.

The smoke run validates startup and frame pacing only.  It does not claim an
R3 open/selection event because no physical Sense controller was present and
`DesktopAutomation=0`; the first headset test remains the interaction proof.
