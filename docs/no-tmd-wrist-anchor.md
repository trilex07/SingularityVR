# Own-wrist anchoring experiment

PID21056 physical feedback: frontend menu is single with matching eyes, but its
background is incomplete. Cinema pause was not exercised. Small FPS dips remain,
unrelated to L2/R2 in this trial. Native gameplay speech is still English. All four
leased installation files and all 1886 WAV originals were restored and independently
hash-verified after exit. No next Steam deployment or launch without fresh `start`.

## Evidence and hypothesis

The existing controller transform rotates/scales about the gun anchor, not the
native wrist. Skin constants are inverse-bind times animated joint. Reconstructing
each joint from the verified noTMD reference, then applying original local230,
reveals the wrist's residual offset after the gun-pivot controller transform.

Eight bounded samples were recorded; five were cinematic, only three gameplay.
They must not all be described as near-face gameplay captures. Frame17519 has no
camera ride and reconstructs residuals of 49.997UU left and 40.844UU right
(approximately 0.76m/0.62m at this user's 65.625UU/m). Frames17194/17356 used a
ride, which the old probe did not copy: their replay assumes the copied controller
transform equals the ride. They are weaker evidence, not independent ride proof.

Hypothesis: using each reconstructed wrist as its own controller pivot removes
the measured gun-anchor residual. Falsifier: the tracked wrist still misses the
physical controller despite the recovered joint reaching the logged target. That
would require checking bind/local/coordinate convention or controller origin,
not guessing another distance offset. New probe records the actual camera ride
and captures only GameplayStereo, outside menus/pause/cinematics.

## Narrow implementation

`NoTmdWristAnchorFix` is default OFF. It applies only with NoTmdPaletteFix,
GloveIsolate2, both tracked hands valid, exact noTMD gloves in gameplay, and no
support grip. Controller rotation/scale is preserved; only pivot translation is
adjusted. Undo the actual BuildGunC/BuildTmdC ride, not its live-controller
surrogate. Invalid matrices fail closed before upload.

The existing left parking shift is bypassed only in this scope because it would
move the newly anchored wrist away from its controller. The user's INI value12
is retained. Gun, TMD, support grip, shadows, world, menus and cinema paths are
not changed by this flag. Existing palette restoration remains authoritative.

L1: synthetic reconstruction/rotation/scale/ride cancellation/fail-closed/gating
tests and the three-record private replay pass. Corrected replay residual is
below0.00002UU. This is algebraic validation, NOT GPU pixel or PSVR2 acceptance.
L2 build and physical own-wrist acceptance remain pending at this note.
