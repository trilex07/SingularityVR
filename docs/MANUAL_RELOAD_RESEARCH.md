# Manual reload research (2026-10-08)

This workstream replaces the current `Use / reload` button gesture with a
physical controller gesture. It does not write ammo directly and does not
change the production `d3d9.cpp` hook yet.

## What the game already provides

The installed `RvGame.xxx` was decompressed and its UE3 export table was
checked. `RvWeaponShared` contains the native reload state and entry points:

- `AttemptReload`
- `ServerStartReload`
- `IsReloading`
- `Reload.BeginState`
- `Reload.ReloadFinished`
- `Reload.InterruptAllActions`
- `Reload.ShouldDeferFire`
- `ReloadFinished`
- `RefillClip`
- `AddShellToClip`

`RvSharedPawn.AnimNotify_RefillClip` is also present. This is strong evidence
that the safe path is to enter the game's own reload state and let its
animation notify update the clip. Calling `RefillClip` or `AddShellToClip`
directly would bypass state, animation, interruption and weapon-specific rules.

The current mod maps native X to the OpenXR right-hand squeeze action. The
original Raven mapping labels X as `Use/reload`; there is no separate shipped
`GBA_Reload` action in the current input table. Therefore the first runtime
instrumentation should compare the following function entries when one X
press is made: `AttemptReload`, `ServerStartReload`, `IsReloading`,
`ReloadFinished`, `RefillClip`, and `AddShellToClip`.

## Proposed gesture

The right hand remains the primary weapon hand. The left hand continues to
acquire the existing two-hand support grip at the fore-end. Reload recognition
is enabled only when that support latch is **not** active:

1. Move the free left hand into the weapon's reload socket (magazine or
   charging-handle anchor).
2. Pull it away far enough to pass the eject/pull threshold.
3. Return it to the socket to complete the insert gesture.
4. Emit one native reload request. The game owns the actual animation, ammo
   refill and completion timing.

The recognizer in `spikes/view_matrix/manual_reload.h` is intentionally pure
math/state. It produces a single request on the pull-away-to-return edge and
has guards for tracking loss, firing, support-grip ownership, weapon changes,
timeouts and repeated held poses. It does not require a button.

The thresholds are starting values only (11 cm approach, 16 cm pull, 10 cm
return) and should be calibrated per weapon profile after one headset pass.
Weapon profiles should eventually provide the socket transform and gesture
type: magazine insert for pistols/SMGs, charging-handle pull for rifles, and a
shell cycle for shotguns. The first implementation should still invoke the
native reload state rather than manipulate individual weapon bones or ammo.

## Native invocation order

1. Instrument `ProcessInternal` for the six reload names above while X is
   pressed and while an empty weapon is fired.
2. On a completed gesture, prefer the existing one-shot `Use/X` action edge if
   it reaches `AttemptReload`. This preserves all game-side validation and
   avoids guessing the UE3 `FFrame` ABI.
3. If a weapon does not accept that edge, resolve and call the confirmed
   `AttemptReload`/`ServerStartReload` function only after the parameter layout
   is captured. Do not call `RefillClip` directly.
4. While the native reload state is active, suppress firing and use the
   existing `OnReloadStarted`/`OnReloadFinished` PSVR2 haptic hooks. Restore
   firing only after `ReloadFinished` (or a proven `IsReloading == false`).

## Deterministic checks completed

`spikes/view_matrix/tests/test-manual-reload.cpp` covers approach, pull,
insert, one-shot request semantics, timeout, support-grip exclusion, firing
exclusion and weapon-change re-arming. It is a desktop-only test and does not
need a headset, OpenXR runtime or DirectInput mouse.

The code is deliberately not wired into the production DLL in this checkpoint:
the next safe change is runtime call-path instrumentation, followed by a
single integration point in the existing input/weapon state code.
