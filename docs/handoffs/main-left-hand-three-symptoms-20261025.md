# Left-hand carry, L2 support and Square FX: bounded physical diagnosis

Source base: `2fdb7934`. The 2026-10-25 PSVR2 session showed that the magazine
still blinked with `ReloadCarryTrackedContinuity=1`. Its carried phase kept
`prep=18`, `replay=2`, and `wrist=1` after the first frame. The wrist-continuity
hypothesis is therefore insufficient. Do not call that feature a fix.

This change adds default-OFF observation only, except that the already enabled
`TmdTrackedBeam` installer may now examine the owned component immediately on
the Square edge (up to 60 short polls). It does not change weapon geometry,
replay selection, TMD powers, input mapping or native ammunition.

For one controlled session enable:

```ini
[Diagnostic]
ReloadCarryRenderProbe=1
DualHandPoseProbe=1
TmdBeamAgeProbe=1
```

Keep the prior reviewed candidate flags and enable `TmdTrackedMuzzle=1` only
as a separately identified original `TMD_Muzzle` socket experiment. The log
distinguishes its hook from the aging-beam attachment hook.

Test without switching to chat: in gameplay, hold Square once and note the FX
origin; use L2 then left Grip on a long gun, release, use R2 then repeat the
same Grip; take a fresh magazine from the left shoulder, hold it visible for
two seconds, insert it, and close the game. MAIN archives the log and restores
the accepted DLL/INI with hash verification.

Interpretation:

- `visual-lock-miss`, absent `prepared`, or intermittent `submitted` receipts
  proves a CPU eligibility/dispatch gap before GPU presentation.
- Stable per-eye `submitted` and stable skin translation while the magazine
  blinks falsifies that explanation; investigate depth/alpha/coverage in the
  captured RT, with the logged exact texture, RT and depth surface identities.
- `dual-hand-pose` paired L2/R2 records distinguish a real source-palette or
  ride discontinuity from a Grip acquisition failure.
- Square census with no live `mAgingBeam` during the actual action rules out
  the current aging-beam installer as the origin correction. A successful
  `TMD_Muzzle` receipt identifies the alternate original producer.

Offline build and tests do not establish a headset fix. Physical acceptance
remains pending for all three symptoms.
