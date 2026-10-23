# L2 weapon visibility: local-owner gate correction

Base: `514bb03`. The physical test still lost the right-hand Volk on L2;
the user clarified that the right glove remains near the body. This is not a
claim that the hand and weapon share one failed draw.

The complete current `view_matrix.log` has SHA256
`9A70040E720A00A98B5D5E3F420E542E5FEA21D8D3BD01DBD4D45D2FAF2D93A9`.
It records `DualWeaponTmdGunScale=1` and a bounded refusal at line 1051:
`local camera-owned pawn unavailable`; it has no `ready` or `scale=0 -> 1`
receipt. The same log earlier/later records a reciprocal local pawn through
the already existing startup/native-player resolver (lines 618, 3308). Thus
the previous candidate did not demonstrate an applied scale override. This
does not yet prove that gun scale is the only cause of invisibility.

Hypothesis: the older inventory camera reflection fails in this live session
while the separate reciprocal native-player path succeeds. Falsifier: the new
bounded owner-source receipt reports both unavailable/disagreeing, or a `ready`
and `scale=0 -> 1` receipt appears while the gun still vanishes. In the latter
case investigate the subsequent five-bone palette and foreground route, not
another guessed scale.

The hook now accepts the already audited native reciprocal owner when the
inventory camera owner is absent. If both exist but disagree, it refuses.
Live control, mesh, class, getter, caller, thread and VR context checks are
unchanged. A bounded context-transition receipt distinguishes no poll from
a context gate, and an owner-source receipt distinguishes absence from
disagreement. These diagnostics do not write the pawn or graphics state.

Offline: x86 MinHook fixture passed, including the exact missing-camera and
conflicting-owner cases; PE32 `/O2` build and emitted hook ABI regression
passed. Steam/PSVR2 visual behavior remains **NOT RUN** for this revision.
