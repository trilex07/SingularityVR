# Centurion cylinder size — SIDE, 2026-10-16

Physical feedback: spare cylinder is much larger and does not resemble the one
in the gun. No Steam files/processes touched. Keep current MAIN integration;
apply this narrow follow-up, NOT the older standalone SIDE DLL over newer MAIN.

## Evidence / smallest change

Private original `ch_inview_alt_pistol_mesh.psk`, SHA256
`4E18A25D5241DE1B3F18100E3BD9373D464EE16BAF3E29D7C60289AE18FE73D4`:
5799 wedges, original rigid `clip` bone #57, 1067 exported UV vertices and 1252
triangles; zero crossing triangles, original material #0. Cache SVRK SHA256
`F21961145D6645EE93A8A2C73AE89380535D918352BABB1FD4F811C67422D89B`,
original AR_Pistol_d albedo cache SHA256
`17A3355AD6CC7E2F35A03883FDF4B314577FEE908773262C5BC199C006C6CD60`.
The existing model is NOT a generated replacement. Preserve both files exactly.

Export bounds: 0.1006398 x 0.1000984 x 0.1258195 metres at 52.5 UU/metre.
Native gun applies `GunScalePct` in `ApplyMeshScale` and converts the world using
`MetresToUU()`; the detached renderer previously applied neither. Therefore the
proper export-to-tracking factor is `(52.5 / MetresToUU()) * GunScalePct/100`.
At 75% gun /80% world this is **0.6**, yielding approximately 60.38 x 60.06 x
75.49 mm. The previous spare was 1.667 times larger in every axis.

Use that factor live for the shoulder stock AND carried spare. Scale only local
geometry about the original clip bone; never hand/shoulder offsets, UVs, draw
pose, gestures, ammo or native gun. No private asset regeneration required.
Shared knife renderer gets an optional modelScale=1 argument: all knife callers
retain identity. No new flag: the existing default-OFF CenturionClipPreview owns
this path. Invalid calibration refuses the preview without mutating gameplay.

Falsifier: if the original spare with matching scale is still a different shape,
compare the actual MAIN-loaded SVRK/SVRT identities and native cylinder draw;
do not guess another mesh or add a separate visual size multiplier.

## Limits and handoff

This fixes measured geometry scale, not the complete appearance. Preview is
still original albedo-only, NOT the native UE3 normal/specular/shadow material.
Physical material matching remains OPEN, not silently claimed from source/hash.
No speculative lighting tint, replacement art, inventory/animation/input change.

L1: test-centurion-clip checks original-vertex equivalence at 31 world scales x
76 gun scales, live calibration/invalid numbers, existing mount/lifecycle/fault
tests; the existing full reload/knife tests must also remain green.
L2: optimized x86 build and exact receipt below. L3/L4 UNKNOWN.
MAIN: preserve accepted config/assets, rebuild current integration; log clip
scale alongside current GunScalePct/MetresToUU. In PSVR2 compare the fresh spare
beside the Centurion cylinder, rotate both hands, insert normally; verify knife
size unchanged. One route, no mid-headset chat switch. If appearance still differs,
identify actual material passes before another shader/material patch.
