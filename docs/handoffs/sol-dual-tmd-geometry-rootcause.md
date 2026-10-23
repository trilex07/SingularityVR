# Dual-TMD geometry and support-grip follow-up (SIDE)

Base `a2244f4`; known-good standalone short-gauntlet source `7e7d8208`;
integration source `d771c40`. This branch is offline-only. The user physically
rejected `a2244f4`; a zero TMD `gunRide` count falsified the previous ride-only
explanation. No Steam files or saves were touched here.

## Evidence and first violated contract

The failed runtime log records `mPlayerHandsStatus=1` (TMD raised), then a real
`two-hand grip: ACQUIRED` transition and `support=1`. At that transition the
compiled presentation policy logs `authored-pair gun=8297 leftDelta=primary`.
The source applies the authored pair to TMD material-0, not the original
weapon-hands pair. A common rigid delta preserves the source TMD hand pose; it
cannot relocate that independently raised hand to the shotgun fore-end. This
is a proven *pose-policy mismatch*, consistent with the failed support image.
For the non-pistol TMD-pose only, the existing weapon-specific fore-end target
is now used. The normal HANDS_WEAPON authored pair and the accepted Centurion
close-support policy are unchanged. Physical placement still needs L4.

The original private PSK and cooked index order establish three native TMD
materials: material 0 has 4402 glove faces in three compact surface islands;
material 1 has 6527 device faces plus 1177 arm/sleeve faces; material 2 has 17
right-forearm faces. The SVTG asset has 7110 faces (device + short original
sleeve + cap) and excludes the measured 280-face long forearm island. The
failed log has a successful two-eye SVTG receipt, but that receipt is printed
once and does not prove every later mixed draw took that route. The log's
`first failed gate=buffer` during Grip was misleading: support intentionally
set `nSeg=0` before SVTG eligibility; it was not proof of GPU allocation
failure. That diagnostic is now suppressed for the intentional hide.

The extended geometry's exact submitting draw is **not yet proven**: the two
user screenshots were not available as local image files in this SIDE checkout,
and the existing census stops before final index dispatch. The new default-OFF
probe records one receipt per state/material/outcome, with frame, native hands
state, support, original index range, palette result, segment count, and stereo
eye draw count, and source IB/VB identities. Outcomes are `hidden`, `svtg`,
`hardware`, `native-full`, `zero`. `native-full` on material 0 can also mean
the existing cuff-index path; on material 1 it is the unfiltered arm risk.
It is bounded to at most 90 one-shot lines and piggybacks on the existing
5400-frame probe budget; no per-draw filesystem or GPU readback was added.

## Narrow safety correction

In focused dual gameplay with the short-gauntlet flag enabled, the exact
material-1 source range (`min=2626`, `verts=4818/4781`, `first=13206`,
`tris=7704`) and original five-bone shader cannot draw its full 7704 faces
when `PrepareIndependentLeft` returns no wrist palette. It draws zero instead.
If the palette is valid, the existing 7110-face SVTG or 49-run hardware-only
fallback remains authoritative. Native animation, shadow, cinema, world draws,
flag OFF and other materials are outside this guard. This prevents one
structurally identified route to the long arm; it does **not** claim the
observed stretched pixels are proven fixed.

`d771c40` combined input ownership with the independent gauntlet but left the
TMD-pose glove using the weapon-hands authored-pair rule. `a2244f4` fixed the
separate exact TMD gun-ride conflict; the user's later `gunRide=0` result shows
that correction was insufficient. Neither prior candidate measured the final
per-material index route while the visual defect occurred.

## Verification and acceptance boundary

- L1: dual policy/wiring, native geometry/index identity, SVTG two-eye draw and
  49-run fallback tests passed. GPU device creation in the restricted sandbox
  failed; the same standalone D3D9/D3D9Ex regressions passed with GPU access.
- L2: production x86 PE32 `/O2` build passed; compiled WeaponLocker factory
  `[eax+20Ch]` ABI regression passed. SHA recorded in build provenance.
- L3/L4: **NOT RUN**. Do not merge or install based on L1/L2 alone.

MAIN's one short controlled session after review: weapon right, TMD left,
rotate left wrist; acquire/release Left Grip near Volk; raise/lower TMD; check
both eyes for short sleeve and no stretched forearm. Preserve the accepted
installation for rollback. If the long shape remains, use the bounded
`dual-tmd route` rows to identify the exact material and final route before
another geometry change. Check R2/L2, weapon switch/holster, FPS and stereo.
