# Centurion second physical feedback — 2026-10-17

SIDE, zero launches/deployment/installed-file writes. User rejects rightward
opening, travel on tilt, invisible/unusable shoulder ammo and gun drifting when
empty. New decision: Circle OPENS the cylinder; tilt only dumps the cases.

## Evidence before intervention

Read-only snapshot of the actual last session, header12:07:36, ending12:09:57:
`build/centurion-reload/feedback-20261017/session-1209.log`, SHA-256
`E385B6E7C9A69456D6ECBC1BE0A06CA5FB062AD44B965C248B1E87363EAF9906`.
VisualFollowup=1. HUD reseat at line5735 changed the zombie instance; later
ammo reads are valid. Circle/open/dump actually ran. The log repeatedly reports
grab-fresh32 times behind physical LEFT shoulder (5.5–12.9cm), then discarded32 times at
38.5–100cm from the AUTHORED controller socket. No successful insertion shown.
Thus "no grab" is not established: the input DID grab, whereas rendered carried
content and physical fit are not established by the draw counter.

Structural issues in the tested source:
- Circle enters Unlocked; lateral/roll gesture is still required before dump.
- Frame12 native PSA translation chooses its own model-space side; its small
  translation norm does NOT prove physical-controller-left opening.
- RootHold freezes only the model-space parent skin matrix. The shader still
  multiplies the CURRENT LocalToWorld and BuildGunC; that composite is NOT held
  relative to the controller. A changing native parent/component/view can move
  the rendered gun while the input socket remains a separate authored offset.
- The old socket is controller +(-35,-35,-80)mm, NOT the rendered open joint.

## Hypothesis and smallest reversible experiment

Separate default-OFF `CenturionTrackedCylinder` (requires VisualFollowup and
CylinderAnimation). Capture the original gun parent relative to the tracked aim
controller through the FULL existing shader chain once per transaction; invert
the CURRENT chain every draw. Hold the parent/controller relationship, not a
matrix in an unrelated space. Preserve native ammo/backend and replay material.
Use a compact physical-left translation for the original rigid clip, scaled by
the measured original clip width5.284UU plus clearance (5.6UU total); retain
original mesh/UV/material/size, do not mirror the mesh or guess PSA axis signs.
Publish the actual rendered open clip joint back as a bounded fresh
pawn/weapon/transaction-bound insertion witness. No witness => no insertion or
shell launch from a guessed point. Do not inflate the shoulder/insert radii.

Falsifiers: full root/controller affine does not roundtrip while native
LocalToWorld changes; clip shift is not controller-left at yaw/pitch/roll; a
stale/foreign socket completes insertion; original gun-child palettes leak;
Circle still needs an opening flick; original OFF behavior changes. Real game
lighting, clip fit/visibility, glove relationship, no drift and insertion remain
MAIN L3/PSVR2 L4 questions, not inferred from offline math.
