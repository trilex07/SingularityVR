# Controller steady aim and contextual HUD — 2026-10-20

## Evidence / hypothesis / falsifier

User accepted the native-aperture scope round broadly ("jest okej"), then
requested reduced controller tremor, health hidden during two-hand support,
ammo hidden at the optical lens, and R1 steady aim for two-hand Kasimov.
This is NOT quantitative 90-FPS acceptance. Its completed log is preserved at
`build/combined-followups/native-aperture-137143d/physical-feedback-1919.log`.
Game exit and restored accepted DLL7A1DE585...9495 / INI60224CA0...4ADC were
independently verified. No new Steam writes or launch in this implementation.

Observed source: DrawHealthPlate/DrawAmmoPlate have no two-hand/optical
exclusions. `g_twoHandActive` is the input latch's atomic result. The optical
near-eye policy is input-thread-owned: publish only its boolean for rendering,
clear before sync/focus/menu early returns, never read its mutable state on
the render thread. Health hiding also resets face-raise hysteresis.

Controller aim arrives directly from xrLocateSpace, with no tremor filter.
Hypothesis: small high-frequency pose changes can be attenuated without
altering HMD tracking. Falsifier: deterministic alternating noise is not
reduced, deliberate movement sticks, or optics/ballistics visibly diverge.
No claim that all shake originates in controller sensor noise.

## Implementation / boundaries

`[Input] WeaponSteady=0` is the default. The candidate enables it explicitly.
Controller-only adaptive position/quaternion filter: 18ms base time constant;
65ms while the named primary Use/grip action is held with two-hand Kasimov
(11389 vertices), right-handed mode, valid tracking and gameplay. No new XR
action polling, allocations, locks or engine offsets. The same filtered
controller pose feeds gun/arm transforms, ballistic direction, native FX and
scope capture. Gesture/reload/holster/UI processing precedes the filter;
knife and busy reload transactions bypass it. Raw motion-speed diagnostics
use a separate prior raw sample, not yesterday's filtered position.

Primary Use is the default right squeeze (Sense R1); consume its native
Use/reload output ONLY in this held two-hand sniper context. Other weapons,
one-hand use, left-handed mode and menus retain existing input routing.
Changing strong/mild mode, weapon/pawn, invalid tracking or >100ms gap reseeds
the filter. >4cm or >0.20rad changes snap immediately; speed-adaptive response
reduces lag on deliberate motion. No physical comfort/accuracy claim yet.

HMD/eye pose and compositor submission are NOT filtered. Existing engine POV
rotation-shake suppression remains unchanged; no experimental camera matrix
rewrites or translation-stabilization flags are enabled here.

## "Pad 11" investigation (not a global replacement)

Installed localization data `RvGame.int / RvUI / ARSniperRifleDesc` says:
"Press [dash] when scope is up to slow time." The input table binds `dash`
to left thumbstick click / XI_LTHUMB (Sense L3), not R1 steady aim. This is
evidence of a separate native slow-time instruction, NOT proof that the
specific observed Pad11 glyph came from that field. The completed session
contains no Pad11 producer receipt. Leave numeric pad aliases unchanged;
capture the actual localization section/key before replacing a native action.
Do not claim native slow-time under the custom PiP has been verified.

## Headset route after a fresh start

Load the sniper save. Raise the health wrist, then acquire two-hand support:
health must hide and return after release; moving into the optical eye box
must hide ammo, moving away must restore it. Compare gentle tremor with R1
released/held while supporting Kasimov, then make a deliberate large aim turn;
check target, scope, hit and VFX agree. Check Circle manual reload, one-hand
Use, pause/resume and return to menu. Stay in the headset until normal exit.
Report any lag/discomfort; do not smooth head motion to mask it.
