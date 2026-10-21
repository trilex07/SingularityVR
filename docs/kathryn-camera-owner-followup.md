# Kathryn / bound hands / gameplay readiness — 2026-10-21

## Failed physical candidate

User reports Kathryn remains in VR with shaking character/hands; bound hands
become freely tracked during escape; enemies attack behind VR loading. Scope,
weapon alignment/VFX and gore were NOT tested in this session.

Candidate 1161168 / DLL 5C9E6E62 was rejected. Private log:
`build/combined-followups/kathryn-native-1161168/physical-20261021-1413.log`,
SHA256 `5E785C98B022996954676A302F0EB1F3B85580B74E1A4D58A5E624A466B701F1`.
Official rollback and accepted-profile postflight passed; installed DLL
`EF3A1F3B9F0641A291F9285DD7283EC7D3B4433A8F87BC210DBC98B721B7B753`
and INI `55A3ED4DBC6E158F01A2219177D825C066E7306DABCE6ACFB1C7118EC9E3F659`
were independently rehashed. No audio changes in this followup; SIDE owns dubbing.

## Evidence and correction

The native-cinematic reader did not bind until late pawn acquisition (log8250,
elapsed14641), despite valid camera/event-controller objects. Its reciprocal
check depended on the previously discovered global pawn. FindController could
retain a readable same-class controller from the previous scene. The log does
not record that cached pointer, so stale-controller identity is a code-supported
failure mechanism, not a runtime-proven pointer mismatch.

Read the current RvPlayerCamera.PCOwner and require reciprocal
PlayerController.PlayerCamera. Cinematic polling now works even while that
controller is unpossessed. Pawn acquisition additionally requires RvPlayerPawn
ancestry and reciprocal Pawn.Controller. This also replaces a stale cached
controller for existing input lookup. No game object writes or new hooks.
The bounded scene witness records camera, cached/current controller, pawn and
the exact refusal stage; it can falsify the ownership hypothesis next run.

The log identifies the actual rope asset through Pawn.Mesh while the policy
never activates. The typed resolver accepted ObjectProperty only. Owned Engine.u
SHA256 `B6B481EFDC1FFDA164A24FB25F2FDF57783F2860D36EE1925A1DE6770B971A34`
proves Pawn.Mesh export19664 is ComponentProperty. The corrected resolver
requires ComponentProperty for Mesh, ObjectProperty for the other ownership
links, dimension1, element size4 and aligned bounded reflected offsets. It
retries missing discovery at most1Hz; successful offsets cache per class.
`tools/reload/audit_native_local_player.py --engine <owned Engine.u>` verifies
all six field types, including PlayerCamera export3921. No game assets shipped.
Exact wp_rope asset ownership remains the restraint criterion; the new bounded
identity receipt exposes the complete path and whether it matched.

The readiness log held an opaque curtain for5s after cinema exit, then rearmed
another3s after acquiring a different pawn, even with ready=3F. Native simulation
continued throughout. Remove the gameplay minimum hold and prevent completed
readiness from rearming on pawn acquisition. During active gameplay, paint only
a small VR INIT notice and preserve the scene. The old overlay never froze
native input: this correction removes blindness during combat, while current
controller acquisition addresses delayed input ownership. Physical movement
acceptance still needs the next run.

## Validation and falsifiers

CPU: current camera selects its own pair while old objects remain live;
unpossessed cinematic owner; invalid reciprocal/type rejection; restraint47;
packed cinematic bits/fingerprint; no readiness rearm after late acquisition.
Own D3D9Ex fixture: preserve scene pixels except the small notice; identical
per-eye notice; restore RT/depth/viewport/scissor; repeat after ResetEx. Existing
scene/cutscene/reload/hand-anchor regressions passed. Python native-action9,
cinematic3 and startup5 passed. Production x86 PE32 /O2 build passed, with
existing compiler/static-analysis warnings. This is offline/own-GPU validation.

Next physical run: load the Kathryn rescue save; stay in headset through the
scene and escape; check cinema, character/hands, bound hands until native
release, visible world and immediate control after cinema. Exit normally.
If valid bCinematicMode remains0 during the faulty scene, inspect its actual
view-target producer rather than adding a timer/Matinee-name classifier. If
restraint identity matches active1 but hands remain wrong, inspect the native
draw ownership path rather than broadening the mesh criterion. Preserve the
combined scope/native-axis/VFX/gore settings for optional later checks.
