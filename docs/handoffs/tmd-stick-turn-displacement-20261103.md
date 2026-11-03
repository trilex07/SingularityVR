# TMD weapon detachment during turning — 2026-11-03

## Accepted baseline

User accepted installed CB43F6DC and authorized main promotion. Remote main was fast-forwarded from ecec5ebc to 752efc07. This does not declare the separately reported stick-turn detachment fixed.

Installed renderer verified CB43F6DCD1D8FD062981ECB840023432656AC5AD95F770C1D23D0CCE77A204BC. No Steam writes or launch in this follow-up.

## Observed contract failure

Archived actual session: build/tmd-stick-turn/evidence/accepted-cb43-session.log (private). At frame 7607 the exact Valkyrie body (11387 vertices, audited six-bone palette) rejects Correct() at displacement despite a fresh wrist (15 ms, preceding frame) and matching reference. The failure forwards the native head/animation-owned palette. Source-root/local/ride/raw-controller/normal-wrist/normal-controller/attachment matrices were recorded in that receipt.

Reconstruction: length(delta.translation) = 258.9090 UU; distance(targetSkin.origin, sourceRoot.origin) = 211.9304 UU. The old test incorrectly treated the translation coefficient of inverse(root)*targetSkin as point displacement. For a rotating rigid transform this coefficient also contains pivot compensation. It is not the displacement of the root being corrected.

Minimal change: evaluate the existing 256 UU bound on targetSkin.origin minus sourceRoot.origin in their common skin space. No threshold increase, guessed offsets, new aim algorithm, tracking bypass, palette ownership or input changes. All identity, freshness, finite/rigid, active-bone and final-witness checks remain. Every active bone receives the same previously accepted rigid delta exactly once.

The exact captured production fixture now reaches final-witness with maximum matrix error 0.000008. A genuinely excessive 2000 UU target displacement still rejects transactionally without altering output. This PROVES the captured rejection defect. Its complete physical effect during all stick turns remains PENDING; do not infer physical PASS from the fixture.

## Verification and next physical gate

Run tools/tests/test-dual-weapon-tmd.ps1 (including OLED pixels), optimized x86 build and compiled WeaponLocker ABI regression. Record exact outputs/build identity in build/tmd-stick-turn/candidate.json.

One targeted PSVR2 check when available: Valkyrie then Volk, Impulse L2, release L2, hold right controller steady and turn the stick both directions, including a full turn; move the right hand and verify the weapon remains attached. Briefly verify support Grip. No new test of unrelated accepted systems is required. If it fails, inspect first rejection gate and its captured matrices; do not widen thresholds speculatively.
